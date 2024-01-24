#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "db.h"
#include "sqlite3/sqlite3.h"

static void db_print_err(sqlite3 *db, char *sql) {
    fprintf(stderr, "SQL: %s\nError: %s\n", sql, sqlite3_errmsg(db));
}

static int db_init_tables(sqlite3 *db) {
    char *s;
    char *err;
    int z;

    s = "CREATE TABLE IF NOT EXISTS cat (cat_id INTEGER PRIMARY KEY NOT NULL, name TEXT);"
        "CREATE TABLE IF NOT EXISTS exp (exp_id INTEGER PRIMARY KEY NOT NULL, date TEXT NOT NULL, desc TEXT NOT NULL DEFAULT '', amt REAL NOT NULL DEFAULT 0.0, cat_id INTEGER NOT NULL DEFAULT 1);";
    z = sqlite3_exec(db, s, 0, 0, &err);
    if (z != 0) {
        fprintf(stderr, "SQL Error: %s\n", err);
        sqlite3_free(err);
        sqlite3_close_v2(db);
        return 1;
    }
    return 0;
}

int create_dbfile(char *dbfile) {
    sqlite3 *db;
    int z;

    z = sqlite3_open(dbfile, &db);
    if (z != 0) {
        fprintf(stderr, "Error opening dbfile '%s': %s\n", dbfile, sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return 1;
    }
    db_init_tables(db);
    sqlite3_close_v2(db);
    return 0;
}

exp_t *exp_new() {
    exp_t *xp = malloc(sizeof(exp_t));
    xp->expid = 0;
    xp->date = date_new_today();
    xp->desc = str_new(0);
    xp->catid = 0;
    xp->amt = 0.0;
    return xp;
}
void exp_free(exp_t *xp) {
    date_free(xp->date);
    str_free(xp->desc);
    free(xp);
}

void exp_dup(exp_t *dest, exp_t *src) {
    dest->expid = src->expid;
    date_dup(dest->date, src->date);
    str_assign(dest->desc, src->desc->s);
    dest->amt = src->amt;
    dest->catid = src->catid;
}

int exp_is_valid(exp_t *xp) {
    if (xp->desc->len == 0)
        return 0;
    return 1;
}


int db_add_cat(sqlite3 *db, cat_t *cat) {
    return 0;
}

int db_edit_cat(sqlite3 *db, cat_t *cat) {
    return 0;
}

int db_del_cat(sqlite3 *db, uint catid) {
    return 0;
}

int db_select_exp(sqlite3 *db, array_t *xps) {
    exp_t *xp;
    sqlite3_stmt *stmt;
    char *s;
    int z;
    char isodate[ISO_DATE_LEN+1];

    s = "SELECT exp_id, date, desc, amt, cat_id FROM exp WHERE 1=1";
    z = sqlite3_prepare_v2(db, s, -1, &stmt, 0);
    if (z != 0) {
        sqlite3_finalize(stmt);
        db_print_err(db, s);
        return 1;
    }

    array_clear(xps);
    while ((z = sqlite3_step(stmt)) == SQLITE_ROW) {
        xp = exp_new();
        xp->expid = sqlite3_column_int64(stmt, 0);
        date_assign_iso(xp->date, (char*)sqlite3_column_text(stmt, 1));
        str_assign(xp->desc, (char*)sqlite3_column_text(stmt, 2));
        xp->amt = sqlite3_column_double(stmt, 3);
        xp->catid = sqlite3_column_int64(stmt, 4);

        array_add(xps, xp);
    }
    if (z != SQLITE_DONE) {
        db_print_err(db, s);
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_add_exp(sqlite3 *db, exp_t *xp) {
    sqlite3_stmt *stmt;
    char *s;
    int z;
    char isodate[ISO_DATE_LEN+1];

    s = "INSERT INTO exp (date, desc, amt, cat_id) VALUES (?, ?, ?, ?);";
    z = sqlite3_prepare_v2(db, s, -1, &stmt, 0);
    if (z != 0) {
        sqlite3_finalize(stmt);
        db_print_err(db, s);
        return 1;
    }
    date_to_iso(xp->date, isodate, sizeof(isodate));
    z = sqlite3_bind_text(stmt, 1, isodate, -1, NULL);
    assert(z == 0);
    z = sqlite3_bind_text(stmt, 2, xp->desc->s, -1, NULL);
    assert(z == 0);
    z = sqlite3_bind_double(stmt, 3, xp->amt);
    assert(z == 0);
    z = sqlite3_bind_int(stmt, 4, xp->catid);
    assert(z == 0);

    z = sqlite3_step(stmt);
    if (z != SQLITE_DONE) {
        db_print_err(db, s);
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_edit_exp(sqlite3 *db, exp_t *xp) {
    return 0;
}

int db_del_exp(sqlite3 *db, uint expid) {
    return 0;
}



