#include "termbox2.h"

typedef uintattr_t clr_t;

typedef struct {
    uint x;
    uint y;
    uint width;
    uint height;
} rect_t;

void draw_ch(char *ch, uint x, uint y, clr_t fg, clr_t bg);
void draw_ch_horz(char *ch, uint x, uint y, uint dx, clr_t fg, clr_t bg);
void draw_ch_vert(char *ch, uint x, uint y, uint dy, clr_t fg, clr_t bg);
void draw_clear(uint x, uint y, uint width, uint height, clr_t bg);
void draw_box(uint x, uint y, uint width, uint height, clr_t fg, clr_t bg);
void draw_box_fill(uint x, uint y, uint width, uint height, clr_t fg, clr_t bg);

void print_text(char *s, uint x, uint y, size_t width, clr_t fg, clr_t bg);

