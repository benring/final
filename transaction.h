#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "config.h"
//#include "state.h"
#include "like_ll.h"

#define ROOM 'R'
#define CHAT 'C'
#define LIKE 'L'

#define MAX_ENTRY_SIZE 256

typedef struct update {
	char 		tag;   		/* log type: [R | C | L] */
	lts_entry 	lts;			/* Global sequence # */
	char  		entry[MAX_ENTRY_SIZE];		/* One of three possible entries */
} update;

typedef struct room_entry {
	char 	room[NAME_LEN];
} room_entry;

typedef struct chat_entry {
	char 	user[NAME_LEN];
	char 	room[NAME_LEN];
	char 	text[CHAT_LEN];
} chat_entry;

#endif
