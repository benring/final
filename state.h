#ifndef STATE_H
#define STATE_H

typedef struct lts_entry {
	unsigned	int		ts;
	unsigned	int		pid;
} lts_entry;



typedef struct room_info {
	char						name[NAME_LEN];
	char						spread_group[NAME_LEN];
//	linkedList<name, server>	attendees;
//	linkedList<chat_message>	history;
} room_info;



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
