/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: outptthrd.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.14  2005/03/26 00:17:46  kohler
 *     Version 2.38.  Added capability to get network code values from the K2
 *     headers.  The "Network" parameter in the config file is now optional.
 *     WMK 3/25/05
 *
 *     Revision 1.13  2004/06/03 16:54:01  lombard
 *     Updated for location code (SCNL) and TYPE_TRACEBUF2 messages.
 *
 *     Revision 1.12  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.10  2001/05/08 00:14:11  kohler
 *     Minor logging changes.
 *
 *     Revision 1.9  2000/09/01 18:16:25  lombard
 *     Change check for backward time to milliseconds instead of seconds
 *
 *     Revision 1.8  2000/08/30 17:34:00  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.7  2000/08/12 18:10:04  lombard
 *     Fixed bug in cb_check_waits that caused circular buffer overflows
 *     Added cb_dumb_buf() to dump buffer indexes to file for debugging
 *
 *     Revision 1.6  2000/07/28 22:36:10  lombard
 *     Moved heartbeats to separate thread; added DontQuick command; removed
 *     redo_com() since it doesn't do any good; other minor bug fixes
 *
 *     Revision 1.5  2000/07/03 20:09:09  lombard
 *     Corrected endtime value in trace_buf packets.
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
 *     Revision 1.2  2000/05/09 23:59:20  lombard
 *     Added restart mechanism
 *
 *     Revision 1.1  2000/05/04 23:48:38  lombard
 *     Initial revision
 *
 *
 *
 */
/*  outptthrd.c:  "Output thread function for K2-to-Earthworm */
/*  */
/*   3/18/99 -- [ET] */
/*  */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>       /* Earthworm main include file */
#include <time.h>
#include <transport.h>       /* Earthworm shared-memory transport routines */
#include <trace_buf.h>       /* Earthworm trace buffer/packet definitions */
#include "k2pktman.h"        /* K2 packet management and SDS functions */
#include "k2ewerrs.h"        /* K2-to-Earthworm error codes */
#include "k2cirbuf.h"        /* K2 circular buffer routines */
#include "k2ewrstrt.h"       /* k2ew restart routines */
#include "glbvars.h"         /* externs for global vars from 'k2ewmain.c' */
#include "terminat.h"        /* program termination defines and functions */
#include "outptthrd.h"       /* header file for this module  */


/**************************************************************************
 * k2ew_outputthread_fn:  Output thread function started up by the main   *
 *      thread; reads data from the circular data buffer                  *
 *      and sends it out to Earthworm                                     *
 *         arg - parameter not used but needs to be declared              *
 **************************************************************************/

thr_ret k2ew_outputthread_fn(void *arg)
{
  static int cd;
  static int rc;
  static unsigned short msec;
  static uint32_t dataseq,timestamp;
  static unsigned char stmnum;
  static int32_t databuff[K2PM_MIN_DBUFSIZ+2];
  static MSG_LOGO logo;
  static TracePacket ew_trace_pkt;
  static int badTimeLogged = 0;
  static double last_endtime[K2_MAX_STREAMS];
  double starttime;

  static int waitingFlag = 0;     /* 1 while waiting for buffer entries */


  /* setup "logo" for data trace messages to Earthworm */
  logo.type = g_trace_ltype;           /* enter logo msg type as "trace" */
  logo.mod = gcfg_module_idnum;        /* enter module ID for 'k2ew' */
  logo.instid = g_instid_val;          /* enter installation ID value */

  /* setup trace packet header items that never change */
  memset(&ew_trace_pkt,0,sizeof(ew_trace_pkt));  /* clear trace packet block */
  strncpy(ew_trace_pkt.trh2.sta,g_stnid,     /* copy over station-ID str */
	  TRACE2_STA_LEN-1);
  ew_trace_pkt.trh2.version[0] = TRACE2_VERSION0;
  ew_trace_pkt.trh2.version[1] = TRACE2_VERSION1;
  
#ifdef _INTEL
  strcpy(ew_trace_pkt.trh2.datatype,"i4");   /* enter data type (Intel ints) */
#endif
#ifdef _SPARC
  strcpy(ew_trace_pkt.trh2.datatype,"s4");   /* enter data type (Sun ints) */
#endif
  ew_trace_pkt.trh2.samprate = (double)g_smprate; /* enter K2 SDS sample rate */
  /* enter # of samples per packet--assumption is that each */
  /*  packet contains 1 second's worth of data (100 samples) */
  ew_trace_pkt.trh2.nsamp = g_smprate;

  if (sizeof(TRACE2_HEADER) + ew_trace_pkt.trh2.nsamp * sizeof(int32_t) >
      MAX_TRACEBUF_SIZ)
  {      /* expected number of samples won't fit into trace buffer */
    logit("et",               /* log error message */
          "Too many samples for trace buffer (%d), aborting read thread\n",
          ew_trace_pkt.trh2.nsamp);
    KillSelfThread();                  /* exit thread function */
  }

  /**************  Main Loop Reading from Circular Buffer *******************/
  do          /* loop while transferring data from circular buffer to EW */
  {                /* attempt to read K2 data block out from circular buffer */
    g_ot_working = 1;  /* Tell the heartbeat threead we're alive */

    if((rc=k2cb_block_out(&stmnum,&dataseq,&timestamp,&msec,databuff)) ==
                                                               K2R_NO_ERROR)
    {    /* block read from circular buffer OK */
      if(gcfg_debug > 1 && waitingFlag != 0)
      {     /* debug enabled and buffer no longer is waiting for entries */
        logit("et","Circular buffer output to EW resumed\n");
        waitingFlag = 0;        /* indicate no longer waiting */
      }
      if(stmnum != g_trk_stmnum || dataseq != g_trk_dataseq)
      {       /* stream or data sequence number mismatches */
        ++g_seq_errcnt;                /* increment error count */
        time(&g_seq_errtime);          /* set time of last sequence error */
                                       /* log error message */
        logit("et","Output Thread: Error detected in sequence read"
                   " from circular buffer");
        if (gcfg_debug > 0)
        {     /* debug output enabled; log more detail */
          logit("e","; expected %lu.%d, got %lu.%d",g_trk_dataseq,
                                               g_trk_stmnum,dataseq,stmnum);
        }
        logit("e","\n");
        g_trk_stmnum = stmnum;           /* sync stream number to received */
        g_trk_dataseq = dataseq;         /* sync data seq number to received */
      }

      /* Get the complete time into starttime, a double */
      starttime = (double)(timestamp + K2_TIME_CONV) + (double)msec * 0.001;

      /* Check packet time; reject packets older than 1981, which would mean
       * the K2 clock has reset to 0 (1 Jan 1980 in KMI land; 31536000 is
       * the number of seconds in 1 year, roughly. */
      if (timestamp < 31536000)
      {  /* Old packet found; reject it */
        if (badTimeLogged == 0)
        {
          logit("et", "Bad time in K2 packet: %lu; stopped sending EW packets\n",
                timestamp + K2_TIME_CONV);
          badTimeLogged = 1;
        }
      }
      /* Check to make sure time is advancing */
      else if (starttime <= last_endtime[stmnum])
      {
        logit("et", "K2 packet time not advancing for stream %d; this start: %lf"
              " last end: %lf\n", stmnum, starttime, last_endtime[stmnum]);
      }
      else
      {  /* Time ok */
        if (badTimeLogged == 1)
        {
          logit("et", "K2 time within limits; resumed sending packets\n");
          badTimeLogged = 0;
        }
        /* setup trace buffer header for Earthworm message */
        /* put in channel, network and location ID descriptions for packet */
        if (stmnum < (unsigned char)g_numstms)
        {       /* stream number is in range; copy over channel ID string */
          strncpy(ew_trace_pkt.trh2.chan,g_stmids_arr[stmnum],
                  TRACE2_CHAN_LEN-1);
          /* make sure string is NULL terminated          */
          ew_trace_pkt.trh2.chan[TRACE2_CHAN_LEN-1] = '\0';

          strncpy(ew_trace_pkt.trh2.net,g_netcode_arr[stmnum],
                  TRACE2_NET_LEN-1);
          /* make sure string is NULL terminated          */
          ew_trace_pkt.trh2.loc[TRACE2_LOC_LEN-1] = '\0';

          strncpy(ew_trace_pkt.trh2.loc,gcfg_locnames_arr[stmnum],
                  TRACE2_LOC_LEN-1);
          /* make sure string is NULL terminated          */
          ew_trace_pkt.trh2.loc[TRACE2_LOC_LEN-1] = '\0';
        }
        else  /* stream number out of range; build "error" channel ID string */
	{
          sprintf(ew_trace_pkt.trh2.chan,"?%hu?",(unsigned short)stmnum);
	  strcpy(ew_trace_pkt.trh2.loc, LOC_NULL_STRING);
	}

        /* calculate and enter start-timestamp for packet */
        ew_trace_pkt.trh2.starttime = starttime;

        /* endtime is the time of last sample in this packet, not the time *
         * of the first sample in the next packet */
        ew_trace_pkt.trh2.endtime = ew_trace_pkt.trh2.starttime +
          (double)(ew_trace_pkt.trh2.nsamp - 1) / ew_trace_pkt.trh2.samprate;

        /* Save the end time for comparison with the next packet for this
         stream */
        last_endtime[stmnum] = ew_trace_pkt.trh2.endtime;

        /* Earthworm pin number is base plus stream number */
        ew_trace_pkt.trh2.pinno = gcfg_base_pinno + stmnum;

        /* copy payload of 32-bit ints into trace buffer (after header) */
        memcpy(&ew_trace_pkt.msg[sizeof(TRACE2_HEADER)], databuff,
               ew_trace_pkt.trh2.nsamp*sizeof(int32_t));

        /* send data trace message to Earthworm */
        if ( (cd = tport_putmsg(&g_tport_region, &logo,
                                (int32_t)sizeof(TRACE2_HEADER) +
                                (int32_t)ew_trace_pkt.trh2.nsamp * sizeof(int32_t),
                                (char *)&ew_trace_pkt)) != PUT_OK)
        {            /* 'put' function returned error code */
          k2ew_enter_exitmsg(K2TERM_EW_PUTMSG,   /* log & enter exit message */
                             "Earthworm 'tport_putmsg()' function failed (%d)",
                             cd);
          g_terminate_flg = 1;        /* set terminate flag for main thread */
          break;                         /* exit loop (and thread) */
        }
        if (gcfg_debug > 2)
          logit("et", "sent <%s.%s.%s> at %lf seq %lu stm %d\n",
                ew_trace_pkt.trh2.sta, ew_trace_pkt.trh2.chan,
                ew_trace_pkt.trh2.net, ew_trace_pkt.trh2.starttime,
                dataseq, stmnum);

      }

      ++g_pktout_okcount;              /* inc packets-delivered-OK count */

      /* increment stream number tracking variable */
      if (++g_trk_stmnum >= (unsigned char)g_numstms)
      {       /* increment puts it past last stream number */
        g_trk_stmnum = (unsigned char)0; /* wrap-around to first stream number */
        ++g_trk_dataseq;                 /* increment data sequence number */
      }
      sleep_ew(10);/* Take a short nap so we don't flood the transport ring */
    }
    else
    {         /* error code returned */
      if (rc == K2R_CB_BUFEMPTY || rc == K2R_CB_WAITENT)
      {       /* buffer emtpy or entry-waiting */
        if(gcfg_debug > 1 && rc == K2R_CB_WAITENT && waitingFlag == 0)
        {     /* debug enabled and buffer is waiting for entries */
          logit("et","Circular buffer is waiting for entries\n");
          waitingFlag = 1;        /* indicate waiting */
        }
        sleep_ew(100);   /* take a short break to let main thread run */
      }
      else
      {       /* not buffer-empty and not entry-waiting */
        if (rc != K2R_CB_SKIPENT)
        {     /* not packet-skip code; log error message */
          logit("et","Error reading from circular buffer\n");
        }
      }
    }
  }
  while(g_terminate_flg == 0);     /* loop until terminate flag is set */

  /* Write the restart file unless station-name test failed in
  * k2mi_report_params() */
  if (gcfg_restart_filename[0] != '\0' && g_validrestart_flag != 1)
    k2ew_write_rsfile(g_trk_dataseq, g_trk_stmnum);

  sleep_ew(10000);  /* give 'k2ew_exit()' a chance to terminate us gracefully */
  return THR_NULL_RET;
}

