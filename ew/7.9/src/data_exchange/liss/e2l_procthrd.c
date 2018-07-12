/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: e2l_procthrd.c 5701 2013-08-05 00:10:26Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:07:59  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:45:13  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * Routines for the ProcessThread, a part of ew2liss.
 */

#include <limits.h>    /* for SHRT_MAX */
#include <earthworm.h>
#include <dcc_std.h>
#include <dcc_time.h>
#include <dcc_misc.h>
#include "ew2liss.h"

int laplogged;   /* To control log entries for queue-lapped problems */

/*
 * ProcessThread: Takes data from input queue; fills peek buffers; compresses
 * data into seed structures; places seed structures into output queue.
 */
thr_ret ProcessThread(void *E2L )
{
  WORLD *pE2L = (WORLD *)E2L;
  char          *WaveBuf;       /* string to hold wave message             */
  TracePacket   *tp;            /* pointer to the actual TRACE_BUF message */  
  long           WaveBufLen;    /* length of WaveBuf                       */
  SCN_BUFFER *this;
  int jSta, ret;
  MSG_LOGO reclogo;
  
  /* Allocate the waveform buffer */
  WaveBufLen = (MAX_TRACEBUF_SIZ + sizeof (int));
  WaveBuf = (char *) malloc ((size_t) WaveBufLen);

  if (WaveBuf == NULL)
  {
    logit ("e", "ew2liss: Cannot allocate waveform buffer\n");
    pE2L->ProcessStatus = -1;
    KillSelfThread();
  }
  
  while( ! pE2L->terminate )
  {
    /* Tell the main thread we're feeling ok */
    pE2L->ProcessStatus = 0;
  
    /* Grap the next SCN from the input queue */
    RequestSpecificMutex(&pE2L->tbQMutex);
    ret = dequeue (&pE2L->tbQ, WaveBuf, &WaveBufLen, &reclogo);
    ReleaseSpecificMutex(&pE2L->tbQMutex);
    
    if (ret < 0)
    {                                 /* empty queue */
      sleep_ew (500);
      continue;
    }
    
    /* Extract the SCN number; recall, it was pasted as an int on the front 
     * of the message by the main thread */
    jSta = *((int*)WaveBuf);
    tp = (TracePacket *)(WaveBuf + sizeof(int));
    this = &(pE2L->pscnB[jSta]);
    
    if ( (this->bestSR = GetBestRate(&tp->trh) ) <= 0.0)
    {
      logit("et", "ew2liss: unable to handle samplerate <%lf> from <%s.%s.%s>\n",
            tp->trh.samprate, tp->trh.sta, tp->trh.chan, tp->trh.net);
      continue;
    }
    
    /* Check for gap between new data in TRACE_BUF and old data in peek buffer */
    if (this->pb_endtime > 0.0) 
    {   
      if (this->pb_endtime + GAP_THRESH / this->bestSR < tp->trh.starttime)
      {           /* Found a gap! Flush the old data */
        ret = FillSeed(this, 1, pE2L);
        if (SendSeed(this, pE2L) == EW_FAILURE)
        {
          logit("et", "ew2liss ProcessThread: fatal error sending packet\n");
          pE2L->ProcessStatus = -1;
          KillSelfThread();
        }
        CleanSeed( this );
      }
    }
    
    if ( FillPeekBuffer(this, (TracePacket *) (WaveBuf + sizeof(int)))
         != EW_SUCCESS)
    {
      logit("et", "ew2liss: error from FillPeekBuffer; exitting\n");
      pE2L->ProcessStatus = -1;
      KillSelfThread();
    }
     
    /* compress as much data as possible into a data frame */
    while ( (ret = FillSeed(this, 0, pE2L)) > 0)
    {     /* SEED buffer is filled and ready for sending */
      if ( SendSeed(this, pE2L) == EW_FAILURE)
      {
        logit("et", "ew2liss ProcessThread: fatal error sending packet\n");
        pE2L->ProcessStatus = -1;
        KillSelfThread();
      }
      CleanSeed( this );
    }
  }  /* end of (while (!terminate...) */
  logit("t", "procthrd: termination requested\n");
  /* Any cleanup to do?, No, so just return... */
  return NULL;
}
  
/*
 * FillPeekBuffer: Transfer trace data from the trace buffer to the 
 *   compressor's peek (input) buffer. Assumes that checks have already been
 *   made for time gap between old and new data.
 * Returns: EW_SUCCESS if all went well.
 *          EW_FAILURE on errors such as insufficient room for trace data.
 */
int FillPeekBuffer(SCN_BUFFER *this, TracePacket *tp)
{
  short *shortP;
  long  *longP;
  int i;
  
  /* Check for adequate space for new data */
  if (this->gdp->adp->ccp->peek_total + tp->trh.nsamp > PEEKELEMS)
  {
    logit("et", "ew2liss: peek buffer overflow for <%s.%s.%s> pb: %d tp: %d\n",
          this->sta, this->chan, this->net, this->gdp->adp->ccp->peek_total,
          tp->trh.nsamp);
    return EW_FAILURE;
  }
  
  if (tp->trh.datatype[1] == '2')
  {
    shortP = (short *)(tp->msg + sizeof(TRACE_HEADER));
    for (i = 0; i < tp->trh.nsamp; i++)
    {
      if (peek_write(this->gdp->adp->ccp, (long *)shortP, 1) != 1)
      {
        logit("et", "ew2liss: peek buffer full for <%s.%s.%s>, pb %d tp %d\n",
              this->sta, this->chan, this->net, 
              this->gdp->adp->ccp->peek_total, tp->trh.nsamp);
        return EW_FAILURE;
      }
      shortP++;
    }
  }
  /* The long data type, not the float data type */
  else if (tp->trh.datatype[1] == '4' && (tp->trh.datatype[0] == 's' || 
                                        tp->trh.datatype[0] == 'i'))
  {
    longP = (long *)(tp->msg + sizeof(TRACE_HEADER));
    if (peek_write(this->gdp->adp->ccp, longP, (short) tp->trh.nsamp) !=
        (short)tp->trh.nsamp)
    {
      logit("et", "ew2liss: peek buffer full for <%s.%s.%s>, pb %d tp %d\n",
            this->sta, this->chan, this->net, 
            this->gdp->adp->ccp->peek_total, tp->trh.nsamp);
      return EW_FAILURE;
    }
  }
  else
  {
    logit("et", "ew2liss: unknown datatype <%s> for <%s.%s.%s>, pb %d tp %d\n",
          tp->trh.datatype, this->sta, this->chan, this->net, 
              this->gdp->adp->ccp->peek_total, tp->trh.nsamp);
    return EW_FAILURE;
  }
  
  /* Update endtime for the peek buffer */
  this->pb_endtime = tp->trh.endtime;
  return EW_SUCCESS;
}


/*
 * FillSeed: fills a miniSEED structure with trace data from peek buffer.
 *  When miniSEED data frames are filled, fills in all the header info.
 *  If `flush' is zero, compression will not be done unless the peek
 *   buffer holds enough data.
 *  If `flush' is 1, then all data will be flushed out of the peek buffer
 *   and the data record will be padded with zeros as needed to fill it.
 *  Checks to make sure the peek buffer doesn't get empty. If it does, then
 *   the compressor ran out of data and inserted padding, so the rest of the
 *   SEED record needs to be flushed before we put in more data. We indicate
 *   this end of continuity by setting the SCN_BUFFER endtime back to zero.
 * Returns: 1 if miniSEED structure is filled and read to ship
 *          0 if more data is needed.
 */
int FillSeed( SCN_BUFFER *this, int flush, WORLD *pE2L )
{
  int frames;
  ccptype ccp = this->gdp->adp->ccp;   /* Compression continuity buffer */
  
  /* Is there enough in the peek buffer to compress, or should we flush it */
  if ( flush == 0 && peek_threshold_avail(ccp) < 0)
    return 0;

  /* If the miniSEED buffer is empty, we need to set its start time */
  if ( ccp->frames == 0 )
    setSeedStartTime( this );
  
  /* Compress as many frames as we can */
  while (peek_threshold_avail(ccp) > 0 || flush == 1)
  {
    /* Compress a single frame of data */
    frames = compress_generic_record(this->gdp, FIRSTFRAME);
    
    if (frames == MAXFRAMES)
    {          /* The miniSEED data buffer is full; prepare for shipment */
      install_SEED_header(this, pE2L);
      return (1);
    }
    /* Did the peek_buffer go empty? If so, we must flush this SEED record */
    if (ccp->peek_total < 1)
    {
      flush = 1;
      this->pb_endtime = 0.0;
    }
  }  /* end of while() loop */

  return (0);
}


/* Set the start time of the SEED record */
void setSeedStartTime( SCN_BUFFER *this )
{
  double pb_starttime;
  STDTIME STDstart;

  /*
   * Figure out the peek buffer start time. It may seem weird to use
   * the pb_endtime to do this, but it seems easier this way. It's quite
   * difficult to keep updating the pb_starttime each time the peek buffer
   * start point moves. Easier is to keep track of the pb_endtime, which we
   * need anyway, to keep track of gaps in front of the next TRACE_BUF packet.
   * Either way, we do a lot of interpolating times from sample numbers and 
   * sample rates.
   */
  pb_starttime = this->pb_endtime - (this->gdp->adp->ccp->peek_total - 1)
    / this->bestSR;
  
  /* Go from Earthworm epoch time as a double to DCC standard form */
  STDstart = ST_CnvUnixDbltoSTD( pb_starttime );

  /* Fill in miniSEED time, swapping bytes where needed */
  this->sdp->Head.starting_time.yr = LocGM68_WORD(STDstart.year);
  this->sdp->Head.starting_time.jday = LocGM68_WORD(STDstart.day);
  this->sdp->Head.starting_time.hr = STDstart.hour;
  this->sdp->Head.starting_time.minute = STDstart.minute;
  this->sdp->Head.starting_time.seconds = STDstart.second;
  this->sdp->Head.starting_time.tenth_millisec = 
    LocGM68_WORD(STDstart.msec * 10);

  return;
}

/*
 * Fill in the miniSEED fixed data and data blockettes with appropriate
 * EW2LISS values; this is not for general use!
 */
void install_SEED_header( SCN_BUFFER *this, WORLD *pE2L )
{
  FILE *sfp;
  char seqText[8];
  int i;
  
  /* Assume the fixed data header has already been cleared;
   * Also, the starting_time is already filled in */

  /* Fill in the sequence number: 6 digits. */
  sfp = fopen( pE2L->seqFile, "w" );
  if ( sfp != (FILE *) NULL )
  {
    fprintf( sfp,
	     "# Next available LISS sequence number:\n" );
    fprintf( sfp, "%s %ld\n", SEQ_COMMAND, pE2L->seqNo );
    fclose( sfp );
  }
  sprintf(seqText, "%06ld", pE2L->seqNo);
  for (i = 0; i < 6; i++)
    this->sdp->Head.sequence[i] = seqText[i];
  pE2L->seqNo = (pE2L->seqNo + 1) % MAXSEQ;
  
  this->sdp->Head.seed_record_type = 'D';
  
  copy_chars(this->sta, this->sdp->Head.station_ID_call_letters, 5);
  copy_chars(this->chan, this->sdp->Head.channel_ID, 3);
  copy_chars(this->net, this->sdp->Head.seednet, 2);
  copy_chars(this->locID, this->sdp->Head.location_id, 2);
  
  this->sdp->Head.samples_in_record = 
    LocGM68_WORD((short)this->gdp->nsamples);
  
  setSampleNumbers(this);

  /* Nothing to put in activity or IO flags */
  this->sdp->Head.data_quality_flags = this->quality[0];
  
  this->sdp->Head.number_of_following_blockettes = 2;
  this->sdp->Head.tenth_msec_correction = LocGM68_LONG((long)0);
  this->sdp->Head.first_data_byte = LocGM68_WORD((short)((FIRSTFRAME+1) * 64));
  this->sdp->Head.first_blockette_byte = LocGM68_WORD((short)48);

  /* The data-only blockette */
  this->sdp->Head.DOB.blockette_type = LocGM68_WORD((short)1000);
  this->sdp->Head.DOB.next_offset = LocGM68_WORD((short)56);
  this->sdp->Head.DOB.encoding_format = 11; /* Steim2 */
  this->sdp->Head.DOB.word_order = 1;
  this->sdp->Head.DOB.rec_length = 9;

  /* The data extension blockette */
  this->sdp->Head.DEB.blockette_type = LocGM68_WORD((short)1001);
  this->sdp->Head.DEB.next_offset = LocGM68_WORD((short)0);
  this->sdp->Head.DEB.qual = 5; /* We're good; what else can we say? */
  this->sdp->Head.DEB.usec99 = 0;
  this->sdp->Head.DEB.state_of_health = 0;  
  this->sdp->Head.DEB.frame_count = 7;
  
  return;
}


void copy_chars( char *ins, char *outs, int n)
{
  char *wrt,chr;
  int i;

  if (ins==NULL) ins="";

  if (outs!=NULL) 
  {
    wrt = outs;
    for (i=0; i<n; i++) outs[i] = ' ';
    for (i=0; i<n; i++) 
    {
      chr = ins[i];
      if (chr=='\0') break;
      *wrt++ = chr;
    }
  }
  return;
}

/*
 * Set the SEED sample rate factor and multiplier from samplerate as double.
 * SEED rules: sm and sr are both short integers.
 *   if sr < 0, it represents seconds per sample
 *   if sr > 0, it represents samples per second
 *   if sm < 0, it is sample rate division factor.
 *   if sm > 0, it is sample rate multiplication factor.
 * Here, we use sm == 0 to indicate we need to compute sm and sr this once.
 */
void setSampleNumbers( SCN_BUFFER *this)
{
  short mult = 1;
  double rate = this->bestSR;
  
  if (rate >= SHRT_MAX)
  {
    while(rate >= SHRT_MAX)
    {
      rate /= 2;
      mult *= 2;
    }
  }
  else if (rate < SHRT_MAX/2 && rate > 1.0)
  {
    while (rate < SHRT_MAX/2)
    {
      rate *= 2;
      mult *= 2;
    }
    mult *= -1;
  }
  else if (rate < 1.0 )
  {
    rate = 1.0/rate;
    if (rate > SHRT_MAX)
    {
      while(rate > SHRT_MAX)
      {
        rate /= 2;
        mult *= 2;
      }
      mult *= -1;
    }
    else if (rate < SHRT_MAX/2 && rate > 1.0)
    {
      while (rate < SHRT_MAX/2)
      {
        rate *= 2;
        mult *= 2;
     }
    }
    rate *= -1;
  }
 
  this->sdp->Head.sample_rate_factor = LocGM68_WORD((short)rate);
  this->sdp->Head.sample_rate_multiplier = LocGM68_WORD(mult);
  return;
}

/*  
 * Determine best samplerate from TRACE_BUF.
 * Assume starttime, endtime and nsamp are more accurate than
 * the advertised samplerate, which is probably the nominal rate.
 * But if there is only one sample, then we can only use the advertised rate.
 * Returns: best sample rate, or -1.0 if none found.
 */
double GetBestRate( TRACE_HEADER *tb )
{
  double rate;
  
  if (tb->nsamp > 1)
  {
    rate = (tb->nsamp - 1) / (tb->endtime - tb->starttime);
    rate = ((double)( 10000 * (unsigned long)(rate + 0.00005))) / 10000.0;
    return rate;
  }
  else if (tb->samprate > 0.0)
    return tb->samprate;
  else
    return -1.0;
}

/*
 * SendSeed: dispatch the SEED record out of ProcessThread. In this case,
 *   we plant the SEED into the output queue. Another possiblity is to 
 *   plot the SEED into an internal transport ring, if there were several
 *   server threads listening. Or we could write to a file of miniSEED
 *   records...
 * Returns: EW_SUCCESS if all went well enough.
 *          EW_FAILURE if we couldn't deal with the problem. We expect that
 *   this will cause thread termination.
 */
int SendSeed(SCN_BUFFER *this, WORLD *pE2L)
{
  int ret;
  char msgText[MAXMESSAGELEN];    /* string for log/error messages */
  
  /* Put the SEED message into the output queue. We have to add a silly logo,
     so we'll use the wave logo even though this isn't right. Who cares! */
  RequestSpecificMutex(&pE2L->seedQMutex);
  ret = enqueue (&(pE2L->seedQ), (char *)this->sdp, LISS_SEED_LEN, 
                 pE2L->waveLogo);
  ReleaseSpecificMutex(&pE2L->seedQMutex);
  
  if (ret == 0)
  {
    if (laplogged > 0)
    {
      logit("et", "ew2liss: output queue no longer full\n");
      laplogged = 0;
    }
    return EW_SUCCESS;
  }
  
  /* Message too big; should only be a programming error */
  if (ret == -1)
    logit("et", "ew2liss: Message too large for output queue; Lost message.\n");
  else if (ret == -3) /* Queue lapped: maybe bad, maybe not */
  {
    if (pE2L->ServerStatus == SS_CONN)
    {    /* Client connected: must be something wrong here */
      if (laplogged == 0)
      {
        sprintf(msgText, "output queue lapped while client connected\n");
        StatusReport (pE2L, pE2L->Ewh.typeError, ERR_OUTQUEUE, msgText);
      }
      else if (laplogged == 20)
      {
        logit("et", "ew2liss: 20 more messages lapped in output queue while client connected\n");
        laplogged = 1;
      }
      laplogged++;
    }
    else if (pE2L->ServerStatus == SS_DISCO)
    {      /* Client not connected: queue is expected to lap */
      if (laplogged  == 0)
      {
        logit("t", "output queue lapped; normal condition with no client\n");
        laplogged = 1;
      }
    }
  }
  
  return EW_SUCCESS;
}

/* CleanSeed: reset the compression structures and clean out the SEED header */
void CleanSeed( SCN_BUFFER *this)
{
  DCC_LONG *fill;
  int i;
  
  clear_generic_compression(this->gdp, FIRSTFRAME);

  fill = (DCC_LONG *)this->sdp ;
  for (i = 0; i < (FIRSTFRAME + 1)*16; i++)
    *(fill++) = 0 ;

  return;
}
