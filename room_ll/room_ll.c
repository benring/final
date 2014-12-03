/******************************************************************************
 * File:     room.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  see room.h
 *
 *****************************************************************************/
#include "room_ll.h"
#include "stdio.h"
#include "stdlib.h"

/* Create an empty list */
room_ll room_ll_create() {
  room_ll result;
  result.first = 0;
  result.last = 0;
  return result;
}

/* Check if the list is empty */
int room_ll_is_empty(room_ll* list) {
  if (list->first) {
    return 0;
  }
  else {
    return 1;
  }
}


/* Append to the end of the list */
int room_ll_append(room_ll* list, room_info data) {
  room_ll_node *node = malloc(sizeof(room_ll_node));
	room_ll_node *old_last;
	
  if (!node) {
    return 0; /* failure */
  }
  node->data = data;
  node->prev = 0;
  node->next = 0;
  if(room_ll_is_empty(list)) {
    list->first = node;
    list->last = node;
  }
  else {
    old_last = list->last;
    old_last->next = node;
    node->prev = old_last;
    list->last = node;
  }
  return 1; /* success */
}


/* Search the whole (unordered) list for an given roomname */ 
room_info* room_ll_get(room_ll* list, char * name) {
  room_info* result = 0;
  room_ll_node* curr = list->first;

  if (room_ll_is_empty(list)) {
    return result;
  }

  while(curr) {
		if (strcmp(curr->data.name, name) == 0) {
			result = &curr->data;
			return result;
		}
		curr = curr->next;
	}
  return result;
}
