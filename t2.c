#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "clib.h"
#include "db.h"
#include "tui.h"

int main(int argc, char *argv[]) {
    int z;
    sqlite3 *db = NULL;
    char *dbfile = NULL;
    str_t *err = str_new(0);

    if (argc < 2) {
        printf("Usage:\n%s expfile\n", argv[0]);
        exit(0);
    }

    for (int i=1; i < argc; i++) {
        char *s = argv[i];

        // prog -i dbfile
        if (strcmp(s, "-i") == 0) {
            if (i == argc-1) {
                printf("Usage:\n%s -i <expense file>\n", argv[0]);
                goto exit_error;
            }
            dbfile = argv[i+1];
            z = create_expense_file(dbfile, &db, err);
            if (z != 0)
                fprintf(stderr, "Error creating '%s': %s\n", dbfile, err->s);
            goto exit_ok;
        }

        // prog dbfile
        dbfile = s;
        break;
    }

    if (!file_exists(dbfile)) {
        z = create_expense_file(dbfile, &db, err);
        if (z != 0) {
            fprintf(stderr, "Can't create '%s': %s\n", dbfile, err->s);
            goto exit_error;
        }
        printf("Created expense file '%s'\n", dbfile);
    } else {
        z = open_expense_file(dbfile, &db, err);
        if (z != 0) {
            fprintf(stderr, "Can't open '%s': %s\n", dbfile, err->s);
            goto exit_error;
        }
    }

    tui_start(db, dbfile);

exit_ok:
    str_free(err);
    return 0;

exit_error:
    str_free(err);
    return 1;
}



