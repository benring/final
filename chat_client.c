
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

static	char		server_group[MAX_GROUP_NAME];
static	char		my_server[MAX_GROUP_NAME];

static	int			state;

void Initialize();
void Print_menu(); 

void Read_message();


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

void User_command()   {
	char	command[130];
	char	group[MAX_GROUP_NAME];
	char 	arg[CHAT_LEN];
	int 	ret, i;

	printf("%s>", User);
	
	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 130, stdin ) == NULL ) 
		disconn_spread(mbox);

	switch( command[0] )  {
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
		join_group(mbox, server_group);
		
		/* Block until we get the Server's full private name 
		 *   OR we could infer it from the membership list, but we 
		 *   may want to do other stuff when the client first connects */
		Receive_welcome_msg();

		state = CONN;
		// ATTACH FD HERE
		break;

	case 'j':
		ret = sscanf(&command[2], "%s", group );
		if( ret < 1 )  {
			printf("Proper usage is j <room_name> \n");
			break;
		}
		else if (state < CONN) {
			loginfo("You must be logged in and connected to join a room\n");
			break;
		}
		logdb ("JOIN ROOM: <%s>\n", group);
		// SEND JOIN COMMAND TO SERVER 
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
		strcpy(User, arg);
		loginfo("Your username is now set to <%s>\n", User);
		state = LOGG;
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
	
	/* Connect to Spread */
	connect_spread(&mbox, User, Private_group);
	E_init();

	/* Initiate Membership join  */
	while (TRUE) {
		User_command();
	}
	
	disconn_spread(mbox);
	
	return(0);
}


void Initialize() {
	state = INIT;
}

void Print_menu()  {
	loginfo("Client Help Menu \n");
	loginfo("\t%-20s%s\n", "u <username>", "Set user name");
	loginfo("\t%-20s%s\n", "c <server #>", "Connect to a server, valid #'s are 1-5");
	loginfo("\t%-20s%s\n", "j <room>", "Join a chat room (must be connected to a server)");
	loginfo("\t%-20s%s\n", "q", "Quit");
}
