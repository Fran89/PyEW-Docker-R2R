/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2c_ser_un.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2003/05/29 13:33:40  friberg
 *     cleaned up some warnings
 *
 *     Revision 1.4  2001/02/14 19:36:01  lombard
 *     Debug logging and bugfix to getmorebytes()
 *
 *     Revision 1.3  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.2  2000/05/09 23:58:54  lombard
 *     Added restart mechanism
 *
 *     Revision 1.1  2000/05/04 23:47:56  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * k2c_ser.c: K2 packet functions for serial IO on Unix. 
 *   These routines must have the same API as k2c_ser_nt.c and k2c_tcp.c.
 *
 *  3/24/00 -- Pete Lombard: File started
 *
 */

#include <earthworm.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_LINUX) || defined(_MACOSX)
#include <sys/ioctl.h>
#else 
#include <sys/filio.h>           /* For FIONBIO on Solaris */
#endif
#include "glbvars.h"
#include "k2comif.h"
#include "k2ewerrs.h"

#define K2C_MAX_FLUSH 1000       /* max count value for 'k2c_flush_recv' */

int tfd = -1;                   /* Our tty port */

/* A buffer for recv functions and its counters */
static unsigned char recv_buff[RB_LEN];
static int rb_read = 0;  /* Where to start reading from recv_buff */
static int rb_count = 0; /* Number of bytes in recv_buff; note: we only write *
                       * (by calling recv()) into recv_buff when it is empty */

/* Internal function prototypes; */
static int GetMoreBytes(int, int);


/************************************************************************
 * k2c_init_io:  tcp port initialization routine                        *
 *      gen_io: generalize IO parameter structure                       *
 *      toutms: timeout in milliseconds; not used                       *
 *      return value:  returns K2R_NO_ERROR if successful               *
 *                     returns K2R_ERROR on error                       *
 *                     or K2R_TIMEOUT                                   *
 ************************************************************************/

int k2c_init_io(s_gen_io *gen_io, int toutms)
{
  int32_t lOnOff;
  struct termios modes;
  int baud;
  
  if (gen_io->mode != IO_TTY_UN )
  {
    logit("et", "k2c_init_io: IO mode is not TTY as expected\n");
    return K2R_ERROR;
  }

  /* open the TTY device, if it isn't already */
  if (tfd < 0)
    tfd = open( gen_io->tty_io.ttyname, O_RDWR );
  if ( tfd < 0 ) 
  {
    logit("et", "k2c_init_io: error opening TTY port <%s>: %s\n", 
          gen_io->tty_io.ttyname, strerror(errno));
    return K2R_ERROR;
  }

  /* Get current settings so we can mess with them */
  if (tcgetattr(tfd, &modes) < 0) 
  {
    close(tfd);
    tfd = -1;
    logit("et", "k2c_init_io: get tty attribute error: %s\n", 
          strerror(errno));
    return K2R_ERROR;
  }

  /* Turn off echo, canonical, extended input, and keyboard signals */
  modes.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  /* Turn off interrupt on break, cr mapping, parity, strip and SW flow control */
  modes.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IXOFF);
  /* Turn off size mask and parity */
  modes.c_cflag &= ~(CSIZE | PARENB);
  /* Set 8-bit size */
  modes.c_cflag |= CS8;
  /* Turn off output processing */
  modes.c_oflag &= ~(OPOST);
  /* Enable character-at-a-time input */
  modes.c_cc[VMIN] = 1;
  modes.c_cc[VTIME] = 0;
  
  switch(gen_io->tty_io.speed)
  {
  case 110: baud = B110; break;
  case 150: baud = B150; break;
  case 300: baud = B300; break;
  case 1200: baud = B1200; break;
  case 2400: baud = B2400; break;
  case 4800: baud = B4800; break;
  case 9600: baud = B9600; break;
  case 19200: baud = B19200; break;
  case 38400: baud = B38400; break;
/* Extended baud rates -- beyond those defined in POSIX.1 (57600 and above) */
#ifdef B57600
  case 57600: baud = B57600; break;
#endif
#ifdef B115200
  case 115200: baud = B115200; break;
#endif
#ifdef B230400
  case 230400: baud = B230400; break;
#endif
 
  default:
    close(tfd);
    tfd = -1;
    logit("et", "k2c_init_io: bad TTY speed %d\n", gen_io->tty_io.speed);
    return K2R_ERROR;
  }
  
  cfsetospeed(&modes, baud);
  cfsetispeed(&modes, baud);
  
  /* Make the actual settings */
  if (tcsetattr(tfd, TCSAFLUSH, &modes) < 0) 
  {
    close(tfd);
    tfd = -1;
    logit("et", "k2c_init_io: error setting tty attributes: %s\n", 
          strerror(errno));
    return K2R_ERROR;
  }
  
  /* Now set the tty to non-blocking mode */
  lOnOff = 1;  /* on */
  if (ioctl(tfd, FIONBIO, &lOnOff) < 0)
  {
    close(tfd);
    tfd = -1;
    logit("et", "k2c_init_io: ioctl error: %s\n", strerror(errno));
    return K2R_ERROR;
  }

  return K2R_NO_ERROR;
}


/************************************************************************
 * k2c_close_io:  closes tty port opened by 'k2c_init_io()'             *
 *         return value: returns K2R_NO_ERROR on success,               *
 *                        K2R_ERROR on error                            *
 ************************************************************************/

int k2c_close_io( void )
{
  int rc = K2R_NO_ERROR;
  
  if ( tfd > 0 )
    if ( close( tfd ) < 0)
    {
      logit("et", "k2c_close_io: error closing tty (%d): %s\n", tfd, 
            strerror(errno));
      rc = K2R_ERROR;
    }
  
  tfd = -1;   /* Mark the tty as closed */
  return rc;
}

/**************************************************************************
 * k2c_tran_buff:  transmits buffer of data (up to 65535 bytes) out TTY   *
 *      port; with optional timeout value                                 *
 *         buff     - address of buffer containing data bytes             *
 *         datalen   - number of bytes from buffer to transmit            *
 *         toutms   - timeout value in milliseconds; (note that the       *
 *                    timeout is reset after each tcp packet is sent)     *
 *         redo     - not used for TTY comms                              *
 *      return value:  returns the number of bytes successfully           *
 *                     transmitted or K2R_ERROR on error,                 *
 *                     K2R_TIMEOUT on timeout                             *
 **************************************************************************/

int k2c_tran_buff(const unsigned char *buff, int datalen, int toutms, int redo)
{
  int pos = 0;
  int nsent, rc;
  fd_set w_set;
  struct timeval tv;
  
  if (gcfg_debug > 3)
    logit("e", "k2c_tran_buff %d\n", datalen);
  
  while( pos < datalen)
  {
    if ( (nsent = write(tfd, (char *)&buff[pos], datalen - pos)) 
         < 0)
    {
      if (errno == EAGAIN)
      {
        tv.tv_sec = toutms / 1000;
        tv.tv_usec = (toutms % 1000) * 1000;
        FD_ZERO(&w_set);
        FD_SET(tfd, &w_set);
      
        rc = select( tfd+1, NULL, &w_set, NULL, &tv);
        if (rc > 0 && FD_ISSET( tfd, &w_set ))
        {
          if ( (nsent = write(tfd, (char *)&buff[pos], datalen - pos)) 
               < 0)
          {
            logit("et", "k2c_tran_buff: write error: %s\n", strerror(errno));
            return K2R_ERROR;
          }
        }
        else if (rc < 0)
        {
          logit("et", "k2c_tran_buff: select error: %s\n", strerror(errno));
          return K2R_ERROR;
        }
        else
          return K2R_TIMEOUT;
      }
      else
      {
        logit("et", "k2c_tran_buff: write error: %s\n", strerror(errno));
        return K2R_ERROR;
      }
    }
    pos += nsent;          /* increment to next buffer position */
  }

  return pos;                /* return # of bytes sent */
}

/**************************************************************************
 * k2c_rcvflg_tout:  waits for data to be received into TTY port          *
 *         toutms   - a timeout value in milliseconds                     *
 *      return value:  returns K2R_NO_ERROR if data is ready to read      *
 *                returns K2R_TIMEOUT if no data to read within timeout   *
 *                     returns K2R_ERROR on error                         *
 **************************************************************************/

int k2c_rcvflg_tout(int toutms)
{
  int sel_ret;
  fd_set r_set;
  struct timeval tv = {0,0};
  
  if (gcfg_debug > 3)
    logit("e", "k2c_rcvflg_tout TO %d\n", toutms);
  
  FD_ZERO( &r_set );
  FD_SET(tfd, &r_set);
  tv.tv_sec = toutms / 1000;
  tv.tv_usec = (toutms % 1000) * 1000;
  
  sel_ret = select( tfd+1, &r_set, NULL, NULL, &tv);
  if (sel_ret > 0 && FD_ISSET( tfd, &r_set ))
  {   /* Looks like there's something to receive */
    if (gcfg_debug > 3)
      logit("e", "k2c_rcvflg_tout ret 0\n");
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
    logit("et", "k2c_rcvflg_tout: error in select(): %s\n", strerror(errno));
    return K2R_ERROR;
  }
}

/**************************************************************************
 * k2c_rcvbt_tout:  returns next byte received into TTY port; waits for   *
 *      data ready with timeout                                           *
 *         toutms   - if non-zero then a timeout value in milliseconds;   *
 *         redo     - not used for TTY comms                              *
 *      return value: returns the received byte (as an int) if successful *
 *                    returns K2R_TIMEOUT if timeout                      *
 *                    returns K2R_ERROR if an error detected              *
 **************************************************************************/

int k2c_rcvbt_tout(int toutms, int redo)
{
  int rc;
  unsigned char bt;

  if (gcfg_debug > 4)
    logit("e", "k2c_rcvbt_tout\n");
  
  if (rb_count <= rb_read)  /* Is the recv_buffer empty? */
  {            /* Yes, fill it up */
    if ( (rc = GetMoreBytes(RB_LEN, toutms)) != K2R_NO_ERROR)
      return rc;
  }
  bt = recv_buff[rb_read++];
  return (int) bt;          /* return received data byte */
}


/**************************************************************************
 * k2c_recv_buff:  fills buffer with received data; waits for given       *
 *      number of bytes to be received; with optional timeout             *
 *         rbuff    - address of buffer to receive data bytes             *
 *         datalen  - number of data bytes to receive                     *
 *         toutms   - timeout value in milliseconds;                      *
 *         redo     - not used for TTY comms                              *
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
    logit("e", "k2c_recv_buff %d\n", datalen);
  
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
        else  /* TIMEOUT, return count if we got any */
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
    logit("e", "k2c_flush_recv TO %d\n", toutms);
  
  rb_read = rb_count = 0;
  if (toutms <= 0)
    bcount = K2C_MAX_FLUSH - 1;
  
  while (bcount < K2C_MAX_FLUSH)
  {
    to_read = (K2C_MAX_FLUSH - bcount < RB_LEN) ? K2C_MAX_FLUSH - bcount : 
      RB_LEN;
    /*  if ( (rc = GetMoreBytes(to_read, toutms)) == K2R_ERROR) */
    rc = GetMoreBytes(to_read, toutms);
    if (gcfg_debug > 3)
      logit("e", "GMB ret %d rb %d\n", rc, rb_count);
    if (rc == K2R_ERROR)
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
  int n_read, sel_ret;
  fd_set r_set;
  struct timeval tv, *ptv;
  
  if (gcfg_debug > 3)
    logit("et", "enter GMB TO %d\n", toutms);
  
  if (to_read < 1 || to_read > RB_LEN)
  {
    logit("et", "GetMoreBytes: invalid read request <%d>\n", to_read);
    return K2R_ERROR;
  }
  rb_read = rb_count = 0;
  
  if ( (n_read = read(tfd, (char *)recv_buff, to_read)) <= 0)
  {
    if ( n_read < 0 && errno != EAGAIN)
    {
      rb_count = 0;
      logit("et", "GetMoreBytes: read error: %s\n", strerror(errno));
      return K2R_ERROR;
    }
    else
    {  /* Would have blocked, so we will select... */
      ptv = &tv;
      tv.tv_sec = toutms / 1000;
      tv.tv_usec = (toutms % 1000) * 1000;
      
      FD_ZERO( &r_set );
      FD_SET(tfd, &r_set);
      
      if (gcfg_debug > 3)
        logit("et", "GMB selecting %lu.%lu\n", tv.tv_sec, tv.tv_usec);
      
      sel_ret = select( tfd+1, &r_set, NULL, NULL, ptv);
      
      if (gcfg_debug > 3)
        logit("et", "GMB sel ret %d\n", sel_ret);
      
      if (sel_ret > 0 && FD_ISSET( tfd, &r_set ))
      {   /* Maybe there's something to receive */
        if ( (n_read = read(tfd, (char *)recv_buff, to_read)) < 0)
        {   /* We had a another booboo; wrap up and go home */
          logit("et", "GetMoreBytes: read error: %s\n", strerror(errno));
          return K2R_ERROR;
        }
        rb_count = n_read;   /* Record how much is in recv_buff */
        if (gcfg_debug > 3)
          logit("e", "GMB: %d\n", rb_count);
        return K2R_NO_ERROR;
      }
      else if (sel_ret == 0)
      {   /* Select timed out */
        if (gcfg_debug > 3)
          logit("e", "GMB timeout\n");
        return K2R_TIMEOUT;
      }
      else
      {   /* Select had a booboo */
        logit("et", "GetMoreBytes: select error: %s\n", strerror(errno));
        return K2R_ERROR;
      }
    }
    
  }
  /* We recv'ed ok the first time */
    rb_count = n_read;
    if (gcfg_debug > 3)
      logit("e", "GMB: %d\n", rb_count);
    return K2R_NO_ERROR;
  }
        



