#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <locale.h>

#include "mainwin.h"

enum exp_field_t {
    EXP_FIELD_EXPID,
    EXP_FIELD_DATE,
    EXP_FIELD_DESC,
    EXP_FIELD_AMT,
    EXP_FIELD_CATID,
    EXP_FIELD_CATNAME,
    EXP_FIELD_NCOLS
};

typedef struct {
    sqlite3 *db;
    str_t *xpfile;
    GtkWidget *win;
} ctx_t;

static GtkWidget *create_menubar(ctx_t *ctx);
static GtkWidget *create_treeview();
static void refresh_treeview(GtkTreeView *tv, array_t *xps);
static void set_treeview_row(GtkListStore *m, GtkTreeIter *it, exp_t *xp);

static void amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);
static void date_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);

static void expense_open(GtkWidget *w, ctx_t *ctx);

GtkWidget *mainwin_new(char *xpfile) {
    GtkWidget *win;
    GtkWidget *vbox;
    GtkWidget *mb;
    GtkWidget *tv;
    ctx_t *ctx;
    sqlite3 *db;

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Expense Buddy");
    gtk_window_set_default_size(GTK_WINDOW(win), 480, 480);

    ctx = malloc(sizeof(ctx_t));
    ctx->db = NULL;
    ctx->xpfile = str_new(0);
    ctx->win = win;

    if (xpfile != NULL && open_expense_file(xpfile, &db) == 0) {
        ctx->db = db;
        str_assign(ctx->xpfile, xpfile);
    }

    mb = create_menubar(ctx);
    tv = create_treeview();

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), mb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tv, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    return win;
}

static GtkWidget *create_menubar(ctx_t *ctx) {
    GtkWidget *mb;
    GtkWidget *m;
    GtkWidget *mi_expense, *mi_expense_new, *mi_expense_open, *mi_sep1, *mi_expense_add, *mi_expense_edit, *mi_expense_delete, *mi_sep2, *mi_expense_about, *mi_expense_quit;

    mb = gtk_menu_bar_new();
    m = gtk_menu_new();
    mi_expense = gtk_menu_item_new_with_mnemonic("_Expense");
    mi_expense_new = gtk_menu_item_new_with_mnemonic("New Expense File");
    mi_expense_open = gtk_menu_item_new_with_mnemonic("_Open Expense File");
    mi_sep1 = gtk_separator_menu_item_new();
    mi_expense_add = gtk_menu_item_new_with_mnemonic("_Add new Expense");
    mi_expense_edit = gtk_menu_item_new_with_mnemonic("_Edit selected Expense");
    mi_expense_delete = gtk_menu_item_new_with_mnemonic("_Delete selected Expense");
    mi_sep2 = gtk_separator_menu_item_new();
    mi_expense_about = gtk_menu_item_new_with_mnemonic("About");
    mi_expense_quit = gtk_menu_item_new_with_mnemonic("_Quit");

    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_sep1);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_add);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_edit);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_delete);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_sep2);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_about);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_quit);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_expense), m);
    gtk_menu_shell_append(GTK_MENU_SHELL(mb), mi_expense);

    g_signal_connect(mi_expense_open, "activate", G_CALLBACK(expense_open), ctx);

    return mb;
}

static void expense_open(GtkWidget *w, ctx_t *ctx) {
    GtkWidget *dlg;
    gchar *xpfile = NULL;
    sqlite3 *db;
    int z;

    dlg = gtk_file_chooser_dialog_new("Open Expense File", GTK_WINDOW(ctx->win),
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      "Open", GTK_RESPONSE_ACCEPT,
                                      "Cancel", GTK_RESPONSE_CANCEL,
                                      NULL);
    z = gtk_dialog_run(GTK_DIALOG(dlg));
    if (z != GTK_RESPONSE_ACCEPT)
        goto exit;

    xpfile = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    if (xpfile == NULL)
        goto exit;

    z = open_expense_file(xpfile, &db);
    if (z != 0)
        goto exit;

    ctx->db = db;
    str_assign(ctx->xpfile, xpfile);

exit:
    if (xpfile != NULL)
        g_free(xpfile);
    gtk_widget_destroy(dlg);
}

static GtkWidget *create_treeview() {
    GtkWidget *tv;
    GtkWidget *sw;
    GtkCellRenderer *r;
    GtkTreeViewColumn *col;
    GtkListStore *ls;
    int xpadding = 10;
    int ypadding = 2;

    tv = gtk_tree_view_new();
    g_object_set(tv, "enable-search", FALSE, NULL);
    gtk_widget_add_events(tv, GDK_KEY_PRESS_MASK);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Date", r, "text", EXP_FIELD_DATE, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_DATE);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_column_set_cell_data_func(col, r, date_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Description", r, "text", EXP_FIELD_DESC, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_DESC);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(col, 200);
    gtk_tree_view_column_set_expand(col, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Amount", r, "text", EXP_FIELD_AMT, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_AMT);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_cell_renderer_set_alignment(r, 1.0, 0);
    gtk_tree_view_column_set_cell_data_func(col, r, amt_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", EXP_FIELD_CATNAME, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_CATNAME);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(EXP_FIELD_NCOLS,
                            G_TYPE_UINT,   // EXP_FIELD_ROWID
                            G_TYPE_LONG,   // EXP_FIELD_DATE
                            G_TYPE_STRING, // EXP_FIELD_DESC
                            G_TYPE_DOUBLE, // EXP_FIELD_AMT
                            G_TYPE_UINT,   // EXP_FIELD_CATID
                            G_TYPE_STRING  // EXP_FIELD_CATNAME
                            );
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);

    return tv;
}
static void amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    gdouble amt;
    char buf[15];

    gtk_tree_model_get(m, it, EXP_FIELD_AMT, &amt, -1);
    snprintf(buf, sizeof(buf), "%'9.2f", amt);
    g_object_set(r, "text", buf, NULL);
}
static void date_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    date_t *dt;
    time_t time;
    char buf[30];
    char *fmt;

    gtk_tree_model_get(m, it, EXP_FIELD_DATE, &time, -1);
    dt = date_new(time);

    fmt = "%Y-%m-%d";
    //fmt = "%b %d";
    //fmt = "%a %d";

    date_strftime(dt, fmt, buf, sizeof(buf));
    g_object_set(r, "text", buf, NULL);
    date_free(dt);
}
static void refresh_treeview(GtkTreeView *tv, array_t *xps) {
    GtkListStore *ls;
    GtkTreeIter it;
    exp_t *xp;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
    assert(ls != NULL);
    gtk_list_store_clear(ls);

    for (int i=0; i < xps->len; i++) {
        xp = xps->items[i];
        gtk_list_store_append(ls, &it);
        set_treeview_row(ls, &it, xp);
    }
}
static void set_treeview_row(GtkListStore *m, GtkTreeIter *it, exp_t *xp) {
    gtk_list_store_set(m, it, 
                       EXP_FIELD_EXPID, xp->expid,
                       EXP_FIELD_DATE, xp->date->time,
                       EXP_FIELD_DESC, xp->desc->s,
                       EXP_FIELD_AMT, xp->amt,
                       EXP_FIELD_CATID, xp->catid,
                       EXP_FIELD_CATNAME, xp->catname->s,
                       -1);
}


