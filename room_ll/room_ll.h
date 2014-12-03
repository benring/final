/******************************************************************************
 * File:     room.h
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Room data structure. Defines a single chat room, to include
 *    name, list of chat messages, and a distro_group which the servers uses
 *    to multicast updates. Includes associates functions to 
 *    create, access, and insert
 *
 *****************************************************************************/
#ifndef ROOM_LL_H
#define ROOM_LL_H

#include "transaction.h"
#include "chat_ll.h"
#include "lts_utils.h"
#include "client_ll.h"

typedef struct room_info {
	char				name[NAME_LEN];
	char        distro_group[NAME_LEN];
	chat_ll			chats;
} room_info;

//	char						attendee_group[NAME_LEN];
//	client_ll                               attendees;

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

/* Append to the end of the list */
int room_ll_append(room_ll* list, room_info data);

/* Search the whole (unordered) list for a given roomname */ 
room_info* room_ll_get(room_ll *list, char * name); 

#endif
