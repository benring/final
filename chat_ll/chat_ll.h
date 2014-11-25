#ifndef CHAT_LL_H
#define CHAT_LL_H

#include "transaction.h"
#include "lts_utils.h"

typedef struct chat_info {
	lts_entry	lts;
	chat_entry 	chat;
	like_ll 	likes;
} chat_info;

typedef struct chat_ll_node {
  chat_info data;
  struct chat_ll_node* prev;
  struct chat_ll_node* next;
} chat_ll_node;

typedef struct chat_ll {
  chat_ll_node* first;
  chat_ll_node* last;
} chat_ll;

/* Create an empty list */
chat_ll chat_ll_create();

/* Check if the list is empty */
int chat_ll_is_empty(chat_ll* list);

/* Print the contents of a list */
void chat_ll_print(chat_ll* list);

/* Append to the end of the list */
int chat_ll_append(chat_ll* list, chat_info data);

/* Insert element in order by lts (assuming list is in order) */
int chat_ll_insert_inorder(chat_ll* list, chat_info data);

/* Insert element in order by lts, searching from the back. (assuming list is in order) */
int chat_ll_insert_inorder_fromback(chat_ll* list, chat_info data);

/* Search the whole (unordered) list for an lts */ 
chat_info* chat_ll_get(chat_ll *list, lts_entry lts);

/* Search the list for an lts, terminates early (assuming list is in order) */ 
chat_info* chat_ll_get_inorder(chat_ll *list, lts_entry lts);

/* Search an ordered list for an lts from the back, terminates early  (assuming list is in order) */
chat_info* chat_ll_get_inorder_fromback(chat_ll *list, lts_entry lts);

int chat_ll_length(chat_ll *list);

void chat_ll_remove_first(chat_ll *list);

#endif
