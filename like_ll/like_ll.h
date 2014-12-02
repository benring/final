#ifndef LIKE_LL_H
#define LIKE_LL_H

#include "config.h"
#include "lts_utils.h"
#include "transaction.h"

typedef struct like_ll_node {
  like_entry data;
  struct like_ll_node* prev;
  struct like_ll_node* next;
} like_ll_node;

typedef struct like_ll {
  like_ll_node* first;
  like_ll_node* last;
} like_ll;

/* Create an empty list */
like_ll like_ll_create();

/* Check if the list is empty */
int like_ll_is_empty(like_ll* list);

/* Print the contents of a list */
void like_ll_print(like_ll* list);

/* Append to the end of the list */
int like_ll_append(like_ll* list, like_entry data);

/* Insert element in order by lts (assuming list is in order) */
int like_ll_insert_inorder(like_ll* list, like_entry data);

/* Insert element in order by lts, searching from the back. (assuming list is in order) */
int like_ll_insert_inorder_fromback(like_ll* list, like_entry data);

/* Search the whole (unordered) list for an lts */ 
like_entry* like_ll_get(like_ll *list, lts_entry lts);

/* Search the list for an lts, terminates early (assuming list is in order) */ 
like_entry* like_ll_get_inorder(like_ll *list, lts_entry lts);

/* Search an ordered list for an lts from the back, terminates early  (assuming list is in order) */
like_entry* like_ll_get_inorder_fromback(like_ll *list, lts_entry lts);

like_entry* like_ll_get_user(like_ll *list, char *user);

int like_ll_count_likes(like_ll *list);

int does_like (like_ll *list, char * name);

int like_ll_update_like(like_ll *like_list, char* user, lts_entry like_lts, char action);
#endif
