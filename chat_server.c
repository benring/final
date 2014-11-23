#include "config.h"
#include "transaction.h"
#include "commo.c"


#define ALL_SERVER_GROUP "server-all"

#define INIT 0
#define READY 1
#define RUN 2
#define RECONCILE 3

/* Forward Declared functions */
static	void		Run();
static	void		Reconcile();
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

typedef struct server_state {
//	linkedList<connected_clients>	clients;
	connected_client				clients[10];

//	linkedList<room_state>			rooms;
	room_state						rooms[10];

//	linkedList<updates>				updates;   /*sorted by lts*/
	int								fsm;
//	int								connected_servers[5];				
} server_state;


static server_state					state;


int main (int argc, char *argv[])  {
	
	/*  Handle Command line arguments */
	if (argc != 2)  {
		  printf("Usage: spread_tester  <server_index_#>\n");
		  exit(0);
	} 
	
	logdb("[TRANSITION] Entering INIT state\n");
	state.fsm = INIT;
	Initialize(argv[1]);
	
	/* Connect to Spread */
	connect_spread(&mbox, User, Private_group);
	E_init();

	/* Initiate Membership join  */
	logdb("[TRANSITION] Entering READY state\n");
	state.fsm = READY;
	join_group(mbox, server_group);
	join_group(mbox, client_group);

	logdb("Attaching reader event handler\n");
	logdb("[TRANSITION] Entering RUN state\n");
	state.fsm = RUN;
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

int num_connected_servers ()  {
	int 	num, i;
	num = 0;
	for (i=0; i<5; i++)  {
		if (connected_svr[i]) {
			num++;
		}
	}
	return num;
}

int	get_server_num (char * svr_name) {
	int num;
	
	// Format for the svr_name is #server-X#ugradYY, thus X=svr_name[8]
	num = (int)(svr_name[8]) - (int)('0');
	logdb ("Get Server %d from <%s>\n", num, svr_name);
	return num-1;  
}


void print_connected_servers () {
	int i;
	logdb("Connected Servers:\n");
	for (i=0; i<5; i++)  {
		if (connected_svr[i]) {
			logdb("  -Server #%d\n", i+1);
		}
	}
}


void handle_server_change(int join, char * who) {
	if (join)  {
		if (num_connected_servers() > 1) {
			state.fsm = RECONCILE;
			Reconcile();
		}
	}
	else {
		/*
		 foreach room in state.rooms:
			foreach a in attendees:
				if a.server == departed_server:
					a.remove()
		 */
	}
	
}

void handle_client_change(int join, char * who) {
	if (join)  {
		//clients.push(who)
		logdb("%s has joined me as a client\n", who);
		send_message(mbox, who, "Welcome");
	}
	
}


void handle_client_command() {return;}
void handle_server_update() {return;}


void Initialize (char * server_index) {
	int 		i;
	char 		index;
	
	my_server_id = atoi(server_index);
	index = (char)(my_server_id + (int)'0');
	
	me = my_server_id - 1;

	strcpy(server_group, SERVER_ALL_GROUP_NAME);
	strcpy(User, SERVER_NAME_PREFIX);
	User[7] = index;
	strcpy(client_group, SERVER_GROUP_PREFIX);
	client_group[8] = index;
	HASHTAG = "#";
	
	for (i=0; i<5; i++) {
		strcpy(server_name[i], "server-");
		server_name[i][7] = (char)(i + (int)'0');
		connected_svr[i] = FALSE;
	}
}

static	void	Reconcile() {return;}

static	void	Run()   {
	char		sender[MAX_GROUP_NAME];
	char		target_groups[5][MAX_GROUP_NAME];
	int		 	num_members;
	int		 	service_type;
	int16		mess_type;
	int		 	endian_mismatch;
	int		 	ret, i;
	
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
		logdb("Received a regular message from <%s>. Contents are: %s\n", sender, (char *) mess);
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

			// AWAYS update connected server list
			for (i=0; i<5; i++) {
				connected_svr[i] = FALSE;
			}
			for (i=0; i<num_members; i++) {
				connected_svr[get_server_num(target_groups[i])] = TRUE;
			}

			handle_server_change(joined, client);
		}
		else {
			// IF WE USE SPread Groups for room attendee lists, we will need to 
			// distinguish "our" client leaving us vs "any" client leaving any room
			handle_client_change(joined, client); 
		}

		//		Send_message(server_group, "Welcome, servers");
//		Send_message(client_group, "Welcome, clients");
	}
	
	/* Bad message */
	else {
		logdb("Received a BAD message\n");	
	}

	print_connected_servers();

}



/*  OTHER HELPER function (for timing) */
float time_diff (struct timeval s, struct timeval e)  {
	float usec;
	usec = (e.tv_usec > s.tv_usec) ?
	       (float)(e.tv_usec-s.tv_usec) :
	       (float)(s.tv_usec-e.tv_usec)*-1;
	return ((float)(e.tv_sec-s.tv_sec) + usec/1000000);
}

