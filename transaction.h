#ifndef TRANSACTION_H
#define TRANSACTION_H

#define ROOM 'R'
#define CHAT 'C'
#define LIKE 'L'


typedef lts_entry {
	int		ts;
	int		pid;
} lts_entry;


typedef struct update {
	char 		tag;   		/* log type: [R | C | L] */
	lts_entry 	lts;			/* Global sequence # */
	char * 		entry;		/* One of three possible entries */
} update;

typedef room_entry {
	char 	user[NAME_LEN];
	int 	server;
	char 	room[NAME_LEN];
	char 	action;
} room_entry;

typedef chat_entry {
	char 	user[NAME_LEN];
	char 	room[NAME_LEN];
	char 	text[CHAT_LEN];
} chat_entry;

typedef like_entry {
	char 		user[NAME_LEN];
	lts_entry 	chat_lts;
	char 		action
} like_entry;

typedef chat_message {
	chat_entry * chat;
	linkedList<like_entry_*>	likes;
} chat_message;


typedef room_state {
	char						name[NAME_LEN];
	char						spread_group[NAME_LEN];
	linkedList<name, server>	attendees;
	linkedList<chat_message>	history;
} room_state;

typedef connected_client {
	char 	spread_grp[NAME_LEN];
	char	user[NAME_LEN];
	char	room[NAME_LEN];	
} connected_client;



/*  Type of functions for turning stable data into current stae */
/*  return types TBD */
getUserList (char * room);
getRecentChats (char * room, int num);
getAllChats (char * room);
appendChat (char * room, char * user, char * chat);
likeChat (char * user, int clognum);
removeLike (char * user, int clognum);

#endif