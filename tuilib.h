#ifndef TUILIB_H
#define TUILIB_H

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
#define ASC_BLOCK_FULL   "█"
#define ASC_BLOCK_TOP    "▀"
#define ASC_BLOCK_BOTTOM "▄"
#define ASC_BLOCK_CENTER "■"
#define ASC_SPACE       ' '

typedef uintattr_t clr_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} rect_t;

typedef struct {
    rect_t frame;
    rect_t content;
} panel_t;

#define ENTRY_BUFSIZE 64
typedef struct {
    char buf[ENTRY_BUFSIZE+1];
    int icur;
    int maxchars;
} entry_t;

typedef struct {
    int ipos;
    int iscrollpos;
    int width;
    int height;
} listbox_t;

typedef struct {
    int prev, next;
} focus_t;

extern clr_t titlefg;
extern clr_t titlebg;
extern clr_t statusfg;
extern clr_t statusbg;
extern clr_t shadowfg;
extern clr_t shadowbg;

extern clr_t textfg;
extern clr_t textbg;
extern clr_t highlightfg;
extern clr_t highlightbg;
extern clr_t colfg;
extern clr_t colbg;
extern clr_t popupfg;
extern clr_t popupbg;

extern clr_t editfieldfg;
extern clr_t editfieldbg;
extern clr_t btnfg;
extern clr_t btnbg;

void set_output_mode(int mode);

rect_t inner_rect(rect_t r);
rect_t outer_rect(rect_t r);

void draw_ch(char *ch, int x, int y, clr_t fg, clr_t bg);
void draw_ch_horz(char *ch, int x, int y, int dx, clr_t fg, clr_t bg);
void draw_ch_vert(char *ch, int x, int y, int dy, clr_t fg, clr_t bg);
void draw_clear(rect_t r, clr_t bg);
void draw_box(rect_t r, clr_t fg, clr_t bg);
void draw_box_ch(rect_t r, clr_t fg, clr_t bg, char *ch);
void draw_box_fill(rect_t r, clr_t fg, clr_t bg);
void draw_divider_vert(int x, int y, int height, clr_t fg, clr_t bg);
void draw_divider_horz(int x, int y, int width, clr_t fg, clr_t bg);

void print_text(char *s, int x, int y, size_t width, clr_t fg, clr_t bg);
void print_text_center(char *s, int x, int y, size_t width, clr_t fg, clr_t bg);
void print_text_right(char *s, int x, int y, size_t width, clr_t fg, clr_t bg);
void print_text_padded(char *s, int x, int y, size_t width, int xpad, clr_t fg, clr_t bg);
void print_text_padded_center(char *s, int x, int y, size_t width, int xpad, clr_t fg, clr_t bg);

panel_t create_panel_frame(int x, int y, int width, int height, int leftpad, int rightpad, int toppad, int bottompad);
panel_t create_panel_center(int content_width, int content_height, int leftpad, int rightpad, int toppad, int bottompad);
void draw_panel(panel_t *p, clr_t fg, clr_t bg);
void draw_panel_shadow(panel_t *p, clr_t fg, clr_t bg, clr_t shadowfg, clr_t shadowbg);

void init_entry(entry_t *e, char *text, int maxchars);
void entry_set_text(entry_t *e, char *text);
void update_entry(entry_t *e, struct tb_event *ev);
void draw_entry(entry_t *e, int x, int y, int show_cursor, clr_t fg, clr_t bg) ;

#endif
