#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "clib.h"
#include "db.h"

int main(int argc, char *argv[]) {
    int z;
    sqlite3 *db;
    char *dbfile = NULL;
    char *s;
    char *err;

    for (int i=1; i < argc; i++) {
        char *s = argv[i];

        // -i dbfile
        if (strcmp(s, "-i") == 0 && i < argc-1) {
            z = create_dbfile(argv[i+1]);
            if (z != 0)
                exit(1);
            break;
        }

        // dbfile
        dbfile = s;
    }

    if (dbfile)
        printf("dbfile: %s\n", dbfile);

    return 0;
}



