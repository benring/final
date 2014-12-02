/******************************************************************************
 * File:     commo.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Communication (commo) wrapper functions to encapsulate
 *    common spread operations to both client & server
 *
 *****************************************************************************/
#ifndef MESSAGE_H
#define MESSAGE_H

#include "transaction.h"

#define MAX_MESSAGE_SIZE 1024

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


/* Prepare a View Message for sending connected servers in network view  */
void prepareViewMsg (Message * m, int svr[MAX_SERVERS]);

/* Prepare a Join Message for joining a new room */
void prepareJoinMsg (Message * m, char * roomname, char * user, lts_entry lts);

/* Prepare a History Message for requesting/sending chat history */
void prepareHistoryMsg (Message * m, char * roomname, char * user);

/* Prepare a Append Message for a new user chat text */
void prepareAppendMsg (Message * m, char * roomname, 
                       char * user, char * text, lts_entry lts);

/* Prepare a Like Message for a user liking/removing like entry for a chat */
void prepareLikeMsg (Message * m, char * user, 
                     lts_entry ref, char act, lts_entry lts);

#endif
