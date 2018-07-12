/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_util.c 6416 2015-10-23 18:26:39Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.9  2002/09/26 19:29:37  dhanych
 *     fixed 'BadBitMap' to 'BadBitmap'
 *
 *     Revision 1.8  2002/09/25 17:33:20  dhanych
 *     added state checking at start of makeGM() to reject traces for error
 *     conditions found by prepTrace(); e.g. gaps, signal-to-noise.
 *
 *     Revision 1.7  2002/02/28 17:03:36  lucky
 *     Moved gma.c and gma.h to libsrc and main include
 *
 *     Revision 1.6  2001/07/18 19:41:36  lombard
 *     Changed XMLDir, TempDir and MappingFile in GMPARAMS struct from string
 *     arrays to string pointers. Changed gm_config.c and gm_util.c to support thes
 *     changes. This solved a problem where the GMPARAMS structure was getting
 *     corrupted when a pointer to it was passed into getGMFromTrace().
 *     It's not clear why this was necessary; purify didn't complain.
 *
 *     Revision 1.5  2001/07/18 19:18:25  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2001/06/10 21:29:07  lombard
 *     fixed memory leak in endEvent.
 *
 *     Revision 1.3  2001/04/21 19:18:32  lombard
 *     removed prohibition of using Z components
 *
 *     Revision 1.2  2001/04/11 21:12:20  lombard
 *     ../localmag/site.h ../localmag/lm_site.h
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_util.c: A bunch of utility functions used for gmew.
 *  
 *    Pete Lombard; January, 2001
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <earthworm.h>
#include <chron3.h>
#include <fft_prep.h>
#include <read_arc.h>
#include <rw_strongmotionII.h>
#include <tlay.h>
#include <transfer.h>
#include <gma.h>
#include "../localmag/lm_misc.h"
#include "gm.h"
#include "gm_sac.h"
#include "gm_util.h"
#include "gm_ws.h"
#include "gm_xml.h"
#include "../localmag/lm_site.h"

/* Standard Spectral Response periods and damping value */
/*#define NSP 3*/
#define NSD 1
static int NSP = 0;
static double spectPer[SM_MAX_RSA];
static double spectDamp[NSD] = {0.05};

/* global variables for gm_util.c: */
static DATABUF gTrace;    /* the trace buffer for raw and processed data  */
static double *gWork;     /* the work array for convertWave               */
static double *gWorkFFT;  /* the work array for fft99                     */
static ResponseStruct gScnPZ;  /* Poles, zeros, and gain for the SCNL      */
static SM_INFO gSM;       /* Strong-motion infor structure                */

/* Internal Function Prototypes */
static int getRespPZ(char *, char *, char *, char *, GMPARAMS *, EVENT *);

/*
 * getGMFromTrace: selector function to call the desired procedure
 *           for getting ground-motion from a trace source such as wave_server,
 *           SAC file, etc.
 *    Returns: 0 on success
 *            -1 on fatal errors
 */
int getGMFromTrace( EVENT *pEv, GMPARAMS *pgmParams)
{
  int rc;
  
  /* Initialize the SAC trace saver. We assume that this is the   *
   * only type of trace saver; if more are added, then we'll need *
   * an initSave() routine that will select the desired saver.    */
  if (pEv->eventId[0] && pgmParams->saveTrace == GM_ST_SAC)
    initSACSave(pEv, pgmParams);

  /* Initialize the XML writer */
  if (pEv->eventId[0] && pgmParams->XMLDir && strlen(pgmParams->XMLDir) > 0)
    (void) Start_XML_file( pgmParams, pEv );

  switch(pgmParams->traceSource)
  {
  case GM_TS_WS:
    rc = getGMFromWS( pEv, pgmParams, &gTrace, &gSM);
    break;
#ifdef EWDB
  case GM_TS_EWDB:
    rc = getGMFromEWDB( pEv, pgmParams, &gTrace, &gSM);
    break;
#endif
  default:
    logit("et", "gmew getGMFromTrace: unknown source: %d\n",
          pgmParams->traceSource);
    rc = -1;
  }

  if (pEv->eventId[0] && pgmParams->saveTrace == GM_ST_SAC)
    termSACSave(pEv, pgmParams);
  
  /* Terminate the XML writer */
  if (pEv->eventId[0] && pgmParams->XMLDir && strlen(pgmParams->XMLDir) > 0)
  {
    if (Close_XML_file( )== 0 && pgmParams->sendActivate == 1) 
    {
       Send_XML_activate(pEv, pgmParams);
    }
  }

  return rc;
}

/*
 * addCompToEvent: Add a new component to the EVENT structure.
 *     Checks that SCN meets the selection criteria of the lists add and pDel,
 *     the station is within the distance limits, and that the limit
 *     on number of stations is not exceeded.
 *   Returns: 0 on success
 *           +1 if comp is a duplicate of one already in the list
 *           +2 if comp is not a selected component
 *           +3 if station number limit is reached
 *           +4 if component direction is not understood
 *           +5 if station is outside distance limit
 *           +6 if station location cannot be determined
 *           -1 on failure (error allocating memory)
 */
int addCompToEvent( char *sta, char *comp, char *net, char *loc, EVENT *pEvt, 
                    GMPARAMS *pgmParams, STA **ppSta, COMP3 **ppComp)
{
  COMP1 *this, *last;
  PSCNLSEL thisSel;
  SCNLPAR keySCNL;
  double dist, lat, lon;
  int i, dir;
  int foundIt = 0, new = 0;
  int selected = 0;
  int mSta, mComp, mNet, mLoc;   /* `match' flags */
  
  /* Is this SCNL on the selection "Add" list?                         *
   * If Add list is empty, assume that all stations are wanted.       */
  if ( (thisSel = pgmParams->pAdd) == (PSCNLSEL)NULL )
    selected = 1;
  else
  {
    while (thisSel != (PSCNLSEL)NULL)
    {
      mSta = mComp = mNet = mLoc = 0;
      if (thisSel->sta[0] == '*' || strcmp(thisSel->sta, sta) == 0) mSta = 1;
      if (thisSel->net[0] == '*' || strcmp(thisSel->net, net) == 0) mNet = 1;
      if (thisSel->comp[0] == '*' || 
          memcmp(thisSel->comp, comp, strlen(thisSel->comp)) == 0) mComp = 1;
      if (thisSel->loc[0] == '*' || 
          memcmp(thisSel->loc, loc, strlen(thisSel->loc)) == 0) mLoc = 1;
      if ( (selected = mSta && mNet && mComp && mLoc) == 1)
        break;
      
      thisSel = thisSel->next;
    }
  }
  if (!selected)
  {
    if (pgmParams->debug & GM_DBG_SEL)
      logit("et", "addCompToEvent: <%s.%s.%s.%s> not in select list: (%d.%d.%d.%d)\n",
            sta, comp, net, loc, mSta, mComp, mNet, mLoc);
    return +2;
  }

  /* Is this SCNL on the selection "Del" list? */
  thisSel = pgmParams->pDel;
  while (thisSel != (PSCNLSEL)NULL)
  {
    mSta = mComp = mNet =  mLoc =0;
    if (thisSel->sta[0] == '*' || strcmp(thisSel->sta, sta) == 0) mSta = 1;
    if (thisSel->net[0] == '*' || strcmp(thisSel->net, net) == 0) mNet = 1;
    if (thisSel->comp[0] == '*' || 
        memcmp(thisSel->comp, comp, strlen(thisSel->comp)) == 0) mComp = 1;
    if (thisSel->loc[0] == '*' || 
        memcmp(thisSel->loc, loc, strlen(thisSel->loc)) == 0) mLoc = 1;
    if ( (mSta && mNet && mComp && mLoc) == 1)
    {
      if (pgmParams->debug & GM_DBG_SEL)
        logit("et", "addCompToEvent: <%s.%s.%s.%s> in delete list: (%d.%d.%d.%d)\n",
              sta, comp, net, loc, mSta, mComp, mNet, mLoc);
      return +2;
    }
    thisSel = thisSel->next;
  }
  
  /* What direction is this component? We do this here, since it could
   * possibly fail. We don't want to allocate a new COMP1 structure (below)
   * and then not use it because this test failed. */
  switch(comp[2])
  {
  case 'E':
  case '2':
    dir = GM_E;
    break;
  case 'N':
  case '3':
    dir = GM_N;
    break;
  case 'Z': 
    dir = GM_Z;
    break;
  default:
    logit("et", "addCompToEvent: unknown component direction for <%s.%s.%s.%s>\n",
          sta, comp, net, loc);
    return +4;
  }

  /* Search the station list for a match of sta, net */
  for (i = 0; i < pEvt->numSta; i++)
  {
    if (strncmp(pEvt->Sta[i].sta, sta, 6) == 0 &&
        strncmp(pEvt->Sta[i].net, net, 3) == 0 )
    {
        foundIt = 1;
        break;  
    }
  }
  /* Our desired sta/net is at index `i' regardless of foundIt value */
  if (foundIt == 0)  /* No match; add to the list if it isn't full */
  {
    if ( pEvt->eventId[0]!=0 && pEvt->eventId[0]!='-' ) {
		/* Find its epicentral distance; make sure its in range */
		dist = getStaDist(sta, comp, net, loc, &lat, &lon, pEvt, pgmParams);
		if ( dist < 0.0 )
		  return +6;
		else if (dist > pgmParams->maxDist)
		{
		  if (pgmParams->debug & GM_DBG_SEL)
			logit("et", "<%s.%s.%s.%s> distance (%e) exceeds limit\n", sta, comp, net, loc,
				  dist);
		  return +5;
		}
	}
    if (pEvt->numSta < pgmParams->maxSta)
    {
      strncpy(pEvt->Sta[i].sta, sta, TRACE_STA_LEN);
      strncpy(pEvt->Sta[i].net, net, TRACE_NET_LEN);
      if ( pEvt->eventId[0] ) {
		  pEvt->Sta[i].lat = lat;
		  pEvt->Sta[i].lon = lon;
		  pEvt->Sta[i].dist = dist;
	  }
   	  pEvt->Sta[i].comp = NULL;
      pEvt->numSta++;
    }
    else
    {
      logit("et", 
            "addCompToEvent: station limit reached; skipping <%s.%s.%s.%s>\n",
            sta, comp, net, loc );
      pEvt->Sta[i].dist = -1.0;
      return +3;
    }
  }
  *ppSta = &pEvt->Sta[i];
  
  if ( pEvt->Sta[i].comp == (COMP1 *)NULL)
  { 
      /* No components for this station, so create a new one */
    if ( (pEvt->Sta[i].comp = (COMP1 *)calloc(sizeof(COMP1), 1)) == 
         (COMP1 *)NULL)
    {
      logit("et", "addCompToEvent: out of memory for COMP1\n");
      return -1;
    }
    this = pEvt->Sta[i].comp;
    new = 1;
  }
  else
  {
    this = pEvt->Sta[i].comp;
    foundIt = 0;
    /* Walk the COMP list */
    do
    {    /* Search for match of the first 2 letters in component name */
      if (memcmp(this->n2, comp, 2) == 0)
      {
        foundIt = 1;
        break;
      }
      last = this;
    } 
    while ( (this = this->next) != (COMP1 *)NULL);
    
    if (foundIt == 0)
    {           /* No match, so add a new COMP1 structure to the list */
      if  ( (last->next = (COMP1 *)calloc(sizeof(COMP1), 1)) == 
            (COMP1 *)NULL)
        return -1;
      new = 1;
      this = last->next;
    }
  }
  /* Now `this' is pointing at our COMP1 structure */

  if (new == 1)
  {      /* Fill in the new COMP1 name */
    this->n2[0] = comp[0]; 
    this->n2[1] = comp[1];
  }
  else if (this->c3[dir].name[0] != 0 && pgmParams->allowDuplicates == 0)
  {  /* This component/direction is a duplicate */
    if ( (pgmParams->debug & GM_DBG_SEL) > 0 )
      logit("et", "addCompToEvent: <%s.%s.%s.%s> rejected as a duplicate\n",
            sta, comp, net, loc);
    return +1;
  }

  /* This component/direction is new for this event */
  strncpy(this->c3[dir].name, comp, 3);
  strncpy(this->c3[dir].loc, loc, 3);
  this->c3[dir].BadBitmap = 0; /* this may just be a different location code, zero out values from prior comps */
  *ppComp = &this->c3[dir];

  /* Search for an SCNLPAR structure for this SCNL */
  if (pgmParams->numSCNLPar > 0)
  {
    memset(&keySCNL, 0, sizeof(SCNLPAR));
    strcpy(keySCNL.sta, sta);
    strcpy(keySCNL.comp, comp);
    strcpy(keySCNL.net, net);
    strcpy(keySCNL.loc, loc);
    this->c3[dir].pSCNLPar = (SCNLPAR *)bsearch(&keySCNL, pgmParams->pSCNLPar,
                                              pgmParams->numSCNLPar,
                                              sizeof(SCNLPAR), CompareSCNLPARs);
  }
  else
    this->c3[dir].pSCNLPar = (SCNLPAR *)NULL;
  
  return 0;
}


/*
 * getStaDist: Determine the epicentral distance for a station, given
 *          the station name "sta", the network name "net", the component
 *          name "comp", and the EVENT structure "pEvt".
 *          Currently, staDist gets station locations from the
 *          site table initialized with the site_load() calls from
 *          site.c. This initialization must be done before calling staDist().
 *   Returns: the epicentral distance if the station could be found
 *           -1.0 if the station location could not be determined.
 */
double getStaDist( char *sta, char *comp, char *net, char *loc, double *pLat, 
                   double *pLon, EVENT *pEvt, GMPARAMS *pgmParams)
{
  double r = -1.0;           /* Epicentral distance */
  SITE *pSite;               /* Site table entry */
  
  switch(pgmParams->staLoc)
  {
  case GM_SL_HYP:
    if ( (pSite = find_site( sta, comp, net, loc )) == 
         (SITE *)NULL)
    {
      logit("et", "getStaDist: <%s.%s.%s.%s> - Not in station list.\n", 
            sta, comp, net, loc);
      return r;
    }
    *pLat = pSite->lat;
    *pLon = pSite->lon;
    if ( pEvt->eventId[0] )
		r = utmcal(pEvt->lat, pEvt->lon, pSite->lat, pSite->lon);
  	else
  		r = 0.0;
    break;
#ifdef EWDB
  case GM_SL_EWDB:
    /* Get station location from EWDB */
    break;
#endif
  }


  return r;
}



/*
 * EstPhaseArrivals: estimate the P and S arrival times for this
 *    station; set the values for all components of this station.
 *    Uses the "tlay" routines for travel-time calculations using
 *    a single multi-layered regional velocity model.
 */  
void EstPhaseArrivals(STA *pSta, EVENT *pEvt, int debug)
{
  TPHASE treg[4];
  int nreg;
  
  nreg = t_region(pSta->dist, pEvt->depth, treg);
  if (nreg == 2)
  {    
    pSta->p_est = treg[0].t;
    pSta->s_est = treg[1].t;
  }
  else
  {
    pSta->p_est = (treg[0].t < treg[2].t) ? treg[0].t : treg[2].t;
    pSta->s_est = (treg[1].t < treg[3].t) ? treg[1].t : treg[3].t;
  }
  
  if (debug)
  {
    logit("et","Est P: %10.4f", treg[0].t);
    if (nreg == 2)
      logit("e", "\n");
    else
      logit("e", "  (%10.4f)\n", treg[2].t);
    logit("et", "Est S: %10.4f", treg[1].t);
    if (nreg == 2)
      logit("e", "\n");
    else
      logit("e", "  (%10.4f)\n", treg[3].t);
  }

  pSta->p_est += pEvt->origin_time;
  pSta->s_est += pEvt->origin_time;
  
  return;
}


/*
 * Return the desired start time for traces from pSta.
 */
double traceStartTime(STA *pSta, GMPARAMS *pgmParams)
{
  return pSta->p_est - pgmParams->traceStart;
}


/*
 * Return the desired end time for traces from pSta.
 */
double traceEndTime(STA *pSta, GMPARAMS *pgmParams)
{
  return pSta->s_est + pgmParams->traceEnd;
}

/*
 * Return the desired peak-search start time for traces from pSta.
 */
double peakSearchStart(STA *pSta, GMPARAMS *pgmParams)
{
  double start;
  
  start = pgmParams->peakSearchStart * (pSta->s_est - pSta->p_est);
  start = (start > pgmParams->peakSearchStartMin) ? start :
    pgmParams->peakSearchStartMin;
  return pSta->s_est - start;
}


/*
 * Return the desired peak-search end time for traces from pSta.
 */
double peakSearchEnd(STA *pSta, GMPARAMS *pgmParams)
{
  double end;
  
  end = pgmParams->peakSearchEnd * (pSta->s_est - pSta->p_est);
  end = (end > pgmParams->peakSearchEndMin) ? end :
    pgmParams->peakSearchEndMin;
  return pSta->s_est + end;
}



/*
 * MakeGM: Transfer a raw trace into synthetic GM traces.
 *         The trace data is in the DATABUF trace, for the SCN identified
 *         by STA and COMP. Instrument response (pole/zero/gain) is
 *         obtained from the specified response source.
 *   Returns: 0 on success
 *           +1 on non-fatal error (no response data for this SCN)
 *           -1 on fatal errors  
 */
int makeGM(STA *pSta, COMP3 *pComp, GMPARAMS *pgmParams, EVENT *pEvt, 
            DATABUF *pTrace)
{
  int i, rc;
  double tTaper, taperFreqs[4] = {1.0, 1.0, 10.0, 10.0};
  long padlen, nfft;
 
  /* reject bad traces (gaps, S/N ratio, etc.)  */
  if ( pComp->BadBitmap )
     return 1;
 
  /* Get the instrument response */
  if ( (rc = getRespPZ(pSta->sta, pComp->name, pSta->net, pComp->loc, pgmParams, 
                       pEvt)) != 0)
  {
    logit("et", "makeGM: no response data for <%s.%s.%s.%s>; skipping\n",
          pSta->sta, pComp->name, pSta->net, pComp->loc);
    return rc;
  }

  if (pComp->pSCNLPar == (SCNLPAR *)NULL)
  {  /* Use the defaults if not set externally */
    /* Set the low-frequency taper band to 0.05 and 0.1 hz.*/
    taperFreqs[0] = 0.05;
    taperFreqs[1] = 0.1;

    /* Set the high-frequency taper band to 90% and 100% of Nyquist */
    taperFreqs[3] = 0.5/pTrace->delta;
    taperFreqs[2] = 0.45/pTrace->delta;

    /* Set acceleration time-series taper length to default 0 */
    tTaper = 0.0;
  }
  else
  {
    for (i = 0; i < 4; i++)
      taperFreqs[i] = pComp->pSCNLPar->fTaper[i];
    tTaper = pComp->pSCNLPar->taperTime;
  }

  if (pgmParams->debug & (GM_DBG_PZG | GM_DBG_TRS | GM_DBG_ARS) )
    printf("\nResponse for <%s.%s.%s.%s> delta %e\nftaper %e %e %e %e\n", 
           pSta->sta, pComp->name, pSta->net, pComp->loc, pTrace->delta, taperFreqs[0],
           taperFreqs[1], taperFreqs[2], taperFreqs[3]);

  padlen = -1;  /* let convertWave figure the padding */
  rc = gma(pTrace->rawData, pTrace->nRaw, pTrace->delta, &gScnPZ, taperFreqs, 
           tTaper, &padlen, &nfft, spectPer, NSP, spectDamp, NSD, 
           pTrace->procData, pTrace->lenProc, gWork, gWorkFFT);
  if (rc < 0)
  {
    switch(rc)
    {
    case -1:
      logit("et", "gma failed: out of memory\n");
      return -1;
      break;
    case -3:
      logit("et", "gma failed: invalid arguments\n");
      return -1;
      break;
    case -4:
      logit("et", "gma failed: FFT error; nfft: %ld\n", nfft);
      return -1;
      break;
    default:
      logit("et", "gma failed: unknown error %d\n", rc);
      return -1;
    }
  }
  /* Do we need to adjust the end of the processed trace? */
  if (nfft - padlen < pTrace->nRaw)
  {    /* gma had to chop some of the end */
    pTrace->endtime -= (pTrace->nRaw - (nfft - padlen)) * pTrace->delta;
    pTrace->nProc = nfft - padlen;
  }
  else
    pTrace->nProc = pTrace->nRaw;

  pTrace->padLen = padlen;
  
  return 0;
}

/*
 * prepTrace: prepare trace data for processing. Preps include: check for
 *    gaps in peak-search window, compute and remove mean, fill gaps,
 *    and check for clipping.
 *    Also fills in the peak-search start and end times.
 */
void prepTrace(DATABUF *pTrace, STA *pSta, COMP3 *pComp, GMPARAMS *pgmParams)
{
  long i, iStart, iEnd, npoints;
  double mean, dMax, dMin, gapEnd, clipLimit, preP, postP;
  GAP *pGap;

  if (pComp->pSCNLPar != (SCNLPAR *)NULL)
    clipLimit = pComp->pSCNLPar->clipLimit;
  else
    clipLimit = 7.55e6; /* 90% of 2^23; clip for 24-bit digitizer */
  
  /* Fill in the peak-search window limits */
  pComp->peakWinStart = peakSearchStart(pSta, pgmParams);
  pComp->peakWinEnd = peakSearchEnd(pSta, pgmParams);

  /* Find mean value of non-gap data */
  pGap = pTrace->gapList;
  i = 0L;
  npoints = 0L;
  mean = 0.0;
  dMax = -4.0e6;
  dMin = 4.0e6;
  /*
   * Loop over all the data, skipping any gaps. Note that a `gap' will not
   * be declared at the end of the data, so the counter `i' will always
   * get to pTrace->nRaw.
   */
  do
  {
    if (pGap == (GAP *)NULL)
    {
      iEnd = pTrace->nRaw;
    }
    else
    {
      iEnd = pGap->firstSamp - 1;

      /* Test for gap within peak-search window */
      gapEnd = pGap->starttime + (pGap->lastSamp - pGap->firstSamp + 1) *
        pTrace->delta;
      if ( (pGap->starttime >= pComp->peakWinStart && 
            pGap->starttime <= pComp->peakWinEnd) ||
           (gapEnd >= pComp->peakWinStart && gapEnd <= pComp->peakWinEnd) ||
           (pGap->starttime < pComp->peakWinStart && 
            gapEnd > pComp->peakWinEnd) )
      {
        logit("et", "trace from <%s.%s.%s.%s> has gap in peak-search window\n",
              pSta->sta, pComp->name, pSta->net, pComp->loc);
        pComp->BadBitmap |= GM_BAD_GAP;
      }
    }
    for (; i < iEnd; i++)
    {
      mean += pTrace->rawData[i];
      dMax = (dMax > pTrace->rawData[i]) ? dMax : pTrace->rawData[i];
      dMin = (dMin < pTrace->rawData[i]) ? dMin : pTrace->rawData[i];
      npoints++;
    }
    if (pGap != (GAP *)NULL)     /* Move the counter over this gap */    
    {
      i = pGap->lastSamp + 1;
      pGap = pGap->next;
    }
  } while (i < pTrace->nRaw );
  
  mean /= (double)npoints;
  if (dMax > clipLimit || dMin < -clipLimit)
  {
    logit("et", "trace from <%s.%s.%s.%s> may be clipped\n", pSta->sta,
          pComp->name, pSta->net, pComp->loc);
    pComp->BadBitmap |= GM_BAD_CLIP;
  }
  
  /* Now remove the mean, and set points inside gaps to zero */
  i = 0;
  do
  {
    if (pGap == (GAP *)NULL)
      iEnd = pTrace->nRaw;
    else
      iEnd = pGap->firstSamp - 1;

    for (; i < iEnd; i++)
      pTrace->rawData[i] -= mean;

    if (pGap != (GAP *)NULL)     /* Fill in the gap with zeros */    
    {
      for ( ;i < pGap->lastSamp + 1; i++)
        pTrace->rawData[i] = 0.0;
      pGap = pGap->next;
    }
  } while (i < pTrace->nRaw );

  /* See how much noise is in the pre-P-arrival data.       *
   * We hope and pray it isn't the coda of a previous event */
  preP = postP = 0.0;
  iEnd = (long)( 0.5 + (pSta->p_est - pTrace->starttime) / 
                 pTrace->delta );
  for (i = 0; i < iEnd; i++)
    preP += pTrace->rawData[i] * pTrace->rawData[i];

  if (iEnd > 0)
    preP /= (double)iEnd;
  
  iStart = (long)( 0.5 + (pComp->peakWinStart - pTrace->starttime) / 
                   pTrace->delta );
  if (iStart < 0 )
    iStart = 0;
  iEnd = (long)( 0.5 + (pComp->peakWinEnd - pTrace->starttime ) / 
                 pTrace->delta);
  if (iEnd > pTrace->nRaw)
    iEnd = pTrace->nRaw;
  for (i = iStart; i < iEnd; i++)
    postP += pTrace->rawData[i] * pTrace->rawData[i];

  if (iEnd > iStart)
    postP /= (double)(iEnd - iStart);
  
  if (postP < preP * pgmParams->snrThresh)
  {
    logit("et", "prepTrace: <%s.%s.%s.%s> event signal (%5g) too small for pre-event threshold (%5g)\n",
          pSta->sta, pComp->name, pSta->net, pComp->loc, postP, 
          pgmParams->snrThresh * preP);
    pComp->BadBitmap |= GM_LOW_SNR;
  }

  return;
}

/* getPeakGM: Find the maximum absolute values of acceleration, velocity,
 *            displacement and Spectra Response from the processed data.
 *            This function looks at the data within the "peak search"
 *            window, defined by peakWinStart and peakWinEnd in the COMP3
 *            structure, for the peak acceleration, velocity and displacement.
 *            It looks from peakWinStart to the end of the padded data for
 *            the peak Spectral Response values.
 *            The peak values are used to fill in a SM_INFO structure
 *            which is returned to be used by the caller.
 */
void getPeakGM(DATABUF *pTrace, COMP3 *pComp, STA *pSta, GMPARAMS *pgmParams,
               EVENT *pEvt, SM_INFO *pSms)
{
  double *acc, *vel, *disp, *psa;
  double mAcc, mVel, mDisp, mPsa;
  long iAcc, iVel, iDisp, iPsa;
  long i, iStart, iEnd;  /* start and end of peak-search window */
  int isp;
  
  memset(pSms, 0, sizeof(SM_INFO));
  strcpy(pSms->sta, pSta->sta);
  strcpy(pSms->comp, pComp->name);
  strcpy(pSms->net, pSta->net);
  strcpy(pSms->loc, pComp->loc);
  /* pSms->loc[0] = '\0'; */
  strcpy(pSms->qid, pEvt->eventId);
  strcpy(pSms->qauthor, pEvt->authorId);
  
  pSms->talt = 0.0;
  pSms->altcode = SM_ALTCODE_NONE;
  pSms->tload = -1.0;
  
  /* Search for the extrema in acc, vel, disp */
  if ( pEvt->eventId[0]==0 ) {
  	pComp->peakWinStart = pTrace->starttime;
  	pComp->peakWinEnd = pComp->peakWinStart + pgmParams->alarmDuration;
  }
  iStart = (long)( 0.5 + (pComp->peakWinStart - pTrace->starttime) / 
				   pTrace->delta );
  if (iStart < 0 )
	iStart = 0;
  iEnd = (long)( 0.5 + (pComp->peakWinEnd - pTrace->starttime ) / 
				 pTrace->delta);
  if (iEnd > pTrace->nProc)
	iEnd = pTrace->nProc;
  
  if (pgmParams->debug & GM_DBG_TIME)
  {
    logit("e", "trace start: %10.4lf end: %10.4lf (%ld)\n", 
          pTrace->starttime - pEvt->origin_time,
          pTrace->endtime - pEvt->origin_time, pTrace->nProc);
    logit("e", "search start: %10.4lf (%ld) end: %10.4lf (%ld)\n", 
          pComp->peakWinStart - pEvt->origin_time, iStart, 
          pComp->peakWinEnd - pEvt->origin_time, iEnd);
  }

  acc = pTrace->procData;
  vel = &pTrace->procData[pTrace->lenProc];
  disp = &pTrace->procData[pTrace->lenProc * 2];
  
  mAcc = mVel = mDisp = 0.0;
  iAcc = iVel = iDisp = -1;
  for (i = iStart; i < iEnd; i++)
  {
    if (fabs(acc[i]) > mAcc)
    {
      mAcc = fabs(acc[i]);
      iAcc = i;
    }
    if (fabs(vel[i]) > mVel)
    {
      mVel = fabs(vel[i]);
      iVel = i;
    }
    if (fabs(disp[i]) > mDisp)
    {
      mDisp = fabs(disp[i]);
      iDisp = i;
    }
  }
  pSms->pga = mAcc;
  pSms->tpga = pTrace->starttime + pTrace->delta * iAcc;
  pSms->t = pSms->tpga;
  
  pSms->pgv = mVel;
  pSms->tpgv = pTrace->starttime + pTrace->delta * iVel;
  if (pSms->tpgv < pSms->t) pSms->t = pSms->tpgv;
  
  pSms->pgd = mDisp;
  pSms->tpgd = pTrace->starttime + pTrace->delta * iDisp;
  if (pSms->tpgd < pSms->t) pSms->t = pSms->tpgd;
  
  /* Search for peak Spectral Response values.             *
   * This section is hard-coded to use the first           *
   * spectDamp value, which is 0.05 as needed by ShakeMap. */ 
  iStart = (long)( 0.5 + (pComp->peakWinStart - pTrace->starttime) / 
                   pTrace->delta );
  if (iStart < 0 )
    iStart = 0;
  /* Search all the way out through the padded data; *
   * This is needed since the peak Spectral Response *
   * may occur  after the forcing is completed.      */
  iEnd = pTrace->nProc + pTrace->padLen;
  
  for (isp = 0; isp < NSP; isp++)
  {
    mPsa = 0.0;
    psa = &pTrace->procData[pTrace->lenProc * (3 + isp)];
    for (i = iStart; i < iEnd; i++)
    {
      if (mPsa < fabs(psa[i])) 
      {
        mPsa = fabs(psa[i]);
        iPsa = i;
      }
    }
    pSms->rsa[isp] = mPsa;
    pSms->pdrsa[isp] = spectPer[isp];
    /* Record the time in case we're saving results in SAC files */
    pComp->RSAPeakTime[isp] = pTrace->starttime + pTrace->delta * iPsa;
  }
  pSms->nrsa = NSP;
  
  /* Write some XML */
  if (pgmParams->XMLDir && strlen(pgmParams->XMLDir) > 0)
    next_XML_SCNL( pgmParams, pSta, pSms );

  return;
}


/*
 * cleanTrace: initialize the global trace buffer, freeing old GAP structures.
 *    Returns: nothing
 */
void cleanTrace( DATABUF *pTrace )
{
  GAP *pGap;
  
  pTrace->nRaw = 0L;
  pTrace->nProc = 0L;
  pTrace->delta = 0.0;
  pTrace->starttime = 0.0;
  pTrace->endtime = 0.0;
  pTrace->nGaps = 0;
  
  /* Clear out the gap list */
  pTrace->nGaps = 0;
  while ( (pGap = pTrace->gapList) != (GAP *)NULL)
  {
    pTrace->gapList = pGap->next;
    free(pGap);
  }
  return;
}

/*
 * initBufs: allocate the three arrays needed for handling trace data
 *           The argument reqLen is the reqeusted length; this value
 *           is used for the raw data array. That value or larger, 
 *           if needed to find a multiple of the FFT factors, is used
 *           for the size of the processed data array. The work array
 *           is sized as needed by the gma routine.
 *           This routine is intended to be called at startup instead
 *           of later in the process life, to minimize memory growth
 *           during process lifetime.
 *   Returns: 0 on success
 *           -1 when out of memory
 */
int initBufs( long reqLen )
{
  long nfft;
  FACT *pF;
  
  /* Array for input data */
  if ( (gTrace.rawData = (double *)malloc(reqLen * sizeof(double))) ==
       (double*)NULL)
  {
    logit("et", "initBuffs: out of memory for rawData\n");
    return -1;
  }
  gTrace.lenRaw = reqLen;
  
  if ( (nfft = prepFFT( reqLen, &pF)) < 0)
  {
    logit("et", "initBuffs: out of memory for FFT factors\n");
    return -1;
  }
  
  /* Number of processed data arrays: 3 for acc, vel and disp, plus *
   * the ones needed for Spectral Response */
  gTrace.numProc = 3 + NSD * NSP;
  if ( (gTrace.procData = 
        (double *)malloc(gTrace.numProc * (nfft + FFT_EXTRA) * 
                         sizeof(double))) == (double*)NULL)
  {
    logit("et", "initBuffs: out of memory for procData\n");
    return -1;
  }
  gTrace.lenProc = nfft + FFT_EXTRA;
  
  /* Work array for frequency response functions in gma:  *
   * 3 for acc, vel, disp, plus one for Spectral Response */
  if ( (gWork = (double *)malloc(4 * (nfft + 2) * 
                                 sizeof(double))) == (double*)NULL)
  {
    logit("et", "initBuffs: out of memory for gWork\n");
    return -1;
  }

  /* Work array for Temperton FFTs */
  if ( (gWorkFFT = 
        (double *)malloc(gTrace.numProc * (nfft + FFT_EXTRA) * 
                         sizeof(double))) == (double*)NULL)
  {
    logit("et", "initBuffs: out of memory for gWorkFFT\n");
    return -1;
  }

  return 0;
}


/*************************************************************
 *                   CompareSCNLPARs()                       *
 *                                                           *
 *  This function is passed to qsort() and bsearch().        *
 *************************************************************/
int CompareSCNLPARs( const void *s1, const void *s2 )
{
   int rc;
   SCNLPAR *t1 = (SCNLPAR *) s1;
   SCNLPAR *t2 = (SCNLPAR *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->comp, t2->comp);
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->net,  t2->net );
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->loc,  t2->loc );
   return(rc);
}

/*************************************************************
 *                   CompareStaDist()                        *
 *                                                           *
 *  This function can be passed to qsort() and bsearch().    *
 *  Compare STA distances (from epicenter); closest first.   *
 *************************************************************/
int CompareStaDist( const void *s1, const void *s2 )
{
   STA *t1 = (STA *) s1;
   STA *t2 = (STA *) s2;

   if (t1->dist < t2->dist)
     return -1;
   else if (t1->dist > t2->dist)
     return 1;

   return 0;
}


/****************** Locally Used Functions Below Here *********************/

/*************************************************************
 *                   CompareDoubles()                        *
 *                                                           *
 *  This function can be passed to qsort() and bsearch().    *
 *  Compare two doubles for ordering in increasing order     *
 *************************************************************/
int CompareDoubles( const void *s1, const void *s2 )
{
  double *d1 = (double *)s1;
  double *d2 = (double *)s2;

  if (*d1 < *d2)
     return -1;
  else if (*d1 > *d2)
    return 1;
  
  return 0;
}

/****************** Locally Used Functions Below Here *********************/

/*
 * GetRespPZ: get response function (as poles and zeros) for the
 *   instrument sta, comp, net from the location specified in GMPARAMS.
 *   Response is filed in static responseStruct gScnPZ for later use
 *   by makeGM().
 *  Resturns: 0 on success
 *           -1 on fatal errors
 *           +1 on non-fatal errors
 */
static int getRespPZ(char *sta, char *comp, char *net, char *loc, GMPARAMS *pgmParams, 
              EVENT *pEvt)
{
  char respfile[PATH_MAX];
  int len, rc;
  
  /* Make sure the respone structure doesn't have any old memory allocated */
  cleanPZ(&gScnPZ);
  memset(respfile, 0, PATH_MAX);
  
  switch(pgmParams->respSource)
  {
#ifdef EWDB
  case GM_RS_EWDB:
    rc = getRespEWDB(sta, comp, net, &gScnPZ);
    return rc;
    break;
#endif
  case GM_RS_FILE:  /* fill in directory; rest of action is after switch() */
    strcpy(respfile, pgmParams->respDir);
    break;
  default:
    logit("et", "getRespPZ: unknown response source <%d>\n", 
          pgmParams->respSource);
    return -1;
  }
  
  /* Continue preparing respfile for PZ access: */
  len = strlen(respfile);
  if (respfile[len - 1] != '/' || respfile[len - 1] != '\\')
  {
    strcat(respfile, "/");
    len++;
  }
  if (fmtFilename(sta, comp, net, loc, respfile, PATH_MAX - len, 
                  pgmParams->respNameFormat) < 0)
  {
    logit("et", "getRespPZ: error formating respfile <%s>\n", respfile);
    return -1;
  }
  if ( (rc = readPZ(respfile, &gScnPZ)) < 0)
  {
    switch(rc)
    {
    case -1:
      logit("et", "getRespPZ: out of memory building pole-zero struct\n");
      break;
    case -2:
      logit("et", "getRespPZ: error parsing pole-zero file <%s> for <%s.%s.%s.%s>\n", 
            respfile, sta, comp, net, loc);
      return +1;  /* Maybe the other resp files will work */
      break;
    case -3:
      logit("et", "getRespPZ: invalid arguments passed to readPZ\n");
      break;
    case -4:
      logit("et", "getRespPZ: error opening pole-zero file <%s> for <%s.%s.%s.%s>\n", 
            respfile, sta, comp, net, loc);
      return +1;  /* Maybe the other resp files will work */
      break;
    default:
      logit("et", "getRespPZ: unknown error <%d> from readPZ(%s)\n", rc,
            respfile);
    }
    return -1;
  }
  /* all done! */
  return 0;
}

/*
 * endEvent: Terminate the event and clean up. Calls cleanup routines
 *           for those data sinks that need it, then cleans up the
 *           EVENT structure in preparation for the next event.
 */
void endEvent(EVENT *pEvt, GMPARAMS *pgmParams)
{
  int i;
  COMP1 *pC1;

  /* Call `endEvent' for those data sinks that may need it: EWDB? */
  
  /* Clean up the EVENT structure in preparation for next event */
  for (i = 0; i < pgmParams->maxSta; i++)
  {
    while ( (pC1 = pEvt->Sta[i].comp) != (COMP1 *)NULL)
    {
      pEvt->Sta[i].comp = pC1->next;
      free(pC1);
    }
  }
  pEvt->numSta = 0;
  memset(pEvt->Sta, 0, pgmParams->maxSta * sizeof(STA));

  return;
}

/*
 * procArc: parse the summary line from a hyp2000 archive message into
 *          the EVENT structure
 *   Returns: 0 on success
 *           -1 on parse errors
 */
int procArc( char *msg, EVENT *pEvt, MSG_LOGO iLogo, MSG_LOGO fLogo)
{
  struct Hsum Sum;          /* Hyp2000 summary data    */
  char shdw = '\0';   /* to store a fake shadow card   */

  /* Process the hypocenter card (1st line of msg) */
  if ( read_hyp( msg, &shdw, &Sum ) < 0 )
  {  /* Should we send an error message? */
    logit( "t", "procArc: error parsing HYPO summary message\n" );
    return( -1 );
  }
  sprintf(pEvt->eventId, "%lu", Sum.qid);
  sprintf(pEvt->authorId, "%03u%03u%03u:%03u%03u%03u", iLogo.type, iLogo.mod,
          iLogo.instid, fLogo.type, fLogo.mod, fLogo.instid);
  pEvt->lon = (double)Sum.lon;
  pEvt->lat = (double)Sum.lat;
  pEvt->depth = (double)Sum.z;
  pEvt->origin_time = Sum.ot - GSEC1970; /* 1600 to 1970, in seconds */

  return 0;
}

/*
 * addSRaddSR2list: add a new spectra response period to the list of those to be
 *           processed
 *   Returns: 0 on success
 *            1 if maximum size of list exceeded
 */
int addSR2list( double newSR ) {
	if ( NSP == SM_MAX_RSA ) {
		logit( "w", "Maximum of %d spectral periods allowed; ignoring '%lf'\n", 
			SM_MAX_RSA, newSR );
		return 1;
	}
	spectPer[NSP++] = newSR;
	return 0;
}


/*
 * checkSRlist: if list of spectra responses to be processed is empty, 
 *           fill w/ default list
 *   Returns: 0 on success
 *            1 if maximum size of list exceeded
 */
void checkSRlist() {
	if ( NSP == 0 ) {
		spectPer[NSP++] = 0.3;
		spectPer[NSP++] = 1.0;
		spectPer[NSP++] = 3.0;
	}
}
