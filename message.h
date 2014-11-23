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
#define ADD_LIKE 12
#define REM_LIKE 13

/*  Basic Message Struct:  TAG & PAYLOAD  */
typedef struct Message {
	char 	tag;
	char *	payload;
} Message;

typedef struct LTS_Vector_Message {
	lts_entry		lts[NUM_SERVERS];
} LTS_Vector_Message;

typedef struct Join_Message {
	char	room[NAME_LEN];
	char	user[NAME_LEN];  // ???????
} Join_Message;



typedef struct LogState {
	int		room;
	int		chat;
	int		like;
} LogState;




#endif
