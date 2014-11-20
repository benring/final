
#include "message.h"

#define LOG_FILE "updates.log"

static 	FILE            	*roomlog;
static 	FILE            	*chatlog;
static 	FILE            	*likelog;

static  char				server_group[NAME_LEN];

typedef struct server_state {
	linkedList<connected_clients>	clients;
	linkedList<room_state>			rooms;
	linkedList<updates>				updates;   /*sorted by lts*/
	int								fsm;
	int								connected_servers[5]				
} server_state;

	
void initialize (server_state state)  {
	/* 1. Load data from log */
	init(state)
	open(LOG_FILE);
	foreach (update in file):
	  insert(state.updates, update);  // Uses sort insert method
	foreach (update in state.update):
	  apply_update (state, update);
}

void reconcile() {
	
	int		min_lts[5]
	int		max_lts[5]
	int		expected_vectors[5]
	int		vectors[5][5]
	
	
	
	
	send lts_vector_msg  (also contains: connected clients)
	for x in 1..5:
		if connected_server[x]:
			expected_vector[x]++
	wait to recive lts_vector_msg from all connected_servers
	while (sum(expected_vector > 0)):
		recv_message()
		if regular_msg && lts_vector_msg:
			expceted_vector[svr]--
			if recv_vector > vectors[svr]
				vectors[svr] = recv_vector
			foreach (client in svr.connected_client_list):
				state.room.addattendee(client)
				
		if regular_msg && from_client
			handle_client_command()
			
		if regular_msg && from_server
			handle_server_update()			
			
		if membership_msg && (any_client_group):
			handle_client_change
		
		if membership_msg && (server_group):
			change = handle_server_change()
			if change == JOIN:
				send lts_vector_msg  <<<-connected clients
				for x in 1..5:
					if connected_server[x]:
						expected_vector[x]++
			if change == LEAVE:
				expected_vector[svr] = 0
		
	foreach server:
		calc min/max lts <--- based on connected servers
	foreach server:
		if max_lts(svrnum) == me:  (include tie-breaker here)
			send (updates.svrnum for max_lts[svr_num] - min_lts[svr_num])  <<-- SEND TO GROUP, can optimize with multi-group multicast
	state = RUN
}

void handle_client_change() {
	if client_leaves:
		remove(state.clients, client)
		foreach(c in clients):
			if c.name == departed_client.name &&
				c.room == departed_client.room:
					USER_LEAVE = false;
					break;
		if (USER_LEAVE):
			state.rooms(c.room).remove(user)
			SEND (Update(R, departed_user, LEAVE))
	
	if client_joins:
		client = <something from msg>
		state.clients.add(client)
}

int handle_server_change(state) {   // RETURNS LEAVE / JOIN
	if server_leaves:
		update state.connected_servers
		foreach room in state.rooms:
			foreach a in attendees:
				if a.server == departed_server:
					a.remove()
	
	if server_joins && membership > 1:
		
		state.fsm = RECONCILE
		return

}



void handle_client_command(state, message) {
	switch message->tag 
	case JOIN_ROOM:
		if user_exists in state.clients:
			if (message.user)).room == message.room
				return????
			else
				state.rooms(c.room).remove(user)
				SEND (Update(ROOM, departed_user, LEAVE))
		if !exists(state.room, message.room):
			state.room.add(message.room)
		if !exists(room.attendees(message.user, server(me))
			entry = room_entry(message.user, me, message.room, JOIN)
			update = (JOIN_ROOM, lts, entry)
			write(update)
			send(server_group, update)
			state.rooms(message.room).add(client)
		state.client(message.client).update(message.room)
		
	case CHAT:
		entry = chat_entry(message.user, message.room, message.text)
		update = (CHAT, lts, entry)
		write(update)
		send(server_group, update)
	
	case LIKE:
		//assumes client has sent valid
		// NEED SOME WAY TO correlate messages & likes LTS's
		if action == LIKE
			add like to: client.room.message.likes(user)
		if action == REMOVE
			remove like from: client.room.message.likes(user)
		entry = like_entry(message.user, )
}

void run (state)  {
	recv_message()
	if membership_msg && (spread_server_group):
		handle_server_change
	
	if membership_msg && (any_client_group):
		handle_client_change
	
	if regular_msg && from_client
		handle_client_command()
		
	if regular_msg && from_server
		handle_server_update()
		
		
}

int main {
		state.fsm = INIT
		initialize(state)
		state.fsm = READY
		join(server_group)
		join(my_server_group)
		state.fsm = RUN
		while (true):
			if state == RUN
				run(state)
			if state == RECONCILE
				reconcile(state)
}