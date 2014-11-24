#ifndef LTS_UTILS_H
#define LTS_UTILS_H

#include "config.h"
#include "state.h"

/* return true if lts1 < lts2 */
int lts_lessthan(lts_entry lts1, lts_entry lts2);

int lts_greaterthan(lts_entry lts1, lts_entry lts2);

int lts_eq(lts_entry lts1, lts_entry lts2);

#endif
