#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

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

#define XP_COL_START 0
#define XP_COL_DESC 0
#define XP_COL_AMT 1
#define XP_COL_CAT 2
#define XP_COL_DATE 3
#define XP_COL_COUNT 4
char *xp_col_names[XP_COL_COUNT] = {"Description", "Amount", "Category", "Date"};
int xp_entry_maxchars[XP_COL_COUNT] = {25, 9, 10, 10};

panel_t listxp;
array_t *listxp_xps=0;
int listxp_ixps=0;
int listxp_scrollpos=0;

panel_t editxp;
int editxp_show=0;
exp_t *editxp_xp;
int editxp_is_edit_entry=0;
entry_t editxp_entries[XP_COL_COUNT];

#define EDITXP_FOCUS_DESC   0
#define EDITXP_FOCUS_AMT    1
#define EDITXP_FOCUS_CAT    2
#define EDITXP_FOCUS_DATE   3
#define EDITXP_FOCUS_SAVE   4
#define EDITXP_FOCUS_COUNT  5
focus_t editxp_focus[EDITXP_FOCUS_COUNT];
int editxp_ifocus=0;

sqlite3 *_db;

static void set_output_mode(int mode);
static void update(struct tb_event *ev);
static void update_listxp(struct tb_event *ev);
static void update_editxp(struct tb_event *ev);
static void draw();
static void draw_shell();
static void draw_listxp();
static void draw_editxp();

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;

    tb_init();
    tb_set_input_mode(TB_INPUT_ESC);
//    set_output_mode(TB_OUTPUT_256);
    set_output_mode(TB_OUTPUT_NORMAL);
    tb_set_clear_attrs(textfg, textbg);

    _db = db;
    listxp_xps = array_new(0);
    editxp_xp = exp_new();
    for (int i=XP_COL_START; i < XP_COL_COUNT; i++)
        init_entry(&editxp_entries[i], "", xp_entry_maxchars[i]);
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

static exp_t *listxp_selected_expense() {
    if (listxp_xps->len == 0)
        return NULL;

    assert(listxp_ixps >= 0 && listxp_ixps < listxp_xps->len);
    return listxp_xps->items[listxp_ixps];
}

static void init_editxp(exp_t *xp) {
    int maxchars;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    entry_t *e;

    e = &editxp_entries[XP_COL_DESC];
    entry_set_text(e, xp->desc->s);

    e = &editxp_entries[XP_COL_AMT];
    snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
    entry_set_text(e, bufamt);

    e = &editxp_entries[XP_COL_CAT];
    entry_set_text(e, xp->catname->s);

    e = &editxp_entries[XP_COL_DATE];
    date_to_iso(xp->date, bufdate, sizeof(bufdate));
    entry_set_text(e, bufdate);

    exp_dup(editxp_xp, xp);
    editxp_show = 1;
    editxp_ifocus = EDITXP_FOCUS_DESC;
    editxp_is_edit_entry = 0;
}

static void update_listxp(struct tb_event *ev) {
    if (listxp_xps->len == 0)
        return;

    // Enter to edit expense row
    if (ev->key == TB_KEY_ENTER) {
        exp_t *xp = listxp_selected_expense();
        if (xp == NULL)
            return;
        init_editxp(xp);
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

static entry_t *editxp_selected_entry() {
    if (editxp_ifocus >= XP_COL_DESC && editxp_ifocus <= XP_COL_DATE)
        return &editxp_entries[editxp_ifocus];
    return NULL;
}

// Restore editxp entry to its original value.
static void editxp_cancel_edit() {
    entry_t *e = editxp_selected_entry();
    exp_t *xp = editxp_xp;

    if (editxp_ifocus == XP_COL_DESC) {
        entry_set_text(e, xp->desc->s);
    } else if (editxp_ifocus == XP_COL_AMT) {
        char bufamt[12];
        snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
        entry_set_text(e, bufamt);
    } else if (editxp_ifocus == XP_COL_CAT) {
        entry_set_text(e, xp->catname->s);
    } else if (editxp_ifocus == XP_COL_DATE) {
        char bufdate[ISO_DATE_LEN+1];
        date_to_iso(xp->date, bufdate, sizeof(bufdate));
        entry_set_text(e, bufdate);
    }

    editxp_is_edit_entry = 0;
    tb_hide_cursor();
}

// Update expense with entry field contents.
static void editxp_apply_edit() {
    entry_t *e = editxp_selected_entry();

    if (editxp_ifocus == XP_COL_DESC) {
        str_assign(editxp_xp->desc, e->buf);
    } else if (editxp_ifocus == XP_COL_AMT) {
        float amt = strtof(e->buf, NULL);
        editxp_xp->amt = amt;
    } else if (editxp_ifocus == XP_COL_CAT) {
        //todo
    } else if (editxp_ifocus == XP_COL_DATE) {
        date_assign_iso(editxp_xp->date, e->buf);
    }

    editxp_is_edit_entry = 0;
    tb_hide_cursor();
}

static void update_editxp(struct tb_event *ev) {
    // Editing field
    if (editxp_is_edit_entry) {
        // ESC to cancel edits
        if (ev->key == TB_KEY_ESC) {
            editxp_cancel_edit();
            return;
        } 
        // Enter to apply edits
        if (ev->key == TB_KEY_ENTER) {
            editxp_apply_edit();
            return;
        }
        // Tab/down to apply edits and switch to next field in edit mode.
        if (ev->key == TB_KEY_TAB || ev->key == TB_KEY_ARROW_DOWN) {
            editxp_apply_edit();

            // Switch to next focus
            assert(editxp_ifocus >= 0 && editxp_ifocus < EDITXP_FOCUS_COUNT);
            editxp_ifocus = editxp_focus[editxp_ifocus].next;

            if (editxp_ifocus >= EDITXP_FOCUS_DESC && editxp_ifocus <= EDITXP_FOCUS_DATE)
                editxp_is_edit_entry = 1;
            else
                editxp_is_edit_entry = 0;
            return;
        }

        entry_t *e = editxp_selected_entry();
        assert(e != NULL);
        update_entry(e, ev);
        return;
    }

    // ESC to close panel without saving changes
    if (ev->key == TB_KEY_ESC) {
        editxp_show = 0;
        return;
    }
    // Enter/Space pressed
    if (ev->key == TB_KEY_ENTER || ev->ch == ' ') {
        // Start editing field
        if (editxp_ifocus >= EDITXP_FOCUS_DESC && editxp_ifocus <= EDITXP_FOCUS_DATE) {
            entry_t *e = editxp_selected_entry();
            assert(e != NULL);
            e->icur = 0;
            editxp_is_edit_entry = 1;
            return;
        }
        // Save edits to listxp and db.
        exp_dup(listxp_selected_expense(), editxp_xp);
        db_edit_exp(_db, editxp_xp);
        editxp_show = 0;
        return;
    }
    // j/k, Up/Down, Tab to switch focus
    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k' || (ev->mod == TB_MOD_SHIFT && ev->key == TB_KEY_TAB)) {
        assert(editxp_ifocus >= 0 && editxp_ifocus < EDITXP_FOCUS_COUNT);
        editxp_ifocus = editxp_focus[editxp_ifocus].prev;
        return;
    }
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j' || ev->key == TB_KEY_TAB) {
        assert(editxp_ifocus >= 0 && editxp_ifocus < EDITXP_FOCUS_COUNT);
        editxp_ifocus = editxp_focus[editxp_ifocus].next;
        return;
    }
}

#if 0
static void update_editxp(struct tb_event *ev) {
    // Editing field
    if (editxp_is_edit_entry) {
        // Enter/ESC to cancel row edit
        if (ev->key == TB_KEY_ESC) {
            editxp_cancel_edit();
            return;
        } 
        if (ev->key == TB_KEY_ENTER) {
            editxp_enter_edit();
            return;
        }

        if (ev->key == TB_KEY_TAB || ev->key == TB_KEY_ARROW_DOWN) {
            editxp_enter_edit();

            editxp_icol++;
            if (editxp_icol >= XP_COL_COUNT)
                editxp_icol = XP_COL_START;
            editxp_is_edit_entry = 1;
        }

        entry_t *e = editxp_selected_entry();
        assert(e != NULL);
        update_entry(e, ev);
        return;
    }

    // ESC to close panel
    if (ev->key == TB_KEY_ESC) {
        // Update expense list with changes.
        exp_dup(listxp_selected_expense(), editxp_xp);

        // Write expense changes.
        db_edit_exp(_db, editxp_xp);

        editxp_show = 0;
        editxp_icol = XP_COL_START;
        editxp_is_edit_entry = 0;
        return;
    }

    // Up/Down to select row
    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k' || (ev->mod == TB_MOD_SHIFT && ev->key == TB_KEY_TAB))
        editxp_icol--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j' || ev->key == TB_KEY_TAB)
        editxp_icol++;

    if (editxp_icol < XP_COL_START)
        editxp_icol = XP_COL_COUNT-1;
    if (editxp_icol >= XP_COL_COUNT)
        editxp_icol = XP_COL_START;

    // Enter to edit field
    if (ev->key == TB_KEY_ENTER) {
        entry_t *e = editxp_selected_entry();
        e->icur = 0;
        editxp_is_edit_entry = 1;
    }
}
#endif

static void draw() {
    tb_clear();
    printf_status("listxp_ixps: %d, listxp_scrollpos: %d", listxp_ixps, listxp_scrollpos);

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

static void draw_listxp() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char buf[128];
    clr_t fg, bg;
    int x,y;

    int xpad;
    int desc_maxwidth;
    int col_widths[XP_COL_COUNT] = {0, 9, 10, 5}; // desc, amt, cat, date

    listxp = create_panel_frame(0,1, tb_width(), tb_height()-2, 0,0, 1,1);
    draw_panel(&listxp, textfg,textbg);

    // Add padding of one space to left and right of each field
    xpad = 1;

    // Amount, Category, Date are fixed width fields
    // Description field width will consume the remaining horizontal space.
    desc_maxwidth = listxp.content.width;
    desc_maxwidth -= col_widths[XP_COL_AMT];
    desc_maxwidth -= col_widths[XP_COL_CAT];
    desc_maxwidth -= col_widths[XP_COL_DATE];
    desc_maxwidth -= XP_COL_COUNT-1;        // space for column separator
    desc_maxwidth -= (XP_COL_COUNT)*xpad*2; // space for padding on left/right side.
    col_widths[XP_COL_DESC] = desc_maxwidth;

    // Print column headings
    x = listxp.content.x;
    y = listxp.frame.y+1;
    fg = textfg;
    bg = textbg;
    for (int i=XP_COL_START; i < XP_COL_COUNT; i++) {
        print_text_padded_center(xp_col_names[i], x,y, col_widths[i], xpad, colfg,colbg);
        x += xpad + col_widths[i] + xpad;
        if (i != XP_COL_COUNT-1) {
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
        for (int icol=XP_COL_START; icol < XP_COL_COUNT; icol++) {
            char *v;
            if (icol == XP_COL_DESC) {
                v = xp->desc->s;
            } else if (icol == XP_COL_AMT) {
                snprintf(bufamt, sizeof(bufamt), "%9.2f", xp->amt);
                v = bufamt;
            } else if (icol == XP_COL_CAT) {
                v = xp->catname->s;
            } else if (icol == XP_COL_DATE) {
                date_strftime(xp->date, "%m-%d", bufdate, sizeof(bufdate));
                v = bufdate;
            }
            print_text_padded(v, x,y, col_widths[icol], xpad, fg,bg);
            x += xpad + col_widths[icol] + xpad;

            if (icol != XP_COL_COUNT-1) {
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

    xp = listxp_selected_expense();
    if (xp != NULL) {
        snprintf(buf, sizeof(buf), "%.*s %.2f", 50, xp->desc->s, xp->amt);
        print_text(buf, x,y, listxp.content.width, statusfg,statusbg);

        snprintf(buf, sizeof(buf), "Expense %d/%ld", listxp_ixps+1, listxp_xps->len);
        x = listxp.content.x + listxp.content.width - strlen(buf);
        tb_print(x,y, statusfg,statusbg, buf);
    }
}

static void draw_editxp_row(int icol, int x, int y, int label_width);

static void draw_editxp() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char bufprompt[1024];
    int x,y;
    int panel_leftpad = 1;
    int panel_rightpad = 1;
    int panel_toppad = 2;
    int panel_bottompad = 0;
    int panel_height;
    size_t label_width = strlen(xp_col_names[XP_COL_DESC])+1;
    size_t field_width = xp_entry_maxchars[XP_COL_DESC];
    clr_t fg,bg;

    assert(listxp_xps->len > 0);
    assert(listxp_ixps < listxp_xps->len);
    xp = editxp_xp;

    panel_height = XP_COL_COUNT + 2;
    editxp = create_panel_center(label_width + field_width, panel_height, panel_leftpad, panel_rightpad, panel_toppad, panel_bottompad);
    draw_panel_shadow(&editxp, popupfg,popupbg, shadowfg,shadowbg);
    print_text_center("Edit Expense", editxp.content.x,editxp.frame.y+1, editxp.content.width, popupfg | TB_BOLD,popupbg);

    x = editxp.content.x;
    y = editxp.content.y;
    draw_editxp_row(XP_COL_DESC, x,y, label_width);
    y++;

    snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
    draw_editxp_row(XP_COL_AMT, x,y, label_width);
    y++;

    draw_editxp_row(XP_COL_CAT, x,y, label_width);
    y++;

    date_to_iso(xp->date, bufdate, sizeof(bufdate));
    draw_editxp_row(XP_COL_DATE, x,y, label_width);
    y++;

    fg = popupfg;
    bg = popupbg;
    if (editxp_ifocus == EDITXP_FOCUS_SAVE) {
        fg = highlightfg;
        bg = highlightbg;
    }
    print_text_center("[Save]", editxp.content.x,editxp.content.y+editxp.content.height-1, editxp.content.width, fg,bg);
}
static void draw_editxp_row(int icol, int x, int y, int label_width) {
    clr_t fg,bg;
    entry_t *e;

    assert(icol >= XP_COL_START || icol < XP_COL_COUNT);
    e = &editxp_entries[icol];

    fg = popupfg;
    bg = popupbg;
    print_text(xp_col_names[icol], x,y, label_width, fg,bg);

    x += label_width;
    if (icol == editxp_ifocus) {
        fg = highlightfg;
        bg = highlightbg;
        if (editxp_is_edit_entry) {
            draw_entry(e, x,y, 1, editfieldfg,editfieldbg);
            return;
        }
    }

    draw_entry(e, x,y, 0, fg,bg);
}

