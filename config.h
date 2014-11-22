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
#define NUM_SERVERS 5
#define NAME_LEN 20
#define CHAT_LEN 80

#define	DEBUG 1
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