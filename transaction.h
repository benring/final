#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "config.h"

#define ROOM 'R'
#define CHAT 'C'
#define LIKE 'L'

#define MAX_ENTRY_SIZE 256


typedef struct lts_entry {
	unsigned	int		ts;
	unsigned	int		pid;
} lts_entry;


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

typedef struct like_entry {
	char 		user[NAME_LEN];
	lts_entry 	lts;
	char 		action;
} like_entry;

typedef struct chat_info {
	lts_entry	lts;
	chat_entry 	chat;
	like_ll 	likes;
} chat_info;


typedef struct room_state {
	char						name[NAME_LEN];
	char						spread_group[NAME_LEN];
//	linkedList<name, server>	attendees;
//	linkedList<chat_message>	history;
} room_state;



typedef struct client_info {
	char	name[MAX_GROUP_NAME];   /* Full Spread name */
	char	user[MAX_GROUP_NAME];	/* Just User Name */
	char	room[MAX_GROUP_NAME];	/* User's current room */
} client_info;




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
