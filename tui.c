#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

char *longtext = "This is some text that took me a long time to write.";

clr_t textfg;
clr_t textbg;
clr_t highlightfg;
clr_t highlightbg;
clr_t titlefg;
clr_t titlebg;
clr_t colfg;
clr_t statusfg;
clr_t statusbg;
clr_t editfg;
clr_t editbg;
clr_t editfieldfg;
clr_t editfieldbg;
clr_t shadowfg;
clr_t shadowbg;

uint statusx=0, statusy=23;

#define EXPENSE_COL_START 0
#define EXPENSE_COL_DESC 0
#define EXPENSE_COL_AMT 1
#define EXPENSE_COL_CAT 2
#define EXPENSE_COL_DATE 3
#define EXPENSE_COL_COUNT 4
char *expense_col_label[EXPENSE_COL_COUNT];
int expense_col_maxwidth[EXPENSE_COL_COUNT];
int expense_field_maxchars[EXPENSE_COL_COUNT];

panel_t listxp;
array_t *listxp_xps=0;
int listxp_ixps=0;
int listxp_scrollpos=0;

panel_t editxp;
int editxp_show;
int editxp_selected_row;
int editxp_is_edit_row;
inputtext_t editxp_input_fields[EXPENSE_COL_COUNT];

static void set_output_mode(int mode);
static void init_state_vars();
static void update(struct tb_event *ev);
static void update_listxp(struct tb_event *ev);
static void update_editxp(struct tb_event *ev);
static void draw();
static void draw_shell();
static void draw_expense_list();
static void draw_expense_edit_panel();

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ESC);
//    set_output_mode(TB_OUTPUT_256);
    set_output_mode(TB_OUTPUT_NORMAL);
    tb_set_clear_attrs(textfg, textbg);

    init_state_vars();

    statusx = 0;
    statusy = tb_height()-1;

    db_select_exp(db, listxp_xps);

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

static void set_output_mode(int mode) {
    if (mode == TB_OUTPUT_256) {
        textfg = 15;
        textbg = 19;
        highlightfg = 16;
        highlightbg = 14;
        titlefg = 16;
        titlebg = 14;
        colfg = 11;
        statusfg = 254;
        statusbg = 245;
        editfg = 236;
        editbg = 252;
        editfieldfg = 236;
        editfieldbg = 14;
        shadowfg = 235;
        shadowbg = 235;
    } else {
        textfg = TB_WHITE;
        textbg = TB_BLUE;
        highlightfg = TB_BLACK;
        highlightbg = TB_CYAN;
        titlefg = TB_WHITE;
        titlebg = TB_BLUE;
        colfg = TB_YELLOW | TB_BOLD;
        statusfg = TB_YELLOW;
        statusbg = TB_BLUE;
        editfg = TB_BLACK;
        editbg = TB_WHITE;
        editfieldfg = TB_BLACK;
        editfieldbg = TB_YELLOW;
        shadowfg = TB_BLACK;
        shadowbg = TB_BLACK;
    }
    tb_set_output_mode(mode);
}

static void init_state_vars() {
    expense_col_label[EXPENSE_COL_DESC] = "Description";
    expense_col_label[EXPENSE_COL_AMT]  = "Amount";
    expense_col_label[EXPENSE_COL_CAT]  = "Category";
    expense_col_label[EXPENSE_COL_DATE] = "Date";

    expense_col_maxwidth[EXPENSE_COL_DESC] = 0;
    expense_col_maxwidth[EXPENSE_COL_AMT]  = 9;
    expense_col_maxwidth[EXPENSE_COL_CAT]  = 10;
    expense_col_maxwidth[EXPENSE_COL_DATE] = 5;

    expense_field_maxchars[EXPENSE_COL_DESC] = 25;
    expense_field_maxchars[EXPENSE_COL_AMT] = 9;
    expense_field_maxchars[EXPENSE_COL_CAT] = 10;
    expense_field_maxchars[EXPENSE_COL_DATE] = 10; // 2024-03-01

    listxp_xps = array_new(0);
    listxp_ixps = 0;
    listxp_scrollpos = 0;

    editxp_show = 0;
    editxp_selected_row = EXPENSE_COL_DESC;
    editxp_is_edit_row = 0;

    for (int i=EXPENSE_COL_START; i < EXPENSE_COL_COUNT; i++)
        init_inputtext(&editxp_input_fields[i], "", expense_field_maxchars[i]);
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
static void update_listxp(struct tb_event *ev) {
    if (listxp_xps->len == 0)
        return;

    // Enter to edit expense row
    if (ev->key == TB_KEY_ENTER) {
        exp_t *xp;
        inputtext_t *input;
        int maxchars;
        char bufdate[ISO_DATE_LEN+1];
        char bufamt[12];

        assert(listxp_xps->len > 0);
        assert(listxp_ixps >= 0 && listxp_ixps < listxp_xps->len);
        xp = listxp_xps->items[listxp_ixps];

        input = &editxp_input_fields[EXPENSE_COL_DESC];
        maxchars = expense_field_maxchars[EXPENSE_COL_DESC];
        init_inputtext(input, xp->desc->s, maxchars);

        input = &editxp_input_fields[EXPENSE_COL_AMT];
        maxchars = expense_field_maxchars[EXPENSE_COL_AMT];
        snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
        init_inputtext(input, bufamt, maxchars);

        input = &editxp_input_fields[EXPENSE_COL_CAT];
        maxchars = expense_field_maxchars[EXPENSE_COL_CAT];
        init_inputtext(input, xp->catname->s, maxchars);

        input = &editxp_input_fields[EXPENSE_COL_DATE];
        maxchars = expense_field_maxchars[EXPENSE_COL_DATE];
        date_to_iso(xp->date, bufdate, sizeof(bufdate));
        init_inputtext(input, bufdate, maxchars);

        editxp_show = 1;
        return;
    }

    // Navigate expense list Up/Down/PgUp/PgDn
    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
        listxp_ixps--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
        listxp_ixps++;
    if (ev->key == TB_KEY_PGUP || ev->key == TB_KEY_CTRL_U)
        listxp_ixps -= listxp.content.height/2;
    if (ev->key == TB_KEY_PGDN || ev->key == TB_KEY_CTRL_D)
        listxp_ixps += listxp.content.height/2;

    if (listxp_ixps < 0)
        listxp_ixps = 0;
    if (listxp_ixps > listxp_xps->len-1)
        listxp_ixps = listxp_xps->len-1;

    if (listxp_ixps < listxp_scrollpos)
        listxp_scrollpos = listxp_ixps;
    if (listxp_ixps > listxp_scrollpos + listxp.content.height-1)
        listxp_scrollpos = listxp_ixps - (listxp.content.height-1);

    if (listxp_scrollpos < 0)
        listxp_scrollpos = 0;
    if (listxp_scrollpos > listxp_xps->len-1)
        listxp_scrollpos = listxp_xps->len-1;

    assert(listxp_ixps >= 0 && listxp_ixps < listxp_xps->len);
}
static void update_editxp(struct tb_event *ev) {
    inputtext_t *input;

    // Editing field
    if (editxp_is_edit_row) {
        assert(editxp_selected_row >= EXPENSE_COL_START && editxp_selected_row < EXPENSE_COL_COUNT);
        input = &editxp_input_fields[editxp_selected_row];

        // Enter/ESC to cancel row edit
        if (ev->key == TB_KEY_ESC) {
            editxp_is_edit_row = 0;
            tb_set_cursor(0,0);
            tb_hide_cursor();
            input->icur = 0;
        } 
        if (ev->key == TB_KEY_ENTER) {
            editxp_is_edit_row = 0;
            tb_set_cursor(0,0);
            tb_hide_cursor();
            input->icur = 0;
        }

        update_inputtext(input, ev);
        return;
    }

    // Up/Down to select row
    // ESC to close panel
    if (ev->key == TB_KEY_ESC) {
        editxp_show = 0;
        editxp_selected_row = EXPENSE_COL_START;
        editxp_is_edit_row = 0;
        return;
    }

    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
        editxp_selected_row--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
        editxp_selected_row++;

    if (editxp_selected_row < EXPENSE_COL_START)
        editxp_selected_row = EXPENSE_COL_START;
    if (editxp_selected_row >= EXPENSE_COL_COUNT)
        editxp_selected_row = EXPENSE_COL_COUNT-1;

    // Enter to edit field
    if (ev->key == TB_KEY_ENTER) {
        editxp_is_edit_row = 1;
    }
}

static void draw() {
    tb_clear();
    printf_status("listxp_ixps: %d, listxp_scrollpos: %d", listxp_ixps, listxp_scrollpos);

    draw_shell();
    draw_expense_list();

    if (editxp_show) {
        draw_expense_edit_panel();
    }

    tb_present();
}

static void draw_shell() {
    print_text(" Expense Buddy Console", 0,0, tb_width(), titlefg | TB_BOLD, titlebg);
}

static void draw_expense_list() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char buf[128];
    clr_t fg, bg;
    int x,y;

    int xpad;
    int desc_maxwidth;

    listxp = create_panel_frame(0,1, tb_width(), tb_height()-2, 0,0, 1,1);
    draw_panel(&listxp, textfg,textbg);

    // Add padding of one space to left and right of each field
    xpad = 1;

    // Amount, Category, Date are fixed width fields
    // Description field width will consume the remaining horizontal space.
    desc_maxwidth = listxp.content.width;
    desc_maxwidth -= expense_col_maxwidth[EXPENSE_COL_AMT];
    desc_maxwidth -= expense_col_maxwidth[EXPENSE_COL_CAT];
    desc_maxwidth -= expense_col_maxwidth[EXPENSE_COL_DATE];
    desc_maxwidth -= EXPENSE_COL_COUNT-1;        // space for column separator
    desc_maxwidth -= (EXPENSE_COL_COUNT)*xpad*2; // space for padding on left/right side.
    expense_col_maxwidth[EXPENSE_COL_DESC] = desc_maxwidth;

    // Print column headings
    x = listxp.content.x;
    y = listxp.frame.y+1;
    fg = textfg;
    bg = textbg;
    for (int i=EXPENSE_COL_START; i < EXPENSE_COL_COUNT; i++) {
        print_text_padded_center(expense_col_label[i], x,y, expense_col_maxwidth[i], xpad, colfg,bg);
        x += xpad + expense_col_maxwidth[i] + xpad;
        if (i != EXPENSE_COL_COUNT-1) {
            draw_ch(ASC_VERTLINE, x,y, fg,bg);
            draw_ch_vert(ASC_VERTLINE, x, listxp.content.y, listxp.content.height, fg,bg);
            x++;
        }
    }

    x = listxp.content.x;
    y = listxp.content.y;

    int row = 0;
    for (int i=listxp_scrollpos; i < listxp_xps->len; i++) {
        if (row > listxp.content.height-1)
            break;
        xp = listxp_xps->items[i];
        if (i == listxp_ixps) {
            fg = highlightfg;
            bg = highlightbg;
        } else {
            fg = textfg;
            bg = textbg;
        }

        // Print one row of columns.
        for (int icol=EXPENSE_COL_START; icol < EXPENSE_COL_COUNT; icol++) {
            char *v;
            if (icol == EXPENSE_COL_DESC) {
                v = xp->desc->s;
            } else if (icol == EXPENSE_COL_AMT) {
                snprintf(bufamt, sizeof(bufamt), "%9.2f", xp->amt);
                v = bufamt;
            } else if (icol == EXPENSE_COL_CAT) {
                v = xp->catname->s;
            } else if (icol == EXPENSE_COL_DATE) {
                date_strftime(xp->date, "%m-%d", bufdate, sizeof(bufdate));
                v = bufdate;
            }
            print_text_padded(v, x,y, expense_col_maxwidth[icol], xpad, fg,bg);
            x += xpad + expense_col_maxwidth[icol] + xpad;

            if (icol != EXPENSE_COL_COUNT-1) {
                draw_ch(ASC_VERTLINE, x,y, fg,bg);
                x++;
            }
        }
        assert(x == listxp.content.x + listxp.content.width);

        y++;
        x = listxp.content.x;
        row++;
    }

    // Status line
    x = listxp.content.x;
    y = listxp.content.y + listxp.content.height;
    draw_ch_horz(" ", x,y, listxp.content.width, statusfg,statusbg);

    if (listxp_xps->len > 0) {
        xp = listxp_xps->items[listxp_ixps];
        snprintf(buf, sizeof(buf), "%.*s %.2f", 50, xp->desc->s, xp->amt);
        print_text(buf, x,y, listxp.content.width, statusfg,statusbg);

        snprintf(buf, sizeof(buf), "Expense %d/%ld", listxp_ixps+1, listxp_xps->len);
        x = listxp.content.x + listxp.content.width - strlen(buf);
        tb_print(x,y, statusfg,statusbg, buf);
    }
}

static void draw_expense_input(int icol, int x, int y, int label_width);

static void draw_expense_edit_panel() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char bufprompt[1024];
    int x,y;
    int panel_leftpad = 1;
    int panel_rightpad = 1;
    int panel_toppad = 2;
    int panel_bottompad = 1;
    int panel_height;
    size_t label_width = strlen(expense_col_label[EXPENSE_COL_DESC])+1;
    size_t field_width = expense_field_maxchars[EXPENSE_COL_DESC];
    clr_t fg,bg;

    assert(listxp_xps->len > 0);
    assert(listxp_ixps < listxp_xps->len);
    xp = listxp_xps->items[listxp_ixps];

    panel_height = EXPENSE_COL_COUNT;
    editxp = create_panel_center(label_width + field_width, panel_height, panel_leftpad, panel_rightpad, panel_toppad, panel_bottompad);
    draw_panel_shadow(&editxp, editfg,editbg, shadowfg,shadowbg);
    print_text_center("Edit Expense", editxp.content.x,editxp.frame.y+1, editxp.content.width, editfg | TB_BOLD,editbg);

    x = editxp.content.x;
    y = editxp.content.y;
    draw_expense_input(EXPENSE_COL_DESC, x,y, label_width);
    y++;

    snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
    draw_expense_input(EXPENSE_COL_AMT, x,y, label_width);
    y++;

    draw_expense_input(EXPENSE_COL_CAT, x,y, label_width);
    y++;

    date_to_iso(xp->date, bufdate, sizeof(bufdate));
    draw_expense_input(EXPENSE_COL_DATE, x,y, label_width);
    y++;

}
static void draw_expense_input(int icol, int x, int y, int label_width) {
    clr_t fg,bg;
    inputtext_t *inputtext;

    assert(icol >= EXPENSE_COL_START || icol < EXPENSE_COL_COUNT);
    inputtext = &editxp_input_fields[icol];

    fg = editfg;
    bg = editbg;
    print_text(expense_col_label[icol], x,y, label_width, fg,bg);

    x += label_width;
    if (icol == editxp_selected_row) {
        fg = highlightfg;
        bg = highlightbg;
        if (editxp_is_edit_row) {
            draw_inputtext(inputtext, x,y, 1, editfieldfg,editfieldbg);
            return;
        }
    }

    draw_inputtext(inputtext, x,y, 0, fg,bg);
}

