#ifndef EXP_H
#define EXP_H

#include "clib.h"
#include "sqlite3/sqlite3.h"

typedef struct {
    uint64_t catid;
    str_t *name;
} cat_t;

typedef struct {
    uint64_t expid;
    date_t *date;
    str_t *desc;
    double amt;
    uint64_t catid;
} exp_t;

exp_t *exp_new();
void exp_free(exp_t *xp);
void exp_dup(exp_t *destxp, exp_t *srcxp);
int exp_is_valid(exp_t *xp);

int create_dbfile(char *dbfile);

int db_add_cat(sqlite3 *db, cat_t *cat);
int db_edit_cat(sqlite3 *db, cat_t *cat);
int db_del_cat(sqlite3 *db, uint catid);

int db_select_exp(sqlite3 *db, array_t *xps);
int db_add_exp(sqlite3 *db, exp_t *xp);
int db_edit_exp(sqlite3 *db, exp_t *xp);
int db_del_exp(sqlite3 *db, uint expid);

#endif
