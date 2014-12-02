#ifndef CLIENT_LL_H
#define CLIENT_LL_H

#include "transaction.h"
#include "chat_ll.h"
#include "lts_utils.h"


typedef struct client_ll_node {
  client_info data;
  struct client_ll_node* prev;
  struct client_ll_node* next;
} client_ll_node;

typedef struct client_ll {
  client_ll_node* first;
  client_ll_node* last;
} client_ll;

/* Create an empty list */
client_ll client_ll_create();

/* Check if the list is empty */
int client_ll_is_empty(client_ll* list);

/* Print the contents of a list */
void client_ll_print(client_ll* list);

/* Append to the end of the list */
int client_ll_append(client_ll* list, client_info data);

/* Search the whole (unordered) list for an lts */ 
client_info* client_ll_get(client_ll *list, char * name); 

client_info* client_ll_remove(client_ll *list, char * name);

#endif
