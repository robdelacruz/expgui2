#ifndef LISTXP_H
#define LISTXP_H

#include "tuilib.h"

extern panel_t listxp;
extern array_t *listxp_xps;
extern int listxp_ixps;
extern int listxp_scrollpos;

exp_t *listxp_selected_expense();
void update_listxp(struct tb_event *ev);
void draw_listxp();

#endif
