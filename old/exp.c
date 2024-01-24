#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "clib.h"
#include "exp.h"

#define MAX_CATEGORIES 50
#define MAX_EXPENSES 32768
#define MAX_YEARS 50
#define CAT_UNCATEGORIZED "Uncategorized"
#define EXPFILE_CATEGORIES "categories"
#define EXPFILE_EXPENSES "expenses"

static void chomp(char *buf);
static char *skip_ws(char *startp);
static char *read_field(char *startp, char **field);
static char *read_field_date(char *startp, date_t *dt);
static char *read_field_double(char *startp, double *field);
static char *read_field_uint(char *startp, uint *n);
static char *read_field_str(char *startp, str_t *str);
static void read_xp_line(char *buf, exp_t *xp);
static void read_cat_line(char *buf, cat_t *cat);

static void add_xp(char *buf, int lineno, array_t *xps);
static void add_cat(char *buf, int lineno, array_t *cats);
static uint db_next_expid(db_t *db);
static uint db_next_catid(db_t *db);

static void sort_expenses_by_date_asc(array_t *xps);
static void sort_expenses_by_date_desc(array_t *xps);
static void sort_expenses_default(array_t *xps);

exp_t *exp_new() {
    exp_t *xp = malloc(sizeof(exp_t));
    xp->rowid = 0;
    xp->dt = date_new_today();
    xp->desc = str_new(0);
    xp->catid = 0;
    xp->amt = 0.0;
    return xp;
}
void exp_free(exp_t *xp) {
    date_free(xp->dt);
    str_free(xp->desc);
    free(xp);
}

void exp_dup(exp_t *dest, exp_t *src) {
    date_dup(dest->dt, src->dt);
    str_assign(dest->desc, src->desc->s);
    dest->catid = src->catid;
    dest->amt = src->amt;
    dest->rowid = src->rowid;
}

int exp_is_valid(exp_t *xp) {
    if (xp->desc->len == 0)
        return 0;
    return 1;
}

static int exp_compare_date_asc(void *xp1, void *xp2) {
    date_t *dt1 = ((exp_t *)xp1)->dt;
    date_t *dt2 = ((exp_t *)xp2)->dt;
    if (dt1->time > dt2->time)
        return 1;
    if (dt1->time < dt2->time)
        return -1;
    return 0;
}
static int exp_compare_date_desc(void *xp1, void *xp2) {
    return -exp_compare_date_asc(xp1, xp2);
}
static void sort_expenses_by_date_asc(array_t *xps) {
    sort_array(xps->items, xps->len, exp_compare_date_asc);
}
static void sort_expenses_by_date_desc(array_t *xps) {
    sort_array(xps->items, xps->len, exp_compare_date_desc);
}
// Sort xps by date (descending) and desc (ascending)
static int exp_compare_default(void *xp1, void *xp2) {
    date_t *dt1 = ((exp_t *)xp1)->dt;
    date_t *dt2 = ((exp_t *)xp2)->dt;
    char *desc1 = ((exp_t *)xp1)->desc->s;
    char *desc2 = ((exp_t *)xp2)->desc->s;
    assert(desc1 != NULL);
    assert(desc2 != NULL);

    if (dt1->time > dt2->time)
        return -1;
    if (dt1->time < dt2->time)
        return 1;
    return strcasecmp(desc1, desc2);
}
static void sort_expenses_default(array_t *xps) {
    sort_array(xps->items, xps->len, exp_compare_default);
}

cat_t *cat_new() {
    cat_t *cat = malloc(sizeof(cat_t));
    cat->id = 0;
    cat->name = str_new(15);
    return cat;
}
void cat_free(cat_t *cat) {
    str_free(cat->name);
    free(cat);
}
void cat_dup(cat_t *dest, cat_t *src) {
    str_assign(dest->name, src->name->s);
}
int cat_is_valid(cat_t *cat) {
    if (cat->id == 0 || cat->name->len == 0)
        return 0;
    return 1;
}

db_t *db_new() {
    db_t *db = malloc(sizeof(db_t));
    db->cats = array_new(MAX_CATEGORIES);
    db->xps = array_new(MAX_EXPENSES);
    db->years = intarray_new(MAX_YEARS);
    db_init_exp_years(db);

    return db;
}
void db_free(db_t *db) {
    array_free(db->xps);
    intarray_free(db->years);
    free(db);
}
void db_reset(db_t *db) {
    array_t *xps = db->xps;
    for (int i=0; i < xps->len; i++) {
        assert(xps->items[i] != NULL);
        exp_free(xps->items[i]);
    }

    array_clear(db->xps);
    intarray_clear(db->years);
    db_init_exp_years(db);
}

#define BUFLINE_SIZE 255
enum loadstate_t {DB_LOAD_NONE, DB_LOAD_CAT, DB_LOAD_EXP};

void db_load_expense_file(db_t *db, FILE *f) {
    array_t *xps = db->xps;
    array_t *cats = db->cats;
    cat_t *cat;
    int lineno = 1;
    char *buf;
    size_t buf_size;
    int z;
    enum loadstate_t state = DB_LOAD_NONE;

    db_reset(db);

    cat = cat_new();
    cat->id = 0;
    str_assign(cat->name, CAT_UNCATEGORIZED);
    array_add(cats, cat);
    db->cats->items[0] = cat;

    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    while (1) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            print_error("getline() error");
            break;
        }
        if (z == -1)
            break;
        chomp(buf);

        if (strlen(buf) == 0)
            continue;
        if (buf[0] == '#') {
            lineno++;
            continue;
        }
        // Line starts with %categories or %expenses
        if (buf[0] == '%') {
            if (strcmp(buf+1, EXPFILE_CATEGORIES) == 0)
                state = DB_LOAD_CAT;
            else if (strcmp(buf+1, EXPFILE_EXPENSES) == 0)
                state = DB_LOAD_EXP;
            else
                fprintf(stderr, "Skipping invalid line: '%s'\n", buf);

            lineno++;
            continue;
        }

        if (state == DB_LOAD_EXP)
            add_xp(buf, lineno, xps);
        else if (state == DB_LOAD_CAT)
            add_cat(buf, lineno, cats);
        lineno++;
    }

    free(buf);

    assert(xps->len <= xps->cap);
    assert(cats->len <= cats->cap);
    if (xps->len == xps->cap)
        fprintf(stderr, "Max expenses (%ld) reached.\n", xps->cap);
    if (cats->len == cats->cap)
        fprintf(stderr, "Max categories (%ld) reached.\n", cats->cap);

    sort_expenses_default(xps);
    db_init_exp_years(db);
}

void db_save_expense_file(db_t *db, FILE *f) {
    cat_t *cat;
    exp_t *xp;
    char isodate[ISO_DATE_LEN+1];

    sort_expenses_default(db->xps);

    // %categories
    fprintf(f, "%%%s\n", EXPFILE_CATEGORIES);
    for (int i=1; i < db->cats->len; i++) {
        cat = db->cats->items[i];
        fprintf(f, "%d; %s\n", cat->id, cat->name->s);
    }
    fprintf(f, "\n");

    // %expenses
    fprintf(f, "%%%s\n", EXPFILE_EXPENSES);
    for (int i=0; i < db->xps->len; i++) {
        xp = db->xps->items[i];
        date_to_iso(xp->dt, isodate, sizeof(isodate));
        fprintf(f, "%s; %s; %.2f; %d\n", isodate, xp->desc->s, xp->amt, xp->catid);
    }
}

static void add_xp(char *buf, int lineno, array_t *xps) {
    exp_t *xp = exp_new();
    read_xp_line(buf, xp);
    if (!exp_is_valid(xp)) {
        fprintf(stderr, "Skipping invalid expense line: %d\n", lineno);
        exp_free(xp);
        return;
    }
    xp->rowid = xps->len;
    if (array_add(xps, xp) != 0) {
        //fprintf(stderr, "Maximum number of expenses reached: %ld\n", xps->cap);
        exp_free(xp);
        return;
    }
}
static void add_cat(char *buf, int lineno, array_t *cats) {
    cat_t *cat = cat_new();
    read_cat_line(buf, cat);
    if (!cat_is_valid(cat)) {
        fprintf(stderr, "Skipping invalid category line: %d\n", lineno);
        cat_free(cat);
        return;
    }
    if (array_add(cats, cat) != 0) {
        //fprintf(stderr, "Maximum number of categories reached: %ld\n", cats->cap);
        cat_free(cat);
        return;
    }
}

// Remove trailing \n or \r chars.
static void chomp(char *buf) {
    ssize_t buf_len = strlen(buf);
    for (int i=buf_len-1; i >= 0; i--) {
        if (buf[i] == '\n' || buf[i] == '\r')
            buf[i] = 0;
    }
}
static void read_xp_line(char *buf, exp_t *xp) {
    // Sample expense line:
    // 2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee

    char *p = buf;
    p = read_field_date(p, xp->dt);
    //printf("read_xp_line() %d-%d-%d\n", date_year(xp->dt), date_month(xp->dt), date_day(xp->dt));
    p = read_field_str(p, xp->desc);
    p = read_field_double(p, &xp->amt);
    p = read_field_uint(p, &xp->catid);
}
static void read_cat_line(char *buf, cat_t *cat) {
    char *p = buf;
    p = read_field_uint(p, &cat->id);
    p = read_field_str(p, cat->name);
}
static char *skip_ws(char *startp) {
    char *p = startp;
    while (*p == ' ')
        p++;
    return p;
}
static char *read_field(char *startp, char **field) {
    char *p = startp;
    while (*p != '\0' && *p != ';')
        p++;

    if (*p == ';') {
        *p = '\0';
        *field = startp;
        return skip_ws(p+1);
    }

    *field = startp;
    return p;
}
static char *read_field_date(char *startp, date_t *dt) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    date_assign_iso(dt, sfield);
    return p;
}
static char *read_field_double(char *startp, double *f) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *f = atof(sfield);
    return p;
}
static char *read_field_uint(char *startp, uint *n) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *n = atoi(sfield);
    return p;
}
static char *read_field_str(char *startp, str_t *str) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    str_assign(str, sfield);
    return p;
}

void db_init_exp_years(db_t *db) {
// Assumes that xps is sorted descending order by date.
    uint lowest_year = 10000;
    size_t j = 0;
    intarray_t *years = db->years;

    assert(years->cap > 0);

    years->items[j] = 0;
    j++;

    for (int i=0; i < db->xps->len; i++) {
        if (j > years->cap-1)
            break;

        exp_t *xp = db->xps->items[i];
        uint xp_year = date_year(xp->dt);
        if (xp_year < lowest_year) {
            years->items[j] = (int)xp_year;
            lowest_year = xp_year;
            j++;
        }
    }
    years->len = j;
}

void db_update_expense(db_t *db, exp_t *savexp) {
    array_t *xps = db->xps;
    exp_t *xp;

    for (int i=0; i < xps->len; i++) {
        xp = xps->items[i];
        if (xp->rowid == savexp->rowid) {
            exp_dup(xp, savexp);
            sort_expenses_default(db->xps);
            db_init_exp_years(db);
            return;
        }
    }
}

int db_add_expense(db_t *db, exp_t *newxp) {
    array_t *xps = db->xps;
    exp_t *xp;

    assert(xps->len <= xps->cap);
    if (xps->len == xps->cap) {
        printf("Maximum number of expenses (%ld) reached.\n", xps->cap);
        return 1;
    }

    xp = exp_new();
    exp_dup(xp, newxp);
    xp->rowid = db_next_expid(db);
    newxp->rowid = xp->rowid;
    array_add(xps, xp);

    sort_expenses_default(db->xps);
    db_init_exp_years(db);
    return 0;
}

void db_del_expense(db_t *db, exp_t *delxp) {
    array_t *xps = db->xps;
    exp_t *xp;

    for (int i=0; i < xps->len; i++) {
        xp = xps->items[i];
        if (xp->rowid == delxp->rowid) {
            array_del(xps, i);
            exp_free(xp);

            sort_expenses_default(db->xps);
            db_init_exp_years(db);
            return;
        }
    }
}

static uint db_next_expid(db_t *db) {
    exp_t *xp;
    int highestid = 0;

    for (int i=0; i < db->xps->len; i++) {
        xp = db->xps->items[i];
        if (xp->rowid > highestid)
            highestid = xp->rowid;
    }
    return highestid+1;
}


void db_update_cat(db_t *db, cat_t *savecat) {
    cat_t *cat;

    for (int i=0; i < db->cats->len; i++) {
        cat = db->cats->items[i];
        if (cat->id == savecat->id) {
            cat_dup(cat, savecat);
            return;
        }
    }
}

void db_add_cat(db_t *db, cat_t *newcat) {
    cat_t *cat;

    assert(db->cats->len <= db->cats->cap);
    if (db->cats->len == db->cats->cap) {
        printf("Maximum number of categories (%ld) reached.\n", db->cats->cap);
        return;
    }

    cat = cat_new();
    cat_dup(cat, newcat);
    cat->id = db_next_catid(db);
    db->cats->items[db->cats->len] = cat;
    db->cats->len++;
}

static uint db_next_catid(db_t *db) {
    cat_t *cat;
    int highestid = 0;

    for (int i=0; i < db->cats->len; i++) {
        cat = db->cats->items[i];
        if (cat->id > highestid)
            highestid = cat->id;
    }
    return highestid+1;
}

cat_t *db_find_cat(db_t *db, uint id) {
    cat_t *cat;
    for (int i=0; i < db->cats->len; i++) {
        cat = db->cats->items[i];
        if (cat->id == id)
            return cat;
    }
    return NULL;
}
char *db_find_cat_name(db_t *db, uint id) {
    cat_t *cat = db_find_cat(db, id);
    if (cat != NULL)
        return cat->name->s;
    return "";
}

