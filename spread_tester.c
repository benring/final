#include "sp.h"

#include "config.h"
#include "commo.c"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


#define ALL_SERVER_GROUP "server-all"

#define JOIN 0
#define SEND 1
#define LISTEN 2
#define DONE 3

/* Forward Declared functions */
static	void		Run();
void 				Initialize (char * server_index);
float 				time_diff (struct timeval s, struct timeval e);

/* Message handling vars */
static	char		User[80];
static  char    	Private_group[MAX_GROUP_NAME];
static  mailbox 	mbox;
static  unsigned int		mess[MAX_MESSAGE_SIZE];
static	char * 		HASHTAG;

/* Protocol vars  */
static 	unsigned int		me;
static  unsigned int		my_server_id;
static  int					connected_svr[5];
static  char				server_name[5][MAX_GROUP_NAME];
static	char				server_group[MAX_GROUP_NAME];
static	char				client_group[MAX_GROUP_NAME];

//static 	FILE            	*sink;
//static 	char            	dest_file[10];



int main (int argc, char *argv[])  {
	int	    		ret, i;

	/*  Handle Command line arguments */
	if (argc != 2)  {
		  printf("Usage: spread_tester  <server_index_#>\n");
		  exit(0);
	} 

	Initialize(argv[1]);
	
	/* Connect to Spread */
	connect_spread(&mbox, User, Private_group);
	E_init();

	/* Initiate Membership join  */
	join_group(mbox, server_group);
	join_group(mbox, client_group);

	logdb("Attaching reader event handler\n");
	E_attach_fd(mbox, READ_FD, Run, 0, NULL, HIGH_PRIORITY );
	E_handle_events();
	
	disconn_spread(mbox);
	
	return(0);
}
 
int is_member (char recipients[5][MAX_GROUP_NAME], int num_recipients, char node[MAX_GROUP_NAME])  {
	int 	found_user, i;
	found_user = FALSE;
	for (i=0; i<num_recipients; i++) {
//			logdb("  Compare: <%s> vs <%s>\n", recipients[i], node);
		if (strcmp(recipients[i], node) == 0) {
			found_user = TRUE;
		}
	}
	return found_user;
}

void handle_server_change(int join, char * who) {return;}
void handle_client_change(int join, char * who) {return;}
void handle_client_command() {return;}
void handle_server_update() {return;}


void Initialize (char * server_index) {
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
	
	for (i=0; i<5; i++) {
		strcpy(server_name[i], "server-");
		server_name[i][7] = (char)(i + (int)'0');
		connected_svr[i] = FALSE;
	}
}


static	void	Run()   {
	char		sender[MAX_GROUP_NAME];
	char		target_groups[5][MAX_GROUP_NAME];
	int		 	num_members;
	int		 	service_type;
	int16		mess_type;
	int		 	endian_mismatch;
	int		 	ret;
	
	char 		full_name[MAX_GROUP_NAME];
	char		client[MAX_GROUP_NAME];
	char		*name;
	int			joined;
	char		*status_change_msg;

	service_type = 0;

	logdb("Waiting for a message...");
	
	/*  Receive Message */
	ret = SP_receive(mbox, &service_type, sender, 100, &num_members, target_groups, 
		&mess_type, &endian_mismatch, sizeof(mess), (char *) mess );
	if (ret < 0 )  {
		SP_error(ret);
		exit(0);
	}
	logdb("Received from %s\n", sender);
	
	// WE CAN USE MULTIPLE HASHTAGS for a complete naming convention
	// e.g.:  #user#client#server#ugradxx
	name = strtok(sender, HASHTAG);

	/*  Processes Message */
	if (Is_regular_mess(service_type))	{
		logdb("Received a regular message from <%s>. Contents are: %s\n", sender, mess);
		logdb("   The user name is <%s>\n", name);
		if (strcmp(sender, server_group) == 0) {
			// MAY WANT TO CHECK IF ITS OUR MESSAGE & DO SOMETHING DIFFERENT (OR IGNORE IT)
			handle_server_update();
		}
		else {
			handle_client_command(sender); 
		}	
	}

	/* Process Group membership messages */
	else if(Is_membership_mess(service_type)) {
		// MAY NEED TO CONSIDER MESS_TYPE to parse mess properly
		memcpy (client, mess + 6, 32);

		joined = is_member(target_groups, num_members, client);
		status_change_msg = (joined) ? "JOINED" : "LEFT";
		loginfo("  User <%s> has %s Group {%s} \n", client, status_change_msg, sender);
		
		if (strcmp(sender, server_group) == 0) {
			handle_server_change(joined, sender);
		}
		else {
			// IF WE USE SPread Groups for room attendee lists, we will need to 
			// distinguish "our" client leaving us vs "any" client leaving any room
			handle_client_change(joined, sender); 
		}

		//		Send_message(server_group, "Welcome, servers");
//		Send_message(client_group, "Welcome, clients");
	}
	
	/* Bad message */
	else {
		logdb("Received a BAD message\n");	
	}

}



/*  OTHER HELPER function (for timing) */
float time_diff (struct timeval s, struct timeval e)  {
	float usec;
	usec = (e.tv_usec > s.tv_usec) ?
	       (float)(e.tv_usec-s.tv_usec) :
	       (float)(s.tv_usec-e.tv_usec)*-1;
	return ((float)(e.tv_sec-s.tv_sec) + usec/1000000);
}

