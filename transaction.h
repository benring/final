#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "config.h"
#include "state.h"
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



typedef struct chat_info {
	lts_entry	lts;
	chat_entry 	chat;
	like_ll 	likes;
} chat_info;

/*  Type of functions for turning stable data into current stae */
/*  return types TBD */
/*
getUserList (char * room);
getRecentChats (char * room, int num);
getAllChats (char * room);
appendChat (char * room, char * user, char * chat);
likeChat (char * user, int clognum);
removeLike (char * user, int clognum);
*/
#endif
