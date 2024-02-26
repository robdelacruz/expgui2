#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

char *longtext = "This is some text that took me a long time to write.";

// Use for TB_OUTPUT_NORMAL
clr_t textfg = TB_WHITE;
clr_t textbg = TB_BLUE;
clr_t colfg = TB_YELLOW | TB_BOLD;
clr_t textselfg = TB_BLACK;
clr_t textselbg = TB_CYAN;

// Use for TB_OUTPUT_256
#if 0
clr_t textfg = 15;
clr_t textbg = 19;
clr_t colfg = 11;
clr_t textselfg = 16;
clr_t textselbg = 14;
#endif

uint statusx=0, statusy=23;
array_t *xps=0;
int sel_xps=0;
exp_t *selxp = NULL;

panel_t explist;

// Amount, Category, Date are fixed width fields
// Description field width will consume the remaining horizontal space.
int field_amt_size = 9;
int field_cat_size = 10;
int field_date_size = 5;

char *col_desc = "Description";
char *col_amt = "Amount";
char *col_cat = "Category";
char *col_date = "Date";

static void resize();
static void update(struct tb_event *ev);
static void draw();
static void draw_frame();
static void draw_explist();
static int draw_explist_field_col(int x, int y, char *col, int field_size, int field_xpad, clr_t fg, clr_t bg);
static int draw_explist_field(int x, int y, char *field, int field_size, int field_xpad, clr_t fg, clr_t bg);

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ALT);
//    tb_set_output_mode(TB_OUTPUT_256);
    tb_set_output_mode(TB_OUTPUT_NORMAL);
    tb_set_clear_attrs(textfg, textbg);

    statusx = 0;
    statusy = tb_height()-1;

    xps = array_new(0);
    db_select_exp(db, xps);
    if (xps->len > 0)
        selxp = xps->items[sel_xps];

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
}

static void printf_status(const char *fmt, ...) {
    char buf[4096];

    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    tb_printf(statusx, statusy, textfg,textbg, buf);
    va_end(vl);
}

static void resize() {
    explist = create_panel(0,1, tb_width(), tb_height()-2, 0,1, textfg,textbg);
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
    resize();
    tb_clear();

    draw_frame();
    draw_explist();

    tb_present();
}

static void draw_frame() {
    print_text(" Expense Buddy Console", 0,0, tb_width(), textselfg | TB_BOLD, textselbg);
}
static void draw_explist() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    clr_t fg, bg;
    int x,y;

    int num_fields;
    int field_xpad;
    int field_desc_size;

    clear_panel(&explist);

    // 4 fields: Description, Amount, Category, Date
    num_fields = 4;

    // Add padding of one space to left and right of each field
    field_xpad = 1;

    // Description
    field_desc_size = explist.content.width;
    field_desc_size -= field_amt_size + field_cat_size + field_date_size;
    field_desc_size -= num_fields-1;              // space for column separator
    field_desc_size -= (num_fields)*field_xpad*2; // space for padding on left/right side.

    // Column headings
    x = explist.content.x;
    y = explist.frame.y+1;
    fg = colfg;
    bg = explist.bg;

    x = draw_explist_field_col(x,y, col_desc, field_desc_size, field_xpad, fg,bg);
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    x++;

    x = draw_explist_field_col(x,y, col_amt, field_amt_size, field_xpad, fg,bg);
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    x++;

    x = draw_explist_field_col(x,y, col_cat, field_cat_size, field_xpad, fg,bg);
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    x++;

    x = draw_explist_field_col(x,y, col_date, field_date_size, field_xpad, fg,bg);

    x = explist.content.x;
    y = explist.content.y;

    int row = 0;
    for (int i=0; i < xps->len; i++) {
        if (row > explist.content.height-1)
            break;
        xp = xps->items[i];
        if (row == sel_xps) {
            fg = textselfg;
            bg = textselbg;
        } else {
            fg = textfg;
            bg = textbg;
        }

        // Row values
        x = draw_explist_field(x,y, xp->desc->s, field_desc_size, field_xpad, fg,bg);
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;

        snprintf(bufamt, sizeof(bufamt), "%9.2f", xp->amt);
        x = draw_explist_field(x,y, bufamt, field_amt_size, field_xpad, fg,bg);
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;

        //x = draw_explist_field(x,y, xp->catname->s, field_cat_size, field_xpad, fg,bg);
        x = draw_explist_field(x,y, "LongCatName12345", field_cat_size, field_xpad, fg,bg);
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;

        date_strftime(xp->date, "%m-%d", bufdate, sizeof(bufdate));
        x = draw_explist_field(x,y, bufdate, field_date_size, field_xpad, fg,bg);

        printf_status("content.width: %d, x: %d", explist.content.x+explist.content.width, x);
        assert(x == explist.content.x + explist.content.width);

        y++;
        x = explist.content.x;
        row++;
    }
}

static int draw_explist_field_col(int x, int y, char *col, int field_size, int field_xpad, clr_t fg, clr_t bg) {
    draw_ch_horz(" ", x,y, field_xpad, fg,bg);
    x += field_xpad;
    print_text_center(col, x,y, field_size, fg,bg);
    x+=field_size;
    draw_ch_horz(" ", x,y, field_xpad, fg,bg);
    x += field_xpad;

    return x;
}

static int draw_explist_field(int x, int y, char *field, int field_size, int field_xpad, clr_t fg, clr_t bg) {
    draw_ch_horz(" ", x,y, field_xpad, fg,bg);
    x += field_xpad;
    print_text(field, x,y, field_size, fg,bg);
    x+=field_size;
    draw_ch_horz(" ", x,y, field_xpad, fg,bg);
    x += field_xpad;

    return x;
}

