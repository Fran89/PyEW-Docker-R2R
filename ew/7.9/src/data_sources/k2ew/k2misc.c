/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2misc.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.32  2008/05/16 20:50:57  dietz
 *     removed FLUSH_IT definition/ifdefs which were left over from testing
 *
 *     Revision 1.31  2007/12/18 13:36:03  paulf
 *     version 2.43 which improves modem handling for new kmi firmware on k2 instruments
 *
 *     Revision 1.30  2007/02/26 17:16:53  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.29  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.28  2005/03/26 00:17:46  kohler
 *     Version 2.38.  Added capability to get network code values from the K2
 *     headers.  The "Network" parameter in the config file is now optional.
 *     WMK 3/25/05
 *
 *     Revision 1.27  2004/06/04 16:36:04  lombard
 *     Fixed parts of extended status message.
 *
 *     Revision 1.26  2003/07/17 19:23:27  friberg
 *     Fixed a status message to include network and station name for the k2
 *     from which the error originated.
 *
 *     Revision 1.25  2003/06/06 01:24:33  lombard
 *     Changed to version 2.34: fix for byte alignment problem in extended status
 *     structure.
 *
 *     Revision 1.24  2003/05/29 13:33:40  friberg
 *     Added in calls to k2info and fixed GPS status message to include
 *     the station name. Changed all status messages to include
 *     network code in addition to station name
 *
 *     Revision 1.23  2003/05/15 00:41:47  lombard
 *     *** empty log message ***
 *
 *     Revision 1.22  2002/05/06 18:24:58  kohler
 *     Fixed typo in log statement in function k2mi_init_blkmde().
 *
 *     Revision 1.21  2002/04/22 16:35:55  lucky
 *     k2ew was being restarted by startstop because it would stop beating its
 *     heart when re-trying to connect. Made it so that we continue to beat our
 *     heart even when we go through the retry loop several times.
 *
 *     Revision 1.20  2002/01/30 14:28:31  friberg
 *     added robust comm recovery for time out case
 *
 *     Revision 1.19  2001/10/19 18:21:28  kohler
 *     k2mi_log_pktinfo() now sends msg to statmgr if a pcdrv error occurs.
 *
 *     Revision 1.18  2001/10/16 22:03:56  friberg
 *     Upgraded to version 2.25
 *
 *     Revision 1.17  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.15  2001/05/23 00:20:00  kohler
 *     Now, optionally writes entire K2 header to a binary file.
 *
 *     Revision 1.14  2001/05/08 00:14:38  kohler
 *     Minor logging changes.
 *
 *     Revision 1.13  2001/04/23 20:24:10  friberg
 *     Added station name remapping using the StationId config parameter.
 *
 *     Revision 1.12  2000/11/28 00:45:46  kohler
 *     Cosmetic changes to log file output.
 *
 *     Revision 1.11  2000/11/07 19:35:01  kohler
 *     Modified to complain if no GPS lock for xx hours.
 *
 *     Revision 1.10  2000/08/30 17:34:00  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.9  2000/07/28 22:36:10  lombard
 *     Moved heartbeats to separate thread; added DontQuick command; removed
 *     redo_com() since it doesn't do any good; other minor bug fixes
 *
 *     Revision 1.8  2000/07/03 18:00:37  lombard
 *     Added code to limit age of waiting packets; stops circ buffer overflows
 *     Added and Deleted some config params.
 *     Added check of K2 station name against restart file station name.
 *     See ChangeLog for complete list.
 *
 *     Revision 1.7  2000/06/18 20:17:46  lombard
 *     transport calls for status and hearbteats was inproperly commented out.
 *
 *     Revision 1.6  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.5  2000/05/17 15:12:42  lombard
 *     bug fix in ping_test
 *
 *     Revision 1.4  2000/05/16 23:39:16  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.3  2000/05/12 19:02:19  lombard
 *     fixed g_stnid typo
 *
 *     Revision 1.2  2000/05/12 04:04:05  lombard
 *     Fixed conversion of disk space units
 *
 *     Revision 1.1  2000/05/04 23:48:20  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2misc.c:  Miscellaneous functions for K2:            */
/*      init_blkmde, ping_test, get_status, get_params    */
/*                                                        */
/*    1/1/99 -- [ET]  File started                        */
/*                                                        */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <time.h>
#include "byteswap.h"
#include "glbvars.h"
#include "k2pktdef.h"        /* K2 packet definitions and types */
#include "k2ewerrs.h"        /* K2-to-Earthworm error codes */
#include "k2comif.h"         /* K2 COM port interface routines */
#include "k2pktio.h"         /* K2 packet send/receive routines */
#include "k2pktman.h"        /* K2 packet management functions */
#include "k2misc.h"          /* header file for this module */
#include "terminat.h"        /* for status/termination codes */
#include "k2info.h"          /* for info packet injection codes */

#define K2MI_INIT_TOUTMS  5000     /* packet timeout during init */
#define K2MI_PING_TOUTMS  1000     /* receive ping timeout in ms */
#define K2MI_PARM_TOUTMS  1000     /* receive params timeout in ms */
#define K2MI_TRAN_TOUTMS  500      /* transmission timeout */
#define K2MI_FLSH_SHORT   100      /* short flush interval in ms */
#define K2MI_FLSH_LONG    500      /* long flush interval in ms */
#define K2MI_MONMODE_MAX  9        /* number of times to try mon-mode cmd */
#define K2MI_BLKMODE_MAX  6        /* number of times to try block-mode cmd */
#define K2MI_PKT_SRCNUM   44       /* arbitrary packet source # for file */

/* buffer size for 'k2mi_init_blkmde()' fn: */
#define K2MI_IBFSIZ ((PACKET_MAX_SIZE-11)/5)

#define MAX_MSG_SIZE      256       /* Large enough for all faults in extended
                                       status message */
static char g_msg[MAX_MSG_SIZE];    /* Buffer for text messages */

static struct PACKET_HDR g_k2mi_hdrblk;          /* received header buffer */
static unsigned char g_k2mi_buff[PACKET_MAX_SIZE-9]; /* received data buffer */
static int rep_on_batt = 0, rep_low_batt = 0, rep_hwstat = 0;
static int rep_temp, rep_disk[2] = {0,0};

/**************************************************************************
 * k2mi_init_blkmde:  verifies communications with K2 unit. If K2 is      *
 *       already in block mode, then return. Otherwise, try to establish  *
 *       communications with the K2                                       *
 *       Sockets are never retried on timeouts                            *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  K2R_POSITIVE if K2 is already sending stream data  *
 *                     K2R_NO_ERROR K2 has been switched to block mode    *
 *                     K2R_ERROR if an error occurred                     *
 *                     K2R_TIMEOUT if a timeout occured while             *
 *                         communicating with K2                          *
 **************************************************************************/

int k2mi_init_blkmde(unsigned char *pseqnum)
{
  int blkmde_count = K2MI_BLKMODE_MAX;
  int monmde_count = K2MI_MONMODE_MAX;
  int dcnt, slen, rc1, rc2;
  unsigned int idx;
  static const char *monmde_str_arr[]={          /* monitor-mode strings */
                                 "\\\\\\\\\r", "\r","\\\\\\\r","\\","\r\\\\\\\\\r","\r\\\\\\\\\r", "\r\\\\\\\\\\\r", "\n\r\n\r", "\r\n", "\n\r\n\r\r"};
  static const char blkmde_str[]="BLOCK\r";      /* block-mode command string */

  idx = 0;                  /* initialize monitor-mode string array index */


  do     /* loop while attempting to activate and confirm K2 block mode */
  {
  	/* Tell heartbeat thread we're happy */
	  g_mt_working = 1;


    if (gcfg_debug > 3)
      logit("e", "flushing...");
    if (k2c_flush_recv(K2MI_FLSH_SHORT) != K2R_NO_ERROR)
      return K2R_ERROR;
    if (gcfg_debug > 3)
      logit("e", "done\n");

    /* check if data is coming in and if it's valid packets */
    if ( (rc1 = k2c_rcvflg_tout(K2MI_FLSH_LONG)) == K2R_ERROR)
    {
      return rc1;
    }
    else if (rc1 == K2R_NO_ERROR )
    {         /* Data ready to be read */
      if ( (rc2 = k2pm_recv_wstmpkt(&g_k2mi_hdrblk, g_k2mi_buff,
                                    gcfg_commtimeout_itvl, 0)) == K2R_POSITIVE)
      {
        if (gcfg_debug > 3)
          logit("e", "k2mi_init_blkmde: stream packet ret on try %d\n",
                blkmde_count - K2MI_BLKMODE_MAX);
        return K2R_POSITIVE;   /* Found K2 already streaming in block mode */

      }
      else if (rc2 == K2R_ERROR)
        return rc2;
    }

    /* no valid packets coming in */
    /* check if K2-ping can be sent and received */
    if ( (rc1 = k2mi_ping_test(1, pseqnum)) == K2R_NO_ERROR)
      return rc1;                   /* ping returned OK, comms are up */
    else if (rc1 == K2R_ERROR)
      return rc1;                   /* comm error; give up */

    monmde_count = K2MI_MONMODE_MAX;
    do
    {    /* loop while attempting to activate and confirm K2 monitor mode */

  	  /* Tell heartbeat thread we're happy */
	  g_mt_working = 1;

      /* send a monitor-mode string */
      if ( (slen = (int)strlen(monmde_str_arr[idx])) == 1)
      {
        /* if '\' (or '\r') then delay a bit before */
        if ( (rc1 = k2c_flush_recv(K2MI_FLSH_LONG)) == K2R_ERROR)
          return rc1;
      }

      if (gcfg_debug > 2)
        logit("e", "sending MM %u\n", idx);

      if ( (rc1 = k2c_tran_buff((const unsigned char *)(monmde_str_arr[idx]),
                                slen, gcfg_commtimeout_itvl, 0)) != slen)
        return rc1;     /* if error transmitting then return */

      /* increment index into monitor-mode string array */
      if(++idx >= (sizeof(monmde_str_arr)/sizeof(monmde_str_arr[0])))
        idx = 0;             /* if past end-of-array then wrap around to beg */

      /* wait a bit for (possible) data to appear */
      /*  (wait a bit longer if '\' (or '\r') command) */
      if ( (rc1 = k2c_rcvflg_tout( ((slen==1)?K2MI_FLSH_LONG:K2MI_FLSH_SHORT)))
           == K2R_ERROR)
        return rc1;

      /* save any data returned from K2 */
      if ( (dcnt = k2c_recv_buff(g_k2mi_buff, K2MI_IBFSIZ,
                                 gcfg_commtimeout_itvl, 0)) > 0)
      {       /* data was received */
        /* Tell heartbeat thread we're happy */
        g_mt_working = 1;

        g_k2mi_buff[dcnt] = '\0';           /* NULL terminate buffer */
        if (gcfg_debug > 2)
          logit("e", "received:  %d bytes '%s'\n", dcnt, g_k2mi_buff);

        /* check if returned data contains monitor-mode prompt ('*') */
        if ( strchr((char *)g_k2mi_buff, (int)'*') != NULL)
          break;             /* if monitor-mode prompt ('*') then exit loop */
      }
      else if (dcnt == K2R_ERROR)
        return dcnt;
    }
    while(--monmde_count);         /* loop if count not expired */
    /* Now we should be in monitor mode, unless monmde_count expired */

    /* send block-mode command string */
    slen = (int)strlen(blkmde_str);
    if (gcfg_debug > 2)
      logit("e", "%s\n",blkmde_str);

    if ( (rc1 = k2c_tran_buff((unsigned char *)blkmde_str, slen,
                                          gcfg_commtimeout_itvl,0)) != slen)
    {    /* error transmitting; return code */
      return (rc1 < 0) ? rc1 : K2R_ERROR;   /* only use code if negative */
                                            /* 1/29/2002 -- [ET] */
    }
    if ( (rc1 = k2c_flush_recv(K2MI_FLSH_SHORT)) == K2R_ERROR)
      return rc1;
  }
  while (--blkmde_count);           /* loop if count not expired */
  logit("et", "k2mi_init_blkmde: failed to put k2 in block mode\n");
  return K2R_ERROR;
}

/* returns -1 if str not found anywhere in buf, or position of first char of str */
char * find_string_(char* buf, char * str, int buf_len) 
{
int i;
int len_of_str;
char *cptr;

len_of_str= (int)strlen(str);
for (i=0; i< buf_len-len_of_str; i++) {
   if ( (cptr=strstr(&buf[i], str)) != NULL) {
      logit("et", "find_string_(): located string %s\n", cptr);
      return cptr;
   }
}
logit("et", "find_string_(): no %s in buffer\n", str);
return NULL;
}

/* returns -1 if MODEM not in buf, or position of first char of MODEM string if found 
	This was written because the raw data stream could have 0s in it which would
	nullify the strstr test! */
char * find_MODEM(char * buf, int len) {

char modem_str[]="MODEM";

     return find_string_(buf, modem_str, len);
}
char * find_PROMPT(char * buf, int len) {
char prompt_line_str[] = "\r * ";

     return find_string_(buf, prompt_line_str, len);
}



static int block_mode_started = 1;

/**************************************************************************
 * k2mi_force_blkmde:  forces communications with K2 unit into BLOCK mode.*
 *       Sockets are never retried on timeouts                            *
 *      return value:  K2R_POSITIVE has been switched to block mode       *
 *                     K2R_ERROR if an error occurred                     *
 *                     K2R_TIMEOUT if a timeout occured while             *
 *                         communicating with K2                          *
 **************************************************************************/
int k2mi_force_blkmde()
{
  int blkmde_count = K2MI_BLKMODE_MAX;
  int monmde_count = K2MI_MONMODE_MAX;
  int dcnt, slen, rc1;
  unsigned int idx;
  static const char *monmde_str_arr[]={          /* monitor-mode strings */
                                 "\n\r\n\r", "\r\n", "\n\r\n\r\r", "\\\\\\\\\r","\\\\\\\r","\\\\\\\\\r","\r\\\\\\\\\r","\n\r\n\r", "\r\n\\\\\\\\\r\r\n", "\\\\\\\\\r\n\r\\\\\\\\\r\n\r\r"};
  static const char blkmde_str[]="BLOCK\r";      /* block-mode command string */
  /* static const char ansmde_str[]="ANSWERMODE\r"; */      /* answermode command string */

  char *cptr;

  int local_timeout_ms = 250;
  int prompt_flag=0;

  idx = 0;                  /* initialize monitor-mode string array index */


  do     /* loop while attempting to activate and confirm K2 block mode */
  {
    /* Tell heartbeat thread we're happy */
    g_mt_working = 1;

    if (gcfg_debug > 2)
      logit("et", "k2mi_force_blkmde: flushing recv buffer...");
    if (k2c_flush_recv(K2MI_FLSH_SHORT) != K2R_NO_ERROR)
      return K2R_ERROR;
    if (gcfg_debug > 2)
      logit("e", "done\n");

    monmde_count = K2MI_MONMODE_MAX;
    do
    {    /* loop while attempting to activate and confirm K2 monitor mode */

      /* Tell heartbeat thread we're happy */
      g_mt_working = 1;

      /* send a monitor-mode string */

      if ( (slen = (int)strlen(monmde_str_arr[idx])) == 1)
      {
        /* if '\' (or '\r') then delay a bit before */
        if ( (rc1 = k2c_flush_recv(K2MI_FLSH_LONG)) == K2R_ERROR)
          return rc1;
      }

      if (gcfg_debug > 2)
        logit("et", "k2mi_force_blkmde: sending MM %u - string '%s'\n", idx, monmde_str_arr[idx]);

      if ( (rc1 = k2c_tran_buff((const unsigned char *)(monmde_str_arr[idx]),
                                slen, local_timeout_ms, 0)) != slen)
        return rc1;     /* if error transmitting then return */

      /* increment index into monitor-mode string array */
      if(++idx >= (sizeof(monmde_str_arr)/sizeof(monmde_str_arr[0])))
        idx = 0;             /* if past end-of-array then wrap around to beg */

      /* wait a bit for (possible) data to appear */
      /*  (wait a bit longer if '\' (or '\r') command) */
      if ( (rc1 = k2c_rcvflg_tout( ((slen==1)?K2MI_FLSH_LONG:K2MI_FLSH_SHORT)))
           == K2R_ERROR)
        return rc1;

      /* save any data returned from K2 - make sure not to redo socket conn */
      if ( (dcnt = k2c_recv_buff(g_k2mi_buff, K2MI_IBFSIZ,
                                 local_timeout_ms, 0)) > 0)
      {       /* data was received */
        /* Tell heartbeat thread we're happy */
        g_mt_working = 1;

        g_k2mi_buff[dcnt] = '\0';           /* NULL terminate buffer */
        if (gcfg_debug > 2)
          logit("et", "k2mi_force_blkmde: received:  %d bytes '%s'\n", dcnt, g_k2mi_buff);

        /* check if returned data contains modem command and its active,
		actual string provided by Dennis Pumphrey of KMI 2007-05-29 */
        cptr = find_MODEM((char *)g_k2mi_buff, dcnt);

        if (cptr == NULL && idx==3) {
          logit("et", "k2mi_force_blkmde: No MODEM strings upon \\r, so assuming BLOCK mode.\n");
	  return K2R_POSITIVE;
        }

	if (cptr != NULL && strstr((char*) cptr, "MODEM in control and active") != NULL) {
          if (gcfg_debug > 2)
            logit("et", "k2mi_force_blkmde: modem in control! not forcing block mode.\n");
          /* for now, just exit and let the modem do its thing till its done */
          return K2R_ERROR; 
          /* later we need to check a config setting if modems should be allowed to take precedent */
	}
	if (cptr != NULL && strstr((char*) cptr, "MODEM in control and inactive") != NULL) {
          if (gcfg_debug > 2)
            logit("et", "k2mi_force_blkmde: modem in control! but inactive, try forcing block mode.\n");
	    idx=4;
	}

        /* check if returned data contains monitor-mode prompt ('*') */
        if ( strchr((char *)g_k2mi_buff, (int)'*') != NULL || find_PROMPT((char *)g_k2mi_buff,dcnt)!=NULL) { 
          if (gcfg_debug > 2)
            logit("et", "k2mi_force_blkmde: received:  command prompt\n");
          prompt_flag=1;
          break;             /* if monitor-mode prompt ('*') then exit loop */
        }
      }
      else if (dcnt == K2R_ERROR)
        return dcnt;
      else if (gcfg_debug>2)
          logit("et", "k2mi_force_blkmde: received:  %d bytes  after timeout of %d ms'\n", dcnt, local_timeout_ms);
    }
    while(--monmde_count);         /* loop if count not expired */
 
    /* for tests to see if sending BLOCK at the end works, fire it in regardless of prompt 2007.12.05 */
    prompt_flag=1;  /* paulf 2007.12.05 */
    /* Now we should be in monitor mode, unless monmde_count expired */
    if (block_mode_started && prompt_flag) {
/*    unsigned char seqnum = 0; */
      /* send block-mode command string */
      slen = (int)strlen(blkmde_str);
      if (gcfg_debug > 2)
        logit("et", "k2mi_force_blkmde: sending %s\n",blkmde_str);

      if ( (rc1 = k2c_tran_buff((unsigned char *)blkmde_str, slen,
                                          local_timeout_ms,0)) != slen)
      {    /* error transmitting; return code */
        return (rc1 < 0) ? rc1 : K2R_ERROR;   /* only use code if negative */
                                            /* 1/29/2002 -- [ET] */
      }
      return K2R_POSITIVE; /* added 2007-11-07 to kick back faster for CGS firmware */

      /* new send start of streaming here for kicks */
/*    logit("et", "k2mi_force_blkmde: sent %s, now sending START_STREAM command\n",blkmde_str); */
/*                                                             */
/*    rc1 = k2pm_send_strctrl(K2SCC_START_STREAM, &seqnum, 0); */
/*                                                             */
/*    block_mode_started++; */ /* this is for when we alternated testing, could be removed 2007.12.05 */
    } 
    if (!prompt_flag) {
      logit("et", "k2mi_init_blkmde: no command prompt received, failed to go into block mode\n");
      return K2R_ERROR;
    }
    if ( (rc1 = k2c_flush_recv(K2MI_FLSH_SHORT)) == K2R_ERROR)
      return rc1;
    if (rc1 == 0) return K2R_POSITIVE;
  }
  while (--blkmde_count);           /* loop if count not expired */
  logit("et", "k2mi_init_blkmde: failed to put k2 in block mode\n");
  return K2R_ERROR;
}


/**************************************************************************
 * k2mi_ping_test:  executes ping test on K2                              *
 *        Sockets are never retried on timeout                            *
 *         count    - number of pings to send and confirm                 *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  returns K2R_NO_ERROR if `count' pings returned;    *
 *                     returns K2R_ERROR on error or K2R_TIMEOUT          *
 **************************************************************************/

int k2mi_ping_test(int count, unsigned char *pseqnum)
{
  int rc, slen;
  static const char ping_test_data[]=
                               "K2 ping test data \\\\\xC0\xC0\\1234567890";

  if (gcfg_debug > 2)
    logit("e", "ping test\n");

  /* check receive and flush any waiting data */
  if (gcfg_debug > 3)
    logit("e", "flushing...");
  if ( (rc = k2c_flush_recv(0)) != K2R_NO_ERROR)
    return rc;
  if (gcfg_debug > 3)
    logit("e", "done\n");

  while(1)    /* loop for each ping send/receive test */
  {           /* send ping packet */
    slen = (int)strlen(ping_test_data);     /* get string length */
    if (gcfg_debug > 3)
      logit("e", "sending ping\n");
    if ( (rc = k2p_send_packet((unsigned char)PKC_PING, *pseqnum,
                               K2MI_PKT_SRCNUM, slen,
                               (unsigned char *)ping_test_data, 0))
         != K2R_NO_ERROR)
      break;                 /* if error then exit loop */

    /* wait for expected response packet (ignore SDS auto-packets) */
    if ( (rc = k2pm_recv_waitpkt(PKR_PING, *pseqnum, K2MI_PKT_SRCNUM,
                                 &g_k2mi_hdrblk, g_k2mi_buff, PKR_STRDATA,
                                 gcfg_commtimeout_itvl, 0)) != K2R_POSITIVE)
    {
      if (rc == K2R_NO_ERROR)  /* something returned, but not a packet */
        rc = K2R_TIMEOUT;      /* call it a timeout */
      break;
    }
    if (rc == K2R_POSITIVE)  /* packet returned */
      rc = K2R_NO_ERROR;     /* switch return value to current convention */

    /* check packet data */
    if(g_k2mi_hdrblk.dataLength != slen ||
       strncmp((char *)g_k2mi_buff, ping_test_data, (size_t)slen) != 0)
    {    /* packet data mismatch */
      rc = K2R_ERROR;            /* setup error code */
      logit("et", "k2mi_ping_test: ping data mismatch\n");
      break;                           /* exit loop */
    }
    if(--count <= 0)              /* decrement count */
      break;                      /* if done then exit loop */
    ++(*pseqnum);                 /* increment sequence number */
  }
  ++(*pseqnum);         /* increment sequence number */
  if (gcfg_debug > 2)
    logit("e", "k2mi_ping_test returning %d\n", rc);

  return rc;            /* return OK code */
}

/**************************************************************************
 * k2mi_get_params:  fetches general K2 parameters (note that none of     *
 *      the numeric multi-byte entries are byte-swapped by this function) *
 *      Never does communication retries.                                 *
 *         pk2hdr  - address of header block to receive parameter data    *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  returns K2R_POSITIVE on return of param packet     *
 *                     K2R_NO_ERROR if no fatal errors occured but param  *
 *                       packet didn't come back;                         *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2mi_get_params(K2_HEADER *pk2hdr, unsigned char *pseqnum)
{
  int rc;
  size_t to_copy;

#ifdef DEBUG
  if(pk2hdr == NULL)              /* if NULL pointer then */
  {
    logit("et", "k2mi_get_params: NULL paramter pk2hdr\n");
    return K2R_ERROR;
  }
#endif

  /* check receive and flush any waiting data */
  if ( (rc = k2c_flush_recv(0)) != K2R_NO_ERROR)
    return rc;

  /* send get-K2-parameters command packet */
  if ( (rc = k2p_send_packet((unsigned char)PKC_GETPARMS, *pseqnum,
                             K2MI_PKT_SRCNUM, (unsigned short)0,
                             NULL, 0)) == K2R_NO_ERROR)
  {      /* send OK; wait for expected response packet (ignore message pkts) */
    if ( (rc = k2pm_recv_waitpkt((unsigned char)PKR_PARMS, *pseqnum,
                                 K2MI_PKT_SRCNUM, &g_k2mi_hdrblk,
                                 g_k2mi_buff, (unsigned char)PKC_MSG,
                                 gcfg_commtimeout_itvl, 0)) == K2R_POSITIVE)
    {         /* 'parameters' packet received OK */
      if (g_k2mi_hdrblk.dataLength == (unsigned short)K2_HEAD_SIZE)
      {       /* packet size OK */
        memset(pk2hdr, 0, sizeof(*pk2hdr));
        /* copy data into K2 parameters header block to be returned */
        /*  (copy smaller of # of bytes received or struct size) */
        to_copy = (sizeof(*pk2hdr) < K2_HEAD_SIZE) ?
          sizeof(*pk2hdr) : K2_HEAD_SIZE;
        memcpy(pk2hdr, g_k2mi_buff, to_copy);
      }
      else    /* unexpected packet size */
      {
        logit("et", "k2mi_get_params: wrong sized packet returned from K2\n");
        return K2R_NO_ERROR;
      }
    }
  }
  ++(*pseqnum);         /* increment sequence number */
  return rc;            /* return code */
}


/**************************************************************************
 * k2mi_get_status:  fetches K2 status block (note that none of the       *
 *      numeric multi-byte entries are byte-swapped by this function)     *
 *         pk2stat  - address of STATUS_INFO block to receive status data *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  returns K2R_POSITIVE on return of status packet;   *
 *                     K2R_NO_ERROR if no fatal errors occured but status *
 *                       packet didn't return;                            *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2mi_get_status(struct STATUS_INFO *pk2stat, unsigned char *pseqnum)
{
  int rc;
  size_t to_copy;

#ifdef DEBUG
  if (pk2stat == NULL)             /* if NULL pointer then */
  {
    logit("et", "k2mi_get_status: NULL paramter pk2stat\n");
    return K2R_ERROR;
  }
#endif

  /* check receive and flush any waiting data */
  if ( (rc = k2c_flush_recv(0)) != K2R_NO_ERROR)
    return rc;

  /* send get-K2-status command packet */
  if ( (rc = k2p_send_packet((unsigned char)PKC_GETSTATUS, *pseqnum,
                             K2MI_PKT_SRCNUM, (unsigned short)0,
                             NULL, 0)) == K2R_NO_ERROR)
  {      /* send OK; wait for expected response packet (ignore none) */
    if ( (rc = k2pm_recv_waitpkt((unsigned char)PKR_STATUS, *pseqnum,
                                 K2MI_PKT_SRCNUM, &g_k2mi_hdrblk,
                                 g_k2mi_buff, (unsigned char)K2PM_IGNR_NONE,
                                 gcfg_commtimeout_itvl, 0)) == K2R_POSITIVE)
    {         /* 'parameters' packet received OK */
      if (g_k2mi_hdrblk.dataLength == (unsigned short)STATUS_SIZE)
      {       /* packet size OK */
        memset(pk2stat, 0, sizeof(*pk2stat));
        /* copy data into K2 parameters status block to be returned */
        /*  (copy smaller of # of bytes received or struct size) */
        to_copy = (sizeof(*pk2stat) < STATUS_SIZE) ?
          sizeof(*pk2stat) : STATUS_SIZE;
        memcpy(pk2stat, g_k2mi_buff, to_copy);
      }
      else    /* unexpected packet size */
      {
        logit("et", "k2mi_get_status: wrong sized packet returned from K2\n");
        return K2R_NO_ERROR;
      }
    }
  }
  ++(*pseqnum);         /* increment sequence number */
  return rc;            /* return code */
}

/**************************************************************************
 * k2mi_get_extstatus:  fetches K2 extended status block (note that none  *
 *      of the numeric multi-byte entries are byte-swapped by this        *
 *      function)                                                         *
 *         pk2stat  - address of EXT_STATUS_INFO block to receive         *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  returns K2R_POSITIVE on return of status packet;   *
 *                     K2R_NO_ERROR if no fatal errors occured but status *
 *                       packet didn't return;                            *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2mi_get_extstatus(struct EXT2_STATUS_INFO *pk2stat, unsigned char *pseqnum,
		       int *pext_size)
{
  int rc;
  size_t to_copy;

#ifdef DEBUG
  if (pk2stat == NULL)             /* if NULL pointer then */
  {
    logit("et", "k2mi_get_extstatus: NULL paramter pk2stat\n");
    return K2R_ERROR;
  }
#endif

  /* check receive and flush any waiting data */
  if ( (rc = k2c_flush_recv(0)) != K2R_NO_ERROR)
    return rc;

  /* send get-K2-status command packet */
  if ( (rc = k2p_send_packet((unsigned char)PKC_EXTSTATUS, *pseqnum,
                             K2MI_PKT_SRCNUM, (unsigned short)0,
                             NULL,0)) == K2R_NO_ERROR)
  {      /* send OK; wait for expected response packet (ignore none) */
    if ( (rc = k2pm_recv_waitpkt((unsigned char)PKR_EXTSTATUS, *pseqnum,
                                 K2MI_PKT_SRCNUM, &g_k2mi_hdrblk,
                                 g_k2mi_buff, (unsigned char)K2PM_IGNR_NONE,
                                 gcfg_commtimeout_itvl, 0)) == K2R_POSITIVE)
    {         /* 'parameters' packet received OK */
      if (g_k2mi_hdrblk.dataLength == (unsigned short)EXT_SIZE)
      {       /* packet size OK */
        memset(pk2stat, 0, sizeof(*pk2stat));
        /* copy data into K2 parameters status block to be returned */
        /*  (copy smaller of # of bytes received or struct size) */
        to_copy = (sizeof(*pk2stat) < EXT_SIZE) ? sizeof(*pk2stat) : EXT_SIZE;
        memcpy(pk2stat, g_k2mi_buff, to_copy);
	*pext_size = EXT_SIZE;
      }
      else if (g_k2mi_hdrblk.dataLength == (unsigned short)EXT2_SIZE)
      {
	  memset(pk2stat, 0, sizeof(*pk2stat));
	  /* copy data into K2 parameters status block to be returned */
	  /*  (copy smaller of # of bytes received or struct size) */
	  to_copy = (sizeof(*pk2stat) < EXT2_SIZE) ? sizeof(*pk2stat) : EXT2_SIZE;
	  memcpy(pk2stat, g_k2mi_buff, to_copy);
	  *pext_size = EXT2_SIZE;
      }
    else    /* unexpected packet size */
      {
        logit("et","k2mi_get_extstatus: wrong sized packet returned from"
              " K2 (was %d, should be %d or %d)\n",
	      (int)(g_k2mi_hdrblk.dataLength),EXT_SIZE, EXT2_SIZE);
	*pext_size = 0;
        return K2R_NO_ERROR;
      }
    }
  }
  ++(*pseqnum);         /* increment sequence number */
  return rc;            /* return code */
}

/**************************************************************************
 * k2mi_req_status:  requests K2 status block                             *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  returns K2R_NO_ERROR if request is transmitted     *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2mi_req_status(unsigned char *pseqnum)
{
  int rc;

  /* send get-K2-status command packet */
  rc = k2p_send_packet((unsigned char)PKC_GETSTATUS, *pseqnum,
                       K2MI_PKT_SRCNUM, (unsigned short)0,
                       NULL,1);
  ++(*pseqnum);         /* increment sequence number */
  return rc;            /* return code */
}

/**************************************************************************
 * k2mi_req_extstatus:  requests K2 extended status block                 *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  returns K2R_NO_ERROR if request is transmitted     *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2mi_req_extstatus(unsigned char *pseqnum)
{
  int rc;

  /* send get-K2-status command packet */
  rc = k2p_send_packet((unsigned char)PKC_EXTSTATUS, *pseqnum,
                       K2MI_PKT_SRCNUM, (unsigned short)0,
                       NULL,1);
  ++(*pseqnum);         /* increment sequence number */
  return rc;            /* return code */
}


/**************************************************************************
 * k2mi_req_params:  requests K2 paramter block                            *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *      return value:  returns K2R_NO_ERROR if request is transmitted     *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2mi_req_params(unsigned char *pseqnum)
{
  int rc;

  /* send get-K2-params command packet */
  rc = k2p_send_packet((unsigned char)PKC_GETPARMS, *pseqnum,
                       K2MI_PKT_SRCNUM, (unsigned short)0,
                       NULL,1);
  ++(*pseqnum);         /* increment sequence number */
  return rc;            /* return code */
}


/**************************************************************************
 * k2mi_log_pktinfo:  logs information about given packet                 *
 *         phdrblk  - address of header block for packet                  *
 *         databuff - address of data buffer for packet                   *
 **************************************************************************/

void k2mi_log_pktinfo(struct PACKET_HDR *phdrblk, unsigned char *databuff)
{
  int slen, index;
  uint32_t alarm, chanmask = 0x00000001;

  switch(phdrblk->typeCode)
  {      /* process packet based on type-code */
  case PKC_MSG:            /* system message packet from K2 */
    /* Ignore these messages */
    if (strncmp((char *)databuff, "CAUTION: Acquisition is ON", 26) == 0)
       break;

    /* Log the message string (if not too long) */
    logit("et","K2 message received: %s\n",
          (strlen((char *)databuff) < PACKET_MAX_SIZE-11) ?
          (char *)databuff: "<unterminated-string>");

    /* If flash memory error is detected, send msg to statmgr */
    /* Added in version 2.26 by W Kohler, added station name to message in 2.35  */
    if ( strncmp((char *)databuff, "pcdrv_raw_read, error", 21) == 0 ) 
    {
       sprintf(g_msg, "K2 <%s>: pcdrv_raw_read error (possibly bad flash memory)",
                g_stnid);
       k2mi_status_hb(g_error_ltype, K2STAT_BAD_DISK, g_msg);
    }
    if ( strncmp((char *)databuff, "pcdrv_raw_write, error", 22) == 0 )
    {
       sprintf(g_msg, "K2 <%s>: pcdrv_raw_write error (possibly bad flash memory)",
                g_stnid);
       k2mi_status_hb(g_error_ltype, K2STAT_BAD_DISK, g_msg);
    }
    break;
  case PKC_SETALARM:       /* alarm message from K2 */
    /* display alarm as 32-bit int (if length OK) */
    if(phdrblk->dataLength == (unsigned short)4)
    {
      sprintf(g_msg, "K2 alarm received for channels: ");
      slen = (int)strlen(g_msg);
      alarm = BYTESWAP_UINT32(((struct K2_ALARM*)databuff)->channelBitMap);
      for (index = 0; index < ABS_MAX_CHANNELS; index++)
      {
        if (alarm & chanmask)
        {
          sprintf(&g_msg[slen], "%d ", index + 1);
          slen = (int)strlen(g_msg);
        }
        chanmask = chanmask << 1;
      }
      logit("et","%s\n", g_msg);
    }
    else
      logit("et","K2 alarm: <unexpected data-length>\n");
    break;
  case PKR_STRDATA:        /* serial data stream auto packet */
  case PKR_STRRQDATA:      /* serial data stream requested packet */
    if ( gcfg_debug > 0 )
       logit("et","Unexpected SDS (%s) packet, "
             "seq-num=%hu, src-num=%hu, data-len=%hu\n",
             ((phdrblk->typeCode == (unsigned char)PKR_STRDATA) ?
             "PKR_STRDATA" : "PKR_STRRQDATA"),
             (unsigned short)(phdrblk->seqNo), phdrblk->source,
             phdrblk->dataLength);
    break;
  case PKR_STATUS:
    if (phdrblk->dataLength == (unsigned short) STATUS_SIZE)
      k2mi_report_status((struct STATUS_INFO *)databuff);
    else
      logit("et", "k2ew: abnormal length status packet received\n");
    break;
 case PKR_EXTSTATUS:
    if (phdrblk->dataLength == (unsigned short)EXT_SIZE)
      k2mi_report_extstatus((struct EXT2_STATUS_INFO *)databuff, EXT_SIZE);
    else if (phdrblk->dataLength == (unsigned short)EXT2_SIZE)
      k2mi_report_extstatus((struct EXT2_STATUS_INFO *)databuff, EXT2_SIZE);
    else
    {
      logit("et", "k2ew: abnormal length extended status packet"
            " received (%d)\n",(int)(phdrblk->dataLength));
      if (phdrblk->dataLength == (unsigned short)(EXT_SIZE*2))
      {  /* extended packet is new double-length size */
        g_extstatus_avail = 0;    /* clear flag to stop future requests */
        if(gcfg_debug > 0)
        {     /* debug enabled; log message */
          logit("et",
                   "Extended status packets will no longer be requested\n");
        }
      }
    }
    break;
  case PKR_PARMS:
    if (phdrblk->dataLength == (unsigned short)K2_HEAD_SIZE)
      k2mi_report_params((K2_HEADER *)databuff);
    else
      logit("et", "k2ew: abnormal length params packet received\n");
    break;
  case PKR_STRNAK:
    logit("et", "K2 STR_NAK rcvd: %s\n",
          k2pm_strnak_errcd(phdrblk, databuff, NULL));
    break;
  default:       /* unknown packet type-code, display message */
    logit("et","Unexpected packet received, type-code=%02hXH, "
          "seq-num=%hu, src-num=%hu, data-len=%hu\n",
          (unsigned short)(phdrblk->typeCode),
          (unsigned short)(phdrblk->seqNo),
          phdrblk->source, phdrblk->dataLength);
  }
}

/*****************************************************************
 * k2mi_report_status: Report information form K2 status packet  *
 *         pk2stat - pointer to K2 status structure              *
 *****************************************************************/

void k2mi_report_status( struct STATUS_INFO *pk2stat)
{
  time_t timevar;
  unsigned short wtemp1, wtemp2;
  char tempchar = (char)0, disk[2] = {'A', 'B'};
  int tempbat, index;
  double batv, diskfree = 0.0;
  static char msg2[MAX_MSG_SIZE];      /* another buffer for text messages */

#ifdef ENABLE_OLD_INJECTION
  if (gcfg_inject_info==1)
	k2info_send(K2INFO_TYPE_STATUS, (char *) pk2stat);
#endif

  /* System time and clock source */
  timevar = (time_t)(BYTESWAP_UINT32(pk2stat->systemTime) + K2_TIME_CONV);
  sprintf(g_msg, "K2 Time = %s", asctime(gmtime(&timevar)));
  g_msg[strlen(g_msg)-1] = '\0';         /* delete the newline left by ctime() */

  switch (pk2stat->clockSource)
  {
  case 0:
    strcat(g_msg, " set from RTC");
    break;
  case 1:
    strcat(g_msg, " set from Keyboard");
    break;
  case 2:
    strcat(g_msg, " set from External Pulse");
    break;
  case 3:
    strcat(g_msg, " set from Internal GPS");
    break;
  default:
    strcat(g_msg, " set from (?)");
  }
  logit("et","%s\n", g_msg);

  /* Battery voltage and hardware status */
  tempbat = (int) pk2stat->batteryStatus;   /* char, no need to swap */
  batv = tempbat/10.0;
  if (tempbat < 0)
  {
    batv = -batv;
  }
  if (gcfg_inject_info==1)
  {
        char msg[500];
        sprintf(msg, "K2 Internal Battery Voltage=%5.2f", batv);
	k2snw_send(msg);
  }
  if (tempbat > 0) /* K2 says battery voltage is zero when it is charging */
  {
    logit("et", "K2 HW Status = %s Battery Status: %0.1f V\n",
          (pk2stat->hardwareStatus == (unsigned char)SF_OK)?"OK":"FAULT", batv);
    if (gcfg_on_batt && rep_on_batt == 0)
    {
    sprintf(g_msg, "K2 <%s>: lost external power; battery voltage: %0.1f V", 
		g_stnid, batv);
    k2mi_status_hb(g_error_ltype, K2STAT_ON_BATT, g_msg);
    rep_on_batt = 1;
    }
  }
  else
  {
    logit("et", "K2 HW Status = %s Battery Status: charging\n",
          (pk2stat->hardwareStatus == (unsigned char)SF_OK)?"OK":"FAULT");
    if (rep_on_batt == 1 && gcfg_on_batt)
    {
    sprintf(g_msg, "K2 <%s>: external power restored", g_stnid);
    k2mi_status_hb(g_error_ltype, K2STAT_OFF_BATT, g_msg);
    rep_on_batt = 0;
    rep_low_batt = 0;
    }
  }

  if (gcfg_battery_alarm > 0 && rep_low_batt == 0 && tempbat > 0
      && batv < 0.1 * gcfg_battery_alarm)
  {
    sprintf(g_msg, "K2 <%s>: low battery voltage: %0.1f V", 
		g_stnid, batv);
    k2mi_status_hb(g_error_ltype, K2STAT_LOW_BATT, g_msg);
    rep_low_batt = 1;
  }
  /* We assume battery voltage goes up only when external power is restored */

  if (gcfg_hwstat_alarm && rep_hwstat == 0
      && pk2stat->hardwareStatus == (unsigned char) SF_FAULT)
  {
    sprintf(g_msg, "K2 <%s> hardware fault", g_stnid);
    k2mi_status_hb(g_error_ltype, K2STAT_HW_FAULT, g_msg);
    rep_hwstat = 1;
  }
  else if (gcfg_hwstat_alarm && rep_hwstat == 1)
  {
    sprintf(g_msg, "K2 <%s> hardware fault cleared", g_stnid);
    k2mi_status_hb(g_error_ltype, K2STAT_HWFLT_CLR, g_msg);
    rep_hwstat = 0;
  }

  /* Event and recording error counts */
  logit("et", "Events: %hu Recording Errors: %hu\n",
        BYTESWAP_UINT16(pk2stat->events),
        BYTESWAP_UINT16(pk2stat->recordingErrors));

  /* Trigger status */
  wtemp1 = BYTESWAP_UINT16(pk2stat->triggerStatus);
  sprintf(g_msg, "Acquisition: ");
  if (wtemp1 & 0x0001)
  {
    strcat(g_msg, "on (");
    if (((wtemp1 & 0x0002) == 0) &&
        ((wtemp1 & 0x0004) == 0) &&
        ((wtemp1 & 0x0010) == 0))
      strcat(g_msg, "not ");
    strcat(g_msg, "triggered);");
  }
  else
    strcat(g_msg, "off;");

  strcat(g_msg, "  Alarm: ");
  if ((wtemp1 & 0x0008) == 0)
    strcat(g_msg, "not ");
  strcat(g_msg, "triggered.");
  logit("et", "%s\n", g_msg);

  /* Disk drive status */
  strcpy(g_msg, "Drive:");
  for (index = 0; index < 2; index++)
  {
    sprintf(&g_msg[strlen(g_msg)], " %c: ", 'A' + index);
    wtemp1 = BYTESWAP_UINT16(pk2stat->driveStatus[index]);
    if (wtemp1 & SF_NOT_READY)
    {
      strcat(g_msg, "not ready");
      if ( gcfg_disk_alarm[index] > 0.0 && rep_disk[index] == 0)
      {
        sprintf(msg2, "K2 <%s>: disk %c failure", 
		g_stnid, disk[index]);
        k2mi_status_hb(g_error_ltype, K2STAT_BAD_DISK, msg2);
        rep_disk[index] = 1;
      }
    }
    else
    {
      if ((wtemp1 & SF_GB) == SF_GB)
      {
        tempchar = 'G';
        diskfree = 1073741824.0;   /* `giga' bytes */
      }
      else
      {
        if ((wtemp1 & SF_MB) == SF_MB)
        {
          tempchar = 'M';
          diskfree = 1048576.0;  /* `mega' bytes */
        }
        else
        {
          if ((wtemp1 & SF_KB) == SF_KB)
          {
            tempchar = 'K';
            diskfree = 1024.0;  /* `Kilo' bytes */
          }
        }
      }
      wtemp2 = wtemp1 & SF_FREE;
      diskfree *= (double)wtemp2;
      sprintf(&g_msg[strlen(g_msg)], "%d %cB FREE", wtemp2, tempchar);
      if (gcfg_inject_info==1)
      {
        char msg[500];
        int disk_kb;
   	disk_kb = wtemp2;
        if (tempchar == 'M') disk_kb *= 1000;
        if (tempchar == 'G') disk_kb *= 1000000;
        sprintf(msg, "K2 Disk Drive %c Free Space Kb=%d", 'A' + index, disk_kb);
	k2snw_send(msg);
      }

      if (gcfg_disk_alarm[index] > 0.0 && diskfree < gcfg_disk_alarm[index]
          && rep_disk[index] == 0)
      {
        sprintf(msg2, "K2 <%s>: low free space on disk A: %d %cB",
                g_stnid, wtemp2, tempchar);
        k2mi_status_hb(g_error_ltype, K2STAT_LOW_DISK, msg2);
        rep_disk[index] = 1;
      }
      else if (gcfg_disk_alarm[index] > 0.0 && rep_disk[index] == 1
               && diskfree >= gcfg_disk_alarm[index] )
      {   /* Disk is no longer full */
        sprintf(msg2, "K2 <%s>: disk space OK; A: %d %cB",
                g_stnid, wtemp2, tempchar);
        k2mi_status_hb(g_error_ltype, K2STAT_OK_DISK, msg2);
        rep_disk[index] = 0;
      }
    }
  }
  logit("et", "%s\n", g_msg);
}

/*****************************************************************
 * k2mi_report_extstatus: Report information form K2 extended    *
 *             status packet                                     *
 *   It seems the K2 doesn't actually support this message, so   *
 *   don't expect this function to be used: PNL 8/22/00          *
 *   Since the data reported here is not included in the params  *
 *   structure, we keep this code around, just in case KMI gets  *
 *   ambitious and implements what they advertise; hah!          *
 *         pk2stat - pointer to K2 extended status structure     *
 *****************************************************************/

void k2mi_report_extstatus( struct EXT2_STATUS_INFO *pk2stat, int ext_size)
{
  unsigned short us_dummy;
  uint32_t ul_dummy;
  unsigned short fault;
  time_t timevar;
  char msg[200];
  
  /* The extended status structure is not byte-aligned for Sparc or Intel *
   * hardware. So we have to copy out the bytes into dummy variables      *
   * before we can manipluate them. Thanks, kinemetrics!                  */

  /* Last restart time */
  memcpy(&ul_dummy, ((char *)pk2stat) + 2, sizeof(uint32_t));
  timevar = (time_t)(BYTESWAP_UINT32(ul_dummy) + K2_TIME_CONV);
  switch (pk2stat->clockSource)
  {
  case 0:
    strcpy(msg, "RTC");
    break;
  case 1:
    strcpy(msg, "Keyboard");
    break;
  case 2:
    strcpy(msg, "External Pulse");
    break;
  case 3:
    strcpy(msg, "Internal GPS");
    break;
  default:
    strcpy(msg, "(?)");
  }
  logit("e", "K2 Extended Status Message: last restart: Time = %s set from %s\n",
        asctime(gmtime(&timevar)), msg);

  /* Fault status */
  memcpy(&us_dummy, ((char *)pk2stat) + 6, sizeof(short));
  fault = BYTESWAP_UINT16(us_dummy);
  strcpy(msg, "K2 fault status: ");
  if (fault != (unsigned short)0)
  {
    if (fault & FAULT_SYSTEM)
      strcat(msg, "bad parameters; ");
    if (fault & FAULT_FLASH)
      strcat(msg, "flash error; ");
    if (fault & FAULT_RAM)
      sprintf(&msg[strlen(msg)], "ram error at %X; ",
              (unsigned)BYTESWAP_UINT32(pk2stat->lastRAMError));
    if (fault & FAULT_PCMCIA)
      strcat(msg, "bad/missing PCMCIA; ");
    if (fault & FAULT_DSP)
      strcat(msg, "failed to load DSP; ");
    if (fault & FAULT_PARMBLK)
      strcat(msg, "param block CRC error; ");
    if (fault & FAULT_FLASH_MAINTENANCE)
      strcat(msg, "flash maintenance required;");

    /* I assume that a fault here will also be reported by the HW fault bit
       in the status message, so we don't bother sending and earthworm status
       message here */
  }
  else
    strcat(msg, "OK");

  logit("e", "%s\n", msg);

  /* inject it if requested to */
#ifdef ENABLE_OLD_INJECTION
  if (gcfg_inject_info==1 && ext_size==EXT_SIZE)
	k2info_send(K2INFO_TYPE_ESTATUS, (char *) pk2stat);
  else if (gcfg_inject_info==1 && ext_size==EXT2_SIZE)
	k2info_send(K2INFO_TYPE_E2STATUS, (char *) pk2stat);
#endif

  if (ext_size == EXT_SIZE) return;
  
  /* Report additional information in the "extended" extended status packet */

  return;
}


/*****************************************************************
 * k2mi_write_header: Write binary K2 header to disk file        *
 *         pk2hdr - pointer to K2 header structure               *
 *****************************************************************/

static void k2mi_write_header( K2_HEADER *k2hdr )
{
   FILE *fp = fopen( gcfg_header_filename, "wb" );
   if ( fp == NULL )
   {
      logit( "e", "Can't open new K2 header file: <%s>\n", gcfg_header_filename );
      return;
   }
   if ( fwrite( k2hdr, sizeof(K2_HEADER), 1, fp ) < 1 )
   {
      logit( "e", "Error writing K2 header file: <%s>\n", gcfg_header_filename );
      fclose( fp );
      return;
   }
   fclose( fp );           /* All ok */
   if (gcfg_debug > 0)
      logit( "et", "K2 header file written: <%s>\n", gcfg_header_filename );
   return;
}


/*****************************************************************
 * k2mi_report_params: Report some information from K2 param     *
 *             packet: temperature and GPS                       *
 *         pk2hdr - pointer to K2 param structure                *
 *****************************************************************/

void k2mi_report_params( K2_HEADER *pk2hdr)
{
  short batt_voltx10;
  double batv;
  short temp;

/* Write K2 header to a disk file
   ******************************/
   if (gcfg_header_filename[0] != '\0')
      k2mi_write_header( pk2hdr );

#ifdef ENABLE_OLD_INJECTION
   if (gcfg_inject_info == 1)
	k2info_send(K2INFO_TYPE_HEADER, (char *) pk2hdr);
#endif


/* System temperature
   ******************/
  temp = (short)BYTESWAP_UINT16(pk2hdr->roParms.misc.temperature);
  logit("et", "Temperature: %d.%1d deg C\n", temp / 10, temp % 10);
  if (gcfg_inject_info==1)
  {
        char msg[500];
        double temperature;
 	temperature = temp/10.0;
        sprintf(msg, "K2 Temperature in Celsius=%5.1f", temperature);
	k2snw_send(msg);
  }

  if ((int)temp < gcfg_templo_alarm && rep_temp == 0)
  {
    sprintf(g_msg, "K2 <%s>: low temperature: %d.%1d C", 
		g_stnid, temp / 10, temp % 10);
    k2mi_status_hb(g_error_ltype, K2STAT_LOW_TEMP, g_msg);
    rep_temp = 1;
  }
  else if ((int)temp > gcfg_temphi_alarm && rep_temp == 0)
  {
    sprintf(g_msg, "K2 <%s>: high temperature: %d.%1d C", 
		g_stnid, temp / 10, temp % 10);
    k2mi_status_hb(g_error_ltype, K2STAT_HIGH_TEMP, g_msg);
    rep_temp = 1;
  }
  else if (rep_temp == 1)
  {
    sprintf(g_msg, "K2 <%s>: temperature OK: %d.%1d C", 
		g_stnid, temp / 10, temp % 10);
    k2mi_status_hb(g_error_ltype, K2STAT_OK_TEMP, g_msg);
    rep_temp = 0;
  }

/* Log the last GPS lock time and drift. If the GPS hasn't locked
   for gcfg_gpslock_alarm hours, send one message to statmgr.
   If and when the GPS locks up again, send another msg to statmgr.
   ***************************************************************/
   {
      const int year2000 = 946080000;   /* Jan 1, 2000 in seconds */

      time_t now      = time(0);        /* PC clock time */
      time_t timevar1 = (time_t)(BYTESWAP_UINT32(pk2hdr->roParms.timing.gpsLastLockTime[0])
                         + K2_TIME_CONV);
      char *timestr1  = asctime(gmtime(&timevar1));

      timestr1[strlen(timestr1)-1] = '\0';
      temp = (short)BYTESWAP_UINT32(pk2hdr->roParms.timing.gpsLastDrift[0]);
      logit("et", "Last GPS lock: %s  Drift: %d msec\n", timestr1, temp );

      if (gcfg_inject_info==1 && (now > year2000) && (timevar1 > year2000))
      {
               char msg[500];
               double snw_hoursSinceLastLock = (now - timevar1) / 3600.;
               sprintf(msg, "K2 Hours Since Last GPS Lock=%.1lf", snw_hoursSinceLastLock);
	       k2snw_send(msg);
      }

      if ( (gcfg_gpslock_alarm >= 0.0) && (now > year2000) && (timevar1 > year2000) )
      {
         static int prev_gpslock  = 1;            /* Assume GPS was locked at startup */
         double timeSinceLastLock = (now - timevar1) / 3600.;

         if ( timeSinceLastLock >= gcfg_gpslock_alarm )
         {
            logit( "et", "WARNING. GPS hasn't locked for %.1lf hours.\n", timeSinceLastLock );
            if ( prev_gpslock == 1 )
            {
               sprintf(g_msg, "K2 <%s>: No GPS lock for %.1lf hours.", 
			g_stnid, timeSinceLastLock );
               k2mi_status_hb(g_error_ltype, K2STAT_GPSLOCK, g_msg);
            }
            prev_gpslock = 0;
         }
         else
         {
            if ( prev_gpslock == 0 ) 
	    {
               sprintf(g_msg, "K2 <%s>: GPS lock acquired.", g_stnid);
               k2mi_status_hb(g_error_ltype, K2STAT_GPSLOCK, g_msg);
	    }
            prev_gpslock = 1;
         }
      }
   }

  /* Battery voltage is reported as negative when supplied from external *
   * source. That's what we need here; internal battery voltage is       *
   * reported in status block, above */
  batt_voltx10 = (signed short)BYTESWAP_UINT16(pk2hdr->roParms.misc.batteryVoltage);

  if (batt_voltx10 < 0)
  {
    batt_voltx10 = -batt_voltx10;
    batv = batt_voltx10 / 10.0;
    logit("et", "External battery voltage: %0.1f V\n", batv);
  }

  /* If k2ew used a restart file to read the station name
   * (g_validrestart_flag == 1), we must verify that name against
   * the K2's idea of station name. If they don't match, someone
   * must have used a non-unique restart-file name in the config file.
   * If k2ew continues, then the trace data will be labeled with the
   * wrong station name: Bad doodoo! */
  if (g_validrestart_flag == 1)
  {      /* restart file was used */
    if(strncmp(g_k2_stnid,pk2hdr->rwParms.misc.stnID,sizeof(g_k2_stnid)-1) != 0
                && strncmp(g_stnid,gcfg_stnid_remap,sizeof(g_stnid)-1) != 0)
    {         /* original station name and remapped name do not match */
      logit("et","k2ew: MAJOR ERROR: K2 station name <%s> does not match "
                 "station name <%s>\n"
                 "\tfrom restart file (\"%s\"); Terminating!\n",
              pk2hdr->rwParms.misc.stnID,g_k2_stnid, gcfg_restart_filename);
      g_terminate_flg = 1;        /* set program terminate flag */

    }
    else      /* name matches OK */
      g_validrestart_flag = 2;    /* so we don't do this test again */
  }
}


/*************************************************************
 * k2mi_status_hb: sends heartbeat or status messages to     *
 *         earthworm transport ring.                         *
 *       type: the message type: heartbeat or status         *
 *       code: the error code for status messages            *
 *       message: message text, if any                       *
 *************************************************************/

void k2mi_status_hb(unsigned char type, short code, char* message )
{
  char          outMsg[MAX_MSG_SIZE];  /* The outgoing message.        */
  time_t        msgTime;        /* Time of the message.                 */

  /*  Get the time of the message                                       */
  time( &msgTime );

  /*  Build & process the message based on the type                     */
  if ( g_heartbeat_logo.type == type )
  {
    sprintf( outMsg, "%ld %d\n", (long) msgTime, g_pidval );

    /*Write the message to the output region                            */
    if ( tport_putmsg( &g_tport_region, &g_heartbeat_logo,
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                       */
      logit( "et", "k2ew: Failed to send a heartbeat message (%d).\n",
             code );
    }
  }
  else
  {
    if ( message )
    {
      sprintf( outMsg, "%ld %hd %s\n", (long) msgTime, code, message );
      logit("et","Error:%d (%s)\n", code, message );
    }
    else
    {
      sprintf( outMsg, "%ld %hd\n", (long) msgTime, code );
      logit("et","Error:%d (No description)\n", code );
    }

    /*Write the message to the output region                         */
    if ( tport_putmsg( &g_tport_region, &g_error_logo,
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                    */
      logit( "et", "k2ew: Failed to send an error message (%d).\n",
             code );
    }
  }
}
