/******************************************************************************
 * File:     message.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  see message.h
 *****************************************************************************/
#include "message.h"


/* Prepare a View Message for sending connected servers in network view  */
void prepareViewMsg (Message * m, int svr[MAX_SERVERS]) {
	ViewMessage *v;
	int i;
	
	m->tag = VIEW_MSG;
	v = (ViewMessage *) &(m->payload);
	for (i=0; i<5; i++) {
		v->connected_server[i] = svr[i];
	}
}

/* Prepare a Join Message for joining a new room */
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

/* Prepare a History Message for requesting/sending chat history */
void prepareHistoryMsg (Message * m, char * roomname, char * user) {
	HistoryMessage *hm;
	logdb("Preparing History Msg:  room: <%s>, user: <%s>\n", roomname, user);
	m->tag = HISTORY_MSG;
	hm = (HistoryMessage *) &(m->payload);
	strcpy(hm->user, user);
	strcpy(hm->room, roomname);
}

/* Prepare a Append Message for a new user chat text */
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

/* Prepare a Like Message for a user liking/removing like entry for a chat */
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

