/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rp_messaging.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log: rp_messaging.c,v $
 *     Revision 1.4  2010/08/31 18:38:47  davidk
 *     Fixed bug in WriteError() where it wasn't writing statmgr parsable messages.
 *
 *     Revision 1.3  2010/08/30 16:22:27  davidk
 *     Added a standard earthwormy function to log an error to a ring for statmgr
 *     (standard *_status function found in most EW modules)
 *
 *     Revision 1.2  2009/02/13 20:40:36  mark
 *     Added SCNL parsing library
 *
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.2  2007/06/08 16:51:08  mark
 *     Changed pickfile writing to commit to disk immediately
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.12  2005/06/20 20:57:30  cjbryan
 *     update error messaging to use reportError
 *
 *     Revision 1.11  2005/02/28 20:35:41  davidk
 *     Moved code that issues earthworm hearbeat into WriteHeartbeat() function
 *     in rp_messaging.c.
 *
 *     Revision 1.10  2005/01/06 21:18:18  labcvs
 *     Backed out of previous change, better solution implemented.  JMP
 *
 *     Revision 1.9  2005/01/05 21:32:20  labcvs
 *     Missed a comma in new code.  JMP
 *
 *     Revision 1.8  2005/01/05 21:24:50  labcvs
 *     Put in fix (kluge) to TimeToDTString to correct the Malformed Picks problem.
 *     Error was caused by fractional seconds + roundoff being greater than one second.
 *     This required a change to the ReportPick function to check a return code.  JMP + AB
 *
 *     Revision 1.7  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.6  2004/08/27 20:39:35  labcvs
 *     disabled sending of ml and mbLg
 *
 *     Revision 1.5  2004/07/21 01:22:27  labcvs
 *     revision and Id expansions different in header comments section.
 *      needtosendMl and needtosendMblg equal FALSE lines are commented out.
 *
 *     Revision 1.3  2004/05/27 20:44:23  cjbryan
 *     changed MAGTYPE to AMPTYPE to match Ray's defs for amp msgs
 *
 *     Revision 1.2  2004/04/23 17:35:58  cjbryan
 *     changed bool to int
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Implementations of utility functions used in the raypicker to
 * send error and output messages.
 * 
 * see rp_messaging.h for comments
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */

/* system includes */
#include <stdio.h>
#include <string.h>
#include <float.h>  /* DBL_MAX */

/* earthworm/hydra includes */
#include <earthworm.h> 

#include <global_pick_rw.h>
#include <global_msg.h>              /* TimeToDTString */
#include <watchdog_client.h>

/* raypicker includes */
#include "rp_messaging.h"
#include "pick_params.h"
#include "returncodes.h"
#include "channel_states.h"



/************************************************************************
 *                      WriteHeartbeat                                  *
 *  Builds an heartbeat message and sends it to shared memory           *
 ************************************************************************/
int WriteHeartbeat(EWParameters ewp, RParams rparams, pid_t MyPid)
{
  MSG_LOGO      logo;
  char          msg[256];
  long          size;
  long          t;
  
  /* Build the message 
  *******************/ 
  logo.instid = ewp.MyInstId;
  logo.mod    = rparams.MyModId;
  logo.type   = ewp.TypeHeartBeat;
  
  time (&t);
  
  sprintf( msg, "%ld %d\n", t,MyPid);
  size = strlen( msg );   /* don't include the null byte in the message */   

                          /* Write the message to shared memory
  ************************************/
  if (SendMessageToRing(&logo, msg, size, &ewp.OutRing) != EW_SUCCESS)
    logit ("et","raypicker:  Error sending heartbeat.\n");
  
  return EW_SUCCESS;
}

/************************************************************************
 *                      WriteError                                  *
 *  Writes and error message to the EW RING                         *
 ************************************************************************/
int WriteError(EWParameters ewp, RParams rparams, pid_t MyPid, char * szError)
{
  MSG_LOGO      logo;
  char        msg[2048];
  long        t;
  
  /* Build the message 
  *******************/ 
  logo.instid = ewp.MyInstId;
  logo.mod    = rparams.MyModId;
  logo.type   = ewp.TypeError;
  
  time( &t );

#ifdef _WINNT
  _snprintf_s( msg, sizeof(msg), _TRUNCATE, "%ld %hd %s\n", t, (short)0, szError);
#else
  snprintf( msg, sizeof(msg), "%ld %hd %s\n", t, (short)0, szError);
#endif

  /* Write the message to shared memory
  ************************************/
  if (SendMessageToRing(&logo, msg, strlen(msg), &ewp.OutRing) != EW_SUCCESS)
    logit ("et","raypicker:  Error sending error message.\n");

  reportError(WD_WARNING_ERROR, 0, "Sent error message: <%s>\n", msg);
  
  return EW_SUCCESS;
}


/************************************************************************
 *                     SendMessageToRing()                              *
 *                                                                      *
 * Send a message to the output ring                                    *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *                                                                      *
 ************************************************************************/
static int SendMessageToRing(MSG_LOGO *logo , char *buffer, int length, 
                             SHM_INFO *OutRing)
{
    switch (tport_putmsg(OutRing, logo, length, buffer))
    {
      case PUT_OK:
          break;
      case PUT_NOTRACK:
          logit("e", "tport_putmsg() exceeded tracking, message not sent:\n%s\n", buffer);
          break;
      case PUT_TOOBIG:
          logit("e", "tport_putmsg() message too large for buffer:\n%s\n", buffer);
          break;
    }
    return EW_SUCCESS;
}

/************************************************************************
 *                         GetPickID()             .                    *
 *                                                                      *
 * Used to get the next pick id from the file RParams.PickIDFile        *
 *                                                                      *
 * @return the new pick id, or -1 for error                             *
 *                                                                      *
 ************************************************************************/
static long GetPickID(RParams params)
{
   /* long type allows up to INT_MAX or negative error codes */
   static long pickId = 0;

   FILE *file = NULL;
   
   if (pickId == 0)
   {
      /* Get first pick number from file */
      if ((file = fopen(params.PickIDFile, "r" )) == NULL)
         logit("e", "GetPickID(): Failed to open file %s to read id number\n", params.PickIDFile);
      else
      {
         fscanf(file, "%ld", &pickId);
         fclose(file );
      }
   }
   
   pickId++;
   
   if ((file = fopen(params.PickIDFile, "wc" )) == NULL)
   {
      logit("e", "GetPickID(): Failed to open file %s to write id number\n", params.PickIDFile);
      return -1;
   }
   fprintf (file, "%ld\n", pickId);
   fflush(file);
   fclose(file);

   return pickId;
}

/************************************************************************
 *                         ReportPick()                                 *
 *                                                                      *
 * Formats and sends a global pick message                              *
 * Ancillary action: resolves the arrival time from among all triggers. *
 * Gets the pick id file name from config.                              *
 *                                                                      *
 * WARNING: Does not check parameters for NULL.                         *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_WARNING = some formatting error                           *
 *                                                                      *
 ************************************************************************/
int ReportPick(RaypickerSCNL *scnl, RParams rparams, EWParameters ewp)
{
    PICK_CHANNEL_INFO        *channelinfo = scnl->pchanInfo;
    MSG_LOGO                  PICK_LOGO = {ewp.TypeGlobalPick, rparams.MyModId, 
											ewp.MyInstId};
    static GLOBAL_PICK_STRUCT PickStruct;
    static GLOBAL_PICK_BUFFER PickBuffer;
   

           double             arrival  = DBL_MAX;       /* pick arrival time */
           char               polarity = ' ';           /* pick polarity     */
           int                f;                        /* loop counter      */
           int                rc = EW_SUCCESS;          /* return code       */
   
    if (InitGlobalPick(&PickStruct) != GLOBAL_MSG_SUCCESS)
    {
      reportError(WD_WARNING_ERROR, NORESULT, 
                   "Failed to initialize global pick message buffer\n");
      rc = EW_WARNING;
    }
    else
    {
      /* Determine the arrival time for the pick based on
       * the earliest arrival time among the filter(s) which
       * are triggering. */
      for (f = 0; f < PICKER_TYPE_COUNT; f++)
      {
          if ((channelinfo->pick_series[f].state == PKCH_STATE_TRIG
                 || channelinfo->pick_series[f].state == PKCH_STATE_CONF) 
                 && channelinfo->pick_series[f].arrival < arrival)
          {
              arrival  = channelinfo->pick_series[f].arrival;
              polarity = channelinfo->pick_series[f].polarity;
          }
      }

      /* Fill in the Global Pick structure */
      PickStruct.logo = PICK_LOGO;
      strcpy(PickStruct.station, scnl->sta);
      strcpy(PickStruct.channel, scnl->chan);
      strcpy(PickStruct.network, scnl->net);
      strcpy(PickStruct.location, scnl->loc);
      strcpy(PickStruct.phase_name, "P");

	  TimeToDTString(arrival, PickStruct.pick_time);

      /*TimeToDTString(arrival, PickStruct.pick_time);*/
      PickStruct.quality = 0.0;
      PickStruct.polarity = polarity;

      if ((PickStruct.sequence = GetPickID(rparams)) == 0)
      {
          /* Invalid pick id returned from GetPickID() */
          reportError(WD_WARNING_ERROR, NORESULT, "invalid pickId returned from GetPickID(). \n");
          rc = EW_WARNING;
      }
      else
      {
          /* pick id okay */
          channelinfo->pickId = PickStruct.sequence;
          channelinfo->pickArrTime = arrival;
      
          if (WritePickToBuffer(&PickStruct, PickBuffer, GLOBAL_PICK_MAXBUFSIZE) != GLOBAL_MSG_SUCCESS)
          {
              reportError(WD_WARNING_ERROR, NORESULT, "Failed to format pick for transmission\n");
              rc = EW_WARNING;
          }
          else
          {
              reportError(WD_INFO, NORESULT, "Sending Pick: %s\n", PickBuffer);
              SendMessageToRing(&PICK_LOGO, PickBuffer, strlen(PickBuffer), &ewp.OutRing);
          }
      }
    }
   
    channelinfo->state = PKCH_STATE_PICKSENT;  
      
    return rc;
}
