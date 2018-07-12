/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: terminat.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
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
/*  terminat.c:  Program termination functions                               */
/*                                                                           */
/*    3/12/99 -- [ET]                                                        */
/*    3/15/00 -- Pete Lombard: modified to remove references to specific IO  */
/*                  modes.                                                   */
/*                                                                           */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>          /* For varargs support */
#include <earthworm.h>       /* Earthworm main include file */
#include "k2misc.h"
#include "k2comif.h"         /* K2 COM port interface routines */
#include "k2cirbuf.h"        /* K2 circular buffer routines */
#include "glbvars.h"         /* externs for global vars from 'k2ewmain.c' */
#include "terminat.h"        /* header file for this module */


static int k2term_code=K2TERM_UNKNOWN;      /* termination code for 'statmgr' */
static char k2term_message[4096]={'\0'};    /* buffer for 'statmgr' term msg */


/**************************************************************************
 * k2ew_enter_exitmsg:  enters the termination code and writes the given  *
 *      message (specified with 'printf()'-style parameters) to the       *
 *      logfile (appends newline and uses 'logit("e",...)') and enters    *
 *      them into the message to be sent to the Earthworm status manager  *
 *      upon 'k2ew' program exit                                          *
 *      Note:  no more than 4095 characters of output allowed in message  *
 *           termcode - exit termination code (one of the 'K2TERM_...'    *
 *                      codes from 'terminat.h' which correspond to the   *
 *                      'statmgr' message codes in 'k2ew.desc')           *
 **************************************************************************/

void k2ew_enter_exitmsg(int termcode,const char *fmtstr,...)
{
  int32_t cnt=0;
  va_list argptr;
  static volatile int infn_flg = 0;

  while (infn_flg != FALSE)
  {      /* wait if another thread is in function */
    if (++cnt > 9999999L)          /* if waiting too long then */
      return;                     /* abort function */
  }
  infn_flg = 1;                /* indicate now in function */

  k2term_code = termcode;         /* enter termination code */
  va_start(argptr,fmtstr);                  /* setup argument list */
  vsprintf(k2term_message, fmtstr, argptr);   /* format into output string */
  va_end(argptr);                           /* cleanup argument list */
  logit("et","%s\n",k2term_message);         /* send to logfile */

  infn_flg = 0;               /* indicate out of function */
}


/**************************************************************************
 * k2ew_exit:  program exit; sends termination message to the Earthworm   *
 *      status manager (using code and message specified in               *
 *          core - flag: if 1, do a core dump (on Unix only)              *
 *      'k2ew_enter_exitmsg()', cleans things up, and exits               *
 **************************************************************************/

void k2ew_exit(int core)
{
  char *sptr;
  static volatile int infn_flg=FALSE;
  static char buff[1024];

  if(infn_flg == 0)      /* don't let any other thread in after us */
  {      /* no other thread using function */
    infn_flg = 1;              /* indicate now in function */
    if (g_output_threadid != (unsigned)-1)    /* if read thread running then */
      KillThread(g_output_threadid);         /* terminate read thread */

    if (k2term_code < 0 || k2term_code > K2TERM_NUMENTS)
      k2term_code = K2TERM_UNKNOWN;    /* if term code out of range then fix */

    while ( (sptr = (char *)strchr(k2term_message,(int)'\n')) != NULL)
      *sptr = ' ';      /* take out any newline chars in msg (just in case) */
    /* build 'statmgr' message using current system time and */
    /*  code and message set by the 'k2ew_enter_exitmsg()' fn */
    sprintf(buff,"K2 <%s>: %s\n",g_stnid, k2term_message);
    k2mi_status_hb(g_error_ltype, (short)k2term_code, buff);

    /* close down circular buffer and serial port */
    /*  (fn calls will be ignored OK if not allocated or not open) */
    k2cb_dealloc_buffer();        /* deallocate circular buffer */
    (void)k2c_close_io();           /* close IO port */
    tport_detach(&g_tport_region);
    logit("te","Exit cleanup completed, %s terminated\n",g_progname_str);

    if (core)
      abort();                  /* Abort for a core dump (Unix only) */
    else
      exit(k2term_code);        /* exit program, report code to OS */
    infn_flg = 0;               /* indicate out of function (just in case) */
  }
}

