#ifndef STATE_H
#define STATE_H

//#include "chat_ll.h"

#include "lts_utils.h"


typedef struct client_info {
	char	name[MAX_GROUP_NAME];   /* Full Spread name */
	char	user[MAX_GROUP_NAME];	/* Just User Name */
	char	room[MAX_GROUP_NAME];	/* User's current room */
} client_info;


typedef struct like_entry {
	char 		user[NAME_LEN];
	lts_entry 	lts;
	char 		action;
} like_entry;

#endif
