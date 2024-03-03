#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

#define EXPLIST 0
#define EXPEDIT 1

char *longtext = "This is some text that took me a long time to write.";

// Use for TB_OUTPUT_NORMAL
clr_t textfg = TB_WHITE;
clr_t textbg = TB_BLUE;
clr_t highlightfg = TB_BLACK;
clr_t highlightbg = TB_CYAN;
clr_t titlefg = TB_WHITE;
clr_t titlebg = TB_BLUE;
clr_t colfg = TB_YELLOW | TB_BOLD;
clr_t statusfg = TB_YELLOW;
clr_t statusbg = TB_BLUE;
clr_t editfg = TB_BLACK;
clr_t editbg = TB_WHITE;
clr_t editfieldfg = TB_BLACK;
clr_t editfieldbg = TB_YELLOW;
clr_t shadowfg = TB_BLACK;
clr_t shadowbg = TB_BLACK;

// Use for TB_OUTPUT_256
#if 0
clr_t textfg = 15;
clr_t textbg = 19;
clr_t highlightfg = 16;
clr_t highlightbg = 14;
clr_t titlefg = 16;
clr_t titlebg = 14;
clr_t colfg = 11;
clr_t statusfg = 254;
clr_t statusbg = 245;
clr_t editfg = 236;
clr_t editbg = 252;
clr_t shadowfg = 235;
clr_t shadowbg = 235;
#endif

uint statusx=0, statusy=23;
array_t *xps=0;
int ixps=0;
int iscrollxps=0;

panel_t explist;
panel_t expedit;
int active_panel = EXPLIST;

#define XPFIELD_START 0
#define XPFIELD_DESC 0
#define XPFIELD_AMT 1
#define XPFIELD_CAT 2
#define XPFIELD_DATE 3
#define XPFIELD_LEN 4

int ixpfield = XPFIELD_DESC;
int feditxpfield = 0;
int icurxpfield = 0;

// Amount, Category, Date are fixed width fields
// Description field width will consume the remaining horizontal space.
int field_amt_size = 9;
int field_cat_size = 10;
int field_date_size = 5;

char *colxpfield[] = {
    "Description",
    "Amount",
    "Category",
    "Date"
};

static void update(struct tb_event *ev);
static void draw();
static void draw_shell();
static void draw_explist();
static void draw_expedit();

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ESC);
//    tb_set_output_mode(TB_OUTPUT_256);
    tb_set_output_mode(TB_OUTPUT_NORMAL);
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

static void update(struct tb_event *ev) {
    if (active_panel == EXPLIST) {
        if (xps->len == 0)
            return;

        if (ev->ch == 'e' || ev->key == TB_KEY_ENTER) {
            active_panel = EXPEDIT;
            return;
        }

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
    } else if (active_panel == EXPEDIT) {
        if (!feditxpfield) {
            // Select field to edit
            if (ev->key == TB_KEY_ESC) {
                active_panel = EXPLIST;
                ixpfield = XPFIELD_START;
                return;
            }

            if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
                ixpfield--;
            if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
                ixpfield++;

            if (ixpfield < XPFIELD_START)
                ixpfield = XPFIELD_START;
            if (ixpfield >= XPFIELD_LEN)
                ixpfield = XPFIELD_LEN-1;

            if (ev->key == TB_KEY_ENTER) {
                feditxpfield = 1;
                icurxpfield = 0;
            }
            return;
        }

        if (feditxpfield) {
            // Editing field
            if (ev->key == TB_KEY_ESC) {
                feditxpfield = 0;
                tb_set_cursor(0,0);
                tb_hide_cursor();
            } 
            if (ev->key == TB_KEY_ENTER) {
                feditxpfield = 0;
                tb_set_cursor(0,0);
                tb_hide_cursor();
            }
        }
    }
}

static void draw() {
    tb_clear();
    printf_status("ixps: %d, iscrollxps: %d", ixps, iscrollxps);

    draw_shell();
    draw_explist();

    if (active_panel == EXPEDIT) {
        draw_expedit();
    }

    tb_present();
}

static void draw_shell() {
    print_text(" Expense Buddy Console", 0,0, tb_width(), titlefg | TB_BOLD, titlebg);
}

static void draw_explist() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char buf[128];
    clr_t fg, bg;
    int x,y;

    int num_fields;
    int xpad;
    int field_desc_size;
    int ifield;

    explist = create_panel_frame(0,1, tb_width(), tb_height()-2, 0,0, 1,1);
    draw_panel(&explist, textfg,textbg);

    // 4 fields: Description, Amount, Category, Date
    num_fields = 4;

    // Add padding of one space to left and right of each field
    xpad = 1;

    // Description
    field_desc_size = explist.content.width;
    field_desc_size -= field_amt_size + field_cat_size + field_date_size;
    field_desc_size -= num_fields-1;              // space for column separator
    field_desc_size -= (num_fields)*xpad*2;       // space for padding on left/right side.

    // Column headings
    x = explist.content.x;
    y = explist.frame.y+1;
    fg = textfg;
    bg = textbg;

    ifield = XPFIELD_DESC;
    print_text_padded_center(colxpfield[ifield], x,y, field_desc_size, xpad, colfg,bg);
    x += xpad + field_desc_size + xpad;
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x, explist.content.y, explist.content.height, fg,bg);
    x++;

    ifield = XPFIELD_AMT;
    print_text_padded_center(colxpfield[ifield], x,y, field_amt_size, xpad, colfg,bg);
    x += xpad + field_amt_size + xpad;
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x, explist.content.y, explist.content.height, fg,bg);
    x++;

    ifield = XPFIELD_CAT;
    print_text_padded_center(colxpfield[ifield], x,y, field_cat_size, xpad, colfg,bg);
    x += xpad + field_cat_size + xpad;
    draw_ch(ASC_VERTLINE, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x, explist.content.y, explist.content.height, fg,bg);
    x++;

    ifield = XPFIELD_DATE;
    print_text_padded_center(colxpfield[ifield], x,y, field_date_size, xpad, colfg,bg);

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
        print_text_padded(xp->desc->s, x,y, field_desc_size, xpad, fg,bg);
        x += xpad + field_desc_size + xpad;
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;

        snprintf(bufamt, sizeof(bufamt), "%9.2f", xp->amt);
        print_text_padded(bufamt, x,y, field_amt_size, xpad, fg,bg);
        x += xpad + field_amt_size + xpad;
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;

        //x = draw_explist_field(x,y, xp->catname->s, field_cat_size, xpad, fg,bg);
        print_text_padded("LongCatName12345", x,y, field_cat_size, xpad, fg,bg);
        x += xpad + field_cat_size + xpad;
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;

        date_strftime(xp->date, "%m-%d", bufdate, sizeof(bufdate));
        print_text_padded(bufdate, x,y, field_date_size, xpad, fg,bg);
        x += xpad + field_date_size + xpad;

        assert(x == explist.content.x + explist.content.width);

        y++;
        x = explist.content.x;
        row++;
    }

    // Status line
    x = explist.content.x;
    y = explist.content.y + explist.content.height;
    draw_ch_horz(" ", x,y, explist.content.width, statusfg,statusbg);

    if (xps->len > 0) {
        xp = xps->items[ixps];
        snprintf(buf, sizeof(buf), "%.*s %.2f", 50, xp->desc->s, xp->amt);
        print_text(buf, x,y, explist.content.width, statusfg,statusbg);

        snprintf(buf, sizeof(buf), "Expense %d/%ld", ixps+1, xps->len);
        x = explist.content.x + explist.content.width - strlen(buf);
        tb_print(x,y, statusfg,statusbg, buf);
    }
}

static void draw_expedit_row(int ifield, int x, int y, int label_width, int field_width, int field_xpad, char *field_val);

static void draw_expedit() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char bufprompt[1024];
    int x,y;
    int field_xpad = 1;
    int panel_leftpad = 1;
    int panel_rightpad = 1;
    int panel_toppad = 2;
    int panel_bottompad = 1;
    int panel_height;
    size_t label_width = strlen(colxpfield[XPFIELD_DESC])+1;
    size_t field_width = 30;
    int ifield;
    clr_t fg,bg;

    assert(xps->len > 0);
    assert(ixps < xps->len-1);
    xp = xps->items[ixps];

    panel_height = XPFIELD_LEN;
    expedit = create_panel_center(label_width + field_width + field_xpad*2, panel_height, panel_leftpad, panel_rightpad, panel_toppad, panel_bottompad);
    draw_panel_shadow(&expedit, editfg,editbg, shadowfg,shadowbg);
    print_text_center("Edit Expense", expedit.content.x,expedit.frame.y+1, expedit.content.width, editfg | TB_BOLD,editbg);

    x = expedit.content.x;
    y = expedit.content.y;
    draw_expedit_row(XPFIELD_DESC, x,y, label_width, field_width, field_xpad, xp->desc->s);
    y++;

    snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
    draw_expedit_row(XPFIELD_AMT, x,y, label_width, field_width, field_xpad, bufamt);
    y++;

    draw_expedit_row(XPFIELD_CAT, x,y, label_width, field_width, field_xpad, xp->catname->s);
    y++;

    date_strftime(xp->date, "%m-%d", bufdate, sizeof(bufdate));
    draw_expedit_row(XPFIELD_DATE, x,y, label_width, field_width, field_xpad, bufdate);
    y++;

}
static void draw_expedit_row(int ifield, int x, int y, int label_width, int field_width, int field_xpad, char *field_val) {
    clr_t fg,bg;
    fg = editfg;
    bg = editbg;
    print_text(colxpfield[ifield], x,y, label_width, fg,bg);

    x += label_width;
    if (ifield == ixpfield) {
        fg = highlightfg;
        bg = highlightbg;
        if (feditxpfield) {
            fg = editfieldfg;
            bg = editfieldbg;
            tb_set_cursor(x+field_xpad,y);
        }
    }
    print_text_padded(field_val, x,y, field_width, field_xpad, fg,bg);
}

