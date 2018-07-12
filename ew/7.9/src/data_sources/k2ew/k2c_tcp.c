
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2c_tcp.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
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
/*
 * k2c_tcp.c: K2 packet functions for TCP IO. These routines must have
 *            the same API as other k2c_*.c routines
 *
 *  3/15/00 -- Pete Lombard: File started
 *
 */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <error_ew.h>
#include <socket_ew.h>
#include "glbvars.h"
#include "k2comif.h"
#include "k2misc.h"
#include "k2ewerrs.h"

#define K2C_MAX_FLUSH 1000          /* max count value for 'k2c_flush_recv' */
#define K2C_RD_SOCK_RETRY 5         /* number of times to retry socket */
#define K2C_RD_SOCK_MS 5000         /* msec to wait during socket redo */
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
 * k2c_init_io:  tcp port initialization routine                        *
 *      pgen_io: generalize IO parameter structure                      *
 *      toutms: timeout in milliseconds for connect()                   *
 *      return value:  returns K2R_NO_ERROR if successful               *
 *                     returns K2R_ERROR on error                       *
 *                     or K2R_TIMEOUT                                   *
 ************************************************************************/

int k2c_init_io(s_gen_io *pgen_io, int toutms)
{
  int error;
  unsigned long addr;
  struct hostent* hp;

  if (pgen_io->mode != IO_TCP )
  {
    logit("et", "k2c_init_io: IO mode is not TCP as expected\n");
    return K2R_ERROR;
  }

  /* Initialize the socket system */
  if (sockInit == 0)
  {
    SocketSysInit();   /* This exits on failure */

    /* Set the socket address structure */
    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons((unsigned short)pgen_io->tcp_io.k2_port);

    /* Assume we have an IP address and try to convert to network format.
     * If that fails, assume a domain name and look up its IP address.
     * Can't trust reverse name lookup, since some place may not have their
     * name server properly configured.
     */
    if ( (addr = inet_addr(pgen_io->tcp_io.k2_address)) != INADDR_NONE )
    {
      saddr.sin_addr.s_addr = addr;
    }
    else
    {       /* it's not a dotted quad IP address */
      if ( (hp = gethostbyname(pgen_io->tcp_io.k2_address)) == NULL)
      {
        logit("et", "k2c_init_io: invalid K2 address <%s>\n",
              pgen_io->tcp_io.k2_address);
        return K2R_ERROR;
      }
      memcpy((void *) &saddr.sin_addr, (void*)hp->h_addr, hp->h_length);
    }
    sockInit = 1;
  }

/* Loop until connect() succeeds
   *****************************/
  while ( 1 )
  {
     g_mt_working = 1;     /* Keep sending heartbeats */

     if ( (sfd = socket_ew(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
     {
       ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
       logit("et", "k2c_init_io: error opening socket: %s\n",
             errormsg);
       return K2R_ERROR;
     }

     if (connect_ew(sfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr),
                    toutms) != -1)
        break;                        /* connect() succeeded */

     sfd = INVALID_SOCKET;            /* connect_ew closes socket on error */
     if ( (error = GetLastError_ew()) == CONNECT_WOULDBLOCK_EW)
     {
       logit("et", "k2c_init_io: Timed out making connection to K2\n");
       if ( !gcfg_dont_quit ) return K2R_TIMEOUT;
     }
     else
     {
       ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
       logit("et", "k2c_init_io: Error making TCP connection to K2: %s\n",
             errormsg);
       if ( !gcfg_dont_quit ) return K2R_ERROR;
     }
     sleep_ew( toutms );
  }

  /* Mark our recv_buff as empty */
  rb_count = rb_read = 0;

  /* We're done! */
  return K2R_NO_ERROR;
}

/************************************************************************
 * k2c_close_io:  closes tcp socket opened by 'k2_init_io()'            *
 *                sets the socket to INVALID_SOCKET                     *
 *         return value: returns K2R_NO_ERROR on success,               *
 *                        K2R_ERROR on error                            *
 ************************************************************************/

int k2c_close_io( void )
{
  if (sfd != INVALID_SOCKET)
  {
    if (closesocket( sfd ) == SOCKET_ERROR)
    {
      ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "k2c_init_io: error closing socket: %s\n",
            errormsg);
      sfd = INVALID_SOCKET;
      return K2R_ERROR;
    }
    sfd = INVALID_SOCKET;
  }
  return K2R_NO_ERROR;
}


/**************************************************************************
 * k2c_tran_buff:  transmits buffer of data (up to 65535 bytes) out TCP   *
 *      port; with optional timeout value                                 *
 *      If socket is re-opened, then entire buffer is sent again          *
 *         buff     - address of buffer containing data bytes             *
 *         datalen   - number of bytes from buffer to transmit            *
 *         toutms   - timeout value in milliseconds; (note that the       *
 *                    timeout is reset after each tcp packet is sent)     *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value:  returns the number of bytes successfully           *
 *                     transmitted or K2R_ERROR on error,                 *
 *                     K2R_TIMEOUT on timeout                             *
 **************************************************************************/

int k2c_tran_buff(const unsigned char *buff, int datalen, int toutms, int redo)
{
  int nsent, error, rc;

  if (gcfg_debug > 3)
    logit("e", "entering k2c_tran_buff\n");
  if (redo < 0) redo = 0;  /* just in case... */
  while ( (nsent = send_ew(sfd, (char *)buff, datalen, 0, toutms)) < datalen)
  {
    if ( (error = socketGetError_ew()) == WOULDBLOCK_EW)
    {
      if (redo--)  /* decrement our local copy of redo */
      {     /* Timeout occured; fiddle with the socket */
        if ( (rc = redo_socket()) == K2R_NO_ERROR)
          continue;    /* socket reopened; try sending again */
        else
        {
          if (rc == K2R_ERROR)
            return rc;   /* error or timeout */
          else
            return nsent;
        }
      }
      return K2R_TIMEOUT;
    }
    else
    {
      ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "k2c_tran_buff: Error sending to K2: %s\n",
            errormsg);
      return K2R_ERROR;
    }
  }
  return datalen;
}



/**************************************************************************
 * k2c_rcvflg_tout:  waits for data to be received into TCP port          *
 *         toutms   - a timeout value in milliseconds                     *
 *      return value:  returns K2R_NO_ERROR if data is ready to read      *
 *                returns K2R_TIMEOUT if no data to read within timeout   *
 *                     returns K2R_ERROR on error                         *
 **************************************************************************/

int k2c_rcvflg_tout(int toutms)
{
  int sel_ret;
  fd_set r_set;
  struct timeval tv;

  if (gcfg_debug > 3)
    logit("e", "entering k2c_rcvflg_tout\n");
  FD_ZERO( &r_set );
  FD_SET(sfd, &r_set);
  tv.tv_sec = toutms / 1000;
  tv.tv_usec = (toutms % 1000) * 1000;

  sel_ret = select( sfd+1, &r_set, NULL, NULL, &tv);
  if (sel_ret > 0 && FD_ISSET( sfd, &r_set ))
  {   /* Looks like there's something to receive */
    if (gcfg_debug > 3)
      logit("e", "k2c_rcvflg_tout ret NO_ERROR\n");
    return K2R_NO_ERROR;
  }
  else if (sel_ret == 0)
  {   /* Select timed out */
    if (gcfg_debug > 3)
      logit("e", "k2c_rcvflg_tout ret TIMEOUT\n");
    return K2R_TIMEOUT;
  }
  else
  {   /* Select had a booboo */
    ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "k2c_rcvflg_tout: error in select(): %s\n",
          errormsg);
    return K2R_ERROR;
  }
}

/**************************************************************************
 * k2c_rcvbt_tout:  returns next byte received into TCP port; waits for   *
 *      data ready with timeout                                           *
 *         toutms   - if non-zero then a timeout value in milliseconds;   *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value: returns the received byte (as an int) if successful *
 *                    returns K2R_TIMEOUT if timeout                      *
 *                    returns K2R_ERROR if an error detected              *
 **************************************************************************/

int k2c_rcvbt_tout(int toutms, int redo)
{
  int rc;
  unsigned char bt;

  if (gcfg_debug > 4)
    logit("e", "entering k2c_rcvbt_tout\n");
  if (redo < 0) redo = 0;
  while (1)
  {
    if (rb_count <= rb_read)  /* Is the recv_buffer empty? */
    {            /* Yes, fill it up */
      if ( (rc = GetMoreBytes(RB_LEN, toutms)) != K2R_NO_ERROR)
      {
        if (rc == K2R_ERROR)
          return rc;
        else if (redo--)  /* decrement our local copy of redo */
        {     /* Timeout occured; fiddle with the socket */
          if ( (rc = redo_socket()) == K2R_NO_ERROR) {
            if (gcfg_force_blkmde) 
	      k2mi_force_blkmde();	/* newly added for modem control regrab, if told to */
            continue;    /* socket reopened; try for more */
	  } else {
            return rc;   /* error or timeout */
          }
        }
        else  /* no more redo's allowed, return the timeout */
          return rc;
      }
    }
    bt = recv_buff[rb_read++];
    return (int) bt;          /* return received data byte */
  }
}


/* ************************************************************************
 * k2c_recv_buff:  fills buffer with received data; waits for given       *
 *      number of bytes to be received; with optional timeout             *
 *         rbuff    - address of buffer to receive data bytes             *
 *         datalen  - number of data bytes to receive                     *
 *         toutms   - timeout value in milliseconds;                      *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value: returns the number of data bytes received and       *
 *                    placed into 'rbuff[]'                               *
 *                    returns K2R_ERROR on error                          *
 *                    returns K2R_TIMEOUT if no data read before timeout  *
 **************************************************************************/

int k2c_recv_buff(unsigned char *rbuff, int datalen, int toutms, int redo)
{
  int bcount = 0;
  int to_read, to_copy, rc;

  if (gcfg_debug > 3)
    logit("e", "entering k2c_recv_buff\n");
  if (redo < 0) redo = 0;  /* just in case... */
  while (bcount < datalen)
  {
    if (rb_count <= rb_read)  /* Is the recv_buffer empty? */
    {    /* Yes, fill it up */
      to_read = (datalen - bcount < RB_LEN) ? datalen - bcount : RB_LEN;
      if ( (rc = GetMoreBytes(to_read, toutms)) != K2R_NO_ERROR)
      {
        if (rc == K2R_ERROR)
          return rc;
        else if (redo--)  /* decrement our local copy of redo */
        {     /* Timeout occured; fiddle with the socket */
          if ( (rc = redo_socket()) == K2R_NO_ERROR) {
            if (gcfg_force_blkmde) 
	      k2mi_force_blkmde();	/* newly added for modem control regrab, if told to */
            continue;    /* socket reopened; try for more */
          } else {
            if (rc == K2R_ERROR || bcount == 0)
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


/**************************************************************************
 * k2c_flush_recv:  flushes out the receive buffer by reading in and      *
 *      discarding incoming data (up to 'K2C_MAX_FLUSH' bytes); with      *
 *      optional timeout                                                  *
 *         toutms - timeout value in milliseconds                         *
 *                  during which any incoming data will be flushed;       *
 *                  if zero then no timeout is used and only the          *
 *                  data already received is flushed                      *
 *      return value:  returns 0 if no error; -1 on error                 *
 **************************************************************************/

int k2c_flush_recv(int toutms)
{
  int bcount = 0;
  int to_read, rc;

  if (gcfg_debug > 3)
    logit("e", "entering k2c_flush_recv; TO %d\n", toutms);
  rb_read = rb_count = 0;
  if (toutms <= 0)
    bcount = K2C_MAX_FLUSH - 1;

  while (bcount < K2C_MAX_FLUSH)
  {
    to_read = (K2C_MAX_FLUSH - bcount < RB_LEN) ? K2C_MAX_FLUSH - bcount :
      RB_LEN;
    rc = GetMoreBytes(to_read, toutms);
    if (gcfg_debug > 3)
      logit("e", "GetMoreBytes returned %d to go: %d\n", rc, K2C_MAX_FLUSH - bcount);
    if ( rc == K2R_ERROR)
      return rc;
    else if (rc == K2R_TIMEOUT)
      return K2R_NO_ERROR;

    bcount += rb_count;
  }
  return K2R_NO_ERROR;
}


/************************************************************************
 * GetMoreBytes: an internal function for filling the static recv_buff. *
 *           to_read: number of bytes to recv; rb_count is set to       *
 *               number of bytes actually read; rb_read is set to 0     *
 *            toutms: if non-zero then a timeout value in milliseconds; *
 *                    if zero then no timeout is used.                  *
 *           returns: K2R_NO_ERROR on success, K2R_ERROR on error       *
 *                    or K2R_TIMEOUT on timeout                         *
 ************************************************************************/

static int GetMoreBytes(int to_read, int toutms)
{
  int n_read, error;

  if (to_read < 1 || to_read > RB_LEN)
  {
    logit("et", "GetMoreBytes: invalid read request <%d>\n", to_read);
    return K2R_ERROR;
  }
  rb_read = rb_count = 0;

  if ( (n_read = recv_ew(sfd, (char *)recv_buff, to_read, 0, toutms)) <= 0)
  {
    /* For NT, added test for n_read == 0 as proxy for timeout */
    if ( n_read == 0 || (error = socketGetError_ew()) == WOULDBLOCK_EW)
      return K2R_TIMEOUT;
    else
    {
      ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "GetMoreBytes: error receiving: %s\n",
            errormsg);
      return K2R_ERROR;
    }
  }
  if (gcfg_debug > 4)
    logit("e", "GMB %d\n", n_read);
  rb_count = n_read;
  return K2R_NO_ERROR;
}


/****************************************************************
 *  redo_socket: closes and reopens and connects the socket to  *
 *               the address set by k2c_init_io(). Waits on     *
 *               connection for K2C_RD_SOCK_MS milliseconds;    *
 *               tries K2C_RD_SOCK_RETRY times if connections   *
 *               time out; sets socket sfd to INVALID_SOCKET on *
 *               failure.                                       *
 *     return value: returns K2R_NO_ERROR on success;           *
 *               K2R_ERROR on error, or                         *
 *               K2R_TIMEOUT on final timeout                   *
 ****************************************************************/
static int redo_socket()
{
  int retry = K2C_RD_SOCK_RETRY;
  int error;

  if ( gcfg_debug > 0 )
     logit("t", "Redoing socket\n");

  while ( 1 )
  {
    if ( !gcfg_dont_quit )
       if ( --retry ) break;

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
        return K2R_ERROR;
      }
      sfd = INVALID_SOCKET;
    }
    sleep_ew(K2C_RD_SOCK_MS);

    if (gcfg_debug > 1)
      logit("et", "Opening socket\n");

    if ( (sfd = socket_ew(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
    {
      ew_fmt_err_msg(socketGetError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "redo_socket: Error opening socket: %s\n",
            errormsg);
      return K2R_ERROR;
    }

    if (connect_ew(sfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr),
                   K2C_RD_SOCK_MS) == -1)
    {
      sfd = INVALID_SOCKET;          /* connect_ew closes socket on error */
      if ( (error = socketGetError_ew()) != CONNECT_WOULDBLOCK_EW)
      {
        ew_fmt_err_msg(error, errormsg, MAX_ERR_MSG);
        logit("et", "redo_socket: Error reconnecting to K2: %d %s\n",
              error, errormsg);
        if ( !gcfg_dont_quit ) return K2R_ERROR;
        if ( g_terminate_flg ) return K2R_ERROR;
      } /* connection timed out; try again */
    }
    else   /* connection made, return OK */
    {
      logit("et", "redo_socket: Successful reconnection to K2\n");
      return K2R_NO_ERROR;
    }
  }
  /* Too many retries; give up */
  logit("et", "redo_socket: Timed out reconnecting to K2\n");
  return K2R_TIMEOUT;
}

