/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_uw.c 2094 2006-03-10 13:03:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.5  2002/08/12 16:55:00  lombard
 *     Corrected call to setTaperTime
 *
 *     Revision 1.4  2002/03/17 18:27:11  lombard
 *     Modified some function calls to support new argument in
 *        traceEndTime and setTaperTime, which now need the EVENT structure.
 *
 *     Revision 1.3  2002/01/24 19:34:09  lombard
 *     Added 5 percent cosine taper in time domain to both ends
 *     of trace data. This is to eliminate `wrap-around' spikes from
 *     the pre-event noise-check window.
 *
 *     Revision 1.2  2002/01/15 21:23:03  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/01/15 03:55:55  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/12/31 17:27:25  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_uw.c: routines for reading and writing UW-format files for localmag.
 *    This file is not expected to compile on NT!!!
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <chron3.h>
#include <time_ew.h>
#include "lm.h"
#include "lm_config.h"
#include "lm_misc.h"
#include "lm_util.h"
#include "lm_uw.h"
#include "Uwdfif/uwdfif.h"
#include "Uwpfif/uwpfif.h"
#include "Stafileif/stafileif.h"

static double gPickRefSec;
static float *seis = (float *)NULL;   /* Array for holding trace data */
static char gDatafile[LM_MAXTXT];
static char gWAdatafile[LM_MAXTXT];
static char gWApickfile[LM_MAXTXT];
static int gfUWSaveInit = 0;

/* Internal Function Prototypes */
static double uwTimeToSec(struct Time *);
static void secToUWTime(double, struct Time *);
static void toWAname(char *, char *);
static void cleanPick(struct Pick *);

/*
 * getUWpick: open and read a UW-format pickfile; copy event information
 *      into EVENT structure.
 *  Returns: 0 on success
 *          -1 on error: can't read pickfile; no origin
 */
int getUWpick(EVENT *pEvt, LMPARAMS *plmParams)
{
  struct Time pickRefTime;
  float origsec, lat, lon, depth;
  char fixd[2];
  
  /* Set up the station location tables */
  if (plmParams->staLoc == LM_SL_UW)
    (void)STAinit_default();  /* returns TRUE or exits */
  

  if (UWPFrd1evt(plmParams->UWpickfile) == FALSE)
  {
    logit("", "getUWpick: error reading pickfile <%s>: %s\n", 
          plmParams->UWpickfile, strerror(errno));
    return -1;
  }
  (void)UWPFrefTime('R', &pickRefTime);  /* always returns TRUE */
  gPickRefSec = uwTimeToSec( &pickRefTime );
  if (UWPForigsec('R', &origsec) == FALSE)
  {
    logit("", "getUWpick: no origin set in <%s>\n", plmParams->UWpickfile);
    return -1;
  }
  pEvt->origin_time = (double)origsec + gPickRefSec;
  if (UWPFevtLatLon('R', &lat, &lon) == FALSE)
  {
    logit("", "getUWpick: no event location for <%s>\n", 
          plmParams->UWpickfile);
    return -1;
  }
  pEvt->lat = (double)lat;
  pEvt->lon = (double)lon;
  if (UWPFdepth('R', &depth, fixd) == FALSE)
    depth = 0.0;
  pEvt->depth = (double)depth;
  
  if (plmParams->debug)
  {
    char date[22];
    date20(pEvt->origin_time + GSEC1970, date);
    logit("", "UW event <%s> origin: %s\n", plmParams->UWpickfile, date);
    logit("", "\tlat: %10.6lf lon: %10.6lf depth: %6.2lf\n", pEvt->lat,
          pEvt->lon, pEvt->depth);
  }
  if (plmParams->traceSource == LM_TS_UW)
  {   /* Open the data file here, so initUWsave() can copy the header */
    /* We assume here that if the datafile we are to open contains W-A *
     * traces, then we have been given a pickfile name that includes   *
     * the ".wa." fragment in its name. The user will find out soon.   */
    strcpy(gDatafile, plmParams->UWpickfile);
    LAST_CHAR(gDatafile) = 'W';

    if (UWDFopen_file(gDatafile) == FALSE)
    {
      logit("", "getUWpick: error opening datafile <%s>: %s\n", gDatafile,
            strerror(errno));
      return -1;
    }
  }
  
  return 0;
}

/*
 * getAmpDirectFromUW: get amplitude data from UW-format pickfile.
 *                     Looks through a UW-format pickfile for WA amplitude
 *                     picks.
 *   Returns: 0 on success
 *           -1 on fatal errors
 */
int getAmpDirectFromUW(EVENT *pEvt, LMPARAMS *plmParams)
{
  struct Pick uwPick;
  STA *pSta;
  COMP3 *pComp;
  int npicks, i, j, rc;
  
  /* Find out how many picks there are */
  (void)UWPFpicks('I', &npicks, &uwPick);  /* Always returns TRUE */

  /* Loop through all the picks but only stop for Z2P picks.  *
   * We expect there will always be one of these for a picked *
   * WA trace. There might also be P2P plus and minus picks   *
   * for this SCNL; we will ask about that and include them in *
   * the COMP3 structure if we find them.                     */
  for (i = 0; i < npicks; i++)
  {
    /* Grap each pick from the UWPF interface */
    (void)UWPFpicks('R', &i, &uwPick);
    if (strcmp(uwPick.type, Z2P_PICK_LABEL) != 0)
      continue;
    if ( (rc = addCompToEvent(uwPick.chan.name, uwPick.chan.comp, "UW", "--", pEvt,
                              plmParams, &pSta, &pComp)) < 0)
      return -1;
    else if (rc == 0)
    {
      pComp->z2pAmp = fabs((double)uwPick.ampvalue);
      pComp->z2pTime = (double)uwPick.amptime + gPickRefSec;
      
      /* set "no-amp" values until we find the picks */
      pComp->p2pMin = pComp->p2pMax = -1.0;
      pComp->p2pMinTime = pComp->p2pMaxTime = -1.0;
      pComp->p2pAmp = -1.0;
      
      if (plmParams->debug)
        logit("", "Processing <%s.%s.%s.%s>\n", pSta->sta, pComp->name, 
              pSta->net, pComp->loc);

      /* Now see if there exist picks for the plus and minus peak-to-peaks */
      if ( (j = UWPFpickIndex(uwPick.chan, P2P_PLUS_LABEL)) < 0)
        continue;  /* No "plus" pick, so a "minus" won't do any good */
      UWPFpicks('R', &j, &uwPick);
      pComp->p2pMax = (double)uwPick.ampvalue;
      pComp->p2pMaxTime = (double)uwPick.amptime + gPickRefSec;
      if (j == i + 1)
        i = j;  /* The "plus" pick followed the Z2P pick, so we *
                 * don't have to look at it again.              */
      
      if ( (j = UWPFpickIndex(uwPick.chan, P2P_MINUS_LABEL)) < 0)
      {   /* No "minus" pick, so forget about the "plus" */
        pComp->p2pMax = -1.0;
        pComp->p2pMaxTime = -1.0;
        continue;
      }
      UWPFpicks('R', &j, &uwPick);
      pComp->p2pMin = (double)uwPick.ampvalue;
      pComp->p2pMinTime = (double)uwPick.amptime + gPickRefSec;
      if (j == i + 1)
        i = j;  /* The "minus" pick followed the Z2P pick, so we *
                 * don't have to look at it again.               */
      pComp->p2pAmp = pComp->p2pMax - pComp->p2pMin;
    }
    /* No need to complain about rc > 0 returns, since *
     * addCompToEvent already did.                     */
  }

  return 0;
}

      
        
/*
 * getAmpFromUW: obtain the peak Wood-Anderson amplitudes from a 
 *                UW-format data. If the trace data is raw, a
 *                Wood-Anderson trace will be synthesized; 
 *                otherwise the W-A trace is read from the data file. 
 *                Peak amplitudes will be measured from W-A trace data. 
 *                Newly synthesized Wood-Anderson data will optionally 
 *                be written to a new file.
 *  Returns: 0 on success
 *  
 */
int getAmpFromUW(EVENT *pEvt, LMPARAMS *plmParams, DATABUF *pTrace)
{
  double UWstart, UWend;  /* times of a UW trace */
  float samplerate;
  char *sta, *comp, net[1], loc[3]; 
  int i, chLen, maxLen, nChan, rc, force_on;
  long j, isamp, nsamp;
  STA *pSta;
  COMP3 *pComp;
  struct Time dataTime;

  /* The datafile is already open, from getUWpick(); Configure() requires *
   * that if we use a UW datafile for traceSource, eventSource must be    *
   * a UW pickfile. This assures that getUWpick() has been called by now. */

  nChan = UWDFdhnchan();
  maxLen = UWDFmax_trace_len();
  if ( (seis = (float *)calloc(maxLen, sizeof(float))) == (float *)NULL)
  {
    logit("", "getAmpFromUW: out of memory for seis array\n");
    return -1;
  }

  loc[0] = '\0';
  strcpy(loc, "--");
  net[0] = '\0';/* UW doesn't use network names */
  force_on = 0; /* We don't want channels if they are mis-wired */
  for (i = 0; i < nChan; i++)
  {
    sta = UWDFchname(i);
    comp = UWDFchcompflg(i);
    if ( (rc = addCompToEvent(sta, comp, net, loc, pEvt, plmParams, &pSta, 
                              &pComp)) < 0)
    {    /* don't complain, addCompToEvent already did */
      return -1;
    }
    else if (rc > 0)
      continue;      /* this SCN not selected for some reason */

    if (plmParams->debug)
      logit("", "Processing <%s.%s.%s.%s>\n", pSta->sta, pComp->name, pSta->net, pComp->loc);
    
    /* This SCN is wanted, so get its trace data */
    EstPhaseArrivals(pSta, pEvt, plmParams->debug & LM_DBG_TIME);
    if (UWDFchret_trace(i, 'F', seis, force_on) == FALSE)
    {
      logit("", "getAmpFromUW: no trace available for %s.%s from <%s>\n",
            sta, comp, plmParams->UWpickfile);
      continue;
    }
    
    /* Initialize the trace buffer */
    cleanTrace();

    /* Copy the desired part of the trace into DATABUF array */
    /* Try for the configured trace start and end times */
    if (plmParams->fWAsource == 0)
      /* set up for taper if trace needs processing */
      setTaperTime(pSta, plmParams, pEvt);

    pTrace->starttime = traceStartTime(pSta, plmParams) - pSta->timeTaper;
    pTrace->endtime = traceEndTime(pSta, plmParams, pEvt) + pSta->timeTaper;

    samplerate = UWDFchsrate(i);
    if (samplerate < 0.1)
    {
      logit("", "getAmpFromUW: %s.%s has unreasonable samplerate: %f\n",
            sta, comp, samplerate);
      continue;
    }
    pTrace->delta = 1.0 / (double)samplerate;
    chLen = UWDFchlen(i);
    (void)UWDFchref_stime(i, &dataTime);  /* Always returns TRUE */
    UWstart = uwTimeToSec( &dataTime );
    UWend = UWstart + (double)(chLen - 1) * pTrace->delta;
    if (UWstart > pTrace->starttime)
    {
      pTrace->starttime = UWstart;
      isamp = 0;
    }
    else
    {  /* where to start reading the UW data into the trace buffer */
      isamp = (long)( 0.5 + (pTrace->starttime - UWstart) / pTrace->delta);
    }
    
    if (UWend < pTrace->endtime)
    {
      pTrace->endtime = UWend;
      nsamp = (long)chLen;
    }
    else
    {
      nsamp = (long)(chLen - ( (UWend - pTrace->endtime) / pTrace->delta));
    }
    /* If it won't fit, chop some of the end */
    if ( nsamp - isamp > pTrace->lenRaw)
    {
      pTrace->endtime -= (double)( (nsamp - isamp - pTrace->lenRaw ) * 
                                   pTrace->delta);
      nsamp = pTrace->lenRaw - isamp;
    }
    pTrace->nRaw = nsamp - isamp;
    j = 0;
    for ( ; isamp < nsamp; isamp++)
      pTrace->rawData[j++] = (double)seis[isamp];
    
    prepTrace(pTrace, pSta, pComp, plmParams, pEvt, plmParams->fWAsource);
    
    if (plmParams->fWAsource == 0)
    {   /* Now we have a generic trace; turn it into a Wood-Anderson trace */
      rc = makeWA( pSta, pComp, plmParams, pEvt );
      if ( rc > 0)
        continue;
      else if (rc < 0)
        return -1;
    }
    
    /* Get the peak-to-peak and zero-to-peak amplitudes */
    getPeakAmp( pTrace, pComp, pSta, plmParams, pEvt);
      
    if (plmParams->saveTrace != LM_ST_NO)
      saveWATrace( pTrace, pSta, pComp, pEvt, plmParams);

  }
  (void)UWDFclose_file();

  return 0;
}

  
/* 
 * writeUWEvent: write out the event data as a UW-format pickfile.
 *               We start with a copy of the incoming pickfile, 
 *               if one exists. Otherwise, we have to create one.
 */
void writeUWEvent(EVENT *pEvt, LMPARAMS *plmParams)
{
  STA *pSta;
  COMP1 *pC1;
  COMP3 *pComp;
  struct Pick waPick;
  struct Time T;
  float origin, lat, lon, depth;
  int dir, i, ncmts, npicks, orig_sec;
  char comment[80];
  
  cleanPick(&waPick);
  if (plmParams->eventSource != LM_ES_UW)
  {  /* No pickfile to copy; set up a new one with origin time and location */
    (void)UWPFstartNewPickfile(); /* Always returns TRUE */
    secToUWTime(pEvt->origin_time, &T);

    /* Save the integer seconds for generating the file name */
    orig_sec = (int)T.sec;

    /* Save the reference time as a double */
    gPickRefSec = pEvt->origin_time - (double)T.sec;

    T.sec = 0.0;  /* Always use zero seconds for UW reference time */
    /* Now T is the reference time as a struct Time */
    (void)UWPFrefTime('S', &T);

    origin = (float)(pEvt->origin_time - gPickRefSec);
    UWPForigsec('S', &origin);

    lat = (float)pEvt->lat;
    lon = (float)pEvt->lon;
    UWPFevtLatLon('S', &lat, &lon);

    depth = (float)pEvt->depth;
    UWPFdepth('S', &depth, "");
    UWPFcomment('A', &ncmts, "Pickfile created by localmag");
    
    if (plmParams->saveTrace == LM_ST_UW)
    {  /* We're making a UW datafile, so we can use its name */
      strcpy(gWApickfile, gWAdatafile);
      LAST_CHAR(gWApickfile) = DEFAULT_PICK_TAG;
    }
    else
    {  /* We have to roll out own name, from the origin time */
      sprintf(gWApickfile, "%02d%02d%02d%02d%02d%1d.wa.a", T.yr % 100,
              T.mon, T.day, T.hr, T.min, orig_sec / 10);
    }
    /* We don't actually use this name until the end of this function */
  }
  else
  {/* We have an incoming pickfile, so we'll modify its name * 
    * for the new one , unless it's already been modified .  */
    if (plmParams->fGetAmpFromSource == FALSE)
      toWAname(plmParams->UWpickfile, gWApickfile);
    else
      strcpy(gWApickfile, plmParams->UWpickfile);
  }
  
  /* Add the Wood-Anderson amplitude picks */  
  for (i = 0; i < pEvt->numSta; i++)
  {
    pSta = &pEvt->Sta[i];
    pC1 = pSta->comp;
    while( pC1 != (COMP1 *)NULL)
    {
      for (dir = 0; dir < 2; dir++)
      {
        pComp = &pC1->c3[dir];
        if (pComp->name[0] == 0 || pComp->z2pTime < 0.0)
          continue;  /* Skip empty component slots */
        strcpy(waPick.chan.name, pSta->sta);
        strcpy(waPick.chan.comp, pComp->name);
        strcpy(waPick.type, Z2P_PICK_LABEL);
        waPick.amptime = (float)(pComp->z2pTime - gPickRefSec);
        waPick.ampvalue = (float)pComp->z2pAmp;
        waPick.ampon = TRUE;
        if (plmParams->fGetAmpFromSource == FALSE)
          waPick.machine_amp = TRUE;
        (void)UWPFpicks('A', &npicks, &waPick); /* Always returns TRUE */

        if (pComp->p2pMaxTime > 0.0)
        {
          strcpy(waPick.type, P2P_PLUS_LABEL);
          waPick.amptime = (float)(pComp->p2pMaxTime - gPickRefSec);
          waPick.ampvalue = (float)pComp->p2pMax;
          (void)UWPFpicks('A', &npicks, &waPick); /* Always returns TRUE */
        }
        if (pComp->p2pMinTime > 0.0)
        {
          strcpy(waPick.type, P2P_MINUS_LABEL);
          waPick.amptime = (float)(pComp->p2pMinTime - gPickRefSec);
          waPick.ampvalue = (float)pComp->p2pMin;
          (void)UWPFpicks('A', &npicks, &waPick); /* Always returns TRUE */
        }
      }
      pC1 = pC1->next;
    }   /* Loop over linked list of COMP1 structs */
  }  /* Loop over all stations */

  /* Remove any old localmag comments */
  (void)UWPFcomment('I', &ncmts, comment);
  for (i = 0; i < ncmts; i++)
  {
    (void)UWPFcomment('R', &i, comment);
    if (memcmp(comment, LM_UW_COMMENT, sizeof(LM_UW_COMMENT)-1) == 0 || 
        strcmp(comment, LM_UW_NOMAG) == 0)
      (void)UWPFcomment('U', &i, comment);
  }

  if (pEvt->mag < 0.0)
    UWPFcomment('A', &ncmts, LM_UW_NOMAG);
  else
  {
    sprintf(comment, "%s %4.2f Nsta %2d", LM_UW_COMMENT, pEvt->mag, 
            pEvt->nMags);
    UWPFcomment('A', &ncmts, comment);
  }
  
  /* Finally we are read to write the pickfile to disk */
  UWPFwr1evt(gWApickfile);
  
  return;
}


/*
 * initUWSave: initialize the UW trace saving mechanism: make a new datafile
 *             name, based on the original pickfile if there is one.
 *             Copy the contents of the raw datafile header, if there is one.
 *             Otherwise set up a new datafile with name based on origin
 *             time.
 *   Returns: nothing; since saving UW data files is not localmag's primary
 *              mission, we don't care if this routine fails. A logit()
 *              entry is sufficient notification of the problem.
 */
void initUWSave(EVENT *pEvt, LMPARAMS *plmParams)
{
  struct tm *t;
  time_t origin;
  
  if (gfUWSaveInit == 1)
  {
    logit("", "initUWSave called when initialized; call termUWSave first\n");
    return;   /* Don't initialize twice */
  }

  gfUWSaveInit = -1; /* means initUWSave failed */
  
  if (plmParams->traceSource == LM_TS_UW)
  {   /* We can use the old datafile name and header contents */
    toWAname(gDatafile, gWAdatafile);
    if (UWDFinit_for_new_write(gWAdatafile) == FALSE)
    {
      logit("", "initUWSave: error initializing new datafile <%s>: %s\n",
            gWAdatafile, strerror(errno));
      return;
    }
    (void)UWDFdup_masthead();  /* Always returns TRUE */
  }
  else if (plmParams->eventSource == LM_ES_UW)
  {    /* We can use the pickfile name, but we have to make a new header */
    toWAname(plmParams->UWpickfile, gWAdatafile);
    LAST_CHAR(gWAdatafile) = 'W';  /* Ah, the beauty of macros... */
    if (UWDFinit_for_new_write(gWAdatafile) == FALSE)
    {
      logit("", "initUWSave: error initializing new datafile <%s>: %s\n",
            gWAdatafile, strerror(errno));
      return;
    }
    /* Can't think of anything to add to the header... */
  }
  else
  {    /* We have to make up a new name for the WA datafile */
    origin = (time_t)pEvt->origin_time;
    t = gmtime(&origin);
    sprintf(gWAdatafile, "%02d%02d%02d%02d%02d%1d.wa.W", t->tm_year % 100,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_mon, t->tm_sec / 10);
    if (UWDFinit_for_new_write(gWAdatafile) == FALSE)
    {
      logit("", "initUWSave: error initializing new datafile <%s>: %s\n",
            gWAdatafile, strerror(errno));
      return;
    }
    /* Can't think of anything to add to the header... */
  }
  if (UWDFwrite_new_head() == FALSE)
  {
    logit("", "initUWSave: error writing header of %s: %s\n", gWAdatafile,
          strerror(errno));
    return;
  }
  
  if (seis == (float *)NULL)
  {
    if ( (seis = (float *)calloc(plmParams->maxTrace, sizeof(float))) == 
         (float *)NULL)
    {
      logit("", "initUWSave: out of memory for seis array\n");
      return;
    }
  }
  
  /* Success: set our flag to say we're initialized */
  gfUWSaveInit = 1;
  return;
}
    
    

/*
 * saveUWWATrace: save the synthetic Wood-Anderson trace to a UW-format
 *                data file. No writing is done yet; the trace is saved
 *                in memory by the UWDFIF routines.
 *                This function returns no status, on the grounds that saving
 *                trace data is not the primary function of localmag.
 *                If saving fails, localmag will continue on unimpeded.
 */
void saveUWWATrace(DATABUF *pTrace, STA *pSta, COMP3 *pComp, 
                   LMPARAMS *plmParams)
{
  struct Time T;
  long i;
  
  /* Make sure we are prepared for action */
  if (gfUWSaveInit == 0)
  {
    logit("", "saveUWWATrace called while UWSave not initialized\n");
    return;
  }
  else if (gfUWSaveInit != 1)
    return;  /* Init failed; don't bother doing anything more */
  
  UWDFset_chname(pSta->sta);
  UWDFset_chcompflg(pComp->name);
  UWDFset_chsrate((float)(1.0/pTrace->delta));
  UWDFset_chsrc(WA_SOURCE_CODE); /* This may need to be an option */

  secToUWTime(pTrace->starttime, &T);
  UWDFset_chref_stime(T);
  
  /* Copy the Wood-Anderson trace data into the datafile buffer. *
   * We may optionally want to scale the trace here so it looks  *
   * OK on xped. Make sure the scaling gets labeled in the file. */
  for (i = 0; i < pTrace->nProc; i++)
  {
    seis[i] = (float)pTrace->procData[i];
  }
  
  UWDFwrite_new_chan('F', pTrace->nProc, seis, 'F');
  
  return;
}


/*
 * termUWSave: terminate the saving process. This is where the UW-format
 *             data file is actually written to disk.
 *                This function returns no status, on the grounds that saving
 *                trace data is not the primary function of localmag.
 *                If saving fails, localmag will continue on unimpeded.
 */
void termUWSave(EVENT *pEvt, LMPARAMS *plmParams)
{

  /* Make sure we are prepared for action */
  if (gfUWSaveInit == 0)
  {
    logit("", "termUWSave called while UWSave not initialized\n");
    return;
  }
  else if (gfUWSaveInit != 1)
    return;  /* Init failed; don't bother doing anything more */

  UWDFclose_new_file();
  gfUWSaveInit = 0;

  return;
}


/*
 * getUWResp: fill in the pole-zero-gain response structure from the
 *            UW-format calibration file.
 *            Currently UW does not use the net code to identify stations.
 *  Returns: 0 on success
 *          +1 if poles and zeros not found for sta.comp
 *          -1 on fatal errors
 */
int getUWResp(char *sta, char *comp, char *net, char *loc, ResponseStruct *pRespPZ,
              EVENT *pEvt)
{
  char tmps[4], tmpc[3];
  int i, yyyymmdd;
  float gain;
  struct Time T;
  struct 
  {
    float x;
    float y;
  } getpoles[50];
  struct 
  {
    float x;
    float y;
  } getzeros[50];

  /* Fill in the arrays tmps and tmpc; don't null-terminate *
   * since they are being passed to fortran.                */                 
  strncpy(tmps, sta, 4);
  if (tmps[3] == '\0')
    tmps[3] = ' ';
  strncpy(tmpc, comp, 3);
  secToUWTime(pEvt->origin_time, &T);
  yyyymmdd = T.yr * 10000 + T.mon * 100 + T.day;
  
  getpzg_(tmps, tmpc, &yyyymmdd, &gain, &pRespPZ->iNumPoles,
          getpoles, &pRespPZ->iNumZeros, getzeros, 4L, 3L);
  if (pRespPZ->iNumPoles == 0)
  {
    logit("", "getUWResp: unable to find response data for %s.%s in calib.sta\n",
          sta, comp);
    return +1;
  }
  if ( (pRespPZ->Poles = (PZNum *)malloc(pRespPZ->iNumPoles * sizeof(PZNum))) 
       == (PZNum *)0 )
  {
    logit("", "getUWResp: out of memory for zeros\n");
    return -1;
  }
  for (i = 0; i < pRespPZ->iNumPoles; i++)
  {
    pRespPZ->Poles[i].dReal = (double)getpoles[i].x;
    pRespPZ->Poles[i].dImag = (double)getpoles[i].y;
  }

  if (pRespPZ->iNumZeros > 0)
  {
    if ( (pRespPZ->Zeros = (PZNum *)malloc(pRespPZ->iNumZeros * sizeof(PZNum))) 
         == (PZNum *)NULL )
    {
      logit("", "getUWResp: out of memory for poles\n");
      return -1;
    }
    for (i = 0; i < pRespPZ->iNumZeros; i++)
    {
      pRespPZ->Zeros[i].dReal = (double)getzeros[i].x;
      pRespPZ->Zeros[i].dImag = (double)getzeros[i].y;
    }
  }
  pRespPZ->dGain = (double)gain;

  return 0;
}

/*
 * getUWStaLoc: look up station location in the UW stafile interface.
 *   Returns: 0 on success
 *           -1 on failure
 */
int getUWStaLoc(char *sta, char *net, double *pLat, double *pLon)
{
  float elev;
  
  /* UW doesn't use netowrk code for station locations now */
  if (STAget_declatlon(sta, pLat, pLon, &elev) == FALSE)
    return -1;
  else
    return 0;
}


/*
 * uwTimeToSec: convert UW Time structure to seconds since 1970, as a double.
 *  Returns: UW time as a double.
 */
static double uwTimeToSec( struct Time *pUW )
{
  struct tm tms;
  time_t sec;
  
  tms.tm_year = pUW->yr - 1900;
  tms.tm_mon = pUW->mon - 1; /* UW months 1 - 12; tm months 0 - 11 */
  tms.tm_mday = pUW->day;
  tms.tm_hour = pUW->hr;
  tms.tm_min = pUW->min;
  tms.tm_sec = (int) pUW->sec;
  tms.tm_isdst = 0;
  sec = timegm_ew(&tms);

  return (double)sec + (double)(pUW->sec - (int)pUW->sec);
}

/*
 * secToUWTime: convert time as seconds (a double) since 1970 to
 *              a UW Time struct.
 */
static void secToUWTime(double sec, struct Time *T)
{
  struct tm *t;
  time_t tSec;
  
  tSec = (time_t)sec;
  t = gmtime(&tSec);
  T->yr = t->tm_year + 1900;
  T->mon = t->tm_mon + 1; /* UW months 1 - 12; tm months 0 - 11 */
  T->day = t->tm_mday;
  T->hr = t->tm_hour;
  T->min = t->tm_min;
  T->sec = (float)(t->tm_sec + (sec - (time_t)sec));
  return;
}


/*
 * toWAname: insert ".wa." before the last character of a string.
 *     Based on function of the same name in uw2ml by George Thomas of UW.
 */
static void toWAname(char *oldname, char *newname)
{
    char tmp[2];
    
    strcpy(newname, oldname);
    tmp[0] = LAST_CHAR(newname);
    tmp[1] = '\0';
    LAST_CHAR(newname) = '\0';
    strcat(newname, WA_FILE_LABEL);
    strcat(newname, tmp);
    
    return;

}

/*
 * cleanPick: clean all entries in the UW Pick structure.
 */
static void cleanPick( struct Pick *p )
{
  p->type[0] = '\0';
  p->pol[0] = '\0';
  p->time = p->etime = p->res = p->dur = p->dist = 0.0;
  p->wt[0] = '\0';
  p->ampon = p->ietime = p->ires = p->idur = FALSE;
  p->pickon = p->machine_time = p->machine_dur = p->machine_amp = 0.0;
  p->amptime = p->ampvalue = 0.0;
  p->chan.name[0] = '\0';
  p->chan.comp[0] = '\0';
  p->chan.chid[0] = '\0';
  
  return;
}

