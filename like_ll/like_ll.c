#include "like_ll.h"
#include "stdio.h"
#include "stdlib.h"

like_ll like_ll_create() {
  like_ll result;
  result.first = 0;
  result.last = 0;
  return result;
}

int like_ll_is_empty(like_ll* list) {
  if (list->first) {
    return 0;
  }
  else {
    return 1;
  }
}

void like_ll_print(like_ll* list) {
  if (like_ll_is_empty(list)) {
    printf("Empty List\n");
  }
  else {
    printf("First: (%d,%d)\n", list->first->data.lts.ts, list->first->data.lts.pid);
    printf("Last: (%d,%d)\n", list->last->data.lts.ts, list->last->data.lts.pid);
    printf("Contents: \n");
    like_ll_node* curr = list->first;
    while (curr) {
      printf("(%d,%d)\n", curr->data.lts.ts, curr->data.lts.pid);
      curr = curr->next;
    }
  }
}

int like_ll_append(like_ll* list, like_entry data) {
  like_ll_node *node = malloc(sizeof(like_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  node->data = data;
  node->prev = 0;
  node->next = 0;

  if(like_ll_is_empty(list)) {
    list->first = node;
    list->last = node;
  }
  else {
    like_ll_node* old_last = list->last;
    old_last->next = node;
    node->prev = old_last;
    list->last = node;
  }

  return 1; /* success */
}

int like_ll_insert_inorder(like_ll* list, like_entry data) {
  /* If list is empty, just append */ 
  if(like_ll_is_empty(list)) {
    like_ll_append(list, data);
    return 1; /* success */
  }

  like_ll_node *node = malloc(sizeof(like_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  
  node->data = data;
  node->prev = 0;
  node->next = 0;
 
  like_ll_node* prev = 0;
  like_ll_node* curr = list->first;
 
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


int like_ll_insert_inorder_fromback(like_ll* list, like_entry data) {
  /* If list is empty, just append */ 
  if(like_ll_is_empty(list)) {
    like_ll_append(list, data);
    return 1; /* success */
  }

  like_ll_node *node = malloc(sizeof(like_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  
  node->data = data;
  node->prev = 0;
  node->next = 0;
 
  like_ll_node* next = 0;
  like_ll_node* curr = list->last;
 
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

like_entry* like_ll_get(like_ll* list, lts_entry lts) {
  like_entry* result = 0;
  if (like_ll_is_empty(list)) {
    return result;
  }

  like_ll_node* curr = list->first;
  while(curr) {
    if(lts_eq(curr->data.lts, lts)) {
      result = &curr->data;
      return result;
    }
    curr = curr->next;
  }
  
  return result;
}

like_entry* like_ll_get_inorder(like_ll* list, lts_entry lts) {
  like_entry* result = 0;
  if (like_ll_is_empty(list)) {
    return result;
  }

  like_ll_node* curr = list->first;
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

like_entry* like_ll_get_inorder_fromback(like_ll* list, lts_entry lts) {
  like_entry* result = 0;
  if (like_ll_is_empty(list)) {
    return result;
  }

  like_ll_node* curr = list->last;
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
