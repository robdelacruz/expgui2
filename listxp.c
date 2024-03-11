#include <assert.h>
#include "clib.h"
#include "db.h"
#include "tuilib.h"
#include "tui.h"

exp_t *listxp_selected_expense(tui_t *t) {
    if (t->listxp_xps->len == 0)
        return NULL;

    assert(t->listxp_ixps >= 0 && t->listxp_ixps < t->listxp_xps->len);
    return t->listxp_xps->items[t->listxp_ixps];
}

void draw_listxp(tui_t *t) {
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

    t->listxp = create_panel_frame(0,1, tb_width(), tb_height()-2, 0,0, 1,1);
    draw_panel(&t->listxp, textfg,textbg);

    // Add padding of one space to left and right of each field
    xpad = 1;

    // Amount, Category, Date are fixed width fields
    // Description field width will consume the remaining horizontal space.
    desc_maxwidth = t->listxp.content.width;
    desc_maxwidth -= col_widths[XP_COL_AMT];
    desc_maxwidth -= col_widths[XP_COL_CAT];
    desc_maxwidth -= col_widths[XP_COL_DATE];
    desc_maxwidth -= XP_COL_COUNT-1;        // space for column separator
    desc_maxwidth -= (XP_COL_COUNT)*xpad*2; // space for padding on left/right side.
    col_widths[XP_COL_DESC] = desc_maxwidth;

    // Print column headings
    x = t->listxp.content.x;
    y = t->listxp.frame.y+1;
    fg = textfg;
    bg = textbg;
    for (int i=XP_COL_DESC; i < XP_COL_COUNT; i++) {
        print_text_padded_center(t->xp_col_names[i], x,y, col_widths[i], xpad, colfg,colbg);
        x += xpad + col_widths[i] + xpad;
        if (i != XP_COL_COUNT-1) {
            draw_ch(ASC_VERTLINE, x,y, fg,bg);
            draw_ch_vert(ASC_VERTLINE, x, t->listxp.content.y, t->listxp.content.height, fg,bg);
            x++;
        }
    }

    x = t->listxp.content.x;
    y = t->listxp.content.y;

    row = 0;
    for (int i=t->listxp_scrollpos; i < t->listxp_xps->len; i++) {
        if (row > t->listxp.content.height-1)
            break;
        xp = t->listxp_xps->items[i];
        if (i == t->listxp_ixps) {
            fg = highlightfg;
            bg = highlightbg;
        } else {
            fg = textfg;
            bg = textbg;
        }

        // Print one row of columns.
        for (int icol=XP_COL_DESC; icol < XP_COL_COUNT; icol++) {
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
        assert(x == t->listxp.content.x + t->listxp.content.width);

        y++;
        x = t->listxp.content.x;
        row++;
    }

    // Status line
    x = t->listxp.content.x;
    y = t->listxp.content.y + t->listxp.content.height;
    draw_ch_horz(" ", x,y, t->listxp.content.width, statusfg,statusbg);

    xp = listxp_selected_expense(t);
    if (xp != NULL) {
        snprintf(buf, sizeof(buf), "%.*s %.2f", 50, xp->desc->s, xp->amt);
        print_text(buf, x,y, t->listxp.content.width, statusfg,statusbg);

        snprintf(buf, sizeof(buf), "Expense %d/%ld", t->listxp_ixps+1, t->listxp_xps->len);
        x = t->listxp.content.x + t->listxp.content.width - strlen(buf);
        tb_print(x,y, statusfg,statusbg, buf);
    }
}

void update_listxp(tui_t *t, struct tb_event *ev) {
    if (t->listxp_xps->len == 0)
        return;

    // Enter to edit expense row
    if (ev->key == TB_KEY_ENTER) {
        exp_t *xp = listxp_selected_expense(t);
        if (xp == NULL)
            return;
        init_editxp(t, xp);
        return;
    }

    // Navigate expense list Up/Down/PgUp/PgDn
    if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k')
        t->listxp_ixps--;
    if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j')
        t->listxp_ixps++;
    if (ev->key == TB_KEY_PGUP || ev->key == TB_KEY_CTRL_U)
        t->listxp_ixps -= t->listxp.content.height/2;
    if (ev->key == TB_KEY_PGDN || ev->key == TB_KEY_CTRL_D)
        t->listxp_ixps += t->listxp.content.height/2;

    if (t->listxp_ixps < 0)
        t->listxp_ixps = 0;
    if (t->listxp_ixps > t->listxp_xps->len-1)
        t->listxp_ixps = t->listxp_xps->len-1;

    if (t->listxp_ixps < t->listxp_scrollpos)
        t->listxp_scrollpos = t->listxp_ixps;
    if (t->listxp_ixps > t->listxp_scrollpos + t->listxp.content.height-1)
        t->listxp_scrollpos = t->listxp_ixps - (t->listxp.content.height-1);

    if (t->listxp_scrollpos < 0)
        t->listxp_scrollpos = 0;
    if (t->listxp_scrollpos > t->listxp_xps->len-1)
        t->listxp_scrollpos = t->listxp_xps->len-1;

    assert(t->listxp_ixps >= 0 && t->listxp_ixps < t->listxp_xps->len);
}

