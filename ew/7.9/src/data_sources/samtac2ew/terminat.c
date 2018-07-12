/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: terminat.c 3536 2009-01-15 22:09:51Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.5  2009/01/15 17:33:22  tim
 *     Dies gracefully when startstop tells it to, and rename main.tmp.c to samtac2ew_main.c
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
 *     Revision 1.1  2008/11/10 16:32:14  tim
 *     *** empty log message ***
 *
 *     Revision 1.5  2008/10/02 21:22:08  kress
 *     V.C.Kress: made k2ew compile properly under 32 bit linux
 *
 *     Revision 1.4  2007/05/10 00:20:14  dietz
 *     included <string.h> to fix missing prototypes
 *
 *     Revision 1.3  2000/08/30 17:34:00  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.2  2000/05/09 23:59:23  lombard
 *     Added restart mechanism
 *
 *     Revision 1.1  2000/05/04 23:48:41  lombard
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>          /* For varargs support */
#include <earthworm.h>       /* Earthworm main include file */
#include "samtac2ew_misc.h"
#include "samtac_comif.h"         /* SAMTAC COM port interface routines */
#include "glbvars.h"         /* externs for global vars from 'k2ewmain.c' */
#include "terminat.h"        /* header file for this module */


static int samtacterm_code=SAMTACTERM_UNKNOWN;      /* termination code for 'statmgr' */
static char samtacterm_message[4096]={'\0'};    /* buffer for 'statmgr' term msg */


/**************************************************************************
 * samtac2ew_enter_exitmsg:  enters the termination code and writes the given  *
 *      message (specified with 'printf()'-style parameters) to the       *
 *      logfile (appends newline and uses 'logit("e",...)') and enters    *
 *      them into the message to be sent to the Earthworm status manager  *
 *      upon 'samtac2ew' program exit                                          *
 *      Note:  no more than 4095 characters of output allowed in message  *
 *           termcode - exit termination code (one of the 'SAMTAC2TERM_...'    *
 *                      codes from 'terminat.h' which correspond to the   *
 *                      'statmgr' message codes in 'samtac2ew.desc')           *
 **************************************************************************/

void samtac2ew_enter_exitmsg(int termcode,const char *fmtstr,...)
{
  long cnt=0;
  va_list argptr;
  static volatile int infn_flg = 0;

  while (infn_flg != FALSE)
  {      /* wait if another thread is in function */
    if (++cnt > 9999999L)          /* if waiting too long then */
      return;                     /* abort function */
  }
  infn_flg = 1;                /* indicate now in function */

  samtacterm_code = termcode;         /* enter termination code */
  va_start(argptr,fmtstr);                  /* setup argument list */
  vsprintf(samtacterm_message, fmtstr, argptr);   /* format into output string */
  va_end(argptr);                           /* cleanup argument list */
  logit("et","%s\n",samtacterm_message);         /* send to logfile */

  infn_flg = 0;               /* indicate out of function */
}


/**************************************************************************
 * samtac2ew_exit:  program exit; sends termination message to the Earthworm   *
 *      status manager (using code and message specified in               *
 *          core - flag: if 1, do a core dump (on Unix only)              *
 *      'k2ew_enter_exitmsg()', cleans things up, and exits               *
 **************************************************************************/

void samtac2ew_exit(int core)
{
  char *sptr;
  static volatile int infn_flg=FALSE;
  static MSG_LOGO logo;
  static char buff[1024];

 // if(infn_flg == 0)      /* don't let any other thread in after us */
 // {      /* no other thread using function */
    //infn_flg = 1;              /* indicate now in function */
    //if (g_output_threadid != (unsigned)-1)    /* if read thread running then */
    //  KillThread(g_output_threadid);         /* terminate read thread */

    if (samtacterm_code < 0 || samtacterm_code > SAMTACTERM_NUMENTS)
      samtacterm_code = SAMTACTERM_UNKNOWN;    /* if term code out of range then fix */

    while ( (sptr = (char *)strchr(samtacterm_message,(int)'\n')) != NULL)
      *sptr = ' ';      /* take out any newline chars in msg (just in case) */
    /* build 'statmgr' message using current system time and */
    /*  code and message set by the 'k2ew_enter_exitmsg()' fn */
    sprintf(buff,"SAMTAC: %s\n", samtacterm_message);
    samtac2mi_status_hb(TypeErr, (short)samtacterm_code, buff);

    /* close down circular buffer and serial port */
    /*  (fn calls will be ignored OK if not allocated or not open) */
    //k2cb_dealloc_buffer();        /* deallocate circular buffer */
    (void)samtac_close_io();           /* close IO port */
    tport_detach(&g_tport_region);
    logit("te","Exit cleanup completed, %s terminated\n",g_progname_str);

    if (core)
      abort();                  /* Abort for a core dump (Unix only) */
    else
      exit(samtacterm_code);        /* exit program, report code to OS */
  //  infn_flg = 0;               /* indicate out of function (just in case) */
//  }
}

void samtac2ew_throw_error()
{
  char *sptr;
  static volatile int infn_flg=FALSE;
  static MSG_LOGO logo;
  static char buff[1024];

  //if(infn_flg == 0)      /* don't let any other thread in after us */
 // {      /* no other thread using function */
 //   infn_flg = 1;              /* indicate now in function */
    //if (g_output_threadid != (unsigned)-1)    /* if read thread running then */
    //  KillThread(g_output_threadid);         /* terminate read thread */

    if (samtacterm_code < 0 || samtacterm_code > SAMTACTERM_NUMENTS)
      samtacterm_code = SAMTACTERM_UNKNOWN;    /* if term code out of range then fix */

    while ( (sptr = (char *)strchr(samtacterm_message,(int)'\n')) != NULL)
      *sptr = ' ';      /* take out any newline chars in msg (just in case) */
    /* build 'statmgr' message using current system time and */
    /*  code and message set by the 'k2ew_enter_exitmsg()' fn */
    sprintf(buff,"SAMTAC: %s\n", samtacterm_message);
    samtac2mi_status_hb(TypeErr, (short)samtacterm_code, buff);

    /* close down circular buffer and serial port */
    /*  (fn calls will be ignored OK if not allocated or not open) */
    //k2cb_dealloc_buffer();        /* deallocate circular buffer */
    //(void)samtac_close_io();           /* close IO port */
    //tport_detach(&g_tport_region);
    //logit("te","Exit cleanup completed, %s terminated\n",g_progname_str);

    //if (core)
     // abort();                  /* Abort for a core dump (Unix only) */
    //else
     // exit(samtacterm_code);        /* exit program, report code to OS */
    //infn_flg = 0;               /* indicate out of function (just in case) */
  //}
}


