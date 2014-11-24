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

#define MAX_MESSAGE_SIZE 1024

/*  Message Tag Codes  */
#define LTS_VECTOR 'L'
#define UPDATE 'U'
#define APPEND_MSG 'A'
#define JOIN_MSG 'J'
#define VIEW_MSG 'V'
#define HISTORY_MSG 'H'

/*  Actions  */
#define JOIN 10
#define LEAVE 11
#define ADD_LIKE 12
#define REM_LIKE 13

/*  Basic Message Struct:  TAG & PAYLOAD  */
typedef struct Message {
	char 	tag;
	char 	payload[MAX_MESSAGE_SIZE-1];
} Message;

typedef struct LTSVectorMessage {
	lts_entry		lts[NUM_SERVERS];
} LTS_Vector_Message;

typedef struct JoinMessage {
	char	user[NAME_LEN];
	char	room[NAME_LEN];
} JoinMessage;

typedef struct AppendMessage {
	char	user[NAME_LEN];
	char	room[NAME_LEN];
	char	text[CHAT_LEN];
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
} LikeMessage;



typedef struct LogState {
	int		room;
	int		chat;
	int		like;
} LogState;



/*  ALl the PREPARE methods to create the new message */
void prepareViewMsg (Message * m, int * svr) {
	ViewMessage *v;
	int i;
	
	m->tag = VIEW_MSG;
	v = (ViewMessage *) &(m->payload);
	for (i=0; i<5; i++) {
		v->connected_server[5] = (svr == 0) ? FALSE : svr[i];
	}
}


void prepareJoinMsg (Message * m, char * roomname, char * user) {
	JoinMessage *jm;
	logdb("Preparing Join Msg:  room: <%s>, user: <%s>\n", roomname, user);
	m->tag = JOIN_MSG;
	jm = (JoinMessage *) &(m->payload);
	strcpy(jm->user, user);
	strcpy(jm->room, roomname);
}

void prepareHistoryMsg (Message * m, char * roomname, char * user) {
	HistoryMessage *hm;
	logdb("Preparing History Msg:  room: <%s>, user: <%s>\n", roomname, user);
	m->tag = HISTORY_MSG;
	hm = (HistoryMessage *) &(m->payload);
	strcpy(hm->user, user);
	strcpy(hm->room, roomname);
}



void prepareAppendMsg (Message * m, char * roomname, char * user, char * text) {
	AppendMessage *am;
	logdb("Preparing Append Msg:  room: <%s>, user: <%s>, text: '%s'\n", roomname, user, text);
	m->tag = APPEND_MSG;
	am = (AppendMessage *) &(m->payload);
	strcpy(am->user, user);
	strcpy(am->room, roomname);
	strcpy(am->text, text);
}


void prepareLikeMsg (Message * m, char * user, lts_entry ref, char act) {
	LikeMessage *lm;
	logdb("Preparing Like Msg:  room: <%s>, user: <%s>\n", roomname, user);
	m->tag = LIKE_MSG;
	lm = (LikeMessage *) &(m->payload);
	strcpy(lm->user, user);
	lm->lts.ts = 0;
	lm->lts.pid = 0;
	lm->action = act;
}


#endif
