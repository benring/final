/* To be included in-place, with access to globals 
 * until/unless we switch to passing around state directly*/
int is_member (char recipients[5][MAX_GROUP_NAME], int num_recipients, char node[MAX_GROUP_NAME])  {
  int found_user, i;
  found_user = FALSE;
  for (i=0; i<num_recipients; i++) {
    if (strcmp(recipients[i], node) == 0) {
      found_user = TRUE;
    }
  }
  return found_user;
}

int num_connected_servers ()  {
  int 	num, i;
  num = 0;
  for (i=0; i<5; i++) {
    if (connected_svr[i]) {
      num++;
    }
  }
  return num;
}

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

void print_connected_servers () {
  int i;
  logdb("Connected Servers:\n");
  for (i=0; i<5; i++)  {
    if (connected_svr[i]) {
      logdb("  -Server #%d\n", i+1);
    }
  }
}

void print_connected_clients() {

  /* Print current clients */
  logdb("Current client list: \n");

  client_ll_print(&connected_clients);
}
