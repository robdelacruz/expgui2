#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "clib.h"
#include "db.h"
#include "tuilib.h"
#include "tui.h"

static void update(tui_t *t, struct tb_event *ev) {
    if (t->editxp_show) {
        update_editxp(t, ev);
        return;
    }
    update_listxp(t, ev);
}

static void draw(tui_t *t) {
    tb_clear();

    print_text(" Expense Buddy Console", 0,0, tb_width(), titlefg | TB_BOLD, titlebg);
    draw_listxp(t);
    if (t->editxp_show) {
        draw_editxp(t);
    }

    tb_present();
}

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;
    tui_t t;

    tb_init();
    tb_set_input_mode(TB_INPUT_ESC);
//    set_output_mode(TB_OUTPUT_256);
    set_output_mode(TB_OUTPUT_NORMAL);
    tb_set_clear_attrs(textfg, textbg);


    t.db = db;
    t.cats = array_new(0);
    t.statusx = 0;
    t.statusy = tb_height()-1;

    t.xp_col_names[XP_COL_DESC] = "Description";
    t.xp_col_names[XP_COL_AMT]  = "Amount";
    t.xp_col_names[XP_COL_CAT]  = "Category";
    t.xp_col_names[XP_COL_DATE] = "Date";
    t.xp_col_maxchars[XP_COL_DESC]  = 25;
    t.xp_col_maxchars[XP_COL_AMT]   = 9;
    t.xp_col_maxchars[XP_COL_CAT]   = 10;
    t.xp_col_maxchars[XP_COL_DATE]  = 10;

    t.listxp_xps = array_new(0);
    t.listxp_ixps = 0;
    t.listxp_scrollpos = 0;

    t.editxp_show = 0;
    t.editxp_xp = exp_new();;
    t.editxp_ifocus = 0;
    t.editxp_is_focus_active = 0;
    init_entry(&t.editxp_entry_desc, "", t.xp_col_maxchars[XP_COL_DESC]);
    init_entry(&t.editxp_entry_amt, "", t.xp_col_maxchars[XP_COL_AMT]);
    init_entry(&t.editxp_entry_date, "", t.xp_col_maxchars[XP_COL_DATE]);
    t.editxp_listbox_cat.ipos = 0;
    t.editxp_listbox_cat.iscrollpos = 0;
    t.editxp_listbox_cat.height = 5;
    t.editxp_listbox_cat.width = t.xp_col_maxchars[XP_COL_CAT];

    t.editxp_focus[EDITXP_FOCUS_DESC].prev   = EDITXP_FOCUS_SAVE;
    t.editxp_focus[EDITXP_FOCUS_DESC].next   = EDITXP_FOCUS_AMT;
    t.editxp_focus[EDITXP_FOCUS_AMT].prev    = EDITXP_FOCUS_DESC;
    t.editxp_focus[EDITXP_FOCUS_AMT].next    = EDITXP_FOCUS_CAT;
    t.editxp_focus[EDITXP_FOCUS_CAT].prev    = EDITXP_FOCUS_AMT;
    t.editxp_focus[EDITXP_FOCUS_CAT].next    = EDITXP_FOCUS_DATE;
    t.editxp_focus[EDITXP_FOCUS_DATE].prev   = EDITXP_FOCUS_CAT;
    t.editxp_focus[EDITXP_FOCUS_DATE].next   = EDITXP_FOCUS_SAVE;
    t.editxp_focus[EDITXP_FOCUS_SAVE].prev   = EDITXP_FOCUS_DATE;
    t.editxp_focus[EDITXP_FOCUS_SAVE].next   = EDITXP_FOCUS_DESC;

    db_select_exp(db, t.listxp_xps);
    db_select_cat(db, t.cats);

    tb_clear();
    draw(&t);
    tb_present();

    while (1) {
        tb_poll_event(&ev);
        if (ev.key == TB_KEY_CTRL_C) 
            break;
        if (ev.ch == 'q') 
            break;

        update(&t, &ev);
        draw(&t);
    }

    tb_shutdown();

    array_free(t.listxp_xps);
    array_free(t.cats);
    exp_free(t.editxp_xp);
}

void printf_status(tui_t *t, const char *fmt, ...) {
    char buf[4096];

    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    tb_printf(t->statusx, t->statusy, textfg,textbg, buf);
    va_end(vl);
}

