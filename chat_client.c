/******************************************************************************
 * File:    chat_client.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:    5 December 2014
 *
 * Description:  Client application program for reliable asynchronous 
 *      distributed chat service over Spread
 *
 *****************************************************************************/
#include "message.h"
#include "commo.c"
#include "chat_ll.h"
#include "like_ll.h"
#include "client_ll.h"
#include "name_ll.h"

#define INIT 0
#define LOGG 1
#define CONN 2
#define RUN 3
#define USER_NAME_LIMIT 20
#define DEFAULT_LINE_DISPLAY 25

/*  Client Global State  */
static	int			  state;
static  chat_ll   chat_room;
static  name_ll   displayed_attendees;
static  int       connected_server[MAX_SERVERS];
static  int       my_server;    /* Indexed from 0 */

/*  Spread groups */
static	char		  my_server_group[MAX_GROUP_NAME]; /* For tracking server membership */
static	char		  my_server_inbox[MAX_GROUP_NAME]; /* For Sending messages */
static	char		  my_room[MAX_GROUP_NAME];         /* For tracking room attendee membership */
static  char      my_room_distrolist[MAX_GROUP_NAME]; /* For Receiving messages */

/* Message handling vars */
static	char		    User[USER_NAME_LIMIT];
static  char    	  Private_group[MAX_GROUP_NAME];
static  mailbox 	  mbox;
static  Message		  *out_msg;
static  Message     in_msg;
static	lts_entry		null_lts;

/* User Interface vars */
static 	char		  last_command;
static  char      last_message[80];
static  int       dmesg = -1;
static  int       chat_lines_to_display = DEFAULT_LINE_DISPLAY;
static  sp_time   display_delay;
static  sp_time   idle;

/* Forward Declare functions */
void Initialize();
void Read_message();
void User_command();
void Display(int code);
void Print_menu(); 
void process_server_message();
void process_client_change(int num_members, char members[MAX_CLIENTS][MAX_GROUP_NAME]);
void login_server (int svr);
void clear_room ();

/*------------------------------------------------------------------------------
 *   Main - Client Appliation MAIN Process execution 
 *----------------------------------------------------------------------------*/
int main (int argc, char *argv[])  {
  
	/*  Handle Command line arguments */
	if (argc > 2)  {
		  printf("Usage: chat_client   {no args}\n");
		  exit(0);
	} 
	
	Initialize();
	
  Display(SHOW_UI);
  while (state < CONN) {
    User_command();
  }
	
  E_init();
	E_attach_fd(0, READ_FD, User_command, 0, NULL, HIGH_PRIORITY );
  E_attach_fd(mbox, READ_FD, Read_message, 0, NULL, LOW_PRIORITY );
	E_handle_events();

	return(0);
}


/*------------------------------------------------------------------------------
 *   Display - Displays chat session & output to user
 *----------------------------------------------------------------------------*/
void Display(int code) {
//  sp_time   now;
//  now = E_get_time();
//  E_sub_time(now, display_delay);
  
  if (code == SHOW_HELP) {
    Print_menu();
  }
  else {
    system("clear");
    loginfo("\n=================================================================================================\n");
    if (state == RUN) {
      loginfo("ROOM:  %s\n", my_room);
      loginfo("Attendees");
      name_ll_print(&displayed_attendees);
      chat_ll_print_num(&chat_room, chat_lines_to_display);
      chat_lines_to_display = DEFAULT_LINE_DISPLAY;
      loginfo("\n-------------------------------------------------------------------------------------------------\n");
    }
    if (dmesg >= 0) {
        loginfo("  %s\n", last_message);
    }
    else if (state < LOGG) {
      loginfo(" Please select a user name\n");
    }
    else if (state < CONN) {
      loginfo("  Please connect to a server\n");
    }
    else if (state < RUN) {
      loginfo("  Please join a chat room\n");
    }
    else {
      loginfo("  Connected to SERVER #%d\n", my_server+1);
    }
    loginfo("-------------------------------------------------------------------------------------------------\n");
  }
  loginfo("\n%s> ", &User[3]);
  dmesg = -1;
  fflush(stdout);
  idle = E_get_time();

}

/*------------------------------------------------------------------------------
 *   process_server_message - Handling function invoked when receiving a server
 *                            message
 *----------------------------------------------------------------------------*/
void process_server_message() {
  AppendMessage 	*am;
	LikeMessage 		*lm;
  ViewMessage     *vm;
  like_entry      *le;
  chat_info       *ch;
  int             i;
//  char            tmpmsg[80] = "";
	
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

      logdb("  New chat from server: LTS (%d,%d), User '%s' said \"%s\"\n", 
        ch->lts.ts, ch->lts.pid, ch->chat.user, ch->chat.text);
      
      /* Append to global chat list  */
      chat_ll_insert_inorder(&chat_room, *ch);
      
      break;  
      
   /* ---------  LIKE MESSAGE -- FROM: SVR  ------------*/
    case LIKE_MSG:
      
      lm = (LikeMessage *) in_msg.payload;

      /*  Create a new like entry */
      le = malloc(sizeof(like_entry));
      strcpy(le->user, lm->user);
      strcpy(le->room, lm->room);
      le->lts.pid = lm->lts.pid;
      le->lts.ts = lm->lts.ts;
      le->action = lm->action;

      logdb("  New like from server: User '%s' requested '%c' on LTS (%d,%d), occurring at LTS (%d, %d)\n", le->user, le->action, lm->ref.ts, lm->ref.pid, le->lts.ts, le->lts.pid);
      
      /* Get the referenced chat from the room & update its like list */
      ch = chat_ll_get_inorder(&chat_room, lm->ref);
      like_ll_update_like(&(ch->likes), le->user, le->room, le->lts, le->action); 
    
      break;

   /* ---------  VIEW MESSAGE -- FROM: SVR  ------------*/
    case VIEW_MSG:
      vm = (ViewMessage *) in_msg.payload;
      
      /*  Reset list of connected servers */
      for (i=0; i<MAX_SERVERS; i++) {
        connected_server[i] = vm->connected_server[i];
      }
      break;
      
    default:
      logerr("ERROR! Received a non-update from server\n");
    }
}

/*------------------------------------------------------------------------------
 *   process_client_change - Handling function invoked when another client
 *              joins/enters a room
 *----------------------------------------------------------------------------*/
void process_client_change(int num_members, 
                           char members[MAX_CLIENTS][MAX_GROUP_NAME]) {
  name_ll_node *curr = displayed_attendees.first;
  int i, found_user;
  char *user;
  char client_name[MAX_GROUP_NAME];
  
  /* Remove attendees from global list based on recent membership msg */
  while (curr) {
    found_user = FALSE;
    for (i=0; i<num_members; i++) {
      strcpy(client_name, members[i]);
      user = strtok(client_name, HASHTAG);
      if (strcmp(curr->data, &user[3]) == 0) {
        found_user = TRUE;
        break;
      }
    }
    if (!found_user) {
      dmesg = sprintf(last_message, "User '%s' has LEFT room <%s>", curr->data, my_room);        
      name_ll_remove(&displayed_attendees, curr->data);
    }
    curr = curr->next;
  }

  /* Check for additions to global attendee list (for current room) */
  for(i=0; i< num_members; i++) {
    strcpy(client_name, members[i]);
    user = strtok(client_name, HASHTAG);
    if (!name_ll_search(&displayed_attendees, &user[3])) {
      name_ll_insert(&displayed_attendees, &user[3]);
      dmesg = sprintf(last_message, "User '%s' has JOINED room <%s>", &user[3], my_room);
    }
  }
}

/*------------------------------------------------------------------------------
 *   Read_message - EVENT HANDLER attached to read incoming spread messages
 *----------------------------------------------------------------------------*/
void Read_message() {
  char		sender[MAX_GROUP_NAME];
  char		target_groups[100][MAX_GROUP_NAME];
  int			num_target_groups;
  int			service_type;
  int16		mess_type;
  int			endian_mismatch;
  int			ret, i;

  char    *name;
  int     server_alive;

  service_type = 0;

  /* Receive message from Spread network */
  ret = SP_receive(mbox, &service_type, sender, 100, &num_target_groups, target_groups,
  	           &mess_type, &endian_mismatch, sizeof(Message), (char *) &in_msg );
  if (ret < 0 )  {
    if (ret == CONNECTION_CLOSED) {
      loginfo("Client is crashing due to spread daemon termination....TERMINATING NOW!\n");
      exit(0);
    }
    SP_error(ret);
    exit(0);
  }

  name = strtok(sender, HASHTAG);
  
  /*  Processes Data Message -- should only be from our server */
  if (Is_regular_mess(service_type))	{
    if (name[0] != 's') {
      logerr("ERROR! Bad Sender. Message received from %s, name: %s\n", sender, name);
      return;
    }
    process_server_message();
  }

  /* Process Group membership messages */
  else if(Is_membership_mess(service_type)) {
    
    if (Is_transition_mess( service_type ))  {
      logdb(" Ignoring TRANSITIONAL message on group: %s\n", sender);
      return;
    }

    if (strcmp(sender, my_server_group) == 0) {
      /* Always verify our server is still connected */
      server_alive = FALSE;
      for (i=0; i<num_target_groups; i++) {
        if (target_groups[i][1] == 's') {
          server_alive = TRUE;
          return;
        }
      }
      if (!server_alive) {
        /* Our server is disconnected */
        dmesg = sprintf(last_message, 
           "Server # %d is disconnected. Please connect to a different server", 
            my_server+1);

       /*  Depart room, if joined to a room */
        if (state == RUN) {
          clear_room();
        }

        /* Detach & disconnect from Spread since we have no server */
        E_detach_fd(mbox, READ_FD);
        leave_group(mbox, my_server_group);
        SP_disconnect(mbox);
        state = LOGG;

      }
      /* OTHERWISE: Ignore other servers joining/leaving  */

    }
    else if (strcmp(sender, my_room) == 0) {
      /* Some other client has joined/left our room  */
      process_client_change(num_target_groups, target_groups);
    }
  }

  /* Bad message */
  else {
    logerr("Received a BAD message\n");
  }

  Display(SHOW_UI);
  fflush(stdout);
}


/*------------------------------------------------------------------------------
 *   User_command - EVENT HANDLER attached to read STDIN from user
 *----------------------------------------------------------------------------*/
 void User_command()   {
	char	command[100];
	char 	arg[CHAT_LEN];
	int 	ret, i, like_num;
  char  tmpmsg[80] = "";
	lts_entry	ref;
  chat_info   *ch;
  
	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 100, stdin ) == NULL ) 
		disconn_spread(mbox);
	
	last_command = command[0];
  
  i = 0;
  while (i<100) {
    if (command[i] == '\n')
      break;
    i++;
  }
  if (i==100) {
    while (getchar() != '\n');
  }

	switch(last_command)  {
  
  /* ------------- USERNAME --------------------------------------*/
	case 'u':
		logdb("SETTING USERNAME\n");
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1) {
			dmesg = sprintf(last_message, "Proper usage is u <user_name>");
			break;
		}
		else if (ret > USER_NAME_LIMIT || strlen(command) > USER_NAME_LIMIT+2)  {
			dmesg = sprintf(last_message, "Please limit your name to %d charaters or less.", USER_NAME_LIMIT);
			break;
		}
    /*  Depart room, if joined to a room */
    if (state == RUN) {
      clear_room();
      state = CONN;
    }
    /*  Disconnect from current server, if already connected */
    if (state == CONN) {
      E_detach_fd(mbox, READ_FD);
      leave_group(mbox, my_server_group);
      SP_disconnect(mbox);
    }
    
    /* Set Null values for the client_id portion of the username */
		User[0] = '0';
		User[1] = '0';
		User[2] = '0';
		strcpy(&User[3], arg);
		logdb("Your username is now set to <%c%c%c%s>\n", User[0], User[1], User[2], &User[3]);
		dmesg = sprintf(last_message, "You username is now set to '%s'", &User[3]);
    
    if (state == CONN) {
      login_server(my_server + 1);  /* Log back into same server */
    }
    else {
      state = LOGG;
    }
		break;
  
  
  /* ------------- CONNECT --------------------------------------*/
	case 'c':
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1 || strlen(arg) > 1)  {
			dmesg = sprintf(last_message, "Proper usage is c <server #>");
			break;
		}
		else if (arg[0] < '1' || arg[0] > '5') {
			dmesg = sprintf(last_message, "Please enter a valid server # between 1-5");
			break;
		}
		else if (state < LOGG) {
			dmesg = sprintf(last_message, "You must select a username before connecting to a server");
			break;
		}
    else if (state >= CONN && arg[0] == (char)(my_server + (int)'1')) {
			dmesg = sprintf(last_message, "You are already logged onto Server #%s", arg);
			break;
    }
    
   /*  Depart room, if joined to a room */
    if (state == RUN) {
      clear_room();
      state = CONN;
    }
    /*  Disconnect from current server, if already connected */
    if (state == CONN) {
      E_detach_fd(mbox, READ_FD);
      leave_group(mbox, my_server_group);
      SP_disconnect(mbox);
      logdb("DISCONNECTING FROM SPREAD!!!\n");
      state = LOGG;
    }
    login_server(atoi(arg));
		break;
 
  
  
  /* ------------- JOIN --------------------------------------*/
	case 'j':
		ret = sscanf(&command[2], "%s", arg);
		if (state < CONN) {
			dmesg = sprintf(last_message, "You must be logged in and connected to a server to join a room.");
			break;
		}
    else if( ret < 1 )  {
			dmesg = sprintf(last_message, "Proper usage is j <room_name> ");
			break;
		}
		else if (ret > MAX_GROUP_NAME  || strlen(command) > MAX_GROUP_NAME+2) {
			dmesg = sprintf(last_message, "Please use a name less than %d characters long.", MAX_GROUP_NAME-12);
      break;
		}

   /*  Depart room, if joined to a room */
    if (state == RUN) {
      clear_room();
    }

		logdb ("JOIN ROOM: <%s>\n", my_room);
		strcpy(my_room, arg);

		/* Send Join message to server */
		prepareJoinMsg(out_msg, my_room);
		send_message(mbox, my_server_inbox, out_msg);

    /*  Clients JOIN 2 groups for a room:
     *    1. Spread Distro group for 'my_server' (to receive updates)
     *    2. Spread Membership group for attendees  */
    my_room_distrolist[0] = User[0];
 		strcpy(&my_room_distrolist[1], my_room);
    join_group(mbox, my_room_distrolist);
    join_group(mbox, my_room);
    
		state = RUN;
		break;
  
  
  /* ------------- APPEND --------------------------------------*/
	case 'a':
		ret = sscanf(&command[2], "%[^\n]s", arg);
		if( ret < 1 )  {
			dmesg = sprintf(last_message, "Proper usage is a <chat_text>");
			break;
		}
		else if (state < RUN) {
			dmesg = sprintf(last_message, "You must be logged in and joined to a room to chat.");
			break;
		}
		if (strlen(arg) > CHAT_LEN || strlen(command) > CHAT_LEN+2) {
			dmesg = sprintf(last_message, " Text chat too long (80 Char max). REJECTING your chat");
      break;
		}
    logdb ("Appending: <%s> to room: <%s>\n", arg, my_room);
		prepareAppendMsg(out_msg, my_room, User, arg, null_lts);
		send_message(mbox, my_server_inbox, out_msg);
		break;


  /* ------------- LIKE --------------------------------------*/
	case 'l':
		if (state < RUN) {
			dmesg = sprintf(last_message, "You must be logged in and joined to a room to like a chat.");
			break;
		}
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1)  {
			dmesg = sprintf(last_message, "Proper usage is l <chat #>");
			break;
		}
		// OTHER LIKE ERROR CHECKING GOES HERE
    like_num = atoi(arg);
		
	  ref = chat_ll_get_lts(&chat_room, like_num);
		logdb ("Trying to Like Chat # <%d>, LTS: (%d,%d)\n", like_num,ref.ts, ref.pid);
    
    if (lts_eq(ref, null_lts)) {
      dmesg = sprintf(last_message, "You cannot like a chat that does not exist.");
      break;
    }

    ch = chat_ll_get(&chat_room, ref);
    
    logdb("User %s is trying to like a message from user %s\n",&User[3],  &(ch->chat.user[3]));
    if (strcmp(&(ch->chat.user[3]), &User[3]) == 0) {
      dmesg = sprintf(last_message, "You cannot like your own chat.");
      break;
    }
    if (does_like(&(ch->likes), &User[3])) {
      dmesg = sprintf(last_message, "You cannot like a chat you already like.");
      break;
    }
    
		prepareLikeMsg(out_msg, &User[3], my_room, ref, ADD_LIKE, null_lts);
		send_message(mbox, my_server_inbox, out_msg);
		break;

  /* ------------- REMOVE LIKE --------------------------------------*/
	case 'r':
		if (state < RUN) {
			dmesg = sprintf(last_message, "You must be logged in and joined to a room to remove a like.");
			break;
		}
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1)  {
			dmesg = sprintf(last_message, "Proper usage is r <chat #> ");
			break;
		}
		// OTHER LIKE ERROR CHECKING GOES HERE
    like_num = atoi(arg);
    ref = chat_ll_get_lts(&chat_room, like_num);
		logdb ("Trying to Un-Like Chat # <%d>, LTS: (%d,%d)\n", like_num,ref.ts, ref.pid);
    
    if (lts_eq(ref, null_lts)) {
      dmesg = sprintf(last_message, "You cannot Un-like a chat that does not exist.");
      break;
    }

    ch = chat_ll_get(&chat_room, ref);
    if (!does_like(&(ch->likes), &User[3])) {
      dmesg = sprintf(last_message, "You cannot Un-like a chat you do not like.");
      break;
    }
    
		prepareLikeMsg(out_msg, &User[3], my_room, ref, REM_LIKE, null_lts);
		send_message(mbox, my_server_inbox, out_msg);
		break;


		
  /* ------------- VIEW SERVERS --------------------------------------*/
	case 'v':
		if (state < CONN) {
			dmesg = sprintf(last_message, "You must be logged in and connected to a server to view connected servers.");
			break;
		}

    strcpy(last_message, "CONNECTED servers:  [");
    for (i=0; i<MAX_SERVERS; i++) {
      if (connected_server[i]) {
        dmesg = sprintf(tmpmsg, " <SVR %d>", i+1);
        strcat(last_message, tmpmsg);
      }
    }
    strcat(last_message, " ]");
		break;


  /* ------------- HISTORY --------------------------------------*/
	case 'h':
		if (state < RUN) {
			dmesg = sprintf(last_message, "You must be logged in and joined to a room to request a chat history");
			break;
		}
    chat_lines_to_display = -1;
		break;
  

  /* ------------- QUIT --------------------------------------*/
	case 'q':
		logdb("QUITTING\n");
		disconn_spread(mbox);
		break;


  /* ------------- PRINT MENU --------------------------------------*/
	case '?':
    Display(SHOW_HELP);
		return;
	
	default:
    loginfo("Invalid command. Printing help menu.\n");
//		dmesg = sprintf(last_message, "Invalid command. Printing help menu.");
    Display(SHOW_HELP);
		return;
	}
  Display(SHOW_UI);
  fflush(stdout);
}

/*------------------------------------------------------------------------------
 *   login_server - client's wrapper function to connect to spread network
 *----------------------------------------------------------------------------*/
void login_server (int svr)  {
  int ret, i;
  char 	client_id[3];
  char  server_id;
  
  logdb ("CONNECT to Server <%d>\n", svr);
  my_server = svr - 1;
  server_id = (char)(my_server + (int)'1');
  my_server_group[8] = server_id;
  User[0]         = server_id;

  i = 1;
  /* Connect to Spread */
  do {
    /* User is allowed up to 100 concurrent sessions on a single daemon */
    if (i == 100)  {
      SP_error(ret);
      logerr("User is only allowed 100 clients per server!\n");
      disconn_spread(mbox);
    }
    sprintf(client_id, "%02d", i++);
    memcpy(&User[1], client_id, 2);
    E_attach_fd(mbox, READ_FD, Read_message, 0, NULL, LOW_PRIORITY );
    ret = connect_spread(&mbox, User, Private_group);
    logdb("CONNECTED TO SPREAD\n");
  } while (ret != ACCEPT_SESSION);

  join_group(mbox, my_server_group);
  my_server_inbox[7] = server_id;
  connected_server[my_server] = TRUE;

  logdb("Actual username is <%c%c%c%s>\n", User[0], User[1], User[2], &User[3]);
  logdb("My Server group is %s\n", my_server_group);
  logdb("My Server inbox is %s\n", my_server_inbox);
  dmesg = sprintf(last_message, "You are now connected to Server #%c", server_id);
  
  state = CONN;
}

/*------------------------------------------------------------------------------
 *   clear_room - function to clear global state when leaving a room
 *----------------------------------------------------------------------------*/
void clear_room ()  {
      /*  Cleare current state if already in room */
    leave_group(mbox, my_room);
    leave_group(mbox, my_room_distrolist);
    my_room[0] = '\0';
    my_room_distrolist[0] = '\0';
    chat_room = chat_ll_create();
    displayed_attendees = name_ll_create();
  state = CONN;
}

/*------------------------------------------------------------------------------
 *   Initialize - function called upon startup
 *----------------------------------------------------------------------------*/
void Initialize() {
	state = INIT;
	out_msg = malloc(MAX_MESSAGE_SIZE);
	null_lts.pid = 0;
	null_lts.ts = 0;
  my_server = -1;
  display_delay.sec = 1;
  display_delay.usec = 0;
  strcpy(my_server_group, SERVER_GROUP_PREFIX);
  strcpy(my_server_inbox, SERVER_NAME_PREFIX);
  dmesg = sprintf(last_message, 
    "Welcome to distributed chat. Press '?' for help at any time");        

}

/*------------------------------------------------------------------------------
 *   Print_menu - Display help message to user
 *----------------------------------------------------------------------------*/
void Print_menu()  {
	loginfo("\nClient Help Menu \n");
	loginfo("\t%-20s%s\n", "u <username>", "Set user name");
	loginfo("\t%-20s%s\n", "c <server #>", "Connect to a server, valid #'s are 1-5");
	loginfo("\t%-20s%s\n", "v", "View connected servers");
	loginfo("\t%-20s%s\n", "j <room>", "Join a chat room");
	loginfo("\t%-20s%s\n", "a <chat_text>", "Append chat text to room");
	loginfo("\t%-20s%s\n", "h", "Get chat room History");
	loginfo("\t%-20s%s\n", "l <chat #>", "Like a chat text");
	loginfo("\t%-20s%s\n", "r <chat #>", "Remove a Like");
	loginfo("\t%-20s%s\n\n", "q", "Quit");
}
