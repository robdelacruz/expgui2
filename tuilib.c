#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "tuilib.h"
#include "clib.h"

// Box characters
// ┌────┬────┐
// │    │    │
// ├────┼────┤
// │    │    │
// └────┴────┘

// Shade chars
// █  ░▒▓█ 
//
// ▀ ▄ ■

// Other chars
// …  (ellipsis)
//
//

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

void set_output_mode(int mode) {
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

rect_t inner_rect(rect_t r) {
    if (r.width > 2) {
        r.x++;
        r.width -= 2;
    }
    if (r.height > 2) {
        r.y++;
        r.height -= 2;
    }
    return r;
}
rect_t outer_rect(rect_t r) {
    r.x--;
    r.y--;
    r.width += 2;
    r.height += 2;
    return r;
}
void center_screen_rect(rect_t *r) {
    r->x = tb_width()/2 - r->width/2;
    r->y = tb_height()/2 - r->height/2;
}

void draw_ch(char *ch, int x, int y, clr_t fg, clr_t bg) {
    tb_print(x,y, fg,bg, ch);
}
void draw_ch_horz(char *ch, int x, int y, int dx, clr_t fg, clr_t bg) {
    for (int i=x; i < x+dx; i++)
        tb_print(i,y, fg,bg, ch);
}
void draw_ch_vert(char *ch, int x, int y, int dy, clr_t fg, clr_t bg) {
    for (int i=y; i < y+dy; i++)
        tb_print(x,i, fg,bg, ch);
}
void draw_clear(rect_t r, clr_t bg) {
    int x = r.x;
    int y = r.y;
    int width = r.width;
    int height = r.height;

    for (int j=y; j < y+height; j++) {
        for (int i=x; i < x+width; i++)
            tb_set_cell(i,j, ASC_SPACE, TB_DEFAULT,bg);
    }
}
void draw_box(rect_t r, clr_t fg, clr_t bg) {
    int x = r.x;
    int y = r.y;
    int width = r.width;
    int height = r.height;

    if (width <= 2 || height <= 2)
        return;

    draw_ch(ASC_TOPLEFT, x,y, fg,bg);
    draw_ch(ASC_TOPRIGHT, x+width-1,y, fg,bg);
    draw_ch(ASC_BOTTOMLEFT, x,y+height-1, fg,bg);
    draw_ch(ASC_BOTTOMRIGHT, x+width-1,y+height-1, fg,bg);
    draw_ch_horz(ASC_HORZLINE, x+1,y, width-2, fg,bg);
    draw_ch_horz(ASC_HORZLINE, x+1,y+height-1, width-2, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x,y+1, height-2, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x+width-1,y+1, height-2, fg,bg);
}
void draw_box_ch(rect_t r, clr_t fg, clr_t bg, char *ch) {
    int x = r.x;
    int y = r.y;
    int width = r.width;
    int height = r.height;

    if (width <= 2 || height <= 2)
        return;

    draw_ch(ch, x,y, fg,bg);
    draw_ch(ch, x+width-1,y, fg,bg);
    draw_ch(ch, x,y+height-1, fg,bg);
    draw_ch(ch, x+width-1,y+height-1, fg,bg);
    draw_ch_horz(ch, x+1,y, width-2, fg,bg);
    draw_ch_horz(ch, x+1,y+height-1, width-2, fg,bg);
    draw_ch_vert(ch, x,y+1, height-2, fg,bg);
    draw_ch_vert(ch, x+width-1,y+1, height-2, fg,bg);
}
void draw_box_fill(rect_t r, clr_t fg, clr_t bg) {
    draw_clear(r, bg);
    draw_box(r, fg,bg);
}
void draw_divider_vert(int x, int y, int height, clr_t fg, clr_t bg) {
    draw_ch(ASC_TOPT, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x,y+1, height-2, fg,bg);
    draw_ch(ASC_BOTTOMT, x,y+height-1, fg,bg);
}
void draw_divider_horz(int x, int y, int width, clr_t fg, clr_t bg) {
    draw_ch(ASC_LEFTT, x,y, fg,bg);
    draw_ch_horz(ASC_HORZLINE, x+1,y, width-2, fg,bg);
    draw_ch(ASC_RIGHTT, x+width-1,y, fg,bg);
}

void print_text(char *s, int x, int y, size_t width, clr_t fg, clr_t bg) {
    size_t s_len;
    int xend;

    s_len = strlen(s);
    if (width == 0)
        width = s_len;
    xend = x+width-1;

    for (int i=0; i < s_len; i++) {
        if (x == xend && i != s_len-1) {
            draw_ch(ASC_ELLIPSIS, x,y, fg,bg);
            x++;
            break;
        }
        tb_set_cell(x,y, s[i], fg,bg);
        x++;
    }
    while (x <= xend) {
        tb_set_cell(x,y, ASC_SPACE, fg,bg);
        x++;
    }
}
void print_text_center(char *s, int x, int y, size_t width, clr_t fg, clr_t bg) {
    size_t s_len = strlen(s);

    if (s_len >= width) {
        print_text(s, x,y, width, fg,bg);
        return;
    }

    //draw_ch_horz(" ", x,y, width, fg,bg);
    x += width/2 - s_len/2;
    tb_print(x,y, fg,bg, s);
}
void print_text_right(char *s, int x, int y, size_t width, clr_t fg, clr_t bg) {
    size_t s_len = strlen(s);

    if (s_len >= width) {
        print_text(s, x,y, width, fg,bg);
        return;
    }

    draw_ch_horz(" ", x,y, width, fg,bg);
    x = x + width - s_len;
    tb_print(x,y, fg,bg, s);
}
void print_text_padded(char *s, int x, int y, size_t width, int xpad, clr_t fg, clr_t bg) {
    draw_ch_horz(" ", x,y, xpad, fg,bg);
    x += xpad;
    print_text(s, x,y, width, fg,bg);
    x += width;
    draw_ch_horz(" ", x,y, xpad, fg,bg);
    x += xpad;
}
void print_text_padded_center(char *s, int x, int y, size_t width, int xpad, clr_t fg, clr_t bg) {
    draw_ch_horz(" ", x,y, xpad, fg,bg);
    x += xpad;
    print_text_center(s, x,y, width, fg,bg);
    x += width;
    draw_ch_horz(" ", x,y, xpad, fg,bg);
    x += xpad;
}

panel_t create_panel_frame(int x, int y, int width, int height, int leftpad, int rightpad, int toppad, int bottompad) {
    panel_t p;

    assert(width > leftpad + rightpad + 1);
    assert(height > toppad + bottompad + 1);

    p.frame.x = x;
    p.frame.y = y;
    p.frame.width = width;
    p.frame.height = height;

    p.content.x = p.frame.x + 1 + leftpad;
    p.content.y = p.frame.y + 1 + toppad;
    p.content.width = p.frame.width - 2 - leftpad - rightpad;
    p.content.height = p.frame.height - 2 - toppad - bottompad;

    return p;
}
panel_t create_panel_center(int content_width, int content_height, int leftpad, int rightpad, int toppad, int bottompad) {
    panel_t p;

    p.frame.width = content_width + leftpad + rightpad + 2;
    p.frame.height = content_height + toppad + bottompad + 2;
    p.frame.x = tb_width()/2 - p.frame.width/2;
    p.frame.y = tb_height()/2 - p.frame.height/2;

    p.content.width = content_width;
    p.content.height = content_height;
    p.content.x = p.frame.x + 1 + leftpad;
    p.content.y = p.frame.y + 1 + toppad;

    return p;
}
void draw_panel(panel_t *p, clr_t fg, clr_t bg) {
    draw_box_fill(p->frame, fg, bg);
}
void draw_panel_shadow(panel_t *p, clr_t fg, clr_t bg, clr_t shadowfg, clr_t shadowbg) {
    int x,y;
    draw_panel(p, fg,bg);

    x = p->frame.x + p->frame.width;
    y = p->frame.y+1;
    draw_ch_vert(ASC_BLOCK_LOW, x,y,   p->frame.height, shadowfg,shadowbg);
    draw_ch_vert(ASC_BLOCK_LOW, x+1,y, p->frame.height, shadowfg,shadowbg);

    x = p->frame.x+1;
    y = p->frame.y + p->frame.height;
    draw_ch_horz(ASC_BLOCK_LOW, x,y, p->frame.width, shadowfg,shadowbg);
}

void init_entry(entry_t *e, char *text, int maxchars) {
    if (maxchars > ENTRY_BUFSIZE)
        maxchars = ENTRY_BUFSIZE;

    strncpy(e->buf, text, maxchars);
    e->buf[maxchars+1] = 0;
    e->icur = 0;
    e->maxchars = maxchars;
}
void entry_set_text(entry_t *e, char *text) {
    strncpy(e->buf, text, e->maxchars);
    e->buf[e->maxchars] = 0;
    e->icur = 0;
}

static void del_char(char *s, size_t s_len, int ipos) {
    if (s_len == 0)
        return;
    if (ipos < 0 || ipos > s_len-1)
        return;

    for (int i=ipos; i < s_len-1; i++) {
        s[i] = s[i+1];
    }
    s[s_len-1] = 0;
}
static int insert_char(char *s, size_t s_len, size_t maxchars, int ipos, char c) {
    assert(s_len <= maxchars);

    if (ipos < 0 || ipos > s_len)
        return 0;
    if (s_len == maxchars)
        return 0;

    if (ipos == s_len) {
        s[ipos] = c;
        s[ipos+1] = 0;
        return 1;
    }

    s[s_len+1] = 0;
    for (int i=s_len; i >= ipos; i--)
        s[i] = s[i-1];
    s[ipos] = c;
    return 1;
}

void update_entry(entry_t *e, struct tb_event *ev) {
    size_t buf_len = strlen(e->buf);

    if (ev->key == TB_KEY_ARROW_LEFT)
        e->icur--;
    if (ev->key == TB_KEY_ARROW_RIGHT)
        e->icur++;

    if (ev->key == TB_KEY_BACKSPACE || ev->key == TB_KEY_BACKSPACE2) {
        if (e->icur > 0) {
            e->icur--;
            del_char(e->buf, buf_len, e->icur);
        }
    }
    if (ev->key == TB_KEY_DELETE) {
        del_char(e->buf, buf_len, e->icur);
    }

    if (e->icur < 0)
        e->icur = 0;
    if (e->icur > buf_len)
        e->icur = buf_len;

    // ascii printable char 32(space) - 126(~)
    if (ev->ch >= 32 && ev->ch <= 126) {
        char ch = (char) ev->ch;
        if (insert_char(e->buf, buf_len, e->maxchars, e->icur, ch))
            e->icur++;
    }

    if (e->icur > e->maxchars-1)
        e->icur = e->maxchars-1;
}

void draw_entry(entry_t *e, int x, int y, int show_cursor, clr_t fg, clr_t bg) {
    print_text(e->buf, x,y, e->maxchars, fg,bg);
    if (show_cursor)
        tb_set_cursor(x+e->icur, y);
}

