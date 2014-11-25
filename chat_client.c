
#include "message.h"
#include "commo.c"
#include "chat_ll.h"
#include "like_ll.h"


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
static	char		    User[USER_NAME_LIMIT];
static  char    	  Private_group[MAX_GROUP_NAME];
static  mailbox 	  mbox;
static  Message		  *out_msg;
static  Message     in_msg;
static	lts_entry		null_lts;
static	char		    mess[MAX_MESSAGE_SIZE];

/*  Client Global State  */
static	int			  state;
static	char		  server_group[MAX_GROUP_NAME];
static	char		  my_room[MAX_GROUP_NAME];
static  char      my_room_distrolist[MAX_GROUP_NAME];
static	char		  server_inbox[MAX_GROUP_NAME];
static 	char		  last_command;
static  chat_ll   chat_room;
static  int       connected_server[5];
static  int       my_server;    /* Indexed from 0,  -1 means disconnected */


void Initialize();
void Print_menu(); 

void process_server_message() {
  Message 				*out_msg;
	AppendMessage 	*am;
	HistoryMessage 	*hm;
	LikeMessage 		*lm;
  ViewMessage     *vm;
  chat_entry      *ce;
  chat_info       *ch;
  int             i;
	
	logdb("Received '%c' Update from server\n", in_msg.tag);

  switch (in_msg.tag) {

    /* ---------  APPEND MESSAGE -- FROM: SVR  ------------*/
    case APPEND_MSG:
			am = (AppendMessage *) in_msg.payload;

      /*  Do not process a duplicate chat message */
      if (chat_ll_get_inorder(&chat_room, am->lts)) {
        return;
      }
      
      /*  Create a new chat entry */
      ch = malloc(sizeof(chat_info));
      ch->lts.pid = am->lts.pid;
      ch->lts.ts = am->lts.ts;
      ch->likes = like_ll_create();
      strcpy(ch->chat.user, am->user);
      strcpy(ch->chat.room, am->room);
      strcpy(ch->chat.text, am->text);

      logdb("  New chat from server: LTS (%d,%d), User <%s> said '%s'\n", 
        ch->lts.ts, ch->lts.pid, ch->chat.user, ch->chat.text);
      
      /* Append to global chat list  */
      chat_ll_insert_inorder(&chat_room, *ch);
      
      /* Trim room to most recent 25 chats */ 
      while(chat_ll_length(&chat_room) > 25) {
        chat_ll_remove_first(&chat_room);
      }
      
      chat_ll_print(&chat_room); 

      break;  
      
   /* ---------  LIKE MESSAGE -- FROM: SVR  ------------*/
    case LIKE_MSG:
      break;

   /* ---------  VIEW MESSAGE -- FROM: SVR  ------------*/
    case VIEW_MSG:
      vm = (ViewMessage *) in_msg.payload;
      for (i=0; i<5; i++) {
        connected_server[i] = vm->connected_server[i];
      }
      logdb("  Received conn-server status\n");
      loginfo("CONNECTED servers:  [ ");
      for (i=0; i<5; i++) {
        if (vm->connected_server[i]) {
          loginfo(" <SVR %d>", i+1);
        }
      }
      loginfo("  ]\n");
      break;

      
    default:
      logerr("ERROR! Received a non-update from server\n");
    }
}

void process_client_change() {return;}

void Read_message() {
  char		sender[MAX_GROUP_NAME];
  char		target_groups[100][MAX_GROUP_NAME];
  int			num_target_groups;
  int			service_type;
  int16		mess_type;
  int			endian_mismatch;
  int			ret;

  char*  	changed_group;
  int    	num_members;
  char*  	members;
  char    *name;

  service_type = 0;

  ret = SP_receive(mbox, &service_type, sender, 100, &num_target_groups, target_groups,
  	           &mess_type, &endian_mismatch, sizeof(Message), (char *) &in_msg );
  if (ret < 0 )  {
    SP_error(ret);
    exit(0);
  }

  logdb("----------------------\n");
  name = strtok(sender, HASHTAG);
  
  /*  Processes Message -- should only be from our server */
  if (Is_regular_mess(service_type))	{
    if (name[0] != 's') {
      logerr("ERROR! Bad Sender. Message received from %s, name: %s\n", sender, name);
      return;
    }
    logdb("  Server message. Contents -->: %s\n", (char *) &in_msg);
    process_server_message();
  }

  /* Process Group membership messages */
  else if(Is_membership_mess(service_type)) {

    /* Re-interpret the fields passed to SP receive */
    changed_group = sender;
    num_members   = num_target_groups;
    members       = target_groups;

    if (strcmp(changed_group, server_group) == 0) {
      if (name[0] == 's') {
        /* Our server is disconnected */
        logdb("Server, %s, is disconnected")
        // TODO: Handle a lost server
      }
      // OTHERWISE: Ignore, other clients join/leave server's client group

    }
    else {
      /* Client has joined/left our room  */
      process_client_change(num_members, members);
    }
  }

  /* Bad message */
  else {
    logdb("Received a BAD message\n");
  }
  
  
  }


void User_command()   {
	char	command[100];
	char	group[MAX_GROUP_NAME];
	char 	arg[CHAT_LEN];
	int 	ret, i;
	char 	client_id[3];
	
	lts_entry	ref;

	printf("%s>", User);
	
	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 100, stdin ) == NULL ) 
		disconn_spread(mbox);
	
	last_command = command[0];

	switch(last_command)  {

  /* ------------- APPEND --------------------------------------*/
	case 'a':
		ret = sscanf(&command[2], "%[^\n]s", arg);
		if( ret < 1 )  {
			printf("Proper usage is a <chat_text> \n");
			break;
		}
		else if (state < RUN) {
			loginfo("You must be logged in and joined to a room to chat.\n");
			break;
		}
		if (strlen(arg) > 80) {
			printf(" Text chat too long (80 Char max). Truncating your chat\n");
		}
		logdb ("Appending: <%s> to room: <%s>\n", arg, my_room);
		prepareAppendMsg(out_msg, my_room, User, arg, null_lts);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;


  /* ------------- CONNECT --------------------------------------*/
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
    
    clear_state();
    /*  Disconnect from current server, if already connected */
    if (my_server >= 0) {
      my_server = -1;
    // TODO:  Any other disconn logic goes here
    }
    my_server = atoi(arg[0]) - 1;

    
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
		
		state = CONN;
		// ATTACH FD HERE
		break;

  /* ------------- HISTORY --------------------------------------*/
	case 'h':
		if (state < RUN) {
			loginfo("You must be logged in and joined to a room to request a chat history.\n");
			break;
		}
		logdb ("Sending history request for <%s>\n", my_room);
		prepareHistoryMsg(out_msg, my_room, User);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		
		break;

  /* ------------- LIKE --------------------------------------*/
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
		prepareLikeMsg(out_msg, User, ref, ADD_LIKE, null_lts);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;

  /* ------------- REMOVE LIKE --------------------------------------*/
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
		prepareLikeMsg(out_msg, User, ref, REM_LIKE, null_lts);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;


  /* ------------- JOIN --------------------------------------*/
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

    clear_state();

		logdb ("JOIN ROOM: <%s>\n", my_room);
		strcpy(my_room, arg);
    
    

		// SEND JOIN COMMAND TO SERVER 
		prepareJoinMsg(out_msg, my_room, User, null_lts);
		logdb("Message contents: <%s>\n", out_msg);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));

    /*  Clients JOIN 2 groups for a room:
     *    1. Spread Distro group for 'my_server' (to receive updates)
     *    2. Spread Membership group for attendees  */
    my_room_distrolist[0] = User[0];
 		strcpy(&my_room_distrolist[1], my_room);
    join_group(mbox, my_room_distrolist);
    join_group(mbox, my_room);

    
		state = RUN;
		
		break;

  /* ------------- QUIT --------------------------------------*/
	case 'q':
		logdb("QUITTING\n");
		disconn_spread(mbox);
		break;
		
  /* ------------- USERNAME --------------------------------------*/
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
		
  /* ------------- VIEW SERVERS --------------------------------------*/
	case 'v':
		if (state < CONN) {
			loginfo("You must be logged in and connected to a server to view connected servers.\n");
			break;
		}
		logdb("Sending view server request to server, %s\n", server_inbox)
		
		out_msg->tag = VIEW_MSG;
		send_message (mbox, server_inbox, out_msg, sizeof(Message));
		break;

  /* ------------- PRINT MENU --------------------------------------*/
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


void clear_state ()  {
      /*  Cleare current state if already in room */
    if (my_room[0] != '\0') {
      leave_group(mbox, my_room);
      leave_group(mbox, my_room_distrolist);
      my_room[0] = '\0';
      my_room_distrolist[0] = '\0';
      // TODO:  Clear the Chat_Msg_List (need either a chat_ll_clear() 
      //  function or we need to free the mem)
      // TODO:  CLEAR Attendee list HERE
      
    }
    chat_room = chat_ll_create();
    // attendee_list = ?????

}

void Initialize() {
	state = INIT;
	out_msg = malloc(MAX_MESSAGE_SIZE);
	null_lts.pid = 0;
	null_lts.ts = 0;
  my_server = -1;
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
