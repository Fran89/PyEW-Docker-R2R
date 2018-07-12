/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2c_ser_nt.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2000/07/28 22:36:10  lombard
 *     Moved heartbeats to separate thread; added DontQuick command; removed
 *     redo_com() since it doesn't do any good; other minor bug fixes
 *
 *     Revision 1.2  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.1  2000/05/04 23:47:53  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * k2c_ser_nt.c:  K2 packet functions to COM port I/O interface routines;
 *             all direct references to platform-specific COM port
 *             functions should be in this module
 *
 * 12/30/98 -- [ET]  File started
 *  3/15/00 -- Pete Lombard: changed named from k2comif.c
 *             Made function calls more generic so that this file
 *             could be replaced by one that does socket or Unix serial comms.
 */

#include <stdio.h>
#include <windows.h>
#include <earthworm.h>
#include <error_ew.h>
#include <time.h>            /* for 'clock()' function       */
#include "glbvars.h"
#include "k2comif.h"         /* general header file for IO   */
#include "k2ewerrs.h"

#define K2C_DEF_COM_TO  500      /* default COM port timeout in msec */
#define K2C_MAX_FLUSH   1000     /* max count value for 'k2c_flush_recv' */
#define K2C_RD_COMM_MS  5000     /* msec to wait during socket redo */
#define MAX_ERR_MSG     128      /* Max length of an error message */

static char errormsg[MAX_ERR_MSG];  /* error message buffer */

/*  macro to convert time in milliseconds to system ticks */
#define MS_TO_TICKS(numms) (((clock_t)(numms) * \
                                         ((short)(CLK_TCK*10.0+0.5)))/10000)
/*  macros to setup and check timing delays */
#define MARK_TIMING(numms) (g_timing_var = clock() + MS_TO_TICKS(numms))
#define DONE_TIMING() (clock() > g_timing_var)

HANDLE g_handle = NULL;   /* port handles, NULL=not open */
static char errormsg[MAX_ERR_MSG];  /* error message buffer */

static clock_t g_timing_var=0;       /* variable used by '_TIMING()' macros */
static DCB g_dcb;
static COMMTIMEOUTS g_ctoblk;
static char g_comstr[16];

/* A buffer for recv functions and its counters */
unsigned char recv_buff[RB_LEN];
int rb_read = 0;     /* Where to start reading from recv_buff */
int rb_count = 0;    /* Number of bytes in recv_buff; note that we only write *
                      * (by calling ReadFile()) into recv_buff when it is     *
                      * empty   */

/* Internal function prototypes; */
static int GetMoreBytes( int );
static int redo_comm( void );  /* Not used */
static int set_timing( int );

/**********************************************************
 * k2c_init_io:  serial port initialization function      *
 *      pgen_io: generalize IO parameter structure        *
 *      toutms: not used                                  *
 *      return value:  returns K2R_NO_ERROR if successful *
 *                     returns K2R_ERROR on error         *
 **********************************************************/

int k2c_init_io(s_gen_io *pgen_io, int toutms)
{
  if (pgen_io->mode != IO_COM_NT)
  {
    logit("et", "k2c_init_io: IO mode is not COMM as expected\n");
    return K2R_ERROR;
  }

  /* build "\\.\COMx" string */
  sprintf(g_comstr,"\\\\.\\COM%d",pgen_io->com_io.commsel);
  /* open handle to COM port */
  if( (g_handle = CreateFile(g_comstr, GENERIC_READ|GENERIC_WRITE, 0,NULL,
                             OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)
  {      /* Windows signaled error */
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "k2c_init_io: error opening %s: %s\n", g_comstr,
          errormsg);
    return K2R_ERROR;
  }
  if (!GetCommState(g_handle, &g_dcb))        /* get default DCB values */
  {    /* error getting default DCB values */
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "k2c_init_io: GetCommState error: %s\n", errormsg);
    CloseHandle(g_handle);             /* close COM port handle */
    g_handle = NULL;                   /* mark it as closed */
    return K2R_ERROR;                  /* return indicator code */
  }

  g_dcb.BaudRate = pgen_io->com_io.speed; /* COM port baud rate       */
  g_dcb.Parity = NOPARITY;
  g_dcb.ByteSize = 8;
  g_dcb.StopBits = ONESTOPBIT;
  g_dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  g_dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
  g_dcb.fOutxCtsFlow = FALSE;
  g_dcb.fOutxDsrFlow = TRUE;
  g_dcb.fOutX = FALSE;
  g_dcb.fInX = FALSE;
  g_dcb.fDsrSensitivity = FALSE;              /* disable DSR sensitivity */
  g_dcb.fErrorChar = FALSE;                   /* disable error replacement */
  g_dcb.fNull = FALSE;                        /* disable null stripping */
  g_dcb.fAbortOnError = FALSE;                /* abort reads/writes on error */
  
  if (!SetCommState(g_handle,&g_dcb))        /* setup new DCB values */
  {    /* error setting DCB values */
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "k2c_init_io: SetCommState error: %s\n", errormsg);
    CloseHandle(g_handle);             /* close COM port handle */
    g_handle = NULL;                   /* mark it as closed */
    return K2R_ERROR;                  /* return indicator code */
  }

  if ( set_timing( K2C_DEF_COM_TO ) != K2R_NO_ERROR)
  {
    CloseHandle(g_handle);             /* close COM port handle */
    g_handle = NULL;                   /* mark it as closed */
    return K2R_ERROR;                  /* return indicator code */
  }

  return K2R_NO_ERROR;                /* return OK code */
}


/***************************************************************
 * k2c_close_io:  closes serial port opened by 'k2_init_io()'  *
 *                sets the HANDLE to NULL                      *
 *         return value: returns K2R_NO_ERROR on success,      *
 *                        K2R_ERROR on error                   *
 ***************************************************************/

int k2c_close_io(void)
{
  if (g_handle != NULL)
  {
    if(!CloseHandle(g_handle))  /* close COM port handle */
    {      /* error signaled by Windows */
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "k2c_CLOSE_io: CloseHandle error: %s\n", errormsg);
      g_handle = NULL;                   /* mark it as closed */
      return K2R_ERROR;                  /* return indicator code */
    }
    g_handle = NULL;
  }
  return K2R_NO_ERROR;
}


/**************************************************************************
 * k2c_tran_buff:  transmits buffer of data (up to 65535 bytes) out COM   *
 *      port; with optional timeout value                                 *
 *         buff     - address of buffer containing data bytes             *
 *         datalen   - number of bytes from buffer to transmit            *
 *         toutms   - timeout value in milliseconds; if 0, no timeout     *
 *         redo     - not used                                            *
 *      return value:  returns the number of bytes successfully           *
 *                     transmitted or K2R_ERROR on error,                 *
 *                     K2R_TIMEOUT on timeout                             *
 **************************************************************************/

int k2c_tran_buff(const unsigned char *buff, int datalen, int toutms, int redo)
{
  int pos = 0;
  DWORD n_written;
  
  if (gcfg_debug > 3)
    logit("e", "entering k2c_tran_buff\n");
  if (toutms != 0)            /* if timeout value non-zero then */
    MARK_TIMING(toutms);      /* mark beginning of timing */
  
  while (pos < datalen)
  {
    if ( WriteFile(g_handle, (LPCVOID)&buff[pos], 
                               (uint32_t)(datalen - pos), &n_written, NULL))
    {
      if (toutms != 0 && DONE_TIMING())
      {     /* timeout in use and expired */
        return K2R_TIMEOUT;
      }
      pos += (int)n_written;
    }
    else
    {
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "k2c_tran_buff: error sending to k2: %s\n",
            errormsg);
      return K2R_ERROR;
    }
  }
  return datalen;                /* return # of bytes sent */
}


/**************************************************************************
 * k2c_rcvflg_tout:  waits for data to be received into COM port; with    *
 *      optional timeout value                                            *
 *         toutms   - if non-zero then a timeout value in milliseconds;   *
 *                    if zero then no timeout is used, wait forever       *
 *      return value:  returns K2R_NO_ERROR if data is ready to read      *
 *                returns K2R_TIMEOUT if no data to read within timeout   *
 *                     returns K2R_ERROR on error                         *
 **************************************************************************/

int k2c_rcvflg_tout(int toutms)
{
  static DWORD errflgs;
  static COMSTAT statblk;

  if (gcfg_debug > 3)
    logit("e", "entering k2c_rcvflg_tout\n");
  if (toutms != 0)               /* if timeout value non-zero then */
  {
    MARK_TIMING(toutms);          /* mark beginning of timing */
    if (set_timing( toutms ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }
  else
  {
    if (set_timing( K2C_DEF_COM_TO ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }
    
  statblk.cbInQue = 0L;
  while(1)
  {      /* loop while waiting for received data ready */
    if (!ClearCommError(g_handle, &errflgs, &statblk))
    {
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "k2c_rcvflg_tout: error checking COMM status: %s\n",
            errormsg);
      return K2R_ERROR;
    }
    if (statblk.cbInQue > 0L)
      {
	if (gcfg_debug > 3)
	  logit("e", "k2c_rcvflg_tout ret NO_ERROR\n");
	return K2R_NO_ERROR;
      }
    if (toutms != 0 && DONE_TIMING())
      {
	if (gcfg_debug > 3)
	  logit("e", "k2c_rcvflg_tout ret TIMEOUT\n");
	return K2R_TIMEOUT; 
      }
    sleep_ew(toutms / 5);
  }
  if (gcfg_debug > 3)
    logit("e", "k2c_rcvflg_tout ret TIMEOUT\n");
  return K2R_TIMEOUT;
}


/**************************************************************************
 * k2c_rcvbt_tout:  returns next byte received into COM port; waits for   *
 *      data ready; with optional timeout value                           *
 *        toutms   - if non-zero then a timeout value in milliseconds;    *
 *        redo     - not used                                             *
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

  if (toutms != 0)               /* if timeout value non-zero then */
  {
    MARK_TIMING(toutms);         /* mark beginning of timing */
    if (set_timing( toutms ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }
  else
  {
    if (set_timing( K2C_DEF_COM_TO ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }

  while (1)
  {
    if (rb_count <= rb_read)  /* Is the recv_buffer empty? */
    {            /* Yes, fill it up */
      if ( (rc = GetMoreBytes(RB_LEN)) != K2R_NO_ERROR)
      {
        if (rc == K2R_ERROR)
          return rc;
        if (redo == 0 && toutms != 0 && DONE_TIMING())
          return K2R_TIMEOUT; 
        else  /* no redo's allowed, return the timeout */
          return rc;
      }
    }
    bt = recv_buff[rb_read++];
    return (int) bt;          /* return received data byte */
  }
}


/* **************************************************************************
 * k2c_recv_buff:  fills buffer with received data; waits for given       *
 *      number of bytes to be received; with optional timeout             *
 *         rbuff    - address of buffer to receive data bytes             *
 *         datalen  - number of data bytes to receive                     *
 *         toutms   - timeout value in milliseconds;                      *
 *         redo     - not used                                            *
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

  if (toutms != 0)               /* if timeout value non-zero then */
  {
    MARK_TIMING(toutms);         /* mark beginning of timing */
    if (set_timing( toutms ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }
  else
  {
    if (set_timing( K2C_DEF_COM_TO ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }
  
  while (bcount < datalen)
  {
    if (rb_count <= rb_read)  /* Is the recv_buffer empty? */
    {    /* Yes, fill it up */
      to_read = (datalen - bcount < RB_LEN) ? datalen - bcount : RB_LEN;
      if ( (rc = GetMoreBytes(to_read)) != K2R_NO_ERROR)
      {
        if (rc == K2R_ERROR)
          return rc;
        if (toutms != 0 && DONE_TIMING())
          return K2R_TIMEOUT; 
        else  /* no redo's allowed, return TIMEOUT */
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
  if (toutms != 0)               /* if timeout value non-zero then */
  {
    MARK_TIMING(toutms);          /* mark beginning of timing */
    if (set_timing( toutms ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }
  else
  {
    if (set_timing( K2C_DEF_COM_TO ) == K2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return K2R_ERROR;
    }
  }
  
  if (toutms <= 0)
    bcount = K2C_MAX_FLUSH - 1;
  
  while (bcount < K2C_MAX_FLUSH)
  {
    to_read = (K2C_MAX_FLUSH - bcount < RB_LEN) ? K2C_MAX_FLUSH - bcount : 
      RB_LEN;
    rc = GetMoreBytes(to_read);
    if (gcfg_debug > 3)
      logit( "e", "GetMoreBytes returned %d; %d bytes to go\n",
                  rc, K2C_MAX_FLUSH - bcount );
    if ( rc == K2R_ERROR)
      return rc;
    else if ( toutms == 0 || DONE_TIMING())
      return K2R_NO_ERROR;  /* nothing to flush is OK */

    bcount += rb_count;
  }
  return K2R_NO_ERROR;
}

/************************************************************************
 * GetMoreBytes: an internal function for filling the static recv_buff. *
 *        Note that comm port timeout settings are done in k2c_init_io. *
 *           to_read: number of bytes to recv; rb_count is set to       *
 *               number of bytes actually read; rb_read is set to 0     *
 *           returns: K2R_NO_ERROR on success, K2R_ERROR on error       *
 *                    or K2R_TIMEOUT on timeout                         *
 ************************************************************************/

static int GetMoreBytes( int to_read )
{
  int n_read;
  static int n_retry;

  if (to_read < 1 || to_read > RB_LEN)
  {
    logit("et", "GetMoreBytes: invalid read request <%d>\n", to_read);
    return K2R_ERROR;
  }
  rb_count = rb_read = 0;
  
  if (!ReadFile(g_handle, (LPVOID)recv_buff, (DWORD)to_read, (LPDWORD)&n_read,
		NULL))
  {  /* An error from ReadFile */
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "GetMoreBytes: error receiving: %s\n",
          errormsg);
    return K2R_ERROR;
  }
  if (gcfg_debug > 4)
    logit("e", "GMB %d\n", n_read);
  rb_count = n_read;
  if (rb_count == 0)
    return K2R_TIMEOUT;
  else
    return K2R_NO_ERROR;
}


/****************************************************************
 *  redo_comm: closes and reopens thecomm port using values set *
 *               k2c_init_io().                                 *
 *     return value: returns K2R_NO_ERROR on success;           *
 *               K2R_ERROR on error, or                         *
 *  Not currently used                                          *
 ****************************************************************/
static int redo_comm()
{
  logit("e", "redo_comm\n");
  if (g_handle != NULL)
  {
    if (gcfg_debug > 1)
      logit("e", "closing COM\n");
    if (!CloseHandle(g_handle))  /* close COM port handle */
    {      /* error signaled by Windows */
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "k2c_CLOSE_io: CloseHandle error: %s\n", errormsg);
      g_handle = NULL;                   /* mark it as closed */
      /* See what happens if we continue here */
    }
    g_handle = NULL;
    
    sleep_ew(K2C_RD_COMM_MS);
    
    /* Open a comm port */
    if (gcfg_debug > 1)
      logit("e", "opening COM\n");
    if( (g_handle = CreateFile(g_comstr, GENERIC_READ|GENERIC_WRITE, 0,NULL,
                               OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)
    {      /* Windows signaled error */
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "redo_comm: error opening %s: %s\n", g_comstr,
            errormsg);
      return K2R_ERROR;
    }
    if (gcfg_debug > 1)
      logit("e", "setting comm timeouts\n");
    
    /* make sure set_timing() sees a change */
    g_ctoblk.ReadTotalTimeoutConstant = (uint32_t)0;
    if ( set_timing( K2C_DEF_COM_TO ) == K2R_ERROR )
    {      /* error setting timeout values */
      CloseHandle(g_handle);             /* close COM port handle */
      g_handle = NULL;                   /* mark it as closed */
      return K2R_ERROR;                  /* return indicator code */
    }
  }
  if (gcfg_debug > 0)
    logit("e", "done with redo_comm\n");
  return K2R_NO_ERROR;
}

/**********************************************************
 * set_timing: Set the timing parameters of the COM port, *
 *           since we can't use select() for timeouts     *
 *      time: number of milliseconds to set the           *
 *                ReadTotalTimeoutConstant value in the   *
 *                COMMTIMEOUTS structure                  *
 *       returns: K2R_NO_ERROR on success;                *
 *                K2R_ERROR on failure                    *
 **********************************************************/

int set_timing( int time )
{

  /* Set the timing values only if they need to be changed */
  if (g_ctoblk.ReadTotalTimeoutConstant != (uint32_t) time)
  {
    if (gcfg_debug > 2)
      logit("e", "Change timing to %d\n", time);
    /* setup Windows COM port timeout values */
    g_ctoblk.ReadIntervalTimeout = 5;                /* setup receive to */
    g_ctoblk.ReadTotalTimeoutMultiplier = (uint32_t)5;  /* return when any chars */
    g_ctoblk.ReadTotalTimeoutConstant = (uint32_t)time; /* read or timeout */
    g_ctoblk.WriteTotalTimeoutMultiplier = (uint32_t)0;
    g_ctoblk.WriteTotalTimeoutConstant = (uint32_t)1;   /* 1ms transmit timeout */
    if (!SetCommTimeouts(g_handle,(LPCOMMTIMEOUTS)&g_ctoblk))
    {      /* error setting timeout values */
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "k2c_init_io: SetCommTimeouts error: %s\n", errormsg);
      return K2R_ERROR;
    }
  }
  return K2R_NO_ERROR;
}
