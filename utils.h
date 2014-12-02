/* To be included in-place, with access to globals 
 * until/unless we switch to passing around state directly*/
//int is_member (char recipients[5][MAX_GROUP_NAME], int num_recipients, char node[MAX_GROUP_NAME])  {
//  int found_user, i;
//  found_user = FALSE;
//  for (i=0; i<num_recipients; i++) {
//    if (strcmp(recipients[i], node) == 0) {
//      found_user = TRUE;
//    }
//  }
//  return found_user;
//}

//
//int num_connected_servers ()  {
//  int 	num, i;
//  num = 0;
//  for (i=0; i<5; i++) {
//    if (connected_svr[i]) {
//      num++;
//    }
//  }
//  return num;
//}

