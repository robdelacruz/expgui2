#include "termbox2.h"

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

typedef uintattr_t clr_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} rect_t;

rect_t inner_rect(rect_t r);
rect_t outer_rect(rect_t r);

void draw_ch(char *ch, int x, int y, clr_t fg, clr_t bg);
void draw_ch_horz(char *ch, int x, int y, int dx, clr_t fg, clr_t bg);
void draw_ch_vert(char *ch, int x, int y, int dy, clr_t fg, clr_t bg);
void draw_clear(int x, int y, int width, int height, clr_t bg);
void draw_box(int x, int y, int width, int height, clr_t fg, clr_t bg);
void draw_box_fill(int x, int y, int width, int height, clr_t fg, clr_t bg);
void draw_divider_vert(int x, int y, int height, clr_t fg, clr_t bg);
void draw_divider_horz(int x, int y, int width, clr_t fg, clr_t bg);

void print_text(char *s, int x, int y, size_t width, clr_t fg, clr_t bg);

