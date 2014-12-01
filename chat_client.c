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
#define USER_NAME_LIMIT 10


//typedef struct client_state {
//	int					server;
//	room_state			room;
//	char				user[NAME_LEN];
//	char				spread_grp[NAME_LEN];
//} client_state;

static char       state_label[4][11]= 
      {"LOGGED OUT",
       "LOGGED IN",
       "CONNECTED",
       "CHATTING"};

/* Message handling vars */
static	char		    User[USER_NAME_LIMIT];
static  char    	  Private_group[MAX_GROUP_NAME];
static  mailbox 	  mbox;
static  Message		  *out_msg;
static  Message     in_msg;
static	lts_entry		null_lts;
static	char		    mess[MAX_MESSAGE_SIZE];
static  sp_time     display_delay;
static  sp_time     idle;


/*  Client Global State  */
static	int			  state;
static	char		  server_group[MAX_GROUP_NAME];
static	char		  my_room[MAX_GROUP_NAME];
static  char      my_room_distrolist[MAX_GROUP_NAME];
static	char		  server_inbox[MAX_GROUP_NAME];
static 	char		  last_command;
static  char      last_message[80];
static  chat_ll   chat_room;
static  client_ll attendees;
static  name_ll   displayed_attendees;
static  int       connected_server[MAX_SERVERS];
static  int       my_server;    /* Indexed from 0,  -1 means disconnected */
static  int       dmesg = -1;


void Initialize();
void Print_menu(); 

void display(int code) {
  sp_time   now;
  now = E_get_time();
  E_sub_time(now, display_delay);
  
  if (code == 1) {
    logdb("Waiting to display\n");
  }
  else {
//    system("clear");
    loginfo("------------------------------------------\n");
    if (state == RUN) {
        loginfo("ROOM:  %s\n", my_room);
        loginfo("Attendees");
        name_ll_print(&displayed_attendees);
        chat_ll_print_num(&chat_room, 25);
        loginfo("------------------------------------------\n");
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
      loginfo("------------------------------------------\n");
  }
  loginfo("\n%s> ", &User[3]);
  dmesg = -1;
  fflush(stdout);
}

void process_server_message() {
  Message 				*out_msg;
	AppendMessage 	*am;
	HistoryMessage 	*hm;
	LikeMessage 		*lm;
  ViewMessage     *vm;
  chat_entry      *ce;
  like_entry      *le;
  chat_info       *ch;
  like_ll         *ll;
  int             i;
  client_ll_node  *curr;
  char            client_name[MAX_GROUP_NAME];
  char            *user;
  char            tmpmsg[80] = "";

	
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
      
      /* Append to chat index list */

      break;  
      
   /* ---------  LIKE MESSAGE -- FROM: SVR  ------------*/
    case LIKE_MSG:
      
      lm = (LikeMessage *) in_msg.payload;

      /*  Create a new like entry */
      le = malloc(sizeof(like_entry));
      strcpy(le->user, lm->user);
      le->lts.pid = lm->lts.pid;
      le->lts.ts = lm->lts.ts;
      le->action = lm->action;

      logdb("  New like from server: User '%s' requested '%c' on LTS (%d,%d), occurring at LTS (%d, %d)\n", le->user, le->action, lm->ref.ts, lm->ref.pid, le->lts.ts, le->lts.pid);
      
      /* Get the referenced chat from the room */
      // TODO:  Error check when switching rooms & like come after for the prev. room
      ch = chat_ll_get_inorder(&chat_room, lm->ref);
      like_ll_update_like(&(ch->likes), le->user, le->lts, le->action); 
    
      break;

   /* ---------  VIEW MESSAGE -- FROM: SVR  ------------*/
    case VIEW_MSG:
      vm = (ViewMessage *) in_msg.payload;
      
      /*  Reset list of connected servers */
      for (i=0; i<MAX_SERVERS; i++) {
        connected_server[i] = vm->connected_server[i];
      }
      strcpy(last_message, "CONNECTED servers:  [ ");
      for (i=0; i<MAX_SERVERS; i++) {
        if (vm->connected_server[i]) {
          dmesg = sprintf(tmpmsg, " <SVR %d>", i+1);
          strcat(last_message, tmpmsg);
        }
      }
      strcat(last_message, "  ]\n");
      
      /* Update displayed attendee list for my_room based on connected svrs */
      curr = attendees.first;
      while (curr) {
        if (curr->data.name[1] == 's') {
          curr = curr->next;
          continue;
        }
        
        i = (int)(curr->data.name[1] - '1');
        strcpy(client_name, curr->data.name);
        user = strtok(client_name, HASHTAG);
        
        /* Add a new unique user name for display */
        if (connected_server[i] && !name_ll_search(&displayed_attendees, &user[3])) {
          name_ll_append(&displayed_attendees, &user[3]);
        }
        
        /* Remove user from display, if was previously dislayed */
        if (!connected_server[i] && name_ll_search(&displayed_attendees, &user[3])) {
          name_ll_remove(&displayed_attendees, &user[3]);
        }
        curr = curr->next;
      }
      break;
      
    default:
      logerr("ERROR! Received a non-update from server\n");
    }
}

void process_client_change(int num_members, 
                           char members[MAX_CLIENTS][MAX_GROUP_NAME]) {
  client_info   *new_client;
  client_ll_node *curr = attendees.first;
  int i;
  char *user;
  int client_server;
  char client_name[MAX_GROUP_NAME];
  
  /* Remove attendees from global list based on recent membership msg */
  while (curr) {
    if (!is_client_in_list(curr->data.name, num_members, members)) {
      client_ll_remove(&attendees, curr->data.name);    

      /*  Process unique usernames only */
      strcpy(client_name, curr->data.name);
      user = strtok(client_name, HASHTAG);
      
      /*  For display, only process clients 
       *        whose server is connected to our server */
      if (user[0] != 's') {
        client_server = (int)(user[0] - '1');

        if (connected_server[client_server] && 
            name_ll_search(&displayed_attendees, &user[3])) {
          name_ll_remove(&displayed_attendees, &user[3]);
          dmesg = sprintf(last_message, "Client '%s' has LEFT room <%s>\n", &user[3], my_room);
        }
      }
    }
    curr = curr->next;
  }

  /* Check for additions to global attendee list (for current room) */
  for(i=0; i< num_members; i++) {
    if(!client_ll_get(&attendees, members[i])) {
      new_client = malloc(sizeof(client_info));
      strcpy(new_client->name, members[i]);
      strcpy(new_client->user, members[i]);
      strcpy(new_client->room, my_room);      
      client_ll_append(&attendees, *new_client);

      /*  Process unique usernames only */
      strcpy(client_name, new_client->name);
      user = strtok(client_name, HASHTAG);

      /*  For display, only add unique clients 
       *        whose server is connected to our server */
      if (user[0] != 's') {
        client_server = (int)(user[0] - '1');
//        logdb("(c-) Checking %s from server %d\n", user, client_server);
        if (connected_server[client_server] && 
            !name_ll_search(&displayed_attendees, &user[3])) {
          name_ll_insert(&displayed_attendees, &user[3]);
          dmesg = sprintf(last_message, "Client '%s' has JOINED room <%s> \n", &user[3], my_room);
        }
      }
    }
  }  
  
  
}

void Read_message() {
  char		sender[MAX_GROUP_NAME];
  char		target_groups[100][MAX_GROUP_NAME];
  int			num_target_groups;
  int			service_type;
  int16		mess_type;
  int			endian_mismatch;
  int			ret, i;

  char*  	changed_group;
  int    	num_members;
  char*  	members;
  char    *name;
  int     server_alive;

  service_type = 0;

//  fflush(stdout);
//  logdb("---------   ");
//	display();

  ret = SP_receive(mbox, &service_type, sender, 100, &num_target_groups, target_groups,
  	           &mess_type, &endian_mismatch, sizeof(Message), (char *) &in_msg );
  if (ret < 0 )  {
    SP_error(ret);
    exit(0);
  }

  name = strtok(sender, HASHTAG);
  
  /*  Processes Message -- should only be from our server */
  if (Is_regular_mess(service_type))	{
    if (name[0] != 's') {
      logerr("ERROR! Bad Sender. Message received from %s, name: %s\n", sender, name);
      return;
    }
//    logdb("  Server message. Contents -->: %s\n", (char *) &in_msg);
    process_server_message();
  }

  /* Process Group membership messages */
  else if(Is_membership_mess(service_type)) {

    /* Re-interpret the fields passed to SP receive */
    changed_group = sender;
    num_members   = num_target_groups;
    members       = target_groups;
    logdb("Membership change for %s\n", sender);

    if (strcmp(changed_group, server_group) == 0) {
      /* Always verify our server is still connected */
      server_alive = FALSE;
      for (i=0; i<num_members; i++) {
        logdb("Check is %s is alive\n", target_groups[i]);
        if (target_groups[i][1] == 's') {
          server_alive = TRUE;
          break;
        }
      }
      if (!server_alive) {
        /* Our server is disconnected */
        dmesg = sprintf(last_message, "Server # %d is disconnected. Please connect to a different server\n.", my_server+1);
        if (state == RUN) {
          leave_group(mbox, my_room);
          leave_group(mbox, my_room_distrolist);
        }
        leave_group(mbox, server_group);
        state = LOGG;
      }
      // OTHERWISE: Ignore other servers joining/leaving

    }
    else {
      /* Client has joined/left our room  */
      process_client_change(num_members, members);
    }
  }

  /* Bad message */
  else {
    logerr("Received a BAD message\n");
  }
  display(0);
  fflush(stdout);
  idle = E_get_time();
}


void User_command()   {
	char	command[100];
	char	group[MAX_GROUP_NAME];
	char 	arg[CHAT_LEN];
	int 	ret, i, like_num;
	
	
	lts_entry	ref;
  chat_info   *ch;
  like_ll     *ll;

  
	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 100, stdin ) == NULL ) 
		disconn_spread(mbox);
	
	last_command = command[0];

	switch(last_command)  {

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
		if (strlen(arg) > 80) {
			dmesg = sprintf(last_message, " Text chat too long (80 Char max). Truncating your chat");
		}
		logdb ("Appending: <%s> to room: <%s>\n", arg, my_room);
		prepareAppendMsg(out_msg, my_room, User, arg, null_lts);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;


  /* ------------- CONNECT --------------------------------------*/
	case 'c':
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1 || strlen(arg) > 1)  {
			dmesg = sprintf(last_message, "Proper usage is c <server #> \n");
			break;
		}
		else if (arg[0] < '1' || arg[0] > '5') {
			dmesg = sprintf(last_message, "Please enter a valid server # between 1-5 \n");
			break;
		}
		else if (state < LOGG) {
			dmesg = sprintf(last_message, "You must select a username before connecting to a server\n");
			break;
		}
    
   /*  Depart room, if joined to a room */
    if (state == RUN) {
      clear_room();
      state = CONN;
    }
    /*  Disconnect from current server, if already connected */
    if (state == CONN) {
      leave_group(mbox, server_group);
      SP_disconnect(mbox);
      state = LOGG;
    }

    login_server(atoi(arg));

		break;

  /* ------------- HISTORY --------------------------------------*/
	case 'h':
		if (state < RUN) {
			dmesg = sprintf(last_message, "You must be logged in and joined to a room to request a chat history.\n");
			break;
		}
		logdb ("Sending history request for <%s>\n", my_room);
		prepareHistoryMsg(out_msg, my_room, User);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		
		break;

  /* ------------- LIKE --------------------------------------*/
	case 'l':
		if (state < RUN) {
			dmesg = sprintf(last_message, "You must be logged in and joined to a room to like a chat.\n");
			break;
		}
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1)  {
			dmesg = sprintf(last_message, "Proper usage is l <chat #> \n");
			break;
		}
		// OTHER LIKE ERROR CHECKING GOES HERE
    like_num = atoi(arg);
		
	  ref = chat_ll_get_lts(&chat_room, like_num);
		logdb ("Trying to Like Chat # <%d>, LTS: (%d,%d)\n", like_num,ref.ts, ref.pid);
    
    if (lts_eq(ref, null_lts)) {
      dmesg = sprintf(last_message, "You cannot like a chat that does not exist.\n");
      break;
    }

    ch = chat_ll_get(&chat_room, ref);
    logdb("Current Like list for this chat:\n");
    like_ll_print(&(ch->likes));
    if (does_like(&(ch->likes), &User[3])) {
      dmesg = sprintf(last_message, "You cannot like a chat you already like.\n");
      break;
    }
    
		prepareLikeMsg(out_msg, &User[3], ref, ADD_LIKE, null_lts);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;

  /* ------------- REMOVE LIKE --------------------------------------*/
	case 'r':
		if (state < RUN) {
			dmesg = sprintf(last_message, "You must be logged in and joined to a room to remove a like.\n");
			break;
		}
		ret = sscanf(&command[2], "%s", arg);
		if (ret < 1)  {
			dmesg = sprintf(last_message, "Proper usage is r <chat #> \n");
			break;
		}
		// OTHER LIKE ERROR CHECKING GOES HERE
    like_num = atoi(arg);
		
    ref = chat_ll_get_lts(&chat_room, like_num);

		logdb ("Trying to Un-Like Chat # <%d>, LTS: (%d,%d)\n", like_num,ref.ts, ref.pid);

    
    if (lts_eq(ref, null_lts)) {
      dmesg = sprintf(last_message, "You cannot Un-like a chat that does not exist.\n");
      break;
    }

    ch = chat_ll_get(&chat_room, ref);
    if (!does_like(&(ch->likes), &User[3])) {
      dmesg = sprintf(last_message, "You cannot Un-like a chat you do not like.\n");
      break;
    }

    
		prepareLikeMsg(out_msg, &User[3], ref, REM_LIKE, null_lts);
		send_message(mbox, server_inbox, out_msg, sizeof(Message));
		break;


  /* ------------- JOIN --------------------------------------*/
	case 'j':
		ret = sscanf(&command[2], "%s", arg);
		if (state < CONN) {
			dmesg = sprintf(last_message, "You must be logged in and connected to a server to join a room\n");
			break;
		}
    else if( ret < 1 )  {
			dmesg = sprintf(last_message, "Proper usage is j <room_name> \n");
			break;
		}
		else if (ret > MAX_GROUP_NAME) {
			dmesg = sprintf(last_message, "Please use a name less than %d characters long\n", MAX_GROUP_NAME-12);
      break;
		}

   /*  Depart room, if joined to a room */
    if (state == RUN) {
      clear_room();
    }

		logdb ("JOIN ROOM: <%s>\n", my_room);
		strcpy(my_room, arg);

		/* Send Join message to server */
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
			dmesg = sprintf(last_message, "Proper usage is u <user_name> \n");
			break;
		}
		else if (ret > USER_NAME_LIMIT)  {
			dmesg = sprintf(last_message, "Please limit your name to %d charaters or less\n", USER_NAME_LIMIT);
			break;
		}
    
    // TODO:  Check requirements, may need to logout/disconn completely 
    /*  Depart room, if joined to a room */
    if (state == RUN) {
      clear_room();
      state = CONN;
    }
    /*  Disconnect from current server, if already connected */
    if (state == CONN) {
      leave_group(mbox, server_group);
      SP_disconnect(mbox);
    }

    
		User[0] = '0';
		User[1] = '0';
		User[2] = '0';
		logdb("user is <%s>, arg is <%s>\n", User, arg);
		strcpy(&User[3], arg);
		loginfo("Your username is now set to <%c%c%c%s>\n", User[0], User[1], User[2], &User[3]);
		dmesg = sprintf(last_message, "Your username is now set to '%s'\n", &User[3]);
    
    if (state == CONN) {
      login_server(my_server + 1);  /* Log back into same server */
    }
    else {
      state = LOGG;
    }
		break;
		
  /* ------------- VIEW SERVERS --------------------------------------*/
	case 'v':
		if (state < CONN) {
			dmesg = sprintf(last_message, "You must be logged in and connected to a server to view connected servers.\n");
			break;
		}
		logdb("Sending view server request to server, %s\n", server_inbox);
		
		out_msg->tag = VIEW_MSG;
		send_message (mbox, server_inbox, out_msg, sizeof(Message));
		break;

  /* ------------- PRINT MENU --------------------------------------*/
	case '?':
		Print_menu();
		break;
	
	default:
		dmesg = sprintf(last_message, "Invalid command. Printing help menu.. \n");
		Print_menu();
	}
  display(0);
  fflush(stdout);
  idle = E_get_time();
}


int main (int argc, char *argv[])  {
  
	/*  Handle Command line arguments */
	if (argc > 2)  {
		  printf("Usage: chat_client   {no args}\n");
		  exit(0);
	} 
	
	Initialize();
	
	while (state < CONN) {
    display(0);
		User_command();
	}
	E_init();
	E_attach_fd(0, READ_FD, User_command, 0, NULL, HIGH_PRIORITY );
  E_attach_fd(mbox, READ_FD, Read_message, 0, NULL, LOW_PRIORITY );
	E_handle_events();

  logdb("EXITTED Spread Event Handling\n");
//	disconn_spread(mbox);
	
	return(0);
}


void login_server (int svr)  {
  int ret, i;
  char 	client_id[3];
  char  server_id;
  
  logdb ("CONNECT to Server <%d>\n", svr);
  my_server = svr - 1;
  strcpy(server_group, SERVER_GROUP_PREFIX);
  server_id = (char)(my_server + (int)'1');
  server_group[8] = server_id;
  User[0]         = server_id;

  i = 1;
  /* Connect to Spread */
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

  logdb("Actual username is <%c%c%c%s>\n", User[0], User[1], User[2], &User[3]);
  dmesg = sprintf(last_message, "Your username is now set to '%s'\n", &User[3]);

  join_group(mbox, server_group);
  strcpy(server_inbox, SERVER_NAME_PREFIX);
  server_inbox[7] = server_id;
  connected_server[my_server] = TRUE;
  
  state = CONN;
}


void clear_room ()  {
      /*  Cleare current state if already in room */
    leave_group(mbox, my_room);
    leave_group(mbox, my_room_distrolist);
    my_room[0] = '\0';
    my_room_distrolist[0] = '\0';
    // TODO:  Clear the Chat_Msg_List (need either a chat_ll_clear() 
    //  function or we need to free the mem)
    chat_room = chat_ll_create();
    attendees = client_ll_create();
    displayed_attendees = name_ll_create();
  state = CONN;
}

void Initialize() {
	state = INIT;
	out_msg = malloc(MAX_MESSAGE_SIZE);
	null_lts.pid = 0;
	null_lts.ts = 0;
  my_server = -1;
  display_delay.sec = 1;
  display_delay.usec = 0;

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
	loginfo("\t%-20s%s\n\n\n", "q", "Quit");

}
