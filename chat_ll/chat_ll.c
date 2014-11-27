#include "chat_ll.h"
#include "stdio.h"
#include "stdlib.h"

chat_ll chat_ll_create() {
  chat_ll result;
  result.first = 0;
  result.last = 0;
  return result;
}

int chat_ll_is_empty(chat_ll* list) {
  if (list->first) {
    return 0;
  }
  else {
    return 1;
  }
}

void chat_ll_print(chat_ll* list) {
  int   index = 1;

  if (chat_ll_is_empty(list)) {
    printf("Empty List\n");
  }
  else {
    chat_ll_node* curr = list->first;
    while (curr) {
//      printf("(%d,%d) | %s: %s\n", curr->data.lts.ts, curr->data.lts.pid, curr->data.chat.user, curr->data.chat.text);
      printf("%d. [%s]: %s\n", index++, &(curr->data.chat.user[3]), curr->data.chat.text);
      curr = curr->next;
    }
  }
}

lts_entry chat_ll_get_lts(chat_ll *list, int i) {
  int curr_index = 1;
  lts_entry blank;
  chat_ll_node* curr_node = list->first;
  
  while (curr_node) {
    if(curr_index == i) {
      return curr_node->data.lts;
    }
    curr_index++;
    curr_node = curr_node->next;
  }

  blank.ts = 0;
  blank.pid = 0;
  return blank;

}

int chat_ll_append(chat_ll* list, chat_info data) {
  chat_ll_node *node = malloc(sizeof(chat_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  node->data = data;
  node->prev = 0;
  node->next = 0;

  if(chat_ll_is_empty(list)) {
    list->first = node;
    list->last = node;
  }
  else {
    chat_ll_node* old_last = list->last;
    old_last->next = node;
    node->prev = old_last;
    list->last = node;
  }

  return 1; /* success */
}

int chat_ll_insert_inorder(chat_ll* list, chat_info data) {
  /* If list is empty, just append */ 
  if(chat_ll_is_empty(list)) {
    chat_ll_append(list, data);
    return 1; /* success */
  }

  chat_ll_node *node = malloc(sizeof(chat_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  
  node->data = data;
  node->prev = 0;
  node->next = 0;
 
  chat_ll_node* prev = 0;
  chat_ll_node* curr = list->first;
 
  /* Find appropriate position for node */
  while(curr && lts_lessthan(curr->data.lts, data.lts)) {
    prev = curr;
    curr = curr->next; 
  }
  
  if(!prev) {
    /* node is now first element in list */
    node->next = curr;
    curr->prev = node;
    list->first = node;
  }
  else if(!curr) {
    /* node is now last element in list */
    node->prev = prev;
    prev->next = node;
    list->last = node;
  }
  else {
    /* node is not first of last */
    node->next = curr;
    node->prev = prev;

    prev->next = node;
    curr->prev = node;
  }
    
  return 1; 
}


int chat_ll_insert_inorder_fromback(chat_ll* list, chat_info data) {
  /* If list is empty, just append */ 
  if(chat_ll_is_empty(list)) {
    chat_ll_append(list, data);
    return 1; /* success */
  }

  chat_ll_node *node = malloc(sizeof(chat_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  
  node->data = data;
  node->prev = 0;
  node->next = 0;
 
  chat_ll_node* next = 0;
  chat_ll_node* curr = list->last;
 
  /* Find appropriate position for node */
  while(curr && lts_greaterthan(curr->data.lts, data.lts)) {
    next = curr;
    curr = curr->prev; 
  }
  
  if(!next) {
    /* node is now last element in list */
    node->prev = curr;
    curr->next = node;
    list->last = node;
  }
  else if(!curr) {
    /* node is now first element in list */
    node->next = next;
    next->prev = node;
    list->first = node;
  }
  else {
    /* node is not first of last */
    node->next = next;
    node->prev = curr;

    next->prev = node;
    curr->next = node;
  }
    
  return 1; 
}

chat_info* chat_ll_get(chat_ll* list, lts_entry lts) {
  chat_info* result = 0;
  if (chat_ll_is_empty(list)) {
    return result;
  }

  chat_ll_node* curr = list->first;
  while(curr) {
    if(lts_eq(curr->data.lts, lts)) {
      result = &curr->data;
      return result;
    }
    curr = curr->next;
  }
  
  return result;
}

chat_info* chat_ll_get_inorder(chat_ll* list, lts_entry lts) {
  chat_info* result = 0;
  if (chat_ll_is_empty(list)) {
    return result;
  }

  chat_ll_node* curr = list->first;
  while(curr) {
    if(lts_eq(curr->data.lts, lts)) {
      result = &curr->data;
      return result;
    }
    else if(lts_greaterthan(curr->data.lts, lts)) {
      /* fail early */
      return result;
    }
    curr = curr->next;
  }
  
  return result;
}

chat_info* chat_ll_get_inorder_fromback(chat_ll* list, lts_entry lts) {
  chat_info* result = 0;
  if (chat_ll_is_empty(list)) {
    return result;
  }

  chat_ll_node* curr = list->last;
  while(curr) {
    if(lts_eq(curr->data.lts, lts)) {
      result = &curr->data;
      return result;
    }
    else if(lts_lessthan(curr->data.lts, lts)) {
      /* fail early */
      return result;
    }
    curr = curr->prev;
  }
  
  return result;
}

int chat_ll_length(chat_ll *list) {
  int len = 0;
  if (chat_ll_is_empty(list)) {
    return 0;
  }
  
  chat_ll_node* c = list->first;
  while (c) {
    len++;
    c = c->next;
  }

  return len;
}

void chat_ll_remove_first(chat_ll *list) {
  if (chat_ll_is_empty(list)) {
    return;
  }
  list->first = list->first->next;
  list->first->prev = 0;
  return;
}
