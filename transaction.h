/******************************************************************************
 * File:     transition.h
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Log entry definitions for all updates
 *
 *****************************************************************************/
#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "config.h"
#include "lts_utils.h"

#define ROOM 'R'
#define CHAT 'C'
#define LIKE 'L'

#define MAX_ENTRY_SIZE 150

typedef struct update {
	char 		      tag;   		/* log type: [R | C | L] */
	lts_entry 	  lts;			/* Global sequence # */
	char  		    entry[MAX_ENTRY_SIZE];		/* One of three possible entries */
} update;

typedef struct room_entry {
	char 	room[NAME_LEN];
} room_entry;

typedef struct chat_entry {
	char 	user[NAME_LEN];
	char 	room[NAME_LEN];
	char 	text[CHAT_LEN];
} chat_entry;

typedef struct client_info {
	char	name[MAX_GROUP_NAME];   /* Full Spread name */
	char	user[MAX_GROUP_NAME];	/* Just User Name */
	char	room[MAX_GROUP_NAME];	/* User's current room */
} client_info;

typedef struct like_entry {
	char 		    user[NAME_LEN];
	lts_entry 	lts;
	char 		    action;
} like_entry;

#endif
