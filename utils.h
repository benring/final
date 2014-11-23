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

void remove_client(int pos) {
  num_connected_clients -= 1;
  /* Copy the last element of the list into this position */
  memcpy(connected_clients[pos], connected_clients[num_connected_clients], MAX_GROUP_NAME);
}

void add_client(char *name) {
  memcpy(connected_clients[num_connected_clients], name, MAX_GROUP_NAME);
  num_connected_clients += 1;
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
  int i;

  /* Print current clients */
  logdb("Current client list: \n");

  for (i=0; i<num_connected_clients; i++) {
    logdb("  -%s\n", connected_clients[i]);
  }

}
