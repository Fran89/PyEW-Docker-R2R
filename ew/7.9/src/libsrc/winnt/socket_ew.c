
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: socket_ew.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/02/04 17:58:24  davidk
 *     Added a new function socketSetError_ew() that calls WSASetLastError() to set
 *     the error for the most recent socket call.
 *
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

            /**********************************************************
             *               socket_ew.c for Windows NT               *
             *                                                        *
             *  Contains system-dependent functions for dealing with  *
             *  sockets.                                              *
             **********************************************************/

#include <stdlib.h>
#include <windows.h>  /* Includes winsock(2).h 
                          For Socket stuff */
#include <earthworm.h>
#include <socket_ew.h>

int SOCKET_SYS_INIT = 0;   /* Global initialization flag,
                              set in SocketSysInit(), 
                              checked in socket_ew()  */

/********************** SocketSysInit ********************
 *              Initialize the socket system             *
 *         We are using Windows socket version 2.2.      *
 *********************************************************/

void SocketSysInit( void )
{
   int     status;
   WSADATA Data;

   status = WSAStartup( MAKEWORD(2,2), &Data );
   if ( status != 0 )
   {
      logit( "et", "WSAStartup failed. Exitting.\n" );
      exit( -1 );
   }
   SOCKET_SYS_INIT++;

   logit( "t", "Socket version (wHighVersion): %x\n", Data.wHighVersion );
   return;
}


/********************** SocketClose **********************
 *                    Close a Socket                     *
 *********************************************************/

void SocketClose( SOCKET soko )
{
   if ( closesocket( soko ) != 0 )
      SocketPerror( "SocketClose()" );
   return;
}


/********************** SocketPerror *********************
 *                Print an error message                 *
 *********************************************************/

void SocketPerror( char *note )
{
   logit( "et", "%s  Error %d encountered.\n", note, socketGetError_ew() );
   return;
}


/************************ sendall() ***********************
*       looks like the standard send(), but does not      *
*       return until either error, or all has been sent   *
*	Also, we break sends into lumps as some           *
*	implementations can't send too much at once.      *
*	Will found this out.
***********************************************************/

#define SENDALL_MAX_LUMP 1024	/* no send() larger  than this */

int sendall(SOCKET socket, const char *msg, long msgLen, int flags)
{
	int   ret;  /* number of bytes actually sent, or error */
	long  nextByte;
	int   nsend;

	nsend = SENDALL_MAX_LUMP; /* try sending in lumps of this size */
	nextByte = 0;

	while ( nextByte<msgLen )
		{
		if ( msgLen-nextByte < nsend ) nsend = msgLen-nextByte; /* last small send? */
		ret = send(socket, (const char*)&msg[nextByte], nsend, flags);
		if (ret < 0)
			{
			logit("t","send error %d\n",ret);
			return( ret );
			}
		nextByte += ret;  /* we actually sent only this many */
		}
	return ( msgLen );
}


/****************** socketGetError_ew *********************
     Returns the error code for the most recent socket error.
**********************************************************/
int socketGetError_ew()
{
  int bob=(int)WSAGetLastError();
  return((int)WSAGetLastError());
}


/********************** socketSetError_ew *****************
     Sets the error code for the most recent socket error.
**********************************************************/
void socketSetError_ew(int error)
{
  WSASetLastError(error);
}


Time_ew GetTime_ew()
{
  FILETIME CurrentTime;
  Time_ew tewCurrentTime;
  DWORD * CTptr;

  GetSystemTimeAsFileTime(&CurrentTime);
  /* I saw some wierd things with 64 bit integers, so I'm kind of
     scared to cast this, since I know it works right now.  DK */
/*  CTptr=&tewCurrentTime;                     */
/*  *CTptr=CurrentTime.dwLowDateTime;          */
/*  CTptr=(unsigned long *)(((int)CTptr) + 4); */
/*  *CTptr=CurrentTime.dwHighDateTime;         */

  /* Changed 'int' to 'size_t' for 64-bit-build compatibility
     and added DWORD* casts to squelch warnings -- ET 2016-05-19 */
  CTptr = (DWORD *)&tewCurrentTime;
  *CTptr = CurrentTime.dwLowDateTime;
  CTptr = (DWORD *)(((size_t)CTptr) + 4);
  *CTptr = CurrentTime.dwHighDateTime;

  return(tewCurrentTime);
}


Time_ew adjustTimeoutLength(int timeout_msec)
{
  return(((Time_ew)timeout_msec) * 10000); /* Convert miliseconds to
                                              100-nanoseconds */
}
 

