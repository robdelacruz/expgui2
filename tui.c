#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

#define CLR_FG TB_GREEN
#define CLR_BG TB_BLACK
#define BOX_FG TB_WHITE
#define BOX_BG TB_BLUE
#define CLR_HIGHLIGHT_FG TB_BLACK
#define CLR_HIGHLIGHT_BG TB_WHITE

char *longtext = "This is some text that took me a long time to write.";

clr_t textfg = TB_GREEN;
clr_t textbg = TB_BLACK;

clr_t xpsfg = TB_WHITE;
clr_t xpsbg = TB_BLUE;
clr_t xpsselfg = TB_BLACK;
clr_t xpsselbg = TB_WHITE;

rect_t screen = {0,0, 80,23};
uint statusx=0, statusy=23;
array_t *xps=0;
rect_t xpsrect;
rect_t xpsrect_box;
int sel_xps=0;

static void update(struct tb_event *ev);
static void draw();

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ALT);
    tb_set_clear_attrs(CLR_FG, CLR_BG);

    screen.x = 0;
    screen.y = 0;
    screen.width = tb_width();
    screen.height = tb_height()-1;
    statusx = 0;
    statusy = tb_height()-1;

    xpsrect_box.x = 0;
    xpsrect_box.y = 0;
    xpsrect_box.width = 80;
    xpsrect_box.height = 25;

    xpsrect.x = xpsrect_box.x+1;
    xpsrect.y = xpsrect_box.y+1;
    xpsrect.width = xpsrect_box.width-2;
    xpsrect.height = xpsrect_box.height-2;

    xps = array_new(0);
    db_select_exp(db, xps);

    tb_clear();
    print_text(longtext, statusx,statusy, screen.width, 1, TB_DEFAULT,TB_DEFAULT);
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
        sel_xps= 0;
    if (sel_xps > xps->len-1)
        sel_xps = xps->len-1;
}

static void draw() {
    exp_t *xp;

    tb_clear();
    draw_box_fill(xpsrect_box.x, xpsrect_box.y, xpsrect_box.width, xpsrect_box.height, xpsfg,xpsbg);

    int row = 0;
    clr_t fg, bg;
    uint x = xpsrect.x;
    uint y = xpsrect.y;


    for (int i=0; i < xps->len; i++) {
        if (row > xpsrect.height-1)
            break;
        xp = xps->items[i];
        if (row == sel_xps) {
            fg = xpsselfg;
            bg = xpsselbg;
        } else {
            fg = xpsfg;
            bg = xpsbg;
        }

        print_text(xp->desc->s, x, y+row, 25, 1, fg,bg);
        tb_printf(xpsrect.x+25, y+row, fg,bg, "%9.2f", xp->amt);
        row++;
    }

    tb_present();
}




