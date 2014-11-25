#include "config.h"
#include "transaction.h"
#include "message.h"
#include "commo.c"
#include "update_ll.h"
#include "like_ll.h"
#include "room_ll.h"

#define ALL_SERVER_GROUP "server-all"

#define INIT 0
#define READY 1
#define RUN 2
#define RECONCILE 3

/* Forward Declared functions */
static	void		Run();
static	void		Reconcile();
void 			Initialize (char * server_index);
float 			time_diff (struct timeval s, struct timeval e);
void 		build_roomEntry (char * r);
void 		build_chatEntry (char * u, char * r, char * t);
void 		build_likeEntry (char * u, lts_entry e, char a);
int 		is_client_in_list(char *cli, int list_len, char cli_list[5][MAX_GROUP_NAME]);

/* Message handling vars */
static	char			User[80];
static  char    		Private_group[MAX_GROUP_NAME];
static  mailbox 		mbox;
static  Message			mess;
static	Message			update_message;

/* Protocol vars  */
static 	unsigned int		me;
static  char    		my_server_id;
static  char				server_name[5][MAX_GROUP_NAME];
static	char				server_group[MAX_GROUP_NAME];
static	char				client_group[MAX_GROUP_NAME];


/*  Server Chat State  */
static	int					my_state;
static  int		        	connected_svr[5];
static	unsigned int		lts;
static  room_ll				rooms;
static	update_ll			updates;

static  client_ll               connected_clients;
static  int             	num_connected_clients = 0;

static	update				*out_update;

//static 	FILE            	*sink;
//static 	char            	dest_file[10];


int main (int argc, char *argv[])  {

  /*  Handle Command line arguments */
  if (argc != 2)  {
  	  printf("Usage: spread_tester  <server_index_#>\n");
  	  exit(0);
  }

  logdb("[TRANSITION] Entering INIT state\n");
  my_state = INIT;
  Initialize(argv[1]);

  /* Connect to Spread */
  connect_spread(&mbox, User, Private_group);
  E_init();

  /* Initiate Membership join  */
  logdb("[TRANSITION] Entering READY state\n");
  my_state = READY;
  join_group(mbox, server_group);
  join_group(mbox, client_group);
  join_group(mbox, server_name[me]);

  logdb("Attaching reader event handler\n");
  logdb("[TRANSITION] Entering RUN state\n");
  my_state = RUN;
  E_attach_fd(mbox, READ_FD, Run, 0, NULL, HIGH_PRIORITY );
  E_handle_events();

  disconn_spread(mbox);

  return(0);
}

#include "utils.h"

void handle_server_change(int num_members, char members[5][MAX_GROUP_NAME]) {
  Message out_msg;
  /* Re-compute the connected server list */
  int new_connected_svr[5];
  int i;
  for (i=0; i<5; i++) {
    new_connected_svr[i] = FALSE;
  }

  /* Detect current members */
  for (i=0; i<num_members; i++) {
    int server_num = get_server_num(members[i]);
    new_connected_svr[server_num] = TRUE;
  }

  /* Compare to previous members */
  for (i=0; i<5; i++) {
    int diff = new_connected_svr[i] - connected_svr[i];
    if (diff != 0) {
      int joined = FALSE;
      if (diff == 1) {
        joined = TRUE;
      }
      char* status_change_msg = (joined) ? "JOINED" : "LEFT";
      loginfo("  Server %d has %s the server-group \n", i+1, status_change_msg);

      /* Update */
      connected_svr[i] = new_connected_svr[i];
    }
  }

  print_connected_servers();

  /*  Send updated list of connected servers to all clients  */
  prepareViewMsg(&out_msg, connected_svr);
  send_message(mbox, client_group, &out_msg, sizeof(Message));
  logdb("Sending updated list of servers to all clients\n");


  /* TODO revisit this:
  if (joined)  {
    if (num_connected_servers() > 1) {
      my_state = RECONCILE;
      Reconcile();
    }
  }
  else {

     foreach room in state.rooms:
    	foreach a in attendees:
    		if a.server == departed_server:
    			a.remove()

  }
  */
}


void handle_client_change(int num_members, char members[5][MAX_GROUP_NAME]) {
  int i;
  client_info new_client;
  /* Check for removals */
  client_ll_node *curr = connected_clients.first;
  while (curr) {
    if (!is_client_in_list(curr->data.name, num_members, members)) {
      printf("Client %s has disconnected from the server! \n", curr->data.name);
      remove_client(curr->data.name);
    }
    curr = curr->next;
  }

  /* Check for additions */
  for(i=0; i< num_members; i++) {
    if(!client_ll_get(&connected_clients, members[i])) {
      printf("Client %s has connected to the server! \n", members[i]);
      add_client(members[i]);
    }
  }

  print_connected_clients();
}

/*  Following "Apply" Functions are designed for applying updates to the 
 *  current global state  */

void apply_room_update (char * name)  {
			room_info		*new_room;
      char        distrolist[MAX_GROUP_NAME];
	
			/*  If this is a new room, create it & add to room list */
			if (room_ll_get(&rooms, name) == 0) {
				new_room = malloc (sizeof(room_info));
				strcpy(new_room->name, name);
				new_room->chats = chat_ll_create();
				room_ll_append(&rooms, *new_room);
				logdb("NEW ROOM created, <%s>\n", new_room->name);
        
    /*  Server JOINs 2 groups for a room:
     *    1. Spread Distro group for to send updates to clients in the room
     *    2. Spread Membership group for attendees  */		
        distrolist[0] = my_server_id;
        strcpy(&distrolist[1], name);
        join_group(mbox, distrolist);
        join_group(mbox, name);
      }
			else {
				logdb("Room '%s' already exists", new_room->name);
			}
}


void apply_chat_update (room_info *rm, chat_entry * ce, lts_entry * ts) {
	chat_info		new_chat;
	chat_ll			*chat_list;
	
	chat_list = &(rm->chats);
	
	strcpy(new_chat.chat.user, ce->user);
	strcpy(new_chat.chat.text, ce->text);
	strcpy(new_chat.chat.room, ce->room);
	new_chat.lts.pid = ts->pid;
	new_chat.lts.ts = ts->ts;
	new_chat.likes = like_ll_create();
	
	logdb("New Chat created for room <%s>: (%d,%d), User <%s> said '%s'\n", 
		new_chat.chat.room, new_chat.lts.ts, new_chat.lts.pid, new_chat.chat.user, new_chat.chat.text);

	chat_ll_insert_inorder_fromback(chat_list, new_chat);

}

void apply_update (update * u) {
	
	room_entry	*re;
	chat_entry  *ce;
	room_info		*rm;
	chat_ll			*chat_list;
	
  /* Update LTS if its higher than our current one  */
  if (u->lts.ts > lts)  {
    lts = u->lts.ts;
  }
  
	switch (u->tag)  {
		case ROOM: 
			re = (room_entry *) &(u->entry);
			apply_room_update (re->room);
			break;
			
		case CHAT:
			ce = (chat_entry *) &(u->entry);
			rm = room_ll_get(&rooms, ce->room);
			// TODO null check?
			chat_list = &(rm->chats);
			if (chat_ll_get_inorder_fromback(chat_list, u->lts) == 0) {
				apply_chat_update (rm, ce, &(u->lts));
			}
			logdb("--- Current state of room: %s -----------\n", rm->name);
			chat_ll_print(chat_list);
			logdb("-----------------------------------\n");
			break;
			
		case LIKE:
		
			break;
			
		default:
			logerr("ERROR! Received bad update from server group, tag was %c\n", u->tag);
	}
	
}

void handle_client_command(char client[MAX_GROUP_NAME]) {
	Message 				out_msg;
	JoinMessage 		*jm;
	AppendMessage 	*am;
	HistoryMessage 	*hm;
	LikeMessage 		*lm;
	
	// TODO:  TEMP CHAT LIST FOR TESTING
	chat_info		*new_chat;
	room_info		*new_room;

	
	logdb("Client Request:  %c\n", mess.tag);
	
	switch (mess.tag) {
    
 /*   -----------  VIEW REQUEST   -  from user ---------- */
		case VIEW_MSG :
			prepareViewMsg(&out_msg, connected_svr);
			send_message(mbox, client, &out_msg, sizeof(Message));
			logdb("VIEW Request. Sending list of servers to client <%s>\n", client);
			break;
			
  /*   -----------  JOIN REQUEST   -  from user ---------- */
		case JOIN_MSG :

			/* Create a new join message */
			jm = (JoinMessage *) mess.payload;
			logdb("JOIN Request from user: <%s> on client: <%s>, for room <%s>\n", 
					jm->user, client, jm->room);
			build_roomEntry(jm->room);
			
			/* Save to disk, store in memory, & apply to global state  */
			// fprintf(....)
			apply_room_update (jm->room);
			update_ll_insert_inorder_fromback(&updates, *out_update);
			
			/* Send update for a new room */
			prepareJoinMsg(&out_msg, jm->room, jm->user, out_update->lts);
			send_message(mbox, server_group, &out_msg, sizeof(Message));
			
			break;

/*   -----------  HISTORY REQUEST   -  from user ---------- */
		case HISTORY_MSG :
			hm = (HistoryMessage *) mess.payload;
			logdb("HISTORY Request from user: <%s> on client: <%s>, for room <%s>\n", 
					hm->user, client, hm->room);
			break;

/*   -----------  APPEND REQUEST   -  from user ---------- */
		case APPEND_MSG :

			/*  Create the new chat entry  */
			am = (AppendMessage *) mess.payload;
			logdb("APPEND Request from user: <%s> on client: <%s>, for room <%s>. Msg is '%s'\n", 
					am->user, client, am->room, am->text);
			build_chatEntry(am->user, am->room, am->text);

			/* Save to disk, store in memory, & apply to global state  */
			// TODO:  fprintf (....)
			update_ll_insert_inorder_fromback(&updates, *out_update);
			apply_update(out_update);
			
			/* Send update for a new chat messge */
			prepareAppendMsg(&out_msg, am->room, am->user, am->text, out_update->lts);
			send_message(mbox, server_group, &out_msg, sizeof(Message));
      
      // TODO:  ONLY SEND TO OTHER CLIENTS IN ROOM, am->room
      send_message(mbox, client_group, &out_msg, sizeof(Message));
      
			break;

/*   -----------  LIKE REQUEST   -  from user ---------- */
		case LIKE_MSG: 
			lm = (LikeMessage *) mess.payload;
			logdb("LIKE Request from user: <%s> on client: <%s>", lm->user, client);
			logdb("Action is '%c' for LTS: %d,%d\n", lm->action, lm->lts.ts, lm->lts.pid);
			

		default :
			break;
	}
	
	return;

}

void handle_server_update() {
	Message 				*out_msg;
	update 		      *new_update;
	JoinMessage 		*jm;
	AppendMessage 	*am;
	HistoryMessage 	*hm;
	LikeMessage 		*lm;
  chat_entry      *ce;
	
	logdb("Received '%c' Update from server\n", mess.tag);

  switch (mess.tag) {

    /* ---------  JOIN MESSAGE -- FROM: SVR  ------------*/
    case JOIN_MSG:
			jm = (JoinMessage *) mess.payload;

      /*  Do not process a duplicate Update entry */
      if (update_ll_get_inorder_fromback(&updates, jm->lts)) {
        return;
      }
      
      /*  Create a new data log entry */
      new_update = malloc (sizeof(update));
      new_update->tag = ROOM;
      new_update->lts.pid = jm->lts.pid;
      new_update->lts.ts = jm->lts.ts;
      strcpy(new_update->entry, jm->room);
      
      logdb("  New Room <%s> from server group\n", jm->room);
      
      /* Save to disk, store in memory, & apply to global state  */
      // fprintf (...)
      update_ll_insert_inorder_fromback(&updates, *new_update);
      apply_update(new_update);
      
      break;
      
    /* ---------  APPEND MESSAGE -- FROM: SVR  ------------*/
    case APPEND_MSG:
			am = (AppendMessage *) mess.payload;

      /*  Do not process a duplicate Update entry */
      if (update_ll_get_inorder_fromback(&updates, am->lts)) {
        return;
      }
      
      /*  Create a new data log entry */
      new_update = malloc (sizeof(update));
      new_update->tag = CHAT;
      new_update->lts.pid = am->lts.pid;
      new_update->lts.ts = am->lts.ts;
      
      ce = (chat_entry *) &(new_update->entry);
      strcpy(ce->user, am->user);
      strcpy(ce->room, am->room);
      strcpy(ce->text, am->text);

      logdb("  New chat on room <%s> from server-group, LTS (%d,%d)\n", ce->room, new_update->lts.ts, new_update->lts.pid);
      
      /* Save to disk, store in memory, & apply to global state  */
      // fprintf (...)
      update_ll_insert_inorder_fromback(&updates, *new_update);
      apply_update(new_update);
      
      /*  Create and send to clients */

  //  TODO:  SHOULD ONLY SEND TO CLIENTS IN THIS ROOM 
      // We will need a search_client_list(room_name) function
      out_msg = malloc (sizeof(Message));
			prepareAppendMsg(out_msg, am->room, am->user, am->text, new_update->lts);
      send_message(mbox, client_group, out_msg, sizeof(Message));
    
      break;  
      
   /* ---------  LIKE MESSAGE -- FROM: SVR  ------------*/
    case LIKE_MSG:
      break;
      
    default:
      logerr("ERROR! Received a non-update from server\n");
    }

	
}

void Initialize (char * server_index) {
  int 		i;
  char 		index;
  int    serverid;

  /* Parse server's index  -- Elaborate to reduce 0/1 indexing confusion */
  my_server_id = server_index;
  serverid = atoi(server_index);
  index = (char)(serverid + (int)'0');

  me = serverid - 1;

  /* Set group name for the all-server group */
  strcpy(server_group, SERVER_ALL_GROUP_NAME);

  /* Set servers spread user to server-i */
  strcpy(User, SERVER_NAME_PREFIX);
  User[7] = index;

  /* Set group name for connected clients */
  strcpy(client_group, SERVER_GROUP_PREFIX);
  client_group[8] = index;

  /* Record the name of each server's group */
  /* Register servers as not connected */
  for (i=0; i<5; i++) {
    strcpy(server_name[i], "server-");
    server_name[i][7] = (char)(1 + i + (int)'0');
    connected_svr[i] = FALSE;
  }
  
  /* Do other program initialization */
  updates = update_ll_create();
  connected_clients = client_ll_create();
  lts = 0;

  update_message.tag = UPDATE;
  out_update = (update *) &(update_message.payload);
  out_update->lts.pid = me;
}

static	void	Reconcile() {return;}


void build_roomEntry (char * r) {
	out_update->tag = ROOM;
	out_update->lts.ts = ++lts;
	strcpy(out_update->entry, r);
}


void build_chatEntry (char * u, char * r, char * t) {
	chat_entry *ce;
	out_update->tag = CHAT;
	out_update->lts.ts = ++lts;
	
	ce = (chat_entry *) &(out_update->entry);
	strcpy(ce->user, u);
	strcpy(ce->room, r);
	strcpy(ce->text, t);
}


void build_likeEntry (char * u, lts_entry e, char a) {
	like_entry *le;
	out_update->tag = LIKE;
	out_update->lts.ts = ++lts;
	
	le = (like_entry *) &(out_update->entry);
	strcpy(le->user, u);
	le->lts.ts = e.ts;
	le->lts.pid = e.pid;
	le->action = a;
}



static	void	Run()   {
  char		sender[MAX_GROUP_NAME];
  // TODO should this be more than 5? for connected clients in a group
  char		target_groups[5][MAX_GROUP_NAME];
  int			num_target_groups;
  int			service_type;
  int16		mess_type;
  int			endian_mismatch;
  int			ret;

  char		*name;

  char*  	changed_group;
  int    	num_members;
  char*  	members;

  service_type = 0;

  ret = SP_receive(mbox, &service_type, sender, 100, &num_target_groups, target_groups,
  	           &mess_type, &endian_mismatch, sizeof(mess), (char *) &mess );
  if (ret < 0 )  {
    SP_error(ret);
    exit(0);
  }

  logdb("----------------------\n");
//  name = strtok(sender, HASHTAG);
  logdb("Received from %s\n", sender);


  /*  Processes Message */
  if (Is_regular_mess(service_type))	{
//    logdb("   The user name is <%s>\n", name);
//    if (strcmp(target_groups[0], server_group) == 0) {
    if (sender[1] == 's') {
      name = strtok(sender, HASHTAG);
      // MAY WANT TO CHECK IF ITS OUR MESSAGE & DO SOMETHING DIFFERENT (OR IGNORE IT)
			if (strcmp(name, server_name[me]) != 0) {
        logdb("  Server Update message. Contents -->: %s\n", (char *) &mess);
				handle_server_update();
			}
    }
    else {
      logdb("  Regular Client Upate Message. Contents -->: %s\n", (char *) &mess);
      handle_client_command(sender);
    }
  }
  /* Process Group membership messages */
  else if(Is_membership_mess(service_type)) {
    name = strtok(sender, HASHTAG);
    /* Re-interpret the fields passed to SP receive */
    changed_group = sender;
    num_members   = num_target_groups;
    members       = target_groups;

    if (strcmp(changed_group, server_group) == 0) {
      handle_server_change(num_members, members);
    }
    else {
      handle_client_change(num_members, members);
    }
  }

  /* Bad message */
  else {
    logdb("Received a BAD message\n");
  }

}

int is_client_in_list(char *cli, int list_len, char cli_list[5][MAX_GROUP_NAME]) {
  int i;
  for(i=0; i<list_len; i++) {
    if(strcmp(cli, cli_list[i]) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}


/*  OTHER HELPER function (for timing) */
float time_diff (struct timeval s, struct timeval e)  {
  float usec;
  usec = (e.tv_usec > s.tv_usec) ?
         (float)(e.tv_usec-s.tv_usec) :
         (float)(s.tv_usec-e.tv_usec)*-1;

  return ((float)(e.tv_sec-s.tv_sec) + usec/1000000);
}
