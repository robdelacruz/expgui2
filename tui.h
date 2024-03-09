#ifndef TUI_H
#define TUI_H

#include <stdlib.h>
#include <stdio.h>
#include "termbox2.h"
#include "clib.h"
#include "tuilib.h"

#define XP_COL_START 0
#define XP_COL_DESC 0
#define XP_COL_AMT 1
#define XP_COL_CAT 2
#define XP_COL_DATE 3
#define XP_COL_COUNT 4

extern char *xp_col_names[XP_COL_COUNT];
extern int xp_entry_maxchars[XP_COL_COUNT];
extern sqlite3 *_db;
extern array_t *cats;

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

void tui_start(sqlite3 *db, char *dbfile);

#endif
