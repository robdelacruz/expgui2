#include <assert.h>
#include "sqlite3/sqlite3.h"
#include "tui.h"
#include "tuilib.h"
#include "clib.h"
#include "db.h"

#define CLR_FG TB_GREEN
#define CLR_BG TB_BLACK
#define BOX_FG TB_WHITE
#define BOX_BG TB_BLUE
#define CLR_HIGHLIGHT_FG TB_BLACK
#define CLR_HIGHLIGHT_BG TB_WHITE

void tui_start(sqlite3 *db, char *dbfile) {
    struct tb_event ev;
    array_t *xps = array_new(0);

    tb_init();
    tb_set_input_mode(TB_INPUT_ALT);
    tb_set_clear_attrs(CLR_FG, CLR_BG);

    rect_t screenrect = {0,0, tb_width(), tb_height()-1};
    uint statusx = 0;
    uint statusy = tb_height()-1;
    char *longtext = "This is some text that took me a long time to write.";

    db_select_exp(db, xps);

    tb_clear();
    tb_printf(statusx, statusy, TB_DEFAULT,TB_DEFAULT, "Status goes here <--");
    draw_box(screenrect.x, screenrect.y, screenrect.width, screenrect.height, BOX_FG,BOX_BG);
    tb_present();

    while (1) {
        tb_poll_event(&ev);
        if (ev.key == TB_KEY_CTRL_C) 
            break;
        if (ev.ch == 'q') 
            break;

        tb_clear();
        print_text(longtext, statusx,statusy, 80, TB_DEFAULT,TB_DEFAULT);
        draw_box(screenrect.x, screenrect.y, screenrect.width, screenrect.height, BOX_FG,BOX_BG);
        tb_present();
    }

    tb_shutdown();
}



