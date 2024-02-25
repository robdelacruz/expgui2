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
void draw_clear(int x, int y, int width, int height, clr_t bg) {
    for (int j=y; j < y+height; j++) {
        for (int i=x; i < x+width; i++)
            tb_set_cell(i,j, ASC_SPACE, TB_DEFAULT,bg);
    }
}
void draw_box(int x, int y, int width, int height, clr_t fg, clr_t bg) {
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
void draw_box_ch(int x, int y, int width, int height, clr_t fg, clr_t bg, char *ch) {
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
void draw_box_fill(int x, int y, int width, int height, clr_t fg, clr_t bg) {
    draw_clear(x,y, width,height, bg);
    draw_box(x,y, width,height, fg,bg);
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
    int dx;

    if (s_len >= width) {
        print_text(s, x,y, width, fg,bg);
        return;
    }

    dx = width/2 - s_len/2;
    x += dx;
    width -= dx;
    print_text(s, x,y, width, fg,bg);
}

