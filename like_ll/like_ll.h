/******************************************************************************
 * File:     like.h
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Like List data structure. A like is an entry of either an 
 *    ADD_LIKE or a REM_LIKE for a chat text. The list is associated with  
 *    a specific chat entry. This implementation provides a sorted linked list 
 *    of likes along with associated functions to create, access, append, insert.
 *    
 *    NOTE: As part of the larger chat system, the LIKES List will only
 *      porcess the most recent like LTS for a single user in the list. Thus, 
 *      there will be at most one element in the like list per user.
 *      This ensure consistency througout the program by always processing
 *      the most recent like action. It should also be noted that data
 *      intgrity is not provided in this implementation and is handled 
 *      elsewhere (e.g. a user cannot REM_LIKE if he/she had not previosly
 *      likes that message).
 *
 *****************************************************************************/
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

/* Returns Like entry for given user */
like_entry* like_ll_get_user(like_ll *list, char *user);

/* Counts total # of Likes in the list */
int like_ll_count_likes(like_ll *list);

/* Returns TRUE if user likes the message, false otherwise */
int does_like (like_ll *list, char * name);

/* Insertion function utilized in the chat program ensuring updated, consisten
 *    LIKE information where only the most latest processed LIKE entry (by LTS)
 *    is retained in the list */
int like_ll_update_like(like_ll *like_list, char* user, lts_entry like_lts, char action);
#endif
