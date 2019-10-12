#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[32];
  int value;

} WP;

WP* new_wp(char *args);
bool check_wp();
void print_wp();
void free_wp(char *args);
#endif
