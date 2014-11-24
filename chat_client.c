
#include "message.h"
#include "commo.c"


#define INIT 0
#define LOGG 1
#define CONN 2
#define RUN 3
#define USER_NAME_LIMIT 10


//typedef struct client_state {
//	int					server;
//	room_state			room;
//	char				user[NAME_LEN];
//	char				spread_grp[NAME_LEN];
//} client_state;


/* Message handling vars */
static	char		User[USER_NAME_LIMIT];
static  char    	Private_group[MAX_GROUP_NAME];
static  mailbox 	mbox;
Message				*out_msg;


static	char		server_group[MAX_GROUP_NAME];
static	char		my_server[MAX_GROUP_NAME];
static	char		my_room[MAX_GROUP_NAME];
static	char		server_inbox[MAX_GROUP_NAME];
static 	char		last_command;

static	int			state;

void Initialize();
void Print_menu(); 

void Read_message() {return;}


void Receive_welcome_msg() {
	char		sender[MAX_GROUP_NAME];
	char		target_groups[5][MAX_GROUP_NAME];
	int		 	num_members;
	int		 	service_type;
	int16		mess_type;
	int		 	endian_mismatch;
	int		 	ret;
	char		mess[MAX_MESSAGE_SIZE];

	do {
		ret = SP_receive(mbox, &service_type, sender, 100, &num_members, target_groups, 
				&mess_type, &endian_mismatch, sizeof(mess), (char *) mess );
		if (ret < 0 )  {
			SP_error(ret);
			exit(0);
		}
//		logdb("Received from %s. Content is: %s\n", sender, mess);
	
	} while (mess[0] != 'W');
	
	strcpy(my_server, sender);
	logdb("My Server is now set to %s\n", my_server);
}


void Run ()  {
	
}

void User_command()   {
	char	command[130];
	char	group[MAX_GROUP_NAME];
	char 	arg[CHAT_LEN];
	int 	ret, i;
	char 	client_id[3];
	
	lts_entry	ref;

	printf("%s>", User);
	
	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 130, stdin ) == NULL ) 
		disconn_spread(mbox);
	
	last_command = command[0];

	switch(last_command)  {

	case 'a':
		ret = sscanf(&command[2], "%s", arg);
		if( ret < 1 )  {
			printf("Proper usage is a <chat_text> \n");
			break;
		}
		else if (state < RUN) {
			loginfo("You must be logged in and joined to a room to chat.\n");
			break;
		}
		logdb ("Appending: <%s> to room: <%s>\n", arg, my_room);
		// SEND APPEND COMMAND TO SERVER 
		prepareAppendMsg(out_msg, my_room, User, arg);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));

		
		break;


	case 'c':
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1 || strlen(arg) > 1)  {
			loginfo("Proper usage is c <server #> \n");
			break;
		}
		else if (arg[0] < '1' || arg[0] > '5') {
			loginfo("Please enter a valid server # between 1-5 \n");
			break;
		}
		else if (state < LOGG) {
			loginfo("You must select a username before connecting to a server\n");
			break;
		}
		logdb ("CONNECT to Server <%s>\n", arg);
		strcpy(server_group, SERVER_GROUP_PREFIX);
		server_group[8] = arg[0];
		
		User[0] = arg[0];

		i = 1;
		do {
			if (i == 100)  {
				SP_error(ret);
				logerr("User is only allowed 100 clients per server!\n");
				disconn_spread(mbox);
			}
			sprintf(client_id, "%02d", i++);
			memcpy(&User[1], client_id, 2);
			ret = connect_spread(&mbox, User, Private_group);
			

		} while (ret != ACCEPT_SESSION);
		/* Connect to Spread */
		E_init();

		loginfo("Your username is now set to <%c%c%c%s>\n", User[0], User[1], User[2], &User[3]);

		join_group(mbox, server_group);
		strcpy(server_inbox, SERVER_NAME_PREFIX);
		server_inbox[7] = arg[0];
		
		/* Block until we get the Server's full private name 
		 *   OR we could infer it from the membership list, but we 
		 *   may want to do other stuff when the client first connects */
//		Receive_welcome_msg();

		state = CONN;
		// ATTACH FD HERE
		break;

	case 'h':
		if (state < RUN) {
			loginfo("You must be logged in and joined to a room to request a chat history.\n");
			break;
		}
		logdb ("Sending history request for <%s>\n", my_room);
		prepareHistoryMsg(out_msg, my_room, User);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		
		break;

	case 'l':
		if (state < RUN) {
			loginfo("You must be logged in and joined to a room to like a chat.\n");
			break;
		}
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1)  {
			loginfo("Proper usage is l <chat #> \n");
			break;
		}
		// OTHER LIKE ERROR CHECKING GOES HERE
		logdb ("Liking Chat # <%s>\n", my_room);
		
		// TEMP DUMMY VAL FOR NOW
		ref.ts = 0;
		ref.pid = 1;
		prepareLikeMsg(out_msg, User, ref, ADD_LIKE);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;

	case 'r':
		if (state < RUN) {
			loginfo("You must be logged in and joined to a room to remove a like.\n");
			break;
		}
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1)  {
			loginfo("Proper usage is r <chat #> \n");
			break;
		}
		// OTHER REMOVE-LIKE ERROR CHECKING GOES HERE
		logdb ("Un-Liking Chat # <%s>\n", my_room);
		
		// TEMP DUMMY VAL FOR NOW
		ref.ts = 0;
		ref.pid = 1;
		prepareLikeMsg(out_msg, User, ref, REM_LIKE);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;


	case 'j':
		ret = sscanf(&command[2], "%s", arg);
		if( ret < 1 )  {
			printf("Proper usage is j <room_name> \n");
			break;
		}
		else if (ret > MAX_GROUP_NAME) {
			loginfo("Please use a name less than %d characters long\n", MAX_GROUP_NAME);
		}
		else if (state < CONN) {
			loginfo("You must be logged in and connected to join a room\n");
			break;
		}
		strcpy(my_room, arg);
		logdb ("JOIN ROOM: <%s>\n", my_room);
		// SEND JOIN COMMAND TO SERVER 
		prepareJoinMsg(out_msg, my_room, User);
		logdb("Message contents: <%s>\n", out_msg);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		state = RUN;
		
		break;

	case 'q':
		logdb("QUITTING\n");
		disconn_spread(mbox);
		break;
		
	case 'u':
		logdb("SETTING USERNAME\n");
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1) {
			loginfo("Proper usage is u <user_name> \n");
			break;
		}
		else if (ret > USER_NAME_LIMIT)  {
			loginfo("Please limit your name to %d charaters or less\n", USER_NAME_LIMIT);
			break;
		}
		User[0] = '0';
		User[1] = '0';
		User[2] = '0';
		loginfo("user is <%s>, arg is <%s>\n", User, arg);
		strcpy(&User[3], arg);
		loginfo("Your username is now set to <%c%c%c%s>\n", User[0], User[1], User[2], &User[3]);
		state = LOGG;
		break;
		
	case 'v':
		if (state < CONN) {
			loginfo("You must be logged in and connected to a server to view connected servers.\n");
			break;
		}
		logdb("Sending view server request to server #%s\n", my_server)
		
		out_msg->tag = VIEW_MSG;
		send_message (mbox, my_server, out_msg, 1);
		break;

	

	case '?':
		Print_menu();
		break;

	
	default:
		loginfo("Invalid command. Printing help menu.. \n");
		Print_menu();
	}
}




int main (int argc, char *argv[])  {
	
	/*  Handle Command line arguments */
	if (argc > 2)  {
		  printf("Usage: chat_client   {no args}\n");
		  exit(0);
	} 
	
	Initialize();
	
	while (state < CONN) {
		User_command();
	}
	
	E_attach_fd( 0, READ_FD, User_command, 0, NULL, LOW_PRIORITY );
	E_attach_fd( mbox, READ_FD, Read_message, 0, NULL, LOW_PRIORITY );
	E_handle_events();

	disconn_spread(mbox);
	
	return(0);
}


void Initialize() {
	state = INIT;
	out_msg = malloc(MAX_MESSAGE_SIZE);
}

void Print_menu()  {
	loginfo("Client Help Menu \n");
	loginfo("\t%-20s%s\n", "u <username>", "Set user name");
	loginfo("\t%-20s%s\n", "c <server #>", "Connect to a server, valid #'s are 1-5");
	loginfo("\t%-20s%s\n", "v", "View connected servers (must be connected to a server)");
	loginfo("\t%-20s%s\n", "j <room>", "Join a chat room (must be connected to a server)");
	loginfo("\t%-20s%s\n", "a <chat_text>", "Append chat text to room (must be joined to a room)");
	loginfo("\t%-20s%s\n", "h", "Get chat room History (must be joined to a room)");
	loginfo("\t%-20s%s\n", "l <chat #>", "Like a chat text (must be joined to a room)");
	loginfo("\t%-20s%s\n", "r <chat #>", "Remove a Like (must be joined to a room & have previously likes that chat text)");
	loginfo("\t%-20s%s\n", "q", "Quit");
}
