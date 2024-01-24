#ifndef EXP_H
#define EXP_H

#include <gtk/gtk.h>
#include "clib.h"

typedef struct {
    uint id;
    str_t *name;
} cat_t;

typedef struct {
    uint rowid;
    date_t *dt;
    str_t *desc;
    double amt;
    uint catid;
} exp_t;

typedef struct {
    array_t *cats;
    array_t *xps;
    intarray_t *years;
} db_t;

cat_t *cat_new();
void cat_free(cat_t *cat);
void cat_dup(cat_t *destcat, cat_t *srccat);
int cat_is_valid(cat_t *cat);

exp_t *exp_new();
void exp_free(exp_t *xp);
void exp_dup(exp_t *destxp, exp_t *srcxp);
int exp_is_valid(exp_t *xp);

db_t *db_new();
void db_free(db_t *db);
void db_reset(db_t *db);

void db_load_expense_file(db_t *db, FILE *f);
void db_save_expense_file(db_t *db, FILE *f);
void db_init_exp_years(db_t *db);

void db_update_expense(db_t *db, exp_t *savexp);
int db_add_expense(db_t *db, exp_t *newxp);
void db_del_expense(db_t *db, exp_t *xp);
void db_update_cat(db_t *db, cat_t *cat);
void db_add_cat(db_t *db, cat_t *cat);
void db_del_cat(db_t *db, cat_t *cat);
cat_t *db_find_cat(db_t *db, uint id);
char *db_find_cat_name(db_t *db, uint id);

#endif
