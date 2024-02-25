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

rect_t screen = {0,0, 80,23};
uint statusx=0, statusy=23;
array_t *xps=0;
int sel_xps=0;
exp_t *selxp = NULL;

//rect_t xpview_area = {1,1, 44,23};
//rect_t xpdetails_area = {46,1, 33,7};
rect_t xpview_area;
rect_t xpdetails_area;

// xpview area segments
// 1 + desc + 1 + amt + cat + 1 + date + 1
int field_desc_size = 0; // remaining space available
int field_amt_size = 9;
int field_cat_size = 10;
int field_date_size = 5;

// xpdetail area segments
// lbl + val + 1
char *lbldesc = " Desc: ";
char *lblamt  = " Amt : ";
char *lblcat  = " Cat : ";
char *lbldate = " Date: ";
int num_labels = 4;
int field_label_size = 0; // strlen(lbldesc)
int field_val_size = 20;

char *coldesc = "Description";
char *colamt = "Amount";
char *colcat = "Category";
char *coldate = "Date";

static void resize_fields();
static void update(struct tb_event *ev);
static void draw();
static void draw_frame();
static void draw_xpview();
static void draw_xpdetail(exp_t *xp);

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ALT);
//    tb_set_output_mode(TB_OUTPUT_256);
    tb_set_output_mode(TB_OUTPUT_NORMAL);
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

static void resize_fields() {
    xpdetails_area.height = num_labels + 1 + 2; // (empty space) + (prompt line) + (textbox)
    field_label_size = strlen(lbldesc);
    xpdetails_area.width = field_label_size + field_val_size + 1;

    xpview_area.width = tb_width() - xpdetails_area.width;
    xpview_area.width -= 3; // leave room for border and dividing line
    field_desc_size = xpview_area.width - (1 + field_amt_size + 1 + field_cat_size + 1 + field_date_size);

    xpview_area.x = 1;
    xpview_area.y = 3;
    xpview_area.height = tb_height() - 2 - xpview_area.y + 1;

    xpdetails_area.x = xpview_area.x + xpview_area.width + 1;
    xpdetails_area.y = xpview_area.y-1;
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
    resize_fields();
    draw_frame();
    draw_xpview();
//    draw_xpdetail(selxp);

    tb_present();
}

static void draw_frame() {
    rect_t r;
    char *asc = ASC_BLOCK_MED;
    clr_t fg = textfg;
    clr_t bg = textbg;
    int xcol, ycol;

    print_text(" Expense Buddy Console", 0,0, tb_width(), textselfg | TB_BOLD, textselbg);


    // Border frame of xpview
    // Make room for one row of column headings
    r = outer_rect(xpview_area);
    r.y -= 1;
    r.height += 1;

    draw_ch(ASC_TOPLEFT, r.x, r.y, fg,bg);
    draw_ch(ASC_TOPT, r.x+r.width-1, r.y, fg,bg);
    draw_ch(ASC_BOTTOMLEFT, r.x, r.y+r.height-1, fg,bg);
    draw_ch(ASC_BOTTOMT, r.x+r.width-1, r.y+r.height-1, fg,bg);
    draw_ch_horz(ASC_HORZLINE, r.x+1, r.y, r.width-2, fg,bg);
    draw_ch_horz(ASC_HORZLINE, r.x+1, r.y+r.height-1, r.width-2, fg,bg);
    draw_ch_vert(ASC_VERTLINE, r.x, r.y+1, r.height-2, fg,bg);
    draw_ch_vert(ASC_VERTLINE, r.x+r.width-1, r.y+1, r.height-2, fg,bg);

    // Column headings
    xcol = r.x+1;
    ycol = r.y+1;
    print_text_center(coldesc, xcol,ycol, field_desc_size, colfg,bg);
    xcol += field_desc_size;
    draw_ch(ASC_VERTLINE, xcol,ycol, fg,bg);
    xcol++;
    print_text_center(colamt, xcol,ycol, field_amt_size, colfg,bg);
    xcol += field_amt_size;
    draw_ch(ASC_VERTLINE, xcol,ycol, fg,bg);
    xcol++;
    print_text_center(colcat, xcol,ycol, field_cat_size, colfg,bg);
    xcol += field_cat_size;
    draw_ch(ASC_VERTLINE, xcol,ycol, fg,bg);
    xcol++;
    print_text_center(coldate, xcol,ycol, field_date_size, colfg,bg);

    // Column dividing lines
    xcol = xpview_area.x + field_desc_size;
    ycol = xpview_area.y;
    draw_ch_vert(ASC_VERTLINE, xcol,ycol, xpview_area.height, fg,bg);
    xcol += field_amt_size+1;
    draw_ch_vert(ASC_VERTLINE, xcol,ycol, xpview_area.height, fg,bg);
    xcol += field_cat_size+1;
    draw_ch_vert(ASC_VERTLINE, xcol,ycol, xpview_area.height, fg,bg);

    r = outer_rect(xpdetails_area);
    draw_ch(ASC_TOPRIGHT, r.x+r.width-1, r.y, fg,bg);
    draw_ch(ASC_BOTTOMRIGHT, r.x+r.width-1, r.y+r.height-1, fg,bg);
    draw_ch(ASC_LEFTT, r.x, r.y+r.height-1, fg,bg);
    draw_ch_horz(ASC_HORZLINE, r.x+1, r.y, r.width-2, fg,bg);
    draw_ch_horz(ASC_HORZLINE, r.x+1, r.y+r.height-1, r.width-2, fg,bg);
    draw_ch_vert(ASC_VERTLINE, r.x+r.width-1, r.y+1, r.height-2, fg,bg);

    //draw_clear(xpview_area.x, xpview_area.y, xpview_area.width, xpview_area.height, TB_YELLOW);
    //draw_clear(xpview_area.x, xpview_area.y, xpview_area.width, xpview_area.height, 19);
    //draw_clear(xpdetails_area.x, xpdetails_area.y, xpdetails_area.width, xpdetails_area.height, TB_CYAN);
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

        print_text(xp->desc->s, x,y, field_desc_size, fg,bg);
        x+=field_desc_size;
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;
        tb_printf(x,y, fg,bg, "%9.2f", xp->amt);
        x+=field_amt_size;
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;
        //print_text(xp->catname->s, x,y, 10, fg,bg);
        print_text("LongCatName12345", x,y, field_cat_size, fg,bg);
        x+=field_cat_size;
        draw_ch(ASC_VERTLINE, x,y, fg,bg);
        x++;
        date_strftime(xp->date, "%m-%d", bufdate, sizeof(bufdate));
        print_text(bufdate, x,y, field_date_size, fg,bg);
        x+=field_date_size;
        assert(x == xpview_area.x + xpview_area.width);

        y++;
        x = xpview_area.x;
        row++;
    }
}
static void draw_xpdetail(exp_t *xp) {
#if 0
    clr_t fg, bg;
    int x = xpdetails_area.x;
    int y = xpdetails_area.y;
    int row = 0;
    char bufdate[ISO_DATE_LEN+1];

    int lbl_len, val_len;

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
#endif
}


