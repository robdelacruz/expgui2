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

#define ASC_TOPLEFT     "┌"
#define ASC_TOPRIGHT    "┐"
#define ASC_BOTTOMLEFT  "└"
#define ASC_BOTTOMRIGHT "┘"
#define ASC_HORZLINE    "─"
#define ASC_VERTLINE    "│"
#define ASC_TOPT        "┬"
#define ASC_BOTTOMT     "┴"
#define ASC_LEFTT       "├"
#define ASC_RIGHTT      "┤"
#define ASC_PLUS        "┼"
#define ASC_ELLIPSIS    "…"
#define ASC_BLOCK_LOW   "░"
#define ASC_BLOCK_MED   "▒"
#define ASC_BLOCK_HIGH  "▓"
#define ASC_BLOCKFULL   "█"
#define ASC_BLOCKTOP    "▀"
#define ASC_BLOCKBOTTOM "▄"
#define ASC_BLOCKCENTER "■"
#define ASC_SPACE       ' '

void draw_ch(char *ch, uint x, uint y, clr_t fg, clr_t bg) {
    tb_print(x,y, fg,bg, ch);
}
void draw_ch_horz(char *ch, uint x, uint y, uint dx, clr_t fg, clr_t bg) {
    for (int i=x; i < x+dx; i++)
        tb_print(i,y, fg,bg, ch);
}
void draw_ch_vert(char *ch, uint x, uint y, uint dy, clr_t fg, clr_t bg) {
    for (int i=y; i < y+dy; i++)
        tb_print(x,i, fg,bg, ch);
}
void draw_clear(uint x, uint y, uint width, uint height, clr_t bg) {
    for (int j=y; j < y+height; j++) {
        for (int i=x; i < x+width; i++)
            tb_set_cell(i,j, ASC_SPACE, TB_DEFAULT,bg);
    }
}
void draw_box(uint x, uint y, uint width, uint height, clr_t fg, clr_t bg) {
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
void draw_box_fill(uint x, uint y, uint width, uint height, clr_t fg, clr_t bg) {
    draw_clear(x,y, width,height, bg);
    draw_box(x,y, width,height, fg,bg);
}
void draw_divider_vert(uint x, uint y, uint height, clr_t fg, clr_t bg) {
    draw_ch(ASC_TOPT, x,y, fg,bg);
    draw_ch_vert(ASC_VERTLINE, x,y+1, height-2, fg,bg);
    draw_ch(ASC_BOTTOMT, x,y+height-1, fg,bg);
}
void draw_divider_horz(uint x, uint y, uint width, clr_t fg, clr_t bg) {
    draw_ch(ASC_LEFTT, x,y, fg,bg);
    draw_ch_horz(ASC_HORZLINE, x+1,y, width-2, fg,bg);
    draw_ch(ASC_RIGHTT, x+width-1,y, fg,bg);
}

void print_text(char *s, uint x, uint y, size_t width, uint xpad, clr_t fg, clr_t bg) {
    size_t s_len = strlen(s);
    uint xend = x+width-1;
    if (width == 0)
        width = s_len;

    draw_ch_horz(" ", x,y, xpad, fg,bg);
    x+=xpad;

    for (int i=0; i < s_len; i++) {
        if (x == xend-xpad && i != s_len-1) {
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

