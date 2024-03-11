#include <assert.h>
#include "clib.h"
#include "db.h"
#include "tuilib.h"
#include "tui.h"

static void set_cat_listbox(listbox_t *lb, uint64_t catid, array_t *cats);

void init_editxp(tui_t *t, exp_t *xp) {
    int maxchars;
    char bufdate[ISO_DATE_LEN+1];
    char bufamt[12];

    entry_set_text(&t->editxp_entry_desc, xp->desc->s);
    set_cat_listbox(&t->editxp_listbox_cat, xp->catid, t->cats);

    snprintf(bufamt, sizeof(bufamt), "%.2f", xp->amt);
    entry_set_text(&t->editxp_entry_amt, bufamt);

    date_to_iso(xp->date, bufdate, sizeof(bufdate));
    entry_set_text(&t->editxp_entry_date, bufdate);

    exp_dup(t->db, t->editxp_xp, xp);
    t->editxp_show = 1;
    t->editxp_ifocus = EDITXP_FOCUS_DESC;
    t->editxp_is_focus_active = 0;
}

static void draw_editxp_entry_row(tui_t *t, int x, int y, char *label, int label_width, int focus_ctrl) {
    clr_t fg, bg;
    int show_cursor;
    entry_t *entry;

    if (focus_ctrl == EDITXP_FOCUS_DESC)
        entry = &t->editxp_entry_desc;
    else if (focus_ctrl == EDITXP_FOCUS_AMT)
        entry = &t->editxp_entry_amt;
    else if (focus_ctrl == EDITXP_FOCUS_DATE)
        entry = &t->editxp_entry_date;
    else return;

    fg = popupfg;
    bg = popupbg;
    show_cursor = 0; 
    print_text(label, x,y, label_width, fg,bg);
    x += label_width;
    if (t->editxp_ifocus == focus_ctrl) {
        fg = highlightfg;
        bg = highlightbg;
        if (t->editxp_is_focus_active) {
            fg = editfieldfg;
            bg = editfieldbg;
            show_cursor = 1;
        }
    }
    draw_entry(entry, x,y, show_cursor, fg,bg);
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

static cat_t *get_selected_cat_from_listbox(listbox_t *lb, array_t *cats) {
    cat_t *cat;

    if (cats->len == 0)
        return NULL;

    assert(lb->ipos >= 0 && lb->ipos < cats->len);
    cat = cats->items[lb->ipos];
    return cat;
}

void draw_editxp(tui_t *t) {
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
    size_t label_width = strlen(t->xp_col_names[XP_COL_DESC])+1;
    size_t field_width = t->xp_col_maxchars[XP_COL_DESC];
    clr_t fg,bg;

    xp = t->editxp_xp;

    panel_height = XP_COL_COUNT + 2;
    t->editxp = create_panel_center(label_width + field_width, panel_height, panel_leftpad, panel_rightpad, panel_toppad, panel_bottompad);
    draw_panel_shadow(&t->editxp, popupfg,popupbg, shadowfg,shadowbg);
    print_text_center("Edit Expense", t->editxp.content.x,t->editxp.frame.y+1, t->editxp.content.width, popupfg | TB_BOLD,popupbg);

    x = t->editxp.content.x;
    y = t->editxp.content.y;

    draw_editxp_entry_row(t, x,y, t->xp_col_names[XP_COL_DESC], label_width, EDITXP_FOCUS_DESC);
    y++;
    draw_editxp_entry_row(t, x,y, t->xp_col_names[XP_COL_AMT], label_width, EDITXP_FOCUS_AMT);
    y++;

    fg = popupfg;
    bg = popupbg;
    print_text(t->xp_col_names[XP_COL_CAT], x,y, label_width, fg,bg);
    x += label_width;
    if (t->editxp_ifocus == EDITXP_FOCUS_CAT) {
        fg = highlightfg;
        bg = highlightbg;
    }
    char *catname = "";
    cat_t *cat = get_selected_cat_from_listbox(&t->editxp_listbox_cat, t->cats);
    if (cat != NULL)
        print_text(cat->name->s, x,y, t->editxp_listbox_cat.width, fg,bg);
    else
        print_text("none", x,y, t->editxp_listbox_cat.width, fg,bg);
    y_cat_listbox = y;
    y++;

    x = t->editxp.content.x;
    draw_editxp_entry_row(t, x,y, t->xp_col_names[XP_COL_DATE], label_width, EDITXP_FOCUS_DATE);
    y++;

    fg = popupfg;
    bg = popupbg;
    if (t->editxp_ifocus == EDITXP_FOCUS_SAVE) {
        fg = highlightfg;
        bg = highlightbg;
    }
    print_text_center("[Save]", t->editxp.content.x,t->editxp.content.y+t->editxp.content.height-1, t->editxp.content.width, fg,bg);

    if (t->editxp_ifocus == EDITXP_FOCUS_CAT && t->editxp_is_focus_active)
        draw_catlistbox(x+label_width,y_cat_listbox, &t->editxp_listbox_cat, t->cats, popupfg,popupbg);
}

static void set_cat_listbox(listbox_t *lb, uint64_t catid, array_t *cats) {
    lb->ipos = 0;
    lb->iscrollpos = 0;
    for (int i=0; i < cats->len; i++) {
        cat_t *cat = cats->items[i];
        if (cat->catid == catid) {
            lb->ipos = i;
            break;
        }
    }
    // If ipos is not visible, scroll until it's visible.
    while (lb->ipos > lb->iscrollpos+lb->height-1 && lb->iscrollpos < cats->len-1)
        lb->iscrollpos++;
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
// Restore editxp entry to its original value.
static void editxp_cancel_edit(tui_t *t) {
    if (t->editxp_ifocus == EDITXP_FOCUS_DESC) {
        entry_set_text(&t->editxp_entry_desc, t->editxp_xp->desc->s);
    } else if (t->editxp_ifocus == EDITXP_FOCUS_AMT) {
        char bufamt[12];
        snprintf(bufamt, sizeof(bufamt), "%.2f", t->editxp_xp->amt);
        entry_set_text(&t->editxp_entry_amt, bufamt);
    } else if (t->editxp_ifocus == EDITXP_FOCUS_CAT) {
        set_cat_listbox(&t->editxp_listbox_cat, t->editxp_xp->catid, t->cats);
    } else if (t->editxp_ifocus == EDITXP_FOCUS_DATE) {
        char bufdate[ISO_DATE_LEN+1];
        date_to_iso(t->editxp_xp->date, bufdate, sizeof(bufdate));
        entry_set_text(&t->editxp_entry_date, bufdate);
    }

    t->editxp_is_focus_active = 0;
    tb_hide_cursor();
}
// Update expense with entry field contents.
static void editxp_apply_edit(tui_t *t) {
    if (t->editxp_ifocus == EDITXP_FOCUS_DESC) {
        str_assign(t->editxp_xp->desc, t->editxp_entry_desc.buf);
    } else if (t->editxp_ifocus == EDITXP_FOCUS_AMT) {
        float amt = strtof(t->editxp_entry_amt.buf, NULL);
        t->editxp_xp->amt = amt;
    } else if (t->editxp_ifocus == EDITXP_FOCUS_CAT) {
        cat_t *cat = get_selected_cat_from_listbox(&t->editxp_listbox_cat, t->cats);
        if (cat != NULL)
            t->editxp_xp->catid = cat->catid;
    } else if (t->editxp_ifocus == EDITXP_FOCUS_DATE) {
        date_assign_iso(t->editxp_xp->date, t->editxp_entry_date.buf);
    }

    t->editxp_is_focus_active = 0;
    tb_hide_cursor();
}

void update_editxp(tui_t *t, struct tb_event *ev) {
    int ifocus = t->editxp_ifocus;

    // Editing field
    if (t->editxp_is_focus_active) {
        // ESC to cancel edits
        if (ev->key == TB_KEY_ESC) {
            editxp_cancel_edit(t);
            return;
        } 
        // Enter to apply edits
        if (ev->key == TB_KEY_ENTER) {
            editxp_apply_edit(t);
            return;
        }
        // Tab/down to apply edits and switch to next field in edit mode.
        if (ev->key == TB_KEY_TAB || ev->key == TB_KEY_ARROW_DOWN) {
            editxp_apply_edit(t);

            // Switch to next focus
            assert(ifocus >= 0 && ifocus < EDITXP_FOCUS_COUNT);
            t->editxp_ifocus = t->editxp_focus[ifocus].next;

            if (ifocus == EDITXP_FOCUS_SAVE)
                t->editxp_is_focus_active = 0;
            else
                t->editxp_is_focus_active = 1;
            return;
        }

        if (ifocus == EDITXP_FOCUS_DESC)
            update_entry(&t->editxp_entry_desc, ev);
        else if (ifocus == EDITXP_FOCUS_AMT)
            update_entry(&t->editxp_entry_amt, ev);
        else if (ifocus == EDITXP_FOCUS_DATE)
            update_entry(&t->editxp_entry_date, ev);
        else if (ifocus == EDITXP_FOCUS_CAT)
            update_catlistbox(ev, &t->editxp_listbox_cat, t->cats);
        return;
    }

    // ESC to close panel without saving changes
    if (ev->key == TB_KEY_ESC) {
        t->editxp_show = 0;
        return;
    }
    // Enter/Space pressed
    if (ev->key == TB_KEY_ENTER || ev->ch == ' ') {
        if (ifocus == EDITXP_FOCUS_DESC) {
            t->editxp_entry_desc.icur = 0;
            t->editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_AMT) {
            t->editxp_entry_amt.icur = 0;
            t->editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_DATE) {
            t->editxp_entry_date.icur = 0;
            t->editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_CAT) {
            t->editxp_is_focus_active = 1;
            return;
        } else if (ifocus == EDITXP_FOCUS_SAVE) {
            // Save edits to listxp and db.
            exp_dup(t->db, listxp_selected_expense(t), t->editxp_xp);
            db_edit_exp(t->db, t->editxp_xp);
            t->editxp_show = 0;
            return;
        }
    }
    // j/k, Up/Down, Tab to switch focus
    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k' || (ev->mod == TB_MOD_SHIFT && ev->key == TB_KEY_TAB)) {
        assert(t->editxp_ifocus >= 0 && t->editxp_ifocus < EDITXP_FOCUS_COUNT);
        t->editxp_ifocus = t->editxp_focus[t->editxp_ifocus].prev;
        return;
    }
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j' || ev->key == TB_KEY_TAB) {
        assert(t->editxp_ifocus >= 0 && t->editxp_ifocus < EDITXP_FOCUS_COUNT);
        t->editxp_ifocus = t->editxp_focus[t->editxp_ifocus].next;
        return;
    }
}

