#include "config.h"
#include "transaction.h"
#include "message.h"
#include "commo.c"
#include "update_ll.h"

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

/* Message handling vars */
static	char			User[80];
static  char    		Private_group[MAX_GROUP_NAME];
static  mailbox 		mbox;
static  Message			mess;
static	char * 			HASHTAG = "#";
static	Message			update_message;

/* Protocol vars  */
static 	unsigned int		me;
static  unsigned int		my_server_id;
static  int		        	connected_svr[5];
static  char			server_name[5][MAX_GROUP_NAME];
static	char			server_group[MAX_GROUP_NAME];
static	char			client_group[MAX_GROUP_NAME];
static  char            connected_clients[20][MAX_GROUP_NAME];
static  int             num_connected_clients = 0;

static	int				my_state;

static	update_ll		updates;
static	unsigned int		lts;
static	update			*out_update;

int is_client_in_list(char *cli, int list_len, char cli_list[5][MAX_GROUP_NAME]) {
  int i;
  for(i=0; i<list_len; i++) {
    if(strcmp(cli, cli_list[i]) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}


void build_roomEntry (char * r);
void build_chatEntry (char * u, char * r, char * t);
void build_likeEntry (char * u, lts_entry e, char a);

//static 	FILE            	*sink;
//static 	char            	dest_file[10];


/*
typedef struct server_state {
	linkedList<connected_clients>	clients;
	linkedList<room_state>			rooms;
	linkedList<updates>				updates;   --sorted
	int								connected_servers[5];
} server_state;
*/

//TODO:  Make LinkedList<Client_info>
//static linkedList_client 	clients;


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
  /* Check for removals */
  for (i=0; i<num_connected_clients; i++) {
    if (!is_client_in_list(connected_clients[i], num_members, members)) {
      printf("Client %s has disconnected from the server! \n", connected_clients[i]);
      remove_client(i);
    }
  }

  /* Check for additions */
  for(i=0; i< num_members; i++) {
    if(!is_client_in_list(members[i], num_connected_clients, connected_clients)) {
      printf("Client %s has connected to the server! \n", members[i]);
      add_client(members[i]);
    }
  }

  print_connected_clients();
}

void handle_client_command(char client[MAX_GROUP_NAME]) {
	Message 		*out_msg;
	JoinMessage 	*jm;
	AppendMessage 	*am;
	HistoryMessage 	*hm;
	LikeMessage 	*lm;
	
	logdb("Client Request:  %c\n", mess.tag);
	
	switch (mess.tag) {
		case VIEW_MSG :
			prepareViewMsg(out_msg, connected_svr);
//			send_message(mbox, client, out_msg);
			logdb("VIEW Request. Sending list of servers to client <%s>\n", client);
			break;
			
		case JOIN_MSG :
			jm = (JoinMessage *) mess.payload;
			logdb("JOIN Request from user: <%s> on client: <%s>, for room <%s>\n", 
					jm->user, client, jm->room);
			build_roomEntry(jm->room);
			
			update_ll_insert_inorder_fromback(&updates, *out_update);
			update_ll_print(&updates);
			
			send_message(mbox, server_group, &update_message, sizeof(Message));
			
			break;

		case HISTORY_MSG :
			hm = (HistoryMessage *) mess.payload;
			logdb("HISTORY Request from user: <%s> on client: <%s>, for room <%s>\n", 
					hm->user, client, hm->room);
			break;

		case APPEND_MSG :
			am = (AppendMessage *) mess.payload;
			logdb("APPEND Request from user: <%s> on client: <%s>, \
				for room <%s>. Msg is '%s'\n", 
					am->user, client, am->room, am->text);
			break;

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
	update 		*in_update;
	
	in_update = (update *) &(mess.payload);
	
	/*  Do not process a duplicate Update entry */
	if (update_ll_get_inorder_fromback(&updates, in_update->lts)) {
		return;
	}

	logdb("Received Update from server\n");
	update_ll_insert_inorder_fromback(&updates, *in_update);
	update_ll_print(&updates);

}

void Initialize (char * server_index) {
  int 		i;
  char 		index;

  /* Parse server's index  */
  my_server_id = atoi(server_index);
  index = (char)(my_server_id + (int)'0');

  me = my_server_id - 1;

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
  int		num_target_groups;
  int		service_type;
  int16		mess_type;
  int		endian_mismatch;
  int		ret;

  char		*name;

  service_type = 0;

  ret = SP_receive(mbox, &service_type, sender, 100, &num_target_groups, target_groups,
  	           &mess_type, &endian_mismatch, sizeof(mess), (char *) &mess );
  if (ret < 0 )  {
    SP_error(ret);
    exit(0);
  }

  logdb("----------------------\n");
  logdb("Received from %s\n", sender);

  // WE CAN USE MULTIPLE HASHTAGS for a complete naming convention
  // e.g.:  #user#client#server#ugradxx
  name = strtok(sender, HASHTAG);

  /*  Processes Message */
  if (Is_regular_mess(service_type))	{
    logdb("Received a regular message from <%s>. Contents are: %s\n", sender, (char *) &mess);
    logdb("   The user name is <%s>\n", name);
    if (strcmp(target_groups[0], server_group) == 0) {
      // MAY WANT TO CHECK IF ITS OUR MESSAGE & DO SOMETHING DIFFERENT (OR IGNORE IT)
	  if (strcmp(name, server_name[me]) != 0) {
		handle_server_update();
	  }
    }
    else {
      handle_client_command(sender);
    }
  }
  /* Process Group membership messages */
  else if(Is_membership_mess(service_type)) {
    /* Re-interpret the fields passed to SP receive */
    char*  changed_group = sender;
    int    num_members   = num_target_groups;
    char*  members       = target_groups;

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

/*  OTHER HELPER function (for timing) */
float time_diff (struct timeval s, struct timeval e)  {
  float usec;
  usec = (e.tv_usec > s.tv_usec) ?
         (float)(e.tv_usec-s.tv_usec) :
         (float)(s.tv_usec-e.tv_usec)*-1;

  return ((float)(e.tv_sec-s.tv_sec) + usec/1000000);
}
