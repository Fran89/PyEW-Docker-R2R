/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sendfileII.h,v 1.3 2006/03/23 00:20:19 dietz Exp $
 *
 *    Revision history:
 *     $Log: sendfileII.h,v $
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
#include "my_log.h"
 
#define VERSION "sendfileII, V1.01"

#define SENDFILE_FAILURE -1
#define SENDFILE_SUCCESS  0
#define BUFLEN 4096          /* Write this many bytes at a time to socket */

void SocketSysInit( void );
void GetConfig( char * );
void LogConfig( char * );
int  chdir_ew( char * );
int  GetFileName( char [] );
int  ConnectToGetfile( void );
int  SendBlockToSocket( char *, int );
int  GetAckFromSocket( int );
void CloseSocketConnection( void );
void sleep_ew( unsigned );
int  getstr( char [], char [], int );
FILE *fopen_excl( const char *, const char * );
void TzSet( char *tz );


#endif
