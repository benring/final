/******************************************************************************
 * File:     chat_server.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Server application program for reliable asynchronous 
 *      distributed chat service over Spread
 *
 *****************************************************************************/
#include "config.h"
#include "transaction.h"
#include "message.h"
#include "commo.c"
#include "update_ll.h"
#include "like_ll.h"
#include "room_ll.h"
#include <limits.h>

#define INIT 0
#define READY 1
#define RUN 2
#define RECONCILE 3

#define DUPLICATE_UPDATE 1
#define UNHANDLED_UPDATE 2
#define PENDING_UPDATE 3
#define SUCCESSFUL_UPDATE 4
#define ALL_all_server_group "server-all"


/*===========================================================================
 *    Data Structures
 * ==========================================================================*/

/*  Main Server Chat State Data Structures */
static	int					    my_state;
static  int		        	connected_svr[MAX_SERVERS];
static	unsigned int		lts;
static  room_ll				  rooms;
static	update_ll			  updates;
static  update_ll       *pending_updates;
static  client_ll       connected_clients;
static  int             num_connected_clients = 0;


/* Message handling vars */
static	char			  User[80];
static  char    		Private_group[MAX_GROUP_NAME];
static  mailbox 		mbox;
static  Message			mess;

/* Spread & server name vars   */
static 	unsigned int		me;               /* Indexed from 0 */
static  char    		    my_server_id;     /* Displayed index name (1-5) */

/* Spread group to which ONLY this server joins so clients can send messages */
static  char				    my_inbox[MAX_GROUP_NAME];         

/* Spread group for all servers */
static	char				    all_server_group[MAX_GROUP_NAME];   

/* Spread group for all connected clients to track membership */
static	char				    my_client_group[MAX_GROUP_NAME];    

/* Other in/output vars */
static	update				       *out_update;
static  FILE                 *logfile;
static  char                 logfilename[NAME_LEN];

/* Reconciliation vars */
static  unsigned int         my_vector[MAX_SERVERS];
static  unsigned int         expected_vectors[MAX_SERVERS];
static  unsigned int         my_responsibility[MAX_SERVERS];
static  lts_entry            min_lts_vector[MAX_SERVERS];
static  lts_entry            max_lts_vector[MAX_SERVERS];

/*===========================================================================
 * Forward Declared functions 
 * ==========================================================================*/

/*  Main Program functions/handlers */
void		Read_message();
void    Except();
void 		Initialize (char * server_index);

/*  High level event handlers in response to client/server action */
void    handle_server_update();
void    handle_server_change(int num_members, 
                          char members[MAX_CLIENTS][MAX_GROUP_NAME]);
void    handle_client_command(char client[MAX_GROUP_NAME]);
void    handle_client_change(int num_members, 
                          char members[MAX_CLIENTS][MAX_GROUP_NAME]);

/*  Functions to create updates fom data */
void 		build_chatEntry (char * u, char * r, char * t);
void 		build_likeEntry (char * u, lts_entry e, char a);
int     log_update(update *u);

/*  Functions to build global state from updates*/
int     apply_update (update * u, int shouldLog, int send_clients);
int     apply_chat_update (chat_entry * ce, lts_entry * ts, int send_clients);
int     apply_like_update(lts_entry ref, lts_entry like_lts, 
                              char* user, char action, int send_clients);
int     incorporate_into_state(update *u, int send_clients);
void    try_pending_updates();

/*  Functions to manage reconciling & recovery for fault tolerance */
int     recover_from_disk(update_ll *list);
void    resend_update (update *u);
void    update_my_vector();
void    Initialize_Reconcile_Data();
void    Handle_lts_vector();
void    Determine_updates_to_send();
void    Send_updates();

/*  Other functions for server operations */
void    send_history_to_client(char *roomname, char *client);
void    create_room (char * name);


/* Helper functions  */
int       get_server_num (char * svr_name);
void      remove_client(char *name);
void      add_client(char *name);
void      print_connected_servers ();
void      print_connected_clients();
void      print_my_vector();



/*------------------------------------------------------------------------------
 *   Main - Server's Appliation MAIN Process execution 
 *----------------------------------------------------------------------------*/
int main (int argc, char *argv[])  {

  /*  Handle Command line arguments */
  if (argc != 2)  {
  	  printf("Usage: spread_tester  <server_index_#>\n");
  	  exit(0);
  }

  loginfo("[TRANSITION] Entering INIT state. Connecting and initializing Spread. \n");
  my_state = INIT;
  Initialize(argv[1]);

  /* Connect to Spread */
  connect_spread(&mbox, User, Private_group);

  /* Initiate Membership join  */
  loginfo("[TRANSITION] Entering READY state. Spread is connected. Recover from disk and join Spread groups. \n");
  my_state = READY;
  
  /* Read the log file */ 
  logfile = fopen(logfilename, "a+b");
  if (logfile) {
    fseek(logfile, 0, SEEK_SET);
    recover_from_disk(&updates);
  }
  else {
    logerr ("ERROR opening file %s.\n", logfilename);
    perror("ERROR opening log file");
  }

  /*  Join spread groups */
  join_group(mbox, all_server_group);
  join_group(mbox, my_client_group);
  join_group(mbox, my_inbox);

  loginfo("[TRANSITION] Entering RUN state. Attach handlers and start processing messages.\n");
  my_state = RUN;

  /* Initiate Spread system event handling loop */
  E_init();
  E_attach_fd(mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY );
  E_attach_fd(mbox, EXCEPT_FD, Except, 0, NULL, HIGH_PRIORITY );
  E_handle_events();

  fclose(logfile);
  disconn_spread(mbox);
  free(pending_updates);

  return(0);
}

/*------------------------------------------------------------------------------
 *   Event handler invoked for every received message. This function will 
 *      receive a message and dispath one the client or server "handler"
 *      function appropriately
 *----------------------------------------------------------------------------*/
void	Read_message()   {
  char		sender[MAX_GROUP_NAME];
  char		target_groups[MAX_CLIENTS][MAX_GROUP_NAME];
  int	        num_target_groups;
  int	        service_type;
  int16		mess_type;
  int		endian_mismatch;
  int		ret;
  char		*name;

  service_type = 0;

  ret = SP_receive(mbox, &service_type, sender, 100, &num_target_groups, target_groups,
  	           &mess_type, &endian_mismatch, sizeof(mess), (char *) &mess );
  if (ret < 0 )  {
    if (ret == CONNECTION_CLOSED) {
      loginfo("Server is crashing due to spread daemon termination....TERMINATING NOW!\n");
      exit(0);
    }
    SP_error(ret);
    exit(0);
  }

  /* Ignore all updates from ourself */ 
  logdb("--------------------------------------------------------\n");
  if (strcmp(sender, Private_group) == 0) {
    logdb("Received from me:  %s. IGNORING \n", Private_group);
    return;
  }
  else { 
    logdb("Received from %s\n", sender);
  }

  /*  Processes Message */
  if (Is_regular_mess(service_type))
  {
    if (sender[1] == 's') {
      name = strtok(sender, HASHTAG);
      if (strcmp(name, my_inbox) != 0) {
        logdb("  Server Update message. Contents -->: %s\n", (char *) &mess);
        if (mess.tag == LTS_VECTOR) {
          Handle_lts_vector();
        } 
        else {
          handle_server_update();
        }
      }
    } 
    else {
      logdb("  Regular Client Upate Message. Contents -->: %s\n", (char *) &mess);
      handle_client_command(sender);
    }
  }
  /* Process Group membership messages */
  else if(Is_membership_mess(service_type))
  {
  
    if (Is_transition_mess( service_type ))  {
      logdb(" Ignoring TRANSITIONAL message on group: %s\n", sender);
      return;
    }
    name = strtok(sender, HASHTAG);
  
    if (strcmp(sender, all_server_group) == 0) {
      handle_server_change(num_target_groups, target_groups);
    } else if (strcmp(sender, my_client_group) == 0) {
      handle_client_change(num_target_groups, target_groups);
    } else {
      logdb ("Group change on <%s>\n", sender);
    }
  }
  
  /* Bad message */
  else
  {
    logdb("Received a BAD message\n");
  }
  
  }

/*------------------------------------------------------------------------------
 *   Inialize -- conduct program intiation
 *----------------------------------------------------------------------------*/
void Initialize (char * server_index) {
  int 		i;
  char 		index;
  int    serverid;

  /* Parse server's index  -- Elaborate to reduce 0/1 indexing confusion */
  my_server_id = server_index[0];
  serverid = atoi(server_index);
  index = (char)(serverid + (int)'0');

  me = serverid - 1;

  /* Set group name for the all-server group */
  strcpy(all_server_group, SERVER_ALL_GROUP_NAME);

  /* Set servers spread user to server-i */
  strcpy(User, SERVER_NAME_PREFIX);
  User[7] = index;

  /* Set group name for connected clients */
  strcpy(my_client_group, SERVER_GROUP_PREFIX);
  my_client_group[8] = index;
  
  /* Set Server's Inbox to 'server-i' */
  // TODO: Remove user/inbox redundency
  strcpy(my_inbox, "server-");
  my_inbox[7] = index;

  /* Record the name of each server's group */
  /* Register servers as not connected */
  for (i=0; i<MAX_SERVERS; i++) {
    connected_svr[i] = FALSE;
    expected_vectors[i] = 0;
    min_lts_vector[i].pid = 10;
    max_lts_vector[i].pid = 10;
    // TODO use MAXINT
    min_lts_vector[i].ts = MAX_INT;
    max_lts_vector[i].ts = 0;
  }
  
  /* Do other program initialization */
  pending_updates = malloc(sizeof(update_ll));
  *pending_updates = update_ll_create();
  updates = update_ll_create();
  connected_clients = client_ll_create();
  lts = 0;

  out_update = malloc (sizeof(update));
  out_update->lts.pid = me;

  sprintf(logfilename, "log_%c.txt", my_server_id);
}


/*------------------------------------------------------------------------------
 *   Except -- event handler for respond to Spread exceptions
 *----------------------------------------------------------------------------*/
void Except()  {
  loginfo("EXECPTION received from Spread. Quitting.\n");
  disconn_spread(mbox);
}


/*------------------------------------------------------------------------------
 *   Chat Application event handling functions. These functions are invoked 
 *        in response to actions for chat operations, to include a client
 *        command, server's update message, or a membership change (server or
 *        client) 
 *----------------------------------------------------------------------------*/
void handle_client_command(char client[MAX_GROUP_NAME]) {
  Message        out_msg;
  JoinMessage 	 *jm;
  AppendMessage  *am;
  LikeMessage 	 *lm;
  
  logdb("Client Request:  %c\n", mess.tag);
	
  switch (mess.tag) {
    case VIEW_MSG :
      /* Send the connected_servers to the client */
      prepareViewMsg(&out_msg, connected_svr);
      send_message(mbox, client, &out_msg);
      loginfo("VIEW Request. Sending list of servers to client <%s>\n", client);
      break;
		
    case JOIN_MSG :
      /* Create and Apply an Update. Send it out to serevers. Then send history to client */
      jm = (JoinMessage *) mess.payload;
      loginfo("JOIN Request from user: client <%s> joins room <%s>\n", client, jm->room);
      create_room(jm->room);
      send_history_to_client(jm->room, client);
      break;

    case APPEND_MSG :
      /*  Create and Apply an Update. Send it out to Servers */
      am = (AppendMessage *) mess.payload;
      loginfo("APPEND Request from user: <%s> on client: <%s>, for room <%s>. Msg is '%s'\n", am->user, client, am->room, am->text);
      build_chatEntry(am->user, am->room, am->text);
      apply_update(out_update, TRUE, TRUE);
      prepareAppendMsg(&out_msg, am->room, am->user, am->text, out_update->lts);
      send_message(mbox, all_server_group, &out_msg);
      break;

    case LIKE_MSG: 
      /* Create and Apply an Update. Send it out to Servers */
      lm = (LikeMessage *) mess.payload;
      loginfo("LIKE Request from user: <%s> on client: <%s>. ", lm->user, client);
      loginfo("Action is '%c' for LTS: %d,%d\n", lm->action, lm->ref.ts, lm->ref.pid);
      
      build_likeEntry(lm->user, lm->ref, lm->action);
      apply_update(out_update, TRUE, TRUE);
      prepareLikeMsg(&out_msg, lm->user, lm->ref, lm->action, out_update->lts);
      send_message(mbox, all_server_group, &out_msg);
      break;

    default:
      logerr("ERROR unhandled command from client\n");
      exit(1);
  }
  return;
}

void handle_server_change(int num_members, char members[MAX_CLIENTS][MAX_GROUP_NAME]) {
  Message out_msg;

  /* Re-compute the connected server list */
  int new_connected_svr[MAX_SERVERS];
  int i;
  int joined;
  int new_members = FALSE;
  for (i=0; i<MAX_SERVERS; i++) {
    new_connected_svr[i] = FALSE;
  }

  /* Detect current members */
  for (i=0; i<num_members; i++) {
    int server_num = get_server_num(members[i]);
    new_connected_svr[server_num] = TRUE;
  }

  /* Compare to previous members */
  for (i=0; i<MAX_SERVERS; i++) {
    int diff = new_connected_svr[i] - connected_svr[i];
    if (diff != 0) {
      joined = FALSE;
      if (diff == 1) {
        joined = TRUE;
        new_members = TRUE;
      }
      char* status_change_msg = (joined) ? "JOINED" : "LEFT";
      logdb("  Server %d has %s the server-group \n", i+1, status_change_msg);

      /* Update */
      connected_svr[i] = new_connected_svr[i];
    }
  }

  loginfo("Server membership has changed. Current members:\n")
  print_connected_servers();

  /*  Send updated list of connected servers to all clients  */
  prepareViewMsg(&out_msg, connected_svr);
  send_message(mbox, my_client_group, &out_msg);
  logdb("Sending updated list of servers to all clients\n");

  // Determine if we need to reconcile. (New members have joined. And more than 1 server)
  int sum = 0;
  for (i = 0; i < MAX_SERVERS; i++) {
    if (connected_svr[i]) {
      sum++;
    }
    else {
      /* Always reset expected vectors to 0 for non-connected servers  */
      expected_vectors[i] = 0;
    }

  }
  if (new_members && sum > 1) {
    my_state = RECONCILE;
    loginfo("[TRANSITION] Entering RECONCILE state. Sending out my vector and waiting for others. \n");
    update_my_vector();

    Initialize_Reconcile_Data();

  }
}

void Initialize_Reconcile_Data () {
  int i;
  Message out_msg;
  lts_entry my_entry;
  LTSVectorMessage  *ltsm;
  
  /*  1a. Initialize Reconcile Data variables */
  for (i=0; i < MAX_SERVERS; i++) {
    max_lts_vector[i].pid = MAX_SERVERS+1;
    min_lts_vector[i].pid = MAX_SERVERS+1;
    max_lts_vector[i].ts = 0;
    min_lts_vector[i].ts = MAX_INT;
    my_responsibility[i] = 0;
    /* Expect an additional vector from each server */
    if (connected_svr[i]) {
      expected_vectors[i]++;
    }
  }

  logdb("New Expected Vector is: \n");
  for (i=0; i < MAX_SERVERS; i++) {
    logdb (" %d", expected_vectors[i]);
  }
  
  /* 1b. PREPARE: My_LTS_Vector */
  out_msg.tag = LTS_VECTOR;
  ltsm = (LTSVectorMessage *) &(out_msg.payload);
  ltsm->sender = me; 
  ltsm->flag = LTS_RECONCILE;
  for (i = 0; i < MAX_SERVERS; i++) {
    ltsm->lts[i] = my_vector[i]; 
  }

  /* 1c. Handle my own vector since it wont be received */
  expected_vectors[me]--;
  logdb("Decremented myself: \n");
  for (i=0; i < MAX_SERVERS; i++) {
    logdb (" %d", expected_vectors[i]);
  }
  
  /* 1d. Process our MY_LTS_Vector as a received vector */
  for (i=0; i < MAX_SERVERS; i++) { 
    my_entry.ts = my_vector[i];
    my_entry.pid = me;
    if (lts_lessthan(my_entry, min_lts_vector[i])) {
      min_lts_vector[i].ts = my_vector[i];
      min_lts_vector[i].pid = me;
    }

    if (max_lts_vector[i].ts == 0 || lts_greaterthan(my_entry, max_lts_vector[i])) {
      max_lts_vector[i].ts = my_vector[i];
      max_lts_vector[i].pid = me;
    }
  }
    
  /* 1e. SEND: MY_LTS_VECTOR  */
  send_message(mbox, all_server_group, &out_msg);
}

void Determine_updates_to_send() {
  lts_entry my_entry;
  int i;
  
  logdb("Received all vectors: Calculating updates to send: \n"); 
  my_entry.pid = me;
  
  /* Determine which updates this server is responsible for */ 
  for (i = 0; i < MAX_SERVERS; i++) {
    my_entry.ts = my_vector[i];
   
    /* Server is always responsible for its own updates */
    if (i == me && max_lts_vector[i].ts != 0) {
      my_responsibility[i] = TRUE;
    }

    /* If a server is not connected, we may be responsible for its updates */
    /* Provided that maximum LTS matches my_entry */
    /* The pid guarantees only 1 server will resend */
    if (max_lts_vector[i].ts != 0 && !connected_svr[i] && lts_eq(my_entry, max_lts_vector[i])) { 
      my_responsibility[i] = TRUE;
    }
  }

  loginfo("Received all Vectors: Min/Max:\n");
  /* Log the Min/Max receieved for each server */
  for (i = 0; i < MAX_SERVERS; i++) {
     loginfo ("Server %d. Min: %d. Max: %d\n", i, min_lts_vector[i].ts, max_lts_vector[i].ts);
  }

  /* Log updates that this server is responsible for */
  for (i = 0; i < MAX_SERVERS; i++) {
    if(my_responsibility[i]) {
      loginfo("I am responsible for Server %d's updates. %d to %d\n", i, min_lts_vector[i].ts, max_lts_vector[i].ts);
    }
  }
  
}

void Send_updates() {
  // TODO flow control?
  update_ll_node    *curr_update;

  int currpid;
  int currts;

  curr_update = updates.first;

  while(curr_update) {
    currpid = curr_update->data.lts.pid;
    currts = curr_update->data.lts.ts;
    
    if (my_responsibility[currpid]) {
      if(currts <= max_lts_vector[currpid].ts && currts >= min_lts_vector[currpid].ts) {
        logdb("Sending missing update (%d, %d)\n", curr_update->data.lts.ts, curr_update->data.lts.pid); 
        resend_update(&(curr_update->data));
      }
    }
    curr_update = curr_update->next;
  }
  

}

void Handle_lts_vector() {
  LTSVectorMessage  *ltsm;
  int               sum = 0;
  int               i;
 
  /* Only accept vectors in RECONCILE state */
  if (my_state != RECONCILE) {
    logerr("ERROR received vector when not in reconcile state\n");
  }
 
  /* 2a. RECV: LTS Vector & Interpret Contents of message */
  ltsm = (LTSVectorMessage *) mess.payload;
  logdb(" RECEIVED AN LTS VECTOR FROM %d\n", ltsm->sender);
 
  /* 2b. Decrement the expected_vector count for the sender */
  expected_vectors[ltsm->sender]--;
  logdb("New Expected Vector is: \n");
  for (i=0; i < MAX_SERVERS; i++) {
    logdb (" %d", expected_vectors[i]);
  }
  logdb("\n");

  /* 2c. Update the min and max lts_vectors if this message contains a new min or max 
         While simultaneously counting how many vectors are still expected */
  lts_entry new_entry;
  sum = 0;
  for(i=0; i < MAX_SERVERS; i++) {
    /* Count expected vectors, reset my_responsibility vector */ 
    sum += expected_vectors[i];
   
    /* Create a new lts with the sender as the pid */
    /* We store both (ts, pid) when tracking the maximum. */
    new_entry.ts = ltsm->lts[i];
    new_entry.pid = ltsm->sender;

    /* Update the min/max if necessary, based on LTS comparison */ 
    if (lts_lessthan(new_entry, min_lts_vector[i])) {
      min_lts_vector[i] = new_entry;
    }
    if (max_lts_vector[i].ts == 0 || lts_greaterthan(new_entry, max_lts_vector[i])) {
      max_lts_vector[i] = new_entry;
    }
  }

  /* If we have received all expected vectors, 
   *    figure out which updates to resend, then resend them */
  if (sum == 0) {
    
    /*  3. Determine if server needs to send updates */
    Determine_updates_to_send();
  
    /*  4. Attempt to send updates & return to RUN state */
    // DO FLOW CONTROL HERE
    Send_updates();
  
    /* Finished RECONCILE state */ 
    loginfo("[TRANSITION] Done with RECONCILE. Entering RUN state.\n");
    my_state = RUN;
  }
}

void handle_server_update() {
  update         new_update;
  AppendMessage  *am;
  LikeMessage 	 *lm;
  chat_entry     *ce;
  like_entry     *le;
  
  logdb("Received '%c' Update from server\n", mess.tag);

  switch (mess.tag) {
    case APPEND_MSG:
      /* Build and apply the update */
      am = (AppendMessage *) mess.payload;

      /*  Create a new data log entry */
      new_update.tag = CHAT;
      new_update.lts.pid = am->lts.pid;
      new_update.lts.ts = am->lts.ts;
      
      ce = (chat_entry *) &(new_update.entry);
      strcpy(ce->user, am->user);
      strcpy(ce->room, am->room);
      strcpy(ce->text, am->text);

      loginfo("  New chat on room <%s> from server-group, LTS (%d,%d)\n", ce->room, new_update.lts.ts, new_update.lts.pid);
      break;  
      
    case LIKE_MSG:
      lm = (LikeMessage *) mess.payload;
      
      /*  Create a new data log entry */
      new_update.tag = LIKE;
      new_update.lts.pid = lm->lts.pid;
      new_update.lts.ts = lm->lts.ts;

      le = (like_entry *) &(new_update.entry);
      strcpy(le->user, lm->user);
      le->action = lm->action;
      le->lts = lm->ref;

      loginfo("  New like from server-group: User '%s' requests '%c' on LTS (%d,%d)\n", le->user, le->action, le->lts.ts, le->lts.pid);
      break;

    default:
      logerr("ERROR! Received a non-update from server\n");
      exit(1);
  }

  apply_update(&new_update, TRUE, TRUE);
}

/* Update the list of connected clients */
void handle_client_change(int num_members, char members[MAX_CLIENTS][MAX_GROUP_NAME]) {
  int i;
  Message out_msg;
  /* Check for removals */
  client_ll_node *curr = connected_clients.first;
  while (curr) {
    if (!is_client_in_list(curr->data.name, num_members, members)) {
      //printf("Client %s has disconnected from the server! \n", curr->data.name);
      remove_client(curr->data.name);
    }
    curr = curr->next;
  }

  /* Check for additions */
  for(i=0; i< num_members; i++) {
    if(!client_ll_get(&connected_clients, members[i])) {
      //printf("Client %s has connected to the server! \n", members[i]);
      add_client(members[i]);
  
      // TODO can we send one multicast to the client group?
      prepareViewMsg(&out_msg, connected_svr);
      send_message(mbox, members[i], &out_msg);
    }
  }

  loginfo("Membership change in clients. New List: \n");
  print_connected_clients();
}


/*------------------------------------------------------------------------------
 *  The "Build" Functions are designed to create a log entry update from data.
 *----------------------------------------------------------------------------*/
void build_chatEntry (char * u, char * r, char * t) {
	chat_entry *ce;
	out_update->tag = CHAT;
	out_update->lts.ts = ++lts;
	
	ce = (chat_entry *) &(out_update->entry);
	strcpy(ce->user, u);
	strcpy(ce->room, r);
	strcpy(ce->text, t);
}

void build_likeEntry (char * u, lts_entry ref, char a) {
	like_entry *le;
	out_update->tag = LIKE;
	out_update->lts.ts = ++lts;
	
	le = (like_entry *) &(out_update->entry);
	strcpy(le->user, u);
	le->lts.ts = ref.ts;
	le->lts.pid = ref.pid;
	le->action = a;
}

int log_update(update *u) {
  if (logfile) {
    if (fwrite(u, sizeof(update), 1, logfile) == 1) {
      fflush(logfile);
      return TRUE;
    }
    else {
      logerr("Failed to log update to disk\n");
      exit(1);
    }
  } 
  else {
    logerr("ERROR: log file is not open. can't log update\n");
    exit(1);
  }
}


/*------------------------------------------------------------------------------
 *  The "Apply" Functions are designed for applying updates to the 
 *      current global state. These are used to transform data/update messages
 *      into the current state.
 *----------------------------------------------------------------------------*/
int apply_update (update * u, int shouldLog, int send_clients) {
	
  /* Update LTS if its higher than our current one  */
  if (u->lts.ts > lts)  {
    lts = u->lts.ts;
  }

  /* Do not process any duplicate updates EVER */
  if (update_ll_get_inorder_fromback(&updates, u->lts)) {
    logdb("DUPLICATE update: (%d, %d). Will not apply\n", u->lts.ts, u->lts.pid);
    return DUPLICATE_UPDATE;
  }
  
  if (shouldLog) {
    log_update(u);
    logdb("LOGGING update: (%d, %d).\n", u->lts.ts, u->lts.pid);
  }

  /* Insert into the list of updates */
  update_ll_insert_inorder_fromback(&updates, *u);

  /* Incorporate into state */
  int result;
  result = incorporate_into_state(u, send_clients);

  /* Upon success, try to apply pending updates */
  if (result == SUCCESSFUL_UPDATE) {
    try_pending_updates();
  }
  /* Or, Stash update as PENDING, if necessary */
  else if (result == PENDING_UPDATE) {
    logdb("Inserting update into PENDING\n");
    update_ll_insert_inorder_fromback(pending_updates, *u);
  } 
  else {
    logerr("ERROR: unhandled update!");
    exit(1);
  }

  return result;
}

void create_room (char * name)  {
  char        distrolist[MAX_GROUP_NAME];
  room_info   new_room;
  
  /*  Check to ensure room does not exist */
  if (room_ll_get(&rooms, name) == 0) {
    /* Build room_info */
    strcpy(new_room.name, name);
    new_room.chats = chat_ll_create();
    distrolist[0] = my_server_id;
    strcpy(&distrolist[1], name);
    strcpy(new_room.distro_group, distrolist);
  
    /* Append to room list */
    room_ll_append(&rooms, new_room);
    loginfo("NEW ROOM created, <%s>\n", new_room.name);
          
  }
  else {
    logdb("Room '%s' already exists\n", name);
  }

}

int apply_chat_update (chat_entry * ce, lts_entry * ts, int send_clients) {
  Message   out_msg;
  chat_info new_chat;
  chat_ll   *chat_list;
  room_info *rm; 

  /* First try to create the room (if it's new) */
  create_room(ce->room);

  /* Grab the room associated with this chat */
  rm = room_ll_get(&rooms, ce->room);
  if (!rm) {
    logerr("Error! Room never created\n");
    disconn_spread(mbox);
  }
    
  /* Grab the list of chats for the room */
  chat_list = &(rm->chats);
  if (!chat_list) {
    logerr("ERROR! Chat list does not exist for this room\n");
    return UNHANDLED_UPDATE;
  }
 
  /* Create and populte new chat_info */
  chat_list = &(rm->chats);
  
  strcpy(new_chat.chat.user, ce->user);
  strcpy(new_chat.chat.text, ce->text);
  strcpy(new_chat.chat.room, ce->room);
  new_chat.lts.pid = ts->pid;
  new_chat.lts.ts = ts->ts;
  new_chat.likes = like_ll_create();
  
  logdb("New Chat created for room <%s>: (%d,%d), User <%s> said '%s'\n", 
  	new_chat.chat.room, new_chat.lts.ts, new_chat.lts.pid, new_chat.chat.user, new_chat.chat.text);
  
  /* Append to chat list for this room */
  chat_ll_insert_inorder_fromback(chat_list, new_chat); /* Send a message to the distro group for this room */ 
  
  if (send_clients)  {
    prepareAppendMsg(&out_msg, ce->room, ce->user, ce->text, new_chat.lts);
    send_message(mbox, rm->distro_group,&out_msg);
  }
  return SUCCESSFUL_UPDATE;
}

int apply_like_update(lts_entry ref, lts_entry like_lts, char* user, char action, int send_clients) {
  Message   out_msg;
  update      *mu;
  chat_entry  *ce;
  room_info   *rm;
  chat_ll     *chat_list;
  chat_info   *ch;
  like_ll     *like_list;
  
  /* Grab the original chat update referenced by the like */
  logdb("Getting room for LTS Ref (%d, %d)\n", ref.ts, ref.pid);
  mu = update_ll_get_inorder(&updates, ref);
  if (mu == NULL) {
    logdb("Update does not exist locally.\n");
    // TODO this may happen after a partition
    return PENDING_UPDATE;
  }
  if (mu->tag != CHAT) {
    logerr("ERROR! Reference to non-chat update\n");
    return UNHANDLED_UPDATE;
  }

  /* Determine the room associated with the chat being liked */
  ce = (chat_entry *) &(mu->entry);
  rm = room_ll_get(&rooms, ce->room);
  
  /* Grab the list of chats for the room */ 
  chat_list = &(rm->chats);
  if (!chat_list) {
    logerr("ERROR! Chat list does not exist for this room\n");
    return UNHANDLED_UPDATE; 
  }

  /* Grab the chat_info and update its like list */
  ch = chat_ll_get_inorder_fromback(chat_list, mu->lts);
  like_list = &(ch->likes);
  like_ll_update_like(like_list, user, like_lts, action);
  
  /* Send a message to the distro group for this room */ 
  if (send_clients)  {
    prepareLikeMsg(&out_msg, user, ref, action, like_lts);
    send_message(mbox, rm->distro_group,&out_msg);
  }
  return SUCCESSFUL_UPDATE;
}

int incorporate_into_state(update *u, int send_clients) {
  chat_entry  *ce;
  like_entry  *le;
  int result;
  
  switch (u->tag)  {
		
    case CHAT:
      ce = (chat_entry *) &(u->entry);
      result = apply_chat_update (ce, &(u->lts), send_clients);
      break;
			
    case LIKE:
      le = (like_entry *) &(u->entry);
      result = apply_like_update(le->lts, u->lts, le->user, le->action, send_clients);
      break;
		
    default:
      logerr("ERROR! Received bad update from server group, tag was %c\n", u->tag);
      result = UNHANDLED_UPDATE;
    }
    return result;
}

void try_pending_updates() {
  update_ll *newpending = malloc(sizeof(update_ll));
  *newpending = update_ll_create();
  update_ll_node *curr_node = pending_updates->first;
  int result;
  
  while(curr_node) {
    result = incorporate_into_state(&(curr_node->data), TRUE); 
    if (result == SUCCESSFUL_UPDATE) {
      logdb("Succesfully applied pending update: (%d, %d)\n", curr_node->data.lts.ts, curr_node->data.lts.pid)
    }
    else {
      update_ll_insert_inorder_fromback(newpending, curr_node->data);
    }

    curr_node = curr_node->next;
  }

  // Swap lists
  update_ll_clear(pending_updates);
  free(pending_updates);
  pending_updates = newpending;
}



/*------------------------------------------------------------------------------
 *  Recovery & Reconciling functions - these are used to recover from disk for
 *      fault tolerance and to reoncile update between server when a network
 *      partition is healed.
 *----------------------------------------------------------------------------*/
int recover_from_disk(update_ll *list) {
  update curr_update;
  int num_in_log = 0;

  if(!logfile) {
    logerr("Cannot recover from disk. File is invalid\n");
    exit(1);
  }

  while (fread(&curr_update, sizeof(update), 1, logfile) == 1) {
    apply_update(&curr_update, FALSE, FALSE);
    num_in_log++;
  }
  
  loginfo("RECOVERY: Recovered %d Updates\n", num_in_log);
  return num_in_log;
}

void update_my_vector() {
  int i = 0;
  update_ll_node *curr;
  /* Reset vector */
  for(i = 0; i < MAX_SERVERS; i++) {
    my_vector[i] = 0;
  }

  /* Compute vector from updates. This can be optimized */
  /* Best solution is to search from the back until we get 1 update from each server */
  curr = updates.first;
  while (curr) {
    my_vector[curr->data.lts.pid] = curr->data.lts.ts;
    curr = curr->next;
  }

  return;
}

void resend_update (update *u) {
  Message out_msg;
  chat_entry  *ce;
  like_entry  *le;

  switch (u->tag)  {
		
    case CHAT:
      ce = (chat_entry *) &(u->entry);
      logdb("Resending chat append msg: '%s' said \"%s\" in room <%s>\n", 
        ce->user, ce->text, ce->room);
      prepareAppendMsg(&out_msg, ce->room, ce->user, ce->text, u->lts);
      break;
			
    case LIKE:
      le = (like_entry *) &(u->entry);
      logdb("Resending like msg: '%s'  did a %c-LIKE on chat-LTS (%d, %d)\n", 
        le->user, le->action, le->lts.ts, le->lts.pid);
      prepareLikeMsg(&out_msg, le->user, le->lts, le->action, u->lts);
      
      break;
		
    default:
      logerr("ERROR! Bad update in the update log, tag was %c\n", u->tag);
      return;
    }
    /* Send update to all servers  */
    send_message(mbox, all_server_group, &out_msg);

}



void send_history_to_client(char *roomname, char *client) {
  room_info    *room;
  chat_info    *chat;
  chat_ll_node *curr_chat_node;
  Message      out_msg;
  like_ll      *likes;
  like_ll_node *curr_like_node;
  like_entry   *like;
  
  room = room_ll_get(&rooms, roomname);
  if (!room) {
    logerr("ERROR! Failed to find room when sending history");
    exit(1);
  }

  curr_chat_node = room->chats.first;
  while(curr_chat_node) {
    chat = &(curr_chat_node->data);
    prepareAppendMsg(&out_msg, chat->chat.room, chat->chat.user, chat->chat.text, chat->lts);
    send_message(mbox, client, &out_msg);

    likes = &(chat->likes);
    curr_like_node = likes->first;
    while(curr_like_node) {
      like = &(curr_like_node->data);
      prepareLikeMsg(&out_msg, like->user, chat->lts, like->action, like->lts);
      send_message(mbox, client, &out_msg);
      curr_like_node = curr_like_node->next;
    }

    curr_chat_node = curr_chat_node->next;
  }
}


/*  Helper functions for managing client/server changes  */
int get_server_num (char * svr_name) {
  int num;
  
  // Format for the svr_name is #server-X#ugradYY, thus X=svr_name[8]
  num = (int)(svr_name[8]) - (int)('0');
  return num-1;  
}

void remove_client(char *name) {
  num_connected_clients -= 1;
  client_ll_remove(&connected_clients, name);
}

void add_client(char *name) {
  client_info new_client;
  strcpy(new_client.name, name);
  num_connected_clients += 1;
  client_ll_append(&connected_clients, new_client);
}



/* Helper functions for display  */
void print_my_vector() {
  int i; 
  logdb("My vector:\n");
  for(i = 0; i < MAX_SERVERS; i++) {
   logdb("%d ", my_vector[i]);
  }
  logdb("\n");
}

void print_connected_servers () {
  int i;
  for (i=0; i<5; i++)  {
    if (connected_svr[i]) {
      logdb("  -Server #%d\n", i+1);
    }
  }
}

void print_connected_clients() {
  client_ll_print(&connected_clients);
}

