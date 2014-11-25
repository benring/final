#ifndef ROOM_LL_H
#define ROOM_LL_H

#include "transaction.h"
#include "chat_ll.h"
#include "lts_utils.h"

typedef struct room_info {
	char						name[NAME_LEN];
	char						spread_group[NAME_LEN];
	chat_ll					chats;
} room_info;

typedef struct room_ll_node {
  room_info data;
  struct room_ll_node* prev;
  struct room_ll_node* next;
} room_ll_node;

typedef struct room_ll {
  room_ll_node* first;
  room_ll_node* last;
} room_ll;

/* Create an empty list */
room_ll room_ll_create();

/* Check if the list is empty */
int room_ll_is_empty(room_ll* list);

/* Print the contents of a list */
void room_ll_print(room_ll* list);

/* Append to the end of the list */
int room_ll_append(room_ll* list, room_info data);

/* Search the whole (unordered) list for an lts */ 
room_info* room_ll_get(room_ll *list, char * name); 

#endif
