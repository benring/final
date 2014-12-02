#include "name_ll.h"
#include "stdio.h"
#include "stdlib.h"

/* Implemented as a set -- items in the list are unique */

name_ll name_ll_create()
{
  name_ll result;
  result.first = 0;
  result.last = 0;
  result.size = 0;
  return result;
}

int name_ll_is_empty(name_ll* list)
{
  if (list->first) {
    return 0;
  } else {
    return 1;
  }
}

void name_ll_print(name_ll* list)
{
  if (name_ll_is_empty(list)) {
    printf("Empty List\n");
  } else {
    name_ll_node* curr = list->first;
    printf(" List [%d]: ", list->size);
    while (curr) {
      printf(" %s", curr->data.name);
      curr = curr->next;
      if (curr) {
        printf(",");
      }
    }
    printf("\n");
  }
}


int name_ll_append(name_ll* list, char name[MAX_GROUP_NAME])
{
  name_ll_node *node = malloc(sizeof(name_ll_node));
  name_ll_node *old_last;

  if (!node) {
    return 0; /* failure */
  }
  
  strcpy(node->data.name, name);
  node->data.count = 1;
  node->prev = 0;
  node->next = 0;

  if(name_ll_is_empty(list)) {
    list->first = node;
    list->last = node;
  } else {
    old_last = list->last;
    old_last->next = node;
    node->prev = old_last;
    list->last = node;
  }
  list->size++;

  return 1; /* success */
}

/*  
 *  Return -1 if found & incremented
 *  Return 1 if not found & sucessfully appended
 *  Will return 0 if append fails
 */
int name_ll_insert(name_ll* list, char *name)
{
  name_ll_node* curr = list->first;
  if (name_ll_is_empty(list)) {
    return name_ll_append(list, name);
  }
  while(curr) {
    if (strcmp(curr->data.name, name) == 0) {
      curr->data.count++;
      return 0;
    }
    curr = curr->next;
  }
  return name_ll_append(list, name);
}

/*  
 *  Return -1 if not found
 *  Return 0 if found & removed
 *  Return 1 if found, count reduced, but name remains in set
 */
int name_ll_remove(name_ll* list, char *name)
{
  name_ll_node* curr = list->first;
  if (name_ll_is_empty(list)) {
    return -1;
  }

  name_ll_node* prev = 0;
  while(curr) {
    if (strcmp(curr->data.name, name) == 0) {
      
      if (curr->data.count > 1) {
        curr->data.count--;
        return 1;
      }
      
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

      list->size--;
      free(curr);
      return 0;
    }
    prev = curr;
    curr = curr->next;
  }
  return -1;
}



int name_ll_search(name_ll* list, char *name)
{
  name_ll_node* curr = list->first;
  if (name_ll_is_empty(list)) {
    return 0;
  }
  while(curr) {
    if (strcmp(curr->data.name, name) == 0) {
      return 1;
    }
    curr = curr->next;
  }
  return 0;
}



