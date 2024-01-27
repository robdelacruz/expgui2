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
    int z;
    sqlite3 *db = NULL;
    char *dbfile = NULL;
    array_t *xps;
    GtkWidget *w;

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

    gtk_init(&argc, &argv);
    w = mainwin_new();

    gtk_widget_show_all(w);

    gtk_main();
}

