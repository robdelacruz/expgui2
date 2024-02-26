#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

char *longtext = "This is some text that took me a long time to write.";

// Use for TB_OUTPUT_NORMAL
#if 0
clr_t textfg = TB_WHITE;
clr_t textbg = TB_BLUE;
clr_t highlightfg = TB_BLACK;
clr_t highlightbg = TB_CYAN;
clr_t colfg = TB_YELLOW | TB_BOLD;
clr_t statusfg = TB_BLACK;
clr_t statusbg = TB_CYAN;
#endif

// Use for TB_OUTPUT_256
clr_t textfg = 15;
clr_t textbg = 19;
clr_t highlightfg = 16;
clr_t highlightbg = 14;
clr_t colfg = 11;
clr_t statusfg = 254;
clr_t statusbg = 245;

uint statusx=0, statusy=23;
array_t *xps=0;
int ixps=0;
int iscrollxps=0;

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
    tb_set_output_mode(TB_OUTPUT_256);
//    tb_set_output_mode(TB_OUTPUT_NORMAL);
    tb_set_clear_attrs(textfg, textbg);

    statusx = 0;
    statusy = tb_height()-1;

    xps = array_new(0);
    db_select_exp(db, xps);

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
    explist = create_panel(0,1, tb_width(), tb_height()-2, 0,0, 1,1, textfg,textbg);
}

static void update(struct tb_event *ev) {
    if (xps->len == 0)
        return;

    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
        ixps--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
        ixps++;
    if (ev->key == TB_KEY_PGUP || ev->key == TB_KEY_CTRL_U)
        ixps -= explist.content.height/2;
    if (ev->key == TB_KEY_PGDN || ev->key == TB_KEY_CTRL_D)
        ixps += explist.content.height/2;

    if (ixps < 0)
        ixps = 0;
    if (ixps > xps->len-1)
        ixps = xps->len-1;

    if (ixps < iscrollxps)
        iscrollxps = ixps;
    if (ixps > iscrollxps + explist.content.height-1)
        iscrollxps = ixps - (explist.content.height-1);

    if (iscrollxps < 0)
        iscrollxps = 0;
    if (iscrollxps > xps->len-1)
        iscrollxps = xps->len-1;

    assert(ixps >= 0 && ixps < xps->len);
}

static void draw() {
    resize();
    tb_clear();
    printf_status("ixps: %d, iscrollxps: %d", ixps, iscrollxps);

    draw_frame();
    draw_explist();

    tb_present();
}

static void draw_frame() {
    print_text(" Expense Buddy Console", 0,0, tb_width(), highlightfg | TB_BOLD, highlightbg);
}
static void draw_explist() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char bufstatus[64];
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
    fg = textfg;
    bg = textbg;

    x = draw_explist_field_col(x,y, col_desc, field_desc_size, field_xpad, colfg,bg);
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x, explist.content.y, explist.content.height, fg,bg);
    x++;
    x = draw_explist_field_col(x,y, col_amt, field_amt_size, field_xpad, colfg,bg);
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x, explist.content.y, explist.content.height, fg,bg);
    x++;
    x = draw_explist_field_col(x,y, col_cat, field_cat_size, field_xpad, colfg,bg);
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x, explist.content.y, explist.content.height, fg,bg);
    x++;
    x = draw_explist_field_col(x,y, col_date, field_date_size, field_xpad, colfg,bg);

    x = explist.content.x;
    y = explist.content.y;

    int row = 0;
    for (int i=iscrollxps; i < xps->len; i++) {
        if (row > explist.content.height-1)
            break;
        xp = xps->items[i];
        if (i == ixps) {
            fg = highlightfg;
            bg = highlightbg;
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

        assert(x == explist.content.x + explist.content.width);

        y++;
        x = explist.content.x;
        row++;
    }

    // Status line
    x = explist.content.x;
    y = explist.content.y + explist.content.height;
    snprintf(bufstatus, sizeof(bufstatus), "Expense %d/%ld", ixps+1, xps->len);
    print_text_right(bufstatus, x,y, explist.content.width, statusfg,statusbg);
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

