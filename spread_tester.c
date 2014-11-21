#include "sp.h"

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


#define MAX_MESSAGE_SIZE 1024

#define JOIN 0
#define SEND 1
#define LISTEN 2
#define DONE 3
#define ALL_SERVER_GROUP "server-all"

#define TOO_FAR_AHEAD 	40

#define END_OF_MESSAGES 0
#define DATA 1

/* Forward Declared functions */
static	void		Read_message();
static  void    	Send_message(char * group, char * mess);
static  void		Bye();
float 				time_diff (struct timeval s, struct timeval e);

/* Message handling vars */
static	char		User[80];
static  char    	Spread_name[80];
static  char    	Private_group[MAX_GROUP_NAME];
static  mailbox 	mbox;
static  unsigned int		mess[MAX_MESSAGE_SIZE];
static	char * 		HASHTAG;

/* Protocol vars  */
static 	unsigned int		me;
static  unsigned int		my_server_id;
static  int					connected_svr[5];
static  char				server_name[5][10];
static	char				server_group[10];
static	char				client_group[10];

static 	FILE            	*sink;
static 	char            	dest_file[10];


void join_group (char name[10])  {
	int res;
	res = SP_join(mbox, name);
	if(res < 0) {
		SP_error(res);
	}
	printf("Joined to <%s>. \n", name);
}

void connect_spread ()  {
	int				ret;
	sp_time 		test_timeout; 
	test_timeout.sec = 5;
	test_timeout.usec = 0;
	ret = SP_connect_timeout( Spread_name, User, 0, 1, &mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION ) {
		SP_error( ret );
		Bye();
	}
	printf("Connected name: <%s>, user is <%s>, private group <%s>\n", Spread_name, User, Private_group );
}

void initialize (char * server_index) {
	int 		i;
	char 		index;
	
	my_server_id = atoi(server_index);
	index = (char)(my_server_id + (int)'0');
	
	me = my_server_id - 1;
	strcpy(server_group, "server-grp");
	strcpy(User, "server-");
	User[7] = index;
	strcpy(client_group, "clients-");
	client_group[8] = index;
	HASHTAG = "#";
	
//	strcpy(Spread_name, "MY_SPREAD_NAME");

	
	for (i=0; i<5; i++) {
		strcpy(server_name[i], "server-");
		server_name[i][7] = (char)(i + (int)'0');
		connected_svr[i] = FALSE;
	}

}

int main (int argc, char *argv[])  {
	int	    		ret, i;
	
	char * 			all_servers_spread;
	char *			message;

	/* Initialization */
	
	/*  Handle Command line arguments */
	if (argc != 2)  {
		  printf("Usage: spread_tester  <server_index_#>\n");
		  Bye();
	} 

	initialize(argv[1]);
	
	/* Connect to Spread */
	connect_spread();
	E_init();

	/* Initiate Membership join  */
	join_group(server_group);
	join_group(client_group);

	logdb("Attaching reader event handler\n");
	E_attach_fd(mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY );
	E_handle_events();
	
	Bye();
	
	return(0);
}
 
static void 	Send_message(char * group, char * mess)   {
	int				ret;
	logdb("Sending to <%s>: '%s'\n", group, mess);
	ret= SP_multicast(mbox, AGREED_MESS, group, 2, strlen(mess), mess);
	if (ret < 0 )  {
		SP_error(ret);
		exit( 0 );
	}
}


static	void	Read_message()   {
	char		sender[MAX_GROUP_NAME];
	char		target_groups[5][MAX_GROUP_NAME];
	int		 	num_members;
	int		 	service_type;
	int16		mess_type;
	int		 	endian_mismatch;
	int		 	ret, i;

	service_type = 0;

	/*  Receive Message */
	ret = SP_receive(mbox, &service_type, sender, 100, &num_members, target_groups, 
		&mess_type, &endian_mismatch, sizeof(mess), (char *) mess );
	if (ret < 0 )  {
		SP_error(ret);
		exit(0);
	}
	
	/*  Processes Message */
	if (Is_regular_mess(service_type))	{
		logdb("Received a regular message from <%s>. Contents are: %s\n", sender, mess);
		logdb("   The user name is <%s>\n", strtok(sender, HASHTAG));
	}

	/* Process Group membership messages */
	else if(Is_membership_mess(service_type)) {
		logdb("Received a membership message from <%s>:\n", sender);
		for (i=0; i<num_members; i++) {
			logdb("  Target Group #%d: %s\n", i, strtok(target_groups[i], HASHTAG));
//			if (connected_srv == )
		}
		Send_message(server_group, "Welcome, servers");
//		Send_message(client_group, "Welcome, clients");
	}
	
	/* Bad message */
	else {
		logdb("Received a BAD message\n");	
	}

}

/*  Called upon termination */
static  void	Bye()  {
	printf("\nTerminating.\n");
	SP_disconnect(mbox);
	exit(0);
}

/*  OTHER HELPER function (for timing) */
float time_diff (struct timeval s, struct timeval e)  {
	float usec;
	usec = (e.tv_usec > s.tv_usec) ?
	       (float)(e.tv_usec-s.tv_usec) :
	       (float)(s.tv_usec-e.tv_usec)*-1;
	return ((float)(e.tv_sec-s.tv_sec) + usec/1000000);
}

