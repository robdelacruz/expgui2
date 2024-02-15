#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "clib.h"
#include "db.h"

#define COLOR_FG TB_GREEN
#define COLOR_BG TB_BLACK

typedef struct {
    uint x;
    uint y;
    uint width;
    uint height;
} rect_t;

typedef struct listbox_s {
    rect_t rect;
    int sel;
    int scrollpos;
    array_t *rows;
    void (*draw_row)(struct listbox_s *lb, int irow, uintattr_t fg, uintattr_t bg);
} listbox_t;

typedef void (listbox_draw_row_fn)(listbox_t *lb, int irow, uintattr_t fg, uintattr_t bg);

static void listbox_update(listbox_t *lb, struct tb_event *ev);
static void listbox_draw(listbox_t *lb);
static void listbox_draw_row(listbox_t *lb, int irow, uintattr_t fg, uintattr_t bg);
static void listbox_draw_string_row(listbox_t *lb, char *s, int irow, uintattr_t fg, uintattr_t bg);
static void listbox_draw_xp_row(listbox_t *lb, int irow, uintattr_t fg, uintattr_t bg);

static listbox_t *listbox_new(uint x, uint y, uint width, uint height, array_t *rows, listbox_draw_row_fn *draw_row) {
    listbox_t *lb = malloc(sizeof(listbox_t));
    lb->rect = (rect_t) {x, y, width, height};
    lb->sel = 0;
    lb->scrollpos = 0;
    lb->rows = rows;
    if (draw_row != NULL)
        lb->draw_row = draw_row;
    else
        lb->draw_row = listbox_draw_row;
    return lb;
}

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;
    listbox_t *lb;
    array_t *xps = array_new(0);

    tb_init();
    tb_set_input_mode(TB_INPUT_ALT);
    tb_set_clear_attrs(COLOR_FG, COLOR_BG);

    db_select_exp(db, xps);
    lb = listbox_new(0,0, tb_width(), tb_height()-1, xps, listbox_draw_xp_row);

    tb_clear();
    tb_printf(0, tb_height()-1, TB_DEFAULT,TB_DEFAULT, "sel: %d, scrollpos: %d", lb->sel, lb->scrollpos);
    listbox_draw(lb);
    tb_present();

    while (1) {
        tb_poll_event(&ev);
        if (ev.key == TB_KEY_CTRL_C) 
            break;
        if (ev.ch == 'q') 
            break;

        tb_clear();
        tb_printf(0, tb_height()-1, TB_DEFAULT,TB_DEFAULT, "sel: %d, scrollpos: %d", lb->sel, lb->scrollpos);
        listbox_update(lb, &ev);
        listbox_draw(lb);
        tb_present();
    }

    tb_shutdown();
}

static void listbox_update(listbox_t *lb, struct tb_event *ev) {
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
    if (lb->sel > lb->rows->len-1)
        lb->sel = lb->rows->len-1;

    if (lb->sel < lb->scrollpos)
        lb->scrollpos = lb->sel;
    if (lb->sel > lb->scrollpos + lb->rect.height-1)
        lb->scrollpos = lb->sel - (lb->rect.height-1);

    if (lb->scrollpos > lb->rows->len-1)
        lb->scrollpos = lb->rows->len-1;
}

static void listbox_draw_xp_row(listbox_t *lb, int irow, uintattr_t fg, uintattr_t bg) {
    char buf[4096];
    char isodate[ISO_DATE_LEN+1];
    exp_t *xp = lb->rows->items[irow];

    date_to_iso(xp->date, isodate, sizeof(isodate));
    snprintf(buf, sizeof(buf), "%s %s %.2f %s", isodate, xp->desc->s, xp->amt, xp->catname->s);
    listbox_draw_string_row(lb, buf, irow, fg, bg);
}

static void listbox_draw(listbox_t *lb) {
    int irow = 0;
    exp_t *xp;
    listbox_draw_row_fn *draw_row;

    if (lb->sel > lb->rows->len-1)
        lb->sel = lb->rows->len-1;
    if (lb->scrollpos > lb->rows->len-1)
        lb->scrollpos = lb->rows->len-1;

    if (lb->draw_row != NULL)
        draw_row = lb->draw_row;
    else
        draw_row = listbox_draw_row;

    for (int i = lb->scrollpos; i < lb->rows->len; i++) {
        if (irow > lb->rect.height-1)
            break;

        if (i == lb->sel)
            (*draw_row)(lb, irow, COLOR_BG, COLOR_FG);
        else
            (*draw_row)(lb, irow, COLOR_FG, COLOR_BG);
        irow++;
    }
}

static void listbox_draw_row(listbox_t *lb, int irow, uintattr_t fg, uintattr_t bg) {
    char *s = lb->rows->items[irow];
    listbox_draw_string_row(lb, s, irow, fg, bg);
}

static void listbox_draw_string_row(listbox_t *lb, char *s, int irow, uintattr_t fg, uintattr_t bg) {
    size_t s_len = strlen(s);
    int i=0;
    int is = 0;

    assert(lb->rect.width > 0);
    assert(irow >= 0 && irow < lb->rect.height);

    tb_set_cell(lb->rect.x+i, lb->rect.y+irow, ' ', fg, bg);
    i++;

    while (i < lb->rect.width && is < s_len) {
        tb_set_cell(lb->rect.x+i, lb->rect.y+irow, s[is], fg, bg);
        i++;
        is++;
    }

    if (i == lb->rect.width)
        return;
    assert(is == s_len);

    while (i < lb->rect.width) {
        tb_set_cell(lb->rect.x+i, lb->rect.y+irow, ' ', fg, bg);
        i++;
    }
}

