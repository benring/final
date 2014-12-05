/******************************************************************************
 * File:     config.c
 * Authors:  Benjamin Ring & Josh Wheeler
 * Date:     5 December 2014
 *
 * Description:  Common static values used in the chat service
 *
 *****************************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include "sp.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


/* Common values */
#define FALSE 0
#define TRUE 1
#define NAME_LEN 32
#define CHAT_LEN 80
#define MAX_CLIENTS 100
#define MAX_SERVERS 5
#define MAX_INT 2147483638

/*  Actions  */
#define JOIN 10
#define LEAVE 11
#define ADD_LIKE 'L'
#define REM_LIKE 'R'

/* Interface (for display) */
#define SHOW_HELP 1
#define SHOW_UI 0
#define NO_REFRESH -1

/*  #define	DEBUG 1  */
#ifdef  DEBUG
#define logdb(args...) printf(args);
#else
#define logdb(args...)
#endif

#define INFO 1 
#ifdef  INFO
#define loginfo(args...) printf(args);
#else
#define loginfo(args...)
#endif

#define ERR 1 
#ifdef  ERR
#define logerr(args...) printf(args);
#else
#define logerr(args...)
#endif

#endif