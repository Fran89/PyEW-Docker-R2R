/* @(#)send.c	1.3 07/01/98 */
/*======================================================================
 *
 * Send packets to their respective rings
 *
 * Revised  :
 *	12Jul06	 (rs) get rate from header packet if we see one 
 *					   incorporate idea of nominal rate into get_samplerate
 *                but keep the restriction of not sending on tracebuffers
 *                that do not match the default rate list or what is defined
 *                in the config file
 *====================================================================*/
#include "import_rtp.h"

#define MY_MOD_ID IMPORT_RTP_SEND

static struct {
    MSG_LOGO wav;
    MSG_LOGO raw;
    MSG_LOGO error;
} Logo;

BOOL init_senders(struct param *par)
{
CHAR *str;

    Logo.wav.instid = par->InstId;
    Logo.wav.mod    = par->Mod;
    if (GetType(str = "TYPE_TRACEBUF2", &Logo.wav.type) != 0) {
        logit("t", "FATAL ERROR: invalid message type for wave ring <%s>\n", str);
        return FALSE;
    }

    Logo.raw.instid = par->InstId;
    Logo.raw.mod    = par->Mod;
    if (GetType(str = "TYPE_REFTEK", &Logo.raw.type) != 0) {
        logit("t", "FATAL ERROR: invalid message type for raw ring<%s>\n", str);
        return FALSE;
    }

    Logo.error.instid = par->InstId;
    Logo.error.mod    = par->Mod;
    if (GetType(str = "TYPE_ERROR", &Logo.error.type) != 0) {
        logit("t", "FATAL ERROR: invalid message type <%s>\n", str);
        return FALSE;
    }

    return TRUE;
}


static char szErrText[256];

#define TRACEBUF_DATA_BYTE_SIZE 4

VOID send_wav(SHM_INFO *region, UINT8 *pkt)
{
int  status;
#ifdef BIG_ENDIAN_HOST
static char *datatype = "s4";
#else
static char *datatype = "i4";
#endif
int length;
REAL64 samprate;
INT32 dbytes;
CHAR *doff;
static CHAR buf[256];
static struct reftek_dt dt;
static struct reftek_eh eh;
static TracePacket trace;
int idt;
int rc;
int max_tbuf_samps_4_byte;

/* Only deal with DT packets for which we can get sample rate */

    if(reftek_type(pkt) == REFTEK_EH) /* set the sample rate for this stream */
    {											/* from the header packet */
      if(reftek_eh(&eh,pkt))	
      {										
        set_samprate_from_eh(&eh,pkt);/* get the rate we think it should be */
      }
    }
    	
    if(reftek_type(pkt) != REFTEK_DT) return;

    if(!reftek_dt(&dt, pkt, TRUE)) 
    {
      logit("t", "WARNING: can't decode DT packet\n");
      return;
    }

/* Find SCNLP data given DAS serial#/stream/chan */
    rc = get_dt_index(&dt, &idt);
    if(rc != 0)
    {
      if(rc == -1)
      {
        sprintf(szErrText, "Error resolving channel (%04X S%d C%d)\n",
                dt.unit, dt.stream, dt.chan);
        reftek2ew_send_error(EW_STATUS_ERROR_CHANNEL_LOOKUP, szErrText);
      }
      else if(rc == -2)
      {
        sprintf(szErrText, "Error: invalid internal pointer while processing "
                           "channel (%04X S%d C%d)\n",
                dt.unit, dt.stream, dt.chan);
        reftek2ew_send_error(EW_STATUS_ERROR_INTERNAL_POINTER, szErrText);
      }
      else if(rc != 1)
      {
        sprintf(szErrText, "Error: unexpected error(%d) while processing "
                           "channel (%04X S%d C%d)\n", rc,
                dt.unit, dt.stream, dt.chan);
        reftek2ew_send_error(EW_STATUS_ERROR_UNKNOWN, szErrText);
      }
      /* else rc == 1, unsupported channel, ignore packet */
      return;
    }

/* Decide what sampel rate we will be using in trace buffer */

    if( (samprate = GetNomSampleRate(idt)) < 0 ) /* Always use rate in scn file if it is set*/
    {											/* set sample rate if not already set or is a big change */
      if(!get_samprate(&dt, &samprate)) return;												
    }

/* Note decompression errors, if any */

    if( dt.dcerr ) 
    {
      logit("t", "DCMP ERR %d: %s.  ", dt.dcerr, reftek_str(pkt, buf));
      if(par.DropPacketsWithDecompressionErrors)
      {
        logit("", "Packet Dropped!\n");
        return;
      }
      else
      {
        logit("", "\n");
      }
    }
       
/* Convert to TracePacket format */

    load_scnlp(&trace, idt);
    trace.trh2.starttime  = dt.tstamp;
    trace.trh2.samprate   = samprate;
	
 /* In case of error, set samprate to 1.0 so it doesn't break anything.
    The packet will be rejected by EWIsValidPacket() when it calls SampleRateIsValid() */

    if(trace.trh2.samprate < 0.0) trace.trh2.samprate = 1.0;
    trace.trh2.endtime    = dt.tstamp + (((REAL64)dt.nsamp - 1.0) / trace.trh2.samprate);
    trace.trh2.nsamp      = dt.nsamp;
    trace.trh2.version[0] = TRACE2_VERSION0;
    trace.trh2.version[1] = TRACE2_VERSION1;
    strcpy(trace.trh2.datatype, datatype);
    doff   = trace.msg + sizeof(TRACE2_HEADER);
    dbytes = trace.trh2.nsamp * TRACEBUF_DATA_BYTE_SIZE;

/* Validate the message */
/* The tracebuf (trace) and the reftek calc'd sample rate(samprate) are
   validated, based on stored channel information for channel(idt).
   dt is used only for log info and is not validated. */

   if(!EWIsValidPacket(&trace, &dt, idt, samprate)) return;


/* the above does not check for 4096 size of tracebuf2  and a DT packet could have more than 1008 samples */

    max_tbuf_samps_4_byte = (MAX_TRACEBUF_SIZ - sizeof(TRACE2_HEADER))/TRACEBUF_DATA_BYTE_SIZE;
    if (trace.trh2.nsamp <= max_tbuf_samps_4_byte) {
        /* Send the single message */
        length = sizeof(TRACE2_HEADER) + dbytes;
        memcpy((void *) doff, (dt.data), (size_t) dbytes);
        RequestMutex();
        status = tport_putmsg(region, &Logo.wav, length, trace.msg);
        ReleaseMutex_ew();
        if (status != PUT_OK) {
            logit("et", "TYPE_TRACEBUF2 tport_putmsg error (ignored)\n");
        }
    } else {
        /* we need to split the message in two */
        if (par.debug > 1) {
           logit("t", "TYPE_TRACEBUF2 message too big, %d bytes, only %d allowed, splitting\n", 
                 sizeof(TRACE2_HEADER) + dbytes, MAX_TRACEBUF_SIZ);
        }
        trace.trh2.nsamp = max_tbuf_samps_4_byte;
        trace.trh2.endtime    = dt.tstamp + ((max_tbuf_samps_4_byte - 1) / trace.trh2.samprate);
        length = sizeof(TRACE2_HEADER) + TRACEBUF_DATA_BYTE_SIZE*max_tbuf_samps_4_byte;
		memcpy((void *) doff, (dt.data), (size_t) TRACEBUF_DATA_BYTE_SIZE*max_tbuf_samps_4_byte);
        RequestMutex();
        status = tport_putmsg(region, &Logo.wav, length, trace.msg);
        ReleaseMutex_ew();
        if (status != PUT_OK) {
            logit("et", "TYPE_TRACEBUF2 tport_putmsg error (ignored) %d bytes in 1st message\n",  length);
        }

        /* now build 2nd packet NB: this does not check if a 3rd packet is possible....should not be! */
        trace.trh2.nsamp = dt.nsamp - max_tbuf_samps_4_byte;
        trace.trh2.starttime  = trace.trh2.endtime + 1.0/trace.trh2.samprate;
        trace.trh2.endtime    = trace.trh2.starttime + ((trace.trh2.nsamp - 1) / trace.trh2.samprate);
        memcpy((void *) doff, &(dt.data[max_tbuf_samps_4_byte]), (size_t) TRACEBUF_DATA_BYTE_SIZE*trace.trh2.nsamp);
        length = sizeof(TRACE2_HEADER) + TRACEBUF_DATA_BYTE_SIZE*trace.trh2.nsamp;
        RequestMutex();
        status = tport_putmsg(region, &Logo.wav, length, trace.msg);
        ReleaseMutex_ew();
        if (status != PUT_OK) {
            logit("et", "TYPE_TRACEBUF2 tport_putmsg error (ignored) %d bytes in 1st message\n", length);
        }
    }
}

VOID send_rtp(SHM_INFO *region, UINT8 *pkt)
{
    static int length = RTP_DASPAKLEN;
    int  status;

    RequestMutex();
    status = tport_putmsg(region, &Logo.raw, length, (char *) pkt);
    ReleaseMutex_ew();

    if (status != PUT_OK) {
        logit("et", "TYPE_REFTEK tport_putmsg error (ignored)\n");
    }
}



/***************************************************************************
* reftek2ew_send_error() builds a error message & puts it into             *
*                 shared memory.  Writes errors to log file & screen.      *
***************************************************************************/
void reftek2ew_send_error(short ierr, char *note )
{
  char	   msg[256];
  long	   size;
  time_t   t;
  
  time( &t );
  sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
  logit( "et", "%s(%d): %s\n", "reftek2ew", Logo.error.mod, note );
  
  size = strlen( msg );   /* don't include the null byte in the message */ 	
  
  /* Write the message to shared memory ********/
  RequestMutex();
  if( tport_putmsg( &par.WavRing.shm, &Logo.error, size, msg ) != PUT_OK )
  {
    logit("et", "%s(%d):  Error sending error:%d.\n", 
          "reftek2ew", Logo.error.mod, ierr );
  }
  ReleaseMutex_ew();
  
  return;
}  /* end reftek2ew_send_error() */


/* validate the message */
BOOL EWIsValidPacket(TracePacket * pTrace, struct reftek_dt * pDT, 
                     INT32 iDT, double dCalcdSamprate)
{
  time_t tNow;
  int		idx;

  /* check starttime vs. endtime */
  if(pTrace->trh2.starttime > pTrace->trh2.endtime)
  {
    sprintf(szErrText,
              "Error: Invalid starttime/endtime in tracebuf: "
              "(%s %s %s %s) %.3f - %.3f, %3d, %5.1f, lp %.3f (%04X S%d C%d)\n",
              pTrace->trh2.sta, pTrace->trh2.chan, 
              pTrace->trh2.net, pTrace->trh2.loc,
              pTrace->trh2.starttime, pTrace->trh2.endtime,
              pTrace->trh2.nsamp, pTrace->trh2.samprate,
              GetLastSampleTime(iDT),
              pDT->unit, pDT->stream, pDT->chan
           );
    reftek2ew_send_error(EW_STATUS_ERROR_INVALID_TRACEBUF, szErrText);
    return(FALSE);
  }
  
  /* check number of samples */
  if(pTrace->trh2.nsamp <= 0)
  {
    sprintf(szErrText,
              "Error: Invalid number of samples in tracebuf: "
              "(%s %s %s %s) %.3f - %.3f, %3d, %5.1f, lp %.3f (%04X S%d C%d)\n",
              pTrace->trh2.sta, pTrace->trh2.chan, 
              pTrace->trh2.net, pTrace->trh2.loc,
              pTrace->trh2.starttime, pTrace->trh2.endtime,
              pTrace->trh2.nsamp, pTrace->trh2.samprate,
              GetLastSampleTime(iDT),
              pDT->unit, pDT->stream, pDT->chan
           );
    reftek2ew_send_error(EW_STATUS_ERROR_INVALID_TRACEBUF, szErrText);
    return(FALSE);
  }

  /* check starttime vs. endtime of last packet */
   if(par.DropPacketsOutOfOrder)
   {
     if(pTrace->trh2.starttime <= GetLastSampleTime(iDT))
     {
       sprintf(szErrText,
                "Error: Starttime of tracebuf earlier than or equal to last timestamp: "
                "(%s %s %s %s) %.3f - %.3f, %3d, %5.1f, last=%.3f (%04X S%d C%d)\n",
                pTrace->trh2.sta, pTrace->trh2.chan, 
                pTrace->trh2.net, pTrace->trh2.loc,
                pTrace->trh2.starttime, pTrace->trh2.endtime,
                pTrace->trh2.nsamp, pTrace->trh2.samprate,
                GetLastSampleTime(iDT),
                pDT->unit, pDT->stream, pDT->chan
              );
       reftek2ew_send_error(EW_STATUS_ERROR_INVALID_TRACEBUF, szErrText);
       return(FALSE);
     }
   }
  
  /* check starttime vs. system clock.  Prevent "jump ahead" packets */
  time(&tNow);
  if(pTrace->trh2.starttime > (tNow + par.TimeJumpTolerance))
  {
    sprintf(szErrText,
              "Error: Starttime of tracebuf appears invalid(in the future)."
              "(%s %s %s %s) %.3f - %.3f, %3d, %5.1f, lp %.3f (%04X S%d C%d), tSys %d\n",
              pTrace->trh2.sta, pTrace->trh2.chan, 
              pTrace->trh2.net, pTrace->trh2.loc,
              pTrace->trh2.starttime, pTrace->trh2.endtime,
              pTrace->trh2.nsamp, pTrace->trh2.samprate,
              GetLastSampleTime(iDT),
              pDT->unit, pDT->stream, pDT->chan, (int) tNow
           );
    reftek2ew_send_error(EW_STATUS_ERROR_INVALID_TRACEBUF, szErrText);
    return(FALSE);
  }
  
  if(par.FilterOnSampleRate)
  {
    if (dCalcdSamprate == 0.0)
    {
      logit("t","Dropping packet (%s %s %s %s) %.3f - %.3f, %3d, (%04X S%d C%d), "
                "because samprate is not yet validated for channel. "
                "Current SampRate (%.2f)\n",
            pTrace->trh2.sta, pTrace->trh2.chan, 
            pTrace->trh2.net, pTrace->trh2.loc,
            pTrace->trh2.starttime, pTrace->trh2.endtime,
            pTrace->trh2.nsamp, 
            pDT->unit, pDT->stream, pDT->chan, 
            dCalcdSamprate
           );
      return(FALSE);
    }
    if (!SampleRateIsAcceptable(dCalcdSamprate,&idx))
    {
      sprintf(szErrText,
              "Error: SampleRate of tracebuf appears invalid. "
              "Should be (%6.2f), is (%6.2f)! "
              "(%s %s %s %s) %.3f - %.3f, %3d, %5.1f, lp %.3f (%04X S%d C%d)\n",
              GetNomSampleRate(iDT), dCalcdSamprate, 
              pTrace->trh2.sta, pTrace->trh2.chan, 
              pTrace->trh2.net, pTrace->trh2.loc,
              pTrace->trh2.starttime, pTrace->trh2.endtime,
              pTrace->trh2.nsamp, pTrace->trh2.samprate,
              GetLastSampleTime(iDT),
              pDT->unit, pDT->stream, pDT->chan
             );
      reftek2ew_send_error(EW_STATUS_ERROR_INVALID_TRACEBUF, szErrText);
      if(par.SendTimeTearPackets==TRUE)
      {
        logit("","Packet sent anyway, because SendTimeTearPackets is set.\n");
      }
      else
      {
        logit("","Packet dropped\n");
        return(FALSE);
      }
    }
  }  /* end if FilterOnSampleRate */

  /* record the last sample time */
  SetLastSampleTime(iDT, pTrace->trh2.endtime);
  return(TRUE);
}  /* end EWIsValidPacket() */

