#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "clib.h"
#include "db.h"
#include "mainwin.h"

int main(int argc, char *argv[]) {
    GtkWidget *w;
    int z;

    gtk_init(&argc, &argv);
    w = mainwin_new();

    gtk_main();
    return 0;
}

#if 0
int main(int argc, char *argv[]) {
    int z;
    sqlite3 *db;
    char *dbfile = NULL;
    exp_t *xp;
    array_t *xps;
    char isodate[ISO_DATE_LEN+1];

    for (int i=1; i < argc; i++) {
        char *s = argv[i];

        // -i dbfile
        if (strcmp(s, "-i") == 0 && i < argc-1) {
            z = create_dbfile(argv[i+1]);
            if (z != 0)
                exit(1);
            i++;
            break;
        }

        // dbfile
        dbfile = s;
    }

    if (dbfile == NULL)
        exit(0);

    printf("dbfile: %s\n", dbfile);
    z = open_dbfile(dbfile, &db);
    if (z != 0) {
        exit(1);
    }

    xp = exp_new();
    xp->expid = 0;
    xp->date = date_new_today();
    xp->desc = str_new(0);
    str_assign(xp->desc, "test");
    xp->amt = 1.23;
    xp->catid = 0;
    db_add_exp(db, xp);

    xps = array_new(0);
    db_select_exp(db, xps);

    for (int i=0; i < xps->len; i++) {
        xp = xps->items[i];
        date_to_iso(xp->date, isodate, sizeof(isodate));
        printf("%ld %s %s %f %ld\n",
                xp->expid,
                isodate,
                xp->desc->s,
                xp->amt,
                xp->catid);
    }

    sqlite3_close_v2(db);

    return 0;
}
#endif

