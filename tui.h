#ifndef TUI_H
#define TUI_H

#include <stdlib.h>
#include <stdio.h>
#include "termbox2.h"
#include "clib.h"
#include "tuilib.h"

enum {
    XP_COL_DESC=0,
    XP_COL_AMT,
    XP_COL_CAT,
    XP_COL_DATE,
    XP_COL_COUNT
};

enum {
    EDITXP_FOCUS_DESC=0,
    EDITXP_FOCUS_AMT,
    EDITXP_FOCUS_CAT,
    EDITXP_FOCUS_DATE,
    EDITXP_FOCUS_SAVE,
    EDITXP_FOCUS_COUNT
};

typedef struct {
    sqlite3*    db;
    array_t*    cats;
    uint        statusx, statusy;
    char*       xp_col_names[XP_COL_COUNT];
    int         xp_col_maxchars[XP_COL_COUNT];

    panel_t     listxp;
    array_t*    listxp_xps;
    int         listxp_ixps;
    int         listxp_scrollpos;

    panel_t     editxp;
    int         editxp_show;
    exp_t*      editxp_xp;
    focus_t     editxp_focus[EDITXP_FOCUS_COUNT];
    int         editxp_ifocus;
    int         editxp_is_focus_active;
    entry_t     editxp_entry_desc;
    entry_t     editxp_entry_amt;
    entry_t     editxp_entry_date;
    listbox_t   editxp_listbox_cat;
} tui_t;

void tui_start(sqlite3 *db, char *dbfile);
void printf_status(tui_t *t, const char *fmt, ...);

exp_t *listxp_selected_expense(tui_t *t);
void draw_listxp(tui_t *t);
void update_listxp(tui_t *t, struct tb_event *ev);

void init_editxp(tui_t *t, exp_t *xp);
void draw_editxp(tui_t *t);
void update_editxp(tui_t *t, struct tb_event *ev);

#endif

