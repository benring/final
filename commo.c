/******************************************************************************
 * File:     commo.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Communication (commo) wrapper functions to encapsulate
 *    common spread operations to both client & server
 *
 *****************************************************************************/
#ifndef COMMO_H
#define COMMO_H

#include "sp.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define SPREAD_DAEMON "10140@localhost"

#define SERVER_ALL_GROUP_NAME "server-grp"
#define SERVER_NAME_PREFIX "server-"
#define SERVER_GROUP_PREFIX	"clients-"

static	char * 			HASHTAG = "#";

/*  join_group:  join mailbox to provided group  */
void join_group (mailbox box, char name[MAX_GROUP_NAME])  {
	int res;
	res = SP_join(box, name);
	if(res < 0) {
		SP_error(res);
	}
}

/*  leave_group:  remove mailbox from provided group  */
void leave_group (mailbox box, char name[MAX_GROUP_NAME])  {
	int res;
	res = SP_leave(box, name);
	if(res < 0) {
		SP_error(res);
	}
}

/*  Called upon termination */
void disconn_spread(mailbox box)  {
	SP_disconnect(box);
	exit(0);
}

/*  connect_spread:  returns the private group name for this node's connection */
int connect_spread (mailbox * box, char user[MAX_GROUP_NAME], char private_group[MAX_GROUP_NAME])  {
	int				ret;
	sp_time 		test_timeout; 
	test_timeout.sec = 5;
	test_timeout.usec = 0;
	ret = SP_connect_timeout(SPREAD_DAEMON, user, 0, 1, box, private_group, test_timeout );
	return ret;
}

/*  send_message:  sends a message to provided group  */
void send_message(mailbox box, char * group, Message * msg)   {
	int				ret;
  int       len;
  
  switch(msg->tag) {
    case JOIN_MSG:
      len = JOIN_MSIZE;
      break;
    case APPEND_MSG:
      len = APPEND_MSIZE;
      break;
    case LIKE_MSG:
      len = LIKE_MSIZE;
      break;
    case VIEW_MSG:
      len = VIEW_MSIZE;
      break;
    case LTS_VECTOR:
      len = LTSVECTOR_MSIZE;
      break; 
    default:
      printf("ERROR Bad message received!");
      exit(0);
  }
  
  
	logdb("Sending to <%s>: '%s'\n", group, (char *) msg);
	ret= SP_multicast(box, AGREED_MESS, group, 2, len, (char *) msg);
	if (ret < 0 )  {
		SP_error(ret);
		exit( 0 );
	}
}

/* is_client_in_list: Helper function to check if a given name is in a list 
 *    of spread names. Used to help clients/servers determine if a user
 *    has joined/left a group */
int is_client_in_list(char *cli, int list_len, char cli_list[MAX_CLIENTS][MAX_GROUP_NAME]) {
  int i;
  for(i=0; i<list_len; i++) {
    if(strcmp(cli, cli_list[i]) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}

#endif
