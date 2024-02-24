#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

char *longtext = "This is some text that took me a long time to write.";

clr_t boxfg = TB_GREEN;
clr_t boxbg = TB_BLACK;
clr_t textfg = TB_GREEN;
clr_t textbg = TB_BLACK;
clr_t textselfg = TB_BLACK;
clr_t textselbg = TB_WHITE;

rect_t screen = {0,0, 80,23};
uint statusx=0, statusy=23;
array_t *xps=0;
int sel_xps=0;
exp_t *selxp = NULL;

rect_t xpview_area = {1,1, 44,23};
rect_t xpdetails_area = {46,1, 33,7};

static void update(struct tb_event *ev);
static void draw();
static void draw_frame();
static void draw_xpview();
static void draw_xpdetail(exp_t *xp);

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ALT);
    tb_set_clear_attrs(textfg, textbg);

    screen.x = 0;
    screen.y = 0;
    screen.width = tb_width();
    screen.height = tb_height()-1;
    statusx = 0;
    statusy = tb_height()-1;

    xps = array_new(0);
    db_select_exp(db, xps);
    if (xps->len > 0)
        selxp = xps->items[sel_xps];

    tb_clear();
    print_text(longtext, statusx,statusy, screen.width, TB_DEFAULT,TB_DEFAULT);
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
}

static void update(struct tb_event *ev) {
    if (xps->len == 0)
        return;

    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
        sel_xps--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
        sel_xps++;

    if (sel_xps < 0)
        sel_xps=0;
    if (sel_xps > xps->len-1)
        sel_xps = xps->len-1;

    assert(sel_xps >= 0 && sel_xps < xps->len);
    selxp = xps->items[sel_xps];
}

static void draw() {
    tb_clear();
    draw_frame();
    draw_xpview();
    draw_xpdetail(selxp);

    tb_present();
}
static void draw_frame() {
    char *asc = ASC_BLOCK_LOW;
    clr_t fg = boxfg;
    clr_t bg = boxbg;

    draw_ch_horz(asc, 0,0, 80, fg,bg);
    draw_ch_horz(asc, 0,24, 80, fg,bg);
    draw_ch_horz(asc, 45,8, 34, fg,bg);
    draw_ch_vert(asc, 0,0, 25, fg,bg);
    draw_ch_vert(asc, 79,0, 25, fg,bg);
    draw_ch_vert(asc, 45,0, 25, fg,bg);

    draw_clear(xpview_area.x, xpview_area.y, xpview_area.width, xpview_area.height, TB_YELLOW);
    draw_clear(xpdetails_area.x, xpdetails_area.y, xpdetails_area.width, xpdetails_area.height, TB_CYAN);
}
static void draw_xpview() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];

    clr_t fg, bg;
    int x = xpview_area.x;
    int y = xpview_area.y;
    int row = 0;

    for (int i=0; i < xps->len; i++) {
        if (row > xpview_area.height-1)
            break;
        xp = xps->items[i];
        if (row == sel_xps) {
            fg = textselfg;
            bg = textselbg;
        } else {
            fg = textfg;
            bg = textbg;
        }

        tb_set_cell(x,y, ASC_SPACE, fg,bg);
        x++;
        print_text(xp->desc->s, x,y, 15, fg,bg);
        x+=15;
        tb_set_cell(x,y, ASC_SPACE, fg,bg);
        x++;
        tb_printf(x,y, fg,bg, "%9.2f", xp->amt);
        x+=9;
        tb_set_cell(x,y, ASC_SPACE, fg,bg);
        x++;
        //print_text(xp->catname->s, x,y, 10, fg,bg);
        print_text("Household", x,y, 10, fg,bg);
        x+=10;
        tb_set_cell(x,y, ASC_SPACE, fg,bg);
        x++;
        date_strftime(xp->date, "%m-%d", bufdate, sizeof(bufdate));
        print_text(bufdate, x,y, 5, fg,bg);
        x+=5;
        tb_set_cell(x,y, ASC_SPACE, fg,bg);
        x++;
        assert(x == 45);

        y++;
        x = xpview_area.x;
        row++;
    }

}
static void draw_xpdetail(exp_t *xp) {
    clr_t fg, bg;
    int x = xpdetails_area.x;
    int y = xpdetails_area.y;
    int row = 0;
    char bufdate[ISO_DATE_LEN+1];

    int lbl_len, val_len;
    char *lbldesc = " Desc: ";
    char *lblamt  = " Amt : ";
    char *lblcat  = " Cat : ";
    char *lbldate = " Date: ";

    if (xp == NULL)
        return;

    fg = textfg;
    bg = textbg;
    lbl_len = strlen(lbldesc);
    val_len = xpdetails_area.width - lbl_len;

    print_text(lbldesc, x,y, lbl_len, fg,bg);
    print_text(xp->desc->s, x+lbl_len,y, val_len, fg,bg); 
    y++;

    print_text(lblamt, x,y, lbl_len, fg,bg);
    tb_printf(x+lbl_len,y, fg,bg, "%9.2f", xp->amt);
    y++;

    print_text(lblcat, x,y, lbl_len, fg,bg);
    //print_text(xp->catname->s, x+lbl_len,y, val_len, fg,bg);
    print_text("Household", x+lbl_len,y, val_len, fg,bg);
    y++;

    print_text(lbldate, x,y, lbl_len, fg,bg);
    date_to_iso(xp->date, bufdate, sizeof(bufdate));
    print_text(bufdate, x+lbl_len,y, val_len, fg,bg);

    y++;
}


