#include "lts_utils.h"

/* return true if lts1 < lts2 */
int lts_lessthan(lts_entry lts1, lts_entry lts2) {
  if (lts1.ts < lts2.ts) {
    return 1;
  }

  if (lts1.ts == lts2.ts && lts1.pid < lts2.pid) {
    return 1;
  }
  
  return 0;
}

int lts_greaterthan(lts_entry lts1, lts_entry lts2) {
  if(lts1.ts > lts2.ts) {
    return 1;
  }
  
  if (lts1.ts == lts2.ts && lts1.pid > lts2.pid) {
    return 1;
  }

  return 0;
}

int lts_eq(lts_entry lts1, lts_entry lts2) {
  if (lts1.ts == lts2.ts && lts1.pid == lts2.pid) {
    return 1;
  }

  return 0;
}
