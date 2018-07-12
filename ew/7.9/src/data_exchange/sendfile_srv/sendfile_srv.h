/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sendfile_srv.h 3347 2008-08-11 17:54:17Z luetgert $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2008/08/11 17:54:17  luetgert
 *     sendfile_srv is the server version of sendfileII.
 *     .
 *
 *     Revision 1.3  2006/03/23 00:20:19  dietz
 *     Changed version to 1.01 (due to SendPause addition)
 *
 *     Revision 1.2  2003/01/02 21:15:45  lombard
 *     Changed version to 1.0
 *
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */

/*   sendfileII.h    */
#ifndef SENDFILEII_H
#define SENDFILEII_H

#include <stdio.h>
 
#define VERSION "sendfile_srv, V1.00"

#define SENDFILE_FAILURE   -1
#define SENDFILE_SUCCESS    0
#define SENDFILE_DONE       1
#define BUFLEN           4096      /* Write this many bytes at a time to socket */
#define MAXPATHSTR        128      /* Max string length for paths   */

void GetConfig( char * );
void LogConfig( char * );

void SocketSysInit( void );
void InitServerSocket( void );
int AcceptConnection( void );
int  SendBlockToSocket( char *, int );
int  GetAckFromSocket( int );
void CloseSocketConnection( void );

void log_init( char *, char *, int, int );
void log( char *, char *, ... );
int  chdir_ew( char * );

int  GetFileName( char [] );
void sleep_ew( unsigned );
int  getstr( char [], char [], int );
FILE *fopen_excl( const char *, const char * );
void TzSet( char *tz );


#endif
