/*! \file
 *
 * \brief Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxptool_sigcondition.c 3898 2010-03-25 13:25:46Z quintiliani $
 *
 */

#include "config.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#else
#warning Management of 'sigcondition' is not thread safe. Anyway, it is not so bad presently.
#endif

static int sigcondition = 0;

/* Safe Thread Synchronization for sigcondition if defined HAVE_PTHREAD_H */
#ifdef HAVE_PTHREAD_H
static pthread_mutex_t mutexsig = PTHREAD_MUTEX_INITIALIZER;
#endif


void nmxptool_sigcondition_init() {
#ifdef HAVE_PTHREAD_H
    pthread_mutex_init(&mutexsig, NULL);
#endif
}


void nmxptool_sigocondition_destroy() {
#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&mutexsig);
#endif
}


int nmxptool_sigcondition_read() {
    int ret = 0;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock (&mutexsig);
#endif
    ret = sigcondition;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock (&mutexsig);
#endif
    return ret;
}


void nmxptool_sigcondition_write(int new_sig) {
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock (&mutexsig);
#endif
    sigcondition = new_sig;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock (&mutexsig);
#endif
}


