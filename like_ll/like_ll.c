/******************************************************************************
 * File:     like.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  see like.h
 *
 *****************************************************************************/
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
  like_ll_node* curr;
  if (like_ll_is_empty(list)) {
    logdb("Empty List\n");
  }
  else {
    logdb("First: (%d,%d)\n", list->first->data.lts.ts, list->first->data.lts.pid);
    logdb("Last: (%d,%d)\n", list->last->data.lts.ts, list->last->data.lts.pid);
    logdb("Contents: \n");
    curr = list->first;
    while (curr) {
      logdb("(%d,%d) '%s'\n", curr->data.lts.ts, curr->data.lts.pid, curr->data.user);
      curr = curr->next;
    }
  }
}

int like_ll_append(like_ll* list, like_entry data) {
  like_ll_node *node = malloc(sizeof(like_ll_node));
  like_ll_node* old_last;
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
    old_last = list->last;
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
  like_ll_node* curr;
  if (like_ll_is_empty(list)) {
    return result;
  }

  curr = list->first;
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

int does_like (like_ll *list, char * name) {
  like_ll_node* curr;

  if (like_ll_is_empty(list)) {
    return FALSE;
  }
  like_ll_print(list);

  curr = list->first;
  while(curr) {
    if(strcmp(curr->data.user, name) == 0) {
      if (curr->data.action == ADD_LIKE) {
        return TRUE;
      }
      else {
        return FALSE;
      }
    }
    curr = curr->next;
  }
  return FALSE;
}

like_entry* like_ll_get_user(like_ll *list, char *user) {

  like_entry* result = 0;
  like_ll_node* curr;
  if (like_ll_is_empty(list)) {
    return result;
  }

  curr = list->first;
  while(curr) {
    if(strcmp(curr->data.user, user) == 0) {
      result = &curr->data;
      return result;
    }
    curr = curr->next;
  }
  
  return result;
}

int like_ll_count_likes(like_ll *list) {
  int count = 0;
  like_ll_node* curr;
  if (like_ll_is_empty(list)) {
    return count;
  }

  curr = list->first;
  while(curr) {
    if (curr->data.action == ADD_LIKE) {
      count++;
    }
    curr = curr->next;
  }
  return count;
}

int like_ll_update_like(like_ll *like_list, char* user, char* room, lts_entry like_lts, char action) {
  like_entry *old_like;
  like_entry new_like;

  /* Check the user's current like status */
  logdb("Searching for user %s likes\n", user);
  old_like = like_ll_get_user(like_list, user);
  if (old_like) {
    if (lts_greaterthan(like_lts, old_like->lts)) {
      logdb("Updating like status from %c to %c for user %s\n", old_like->action, action, user);
      old_like->lts = like_lts;
      old_like->action = action;
    }
  }
  else {
  
    /* Build the new like */
    logdb("New like occured at LTS (%d, %d)\n", like_lts.ts, like_lts.pid);
    
    strcpy(new_like.user, user);  
    strcpy(new_like.room, room);  
    new_like.action = action;
    new_like.lts = like_lts;

    /* insert it to the like list */
    like_ll_insert_inorder_fromback(like_list, new_like);
  }

  return TRUE;
}
