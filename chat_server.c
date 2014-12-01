#include "config.h"
#include "transaction.h"
#include "message.h"
#include "commo.c"
#include "update_ll.h"
#include "like_ll.h"
#include "room_ll.h"
#include <limits.h>
#define ALL_SERVER_GROUP "server-all"

#define INIT 0
#define READY 1
#define RUN 2
#define RECONCILE 3

#define DUPLICATE_UPDATE 1
#define UNHANDLED_UPDATE 2
#define PENDING_UPDATE 3
#define SUCCESSFUL_UPDATE 4

/* Forward Declared functions */
static	void		Run();
static	void		Reconcile();
void 			Initialize (char * server_index);
float 			time_diff (struct timeval s, struct timeval e);
void 		build_roomEntry (char * r);
void 		build_chatEntry (char * u, char * r, char * t);
void 		build_likeEntry (char * u, lts_entry e, char a);

/* Message handling vars */
static	char			User[80];
static  char    		Private_group[MAX_GROUP_NAME];
static  mailbox 		mbox;
static  Message			mess;
static	Message			update_message;

/* Protocol vars  */
static 	unsigned int		me;
static  char    		my_server_id;
static  char				server_name[MAX_SERVERS][MAX_GROUP_NAME];
static  char				server_inbox[MAX_SERVERS][MAX_GROUP_NAME];
static	char				server_group[MAX_GROUP_NAME];
static	char				client_group[MAX_GROUP_NAME];


/*  Server Chat State  */
static	int					my_state;
static  int		        	connected_svr[MAX_SERVERS];
static	unsigned int		lts;
static  room_ll				rooms;
static	update_ll			updates;
static  update_ll                      *pending_updates;

static  client_ll               connected_clients;
static  int             	num_connected_clients = 0;

static	update				*out_update;

static FILE                             *logfile;
static char                 logfilename[NAME_LEN];
static unsigned int         my_vector[MAX_SERVERS];
static unsigned int         expected_vectors[MAX_SERVERS];
static unsigned int         my_responsibility[MAX_SERVERS];
static unsigned int         min_lts_vector[MAX_SERVERS];
static unsigned int         max_lts_vector[MAX_SERVERS];
static unsigned int         all_svr_lts_vectors[MAX_SERVERS][MAX_SERVERS];




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
  
  /* Read the log file */ 
  if (logfile = fopen(logfilename, "a+b")) {
    fseek(logfile, 0, SEEK_SET);
    recover_from_disk(&updates);
  }

  join_group(mbox, server_group);
  join_group(mbox, client_group);
  join_group(mbox, server_name[me]);

  logdb("Attaching reader event handler\n");
  logdb("[TRANSITION] Entering RUN state\n");
  my_state = RUN;
  E_attach_fd(mbox, READ_FD, Run, 0, NULL, HIGH_PRIORITY );
  E_handle_events();
  fclose(logfile);

  disconn_spread(mbox);
  free(pending_updates);

  return(0);
}

#include "utils.h"

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

  printf("My vector:\n");
  for(i = 0; i < MAX_SERVERS; i++) {
    printf("%d ", my_vector[i]);
  }
  printf("\n");

  return;
}

void handle_server_change(int num_members, char members[MAX_CLIENTS][MAX_GROUP_NAME]) {
  Message out_msg;
  LTSVectorMessage *ltsm;

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
    strcpy(server_inbox[i],  members[i]);
    logdb("Server #%d: Private MSG inbox is %s", i, server_inbox[i]);
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
      loginfo("  Server %d has %s the server-group \n", i+1, status_change_msg);

      /* Update */
      connected_svr[i] = new_connected_svr[i];
    }
  }

  print_connected_servers();

  /*  Send updated list of connected servers to all clients  */
  prepareViewMsg(&out_msg, connected_svr);
  send_message(mbox, client_group, (char *) &out_msg, sizeof(Message));
  logdb("Sending updated list of servers to all clients\n");

  if (new_members) {
    my_state = RECONCILE;
    update_my_vector();

    /* Expect an additional vector from each server */
    for (i=0; i < MAX_SERVERS; i++) {
      if (connected_svr[i]) {
        expected_vectors[i]++;
      }
    }

    /* Send out my vector */
    out_msg.tag = LTS_VECTOR;
    ltsm = &(out_msg.payload);
    ltsm->sender = me; 
    for (i = 0; i < MAX_SERVERS; i++) {
      ltsm->lts[i] = my_vector[i]; 
    }

    /* Handle my own vector since it wont be received */
    expected_vectors[me]--;
    
    for (i=0; i < MAX_SERVERS; i++) { 
      if (my_vector[i] < min_lts_vector[i]) {
        min_lts_vector[i] = my_vector[i];
      }

      if (max_lts_vector[i] == 0 || my_vector[i] > max_lts_vector[i]) {
        max_lts_vector[i] = my_vector[i];
      }
    }

    logdb("Sending my lts vector\n");
    send_message(mbox, server_group, &out_msg, sizeof(Message));
  }

  Reconcile();
}

/* Update the list of connected clients */
void handle_client_change(int num_members, char members[MAX_CLIENTS][MAX_GROUP_NAME]) {
  int i;
  Message out_msg;
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
      
 			prepareViewMsg(&out_msg, connected_svr);
			send_message(mbox, members[i], &out_msg, sizeof(Message));

      
    }
  }

  print_connected_clients();
}

/*  Following "Apply" Functions are designed for applying updates to the 
 *  current global state  */

int apply_room_update (char * name)  {
  char        distrolist[MAX_GROUP_NAME];
  room_info   new_room;
  
  /*  If this is a new room, create it & add to room list */
  if (room_ll_get(&rooms, name) == 0) {
    /* Build room_info */
    strcpy(new_room.name, name);
    new_room.chats = chat_ll_create();
    new_room.attendees = client_ll_create();
    distrolist[0] = my_server_id;
    strcpy(&distrolist[1], name);
    strcpy(new_room.distro_group, distrolist);
  
    /* Append to room list */
    room_ll_append(&rooms, new_room);
    logdb("NEW ROOM created, <%s>\n", new_room.name);
          
    /*  Server JOINs 2 groups for a room: *    1. Spread Distro group for to send updates to clients in the room *    2. Spread Membership group for attendees  */
    	
    logdb("Attempting to JOIN room, %s, distrolist: %s  [%c]\n", name, distrolist, my_server_id);
    join_group(mbox, distrolist);
    join_group(mbox, name);
  }
  else {
    logdb("Room '%s' already exists", name);
  }

  return SUCCESSFUL_UPDATE;
}

int apply_chat_update (chat_entry * ce, lts_entry * ts) {
  // TODO: always send out the updates? even during recovery?
  // seems redundant. but maybe necessary if we crashed
  Message   out_msg;
  chat_info new_chat;
  chat_ll   *chat_list;
  room_info *rm; 

  /* Grab the room associated with this chat */
  rm = room_ll_get(&rooms, ce->room);
  if (!rm) {
    logdb("WARNING! Room does not exist for thsi chat\n");
    return PENDING_UPDATE;
  }
    
  /* Grab the list of chats for the room */
  chat_list = &(rm->chats);
  if (!chat_list) {
    logdb("ERROR! Chat list does not exist for this room\n");
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
  prepareAppendMsg(&out_msg, ce->room, ce->user, ce->text, new_chat.lts);
  send_message(mbox, rm->distro_group,(char *) &out_msg, sizeof(Message));
  return SUCCESSFUL_UPDATE;
}

int apply_like_update(lts_entry ref, lts_entry like_lts, char* user, char action) {
  Message   out_msg;
  update      *mu;
  chat_entry  *ce;
  room_info   *rm;
  chat_ll     *chat_list;
  chat_info   *ch;
  like_ll     *like_list;
  like_entry  new_like;
  like_entry  *old_like;
  
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
  logdb("Current Like list for associated chat at LTS (%d, %d)\n", ref.ts, ref.pid);

  like_list = &(ch->likes);
  like_ll_print(like_list);

  like_ll_update_like(like_list, user, like_lts, action);

  like_ll_print(like_list);

    // TODO I think both of these things can happen after a partition
    // So maybe checks can be client side only
    //if (does_like(like_list, user) && action == ADD_LIKE) {
    //  loginfo("User cannot like a chat s/he already likes!\n");
    //  // TODO: Return LIKE-REJECT MSG to client
    //  return 0;
    //}

    //if (!does_like(like_list, user) && action == REM_LIKE) {
    //  loginfo("User cannot remove a like a non-liked chat\n");
    //  // TODO: Return LIKE-REJECT MSG to client
    //  return 0;
    //}

  
  /* Send a message to the distro group for this room */ 
  prepareLikeMsg(&out_msg, user, ref, action, like_lts);
  send_message(mbox, rm->distro_group,(char *) &out_msg, sizeof(Message));

  return SUCCESSFUL_UPDATE;
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

int recover_from_disk(update_ll *list) {
  update curr_update;
  int num_in_log = 0;

  if(!logfile) {
    logerr("Cannot recover from disk. File is invalid\n");
    exit(1);
  }

  while (fread(&curr_update, sizeof(update), 1, logfile) == 1) {
    apply_update(&curr_update, FALSE);
    num_in_log++;
  }
  
  logdb("Recovered %d Updates\n", num_in_log);
  return num_in_log;
}

int incorporate_into_state(update *u) {
  room_entry  *re;
  chat_entry  *ce;
  like_entry  *le;
  int result;
  
  switch (u->tag)  {
    case ROOM: 
      re = (room_entry *) &(u->entry);
      result = apply_room_update (re->room);
      break;
		
    case CHAT:
      ce = (chat_entry *) &(u->entry);
      result = apply_chat_update (ce, &(u->lts));
      break;
			
    case LIKE:
      le = (like_entry *) &(u->entry);
      result = apply_like_update(le->lts, u->lts, le->user, le->action);
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
  
  logdb("Trying to apply pending updates. Before:\n");
  update_ll_print(pending_updates);
 
  while(curr_node) {
    result = incorporate_into_state(&(curr_node->data)); 
    if (result == SUCCESSFUL_UPDATE) {
      logdb("Succesfully applied pending update: (%d, %d)\n", curr_node->data.lts.ts, curr_node->data.lts.pid)
    }
    else {
      update_ll_insert_inorder_fromback(newpending, curr_node->data);
    }

    curr_node = curr_node->next;
  }

  // Swap lists
  // TODO clear() function to free each node
  free(pending_updates);
  pending_updates = newpending;
  logdb("After: ");
  update_ll_print(pending_updates);
}

int apply_update (update * u, int shouldLog) {
	
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
  result = incorporate_into_state(u);

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

void resend_update (update *u, int recv_svr[MAX_SERVERS]) {
  Message out_msg;
  room_entry  *re;
  chat_entry  *ce;
  like_entry  *le;
  int         i;

  switch (u->tag)  {
    case ROOM: 
      re = (room_entry *) &(u->entry);
      logdb("Resending room update on %s\n", re->room);
      prepareJoinMsg (&out_msg, re->room, "RECONCILE", u->lts);
      break;
		
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
    // FOR NOW:  default to send to all servers
    send_message(mbox, server_group, (char *)&out_msg, sizeof(Message));

    /* TODO:  Accept a list of servers to send to and only send to those:
     * 
    for (i=0; i<MAX_SERVERS; i++) {
      if (recv_svr[i]) {
        send_message(mbox, server_inbox[i], (char *)&out_msg, sizeof(Message));
      }
    }
     */
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
    send_message(mbox, client, (char *) &out_msg, sizeof(Message));

    likes = &(chat->likes);
    curr_like_node = likes->first;
    while(curr_like_node) {
      like = &(curr_like_node->data);
      prepareLikeMsg(&out_msg, like->user, chat->lts, like->action, like->lts);
      send_message(mbox, client, (char *) &out_msg, sizeof(Message));
      curr_like_node = curr_like_node->next;
    }

    curr_chat_node = curr_chat_node->next;
  }
}

void handle_client_command(char client[MAX_GROUP_NAME]) {
  Message        out_msg;
  JoinMessage 	 *jm;
  AppendMessage  *am;
  HistoryMessage *hm;
  LikeMessage 	 *lm;
  
  logdb("Client Request:  %c\n", mess.tag);
	
  switch (mess.tag) {
    case VIEW_MSG :
      /* Send the connected_servers to the client */
      prepareViewMsg(&out_msg, connected_svr);
      send_message(mbox, client, (char *)&out_msg, sizeof(Message));
      logdb("VIEW Request. Sending list of servers to client <%s>\n", client);
      break;
		
    case HISTORY_MSG :
      // TODO this is a no op right now. If the client already has the history, maybe we remove this type of MSG
      hm = (HistoryMessage *) mess.payload;
      logdb("HISTORY Request from user: <%s> on client: <%s>, for room <%s>\n", hm->user, client, hm->room);
      break;

    case JOIN_MSG :
      /* Create and Apply an Update. Send it out to serevers. Then send history to client */
      jm = (JoinMessage *) mess.payload;
      logdb("JOIN Request from user: <%s> on client: <%s>, for room <%s>\n", jm->user, client, jm->room);
      build_roomEntry(jm->room);
      apply_update(out_update, TRUE);
      prepareJoinMsg(&out_msg, jm->room, jm->user, out_update->lts);
      send_message(mbox, server_group, (char *)&out_msg, sizeof(Message));
      send_history_to_client(jm->room, client);
      break;

    case APPEND_MSG :
      /*  Create and Apply an Update. Send it out to Servers */
      am = (AppendMessage *) mess.payload;
      logdb("APPEND Request from user: <%s> on client: <%s>, for room <%s>. Msg is '%s'\n", am->user, client, am->room, am->text);
      build_chatEntry(am->user, am->room, am->text);
      apply_update(out_update, TRUE);
      prepareAppendMsg(&out_msg, am->room, am->user, am->text, out_update->lts);
      send_message(mbox, server_group, (char *)&out_msg, sizeof(Message));
      break;

    case LIKE_MSG: 
      /* Create and Apply an Update. Send it out to Servers */
      lm = (LikeMessage *) mess.payload;
      logdb("LIKE Request from user: <%s> on client: <%s>\n", lm->user, client);
      logdb("Action is '%c' for LTS: %d,%d\n", lm->action, lm->ref.ts, lm->ref.pid);
      
      build_likeEntry(lm->user, lm->ref, lm->action);
      
      // TODO: MAY NEED TO RTN SUCCESS on APPLY_UPDATE; FOR LIKEs
        // ONLY SEND IF APPLY_UPDATE RETURNS SUCCESS
      apply_update(out_update, TRUE);
      prepareLikeMsg(&out_msg, lm->user, lm->ref, lm->action, out_update->lts);
      send_message(mbox, server_group, (char *)&out_msg, sizeof(Message));
      break;

    default:
      logerr("ERROR unhandled command from client\n");
      exit(1);
  }
  return;
}

void handle_server_update() {
  update         new_update, *mu;
  JoinMessage    *jm;
  AppendMessage  *am;
  //HistoryMessage *hm;
  LTSVectorMessage *ltsm;
  LikeMessage 	 *lm;
  chat_entry     *ce;
  like_entry     *le;
  int sum = 0;
  int i;
  int shouldApply = TRUE;
  update_ll_node *curr_update;
  int           send_svr_update[MAX_SERVERS];
	
  logdb("Received '%c' Update from server\n", mess.tag);

  switch (mess.tag) {
    case JOIN_MSG:
      /* Build and appy the update */
      jm = (JoinMessage *) mess.payload;

      new_update.tag = ROOM;
      new_update.lts.pid = jm->lts.pid;
      new_update.lts.ts = jm->lts.ts;
      strcpy(new_update.entry, jm->room);
      
      logdb("  New Room <%s> from server group\n", jm->room);
      break;
      
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

      logdb("  New chat on room <%s> from server-group, LTS (%d,%d)\n", ce->room, new_update.lts.ts, new_update.lts.pid);
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

      logdb("  New like from server-group: User '%s' requests '%c' on LTS (%d,%d)\n", le->user, le->action, le->lts.ts, le->lts.pid);

      break;
    case LTS_VECTOR:
      shouldApply = FALSE;

      if (my_state != RECONCILE) {
        logerr("ERROR received vector when not in reconcile state\n");
      }

      ltsm = (LTSVectorMessage *) mess.payload;
      logdb(" RECEIVED AN LTS VECTOR FROM %d\n", ltsm->sender);
      for (i=0; i < MAX_SERVERS; i++) {
        logdb("  %d", ltsm->lts[i]);
        /* Update global svr_lts tracker -- for sending updates */
        if (ltsm->lts[i] > all_svr_lts_vectors[ltsm->sender][i]) {
          all_svr_lts_vectors[ltsm->sender][i] = ltsm->lts[i];
        }
      }
      logdb("\n");
      expected_vectors[ltsm->sender]--;
      
      for(i=0; i < MAX_SERVERS; i++) {
        sum += expected_vectors[i];
        my_responsibility[i] = 0;

        if (ltsm->lts[i] < min_lts_vector[i]) {
          min_lts_vector[i] = ltsm->lts[i]; 
        }
        if (max_lts_vector[i] == 0 || ltsm->lts[i] > max_lts_vector[i]) {
          max_lts_vector[i] = ltsm->lts[i];
        }
      }

      if (sum == 0) {
        /* Send out missing updates */
        for (i = 0; i < MAX_SERVERS; i++) {
          if (i == me && max_lts_vector[i] != 0) {
            my_responsibility[i] = TRUE;
          }
          // TODO this may cause multiple senders.
          // we can fix this by storing the pid along with the max LTS
          // and only that pid should send
          if (max_lts_vector[i] != 0 && my_vector[i] >= max_lts_vector[i] && !connected_svr[i]) {
            my_responsibility[i] = TRUE;
          }
        }

        logdb("Calculating updates to send: \n"); 
        
        for (i = 0; i < MAX_SERVERS; i++) {
           logdb ("Server %d. Min: %d. Max: %d\n", i, min_lts_vector[i], max_lts_vector[i]);
        }

        for (i = 0; i < MAX_SERVERS; i++) {
          if(my_responsibility[i]) {
            logdb("I am responsible for Server %d's updates. %d to %d\n", i, min_lts_vector[i], max_lts_vector[i]);
          }
        }

        int currpid;
        int currts;
        /* Send out updates */ 
        curr_update = updates.first;
        while(curr_update) {
          currpid = curr_update->data.lts.pid;
          currts = curr_update->data.lts.ts;
          if (my_responsibility[currpid]) {
            if(currts <= max_lts_vector[currpid] && currts >= min_lts_vector[currpid]) {
              logdb("Sending missing update (%d, %d)\n", curr_update->data.lts.ts, curr_update->data.lts.pid); 
              
              /* Only send to servers who need the update */
              for (i=0; i<MAX_SERVERS; i++) {
                send_svr_update[i] = FALSE;
                if (i != me && currts < all_svr_lts_vectors[i][currpid]) {
                  send_svr_update[i] = TRUE;
                }
              }
              resend_update(&(curr_update->data), send_svr_update);

              /* Resend the update */
              /* TODO: Update resend_update to also accept a server_mask specifying specific
                      servers which shoudl receive the updates 
                 will need a simple for-loop here to determine that mask  */

            }
            
          }
          curr_update = curr_update->next;
        }

      }

      break;
      
    default:
      logerr("ERROR! Received a non-update from server\n");
      exit(1);
  }

  // Hack for now. dont apply an LTS Vector, for example.
  if (shouldApply) {
    apply_update(&new_update, TRUE);
  }
}


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
  strcpy(server_group, SERVER_ALL_GROUP_NAME);

  /* Set servers spread user to server-i */
  strcpy(User, SERVER_NAME_PREFIX);
  User[7] = index;

  /* Set group name for connected clients */
  strcpy(client_group, SERVER_GROUP_PREFIX);
  client_group[8] = index;

  /* Record the name of each server's group */
  /* Register servers as not connected */
  for (i=0; i<MAX_SERVERS; i++) {
    strcpy(server_name[i], "server-");
    server_name[i][7] = (char)(1 + i + (int)'0');
    connected_svr[i] = FALSE;
    expected_vectors[i] = 0;
    min_lts_vector[i] = UINT_MAX;
    max_lts_vector[i] = 0;
  }
  
  /* Do other program initialization */
  pending_updates = malloc(sizeof(update_ll));
  *pending_updates = update_ll_create();
  updates = update_ll_create();
  connected_clients = client_ll_create();
  lts = 0;

  update_message.tag = UPDATE;
  out_update = (update *) &(update_message.payload);
  out_update->lts.pid = me;

  sprintf(logfilename, "log_%c.txt", my_server_id);
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



static	void	Run()   {
  char		sender[MAX_GROUP_NAME];
  char		target_groups[MAX_CLIENTS][MAX_GROUP_NAME];
  int			num_target_groups;
  int			service_type;
  int16		mess_type;
  int			endian_mismatch;
  int			ret;

  char		*name;

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
    //changed_group = sender;
    //num_members   = num_target_groups;
    //members       = target_groups;

    if (strcmp(sender, server_group) == 0) {
      handle_server_change(num_target_groups, target_groups);
    }
    else if (strcmp(sender, client_group) == 0) {
      handle_client_change(num_target_groups, target_groups);
    }
    else {
      logdb ("Group change on <%s>\n", sender);
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
