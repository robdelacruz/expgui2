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

    draw_ch_horz(" ", x,y, width, fg,bg);
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

panel_t create_panel(int x, int y, int width, int height, int leftpad, int rightpad, int toppad, int bottompad) {
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
panel_t create_panel_center(int width, int height, int leftpad, int rightpad, int toppad, int bottompad) {
    panel_t p;

    assert(width > leftpad + rightpad + 1);
    assert(height > toppad + bottompad + 1);

    p.frame.x = tb_width()/2 - width/2;
    p.frame.y = tb_height()/2 - height/2;
    p.frame.width = width;
    p.frame.height = height;

    p.content.x = p.frame.x + 1 + leftpad;
    p.content.y = p.frame.y + 1 + toppad;
    p.content.width = p.frame.width - 2 - leftpad - rightpad;
    p.content.height = p.frame.height - 2 - toppad - bottompad;

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

