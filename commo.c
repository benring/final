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

void join_group (mailbox box, char name[MAX_GROUP_NAME])  {
	int res;
	res = SP_join(box, name);
	if(res < 0) {
		SP_error(res);
	}
	//printf("Joined to {%s}. \n", name);
}

void leave_group (mailbox box, char name[MAX_GROUP_NAME])  {
	int res;
	res = SP_leave(box, name);
	if(res < 0) {
		SP_error(res);
	}
	//printf("Left Group: {%s}. \n", name);
}


/*  Called upon termination */
void disconn_spread(mailbox box)  {
	//printf("\nTerminating.\n");
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
	//loginfo("Connected to daemon: '%s', user is <%s>, private group {%s}\n", SPREAD_DAEMON, user, private_group );
	return ret;
}


void send_message(mailbox box, char * group, char * msg, int len)   {
	int				ret;
	logdb("Sending to <%s>: '%s'\n", group, msg);
	ret= SP_multicast(box, AGREED_MESS, group, 2, len, msg);
	if (ret < 0 )  {
		SP_error(ret);
		exit( 0 );
	}
}


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
