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

static void setupui(GtkWidget *win);

GtkWidget *mainwin_new() {
    GtkWidget *win;

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Expense Buddy");
    gtk_window_set_default_size(GTK_WINDOW(win), 480, 480);

    setupui(win);
    gtk_widget_show_all(win);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    return win;
}

static void setupui(GtkWidget *win) {
}

