
/***************************************************************************
 * network.c
 *
 * General network communication routines, using libdali primitives
 *
 * Written by Chad Trabant
 *
 * modified: 2013.144
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/select.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <netdb.h>
#endif

#include <libmseed.h>

#include <portable.h>

#include "util.h"


/***************************************************************************
 * my_connect():
 *
 * Open a TCP/IP network socket connection and set socket-level
 * timeout if supported by the system.
 *
 * Returns 0 on errors, otherwise the socket descriptor created.
 ***************************************************************************/
extern int
my_connect (char *host, char *port, int *socktimeout, int verbose)
{
  int sock;
  size_t addrlen;
  struct sockaddr addr;
  
  /* Sanity check for the host and port specified */
  if ( ! host || strlen(host) <= 0 )
    {
      ms_log (1, "server address specified incorrectly: %s\n", (host)?host:"");
      return 0;
    }
  
  if ( ! port || strlen(port) <= 0 )
    {
      ms_log (1, "port specified incorrectly: %s\n", (port)?port:"");
      return 0;
    }
  
  /* Resolve server address */
  if ( dlp_getaddrinfo (host, port, &addr, &addrlen) )
    {
      ms_log (2, "cannot resolve hostname %s\n", host );
      return -1;
    }
  
  /* Create a socket */
  if ( (sock = socket (PF_INET, SOCK_STREAM, 0)) < 0 )
    {
      ms_log (2, "[%s:%s] socket(): %s\n", host, port, dlp_strerror ());
      dlp_sockclose (sock);
      return -1;
    }
  
  /* Set socket I/O timeouts if possible */
  if ( *socktimeout )
    {
      int timeout = (socktimeout > 0) ? *socktimeout : - *socktimeout;
      
      if ( dlp_setsocktimeo (sock, timeout) == 1 )
        {
	  if ( verbose )
	    ms_log (1, "[%s:%s] using system socket timeouts\n", host, port);
          
          /* Negate timeout to indicate socket timeouts are set */
          *socktimeout = - timeout;
        }
    }
  
  /* Connect socket */
  if ( (dlp_sockconnect (sock, (struct sockaddr *) &addr, addrlen)) )
    {
      if ( verbose )
	ms_log (2, "[%s:%s] connect(): %s\n", host, port, dlp_strerror());
      
      dlp_sockclose (sock);
      return -1;
    }
  
  /* Set socket to non-blocking */
  if ( dlp_socknoblock(sock) )
    {
      ms_log (2, "Error setting socket to non-blocking\n");
      dlp_sockclose (sock);
      return -1;
    }
  
  return sock;
}  /* End of my_connect() */


/***************************************************************************
 * my_recv():
 *
 * recv() up to 'readlen' data from 'sock' into a specified 'buffer'.
 *
 * 'socktimeout' is a timeout in seconds and interpretted as follows:
 *   0 = non-blocking recv request
 *  >0 = timeout for recv in seconds
 *  <0 = timeout for recv using socket options set in my_connect()
 *
 * Returns values
 *   number of bytes read on success
 *   0 when no data available on non-blocking socket
 *  -1 on connection shutdown
 *  -2 on error
 ***************************************************************************/
extern int
my_recv (int sock, void *buffer, size_t readlen, int socktimeout)
{
  int nrecv;
  char *bptr = buffer;
  
  if ( ! buffer )
    {
      return -2;
    }
  
  /* Set socket to blocking if timeout requested */
  if ( socktimeout != 0 )
    {
      if ( dlp_sockblock (sock) )
        {
          ms_log (2, "Error setting socket to blocking: %s\n", dlp_strerror ());
          return -2;
        }
    }
  
  /* Set timeout alarm if needed */
  if ( socktimeout > 0 )
    {
      if ( dlp_setioalarm (socktimeout) )
        {
          ms_log (2, "Error setting network I/O timeout\n");
        }
    }
  
  /* Recv up to readlen bytes */
  if ( (nrecv = recv(sock, bptr, readlen, 0)) < 0 )
    {
      /* Report error if non-blocking and no data received */
      if ( socktimeout != 0 && ! dlp_noblockcheck() )
	{
	  ms_log (2, "recv(%d): %d %s\n", sock, nrecv, dlp_strerror ());
	  nrecv = -2;
	}
    }
  
  /* Peer completed an orderly shutdown */
  if ( nrecv == 0 )
    {
      nrecv = -1;
    }
  
  /* Cancel timeout alarm if set */
  if ( socktimeout > 0 )
    {
      if ( dlp_setioalarm (0) )
        {
          ms_log (2, "error cancelling network I/O timeout\n");
        }
    }
  
  /* Set socket to non-blocking if blocking was used for timeout */
  if ( socktimeout != 0 )
    {
      if ( dlp_socknoblock (sock) )
        {
          ms_log (2, "error setting socket to non-blocking: %s\n", dlp_strerror ());
          return -2;
        }
    }
  
  return nrecv;
}  /* End of my_recv() */


/***************************************************************************
 * my_send():
 *
 * send() 'writelen' bytes from 'buffer' to 'sock'.
 * 'socktimeout' is a timeout in seconds.
 *
 * Returns -1 on error, 0 on success.
 ***************************************************************************/
extern int
my_send (int sock, void *buffer, size_t sendlen, int socktimeout)
{
  /* Set socket to blocking */
  if ( dlp_sockblock (sock) )
    {
      ms_log (2, "error setting socket to blocking\n");
      return -1;
    }
  
  /* Set timeout alarm if needed */
  if ( socktimeout > 0 )
    {
      if ( dlp_setioalarm (socktimeout) )
        {
          ms_log (2, "error setting network I/O timeout\n");
        }
    }
  
  /* Send data */
  if ( send (sock, buffer, sendlen, 0) != sendlen )
    {
      ms_log (2, "error send()ing data\n");
      return -1;
    }
  
  /* Cancel timeout alarm if set */
  if ( socktimeout > 0 )
    {
      if ( dlp_setioalarm (0) )
        {
          ms_log (2, "error cancelling network I/O timeout\n");
        }
    }
  
  /* Set socket to non-blocking */
  if ( dlp_socknoblock (sock) )
    {
      ms_log (2, "error setting socket to non-blocking\n");
      return -1;
    }
  
  return 0;
}  /* End of my_send() */
