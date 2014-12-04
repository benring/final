/******************************************************************************
 * File:     update.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  see update.h
 *
 *****************************************************************************/
#include "update_ll.h"
#include "stdio.h"
#include "stdlib.h"

update_ll update_ll_create() {
  update_ll result;
  result.first = 0;
  result.last = 0;
  return result;
}

int update_ll_is_empty(update_ll* list) {
  if (list->first) {
    return 0;
  }
  else {
    return 1;
  }
}

void update_ll_print(update_ll* list) {
  if (update_ll_is_empty(list)) {
    printf("Empty List\n");
  }
  else {
    printf("First: (%d,%d)\n", list->first->data.lts.ts, list->first->data.lts.pid);
    printf("Last: (%d,%d)\n", list->last->data.lts.ts, list->last->data.lts.pid);
    printf("Contents: \n");
    update_ll_node* curr = list->first;
    while (curr) {
      printf("(%d,%d)\n", curr->data.lts.ts, curr->data.lts.pid);
      curr = curr->next;
    }
  }
}

int update_ll_append(update_ll* list, ELEM data) {
  update_ll_node *node = malloc(sizeof(update_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  node->data = data;
  node->prev = 0;
  node->next = 0;

  if(update_ll_is_empty(list)) {
    list->first = node;
    list->last = node;
  }
  else {
    update_ll_node* old_last = list->last;
    old_last->next = node;
    node->prev = old_last;
    list->last = node;
  }

  return 1; /* success */
}


int update_ll_insert_inorder(update_ll* list, ELEM data) {
  /* If list is empty, just append */ 
  if(update_ll_is_empty(list)) {
    update_ll_append(list, data);
    return 1; /* success */
  }

  update_ll_node *node = malloc(sizeof(update_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  
  node->data = data;
  node->prev = 0;
  node->next = 0;
 
  update_ll_node* prev = 0;
  update_ll_node* curr = list->first;
 
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


int update_ll_insert_inorder_fromback(update_ll* list, ELEM data) {
  /* If list is empty, just append */ 
  if(update_ll_is_empty(list)) {
    update_ll_append(list, data);
    return 1; /* success */
  }

  update_ll_node *node = malloc(sizeof(update_ll_node));
  if (!node) {
    return 0; /* failure */
  }
  
  node->data = data;
  node->prev = 0;
  node->next = 0;
 
  update_ll_node* next = 0;
  update_ll_node* curr = list->last;
 
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

ELEM* update_ll_get(update_ll* list, lts_entry lts) {
  ELEM* result = 0;
  if (update_ll_is_empty(list)) {
    return result;
  }

  update_ll_node* curr = list->first;
  while(curr) {
    if(lts_eq(curr->data.lts, lts)) {
      result = &curr->data;
      return result;
    }
    curr = curr->next;
  }
  
  return result;
}

ELEM* update_ll_get_inorder(update_ll* list, lts_entry lts) {
  ELEM* result = 0;
  if (update_ll_is_empty(list)) {
    return result;
  }

  update_ll_node* curr = list->first;
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

ELEM* update_ll_get_inorder_fromback(update_ll* list, lts_entry lts) {
  ELEM* result = 0;
  if (update_ll_is_empty(list)) {
    return result;
  }

  update_ll_node* curr = list->last;
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

int update_ll_clear(update_ll* list) {
  update_ll_node* curr = list->first;
  update_ll_node* next;
  while(curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }
  list->first = 0;
  list->last = 0;
  return 1;
}

int update_ll_trim(update_ll *list, int ts, int pid) {
  update_ll_node* curr = list->first;
  lts_entry tmp;
  tmp.ts = ts;
  tmp.pid = pid;
  if (update_ll_is_empty(list)) {
    return -1;
  }

  update_ll_node* prev = 0;
  update_ll_node* next = 0;
  while(curr) { 
    /* Terminate early: */
    if (lts_greaterthan(curr->data.lts, tmp)) {
      return 0;
    }
    next = curr->next;
    if (curr->data.lts.pid == pid && curr->data.lts.ts < ts) {
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

      free(curr);
    }
    else {
      prev = curr;
    }
    curr = next;
  }
  return 0;
}

update update_ll_pop(update_ll *list) {
  update result;
  update_ll_node* curr;
  
  if (update_ll_is_empty(list)) {
    printf("ERROR. CANNOT POP FROM EMPTY UPDATE LIST\n");
    exit(1);
  }
  curr = list->first;
  
  list->first = curr->next;
  if (list->first) {
    list->first->prev = 0; 
  }
  else {
    list->last = 0;
  }

  result = curr->data;
  free(curr);
  return result; 
}
