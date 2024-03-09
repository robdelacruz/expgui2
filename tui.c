#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"
#include "tui.h"
#include "listxp.h"
#include "editxp.h"

char *longtext = "This is some text that took me a long time to write.";

clr_t titlefg;
clr_t titlebg;
clr_t statusfg;
clr_t statusbg;
clr_t shadowfg;
clr_t shadowbg;

clr_t textfg;
clr_t textbg;
clr_t highlightfg;
clr_t highlightbg;
clr_t colfg;
clr_t colbg;
clr_t popupfg;
clr_t popupbg;

clr_t editfieldfg;
clr_t editfieldbg;
clr_t btnfg;
clr_t btnbg;

uint statusx=0, statusy=23;

char *xp_col_names[XP_COL_COUNT] = {"Description", "Amount", "Category", "Date"};
int xp_entry_maxchars[XP_COL_COUNT] = {25, 9, 10, 10};
sqlite3 *_db;
array_t *cats=0;

static void set_output_mode(int mode);
static void update(struct tb_event *ev);
static void draw();
static void draw_shell();

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ESC);
//    set_output_mode(TB_OUTPUT_256);
    set_output_mode(TB_OUTPUT_NORMAL);
    tb_set_clear_attrs(textfg, textbg);

    statusx = 0;
    statusy = tb_height()-1;

    _db = db;
    cats = array_new(0);
    listxp_xps = array_new(0);
    editxp_xp = exp_new();

    db_select_exp(db, listxp_xps);
    db_select_cat(db, cats);

    init_entry(&editxp_entry_desc, "", xp_entry_maxchars[XP_COL_DESC]);
    init_entry(&editxp_entry_amt, "", xp_entry_maxchars[XP_COL_AMT]);
    init_entry(&editxp_entry_date, "", xp_entry_maxchars[XP_COL_DATE]);
    editxp_listbox_cat.ipos = 0;
    editxp_listbox_cat.iscrollpos = 0;
    editxp_listbox_cat.height = 5;
    editxp_listbox_cat.width = xp_entry_maxchars[XP_COL_CAT];

    editxp_focus[EDITXP_FOCUS_DESC].prev   = EDITXP_FOCUS_SAVE;
    editxp_focus[EDITXP_FOCUS_DESC].next   = EDITXP_FOCUS_AMT;
    editxp_focus[EDITXP_FOCUS_AMT].prev    = EDITXP_FOCUS_DESC;
    editxp_focus[EDITXP_FOCUS_AMT].next    = EDITXP_FOCUS_CAT;
    editxp_focus[EDITXP_FOCUS_CAT].prev    = EDITXP_FOCUS_AMT;
    editxp_focus[EDITXP_FOCUS_CAT].next    = EDITXP_FOCUS_DATE;
    editxp_focus[EDITXP_FOCUS_DATE].prev   = EDITXP_FOCUS_CAT;
    editxp_focus[EDITXP_FOCUS_DATE].next   = EDITXP_FOCUS_SAVE;
    editxp_focus[EDITXP_FOCUS_SAVE].prev   = EDITXP_FOCUS_DATE;
    editxp_focus[EDITXP_FOCUS_SAVE].next   = EDITXP_FOCUS_DESC;

    tb_clear();
    print_text(longtext, statusx,statusy, tb_width(), TB_DEFAULT,TB_DEFAULT);
    draw();
    tb_present();

    while (1) {
        tb_poll_event(&ev);
        if (ev.key == TB_KEY_CTRL_C) 
            break;
        if (ev.ch == 'q') 
            break;

        update(&ev);
        draw();
    }

    tb_shutdown();

    array_free(listxp_xps);
    exp_free(editxp_xp);
}

static void set_output_mode(int mode) {
    if (mode == TB_OUTPUT_256) {
        titlefg = 16;
        titlebg = 14;
        statusfg = 254;
        statusbg = 245;
        shadowfg = 235;
        shadowbg = 235;

        textfg = 15;
        textbg = 19;
        highlightfg = 16;
        highlightbg = 14;
        colfg = 11;
        colbg = textbg;
        popupfg = 236;
        popupbg = 252;

        editfieldfg = highlightfg|TB_BOLD;
        editfieldbg = highlightbg;
        btnfg = popupfg;
        btnbg = popupbg;
    } else {
        titlefg = TB_WHITE;
        titlebg = TB_BLUE;
        statusfg = TB_YELLOW;
        statusbg = TB_BLUE;
        shadowfg = TB_BLACK;
        shadowbg = TB_BLACK;

        textfg = TB_WHITE;
        textbg = TB_BLUE;
        highlightfg = TB_BLACK;
        highlightbg = TB_CYAN;
        colfg = TB_YELLOW | TB_BOLD;
        colbg = textbg;
        popupfg = TB_BLACK;
        popupbg = TB_WHITE;

        editfieldfg = TB_BLACK;
        editfieldbg = TB_YELLOW;
        btnfg = TB_BLACK;
        btnbg = TB_YELLOW;
    }
    tb_set_output_mode(mode);
}

static void printf_status(const char *fmt, ...) {
    char buf[4096];

    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    tb_printf(statusx, statusy, textfg,textbg, buf);
    va_end(vl);
}

static void update(struct tb_event *ev) {
    if (editxp_show) {
        update_editxp(ev);
        return;
    }
    update_listxp(ev);
}

static void draw() {
    tb_clear();

    draw_shell();
    draw_listxp();
    if (editxp_show) {
        draw_editxp();
    }

    tb_present();
}

static void draw_shell() {
    print_text(" Expense Buddy Console", 0,0, tb_width(), titlefg | TB_BOLD, titlebg);
}

