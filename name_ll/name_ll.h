/******************************************************************************
 * File:     name.h
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Name List data structure. A name is a string and this list
 *    is used to retain unique users in a chat room. As such, this implemention
 *    is a set where all entries are unique. Provides for create, search, 
 *    print, remove, and insert.
 *
 *****************************************************************************/
#ifndef NAME_LL_H
#define NAME_LL_H

#include "transaction.h"
#include "lts_utils.h"

typedef struct name_ll_node {
	char    data[MAX_GROUP_NAME];
  struct name_ll_node* prev;
  struct name_ll_node* next;
} name_ll_node;

typedef struct name_ll {
  name_ll_node* first;
  name_ll_node* last;
  int           size;
} name_ll;

/* Create an empty list */
name_ll name_ll_create();

/* Check if the list is empty */
int name_ll_is_empty(name_ll* list);

/* Print the contents of a list */
void name_ll_print(name_ll* list);

/* Append to the end of the list */
int name_ll_append(name_ll* list, char *name);

/* Insert into the list, if the name does not already exist */
int name_ll_insert(name_ll* list, char *name);

/* removes name from the list */
int name_ll_remove(name_ll* list, char *name);

/* Search the whole list for a name, return true if found, false otherwise */ 
int name_ll_search(name_ll* list, char *name);

#endif
