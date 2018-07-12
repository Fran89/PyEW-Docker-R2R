/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqschassis.h 1396 2004-04-14 20:07:09Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/04/14 20:07:09  dietz
 *     modifications to support location code
 *
 *     Revision 1.2  2002/07/09 18:11:55  dietz
 *     logit changes
 *
 *     Revision 1.1  2002/03/15 23:10:09  dietz
 *     Initial revision
 *
 */

/*   naqschassis.h    */
 
#ifndef _NAQSCHASSIS_H
#define _NAQSCHASSIS_H

#include <time.h>
#include <trace_buf.h>
#include "nmx_api.h"

/* Reserve error numbers 0-49 for naqschassis */
#define ERR_NOCONNECT    0    /* Lost connect to NaqsServer             */
#define ERR_RECONNECT    1    /* Regained connection to NaqsServer      */

/* Function Prototypes 
 *********************/
/* naqschassis.c */
void naqschassis_error( short, char *, time_t );
void naqschassis_shutdown( int );

/* sockit.c */
int  ConnectToNaqs( char *ipadr, int port );
int  SendToNaqs( char *msg, int msglen );
int  RecvFromNaqs( NMXHDR *nhd, char **buf, int *buflen );
int  DisconnectFromNaqs( void );

/* naqsclient source files */
int  naqsclient_com( char *fname );  /* read client-specific commands */
int  naqsclient_init( void );        /* initialize client */
int  naqsclient_process( long rctype, NMXHDR *naqshd, 
                         char **pbuf, int *pbuflen );
void naqsclient_shutdown( void );

#endif
