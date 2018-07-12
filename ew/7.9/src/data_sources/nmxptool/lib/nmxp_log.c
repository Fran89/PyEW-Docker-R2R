/*! \file
 *
 * \brief Log for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_log.c 4165 2011-01-24 15:19:28Z quintiliani $
 *
 */

#include "nmxp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <time.h>

#include "config.h"

#define MAX_LOG_MESSAGE_LENGTH 8000

#define LIBRARY_NAME "libnmxp"
#define NMXP_LOG_PREFIX "nmxp"

static char nmxp_log_prefix[MAX_LOG_MESSAGE_LENGTH] = NMXP_LOG_PREFIX;

void nmxp_log_set_prefix(char *prefix) {
    if(prefix) {
	snprintf(nmxp_log_prefix, MAX_LOG_MESSAGE_LENGTH, "%s [%s]", NMXP_LOG_PREFIX, NMXP_LOG_STR(prefix));
    }
}

void nmxp_log_get_prefix(char *ret, int size ) {
    strncpy(ret, nmxp_log_prefix,size);
}


const char *nmxp_log_version() {
    static char ret_str[MAX_LOG_MESSAGE_LENGTH] = "";
    snprintf(ret_str, MAX_LOG_MESSAGE_LENGTH, "%s-%s", NMXP_LOG_STR(LIBRARY_NAME), NMXP_LOG_STR(PACKAGE_VERSION));
    return ret_str;
}


int nmxp_log_stdout(char *msg) {
    int ret = fprintf(stdout, "%s", msg);
    fflush(stdout);
    return ret;
}

int nmxp_log_stderr(char *msg) {
    int ret = fprintf(stderr, "%s", msg);
    fflush(stderr);
    return ret;
}

#define NMXP_MAX_FUNC_LOG 10

/* private variables */
static int n_func_log = 0;
static int n_func_log_err = 0;
static int (*p_func_log[NMXP_MAX_FUNC_LOG]) (char *);
static int (*p_func_log_err[NMXP_MAX_FUNC_LOG]) (char *);


void nmxp_log_init(int (*func_log)(char *), int (*func_log_err)(char *)) {
    n_func_log = 0;
    n_func_log_err = 0;
    nmxp_log_add(func_log, func_log_err);
}


void nmxp_log_add(int (*func_log)(char *), int (*func_log_err)(char *)) {
    if(func_log != NULL) {
	p_func_log[n_func_log++] = func_log;
    }
    if(func_log_err != NULL) {
	p_func_log_err[n_func_log_err++] = func_log_err;
    }

}

void nmxp_log_rem(int (*func_log)(char *), int (*func_log_err)(char *)) {
    int i = 0;
    int j = 0;
    if(func_log != NULL) {
	i = 0;
	while(i < n_func_log  &&  p_func_log[i] != func_log) {
	    i++;
	}
	if(i < n_func_log) {
	    for(j=i; j < n_func_log-1; j++) {
		 p_func_log[j] = p_func_log[j+1];
	    }
	    n_func_log--;
	} else {
	    /* TODO not found */
	}
    }

    if(func_log_err != NULL) {
	i = 0;
	while(i < n_func_log_err  &&  p_func_log_err[i] != func_log_err) {
	    i++;
	}
	if(i < n_func_log_err) {
	    for(j=i; j < n_func_log_err-1; j++) {
		 p_func_log_err[j] = p_func_log_err[j+1];
	    }
	    n_func_log_err--;
	} else {
	    /* TODO not found */
	}
    }

}


/* Private function */
void nmxp_log_print_all(char *message, int (*a_func_log[NMXP_MAX_FUNC_LOG]) (char *), int a_n_func_log) {
    int i;
    if(a_n_func_log > 0) {
	for(i=0; i < a_n_func_log; i++) {
	    a_func_log[i](message);
	}
    } else {
	nmxp_log_stdout(message);
    }
}

#define MAX_SIZE_TIMESTR 100
int nmxp_log(int level, int verb, ... )
{
  static int staticverb = 0;
  int retvalue = 0;
  char message[MAX_LOG_MESSAGE_LENGTH];
  char message_final[MAX_LOG_MESSAGE_LENGTH];
  char prefix[MAX_LOG_MESSAGE_LENGTH];  
  char timestr[MAX_SIZE_TIMESTR];
  char *format;

  va_list listptr;
  time_t loc_time;

  if ( level == NMXP_LOG_SET ) {
    staticverb = verb;
    retvalue = staticverb;
  } else if ( (verb & staticverb)  ||  level == NMXP_LOG_ERR  ||  verb == NMXP_LOG_D_ANY ) {

    va_start(listptr, verb);
    format = va_arg(listptr, char *);

    /* Build local time string and cut off the newline */
    time(&loc_time);
    /*use reentrant ctime_r -D_POSIX_PTHREAD_SEMANTICS*/
    ctime_r(&loc_time,timestr);

    timestr[strlen(timestr) - 1] = '\0';
 
    retvalue = vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, format, listptr);
    nmxp_log_get_prefix(prefix,MAX_LOG_MESSAGE_LENGTH);
    switch(level) {
	case NMXP_LOG_ERR:
	    //snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s - %s error: %s", timestr, nmxp_log_get_prefix(), message);
	    snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s - %s error: %s", timestr, prefix, message);            
	    nmxp_log_print_all(message_final, p_func_log_err, n_func_log_err);
	    break;
	case NMXP_LOG_WARN:
	    //snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s - %s warning: %s", timestr, nmxp_log_get_prefix(), message);
	    snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s - %s warning: %s", timestr, prefix, message);            
	    nmxp_log_print_all(message_final, p_func_log, n_func_log);
	    break;
	case NMXP_LOG_NORM_NO:
	    snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s", message);
	    nmxp_log_print_all(message_final, p_func_log, n_func_log);
	    break;
	case NMXP_LOG_NORM_PKG:
	    //snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s: %s", nmxp_log_get_prefix(), message);
	    snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s: %s", prefix, message);            
	    nmxp_log_print_all(message_final, p_func_log, n_func_log);
	    break;
	default:
	    //snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s - %s: %s", timestr, nmxp_log_get_prefix(), message);
	    snprintf(message_final, MAX_LOG_MESSAGE_LENGTH, "%s - %s: %s", timestr, prefix, message);            
	    nmxp_log_print_all(message_final, p_func_log, n_func_log);
	    break;
    }

    va_end(listptr);
  }

  return retvalue;
} /* End of nmxp_log() */

