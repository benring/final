/******************************************************************************
 *  File:   message.h
 *
 *  Author: Benjamin Ring & Josh Wheeler
 *  Date:   
 *  Description:  Define all message structures and associated functions for 
 *      preparing and receiving  messages
 ********************************************************************************/


#ifndef MESSAGE_H
#define MESSAGE_H

#include "transaction.h"

#define MAX_MESSAGE_SIZE 256

/*  Message Tag Codes  */
#define LTS_VECTOR 'T'
#define UPDATE 'U'
#define APPEND_MSG 'A'
#define JOIN_MSG 'J'
#define VIEW_MSG 'V'
#define HISTORY_MSG 'H'
#define LIKE_MSG 'L'


/*  Basic Message Struct:  TAG & PAYLOAD  */
typedef struct Message {
	char 	tag;
	char 	payload[MAX_MESSAGE_SIZE-1];
} Message;

typedef struct LTSVectorMessage {
        unsigned  int sender;
	unsigned  int lts[MAX_SERVERS];
} LTSVectorMessage;

typedef struct JoinMessage {
	char	user[NAME_LEN];
	char	room[NAME_LEN];
	lts_entry	lts;
} JoinMessage;

typedef struct AppendMessage {
	char	user[NAME_LEN];
	char	room[NAME_LEN];
	char	text[CHAT_LEN];
	lts_entry	lts;
} AppendMessage;

typedef struct HistoryMessage {
	char	user[NAME_LEN];
	char	room[NAME_LEN];
} HistoryMessage;


typedef struct ViewMessage {
	int		connected_server[5];
} ViewMessage;

typedef struct LikeMessage {
	char		user[NAME_LEN];
	lts_entry	lts;
	char		action;
	lts_entry	ref;
} LikeMessage;




typedef struct LogState {
	int		room;
	int		chat;
	int		like;
} LogState;



/*  ALl the PREPARE methods to create the new message */
void prepareViewMsg (Message * m, int svr[5]) {
	ViewMessage *v;
	int i;
	
	m->tag = VIEW_MSG;
	v = (ViewMessage *) &(m->payload);
	for (i=0; i<5; i++) {
		v->connected_server[i] = svr[i];
	}
}


void prepareJoinMsg (Message * m, char * roomname, char * user, lts_entry lts) {
	JoinMessage *jm;
	logdb("Preparing Join Msg:  room: <%s>, user: <%s>\n", roomname, user);
	m->tag = JOIN_MSG;
	jm = (JoinMessage *) &(m->payload);
	strcpy(jm->user, user);
	strcpy(jm->room, roomname);
	jm->lts.pid = lts.pid;
	jm->lts.ts = lts.ts;
}

void prepareHistoryMsg (Message * m, char * roomname, char * user) {
	HistoryMessage *hm;
	logdb("Preparing History Msg:  room: <%s>, user: <%s>\n", roomname, user);
	m->tag = HISTORY_MSG;
	hm = (HistoryMessage *) &(m->payload);
	strcpy(hm->user, user);
	strcpy(hm->room, roomname);
}



void prepareAppendMsg (Message * m, char * roomname, char * user, char * text, lts_entry lts) {
	AppendMessage *am;
	logdb("Preparing Append Msg:  room: <%s>, user: <%s>, text: '%s'\n", roomname, user, text);
	m->tag = APPEND_MSG;
	am = (AppendMessage *) &(m->payload);
	strcpy(am->user, user);
	strcpy(am->room, roomname);
	strcpy(am->text, text);
	am->lts.pid = lts.pid;
	am->lts.ts = lts.ts;
}


void prepareLikeMsg (Message * m, char * user, lts_entry ref, char act, lts_entry lts) {
	LikeMessage *lm;
	logdb("Preparing Like Msg:  user: <%s>, action: <%c>, LTS: %d,%d\n", 
			user, act, ref.ts, ref.pid);
	m->tag = LIKE_MSG;
	lm = (LikeMessage *) &(m->payload);
	strcpy(lm->user, user);
	lm->ref.ts = ref.ts;
	lm->ref.pid = ref.pid;
	lm->action = act;
	lm->lts.pid = lts.pid;
	lm->lts.ts = lts.ts;
}


#endif
