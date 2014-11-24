#include "room_ll.h"
#include "stdio.h"
#include "stdlib.h"

room_ll room_ll_create() {
  room_ll result;
  result.first = 0;
  result.last = 0;
  return result;
}

int room_ll_is_empty(room_ll* list) {
  if (list->first) {
    return 0;
  }
  else {
    return 1;
  }
}

void room_ll_print(room_ll* list) {
  if (room_ll_is_empty(list)) {
    printf("Empty List\n");
  }
  else {
    /*
    printf("First: (%d,%d)\n", list->first->data.lts.ts, list->first->data.lts.pid);
    printf("Last: (%d,%d)\n", list->last->data.lts.ts, list->last->data.lts.pid);
    printf("Contents: \n");
    room_ll_node* curr = list->first;
    while (curr) {
      printf("(%d,%d)\n", curr->data.lts.ts, curr->data.lts.pid);
      curr = curr->next;
    }
    */
  }
  
}

int room_ll_append(room_ll* list, room_info data) {
  room_ll_node *node = malloc(sizeof(room_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  node->data = data;
  node->prev = 0;
  node->next = 0;

  if(room_ll_is_empty(list)) {
    list->first = node;
    list->last = node;
  }
  else {
    room_ll_node* old_last = list->last;
    old_last->next = node;
    node->prev = old_last;
    list->last = node;
  }

  return 1; /* success */
}

/*room_info* room_ll_get(room_ll* list, lts_entry lts) {
  room_info* result = 0;
  if (room_ll_is_empty(list)) {
    return result;
  }

  room_ll_node* curr = list->first;
  while(curr) {
    if(lts_eq(curr->data.lts, lts)) {
      result = &curr->data;
      return result;
    }
    curr = curr->next;
  }
  
  return result;
}
*/
