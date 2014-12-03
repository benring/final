/******************************************************************************
 * File:     chat.h
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Chat data structure. A chat is a line of text with a 
 *    an LTS time stamp and a list of "likes." This implementation includes
 *    structure for a single chat and a linked list of chats along with 
 *    associated functions to create, access, append, insert. If using the
 *    insert_inorder, this list will retain elements sorted from lowest
 *    to highest LTS. 
 *
 *****************************************************************************/
#ifndef CHAT_LL_H
#define CHAT_LL_H

#include "transaction.h"
#include "lts_utils.h"
#include "like_ll.h"

typedef struct chat_info {
	lts_entry	  lts;
	chat_entry 	chat;
	like_ll 	  likes;
} chat_info;

typedef struct chat_ll_node {
  chat_info data;
  struct chat_ll_node* prev;
  struct chat_ll_node* next;
} chat_ll_node;

typedef struct chat_ll {
  chat_ll_node* first;
  chat_ll_node* last;
  int           size;
} chat_ll;

/* Create an empty list */
chat_ll chat_ll_create();

/* Check if the list is empty */
int chat_ll_is_empty(chat_ll* list);

/* Print the contents of a list */
void chat_ll_print(chat_ll* list);

/* Print last num elements of a list */
void chat_ll_print_num(chat_ll* list, int num);

/* Given an index, retrieve the corresponding LTS value */
lts_entry chat_ll_get_lts(chat_ll *list, int i);

/* Append to the end of the list */
int chat_ll_append(chat_ll* list, chat_info data);

/* Insert element in order by lts (assuming list is in order) */
int chat_ll_insert_inorder(chat_ll* list, chat_info data);

/* Insert element in order by lts, searching from the back. (assuming list is in order) */
int chat_ll_insert_inorder_fromback(chat_ll* list, chat_info data);

/* Search the whole (unordered) list for an lts & return the chat*/ 
chat_info* chat_ll_get(chat_ll *list, lts_entry lts);

/* Search the list for an lts, terminates early (assuming list is in order) */ 
chat_info* chat_ll_get_inorder(chat_ll *list, lts_entry lts);

/* Search an ordered list for an lts from the back, terminates early  (assuming list is in order) */
chat_info* chat_ll_get_inorder_fromback(chat_ll *list, lts_entry lts);

/* Get the # of chats in the list */
int chat_ll_length(chat_ll *list);

/* Removes first element from the list */
void chat_ll_remove_first(chat_ll *list);


#endif
