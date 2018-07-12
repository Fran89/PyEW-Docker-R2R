/*=====================================================================
// Copyright (C) 2000,2001 Instrumental Software Technologies, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code, or portions of this source code,
//    must retain the above copyright notice, this list of conditions
//    and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgment:
//    "This product includes software developed by Instrumental
//    Software Technologies, Inc. (http://www.isti.com)"
// 4. If the software is provided with, or as part of a commercial
//    product, or is used in other commercial software products the
//    customer must be informed that "This product includes software
//    developed by Instrumental Software Technologies, Inc.
//    (http://www.isti.com)"
// 5. The names "Instrumental Software Technologies, Inc." and "ISTI"
//    must not be used to endorse or promote products derived from
//    this software without prior written permission. For written
//    permission, please contact "info@isti.com".
// 6. Products derived from this software may not be called "ISTI"
//    nor may "ISTI" appear in their names without prior written
//    permission of Instrumental Software Technologies, Inc.
// 7. Redistributions of any form whatsoever must retain the following
//    acknowledgment:
//    "This product includes software developed by Instrumental
//    Software Technologies, Inc. (http://www.isti.com/)."
// THIS SOFTWARE IS PROVIDED BY INSTRUMENTAL SOFTWARE
// TECHNOLOGIES, INC. "AS IS" AND ANY EXPRESSED OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL INSTRUMENTAL SOFTWARE TECHNOLOGIES,
// INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//=====================================================================
//  A current version of the software can be found at
//                http://www.isti.com
//  Bug reports and comments should be directed to
//  Instrumental Software Technologies, Inc. at info@isti.com
//=====================================================================
// This work was funded by the IRIS Data Management Center
// http://www.iris.washington.edu
//===================================================================== 
*/

/* NOTICE: reviewed and modified 
 * Ilya Dricker ISTI 06/13/2012 */

/* NOTICE: this file contains the function wsAppendMenuNoSocketReconnect()
 * which is derived from the EarthWorm function wsAppendMenu() in the file
 * src/libsrc/util/ws_clientII.c. wsAppendMenuNoSocketReconnect() attempts
 * to update menu from the WaveServer without dropping the socket connection
 * to the WaveServer. Therefore, it is assumed that the socket connection
 * is active.
 * The other functions in ths file are just copies of the corresponding 
 * functions from ws_clientII.c and are included because they defined as 
 * internal static in the original file.
 * See my notes on this below.
 * Ilya Dricker ISTI 02/26/01
*/	

/* These are the notes derived from the top of ws_clientII.c IGD 02/26/01 */
/*************************************************************************/
/*
 *     Revision history of ws_clientII.c : 
 *     Revision 1.3  2000/09/17 18:37:57  lombard
 *     wsSearchSCN now really returns the first menu with matching scn.
 *     wsGetTraceBin will now try another server if the first returns a gap.
 *
 *     Revision 1.2  2000/03/08 18:14:06  luetgert
 *     fixed memmove problem in wsParseAsciiHeaderReply.
 *
 *     Revision 1.1  2000/02/14 18:51:48  lucky
 *     Initial revision
 *
 *
 */

/* Version II of WaveServerIV client utility routines. This file contains
 * various routines useful for dealing with WaveServerIV and beyond. */

/* 5/17/98: Fixed bug in wsWaitAscii: it would fill reply buffer but never
 * report overflow. PNL */

/* The philosophy behind the version II changes is:
 * - Leave sockets open unless there is an error on the socket; calling 
 *   progam should ensure all sockets are closed before it quits.
 * - Routines are thread-safe. However, the calling thread must ensure its
 *   socket descriptors are unique or mutexed so that requests and replies
 *   can't overlap.
 * - Routines are layered on top of Dave Kragness's socket wrappers and thus
 *   do no timing themselves. Exceptions to this are wsWaitAscii and 
 *   wsWaitBinHeader which need special timing of recv().
 * Pete Lombard, University of Washington Geophysics; 1/4/98
 */
/*************************************************************************/

/*
**   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
**   CHECKED IT OUT USING THE COMMAND CHECKOUT.
**
**    $Id: isti_ws_client.c 4857 2012-06-19 18:01:15Z ilya $
** 
**	Revision history:
**	$Log$
**	Revision 1.2  2010/04/28 13:48:49  ilya
**	Update to sync with IRIS DMC version
**
**	Revision 1.5  2010/04/22 17:29:33  ilya
**	Fixed the code for EW below 7.0
**	
**	Revision 1.4  2007/09/17 16:29:48  ilya
**	Fixing problems with 78.1 EW integration
**	
**	Revision 1.3  2007/04/12 19:21:04  hal
**	Added ew7.0+ support (-dSUPPORT_SCNL) and linux port (-D_LINUX)
**	
**	-Hal
**	
**	Revision 1.2  2002/04/02 14:42:01  ilya
**	Version 02-Apr-2002: catchup algorithm
**	
**	Revision 1.1.1.1  2002/01/24 18:32:05  ilya
**	Exporting only ew2mseed!
**	
**	Revision 1.1.1.1  2001/11/20 21:47:00  ilya
**	First CVS commit
**	
 * Revision 1.4  2001/03/15  16:38:40  comserv
 * Added an extern function declaration to supress compilation warning
 *
 * Revision 1.3  2001/02/28  16:33:32  comserv
 * 	Modified leading comments for
 * 	 int wsAppendMenuNoSocketReconnect()
 *
 * Revision 1.2  2001/02/27  23:00:24  comserv
 * a function wsAppendMenuNoSocketReconnect() is created by strong
 * 	modifications of wsAppendMenu()
 *
 * Revision 1.1  2001/02/23  21:55:15  comserv
 * Initial revision
 *
*/

/* Standard includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h> 

/* Includes from Earthworm distribution */
#include "earthworm.h"
#include "trace_buf.h"
#include <socket_ew.h>
#include <time_ew.h>
#include "ew2mseed.h"

#define WS_MAX_RECV_BUF_LEN 4096

extern int WS_CL_DEBUG;
static int menu_reqid = -1;   


/* Protoypes for internal functions */
/*
 * The functions below are the copies of the internal static functions
 * from Earthwom distribution v5.1 and are derived from the file
 *	~/v5.1/src/libsrc/util/ws_clientII.c
 * The reason of inclusion these functions in this source file is
 * that they all defined as static in ws_clientII.c and therefore are
 * unreachable from any other file except ws_clientII.c
 * It will be necessary to update these functions in the future to
 * keep them compatile with the future vesions of the EarthWorm 
 *	Ilya Dricker ISTI 02/26/01 
*/
static int  wsWaitAscii( WS_MENU, char*, int, int );
static int  wsParseMenuReply( WS_MENU, char* );
static void wsSkipN( char*, int, int* );
extern Time_ew adjustTimeoutLength(int);
extern struct timeval FAR * resetTimeout(struct timeval FAR *);
 
/**************************************************************************
 *      int wsAppendMenuNoSocketReconnect()                               *
 *       updates menu->pcsn only and assumes that the menu->sock          *
 *	is VALID!. Non-regardless of the success of failure a pointer     *
 * 	(*old_menu) is not changed. Also the call guarantees that         *
 *	menu->addr, menu->port, menu>sock and menu->next would not        *
 *	change thier values. If the call is succesfull the linked         *  
 *	list menu->pscn is updated from the waveserver. If the call       *
 *	fails, menu->pcsn is not changed.  The function cleans after      *   
 *	itself, so there is no need for garbage collection in the         * 
 *	calling function.                                                 *
 *	Notice that the menu_requid values here are negative and          * 
 *	ascending to distinguish them from those generated in             *
 *	ws_clientII.c                                                     *
 *	Ilya Dricker (i.dricker@isti.com) ISTI 02/27/01                   *  
 **************************************************************************/

int wsAppendMenuNoSocketReconnect( WS_MENU_REC * old_menu, int verbosity, 
		  int timeout )
     /*
Arguments:
 WS_MENU_REC *:  a pointre to a VALID menu to be updated
     verbosity:  should be 5 to switch on logging
       timeout:  timeout interval in milliseconds,
        return:  WS_ERR_NONE:  if all went well.
		 WS_ERR_BROKEN_CONNECTION:  if the connection broke.
		 WS_ERR_TIMEOUT: if a timeout occured.
                 WS_ERR_MEMORY: if out of memory.
                 WS_ERR_INPUT: if bad input parameters.
		 WS_ERR_PARSE: if parser failed.
		 WS_ERR_EMPY_MENU: i menu is empty
*/
{
  int ret, len, err;
  WS_MENU new_menu = NULL;
  char request[wsREQLEN];
  char* reply = NULL;
  
  if (verbosity == 5)
    WS_CL_DEBUG = 1;

 
  new_menu = ( WS_MENU_REC* )calloc(sizeof(WS_MENU_REC),1);
  reply = ( char* )malloc( wsREPLEN );
  if ( !new_menu || !reply )
    {
      ret = WS_ERR_MEMORY;
      if (WS_CL_DEBUG) logit("e", "wsAppendMenuNoSocketReconnect: memory allocation error\n");
      goto Abort;
    }

  strcpy( new_menu->addr, old_menu->addr );
  strcpy( new_menu->port, old_menu->port );
  new_menu->next = old_menu->next;
  new_menu->sock = old_menu->sock;

#ifdef SUPPORT_SCNL
  sprintf( request, "MENU: %d SCNL\n", menu_reqid-- );
#else
  sprintf( request, "MENU: %d \n", menu_reqid-- );
#endif
  len = strlen(request);
  if ( ( ret =  send_ew( new_menu->sock, request, len, 0, timeout ) ) != len ) {
    if (ret < 0 )
    {
      if (WS_CL_DEBUG) logit("e", "wsAppendMenuNoSocketReconnect: connection broke to server %s:%s\n",
            new_menu->addr, new_menu->port);
      ret = WS_ERR_BROKEN_CONNECTION;
    } else {
     if (WS_CL_DEBUG) logit("e", "wsAppendMenuNoSocketReconnect: server %s:%s timed out\n",
			      new_menu->addr,  new_menu->port);
      ret = WS_ERR_TIMEOUT;
    }
    goto Abort;
  }

  ret = wsWaitAscii( new_menu, reply, wsREPLEN, timeout );
  if ( ret != WS_ERR_NONE && ret != WS_ERR_BUFFER_OVERFLOW )
    /* we might have received some useful data in spite of the overflow */
    {
      goto Abort;
    }

  if ( ( err = wsParseMenuReply( new_menu, reply ) ) != WS_ERR_NONE )
    {
      if ( ret == WS_ERR_BUFFER_OVERFLOW && err == WS_ERR_PARSE )
	{
	  if (WS_CL_DEBUG) logit("e", "wsAppendMenuNoSocketReconnect: buffer overflow; parse failure\n");
	} 
      else
	{
	  ret = err;
	}
      goto Abort;
    }

#ifdef SUPPORT_SCNL
  if ( new_menu->pscnl == NULL )
    {
      if (WS_CL_DEBUG) logit("e", "wsAppendMenuNoSocketReconnect: no SCNL at server %s:%s\n",
			     new_menu->addr, new_menu->port);
#else
  if ( new_menu->pscn == NULL )
    {
      if (WS_CL_DEBUG) logit("e", "wsAppendMenuNoSocketReconnect: no SCN at server %s:%s\n",
			     new_menu->addr, new_menu->port);
#endif   
      ret = WS_ERR_EMPTY_MENU;
      goto Abort;
    }
  
  /* Add new pscn to the old_menu */
#ifdef SUPPORT_SCNL
  wsKillPSCNL(old_menu->pscnl);
  old_menu->pscnl = new_menu->pscnl;
#else
  wsKillPSCN(old_menu->pscn);
  old_menu->pscn = new_menu->pscn;
#endif

  free ( new_menu );		 
  if ( reply ) free( reply );
  return WS_ERR_NONE;

Abort:
  /* An error occured, so clean up the mess */
  if ( reply ) free( reply );
  if ( new_menu )
    {
 #ifdef SUPPORT_SCNL
      wsKillPSCNL( new_menu->pscnl );
#else
	wsKillPSCN( new_menu->pscn );
#endif
      free( new_menu );
    }
  return ret;
}

/*******************************************************************
 * wsWaitAscii: Retrieve an ASCII message.                         *
 * The message will be terminated by a newline; nothing else is    *
 * expected after the newline, so we can read several characters   *
 * at a time without fear of reading past the newline.             *
 * This message may have internal nulls which will be converted to *
 * spaces.                                                         *
 * Returns after newline is read, when timeout expires if set,     *
 * or on error.                                                    *
 *******************************************************************/
static int wsWaitAscii( WS_MENU menu, char* buf, int buflen, int timeout_msec )
     /*
Arguments:
           menu: menu of server from which message is received
	    buf: buffer in which to place the message, terminated by null.
         buflen: number of bytes in the buffer.
   timeout_msec: timout interval in milliseconds. 
         return: WS_ERR_NONE: all went well.
	         WS_ERR_BUFFER_OVERFLOW: ran out of space before the message
                  end; calling program will have to decide how serious this is.
	         WS_ERR_INPUT: missing input parameters.
		 WS_ERR_SOCKET: error setting socket options.
		 WS_ERR_TIMEOUT: time expired before we read everything.
		 WS_ERR_BROKEN_CONNECTION: if the connection failed.
		 */
{
  int ii, ir = 0;  /* character counters */
  int nr = 0;
  char c = '\0';
  int len = 0;
  int ret, ioctl_ret;
  fd_set ReadableSockets;
  Time_ew StartTime;
  struct timeval SelectTimeout;
  Time_ew timeout = adjustTimeoutLength(timeout_msec);
  long lOnOff;
  
  if ( !buf || buflen <= 0 )
    {
      if (WS_CL_DEBUG) logit( "e", "wsWaitAscii(): no input buffer\n");
      return WS_ERR_INPUT;
    }

  /* If there is no timeout, make the socket blocking */
  if (timeout_msec == -1)
    {
      timeout = 0;
      lOnOff = 0;
      ioctl_ret = ioctlsocket(menu->sock, FIONBIO, &lOnOff);
      if (ioctl_ret == SOCKET_ERROR)
	{
	  ret = WS_ERR_SOCKET;
	  if (WS_CL_DEBUG) logit("et",
		"wsWaitAscii: error %s occurred during change to blocking\n",
				 socketGetError_ew());
	goto Done;
	}
    }

  StartTime = GetTime_ew(); /* the timer starts here */
  while ( c != '\n' )
    {
      if ((timeout) && (GetTime_ew() - timeout) > StartTime ) 
	{
	  ret = WS_ERR_TIMEOUT;
		  if (WS_CL_DEBUG) logit("et", "wsWaitAscii timed out\n");
	  goto Done;
	}
      if ( ir >= buflen - 2 )
	{
	  if (WS_CL_DEBUG)
	    logit( "e", "wsWaitAscii(): reply buffer overflows\n" );
	  ret = WS_ERR_BUFFER_OVERFLOW;
	  goto Done;
	}
      len = WS_MAX_RECV_BUF_LEN;
      if ( ir + len >= buflen - 1 )
	len = buflen - ir - 2; /* leave room for the terminating null */
      nr = recv( menu->sock, &buf[ir], len, 0 );
      if ( nr == -1 && socketGetError_ew() == WOULDBLOCK_EW ) 
	{
	  FD_ZERO( &ReadableSockets );
	  FD_SET( menu->sock, &ReadableSockets );
	  while (( !select(menu->sock + 1, &ReadableSockets, 0, 0,
			   resetTimeout( &SelectTimeout)))) 
	    {
	      if ((timeout) && (GetTime_ew() - timeout) > StartTime ) 
		{
		  ret = WS_ERR_TIMEOUT;
		  if (WS_CL_DEBUG) logit("et", "wsWaitAscii timed out\n");
		  goto Done;
		}
	      FD_ZERO( &ReadableSockets );
	      FD_SET( menu->sock, &ReadableSockets );
	      sleep_ew(100); /* wait a little and try again */
	    }
	  /* poll() says won't block */
	  nr = recv( menu->sock, &buf[ir], len, 0 ); 
	}
      
      if ( nr == -1 || nr > len )
	{
	  /* trouble reading socket        */
	  ret = WS_ERR_BROKEN_CONNECTION;
	  if (WS_CL_DEBUG) logit( "e", "wsWaitAscii(): Error on socket recv()\n" );
	  goto Done;
	}
      if ( nr > 0 )
	{
	  ii = 0;
	  /* got something, adjust ir and c  */
	  ir += nr;
	  c = buf[ir-1];
	  
	  /* replace NULL char in ascii string with SPACE char */
	  while ( ii < nr ) 
	    {
	      if ( !buf[ir-nr+ii] ) buf[ir-nr+ii] = ' ';
	      ++ii;
	    }
	}
    }
  
  ret = WS_ERR_NONE;
Done:
  buf[ir] = '\0';                 /* null-terminate the reply      */
  /* If there was no timeout, then change the socket back to non-blocking */
  if (timeout_msec == -1) 
    {
      lOnOff = 1;
      ioctl_ret = ioctlsocket( menu->sock, FIONBIO, &lOnOff);
      if (ioctl_ret == SOCKET_ERROR) 
	{
	  
	  if (WS_CL_DEBUG) logit("et", "wsWaitAScii: error %s occurred during change to non-blocking\n", 
				 socketGetError_ew() );
	  ret = WS_ERR_SOCKET;
	}
    }
  return ret;
}

/***********************************************************************
 * wsParseMenuReply: parse the reply we got from the waveserver into   *
 * a menu list. Handles replies to MENU, MENUPIN and MENUSCN requests. *
 ***********************************************************************/
static int wsParseMenuReply( WS_MENU menu, char* reply )
{
  /* Arguments:
   *       menu: pointer to menu structure to be allocated and filled in.
   *      reply: pointer to reply to be parsed.
   *   Returns: WS_ERR_NONE:  if all went well
   *            WS_ERR_INPUT: if bad input parameters
   *            WS_ERR_PARSE: if we couldn't parse the reply
   *            WS_ERR_MEMORY: if out of memory
   */
  int reqid = 0;
  int pinno = 0;
  /* allow extra room in these input buffer fields,  so that we are unlikely to clobber
     memory if the data coming in is larger than the spec'd definition. */
  char    sta[TRACE2_STA_LEN*3];         /* Site name */
  char    chan[TRACE2_CHAN_LEN*3];       /* Component/channel code */
  char    net[TRACE2_NET_LEN*3];         /* Network name */
  char    loc[TRACE2_LOC_LEN*3];         /* Location code */
  double tankStarttime = 0.0, tankEndtime = 0.0;
  char datatype[3];
  int scn_pos = 0;
  int skip = 0;

  if ( !reply || !menu )
  {
    if (WS_CL_DEBUG) logit("e", "wsParseMenuReply: WS_ERR_INPUT\n");
    return WS_ERR_INPUT;
  }

  if ( sscanf( &reply[scn_pos], "%d", &reqid ) < 1 )
  {
    if (WS_CL_DEBUG)
      logit( "e","wsParseMenuReply(): error parsing reqid\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 1, &scn_pos );
  while ( reply[scn_pos] && reply[scn_pos] != '\n' )
  {
#ifdef SUPPORT_SCNL
    WS_PSCNL pscn = NULL;
    skip = 8;
#else
    WS_PSCN pscn = NULL;
    skip = 7;
#endif
#ifdef SUPPORT_SCNL
    if ( sscanf( &reply[scn_pos], "%d %s %s %s %s %lf %lf %s",
                   &pinno, sta, chan, net, loc,
                   &tankStarttime, &tankEndtime, datatype ) < 8 )
#else
    if ( sscanf( &reply[scn_pos], "%d %s %s %s %lf %lf %s",
                  &pinno, sta, chan, net, 
                  &tankStarttime, &tankEndtime, datatype ) < 7 )
#endif
    {
      if (WS_CL_DEBUG)
        logit( "e","wsParseMenuReply(): error decoding reply<%s>\n", &reply[scn_pos] );
      return WS_ERR_PARSE;
    }
#ifdef SUPPORT_SCNL
    pscn = ( WS_PSCNL_REC* )calloc(sizeof(WS_PSCNL_REC),1);
#else
    pscn = ( WS_PSCN_REC* )calloc(sizeof(WS_PSCN_REC),1);
#endif
    if ( !pscn )
    {
      if (WS_CL_DEBUG)
        logit("e", "wsParseMenuReply(): error allocating memory\n");
      return WS_ERR_MEMORY;
    }

#ifdef SUPPORT_SCNL
    pscn->next = menu->pscnl;
#else
    pscn->next = menu->pscn;
#endif
    pscn->pinno = pinno;
    
   /* truncate the SCNL input buffers at the spec'd Tracebuf2 length */
   sta[TRACE2_STA_LEN-1]=0x00;
   chan[TRACE2_CHAN_LEN-1]=0x00;
   net[TRACE2_NET_LEN-1]=0x00;
#ifdef SUPPORT_SCNL
   loc[TRACE2_LOC_LEN-1]=0x00;
#endif

   strcpy( pscn->sta, sta );
   strcpy( pscn->chan, chan );
   strcpy( pscn->net, net );
   pscn->tankStarttime = tankStarttime;
   pscn->tankEndtime = tankEndtime;
#ifdef SUPPORT_SCNL
   strcpy( pscn->loc, loc );
   menu->pscnl = pscn;
#else
   menu->pscn = pscn;
#endif
   wsSkipN( reply, skip, &scn_pos );
  }
  return WS_ERR_NONE;
}

/**************************************************************************
 *      wsSkipN: moves forward the pointer *posp in buf by moving forward *
 *      cnt words.  Words are delimited by either space or horizontal     *
 *      tabs; newline marks the end of the buffer.                        *
 **************************************************************************/
static void wsSkipN( char* buf, int cnt, int* posp )
{
  int pos = *posp;

  while ( cnt )
    {
      while ( buf[pos] != ' ' && buf[pos] != '\t' )
	{
	  if ( !buf[pos] )
	    {
	      goto done;
	    }
	  if ( buf[pos] == '\n' )
	    {
	      ++pos;
	      goto done;
	    }
	  ++pos;
	}
      --cnt;
      while ( buf[pos] == ' ' || buf[pos] == '\t' )
	{
	  ++pos;
	}
    }
done:
  *posp = pos;
}

 
