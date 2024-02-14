#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "clib.h"
#include "db.h"

typedef struct {
    uint x;
    uint y;
    uint width;
    uint height;
} rect_t;

typedef struct {
    rect_t rect;
    int sel;
    int scrollpos;
} listbox_t;

static void update_listbox(listbox_t *lb, array_t *xps, struct tb_event *ev);
static void draw_listbox(listbox_t *lb, array_t *xps);
static void draw_listbox_row(listbox_t *lb, char *s, int rownum, uintattr_t fg, uintattr_t bg);

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ALT);
    tb_set_clear_attrs(TB_WHITE, TB_BLUE);

    rect_t lbrect = {1,0, tb_width()-2, tb_height()-1};
    listbox_t lb = {lbrect, 0, 0};
    array_t *xps = array_new(0);
    db_select_exp(db, xps);

    tb_clear();
    tb_printf(0, tb_height()-1, TB_DEFAULT,TB_DEFAULT, "sel: %d, scrollpos: %d", lb.sel, lb.scrollpos);
    draw_listbox(&lb, xps);
    tb_present();

    while (1) {
        tb_poll_event(&ev);
        if (ev.key == TB_KEY_CTRL_C) 
            break;
        if (ev.ch == 'q') 
            break;

        tb_clear();
        tb_printf(0, tb_height()-1, TB_DEFAULT,TB_DEFAULT, "sel: %d, scrollpos: %d", lb.sel, lb.scrollpos);
        update_listbox(&lb, xps, &ev);
        draw_listbox(&lb, xps);
        tb_present();
    }

    tb_shutdown();
}

static void update_listbox(listbox_t *lb, array_t *xps, struct tb_event *ev) {
    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
        lb->sel--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
        lb->sel++;
    if (ev->key == TB_KEY_PGUP || ev->key == TB_KEY_CTRL_U)
        lb->sel -= lb->rect.height;
    if (ev->key == TB_KEY_PGDN || ev->key == TB_KEY_CTRL_D)
        lb->sel += lb->rect.height;

    if (lb->sel < 0)
        lb->sel = 0;
    if (lb->sel > xps->len-1)
        lb->sel = xps->len-1;

    if (lb->sel < lb->scrollpos)
        lb->scrollpos = lb->sel;
    if (lb->sel > lb->scrollpos + lb->rect.height-1)
        lb->scrollpos = lb->sel - (lb->rect.height-1);

    if (lb->scrollpos > xps->len-1)
        lb->scrollpos = xps->len-1;
}

static void draw_listbox(listbox_t *lb, array_t *xps) {
    int rownum = 0;
    exp_t *xp;

    if (lb->sel > xps->len-1)
        lb->sel = xps->len-1;
    if (lb->scrollpos > xps->len-1)
        lb->scrollpos = xps->len-1;

    for (int i = lb->scrollpos; i < xps->len; i++) {
        if (rownum > lb->rect.height-1)
            break;

        xp = xps->items[i];
        if (i == lb->sel)
            draw_listbox_row(lb, xp->desc->s, rownum, TB_BLUE, TB_WHITE);
        else
            draw_listbox_row(lb, xp->desc->s, rownum, TB_WHITE, TB_BLUE);
        rownum++;
    }
}

static void draw_listbox_row(listbox_t *lb, char *s, int rownum, uintattr_t fg, uintattr_t bg) {
    size_t s_len = strlen(s);
    int i=0;
    int is = 0;

    assert(lb->rect.width > 0);
    assert(rownum >= 0 && rownum < lb->rect.height);

    tb_set_cell(lb->rect.x+i, lb->rect.y+rownum, ' ', fg, bg);
    i++;

    while (i < lb->rect.width && is < s_len) {
        tb_set_cell(lb->rect.x+i, lb->rect.y+rownum, s[is], fg, bg);
        i++;
        is++;
    }

    if (i == lb->rect.width)
        return;
    assert(is == s_len);

    while (i < lb->rect.width) {
        tb_set_cell(lb->rect.x+i, lb->rect.y+rownum, ' ', fg, bg);
        i++;
    }
}

