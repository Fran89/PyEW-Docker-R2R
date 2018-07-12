/*****************************************************************************
 * network.c - some simple socket based network functions
 *
 *****************************************************************************/
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <arpa/inet.h>
#include <arpa/telnet.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <platform.h>
#include "geolib.h"

/* private global variables */
static int net_timeout;
static char server_name [100];	/* host name of server */

/*****************************************************************************
 *****************************************************************************
 *********** This section deals with functions for network servers ***********
 *****************************************************************************
 *****************************************************************************/

/* we dont need server functions for EW (and they don't compile easily
 * on WINDOWS) so they have been removed */

/*****************************************************************************
 *****************************************************************************
 *********** This section deals with functions for network clients ***********
 *****************************************************************************
 *****************************************************************************/

/***************************************************************************
 * connect_to_server
 *
 * Description: attempt to connect to the given server
 *
 * Input parameters: dest_host - the host name to connect to
 *                   dest_port - the port to connect to
 *                   src_port - the port to connect from OR
 *                              -1 to allow system to choose a socket
 *                   tmout - the timeout (in seconds) to use on the
 *                           connection (or -1 for no timeouts)
 *                   allow_reuse - allow socket re-use
 * Output parameters: socket_handle - the connection handle
 * Returns: TRUE if connection was made, FALSE otherwise
 *
 * Comments: 
 *
 **************************************************************************/
int connect_to_server (char *dest_host, int dest_port, int src_port, 
                       int tmout, int allow_reuse, int *socket_handle)

{

  int count, connected, status;
  unsigned long addr;
  char *dummy_adr_ptr [2], on;
  struct hostent *hosts_entry, dummy_entry;
  static struct sockaddr_in remote_socket_addr, local_socket_addr;

#ifndef WIN32
  int socket_type;
#endif

  /* store global parameters */
  net_timeout = tmout;

  /* initialise the socket structures and the hosts name entry */
  memset (&remote_socket_addr, 0, sizeof (remote_socket_addr));
  hosts_entry = (struct hostent *) 0;

  /* have we been given an INTERNET address ?? */
  addr = inet_addr (dest_host);
  if (addr != -1)
  {
    /* yes - make a dummy host entry for it */
    dummy_entry.h_addrtype = AF_INET;
    dummy_adr_ptr [0] = (char *) &addr;
    dummy_adr_ptr [1] = (char *) 0;
    dummy_entry.h_addr_list = dummy_adr_ptr;
    dummy_entry.h_length = sizeof (addr);
    hosts_entry = &dummy_entry;
  }
  /* if its not an Internet address then see if it is a known host name */
  else hosts_entry = gethostbyname (dest_host);

  /* if we haven't resolved it by now then it can't be resolved */
  if (! hosts_entry) return GEO_FALSE;

  /* for each address in the host entry ... */
  count = 0;
  connected = GEO_FALSE;
  remote_socket_addr.sin_family = hosts_entry->h_addrtype;
  remote_socket_addr.sin_port = htons ((u_short) dest_port);
  while (*(hosts_entry->h_addr_list +count) && (! connected))
  {
    /* attempt to create a scoket */ 
    *socket_handle = socket (remote_socket_addr.sin_family, SOCK_STREAM, 0);
    if (*socket_handle < 0) return GEO_FALSE;
    else
    {
      /* set options */
      if (allow_reuse)
      {
        on = 1;
        setsockopt (*socket_handle, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
      }

      /* attempt to bind the local end of the connection */
      if (src_port >= 0)
      {
        local_socket_addr.sin_family = AF_INET;
        local_socket_addr.sin_addr.s_addr = INADDR_ANY;
        local_socket_addr.sin_port = htons ((u_short) src_port);
        if (bind (*socket_handle, (struct sockaddr *) &local_socket_addr, sizeof(local_socket_addr)))
	{
#ifdef WIN32
          closesocket (*socket_handle);
#else
	  close (*socket_handle);
#endif
          return GEO_FALSE;
	}
      }
      
      /* attempt to connect to the host using the socket */
      memcpy (&remote_socket_addr.sin_addr,
              *(hosts_entry->h_addr_list +count),
              hosts_entry->h_length);
      status = connect (*socket_handle,
		        (struct sockaddr *) &remote_socket_addr,
		         sizeof (remote_socket_addr));
      if (status)
      {
        /* if you get here then the connection failed - tidy up */
#ifdef WIN32
        closesocket (*socket_handle);
#else
	close (*socket_handle);
#endif
        count ++;
      }
      else connected = GEO_TRUE;
    }
  }

  /* set the required options on the socket */
#ifndef WIN32
  signal (SIGPIPE, SIG_IGN);
  socket_type = IPTOS_LOWDELAY;
  if (setsockopt (*socket_handle, IPPROTO_IP, IP_TOS, 
                 (char *) &socket_type, sizeof(int)) < 0)    
  {
    close_socket (*socket_handle);
    return GEO_FALSE;
  }
#endif

  /* store the server's name - first try a reverse lookup on the
   * address - if that fails us the given name */
  hosts_entry = gethostbyaddr (hosts_entry->h_addr_list[0],
                               strlen (hosts_entry->h_addr_list[0]), AF_INET);
  strcpy (server_name, dest_host);
  if (hosts_entry)
  {
    if (hosts_entry->h_name)
    {
      if (*(hosts_entry->h_name)) strcpy (server_name, hosts_entry->h_name);
    }
  }

  return GEO_TRUE;

}



/*****************************************************************************
 *****************************************************************************
 ****** This section deals with functions for both clients and servers *******
 *****************************************************************************
 *****************************************************************************/

#ifdef WIN32
/*****************************************************************************
 * init_winsock
 *
 * Description: special intialisation needed for Windows socket library
 *
 * Input parameters: none
 * Output parameters: none
 * Returns: TRUE if WinSock initialised OK, FALSE otherwise
 *
 * Comments:
 *
 *****************************************************************************/
int init_winsock (void)

{

  int status;
  WSADATA Data;

  static int winsock_init = GEO_FALSE;


  if (winsock_init) return GEO_TRUE;
  status = WSAStartup (MAKEWORD (2,2), &Data);
  if ( status != 0 ) return GEO_FALSE;
  winsock_init = GEO_TRUE;
  return GEO_TRUE;

}
#endif

/*****************************************************************************
 * get_server_name
 *
 * Description: obtain the name of the server
 *
 * Input parameters: none
 * Output parameters: none
 * Returns: the server name
 *
 * Comments:
 *
 *****************************************************************************/
char *get_server_name (void)

{

  return server_name;

}

/*****************************************************************************
 * get_socket_address
 *
 * Description: get the IP address of the local machine from a socket
 *
 * Input parameters: socket - the socket to get information from
 * Output parameters: address - the address as a long OR NULL
 *                              for no output
 *                    port - the port as an int OR NULL
 *                           for no output
 *                    string - a string version of the address OR NULL
 *                             for no output
 * Returns: TRUE if the information was received OK, FALSE otherwise
 *****************************************************************************/
int get_socket_address (int socket, uint32_t *address, int *port,
                        char *string)

{

  unsigned int length;
  struct sockaddr_in socket_addr;


  length = sizeof (socket_addr);
  if (getsockname (socket, (struct sockaddr *) &socket_addr, &length) != 0)
    return GEO_FALSE;
  if (address)
  {
    memcpy (address, &(socket_addr.sin_addr), sizeof (address));
    *address = ntohl (*address);
  }
  if (port) *port = socket_addr.sin_port;
  if (string) strcpy (string, inet_ntoa (socket_addr.sin_addr));
  return GEO_TRUE;
  
}

/*****************************************************************************
 * close_socket
 *
 * Description: closes a socket neatly
 *
 * Input parameters: sock - the socket to close
 * Output parameters: 
 * Returns: none
 *
 * Comments:
 *
 *****************************************************************************/
void close_socket (int sock)

{

  /* shutdown (sock, 2); */
#ifdef WIN32
  closesocket (sock);
#else
 close (sock);
#endif

}

/*****************************************************************************
 * net_partial_read
 *
 * Description: read from a network handle
 *
 * Input parameters: handle - the handle to read from
 *                   max_len - the max number of bytes to read
 * Output parameters: msg - the message that was read
 * Returns: The number of bytes read OR
 *          NET_DISCONNECT - the network connection was disconnected
 *          NET_ERROR - there was an error reading from the handle
 *          NET_TIMEOUT - the call timed out
 *
 * Comments: The output msg is NOT NULL terminated
 *
 * The number of bytes read may be less than the number requested since
 * a read on a socket will return when some (but not neccessarily all) data
 * is available
 *
 *****************************************************************************/
int net_partial_read (int handle, char *msg, int max_len)

{

  int status;
  fd_set rfd;
  struct timeval timeval;


  /* use select to check data is available */
  FD_ZERO (&rfd);
  FD_SET ((unsigned) handle, &rfd);
  timeval.tv_sec = net_timeout;
  timeval.tv_usec = 0;
  status = select (handle +1, &rfd, 0, 0, &timeval);
  if (status < 0) return NET_ERROR;
  if (status == 0) return NET_TIMEOUT;

  /* read data from the socket */
  status = recv (handle, msg, max_len, 0);
  if (status <= 0)
  {
    if (status > 0) return status;
    if (status < 0) return NET_ERROR;
    return NET_DISCONNECT;
  }

  return status;
  
}

/*****************************************************************************
 * net_read
 *
 * Description: read from a network handle
 *
 * Input parameters: handle - the handle to read from
 *                   max_len - the max number of bytes to read
 * Output parameters: msg - the message that was read
 * Returns: The number of bytes read OR
 *          NET_DISCONNECT - the network connection was disconnected
 *          NET_ERROR - there was an error reading from the handle
 *          NET_TIMEOUT - the call timed out
 *
 * Comments: The output msg is NOT NULL terminated
 *
 *****************************************************************************/
int net_read (int handle, char *msg, int max_len)

{

  int length, status;
  fd_set rfd;
  struct timeval timeval;


  /* a read on a socket will return when some (but not neccessarily all) data
   * is available, so we loop until we have either got all requested data
   * or a timeout / error occurs - if you want to modify this behaviour
   * have a look at the SO_RCVBUF, SO_RCVLOWAT and SO_RCVTIMEO options
   * to the setsockopt() system call */
  length = 0;
  while (length < max_len)
  {
    /* use select to check data is available */
    FD_ZERO (&rfd);
    FD_SET ((unsigned) handle, &rfd);
    timeval.tv_sec = net_timeout;
    timeval.tv_usec = 0;
    status = select (handle +1, &rfd, 0, 0, &timeval);
    if (status < 0) return NET_ERROR;
    if (status == 0) return NET_TIMEOUT;

    /* read data from the socket */
    status = recv (handle, msg + length, max_len - length, 0);
    if (status <= 0)
    {
      if (length) return length;
      if (status < 0) return NET_ERROR;
      return NET_DISCONNECT;
    }
    length += status;
  }

  return length;
  
}

/*****************************************************************************
 * net_read_line
 *
 * Description: read from a network handle up to a terminating char
 *
 * Input parameters: handle - the handle to read from
 *                   max_len - the max number of bytes to read
 *                   term_chars - array of characters that will terminate
 *                                the read if they are received
 *                   term_char_len - length of the term_chars array
 * Output parameters: msg - the message that was read
 * Returns: The number of bytes read OR
 *          NET_DISCONNECT - the network connection was disconnected
 *          NET_ERROR - there was an error reading from the handle
 *          NET_TIMEOUT - the call timed out
 *
 * Comments: The output msg is NOT NULL terminated
 *
 *****************************************************************************/
int net_read_line (int handle, char *msg, int max_len, char *term_chars,
                   int term_char_len)

{

  int count, status, count2;


  for (count=0; count<max_len; count++)
  {
    status = net_read (handle, msg, 1);
    if (status != 1) return status;

    for (count2=0; count2<term_char_len; count2++)
    {
      if (*msg == *(term_chars + count2)) return count;
    }

    msg ++;
  }

  return max_len;

}

/*****************************************************************************
 * net_write
 *
 * Description: write to a network handle
 *
 * Input parameters: handle - the handle to write to
 *                   msg - the message to write
 *                   max_len - the number of bytes to write
 * Output parameters: none
 * Returns: The number of bytes written OR
 *          NET_DISCONNECT - the network connection was disconnected
 *          NET_ERROR - there was an error reading from the handle
 *          NET_TIMEOUT - the call timed out
 *
 * Comments: The output msg is NOT NULL terminated
 *
 *****************************************************************************/
int net_write (int handle, char *msg, int max_len)

{

  int length, status;
  fd_set wfd;
  struct timeval timeval;


  /* use select to check data can be written */
  FD_ZERO (&wfd);
  FD_SET ((unsigned) handle, &wfd);
  timeval.tv_sec = net_timeout;
  timeval.tv_usec = 0;
  status = select (handle +1, 0, &wfd, 0, &timeval);
  if (status < 0) return NET_ERROR;
  if (status == 0) return NET_TIMEOUT;

  /* write data to the socket */
  length = send (handle, msg, max_len, 0);
  if (length < 0) return NET_ERROR;
  if (length == 0) return NET_DISCONNECT;
  return length;
  
}


