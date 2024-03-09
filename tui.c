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

sqlite3 *_db;
array_t *cats=0;

panel_t listxp;
array_t *listxp_xps=0;
int listxp_ixps=0;
int listxp_scrollpos=0;

#define EDITXP_FOCUS_DESC   0
#define EDITXP_FOCUS_AMT    1
#define EDITXP_FOCUS_CAT    2
#define EDITXP_FOCUS_DATE   3
#define EDITXP_FOCUS_SAVE   4
#define EDITXP_FOCUS_COUNT  5

panel_t editxp;
int editxp_show=0;
exp_t *editxp_xp;
focus_t editxp_focus[EDITXP_FOCUS_COUNT];
int editxp_ifocus=0;
int editxp_is_focus_active=0;
entry_t editxp_entry_desc;
entry_t editxp_entry_amt;
entry_t editxp_entry_date;
listbox_t editxp_listbox_cat;

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

    statusx = 0;
    statusy = tb_height()-1;

    _db = db;
    cats = array_new(0);
    listxp_xps = array_new(0);
    editxp_xp = exp_new();

    db_select_exp(db, listxp_xps);
    db_select_cat(db, cats);

    init_entry(&editxp_entry_desc, "", xp_entry_maxchars[XP_COL_DESC]);
    init_entry(&editxp_entry_amt, "", xp_entry_maxchars[XP_COL_AMT]);
    init_entry(&editxp_entry_date, "", xp_entry_maxchars[XP_COL_DATE]);
    editxp_listbox_cat.ipos = 0;
    editxp_listbox_cat.iscrollpos = 0;
    editxp_listbox_cat.height = 5;
    editxp_listbox_cat.width = xp_entry_maxchars[XP_COL_CAT];

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

    array_free(listxp_xps);
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

static void update_catlistbox(struct tb_event *ev, listbox_t *lb, array_t *cats) {
    if (cats->len == 0)
        return;

    // Navigate Up/Down/PgUp/PgDn
    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
        lb->ipos--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
        lb->ipos++;
    if (ev->key == TB_KEY_PGUP || ev->key == TB_KEY_CTRL_U)
        lb->ipos -= lb->height/2;
    if (ev->key == TB_KEY_PGDN || ev->key == TB_KEY_CTRL_D)
        lb->ipos += lb->height/2;

    if (lb->ipos < 0)
        lb->ipos = 0;
    if (lb->ipos > cats->len-1)
        lb->ipos = cats->len-1;

    if (lb->ipos < lb->iscrollpos)
        lb->iscrollpos = lb->ipos;
    if (lb->ipos > lb->iscrollpos + lb->height-1)
        lb->iscrollpos = lb->ipos - (lb->height-1);

    if (lb->iscrollpos < 0)
        lb->iscrollpos = 0;
    if (lb->iscrollpos > cats->len-1)
        lb->iscrollpos = cats->len-1;

    assert(lb->ipos >= 0 && lb->ipos < cats->len);
}

static void draw_catlistbox(int x, int y, listbox_t *lb, array_t *cats, clr_t lb_fg, clr_t lb_bg) {
    clr_t fg, bg;
    int row;
    cat_t *cat;
    int xpad = 0;
    int height = lb->height;
    panel_t p;

    if (height > cats->len)
        height = cats->len;

    p = create_panel_frame(x,y, lb->width+2,height+2, 0,0,0,0);
//    draw_panel_shadow(&p, lb_fg,lb_bg, shadowfg,shadowbg);
    draw_panel(&p, lb_fg,lb_bg);

    x = p.content.x;
    y = p.content.y;
    row = 0;
    for (int i=lb->iscrollpos; i < cats->len; i++) {
        if (row > height-1)
            break;
        cat = cats->items[i];
        if (i == lb->ipos) {
            fg = highlightfg;
            bg = highlightbg;
        } else {
            fg = lb_fg;
            bg = lb_bg;
        }
        print_text_padded(cat->name->s, x,y, lb->width, xpad, fg,bg);

        y++;
        row++;
    }
}

static void update(struct tb_event *ev) {
    if (editxp_show) {
        update_editxp(ev);
        return;
    }
    update_listxp(ev);
}

static void draw() {
    tb_clear();

    draw_shell();
    draw_listxp();
    if (editxp_show) {
        draw_editxp();
    }

    tb_present();
}

static exp_t *listxp_selected_expense() {
    if (listxp_xps->len == 0)
        return NULL;

    assert(listxp_ixps >= 0 && listxp_ixps < listxp_xps->len);
    return listxp_xps->items[listxp_ixps];
}

static void set_cat_listbox(listbox_t *lb, uint64_t catid) {
    lb->ipos = 0;
    for (int i=0; i < cats->len; i++) {
        cat_t *cat = cats->items[i];
        if (cat->catid == catid) {
            lb->ipos = i;
            break;
        }
    }
}
static cat_t *get_selected_cat_from_listbox(listbox_t *lb) {
    cat_t *cat;

    if (cats->len == 0)
        return NULL;

    assert(lb->ipos >= 0 && lb->ipos < cats->len);
    cat = cats->items[lb->ipos];
    return cat;
}

static void init_editxp(exp_t *xp) {
    int maxchars;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];

    entry_set_text(&editxp_entry_desc, xp->desc->s);
    set_cat_listbox(&editxp_listbox_cat, xp->catid);

    snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
    entry_set_text(&editxp_entry_amt, bufamt);

    date_to_iso(xp->date, bufdate, sizeof(bufdate));
    entry_set_text(&editxp_entry_date, bufdate);

    exp_dup(_db, editxp_xp, xp);
    editxp_show = 1;
    editxp_ifocus = EDITXP_FOCUS_DESC;
    editxp_is_focus_active = 0;
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

// Restore editxp entry to its original value.
static void editxp_cancel_edit() {
    if (editxp_ifocus == EDITXP_FOCUS_DESC) {
        entry_set_text(&editxp_entry_desc, editxp_xp->desc->s);
    } else if (editxp_ifocus == EDITXP_FOCUS_AMT) {
        char bufamt[12];
        snprintf(bufamt, sizeof(bufamt), "%.2f", editxp_xp->amt);
        entry_set_text(&editxp_entry_amt, bufamt);
    } else if (editxp_ifocus == EDITXP_FOCUS_CAT) {
        set_cat_listbox(&editxp_listbox_cat, editxp_xp->catid);
    } else if (editxp_ifocus == EDITXP_FOCUS_DATE) {
        char bufdate[ISO_DATE_LEN+1];
        date_to_iso(editxp_xp->date, bufdate, sizeof(bufdate));
        entry_set_text(&editxp_entry_date, bufdate);
    }

    editxp_is_focus_active = 0;
    tb_hide_cursor();
}

// Update expense with entry field contents.
static void editxp_apply_edit() {
    if (editxp_ifocus == EDITXP_FOCUS_DESC) {
        str_assign(editxp_xp->desc, editxp_entry_desc.buf);
    } else if (editxp_ifocus == EDITXP_FOCUS_AMT) {
        float amt = strtof(editxp_entry_amt.buf, NULL);
        editxp_xp->amt = amt;
    } else if (editxp_ifocus == EDITXP_FOCUS_CAT) {
        cat_t *cat = get_selected_cat_from_listbox(&editxp_listbox_cat);
        if (cat != NULL)
            editxp_xp->catid = cat->catid;
    } else if (editxp_ifocus == EDITXP_FOCUS_DATE) {
        date_assign_iso(editxp_xp->date, editxp_entry_date.buf);
    }

    editxp_is_focus_active = 0;
    tb_hide_cursor();
}

static void update_editxp(struct tb_event *ev) {
    int ifocus = editxp_ifocus;

    // Editing field
    if (editxp_is_focus_active) {
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
            assert(ifocus >= 0 && ifocus < EDITXP_FOCUS_COUNT);
            editxp_ifocus = editxp_focus[ifocus].next;

            if (ifocus == EDITXP_FOCUS_SAVE)
                editxp_is_focus_active = 0;
            else
                editxp_is_focus_active = 1;
            return;
        }

        if (ifocus == EDITXP_FOCUS_DESC)
            update_entry(&editxp_entry_desc, ev);
        else if (ifocus == EDITXP_FOCUS_AMT)
            update_entry(&editxp_entry_amt, ev);
        else if (ifocus == EDITXP_FOCUS_DATE)
            update_entry(&editxp_entry_date, ev);
        else if (ifocus == EDITXP_FOCUS_CAT)
            update_catlistbox(ev, &editxp_listbox_cat, cats);
        return;
    }

    // ESC to close panel without saving changes
    if (ev->key == TB_KEY_ESC) {
        editxp_show = 0;
        return;
    }
    // Enter/Space pressed
    if (ev->key == TB_KEY_ENTER || ev->ch == ' ') {
        if (ifocus == EDITXP_FOCUS_DESC) {
            editxp_entry_desc.icur = 0;
            editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_AMT) {
            editxp_entry_amt.icur = 0;
            editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_DATE) {
            editxp_entry_date.icur = 0;
            editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_CAT) {
            editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_SAVE) {
            // Save edits to listxp and db.
            exp_dup(_db, listxp_selected_expense(), editxp_xp);
            db_edit_exp(_db, editxp_xp);
            editxp_show = 0;
            return;
        }
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
    int row;

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

    row = 0;
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

static void draw_editxp_entry_row(int x, int y, char *label, int label_width, int focus_ctrl) {
    clr_t fg, bg;
    int show_cursor;
    entry_t *entry;

    if (focus_ctrl == EDITXP_FOCUS_DESC)
        entry = &editxp_entry_desc;
    else if (focus_ctrl == EDITXP_FOCUS_AMT)
        entry = &editxp_entry_amt;
    else if (focus_ctrl == EDITXP_FOCUS_DATE)
        entry = &editxp_entry_date;
    else return;

    fg = popupfg;
    bg = popupbg;
    show_cursor = 0; 
    print_text(label, x,y, label_width, fg,bg);
    x += label_width;
    if (editxp_ifocus == focus_ctrl) {
        fg = highlightfg;
        bg = highlightbg;
        if (editxp_is_focus_active) {
            fg = editfieldfg;
            bg = editfieldbg;
            show_cursor = 1;
        }
    }
    draw_entry(entry, x,y, show_cursor, fg,bg);
}

static void draw_editxp() {
    exp_t *xp;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];
    char bufprompt[1024];
    int x,y, y_cat_listbox;
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

    draw_editxp_entry_row(x,y, xp_col_names[XP_COL_DESC], label_width, EDITXP_FOCUS_DESC);
    y++;
    draw_editxp_entry_row(x,y, xp_col_names[XP_COL_AMT], label_width, EDITXP_FOCUS_AMT);
    y++;

    fg = popupfg;
    bg = popupbg;
    print_text(xp_col_names[XP_COL_CAT], x,y, label_width, fg,bg);
    x += label_width;
    if (editxp_ifocus == EDITXP_FOCUS_CAT) {
        fg = highlightfg;
        bg = highlightbg;
    }
    char *catname = "";
    cat_t *cat = get_selected_cat_from_listbox(&editxp_listbox_cat);
    if (cat != NULL)
        print_text(cat->name->s, x,y, editxp_listbox_cat.width, fg,bg);
    else
        print_text("none", x,y, editxp_listbox_cat.width, fg,bg);
    y_cat_listbox = y;
    y++;

    x = editxp.content.x;
    draw_editxp_entry_row(x,y, xp_col_names[XP_COL_DATE], label_width, EDITXP_FOCUS_DATE);
    y++;

    fg = popupfg;
    bg = popupbg;
    if (editxp_ifocus == EDITXP_FOCUS_SAVE) {
        fg = highlightfg;
        bg = highlightbg;
    }
    print_text_center("[Save]", editxp.content.x,editxp.content.y+editxp.content.height-1, editxp.content.width, fg,bg);

    if (editxp_ifocus == EDITXP_FOCUS_CAT && editxp_is_focus_active)
        //draw_catlistbox(x+label_width,y_cat_listbox, &editxp_listbox_cat, cats, textfg,textbg);
        draw_catlistbox(x+label_width,y_cat_listbox, &editxp_listbox_cat, cats, popupfg,popupbg);
}

