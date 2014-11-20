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

/*  Actions  */
#define JOIN 10
#define LEAVE 11
#define LIKE 12
#define REMOVE 13

/*  Basic Message Struct:  TAG & PAYLOAD  */
typedef struct Message {
	char 	tag;
	char *	payload;
} Message;

typedef LTS_Vector_Message {
	lts_entry		lts[NUM_SERVERS]
}

typedef struct Join_Message {
	char	room[NAME_LEN];
	char	user[NAME_LEN];  // ???????
}



typedef struct LogState {
	int		room;
	int		chat;
	int		like;
} LogState;




#endif
