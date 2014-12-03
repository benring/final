/******************************************************************************
 * File:     update.h
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Update list data structure. The Update list is used by the
 *  server to retain all the updates received from both clients and other
 *  servers. The list is sorted by LTS for efficiency. Provides functions for
 *  create, print, insert, access, and clear.
 *
 *****************************************************************************/
#ifndef UPDATE_LL_H
#define UPDATE_LL_H

#define ELEM update

#include "transaction.h"
#include "lts_utils.h"

typedef struct update_ll_node {
  ELEM data;
  struct update_ll_node* prev;
  struct update_ll_node* next;
} update_ll_node;

typedef struct update_ll {
  update_ll_node* first;
  update_ll_node* last;
} update_ll;

/* Create an empty list */
update_ll update_ll_create();

/* Check if the list is empty */
int update_ll_is_empty(update_ll* list);

/* Print the contents of a list */
void update_ll_print(update_ll* list);

/* Append to the end of the list */
int update_ll_append(update_ll* list, ELEM data);

/* Insert element in order by lts (assuming list is in order) */
int update_ll_insert_inorder(update_ll* list, ELEM data);

/* Insert element in order by lts, searching from the back. (assuming list is in order) */
int update_ll_insert_inorder_fromback(update_ll* list, ELEM data);

/* Search the whole (unordered) list for an lts */ 
ELEM* update_ll_get(update_ll *list, lts_entry lts);

/* Search the list for an lts, terminates early (assuming list is in order) */ 
ELEM* update_ll_get_inorder(update_ll *list, lts_entry lts);

/* Search an ordered list for an lts from the back, terminates early  (assuming list is in order) */
ELEM* update_ll_get_inorder_fromback(update_ll *list, lts_entry lts);

int update_ll_clear(update_ll* list);
#endif

