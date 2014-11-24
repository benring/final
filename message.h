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

/*  Message Tag Codes  */
#define LTS_VECTOR 'L'
#define UPDATE 'U'
#define JOIN_ROOM 'J'
#define VIEW_MSG 'V'

/*  Actions  */
#define JOIN 10
#define LEAVE 11
#define ADD_LIKE 12
#define REM_LIKE 13

/*  Basic Message Struct:  TAG & PAYLOAD  */
typedef struct Message {
	char 	tag;
	char *	payload;
} Message;

typedef struct LTSVectorMessage {
	lts_entry		lts[NUM_SERVERS];
} LTS_Vector_Message;

typedef struct JoinMessage {
	char	room[NAME_LEN];
	char	user[NAME_LEN];  // ???????
} Join_Message;

typedef struct ViewMessage {
	int		connected_server[5];
} ViewMessage;


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

#endif
