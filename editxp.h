#ifndef EDITXP_H
#define EDITXP_H

#include "tuilib.h"

#define EDITXP_FOCUS_DESC   0
#define EDITXP_FOCUS_AMT    1
#define EDITXP_FOCUS_CAT    2
#define EDITXP_FOCUS_DATE   3
#define EDITXP_FOCUS_SAVE   4
#define EDITXP_FOCUS_COUNT  5

extern panel_t editxp;
extern int editxp_show;
extern exp_t *editxp_xp;
extern focus_t editxp_focus[EDITXP_FOCUS_COUNT];
extern int editxp_ifocus;
extern int editxp_is_focus_active;
extern entry_t editxp_entry_desc;
extern entry_t editxp_entry_amt;
extern entry_t editxp_entry_date;
extern listbox_t editxp_listbox_cat;

void init_editxp(exp_t *xp);
void update_editxp(struct tb_event *ev);
void draw_editxp();

#endif
