
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ws_clientII.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2003/01/31 16:08:20  patton
 *     Initial revision
 *
 *     Revision 1.5  2002/03/19 22:15:15  davidk
 *     Fixed bug reported by IlyaDricker in wsGetTraceBin() where the function
 *     was not correctly handling the return code from wsParseBinHeaderReply().
 *
 *     Revision 1.4  2001/04/17 17:31:41  davidk
 *     Added explicit (SOCKET) typecasts in FD_SET to get rid of compiler warnings on NT.
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <earthworm.h>
#include <socket_ew.h>
#include <time_ew.h>

#include "ws_clientII.h"

#define WS_MAX_RECV_BUF_LEN 4096

static int menu_reqid = 0;
int WS_CL_DEBUG = 0;

extern int recv_all (SOCKET, char FAR*,int,int,int);


/* Protoypes for internal functions */
static int  wsWaitAscii( WS_MENU, char*, int, int, FILE * );
static int  wsWaitBinHeader( WS_MENU, char*, int, int, FILE * );
static int  wsParseMenuReply( WS_MENU, char*, FILE * );
static int  wsParseBinHeaderReply( TRACE_REQ*, char*, FILE * );
static int  wsParseAsciiHeaderReply( TRACE_REQ*, char*, FILE * );
static void wsSkipN( char*, int, int*, FILE * );
struct timeval FAR * resetTimeout(struct timeval FAR *);
Time_ew adjustTimeoutLength(int timeout_msec);

/**************************************************************************
 *      wsAppendMenu: builds a combined menu from many waveservers.       *
 *      Called with the address and port of one waveserver.               *
 *      On the first call it creates a linked list to store the 'menu'    *
 *      reply from the indicated waveserver.                              *
 *      On subsequent calls, it appends the menu replies to the list.     *
 **************************************************************************/

int wsAppendMenu( char* ipAdr, char* port, WS_MENU_QUEUE_REC* menu_queue, 
		  int timeout, FILE *out_file )
     /*
Arguments:
         ipAdr:  is the dot form of the IP address as a char string.
          port:  TCP port number as a char string.
    menu_queue:  caller-supplied pointer to the list of menus.
       timeout:  timeout interval in milliseconds,
        return:  WS_ERR_NONE:  if all went well.
                 WS_ERR_NO_CONNECTION: if we could not get a connection.
                 WS_ERR_SOCKET: if we could not create a socket.
		 WS_ERR_BROKEN_CONNECTION:  if the connection broke.
		 WS_ERR_TIMEOUT: if a timeout occured.
                 WS_ERR_MEMORY: if out of memory.
                 WS_ERR_INPUT: if bad input parameters.
		 WS_ERR_PARSE: if parser failed.
*/
{
  int ret, len, err;
  WS_MENU menu = NULL;
  char request[wsREQLEN];
  char* reply = NULL;

  fprintf(out_file, "wsAppendMenu:\n");

  if ( !ipAdr || !port ||
       (strlen(ipAdr) >= wsADRLEN) || (strlen(port) >= wsADRLEN) )
    {
      ret = WS_ERR_INPUT;
      if (WS_CL_DEBUG) fprintf(out_file,"   wsAppendMenu: bad address: %s port: %s\n",
			     ipAdr, port);
      goto Abort;
    }

  fprintf(out_file, "   ipAdr: %s  port: %s  timeout: %d\n\n", ipAdr, port, timeout);


  menu = ( WS_MENU_REC* )calloc(sizeof(WS_MENU_REC),1);
  reply = ( char* )malloc( wsREPLEN );
  if ( !menu || !reply )
    {
      ret = WS_ERR_MEMORY;
      if (WS_CL_DEBUG) fprintf(out_file, "   wsAppendMenu: memory allocation error\n");
      goto Abort;
    }

  strcpy( menu->addr, ipAdr );
  strcpy( menu->port, port );
  if ( wsAttachServer( menu, timeout, out_file ) != WS_ERR_NONE )
    {
      ret = WS_ERR_NO_CONNECTION;
      goto Abort;
    }

  sprintf( request, "MENU: %d \n", menu_reqid++ );
  len = (int)strlen(request);
  if ( ( ret =  send_ew( menu->sock, request, len, 0, timeout ) ) != len ) {
    if (ret < 0 )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "   wsAppendMenu: connection broke to server %s:%s\n",
			     ipAdr, port);
      ret = WS_ERR_BROKEN_CONNECTION;
    } else {
      if (WS_CL_DEBUG) fprintf(out_file, "   wsAppendMenu: server %s:%s timed out\n",
			     ipAdr, port);
      ret = WS_ERR_TIMEOUT;
    }
    goto Abort;
  }
  fprintf(out_file, "   Request: %s\n", request);

  ret = wsWaitAscii( menu, reply, wsREPLEN, timeout, out_file );
  if ( ret != WS_ERR_NONE && ret != WS_ERR_BUFFER_OVERFLOW )
    /* we might have received some useful data in spite of the overflow */
    {
      goto Abort;
    }

  if ( ( err = wsParseMenuReply( menu, reply, out_file ) ) != WS_ERR_NONE )
    {
      if ( ret == WS_ERR_BUFFER_OVERFLOW && err == WS_ERR_PARSE )
	{
	  if (WS_CL_DEBUG) fprintf(out_file, "wsAppendMenu: buffer overflow; parse failure\n");
	} 
      else
	{
	  ret = err;
	}
      goto Abort;
    }

  if ( menu->pscn == NULL )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsAppendMenu: no SCN at server %s:%s\n",
			     menu->addr, menu->port);
      ret = WS_ERR_EMPTY_MENU;
      goto Abort;
    }
  
  /* Add the menu to the possibly empty menu queue */
  if ( !menu_queue->head )
    {
      menu_queue->head = menu_queue->tail = menu;
    }
  else
    {
      menu_queue->tail->next = menu;
      menu_queue->tail = menu;
    }
  if ( reply ) free( reply );
  fprintf(out_file, "End of wsAppendMenu\n");
  return WS_ERR_NONE;

Abort:
  /* An error occured, so clean up the mess */
  wsDetachServer( menu, out_file );
  if ( reply ) free( reply );
  if ( menu )
    {
      wsKillPSCN( menu->pscn, out_file );
      free( menu );
    }
  return ret;
}


/************************************************************************** 
 * wsKillMenu: Gracefully closes all the server sockets and releases the  *
 *             linked list of menus created by wsAppendMenu               *
 **************************************************************************/
void wsKillMenu( WS_MENU_QUEUE_REC* menu_queue, FILE *out_file )
     /*
Arguments:
    menu_queue:  caller-supplied pointer to the list of menus.
    */
{
  WS_MENU menu = menu_queue->head;

  fprintf(out_file, "wsKillMenu:\n");
  while ( menu )
    {
      WS_MENU next = menu->next;

      if ( menu->sock != -1 )
	closesocket_ew( menu->sock, SOCKET_CLOSE_GRACEFULLY_EW );
      wsKillPSCN( menu->pscn, out_file );
      free( menu );
      menu = next;
    }
  menu_queue->head = menu_queue->tail = NULL;
  menu_reqid = 0;
  fprintf(out_file, "   All sockets closed, linked list released\n\n");

  fprintf(out_file, "End of wsKillMenu\n");
  return;
}


/*********************************************************************
 * wsGetTraceBin: retrieves the piece of raw trace data specified in *
 * the structure 'getThis': The current menu list, as built by the   *
 * routines above will be searched for a matching SCN. If a match    *
 * is found, the associated wave server will be contacted, and a     *
 * request for the trace snippet will be made.                       *
 *********************************************************************/
int wsGetTraceBin( TRACE_REQ* getThis, WS_MENU_QUEUE_REC* menu_queue,
		   int timeout, FILE *out_file )
     /*
Arguments:
        getThis:   a TRACE_REQ structure (see ws_client.h), with the
                   request portion filled in.
     menu_queue:   pointer to the list of server menus.
        timeout:   Time in milliseconds to wait for reply
         return:   WS_ERR_NONE: all went well.
                   WS_WRN_FLAGGED: wave server returned error flag instead
                      of trace data.
                   WS_ERR_EMPTY_MENU: No menu list found.
                   WS_ERR_SCN_NOT_IN_MENU: SCN not found in menu list.
		   WS_ERR_NO_CONNECTION: if socket was already closed
	  The socket will be closed for the following:
                   WS_ERR_BUFFER_OVERFLOW: trace buffer supplied too small.
		   WS_ERR_TIMEOUT: if a timeout occured
		   WS_ERR_BROKEN_CONNECTION: if a connection broke.
		   WS_ERR_SOCKET: problem changing socket options.
     */
{
  int ret, len, err = WS_ERR_NONE;
  WS_MENU menu = NULL;
  WS_PSCN pscn = NULL;
  char request[wsREQLEN];

  fprintf(out_file, "wsGetTraceBin:\n\n");
  if ( menu_queue->head == NULL )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsGetTraceBin: empty menu\n");
      return WS_ERR_EMPTY_MENU;
    }

 next_menu:
  if ( ( ret = wsSearchSCN( getThis, &menu, &pscn, menu_queue, out_file)) != WS_ERR_NONE )
    {
      if (err != WS_ERR_NONE)
        return err;
      else
        return ret;  /* WS_ERR_SCN_NOT_IN_MENU */
    }

  if (menu->sock < 0) 
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsGetTraceBin: no socket for %s:%s\n", 
			     menu->addr, menu->port );
      return WS_ERR_NO_CONNECTION;
    }

  sprintf( request, "GETSCNRAW: %d %s %s %s %lf %lf\n",
           menu_reqid++, pscn->sta, pscn->chan, pscn->net,
           getThis->reqStarttime, getThis->reqEndtime );

  len = (int)strlen(request);

  fprintf(out_file, "Request is: %s\n", request);
  if ( (ret = send_ew( menu->sock, request, len, 0, timeout )) != len )
    {
      if (ret < 0 )
	{
	  ret = WS_ERR_BROKEN_CONNECTION;
	} else {
	  ret = WS_ERR_TIMEOUT;
	}
      goto Abort;
    }

  ret = wsWaitBinHeader( menu, getThis->pBuf, getThis->bufLen, timeout, out_file );
  switch (ret) {
  case WS_ERR_INPUT:
    return ret;
    break;
  case WS_ERR_NONE:
    break;
  default:
    /* buffer overflow, timeout, socket error, or broken connection */
      goto Abort;
  }

  err = wsParseBinHeaderReply( getThis, getThis->pBuf, out_file );
  switch (err) {
  case WS_ERR_INPUT:
  case WS_WRN_FLAGGED:  /* no trace data to get */
    goto next_menu;
    break;
  case WS_ERR_PARSE:  /* can't get pending trace data, so close socket */
    /* DK 03192002 setting ret due to bug reported by Ilya Dricker */
    if(ret == WS_ERR_NONE)
      ret = err;
    /* end DK 0319 change */
    goto Abort;
    break;
  default:  /* no error, so continue */
    break;
  }
  
  if ( getThis->actLen )
    {
      long int binLen = getThis->actLen;
      
      if ( (int)getThis->bufLen < binLen )
	binLen = (int)getThis->bufLen;
      ret = recv_all( menu->sock, getThis->pBuf, binLen, 0, timeout );
      
      if ( ret != binLen ) 
	{
	  if ( ret < 0 )
	    {
	      ret = WS_ERR_BROKEN_CONNECTION;
	    } 
	  else 
	    {
	      ret = WS_ERR_TIMEOUT;
	    }
	  goto Abort;
	}
      
      if ( binLen < getThis->actLen ) 
	{
	  getThis->actLen = binLen;
	  ret = WS_ERR_BUFFER_OVERFLOW;
	  if (WS_CL_DEBUG) fprintf(out_file, "wsGetTraceBin(): buffer full, trace truncated\n" );
	  goto Abort;
	}
    }
  fprintf(out_file, "End of wsGetTraceBin.\n");
  return WS_ERR_NONE;
  
Abort:
  wsDetachServer( menu, out_file );
  if (WS_CL_DEBUG) 
    {
      if  ( ret == WS_ERR_TIMEOUT ) {
	fprintf(out_file, "wsGetTraceBin(): server %s:%s timed out\n", menu->addr,
	       menu->port );
      } else if ( ret == WS_ERR_BROKEN_CONNECTION ) {
	fprintf(out_file, "wsGetTraceBin(): broken connection to server %s:%s\n",
	       menu->addr, menu->port);
      }
    }

  fprintf(out_file, "End of wsGetTraceBin(abort)\n\n");  
  return ret;
}

/**************************************************************************
 *      wsGetTraceAscii: retrieves the ascii trace data specified in      *
 *      the structure 'getThis': The current menu list, as buit by the    *
 *      routines above will be searched for a matching SCN. If a match    *
 *      is found, the associated wave server will be contacted, and a     *
 *      request for the trace snippet will be made.                       *
 **************************************************************************/

int wsGetTraceAscii( TRACE_REQ* getThis, WS_MENU_QUEUE_REC* menu_queue, 
		     int timeout, FILE *out_file )
     /*
Arguments:
        getThis:   a TRACE_REQ structure (see ws_client.h), with the
                   request portion filled in.
     menu_queue:   pointer to list of menues.
        timeout:   timeout interval in milliseconds.
        return:    WS_ERR_NONE: all went well.
                   WS_ERR_EMPTY_MENU: No menu list found.
                   WS_ERR_SCN_NOT_IN_MENU: SCN not found in menu list.
                   WS_ERR_BUFFER_OVERFLOW: buffer too small for reply
		   WS_ERR_BROKEN_CONNECTION: couldn't talk to server
		   WS_ERR_TIMEOUT: if a timeout occured
		   WS_ERR_NO_CONNECTION: There was no connection for the server
                   WS_WRN_FLAGGED: wave server returned flag warning.
		     the flag character is in the TRACE_REQ structure.
		     */
{
  int ret, len, err;
  WS_MENU menu = NULL;
  WS_PSCN pscn = NULL;
  char request[wsREQLEN];

  fprintf(out_file, "wsGetTraceAscii:\n\n");
  
  if ( menu_queue->head == NULL )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsGetTraceAscii: empty menu\n");
      return WS_ERR_EMPTY_MENU;
    }
  
  if ( (ret = wsSearchSCN( getThis, &menu, &pscn, menu_queue, out_file )) != WS_ERR_NONE )
    {
      return ret; /* WS_ERR_SCN_NOT_IN_MENU */
    }
		     
  if ( menu->sock < 0 )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsGetTraceAscii: no socket for %s:%s\n",
			     menu->addr, menu->port );
      return WS_ERR_NO_CONNECTION;
    }
  
  sprintf( request, "GETSCN: %d %s %s %s %lf %lf %d\n",
           menu_reqid++, pscn->sta, pscn->chan, pscn->net,
           getThis->reqStarttime, getThis->reqEndtime, getThis->fill );
  len = (int)strlen( request );
  
  fprintf(out_file, "Request is: %s\n", request);
  if ( ( ret = send_ew( menu->sock, request, len, 0, timeout )) != len )
    {
      if (ret < 0 )
	{
	  if (WS_CL_DEBUG) 
	    fprintf(out_file, "wsGetTraceAscii: connection broke to server %s:%s\n", 
		  menu->addr, menu->port );
	  ret = WS_ERR_BROKEN_CONNECTION;
	} else {
	  if (WS_CL_DEBUG)
	    fprintf(out_file, "wsGetTraceAscii: server %s:%s timed out\n", 
		  menu->addr, menu->port );
	  ret = WS_ERR_TIMEOUT;
	}
      goto Abort;
    }
  
  ret = wsWaitAscii( menu, getThis->pBuf, getThis->bufLen, timeout, out_file );
  if ( ret != WS_ERR_NONE && ret != WS_ERR_BUFFER_OVERFLOW )
    {
      goto Abort;
    }
  else
    {
    err = wsParseAsciiHeaderReply( getThis, getThis->pBuf, out_file );
    if ( err < WS_ERR_NONE )
	{
	  if ( ( ret == WS_ERR_BUFFER_OVERFLOW ) && WS_CL_DEBUG )
	    fprintf(out_file, "wsGetTraceAscii(): buffer overflow; parse error\n" );
	  return err;
	}
    }
  /* wsParseAsciiReply puts the trace data into getThis, so now we're done */
  fprintf(out_file, "End of wsGetTraceAscii.\n");
  return WS_ERR_NONE;
  
Abort:
  wsDetachServer( menu, out_file );
  fprintf(out_file, "End of wsGetTraceAscii(abort).\n");
  return ret;
}

/*******************************************
 *  wsKillPSCN: Deallocates the PSCN list  *
 *******************************************/
void wsKillPSCN( WS_PSCN pscn, FILE *out_file )
     /*
Arguments:
       pscn: pointer to a list of scn structures
       */
{
/*  fprintf(out_file, "wsKillPSCN:\n\n");
  fprintf(out_file, "Deallocating the PSCN list.\n");*/
  while ( pscn )
    {
      WS_PSCN next = pscn->next;

      free( pscn );
      pscn = next;
    }
/*  fprintf(out_file, "End of wsKillPSCN\n");*/
  return;
}

/*****************************************************************************
 * wsGetServerPSCN: Return the pscn list for this server from the menu queue *
 *****************************************************************************/
int wsGetServerPSCN( char* addr, char* port, WS_PSCN* pscnp,
		     WS_MENU_QUEUE_REC* menu_queue, FILE *out_file )
     /*
Arguments: 
       addr: IP address of the server
       port: port number of the server
      pscnp: pointer to the pscn list to be returned
 menu_queue: pointer to list of menus.
     return: WS_ERR_NONE: all went well
             WS_ERR_EMPTY_MENU: no menu in the queue
	     WS_ERR_SERVER_NOT_IN_MENU: server's menu not in the queue
	     */
{
  /*fprintf(out_file, "wsGetServerPSCN:\n\n");*/

  int ret = menu_queue->head ? WS_ERR_SERVER_NOT_IN_MENU : WS_ERR_EMPTY_MENU;
  WS_MENU menu = menu_queue->head;

  /*fprintf(out_file, "Searching menu\n");*/
  *pscnp = NULL;
  while ( menu )
    {
      if ( strcmp( addr, menu->addr ) == 0 &&
	   strcmp( port, menu->port ) == 0 )
	{
	  ret = WS_ERR_NONE;
	  *pscnp = menu->pscn;
	  break;
	}
      menu = menu->next;
    }

    if (WS_CL_DEBUG) 
      {
	if ( ret == WS_ERR_SERVER_NOT_IN_MENU )
	  fprintf(out_file,"wsGetServerPSCN(): WS_ERR_SERVER_NOT_IN_MENU\n" );
	else if ( ret == WS_ERR_EMPTY_MENU )
	  fprintf(out_file,"wsGetServerPSCN(): WS_ERR_EMPTY_MENU\n" );
      }
  
  /*fprintf(out_file, "End of wsGetServerPSCN.\n\n");*/
  return ret;
}


/***********************************************************************
 * wsSearchSCN: Find menu and PSCN in queue which will serve this scn  *
 *              If menup points to a menu in the menu_queue, search    *
 *              starts at the next menu after *menup; otherwise,       *
 *              search starts at menu_queue->head.                     *
 *              If the SCN is listed more than once in the queue, only *
 *              the first menu and PSCN will be returned.              *
 ***********************************************************************/
int wsSearchSCN( TRACE_REQ* getThis, WS_MENU* menup, WS_PSCN* pscnp,
			WS_MENU_QUEUE_REC* menu_queue, FILE *out_file )
     /*
Arguments:
      getThis: a TRACE_REQ structure with the SCN to search for.
        menup: pointer to the menu to return.
        pscnp: pointer to the pscn list to return.
      returns: WS_ERR_NONE: if all went well
               WS_ERR_EMPTY_MENU: no menus in the queue
               WS_ERR_SERVER_NOT_IN_MENU: scn not in the queue
	       */
{
  int ret;
  WS_MENU menu;

  if ( *menup != NULL)
    menu = (*menup)->next;
  else
    menu  = menu_queue->head;
  ret = menu ? WS_ERR_SCN_NOT_IN_MENU : WS_ERR_EMPTY_MENU;
  *pscnp = NULL;

  while ( menu )
    {
      WS_PSCN pscn = menu->pscn;
      while ( pscn )
	{
	  if ( strcmp( getThis->sta, pscn->sta ) == 0 &&
	       strcmp( getThis->chan, pscn->chan ) == 0 &&
	       strcmp( getThis->net, pscn->net ) == 0 )
	    {
	      ret = WS_ERR_NONE;
	      *menup = menu;
	      *pscnp = pscn;
	      goto exit;
	    }
	  pscn = pscn->next;
	}
      menu = menu->next;
    }

 exit:
  if (WS_CL_DEBUG) 
    {
      if ( ret == WS_ERR_SCN_NOT_IN_MENU )
	fprintf(out_file,"wsSearchSCN(): WS_ERR_SCN_NOT_IN_MENU\n" );
      else if ( ret == WS_ERR_EMPTY_MENU )
	fprintf(out_file,"wsSearchSCN(): WS_ERR_EMPTY_MENU\n" );
    }
  
  return ret;
}

/***********************************************************************
 *  wsAttachServer: Open a connection to a server. The timeout starts  *
 *    when connect() is called by connect_ew() in socket_ew_common.c   *
 ***********************************************************************/
int wsAttachServer( WS_MENU menu, int timeout, FILE *out_file )
     /*
Arguemnts:
       menu: pointer to the menu of the server
    timeout: time interval in milliseconds; use -1 for no timeout.
    returns: WS_ERR_NONE: if all went well.
             WS_ERR_INPUT: if menu is missing.
             WS_ERR_SOCKET: if a socket error occurred.
	     WS_ERR_NO_CONNECTION: if a connection could not be established
	     */
{
  int                ret = WS_ERR_NONE;
  SOCKET             sock = 0;   /* Socket descriptor                  */
  struct sockaddr_in s_in ;      /* Server's socket address stucture   */

  fprintf(out_file, "wsAttachServer:\n\n");
  if ( !menu )
    {
      ret = WS_ERR_INPUT;
      if (WS_CL_DEBUG) fprintf(out_file, "wsAttachServer(): WS_ERR_INPUT\n");
      goto Abort;
    }
  if ( menu->sock > 0 )  /* maybe already connected, so disconnect first */
    {
      wsDetachServer( menu, out_file );
    }

  /* open a non_blocking socket
  *****************************/
  if ( ( sock = socket_ew( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
    {
      ret = WS_ERR_SOCKET;
      if (WS_CL_DEBUG) 
	fprintf(out_file, "wsAttachServer(): socket_ew() call failed\n" );
      goto Abort;
    }

  /* Stuff address and port into socket structure
  ********************************************/
  memset( (char *)&s_in, '\0', sizeof(s_in) );
  s_in.sin_family = AF_INET;
  s_in.sin_port   = htons( (short)atoi(menu->port) );

#ifdef _LINUX
  if ((int)(s_in.sin_addr.s_addr = inet_addr(menu->addr)) == -1)
#else
  if ((int)(s_in.sin_addr.S_un.S_addr = inet_addr(menu->addr)) == -1)
#endif
    {
      ret = WS_ERR_NO_CONNECTION;
      if (WS_CL_DEBUG) 
	fprintf(out_file, "wsAttachServer(): inet_addr failed on <%s>\n",
	       menu->addr );
      goto Abort;
    }

  if ( connect_ew( sock, (struct sockaddr *)&s_in, sizeof(s_in), timeout) == -1 )
    {
      ret = WS_ERR_NO_CONNECTION;
      if (WS_CL_DEBUG) 
	fprintf(out_file, "wsAttachServer(): connect() call failed\n" );
      goto Abort;
    }
  menu->sock = sock;

  ret = WS_ERR_NONE;
  fprintf(out_file, "End of wsAttachServer.\n\n");
  return ret;

  /* An error occured;
   * don't blab about here since we already did earlier. */
Abort:
  menu->sock = (SOCKET)-1; /* mark the socket as dead */
  return ret;
}


/*********************************************************************
 * wsDetachServer: Immediately disconnect from a socket if it's open *
 *********************************************************************/
void wsDetachServer( WS_MENU menu, FILE *out_file )
     /*  
Arguments:
           menu: menu of server to be detached
      */
{
  if ( !menu || menu->sock == -1 )
    return;
  closesocket_ew( menu->sock, SOCKET_CLOSE_IMMEDIATELY_EW );
  menu->sock = (SOCKET)-1;
}


/*********************************************************************
 * wsWaitBinHeader: Retrieve the ASCII header of a binary message.   *
 * The header will be terminated by a newline, but binary characters *
 * will follow in the same message. Thus this routine must read one  *
 * character at a time. Since the header is relatively short, this   *
 * should not be much a performance hit.                             *
 * Returns after newline is read, when timeout expires if set,       *
 * or on error.                                                      *
 *********************************************************************/
static int wsWaitBinHeader( WS_MENU menu, char* buf, int buflen,
			    int timeout_msec, FILE *out_file )
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
  int ir = 0;
  int nr = 0;
  char c = '\0';
  int ret, ioctl_ret;
  fd_set ReadableSockets;
  Time_ew StartTime;
  struct timeval SelectTimeout;
  Time_ew timeout = adjustTimeoutLength(timeout_msec);
  long lOnOff;

  fprintf(out_file, "wsWaitBinHeader:\n\n");
  if ( !buf || buflen <= 0 )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsWaitBinHeader(): no input buffer\n");
      return WS_ERR_INPUT;
    }
  
  /* If there is no timeout, make the socket blocking */
  if (timeout_msec == -1)
    {
      timeout = 0;
      lOnOff = 0;
      ioctl_ret = ioctlsocket(menu->sock, FIONBIO, (u_long *)&lOnOff);
      if (ioctl_ret == SOCKET_ERROR)
	{
	  ret = WS_ERR_SOCKET;
	  if (WS_CL_DEBUG) 
	    fprintf(out_file, "wsWaitBinHeader: error %d occurred during change to blocking\n",
		socketGetError_ew());
	  goto Done;
	}
    }

  StartTime = GetTime_ew(); /* the timer starts here */
  /* Start reading the socket, one character at a time */
  while ( c != '\n' )
    {
      if ((timeout) && (GetTime_ew() - timeout) > StartTime )
	{
	  ret = WS_ERR_TIMEOUT;
	  if (WS_CL_DEBUG) fprintf(out_file, "wsWaitBinHeader timed out\n");
	  goto Done;
	}
      if ( ir == buflen - 1 )
	{
	  /* stop if there's no more room  */
	  if (WS_CL_DEBUG) 
	    fprintf(out_file, "wsWaitBinHeader(): reply buffer overflows\n" );
	  ret = WS_ERR_BUFFER_OVERFLOW;
	  goto Done;
	}

      /* try to get a char from socket */
      nr = recv( menu->sock, &c, 1, 0 );
      if ( nr == -1 && socketGetError_ew() == WOULDBLOCK_EW ) 
	{
	  FD_ZERO( &ReadableSockets );
	  FD_SET( (SOCKET)(menu->sock), &ReadableSockets );
	  while (( !select(menu->sock + 1, &ReadableSockets, 0, 0,
			   resetTimeout( &SelectTimeout)))) 
	    {
	      if ((timeout) && (GetTime_ew() - timeout) > StartTime ) 
		{
		  ret = WS_ERR_TIMEOUT;
		  if (WS_CL_DEBUG) fprintf(out_file, "wsWaitBinHeader timed out\n");
		  goto Done;
		}
	      FD_ZERO( &ReadableSockets );
	      FD_SET( (SOCKET)(menu->sock), &ReadableSockets );
	      sleep_ew(100); /* wait a little and try again */
	    }
	  /* select() says won't block */
	  nr = recv( menu->sock, &c, 1, 0 ); 
	}
      if ( nr == 1 && c != 0 )
	{
	  /* got a character; save it      */
	  buf[ir++] = c;
	}
      else if ( nr == -1 )
	{
	  /* trouble reading socket        */
	  ret = WS_ERR_BROKEN_CONNECTION;
	  if (WS_CL_DEBUG) 
	    fprintf(out_file, "wsWaitBinHeader(): Error on socket recv()\n" );
	  goto Done;
	}
    }
  buf[ir] = '\0';                   /* null-terminate the buf      */

  fprintf(out_file, "Binary header is: %s\n", buf);
  ret = WS_ERR_NONE;


Done:
  buf[ir] = '\0';                 /* null-terminate the buf      */
  /* If there was no timeout, then change the socket back to non-blocking */
  if (timeout_msec == -1) 
    {
      lOnOff = 1;
      ioctl_ret = ioctlsocket( menu->sock, FIONBIO, (u_long *)&lOnOff);
      if (ioctl_ret == SOCKET_ERROR)
	{
	  
	  if (WS_CL_DEBUG) 
	    fprintf(out_file,"wsWaitBinHeader: error %d occurred during change to non-blocking\n",
	      socketGetError_ew() );
	  ret = WS_ERR_SOCKET;
	}
    }
  fprintf(out_file, "End of wsWaitBinHeader.\n\n");
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
static int wsWaitAscii( WS_MENU menu, char* buf, int buflen, int timeout_msec, FILE *out_file )
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

  fprintf(out_file, "wsWaitAscii:\n\n");
  if ( !buf || buflen <= 0 )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsWaitAscii(): no input buffer\n");
      return WS_ERR_INPUT;
    }

  /* If there is no timeout, make the socket blocking */
  if (timeout_msec == -1)
    {
      timeout = 0;
      lOnOff = 0;
      ioctl_ret = ioctlsocket(menu->sock, FIONBIO, (u_long *)&lOnOff);
      if (ioctl_ret == SOCKET_ERROR)
	{
	  ret = WS_ERR_SOCKET;
	  if (WS_CL_DEBUG) fprintf(out_file,
		"wsWaitAscii: error %d occurred during change to blocking\n",
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
	  if (WS_CL_DEBUG) fprintf(out_file, "wsWaitAscii timed out\n");
	  goto Done;
	}
      if ( ir >= buflen - 2 )
	{
	  if (WS_CL_DEBUG)
	    fprintf(out_file, "wsWaitAscii(): reply buffer overflows\n" );
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
	  FD_SET( (SOCKET)(menu->sock), &ReadableSockets );
	  while (( !select(menu->sock + 1, &ReadableSockets, 0, 0,
			   resetTimeout( &SelectTimeout)))) 
	    {
	      if ((timeout) && (GetTime_ew() - timeout) > StartTime ) 
		{
		  ret = WS_ERR_TIMEOUT;
		  if (WS_CL_DEBUG) fprintf(out_file, "wsWaitAscii timed out\n");
		  goto Done;
		}
	      FD_ZERO( &ReadableSockets );
	      FD_SET( (SOCKET)(menu->sock), &ReadableSockets );
	      sleep_ew(100); /* wait a little and try again */
	    }
	  /* poll() says won't block */
	  nr = recv( menu->sock, &buf[ir], len, 0 ); 
	}
      
      if ( nr == -1 || nr > len )
	{
	  /* trouble reading socket        */
	  ret = WS_ERR_BROKEN_CONNECTION;
	  if (WS_CL_DEBUG) fprintf(out_file, "wsWaitAscii(): Error on socket recv()\n" );
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
  fprintf(out_file, "Reply was: %s\n", buf);

  /* If there was no timeout, then change the socket back to non-blocking */
  if (timeout_msec == -1) 
    {
      lOnOff = 1;
      ioctl_ret = ioctlsocket( menu->sock, FIONBIO, (u_long *)&lOnOff);
      if (ioctl_ret == SOCKET_ERROR) 
	{
	  
	  if (WS_CL_DEBUG) fprintf(out_file, "wsWaitAScii: error %d occurred during change to non-blocking\n",
				 socketGetError_ew() );
	  ret = WS_ERR_SOCKET;
	}
    }
  fprintf(out_file, "End of wsWaitAscii.\n\n");
  return ret;
}

/***********************************************************************
 * wsParseMenuReply: parse the reply we got from the waveserver into   *
 * a menu list. Handles replies to MENU, MENUPIN and MENUSCN requests. *
 ***********************************************************************/
static int wsParseMenuReply( WS_MENU menu, char* reply, FILE *out_file )
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
  char sta[7];
  char chan[9];
  char net[9];
  double tankStarttime = 0.0, tankEndtime = 0.0;
  char datatype[3];
  int scn_pos = 0;

  fprintf(out_file, "wsParseMenuReply:\n\n");
  if ( !reply || !menu )
    {
      if (WS_CL_DEBUG) fprintf(out_file, "wsParseMenuReply: WS_ERR_INPUT\n");
      return WS_ERR_INPUT;
    }

  if ( sscanf( &reply[scn_pos], "%d", &reqid ) < 1 )
    {
      if (WS_CL_DEBUG)
	fprintf(out_file,"wsParseMenuReply(): error parsing reqid\n" );
      return WS_ERR_PARSE;
    }
  wsSkipN( reply, 1, &scn_pos, out_file );
  while ( reply[scn_pos] && reply[scn_pos] != '\n' )
    {
      WS_PSCN pscn = NULL;
      if ( sscanf( &reply[scn_pos], "%d %s %s %s %lf %lf %s",
		   &pinno, sta, chan, net,
		   &tankStarttime, &tankEndtime, datatype ) < 7 )
	{
	  if (WS_CL_DEBUG)
	    fprintf(out_file,"wsParseMenuReply(): error decoding reply<%s>\n",
		 &reply[scn_pos] );
	  return WS_ERR_PARSE;
	}
      pscn = ( WS_PSCN_REC* )calloc(sizeof(WS_PSCN_REC),1);
      if ( !pscn )
	{
	  if (WS_CL_DEBUG)
	    fprintf(out_file, "wsParseMenuReply(): error allocating memory\n");
	  return WS_ERR_MEMORY;
	}

      pscn->next = menu->pscn;
      pscn->pinno = (short)pinno;
      strcpy( pscn->sta, sta );
      strcpy( pscn->chan, chan );
      strcpy( pscn->net, net );
      pscn->tankStarttime = tankStarttime;
      pscn->tankEndtime = tankEndtime;
      menu->pscn = pscn;

      fprintf(out_file, "Pinno: %d\tSta: %s\tChan: %s\tNet: %s\tStarttime: %f\tEndTime: %f\tDatatype: %s\n",
	          pscn->pinno, pscn->sta, pscn->chan, pscn->net, pscn->tankStarttime, pscn->tankEndtime, datatype); 


      wsSkipN( reply, 7, &scn_pos, out_file );
    }

  fprintf(out_file, "end of wsParseMenuReply\n\n");
  return WS_ERR_NONE;
}

/***********************************************************************
 * wsParseBinHeaderReply: parse the reply we got from the waveserver   *
 * into a TRACE_REQ structure. Handles the header for replies reply to *
 * GETSCNRAW requests.                                                 *
 ***********************************************************************/
static int wsParseBinHeaderReply( TRACE_REQ* getThis, char* reply, FILE *out_file )
{
  /* Arguments:
   *    getThis: pointer to TRACE_REQ structure to be filled with reply info
   *      reply: pointer to reply to be parsed.
   *   Returns: WS_ERR_NONE:  if all went well
   *            WS_ERR_INPUT: if bad input parameters
   *            WS_ERR_PARSE: if we couldn't parse part of the reply
   *            WS_WRN_FLAGGED: server sent us a no-data flag
   */
  int reqid = 0;
  int pinno = 0;
  char sta[7];
  char chan[9];
  char net[9];
  char flag[9];
  char datatype[3];
  double tankStarttime = 0.0, tankEndtime = 0.0;
  int bin_len = 0;
  int scn_pos = 0;

  fprintf(out_file, "wsParseBinHeader:\n\n");
  if ( !reply || !getThis )
    {
      if (WS_CL_DEBUG)
	fprintf(out_file, "wsParseBinHeaderReply(): bad input parameters\n");
      return WS_ERR_INPUT;
    }

  if ( sscanf( &reply[scn_pos], "%d %d", &reqid, &pinno ) < 2 )
    {
      if (WS_CL_DEBUG)
	fprintf(out_file,"wsParseBinHeaderReply(): error parsing reqid/pinno\n" );
      return WS_ERR_PARSE;
    }
  wsSkipN( reply, 2, &scn_pos, out_file );

  if ( sscanf( &reply[scn_pos], "%s %s %s", sta, chan, net ) < 3 )
    {
      if (WS_CL_DEBUG) fprintf(out_file,"wsParseBinHeaderReply(): error parsing SCN\n" );
      return WS_ERR_PARSE;
    }
  wsSkipN( reply, 3, &scn_pos, out_file );

  if ( sscanf( &reply[scn_pos], "%s %s", flag, datatype ) < 2 )
    {
      if (WS_CL_DEBUG)
	fprintf(out_file, "wsParseBinHeaderReply(): error parsing flag/datatype\n" );
      return WS_ERR_PARSE;
    }
  wsSkipN( reply, 2, &scn_pos, out_file );

  if ( strlen(flag) == 1 )
    {
      if ( sscanf( &reply[scn_pos], "%lf %lf", &tankStarttime, 
		   &tankEndtime ) < 2 )
	{
	  if (WS_CL_DEBUG) 
	    fprintf(out_file, "wsParseBinHeaderReply(): error parsing starttime/endtime\n" );
	  return WS_ERR_PARSE;
	}
      wsSkipN( reply, 2, &scn_pos, out_file );

      if ( sscanf( &reply[scn_pos], "%d", &bin_len ) < 1 )
	{
	  if (WS_CL_DEBUG) 
	    fprintf(out_file, "wsParseBinHeaderReply(): error parsing bin_len\n" );
	  return WS_ERR_PARSE;
	}
      wsSkipN( reply, 1, &scn_pos, out_file );

    }
  else if ( strlen(flag) == 2 )
    {
      tankStarttime = 0.0;
      tankEndtime = 0.0;
      bin_len = 0;
      if ( strcmp(flag,"FL") == 0 )
	{
	  if ( sscanf( &reply[scn_pos], "%lf", &tankStarttime) < 1 )
	    {
	      if (WS_CL_DEBUG) 
		fprintf(out_file, "wsParseBinHeaderReply(): error parsing starttime\n" );
	      return WS_ERR_PARSE;
	    }
	  wsSkipN( reply, 1, &scn_pos, out_file );
	}
      else if ( strcmp(flag,"FR") == 0 )
	{
	  if ( sscanf( &reply[scn_pos], "%lf", &tankEndtime) < 1 )
	    {
	      if (WS_CL_DEBUG) 
		fprintf(out_file, "wsParseBinHeaderReply(): error parsing endtime\n" );
	      return WS_ERR_PARSE;
	    }
	  wsSkipN( reply, 1, &scn_pos, out_file );
	}
    }
  else
    {
      if (WS_CL_DEBUG) 
	fprintf(out_file, "wsParseBinHeaderReply(): bad flag[%s]\n", flag );
      return WS_ERR_PARSE;
    }

  getThis->pinno = (short)pinno;
  getThis->actStarttime = tankStarttime;
  getThis->actEndtime = tankEndtime;
  getThis->samprate = (double) 0.0; /* server doesn't send this */
  getThis->actLen = bin_len;

  fprintf(out_file, "Sta: %s\n", sta);
  fprintf(out_file, "Chan: %s\n", chan);
  fprintf(out_file, "Net: %s\n", net);
  fprintf(out_file, "pinno: %d\n", pinno);
  fprintf(out_file, "actStarttime: %f\n", tankStarttime);
  fprintf(out_file, "actEndtime: %f\n", tankEndtime);
  fprintf(out_file, "samprate: 0.0\n" );
  fprintf(out_file, "actLen: %d\n", bin_len);

  if ( strlen( flag ) >= 2 ) {
    getThis->retFlag = flag[1];
	fprintf(out_file, "retFlag: %s\n", &flag[1]);
	fprintf(out_file, "End of wsParseBinHeader.\n\n");
    return WS_WRN_FLAGGED;
  } else {
    getThis->retFlag = '\0';
	fprintf(out_file, "retFlag:  \n");
	fprintf(out_file, "End of wsParseBinHeader.\n\n");
    return WS_ERR_NONE;
  }
}


/***********************************************************************
 * wsParseAsciiHeaderReply: parse the reply we got from the waveserver *
 * into a TRACE_REQ structure. Handles the header for replies reply to *
 * GETSCN and GETPIN requests.                                         *
 ***********************************************************************/
static int wsParseAsciiHeaderReply( TRACE_REQ* getThis, char* reply, FILE *out_file )
{
  /* Arguments:
   *    getThis: pointer to TRACE_REQ structure to be filled with reply info
   *      reply: pointer to reply to be parsed.
   *   Returns: WS_ERR_NONE:  if all went well
   *            WS_ERR_INPUT: if bad input parameters
   *            WS_ERR_PARSE: if we couldn't parse part of the reply
   *            WS_WRN_FLAGGED: server sent us a no-data flag
   */
  int reqid = 0;
  int pinno = 0;
  char sta[7];
  char chan[9];
  char net[9];
  char flag[9];
  char datatype[3];
  double tankStarttime = 0.0, samprate = 0.0;
  int scn_pos = 0;

  fprintf(out_file, "wsParseAsciiHeaderReply:\n\n");
  if ( !reply )
    {
      if (WS_CL_DEBUG) 
	fprintf(out_file,"wsParseAsciiHeaderReply(): bad input parameters\n");
      return WS_ERR_INPUT;
    }

  if ( sscanf( &reply[scn_pos], "%d %d", &reqid, &pinno ) < 2 )
    {
      if (WS_CL_DEBUG) 
	fprintf(out_file,"wsParseAsciiHeaderReply(): error parsing reqid/pinno\n" );
      return WS_ERR_PARSE;
    }
  wsSkipN( reply, 2, &scn_pos, out_file );

  if ( sscanf( &reply[scn_pos], "%s %s %s", sta, chan, net ) < 3 )
    {
      if (WS_CL_DEBUG) 
	fprintf(out_file,"wsParseAsciiHeaderReply(): error parsing SCN\n" );
      return WS_ERR_PARSE;
    }
  wsSkipN( reply, 3, &scn_pos, out_file );

  if ( sscanf( &reply[scn_pos], "%s %s", flag, datatype ) < 2 )
    {
      if (WS_CL_DEBUG) 
	fprintf(out_file,"wsParseAsciiHeaderReply(): error parsing flag/datatype\n" );
      return WS_ERR_PARSE;
    }
  wsSkipN( reply, 2, &scn_pos, out_file );

  if ( strlen(flag) == 1 || strcmp(flag,"FG") == 0 )
    {
      if ( sscanf( &reply[scn_pos], "%lf %lf", &tankStarttime, &samprate ) < 2 )
	{
	  if (WS_CL_DEBUG)
	    fprintf(out_file,"wsParseAsciiHeaderReply(): error parsing startT/samprate\n" );
	  return WS_ERR_PARSE;
	}
      wsSkipN( reply, 2, &scn_pos, out_file );
    }
  else if ( strlen(flag) == 2 )
    {
      tankStarttime = 0.0;
      samprate = 0.0;
      if ( strcmp(flag,"FL") == 0 )
	{
	  if ( sscanf( &reply[scn_pos], "%lf", &tankStarttime) < 1 )
	    {
	      if (WS_CL_DEBUG)
		fprintf(out_file,"wsParseAsciiHeaderReply(): error parsing startTime\n" );
	      return WS_ERR_PARSE;
	    }
	  wsSkipN( reply, 1, &scn_pos, out_file );
	}
      else if ( strcmp(flag,"FR") == 0 )
	{
	  if ( sscanf( &reply[scn_pos], "%lf", &samprate) < 1 )
	    {
	      if (WS_CL_DEBUG)
		fprintf(out_file,"wsParseAsciiHeaderReply(): error parsing samprate\n" );
	      return WS_ERR_PARSE;
	    }
	  wsSkipN( reply, 1, &scn_pos, out_file );
	}
    }

  getThis->pinno = (short)pinno;
  getThis->actStarttime = tankStarttime;
  getThis->actEndtime = (double) 0.0;
  getThis->samprate = samprate;
  getThis->actLen = (long)strlen( reply ) - scn_pos;
  memmove(reply, &reply[scn_pos], getThis->actLen);
  reply[getThis->actLen] = 0;

  fprintf(out_file, "Sta: %s\n", sta);
  fprintf(out_file, "Chan: %s\n", chan);
  fprintf(out_file, "Net: %s\n", net);
  fprintf(out_file, "pinno: %d\n", pinno);
  fprintf(out_file, "actStarttime: %f\n", tankStarttime);
  fprintf(out_file, "actEndtime: 0.0\n");
  fprintf(out_file, "samprate: %f\n", samprate );
  fprintf(out_file, "actLen: %d\n", getThis->actLen);  

  if ( strlen( flag ) >= 2 ) {
    getThis->retFlag = flag[1];
	fprintf(out_file, "retFlag: %s\n", &flag[1]);
	fprintf(out_file, "End of wsParseAsciiHeaderReply.\n\n");
    return WS_WRN_FLAGGED;
  } else {
    getThis->retFlag = '\0';
	fprintf(out_file, "retFlag:  \n");
	fprintf(out_file, "End of wsParseAsciiHeaderReply.\n\n");
    return WS_ERR_NONE;
  }
}


/**************************************************************************
 *      wsSkipN: moves forward the pointer *posp in buf by moving forward *
 *      cnt words.  Words are delimited by either space or horizontal     *
 *      tabs; newline marks the end of the buffer.                        *
 **************************************************************************/
static void wsSkipN( char* buf, int cnt, int* posp, FILE *out_file )
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

int setWsClient_ewDebug(int debug, FILE *out_file)
{
  /* setWsClient_ewDebug() turns debugging on or off for 
     the ws_clientII routines
     */
  WS_CL_DEBUG=debug;
  return(0);
}
