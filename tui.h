#ifndef TUI_H
#define TUI_H

#include <stdlib.h>
#include <stdio.h>
#include "termbox2.h"
#include "clib.h"

void tui_start(sqlite3 *db, char *dbfile);

#endif
