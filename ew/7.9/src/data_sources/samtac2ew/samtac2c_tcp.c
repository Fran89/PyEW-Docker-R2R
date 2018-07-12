
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac2c_tcp.c 3536 2009-01-15 22:09:51Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.6  2009/01/13 17:12:53  tim
 *     Clean up source
 *
 *     Revision 1.5  2009/01/13 15:41:27  tim
 *     Removed more k2 references
 *
 *     Revision 1.4  2009/01/12 20:52:32  tim
 *     Removing K2 references
 *
 *     Revision 1.3  2008/10/29 21:12:36  tim
 *     error handling
 *
 *     Revision 1.2  2008/10/29 15:48:26  tim
 *     renamed outputs for samtac instead of k2
 *
 *     Revision 1.1  2008/10/21 22:52:51  tim
 *     *** empty log message ***
 *
 *     Revision 1.11  2007/12/18 13:36:03  paulf
 *     version 2.43 which improves modem handling for new kmi firmware on k2 instruments
 *
 *     Revision 1.10  2007/05/10 00:20:14  dietz
 *     included <string.h> to fix missing prototypes
 *
 *     Revision 1.9  2003/08/22 20:12:13  friberg
 *     added check for terminate flag to redo_socket in k2c_tcp.c
 *     to prevent k2ew_tcp from taking too long to exit when in this
 *     function call and a terminate signal arrives.
 *
 *     Revision 1.8  2002/05/06 18:27:43  kohler
 *     Added line to function k2c_init_io() so that heartbeats will be sent
 *     to statmgr which k2ew is attempting to make a socket connection to
 *     the K2.
 *
 *     Revision 1.7  2001/05/08 00:14:53  kohler
 *     Minor logging changes.
 *
 *     Revision 1.6  2000/11/29 20:35:33  kohler
 *     When DontQuit parameter is set, program no longer exits
 *     when it can't connect to the MSS100.  Instead, it retries
 *     forever.
 *
 *     Revision 1.5  2000/08/30 17:32:46  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.4  2000/07/03 18:00:37  lombard
 *     Added code to limit age of waiting packets; stops circ buffer overflows
 *     Added and Deleted some config params.
 *     Added check of K2 station name against restart file station name.
 *     See ChangeLog for complete list.
 *
 *     Revision 1.3  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.2  2000/05/16 23:39:16  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.1  2000/05/04 23:47:57  lombard
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <error_ew.h>
#include <socket_ew.h>
#include "glbvars.h"
#include "samtac_comif.h"
#include "samtac2ew_errs.h"

#define SAMTAC_RD_SOCK_RETRY 5         /* number of times to retry socket */
#define SAMTAC_RD_SOCK_MS 5000         /* msec to wait during socket redo */
#define MAX_ERR_MSG  128

static char errormsg[MAX_ERR_MSG];  /* error message buffer */

static SOCKET sfd;                  /* Our socket */
struct sockaddr_in saddr;           /* socket address structure  */
static int sockInit = 0;            /* Socket System init flag */

/* A buffer for recv functions and its counters */
static unsigned char recv_buff[RB_LEN];
static int rb_read = 0;  /* Where to start reading from recv_buff */
static int rb_count = 0; /* Number of bytes in recv_buff; note: we only write *
                       * (by calling recv()) into recv_buff when it is empty */

/* Internal function prototypes; */
static int GetMoreBytes(int, int);
static int redo_socket();

/************************************************************************
 * samtac_init_io:  tcp port initialization routine                        *
 *      gen_io: generalize IO parameter structure                       *
 *      toutms: timeout in milliseconds for connect()                   *
 *      return value:  returns SAMTAC2R_NO_ERROR if successful               *
 *                     returns SAMTAC2R_ERROR on error                       *
 *                     or SAMTAC2R_TIMEOUT                                   *
 ************************************************************************/

int samtac_init_io(s_gen_io *gen_io, int toutms)
{
	
  int addr, error;
  struct hostent* hp;
  if (gen_io->mode != IO_TCP )
  {
    logit("et", "samtac_init_io: IO mode is not TCP as expected\n");
    return SAMTAC2R_ERROR;
  }

  /* Initialize the socket system */
  if (sockInit == 0)
  {

    SocketSysInit();   /* This exits on failure */

    /* Set the socket address structure */
    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons((unsigned short)gen_io->tcp_io.samtac_port);

    /* Assume we have an IP address and try to convert to network format.
     * If that fails, assume a domain name and look up its IP address.
     * Can't trust reverse name lookup, since some place may not have their
     * name server properly configured.
     */
    if ( (addr = inet_addr(gen_io->tcp_io.samtac_address)) != INADDR_NONE )
    {
      saddr.sin_addr.s_addr = addr;
    }
    sockInit = 1;
  }

/* Loop until connect() succeeds
   *****************************/
  while ( 1 )
  {
    // g_mt_working = 1;     /* Keep sending heartbeats */

     if ( (sfd = socket_ew(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
     {
       ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
       logit("et", "samtac2ew_init_io: error opening socket: %s\n",
             errormsg);
       return SAMTAC2R_ERROR;
     }

     if (connect_ew(sfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr),
                    toutms) != -1)
        break;                        /* connect() succeeded */

     sfd = INVALID_SOCKET;            /* connect_ew closes socket on error */
     if ( (error = GetLastError_ew()) == CONNECT_WOULDBLOCK_EW)
     {
       logit("et", "samtac2ew_init_io: Timed out making connection to SAMTAC\n");
       return SAMTAC2R_TIMEOUT;
     }
     else
     {
       ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
       logit("et", "samtac2ew_init_io: Error making TCP connection to SAMTAC: %s\n",
             errormsg);
       return SAMTAC2R_ERROR;
     }
     sleep_ew( toutms );
  }

  /* Mark our recv_buff as empty */
  rb_count = rb_read = 0;
	  /* We're done! */
  return SAMTAC2R_NO_ERROR;
}

/************************************************************************
 * samtac_close_io:  closes tcp socket opened by 'samtac_init_io()'            *
 *                sets the socket to INVALID_SOCKET                     *
 *         return value: returns SAMTAC2R_NO_ERROR on success,               *
 *                        SAMTAC2R_ERROR on error                            *
 ************************************************************************/

int samtac_close_io( void )
{
  if (sfd != INVALID_SOCKET)
  {
    if (closesocket( sfd ) == SOCKET_ERROR)
    {
      ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "samtac2ew_init_io: error closing socket: %s\n",
            errormsg);
      sfd = INVALID_SOCKET;
      return SAMTAC2R_ERROR;
    }
    sfd = INVALID_SOCKET;
  }
  return SAMTAC2R_NO_ERROR;
}


/* ************************************************************************
 * samtac_recv_buff:  fills buffer with received data; waits for given       *
 *      number of bytes to be received; with optional timeout             *
 *         rbuff    - address of buffer to receive data bytes             *
 *         datalen  - number of data bytes to receive                     *
 *         toutms   - timeout value in milliseconds;                      *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value: returns the number of data bytes received and       *
 *                    placed into 'rbuff[]'                               *
 *                    returns SAMTAC2R_ERROR on error                          *
 *                    returns SAMTAC2R_TIMEOUT if no data read before timeout  *
 **************************************************************************/

int samtac_recv_buff(unsigned char *rbuff, int datalen, int toutms, int redo)
{
  int bcount = 0;
  int to_read, to_copy, rc;

  if (gcfg_debug > 3)
    logit("e", "entering samtac2ew_recv_buff\n");
  if (redo < 0) redo = 0;  /* just in case... */
  while (bcount < datalen)
  {
    if (rb_count <= rb_read)  /* Is the recv_buffer empty? */
    {    /* Yes, fill it up */
      to_read = (datalen - bcount < RB_LEN) ? datalen - bcount : RB_LEN;
      if ( (rc = GetMoreBytes(to_read, toutms)) != SAMTAC2R_NO_ERROR)
      {
        if (rc == SAMTAC2R_ERROR)
          return rc;
        else if (redo--)  /* decrement our local copy of redo */
        {     /* Timeout occured; fiddle with the socket */
          if ( (rc = redo_socket()) == SAMTAC2R_NO_ERROR) {
            continue;    /* socket reopened; try for more */
          } else {
            if (rc == SAMTAC2R_ERROR || bcount == 0)
              return rc;   /* error or timeout */
            else
              return bcount;
          }
        }
        else  /* no more redo's allowed, return TIMEOUT */
          return ( (bcount > 0) ? bcount : rc);
      }
    }
    /* Copy from recv_buff into our output buffer *
     * Don't exceed capacity of either buffer     */
    to_copy = (rb_count - rb_read < datalen - bcount) ?
      rb_count - rb_read : datalen - bcount;
    memcpy(&rbuff[bcount], &recv_buff[rb_read], to_copy);
    rb_read += to_copy;
    bcount += to_copy;
  }
  return bcount;
}

/************************************************************************
 * GetMoreBytes: an internal function for filling the static recv_buff. *
 *           to_read: number of bytes to recv; rb_count is set to       *
 *               number of bytes actually read; rb_read is set to 0     *
 *            toutms: if non-zero then a timeout value in milliseconds; *
 *                    if zero then no timeout is used.                  *
 *           returns: SAMTAC2R_NO_ERROR on success, SAMTAC2R_ERROR on error       *
 *                    or SAMTAC2R_TIMEOUT on timeout                         *
 ************************************************************************/

static int GetMoreBytes(int to_read, int toutms)
{
  int n_read, error;

  if (to_read < 1 || to_read > RB_LEN)
  {
    logit("et", "GetMoreBytes: invalid read request <%d>\n", to_read);
    return SAMTAC2R_ERROR;
  }
  rb_read = rb_count = 0;

  if ( (n_read = recv_ew(sfd, (char *)recv_buff, to_read, 0, toutms)) <= 0)
  {
    /* For NT, added test for n_read == 0 as proxy for timeout */
    if ( n_read == 0 || (error = socketGetError_ew()) == WOULDBLOCK_EW)
      return SAMTAC2R_TIMEOUT;
    else
    {
      ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "GetMoreBytes: error receiving: %s\n",
            errormsg);
      return SAMTAC2R_ERROR;
    }
  }
  if (gcfg_debug > 4)
    logit("e", "GMB %d\n", n_read);
  rb_count = n_read;
  return SAMTAC2R_NO_ERROR;
}


/****************************************************************
 *  redo_socket: closes and reopens and connects the socket to  *
 *               the address set by samtac_init_io(). Waits on     *
 *               connection for SAMTAC_RD_SOCK_MS milliseconds;    *
 *               tries SAMTAC_RD_SOCK_RETRY times if connections   *
 *               time out; sets socket sfd to INVALID_SOCKET on *
 *               failure.                                       *
 *     return value: returns SAMTAC2R_NO_ERROR on success;           *
 *               SAMTAC2R_ERROR on error, or                         *
 *               SAMTAC2R_TIMEOUT on final timeout                   *
 ****************************************************************/
static int redo_socket()
{
  int retry = SAMTAC_RD_SOCK_RETRY;
  int error;

  if ( gcfg_debug > 0 )
     logit("t", "Redoing socket\n");

  while ( 1 )
  {
    if (sfd != INVALID_SOCKET)
    {
      if (gcfg_debug > 1)
        logit("et", "Closing socket\n");
      if (closesocket(sfd) == SOCKET_ERROR)
      {
        ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
        logit("et", "redo_socket: Error closing socket: %s\n",
              errormsg);
        sfd = INVALID_SOCKET;
        return SAMTAC2R_ERROR;
      }
      sfd = INVALID_SOCKET;
    }
    sleep_ew(SAMTAC_RD_SOCK_MS);

    if (gcfg_debug > 1)
      logit("et", "Opening socket\n");

    if ( (sfd = socket_ew(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
    {
      ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "redo_socket: Error opening socket: %s\n",
            errormsg);
      return SAMTAC2R_ERROR;
    }

    if (connect_ew(sfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr),
                   SAMTAC_RD_SOCK_MS) == -1)
    {
      sfd = INVALID_SOCKET;          /* connect_ew closes socket on error */
      if ( (error = socketGetError_ew()) != CONNECT_WOULDBLOCK_EW)
      {
        ew_fmt_err_msg(error, errormsg, MAX_ERR_MSG);
        logit("et", "redo_socket: Error reconnecting to SAMTAC: %d %s\n",
              error, errormsg);
        if ( g_terminate_flg ) return SAMTAC2R_ERROR;
      } /* connection timed out; try again */
    }
    else   /* connection made, return OK */
    {
      logit("et", "redo_socket: Successful reconnection to SAMTAC\n");
      return SAMTAC2R_NO_ERROR;
    }
  }
  /* Too many retries; give up */
  logit("et", "redo_socket: Timed out reconnecting to SAMTAC\n");
  return SAMTAC2R_TIMEOUT;
}

