#ifndef NAME_LL_H
#define NAME_LL_H

#include "transaction.h"
#include "lts_utils.h"

typedef struct name_counter {
	char name[MAX_GROUP_NAME];
	int  count;
} name_counter;

typedef struct name_ll_node {
  name_counter data;
  struct name_ll_node* prev;
  struct name_ll_node* next;
} name_ll_node;

typedef struct name_ll {
  name_ll_node* first;
  name_ll_node* last;
  int           size;
} name_ll;

/* Create an empty list */
name_ll name_ll_create();

/* Check if the list is empty */
int name_ll_is_empty(name_ll* list);

/* Print the contents of a list */
void name_ll_print(name_ll* list);

/* Append to the end of the list */
int name_ll_append(name_ll* list, char *name);

int name_ll_insert(name_ll* list, char *name);

int name_ll_remove(name_ll* list, char *name);

/* Search the whole list for a name, return true or false */ 
int name_ll_search(name_ll* list, char *name);

#endif
