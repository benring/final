#include "client_ll.h"
#include "stdio.h"
#include "stdlib.h"

client_ll client_ll_create() {
  client_ll result;
  result.first = 0;
  result.last = 0;
  return result;
}

int client_ll_is_empty(client_ll* list) {
  if (list->first) {
    return 0;
  }
  else {
    return 1;
  }
}

void client_ll_print(client_ll* list) {
  if (client_ll_is_empty(list)) {
    printf("Empty List\n");
  }
  else {
    printf("First: %s\n", list->first->data.name);
    printf("Last: %s\n", list->last->data.name);
    printf("Contents: \n");
    client_ll_node* curr = list->first;
    while (curr) {
      printf("%s\n", curr->data.name);
      curr = curr->next;
    }
  }
  
}

int client_ll_append(client_ll* list, client_info data) {
  client_ll_node *node = malloc(sizeof(client_ll_node));
	client_ll_node *old_last;
	
  if (!node) {
    return 0; /* failure */
  }

  node->data = data;
  node->prev = 0;
  node->next = 0;

  if(client_ll_is_empty(list)) {
    list->first = node;
    list->last = node;
  }
  else {
    old_last = list->last;
    old_last->next = node;
    node->prev = old_last;
    list->last = node;
  }

  return 1; /* success */
}

client_info* client_ll_get(client_ll* list, char * name) {
  client_info* result = 0;
  client_ll_node* curr = list->first;

  if (client_ll_is_empty(list)) {
    return result;
  }

  while(curr) {
		if (strcmp(curr->data.name, name) == 0) {
			result = &curr->data;
			return result;
		}
		curr = curr->next;
	}
  return result;
}

client_info* client_ll_remove(client_ll* list, char * name) {
  client_info* result = 0;
  client_ll_node* curr = list->first;
  if (client_ll_is_empty(list)) {
    return result;
  }

  client_ll_node* prev = 0;
  while(curr) {
		if (strcmp(curr->data.name, name) == 0) {
		        /* removing head of list */
		        if (!prev) {
                          list->first = curr->next;                        
                        }
                        /* removing tail of list */
                        if (curr == list->last) {
                          list->last = curr->prev;
                        }
                        if (prev) {
                          prev->next = curr->next;
                        }
                        if (curr->next) {
                          curr->next->prev = prev;
                        }
                        
			result = &curr->data;
			return result;
		}
		prev = curr;
		curr = curr->next;
	}
  return result;
}
