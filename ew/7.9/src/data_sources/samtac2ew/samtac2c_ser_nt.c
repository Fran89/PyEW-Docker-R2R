/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac2c_ser_nt.c 3536 2009-01-15 22:09:51Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.4  2009/01/13 17:12:53  tim
 *     Clean up source
 *
 *     Revision 1.3  2009/01/13 15:41:27  tim
 *     Removed more k2 references
 *
 *     Revision 1.2  2009/01/12 20:52:32  tim
 *     Removing K2 references
 *
 *     Revision 1.1  2008/10/21 22:52:51  tim
 *     *** empty log message ***
 *
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
 */

#include <stdio.h>
#include <windows.h>
#include <earthworm.h>
#include <error_ew.h>
#include <time.h>            /* for 'clock()' function       */
#include "glbvars.h"
#include "samtac_comif.h"         /* general header file for IO   */
#include "samtac2ew_errs.h"

#define SAMTAC_DEF_COM_TO  500      /* default COM port timeout in msec */
#define SAMTAC_RD_COMM_MS  5000     /* msec to wait during socket redo */
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
 * samtac_init_io:  serial port initialization function      *
 *         gen_io: generalize IO parameter structure      *
 *      toutms: not used                                  *
 *      return value:  returns SAMTAC2R_NO_ERROR if successful *
 *                     returns SAMTAC2R_ERROR on error         *
 **********************************************************/

int samtac_init_io(s_gen_io *gen_io, int toutms)
{
  if (gen_io->mode != IO_COM_NT)
  {
    logit("et", "samtac_init_io: IO mode is not COMM as expected\n");
    return SAMTAC2R_ERROR;
  }

  /* build "\\.\COMx" string */
  sprintf(g_comstr,"\\\\.\\COM%d",gen_io->com_io.commsel);
  /* open handle to COM port */
  if( (g_handle = CreateFile(g_comstr, GENERIC_READ|GENERIC_WRITE, 0,NULL,
                             OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)
  {      /* Windows signaled error */
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "samtac_init_io: error opening %s: %s\n", g_comstr,
          errormsg);
    return SAMTAC2R_ERROR;
  }
  if (!GetCommState(g_handle, &g_dcb))        /* get default DCB values */
  {    /* error getting default DCB values */
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "samtac_init_io: GetCommState error: %s\n", errormsg);
    CloseHandle(g_handle);             /* close COM port handle */
    g_handle = NULL;                   /* mark it as closed */
    return SAMTAC2R_ERROR;                  /* return indicator code */
  }

  g_dcb.BaudRate = gen_io->com_io.speed; /* COM port baud rate       */
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
    logit("et", "samtac_init_io: SetCommState error: %s\n", errormsg);
    CloseHandle(g_handle);             /* close COM port handle */
    g_handle = NULL;                   /* mark it as closed */
    return SAMTAC2R_ERROR;                  /* return indicator code */
  }

  if ( set_timing( SAMTAC_DEF_COM_TO ) != SAMTAC2R_NO_ERROR)
  {
    CloseHandle(g_handle);             /* close COM port handle */
    g_handle = NULL;                   /* mark it as closed */
    return SAMTAC2R_ERROR;                  /* return indicator code */
  }

  return SAMTAC2R_NO_ERROR;                /* return OK code */
}


/***************************************************************
 * samtac_close_io:  closes serial port opened by 'samtac_init_io()'  *
 *                sets the HANDLE to NULL                      *
 *         return value: returns SAMTAC2R_NO_ERROR on success,      *
 *                        SAMTAC2R_ERROR on error                   *
 ***************************************************************/

int samtac_close_io(void)
{
  if (g_handle != NULL)
  {
    if(!CloseHandle(g_handle))  /* close COM port handle */
    {      /* error signaled by Windows */
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "samtac_CLOSE_io: CloseHandle error: %s\n", errormsg);
      g_handle = NULL;                   /* mark it as closed */
      return SAMTAC2R_ERROR;                  /* return indicator code */
    }
    g_handle = NULL;
  }
  return SAMTAC2R_NO_ERROR;
}

/* **************************************************************************
 * samtac_recv_buff:  fills buffer with received data; waits for given       *
 *      number of bytes to be received; with optional timeout             *
 *         rbuff    - address of buffer to receive data bytes             *
 *         datalen  - number of data bytes to receive                     *
 *         toutms   - timeout value in milliseconds;                      *
 *         redo     - not used                                            *
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
    logit("e", "entering samtac_recv_buff\n");

  if (toutms != 0)               /* if timeout value non-zero then */
  {
    MARK_TIMING(toutms);         /* mark beginning of timing */
    if (set_timing( toutms ) == SAMTAC2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return SAMTAC2R_ERROR;
    }
  }
  else
  {
    if (set_timing( SAMTAC_DEF_COM_TO ) == SAMTAC2R_ERROR)
    {
      CloseHandle( g_handle );
      g_handle = NULL;
      return SAMTAC2R_ERROR;
    }
  }
  
  while (bcount < datalen)
  {
    if (rb_count <= rb_read)  /* Is the recv_buffer empty? */
    {    /* Yes, fill it up */
      to_read = (datalen - bcount < RB_LEN) ? datalen - bcount : RB_LEN;
      if ( (rc = GetMoreBytes(to_read)) != SAMTAC2R_NO_ERROR)
      {
        if (rc == SAMTAC2R_ERROR)
          return rc;
        if (toutms != 0 && DONE_TIMING())
          return SAMTAC2R_TIMEOUT; 
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

/************************************************************************
 * GetMoreBytes: an internal function for filling the static recv_buff. *
 *        Note that comm port timeout settings are done in samtac_init_io. *
 *           to_read: number of bytes to recv; rb_count is set to       *
 *               number of bytes actually read; rb_read is set to 0     *
 *           returns: SAMTAC2R_NO_ERROR on success, SAMTAC2R_ERROR on error       *
 *                    or SAMTAC2R_TIMEOUT on timeout                         *
 ************************************************************************/

static int GetMoreBytes( int to_read )
{
  int n_read;
  static int n_retry;

  if (to_read < 1 || to_read > RB_LEN)
  {
    logit("et", "GetMoreBytes: invalid read request <%d>\n", to_read);
    return SAMTAC2R_ERROR;
  }
  rb_count = rb_read = 0;
  
  if (!ReadFile(g_handle, (LPVOID)recv_buff, (DWORD)to_read, (LPDWORD)&n_read,
		NULL))
  {  /* An error from ReadFile */
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "GetMoreBytes: error receiving: %s\n",
          errormsg);
    return SAMTAC2R_ERROR;
  }
  if (gcfg_debug > 4)
    logit("e", "GMB %d\n", n_read);
  rb_count = n_read;
  if (rb_count == 0)
    return SAMTAC2R_TIMEOUT;
  else
    return SAMTAC2R_NO_ERROR;
}


/****************************************************************
 *  redo_comm: closes and reopens thecomm port using values set *
 *               samtac_init_io().                                 *
 *     return value: returns SAMTAC2R_NO_ERROR on success;           *
 *               SAMTAC2R_ERROR on error, or                         *
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
      logit("et", "samtac_CLOSE_io: CloseHandle error: %s\n", errormsg);
      g_handle = NULL;                   /* mark it as closed */
      /* See what happens if we continue here */
    }
    g_handle = NULL;
    
    sleep_ew(SAMTAC_RD_COMM_MS);
    
    /* Open a comm port */
    if (gcfg_debug > 1)
      logit("e", "opening COM\n");
    if( (g_handle = CreateFile(g_comstr, GENERIC_READ|GENERIC_WRITE, 0,NULL,
                               OPEN_EXISTING,0,NULL)) == INVALID_HANDLE_VALUE)
    {      /* Windows signaled error */
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "redo_comm: error opening %s: %s\n", g_comstr,
            errormsg);
      return SAMTAC2R_ERROR;
    }
    if (gcfg_debug > 1)
      logit("e", "setting comm timeouts\n");
    
    /* make sure set_timing() sees a change */
    g_ctoblk.ReadTotalTimeoutConstant = (unsigned long)0; 
    if ( set_timing( SAMTAC_DEF_COM_TO ) == SAMTAC2R_ERROR )
    {      /* error setting timeout values */
      CloseHandle(g_handle);             /* close COM port handle */
      g_handle = NULL;                   /* mark it as closed */
      return SAMTAC2R_ERROR;                  /* return indicator code */
    }
  }
  if (gcfg_debug > 0)
    logit("e", "done with redo_comm\n");
  return SAMTAC2R_NO_ERROR;
}

/**********************************************************
 * set_timing: Set the timing parameters of the COM port, *
 *           since we can't use select() for timeouts     *
 *      time: number of milliseconds to set the           *
 *                ReadTotalTimeoutConstant value in the   *
 *                COMMTIMEOUTS structure                  *
 *       returns: SAMTAC2R_NO_ERROR on success;                *
 *                SAMTAC2R_ERROR on failure                    *
 **********************************************************/

int set_timing( int time )
{

  /* Set the timing values only if they need to be changed */
  if (g_ctoblk.ReadTotalTimeoutConstant != (unsigned long) time)
  {
    if (gcfg_debug > 2)
      logit("e", "Change timing to %d\n", time);
    /* setup Windows COM port timeout values */
    g_ctoblk.ReadIntervalTimeout = 5;                /* setup receive to */
    g_ctoblk.ReadTotalTimeoutMultiplier = (unsigned long)5; /* return when any chars */
    g_ctoblk.ReadTotalTimeoutConstant = (unsigned long)time;   /* read or timeout */
    g_ctoblk.WriteTotalTimeoutMultiplier = (unsigned long)0;
    g_ctoblk.WriteTotalTimeoutConstant = (unsigned long)1;   /* 1ms transmit timeout */
    if (!SetCommTimeouts(g_handle,(LPCOMMTIMEOUTS)&g_ctoblk))
    {      /* error setting timeout values */
      ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
      logit("et", "samtac_init_io: SetCommTimeouts error: %s\n", errormsg);
      return SAMTAC2R_ERROR;
    }
  }
  return SAMTAC2R_NO_ERROR;
}
