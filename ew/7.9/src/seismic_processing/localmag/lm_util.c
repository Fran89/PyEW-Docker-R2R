/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_util.c 5736 2013-08-07 13:48:03Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.20  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.19  2005/08/23 01:56:28  friberg
 *     removed debugging statements and changed z2pThresh pre-event noise calculation
 *
 *     Revision 1.18  2005/08/15 20:58:04  friberg
 *     minor fixes and debugging statements for 2.0.4
 *
 *     Revision 1.17  2005/08/15 15:30:54  friberg
 *     version 2.0.3, added in notUsed flag to PCOMP1 to indicate that the
 *     channels from this component set were not used. This can only
 *     happen currently because of the require2Horizontals configuration
 *     parameter.
 *
 *     Revision 1.16  2005/08/08 18:47:19  friberg
 *     fixed require2Horizontals issue
 *
 *     Revision 1.15  2005/08/08 18:38:14  friberg
 *
 *     Fixed bug from last version that had station corrections added in twice.
 *     Added in new directive require2Horizontals to require 2 components for a station Ml
 *     	example:   require2Horizontals 1
 *     	Note needs the 1 after the directive to be used
 *     Added in new directive useMedian
 *     	example:   useMedian
 *     	Note for this one, no flag is needed after the directive
 *     Also updated the Doc files.
 *
 *     Revision 1.14  2002/10/29 18:48:11  lucky
 *     Added origin version number tracking.. this is how we associate mags to
 *     origins.
 *
 *     Revision 1.13  2002/03/17 18:32:49  lombard
 *     Changed the way trace times and search times are set. Unchanged is the
 *       trace start: a specified number of seconds before the first P arrival
 *       from the layered velocity model.
 *       Trace end is now a specified number of seconds after the Sg arrival
 *       computed using a specified Sg speed instead of the layered model.
 *       Taper times are added to each end of the trace to get the 10% taper
 *       length as before.
 *       Search start is now a fixed number of seconds before the selected
 *       search start phase - either first P or first S arrival from the
 *       layered model.
 *       Search end is now a specified number of seconds after the Sg arrival
 *       computed using a specified Sg speed.
 *       The Sg arrival is determined using hypocentral distance, so event dept
 *     must be passed to traceEndTime() and searchEndTime().
 *       To determine how long localmag should wait for traces to arrive at
 *       the wave_server before processing a realtime event, a new function
 *       setMaxDelay computes the tarce end time for a hypothetical station
 *       at maxDist from an event on the surface. The optional extraWait is
 *       added to this time; this function is called during startup if
 *       localmag is running as an earthworm module connected to transport.
 *     Changed the way channels are selected for use in the station magnitude:
 *       before, both the E and N channels had to have peak amps before they
 *       could be used, although single channels were included in the Mag message.
 *       Now, any channel with an amplitude is counted in the station average.
 *
 *     Revision 1.12  2002/02/04 17:02:11  lombard
 *     Added checks for short traces (such as might come from wave_server
 *     for a new event).
 *
 *     Revision 1.11  2002/01/24 19:34:09  lombard
 *     Added 5 percent cosine taper in time domain to both ends
 *     of trace data. This is to eliminate `wrap-around' spikes from
 *     the pre-event noise-check window.
 *
 *     Revision 1.10  2002/01/15 21:23:03  lucky
 *     *** empty log message ***
 *
 *     Revision 1.9  2001/06/21 21:22:22  lucky
 *     Modified the code to support the new amp pick format: there can be one
 *     or two picks, each consisting of time, amplitude, and period. This is
 *     reflected in the new TYPE_MAGNITUDE message, as well as the SAC header
 *     fields that get filled in.
 *     Also, the labels for SAC fields were shortened to comply with the K_LEN
 *     limitation from sachead.h
 *
 *     Revision 1.8  2001/06/10 21:25:13  lombard
 *     Fixed memory leak in endEvent.
 *
 *     Revision 1.7  2001/04/11 21:07:08  lombard
 *     "site.?" renamed to "lm_site.?" for clarity.
 *
 *     Revision 1.6  2001/03/30 22:56:54  lombard
 *     Added cleanup of STA array after event.
 *
 *     Revision 1.5  2001/03/01 05:25:44  lombard
 *     changed FFT package to fft99; fixed bugs in handling of SCNPars;
 *     changed output to Magnitude message using rw_mag.c
 *
 *     Revision 1.4  2001/01/15 03:55:55  lombard
 *     bug fixes, change of main loop, addition of stacker thread;
 *     moved fft_prep, transfer and sing to libsrc/util.
 *
 *     Revision 1.3  2000/12/31 17:27:25  lombard
 *     More bug fixes and cleanup.
 *
 *     Revision 1.2  2000/12/25 22:14:39  lombard
 *     bug fixes and development
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_util.c: A bunch of utility functions used for local_mag.
 *    Pete Lombard; Sept, 2000
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <earthworm.h>
#include <chron3.h>
#include <fft_prep.h>
#include <kom.h>
#include <read_arc.h>
#include <time_ew.h>
#include <tlay.h>
#include <transfer.h>
#include "lm.h"
#include "lm_config.h"
#ifdef EWDB
#include "lm_ewdb.h"
#endif
#include "lm_misc.h"
#include "lm_sac.h"
#include "lm_util.h"
#ifdef UW
#include "lm_uw.h"
#endif
#include "lm_ws.h"
#include "lm_site.h"

/* global variables for lm_util.c: */
static DATABUF gTrace;    /* the trace buffer for raw and processed data  */
static double *gWork;     /* the work array for convertWave               */
static double *gWorkFFT;  /* the work array for fft99                     */
static ResponseStruct gScnlPZ;  /* Poles, zeros, and gain for the SCNL    */
static ResponseStruct gWaPZ;   /* Poles, zeros, and gain for synthetic WA */
static LOGA0 *gLa0tab;    /* Table of LogA0 values vs. distance           */

/* Internal function prototypes */
static int getArc(char *, EVENT *);
static void initSaveTrace(EVENT *, LMPARAMS *);
static void termSaveTrace(EVENT *, LMPARAMS *);
static LOGA0 *logA0( double, int );
static int CompareDoubles( const void *, const void * );
static int getRespPZ(char *, char *, char *, char *, LMPARAMS *, EVENT *);

/*
 * getEvent: selector function to call the appropriate procedure for
 *           reading in the event information.
 * PRELIMINARY: Needs to be fleshed out as far as passing in various
 *           parameters for the different types of event sources.
 *    Returns: 0 on success;
 *            -1 on failure
 */
int getEvent( EVENT *pEvt, LMPARAMS *plmParams )
{
  int rc;
  
  /* Get the initial event info from those sources that provide it      *
   * this way; others such as SAC provvide event info from trace files. */
  switch (plmParams->eventSource)
  {
  case LM_ES_ARCH:
    /* Read a hyp2000 archive message, from stdin or from file */
    /* For now, we will read arc msgs only from stdin */
    if ( (rc = getArc("", pEvt)) < 0)
      return rc;
    break;
#ifdef UW
  case LM_ES_UW:
    /* Read a UW-format pick file and get preliminary event info */
    if ( (rc = getUWpick(pEvt, plmParams)) < 0)
      return rc;
    break;
#endif
#ifdef EWDB
  case LM_ES_EWDB:
    if ( (rc = getDBEvent(pEvt, plmParams)) < 0)
      return rc; 
    break;
#endif    
    /* Insert other types of event sources here */
    /* Perhaps we can read event info from a group of SAC files *
     * or query the database with an event ID */
  case LM_ES_SAC:
    break;    /* nothing to do here; get event when reading SAC files */
  default:
    logit("", "getEvent: unknown event source <%d>\n", 
          plmParams->eventSource);
    return -1;
  }
  return 0;
}

/*
 * getAmpFromTrace: selector function to call the desired procedure
 *           for getting Wood-Anderson from a trace source such as wave_server,
 *           SAC file, etc.
 *    Returns: 0 on success
 *            -1 on fatal errors
 */
int getAmpFromTrace( EVENT *pEv, LMPARAMS *plmParams)
{
  int rc;
  
  if (plmParams->saveTrace != LM_ST_NO)
    initSaveTrace(pEv, plmParams);

  switch(plmParams->traceSource)
  {
  case LM_TS_WS:
    rc = getAmpFromWS( pEv, plmParams, &gTrace);
    break;
#ifdef EWDB
  case LM_TS_EWDB:
    rc = getAmpFromEWDB( pEv, plmParams, &gTrace);
    break;
#endif
  case LM_TS_SAC:
    rc = getAmpFromSAC( pEv, plmParams, &gTrace);
    break;
#ifdef UW
  case LM_TS_UW:
    rc = getAmpFromUW( pEv, plmParams, &gTrace);
    break;
#endif
  default:
    logit("", "localmag getAmpFromTrace: unknown source: %d\n",
          plmParams->traceSource);
    rc = -1;
  }

  if (plmParams->saveTrace != LM_ST_NO)
    termSaveTrace(pEv, plmParams);

  return rc;
}


/*
 * addCompToEvent: Add a new component to the EVENT structure.
 *     Checks that SCNL meets the selection criteria of the lists add and pDel,
 *     the component is not vertical (third character not `Z' (if verticalsAllowed is not set),
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
 *      Calls logit() to report errors and warnings.
 */
int addCompToEvent( char *sta, char *comp, char *net, char *loc, EVENT *pEvt, 
                    LMPARAMS *plmParams, STA **ppSta, COMP3 **ppComp)
{
  COMP1 *this, *last;
  PSCNLSEL thisSel;
  SCNLPAR keySCNL;
  char orientation;
  double dist, lat, lon;
  int i, dir;
  int foundIt = 0, new = 0;
  int addStation = 0;
  int neverMatch = 1;
  int OtherPhaseThanP = 0;
  int jj;
  int selected = 0;
  int mSta, mComp, mNet, mLoc;   /* `match' flags */
  float maxPwt, Pwt_threshold;
  
  /* Is this SCNL on the selection "Add" list? If Add list is empty, assume *
   * that all stations are wanted, but never vertical components.          */
  if ( ((thisSel = plmParams->pAdd) == (PSCNLSEL)NULL) &&  
       ((comp[2] != 'Z' && !plmParams->allowVerticals) || 
        (plmParams->allowVerticals && comp[2] == 'Z'))
     )
  {
    selected = 1;
  }
  else
  {
    while (thisSel != (PSCNLSEL)NULL)
    {
      mSta = mComp = mNet = mLoc = 0;
      if (thisSel->sta[0] == '*' || strcmp(thisSel->sta, sta) == 0) mSta = 1;
      if (thisSel->net[0] == '*' || strcmp(thisSel->net, net) == 0) mNet = 1;
      if ( (thisSel->comp[0] == '*' || memcmp(thisSel->comp, comp, strlen(thisSel->comp)) == 0))
        mComp = 1;
      if (plmParams->allowVerticals==0 && comp[2] == 'Z') mComp = 0; /* remove Z if allowVerticals is not set */
      if (thisSel->loc[0] == '*' || strcmp(thisSel->loc, loc) == 0) mLoc = 1;
      if ( (selected = mSta && mNet && mComp && mLoc) == 1)
      {
        logit("", "addCompToEvent: <%s.%s.%s.%s> in select list: (%d.%d.%d.%d) allowVerticals = %d\n",
            sta, comp, net, loc, mSta, mComp, mNet, mLoc, plmParams->allowVerticals);
        break;
      }
      
      thisSel = thisSel->next;
    }
  }
  if (!selected)
  {
    if (plmParams->debug & LM_DBG_SEL)
      logit("", "addCompToEvent: <%s.%s.%s.%s> not in select list: (%d.%d.%d.%d)\n",
            sta, comp, net, loc, mSta, mComp, mNet, mLoc);
    return +2;
  }

  /* Is this SCNL on the selection "Del" list? */
  thisSel = plmParams->pDel;
  while (thisSel != (PSCNLSEL)NULL)
  {
    mSta = mComp = mNet = mLoc = 0;
    if (thisSel->sta[0] == '*' || strcmp(thisSel->sta, sta) == 0) mSta = 1;
    if (thisSel->net[0] == '*' || strcmp(thisSel->net, net) == 0) mNet = 1;
    if (thisSel->comp[0] == '*' || 
        memcmp(thisSel->comp, comp, strlen(thisSel->comp)) == 0) mComp = 1;
    if (thisSel->loc[0] == '*' || 
        memcmp(thisSel->loc, loc, strlen(thisSel->loc)) == 0) mLoc = 1;
    if ( (mSta && mNet && mComp && mLoc) == 1)
    {
      if (plmParams->debug & LM_DBG_SEL)
        logit("", "addCompToEvent: <%s.%s.%s.%s> in delete list: (%d.%d.%d.%d)\n",
              sta, comp, net, loc, mSta, mComp, mNet, mLoc);
      return +2;
    }
    thisSel = thisSel->next;
  }

  /* Is this SCNL on the SkipStationsNotInArc list? */
  if( plmParams->SkipStationsNotInArc ) {
    /*
     * Consider only the stations with P phase within the ARC message, and the
     * weight of phase assigned by Hypoinverse.
     *
     * Exclude from the magnitude computation the stations with a weight less
     * than 10% of the maximum weight among the all phases within the ARC message.
     *
     */

    /* Computation of max P weight */
    maxPwt = -1000000000.0;
    for(jj=0; jj < pEvt->numArcPck; jj++ ) {
      if(pEvt->ArcPck[jj].Ponset == 'P') {
	if(maxPwt < pEvt->ArcPck[jj].Pwt) {
	  maxPwt = pEvt->ArcPck[jj].Pwt;
	}
      }
    }

    /* Set P weight threshold to plmParams->MinWeightPercent per cent of maxPwt */
    Pwt_threshold = maxPwt * (plmParams->MinWeightPercent / 100.0);

    /* Set some flags */
    OtherPhaseThanP = 0;
    neverMatch = 1;
    addStation = 0;
    jj = 0;
    while( jj < pEvt->numArcPck && addStation == 0) {

      /* Check if the current ArcPck 'site.net' are equal to 'sta.net' of the current SCNL */
      if(strcmp(sta, pEvt->ArcPck[jj].site) == 0 && strcmp(net, pEvt->ArcPck[jj].net) == 0) {
	neverMatch = 0;

	/* Check if the current ArcPck item is a P phase */
	if(pEvt->ArcPck[jj].Ponset == 'P') {
	  /* Add only if the Pwt is greater or equal to the Pwt_threshold */
	  if(pEvt->ArcPck[jj].Pwt >= Pwt_threshold) {
	    addStation = 1;
	  } else {
	    logit("", "addCompToEvent: station <%s.%s.%s.%s> skipped because P weight %.2f is less than threshold %.2f. (SkipStationsNotInArc)\n",
		sta, comp, net, loc, pEvt->ArcPck[jj].Pwt, Pwt_threshold);
	  }
	}

	/* Check if the current ArcPck item is not a P phase */
	else {
	    OtherPhaseThanP = 1;
	}

      }

      jj++;
    }

    if(addStation) {

      logit("", "addCompToEvent: station <%s.%s.%s.%s> added. (SkipStationsNotInArc)\n",
	  sta, comp, net, loc);

    } else {

      /* Other log messages should be already print out */
      if(neverMatch) {
	logit("", "addCompToEvent: station <%s.%s.%s.%s> skipped because there is not any phases. (SkipStationsNotInArc)\n",
	    sta, comp, net, loc);
      } else {
	  if(OtherPhaseThanP) {
	      logit("", "addCompToEvent: station <%s.%s.%s.%s> skipped because the phase found is not P. (SkipStationsNotInArc)\n",
		      sta, comp, net, loc);
	  }
      }

      /* If it does not match SkipStationsNotInArc rules then return */
      return +2;
    }

  }

  orientation = comp[2]; /* the 3rd char in the seed channel name holds orientation info ZNE, 123 etc */

  /* try map a number to a channel string */
  if (strlen(plmParams->ChannelNumberMap) > 1) 
  {
    int num;
    num = atoi(&comp[2]);
    if (num >= 1 && num <= 3) 
    {
      orientation = plmParams->ChannelNumberMap[num];
      logit("", "addCompToEvent: mapping orientation number %d to orientation %c SCNL now <%s.%s.%s.%s>\n",
          num, orientation, sta, comp, net, loc);
    }
  }
  
  /* What direction is this component? We do this here, since it could
   * possibly fail. We don't want to allocate a new COMP1 structure (below)
   * and then not use it because this test failed. */
  switch(orientation)
  {
  case 'E':
    dir = LM_E;
    break;
  case 'N':
    dir = LM_N;
    break;
  case 'Z':   /* This won't happen since we reject `Z' above */
    dir = LM_Z;
    break;
  default:
    logit("", "addCompToEvent: unknown component direction for <%s.%s.%s.%s>\n",
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
    /* Find its epicentral distance; make sure its in range */
    dist = getStaDist(sta, comp, net, loc, &lat, &lon, pEvt, plmParams);
    if ( dist < 0.0 )
      return +6;
    else if (dist > plmParams->maxDist)
    {
      if (plmParams->debug & LM_DBG_SEL)
        logit("", "<%s.%s.%s.%s> distance (%e) exceeds limit\n", sta, comp, net, loc,
              dist);
      return +5;
    }
    if (pEvt->numSta < plmParams->maxSta)
    {
      strncpy(pEvt->Sta[i].sta, sta, TRACE_STA_LEN);
      strncpy(pEvt->Sta[i].net, net, TRACE_NET_LEN);
      pEvt->Sta[i].lat = lat;
      pEvt->Sta[i].lon = lon;
      pEvt->Sta[i].dist = dist;
      pEvt->numSta++;
    }
    else
    {
      logit("", "addCompToEvent: station limit reached; skipping <%s.%s.%s.%s>\n",
            sta, comp, net, loc);
      pEvt->Sta[i].dist = -1.0;
      pEvt->Sta[i].mag = NO_MAG;
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
      logit("", "addCompToEvent: out of memory for COMP1\n");
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
      if (memcmp(this->n2, comp, 2) == 0 && memcmp(this->loc, loc, 2) == 0)
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
        /* Fill in the new COMP1 loc */
    this->loc[0] = loc[0]; 
    this->loc[1] = loc[1];
  }
  else if (this->c3[dir].name[0] != 0 && 
	(this->c3[dir].p2pMinTime != -1.0 ||  /* it was successfully processed before */
	(this->c3[dir].BadBitmap & (LM_BAD_CLIP | LM_BAD_Z2P)) ) )  /* OR it had bad unrecoverable values */
  {  /* This component/direction is a duplicate AND had an amp retrieved for it before!*/
    if (plmParams->debug & LM_DBG_SEL)
      logit("", "addCompToEvent: <%s.%s.%s.%s> rejected as a duplicate, because already processed and/or possibly bad.\n",
            sta, comp, net, loc);
    return +1;
  }

  /* This component/direction is new for this event -OR- it was seen before, but no amp was found possibly due to missing
	data at a WaveServer source. The duplicate check above was modified to also check if an amp was found or
	not by simply testing  the BadBitmap for the cases CLIP'ed or Z2P threshold not met.
  */
  strncpy(this->c3[dir].name, comp, 3);
  strncpy(this->c3[dir].loc, loc, 2);
  this->c3[dir].p2pMinTime = -1.0;  /* Set some `null' values */
  this->c3[dir].p2pMaxTime = -1.0;
  this->c3[dir].z2pTime = -1.0;
  this->c3[dir].BadBitmap = 0;
  *ppComp = &this->c3[dir];

  /* Search for an SCNLPAR structure for this SCNL */
  if (plmParams->numSCNLPar > 0)
  {
    memset(&keySCNL, 0, sizeof(SCNLPAR));
    strcpy(keySCNL.sta, sta);
    strcpy(keySCNL.comp, comp);
    strcpy(keySCNL.net, net);
    strcpy(keySCNL.loc, loc);
    this->c3[dir].pSCNLPar = (SCNLPAR *)bsearch(&keySCNL, plmParams->pSCNLPar,
                                              plmParams->numSCNLPar,
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
                   double *pLon, EVENT *pEvt, LMPARAMS *plmParams)
{
  double r = -1.0;           /* Epicentral distance */
  SITE *pSite;               /* Site table entry */
  int rc;
  
  switch(plmParams->staLoc)
  {
  case LM_SL_HYP:
    if ( (pSite = find_site( sta, comp, net, loc)) == 
         (SITE *)NULL)
    {
      logit("", "getStaDist: <%s.%s.%s.%s> - Not in station list.\n", 
            sta, comp, net, loc);
      return r;
    }
    *pLat = pSite->lat;
    *pLon = pSite->lon;
    r = utmcal(pEvt->lat, pEvt->lon, pSite->lat, pSite->lon);
    break;
  case LM_SL_SAC:
    if ( (rc = getSACStaDist(&r, pLat, pLon)) == +1)
      r = utmcal(pEvt->lat, pEvt->lon, *pLat, *pLon);
    /* If getSACStaDist returns -1 (no loc or dist data), r is left at -1.0 */
    break;
#ifdef UW
  case LM_SL_UW:
    if ( (rc = getUWStaLoc(sta, net, pLat, pLon)) < 0)
      return -1.0;
    r = utmcal(pEvt->lat, pEvt->lon, *pLat, *pLon);
    break;
#endif
#ifdef EWDB
  case LM_SL_EWDB:
    /* Get station location from EWDB */
    break;
#endif
  }

  return r;
}



/*
 * getWAResp: fill in response structure with Wood-Anderson response
 *            information.
 * Default values are hard-wired, but we allow the user to set values by
 * passing in a WA_PARAMS structure with the desired values.
 *
 *  Arguments: pWA: pointer to a WA_PARAMS structure with the desired
 *                 instrument parameters. If pWA is NULL, then the default
 *                 values (see below) are used.
 *  returns: 0 on success
 *          -1 on failure (error allocating memory; not logged)
 */
int getWAResp( WA_PARAMS *pWA)
{
  double per, damp;
  double omega, r;
  
  /*
   * The Wood-Anderson is an optical-mechanical seismometer. The standard
   * parameters for its transfer function are:
   * period: 0.8 seconds; damping 0.8 critical; gain: 2800
   * However, testing by Uhrhammer & Collins (BSSA 1990, V80 p702-716)
   * indicates better values are:
   * period 0.8 seconds; damping 0.7 critical; gain 2080
   * We provide the standard values as the default, but the user can 
   * override these defaults.
   */

  gWaPZ.iNumZeros = 2;
  gWaPZ.iNumPoles = 2;
  if ( (gWaPZ.Zeros = (PZNum *)malloc(2 * sizeof(PZNum))) == (PZNum *)0 ||
       (gWaPZ.Poles = (PZNum *)malloc(2 * sizeof(PZNum))) == (PZNum *)0 )
    return -1;
  
  gWaPZ.Zeros[0].dReal = gWaPZ.Zeros[0].dImag = 0.0;
  gWaPZ.Zeros[1].dReal = gWaPZ.Zeros[1].dImag = 0.0;
  
  if (pWA != (WA_PARAMS *)NULL)
  {   /* Set non-standard Wood-Anderson coefficients */
    per = pWA->period;
    damp = pWA->damp;
    gWaPZ.dGain = pWA->gain;
  }
  else
  {   /* Set the default Wood-Anderson coefficients */
    per = 0.8;
    damp = 0.8;
    gWaPZ.dGain = 2800.0;
  }
  
  /* Change from nanometers of ground motion to millimeters */
  gWaPZ.dGain *= 1.0e-6;

  omega = 2.0 * PI / per;
  r = sqrt(1.0 - damp * damp);
  gWaPZ.Poles[0].dReal = - omega * damp;
  gWaPZ.Poles[0].dImag = omega * r;
  gWaPZ.Poles[1].dReal = - omega * damp;
  gWaPZ.Poles[1].dImag = - omega * r;
  
  return 0;
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
    logit("", "Est P: %10.4lf", treg[0].t);
    if (nreg == 2)
      logit("", "\n");
    else
      logit("", "  (%10.4lf)\n", treg[2].t);
    logit("", "Est S: %10.4lf", treg[1].t);
    if (nreg == 2)
      logit("", "\n");
    else
      logit("", "  (%10.4lf)\n", treg[3].t);
  }

  pSta->p_est += pEvt->origin_time;
  pSta->s_est += pEvt->origin_time;
  
  return;
}

/*
 * set the lengths of the pre- and post- tapers for the time series
 */
void setTaperTime(STA *pSta, LMPARAMS *plmParams, EVENT *pEvt)
{
  double totalTaper;
  double traceLen;
  
  traceLen = traceEndTime(pSta, plmParams, pEvt) - 
    traceStartTime(pSta, plmParams);
  totalTaper = TD_TAPER * traceLen / (1.0 - TD_TAPER);
  pSta->timeTaper = 0.5 * totalTaper;
  return;
}


/*
 * Return the desired start time for traces from pSta.
 */
double traceStartTime(STA *pSta, LMPARAMS *plmParams)
{
  return pSta->p_est - plmParams->traceStart;
}


/*
 * Return the desired end time for traces from pSta.
 */
double traceEndTime(STA *pSta, LMPARAMS *plmParams, EVENT *pEvt)
{
  double end, dist;
  
  dist = sqrt(pSta->dist * pSta->dist + pEvt->depth * pEvt->depth);
  end = pEvt->origin_time + plmParams->traceEnd + 
    dist / plmParams->SgSpeed;
  return end;
}

/*
 * Return the desired peak-search start time for traces from pSta.
 */
double peakSearchStart(STA *pSta, LMPARAMS *plmParams)
{
  double start;
  
  switch (plmParams->searchStartPhase) {
  case LM_SSP_P:
    start = pSta->p_est - plmParams->peakSearchStart;
    break;
  case LM_SSP_S:
    start = pSta->s_est - plmParams->peakSearchStart;
    break;
  default:  /* Should never happen; ReadConfig won't allow it */
    start = pSta->p_est - plmParams->peakSearchStart;
  }
  
  return start;
}


/*
 * Return the desired peak-search end time for traces from pSta.
 */
double peakSearchEnd(STA *pSta, LMPARAMS *plmParams, EVENT *pEvt)
{
  double end, dist;
  
  dist = sqrt(pSta->dist * pSta->dist + pEvt->depth * pEvt->depth);
  end = pEvt->origin_time + plmParams->peakSearchEnd + 
    dist / plmParams->SgSpeed;
  return end;
}

void setMaxDelay( LMPARAMS *plmParams, EVENT *pEvt)
{
  STA dummy;
  
  dummy.dist = plmParams->maxDist;
  pEvt->depth = 0.0;
  pEvt->origin_time = 0.0;
  
  if (plmParams->debug & LM_DBG_TIME)
    logit("", "phase estimates for max distance %lf:\n", plmParams->maxDist);
  EstPhaseArrivals(&dummy, pEvt, plmParams->debug & LM_DBG_TIME);
  setTaperTime(&dummy, plmParams, pEvt);
  
  plmParams->waitTime += traceEndTime(&dummy, plmParams, pEvt) + 
    dummy.timeTaper;

  if (plmParams->debug & LM_DBG_TIME) {
    logit("", "taper time: %.2lf trace start %.2lf end %.2lf\n",
          dummy.timeTaper, traceStartTime(&dummy, plmParams),
          traceEndTime(&dummy, plmParams, pEvt));
    logit("", "total wait time: %.2lf\n", plmParams->waitTime);
  }

  return;
}


/*
 * MakeWA: Transfer a raw trace into a synthetic wood-Anderson trace.
 *         The trace data is in the DATABUF trace, for the SCNL identified
 *         by STA and COMP. Instrument response (pole/zero/gain) is
 *         obtained from the specified response source.
 *   Returns: 0 on success
 *           +1 on non-fatal error (no response data for this SCNL)
 *           -1 on fatal errors  
 */
int makeWA(STA *pSta, COMP3 *pComp, LMPARAMS *plmParams, EVENT *pEvt)
{
  int i, rc;
  double taperFreqs[4] = {1.0, 1.0, 10.0, 10.0};
  long padlen, nfft;
  
  /* don't waste time with bad traces */
  if (pComp->BadBitmap)
    return 1;

  /* Get the instrument response */
  if ( (rc = getRespPZ(pSta->sta, pComp->name, pSta->net, pComp->loc, plmParams, 
                       pEvt)) != 0)
  {
    logit("", "makeWA: no response data for <%s.%s.%s.%s>; skipping\n",
          pSta->sta, pComp->name, pSta->net, pComp->loc);
    return rc;
  }

  if (pComp->pSCNLPar == (SCNLPAR *)NULL)
  {
    /* Set the high-frequency taper band to 90% and 100% of Nyquist */
    taperFreqs[3] = 0.5/gTrace.delta;
    taperFreqs[2] = 0.45/gTrace.delta;
  }
  else
    for (i = 0; i < 4; i++)
      taperFreqs[i] = pComp->pSCNLPar->fTaper[i];

  if (plmParams->debug & (LM_DBG_PZG | LM_DBG_TRS | LM_DBG_ARS))
    printf("\nResponse for <%s.%s.%s.%s>\n", pSta->sta, pComp->name, pSta->net, pComp->loc);

  padlen = -1;  /* let convertWave figure the padding */
  for (i=0; i< gTrace.lenProc; i++)
    gTrace.procData[i] = 0.0;
  rc = convertWave(gTrace.rawData, gTrace.nRaw, gTrace.delta, &gScnlPZ, 
                   &gWaPZ, taperFreqs, 0, &padlen, &nfft, gTrace.procData,
                   gTrace.lenProc, gWork, gWorkFFT);
  if (rc < 0)
  {
    switch(rc)
    {
    case -1:
      logit("", "convertWave failed: out of memory\n");
      return -1;
      break;
    case -3:
      logit("", "convertWave failed: invalid arguments\n");
      return -1;
      break;
    case -4:
      logit("", "convertWave failed: FFT error; nfft: %ld\n", nfft);
      return -1;
      break;
    default:
      logit("", "convertWave failed: unknown error %d\n", rc);
      return -1;
    }
  }
  /* Do we need to adjust the end of the processed trace? */
  if (nfft - padlen < gTrace.nRaw)
  {    /* convertWave had to chop some of the end */
    gTrace.endtime -= (gTrace.nRaw - (nfft - padlen)) * gTrace.delta;
    gTrace.nProc = nfft - padlen;
  }
  else
    gTrace.nProc = gTrace.nRaw;

  gTrace.padLen = padlen;
  
  if (plmParams->debug & LM_DBG_TRC)
  {
    double time;
    
    printf("\ninput and output trace data\n");
    for (i = 0; i < gTrace.nRaw; i++)
    {
      time = gTrace.delta * i + gTrace.starttime;
      printf("%5d  %12.4f  %12.5e   %12.5e\n", i, time, gTrace.rawData[i],
             gTrace.procData[i]);
    }
    for (; i < nfft; i++)
    {
      time = gTrace.delta * i + gTrace.starttime;
      printf("%5d  %12.4f                 %12.5e\n", i, time, 
             gTrace.procData[i]);
    }
  }
  
  return 0;
}

/*
 * prepTrace: prepare trace data for processing. Preps include: check for
 *    gaps in peak-search window, compute and remove mean, fill gaps,
 *    and check for clipping.
 *    Also fills in the peak-search start and end times.
 *    If putProc is set, then copy from rawData to procData since the raw
 *    has already been processed into W-A.
 */
void prepTrace(DATABUF *pTrace, STA *pSta, COMP3 *pComp, LMPARAMS *plmParams,
               EVENT *pEvt, int putProc)
{
  long i, iEnd, npoints;
  double mean, dMax, dMin, gapEnd, clipLimit;
  GAP *pGap;

  if (pComp->pSCNLPar != (SCNLPAR *)NULL)
    clipLimit = pComp->pSCNLPar->clipLimit;
  else
    clipLimit = 7.55e6; /* 90% of 2^23; clip for 24-bit digitizer */
  
  /* Fill in the peak-search window limits */
  pComp->peakWinStart = peakSearchStart(pSta, plmParams);
  pComp->peakWinEnd = peakSearchEnd(pSta, plmParams, pEvt);

  if (pComp->peakWinStart > pTrace->endtime) {
    logit("e", "trace <%s.%s.%s.%s> ends before search window starts; skipping\n",
          pSta->sta, pComp->name, pSta->net, pComp->loc);
    pComp->BadBitmap |= LM_BAD_SHORT;
  }
  else if (pComp->peakWinEnd > pTrace->endtime) {
    logit("", "trace <%s.%s.%s.%s> ends before search window ends\n",
          pSta->sta, pComp->name, pSta->net, pComp->loc);
  }
  

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
        logit("", "trace from <%s.%s.%s.%s> has gap in peak-search window\n",
              pSta->sta, pComp->name, pSta->net, pComp->loc);
        pComp->BadBitmap |= LM_BAD_GAP;
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
    logit("", "trace from <%s.%s.%s.%s> may be clipped\n", pSta->sta,
          pComp->name, pSta->net, pComp->loc);
    pComp->BadBitmap |= LM_BAD_CLIP;
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

  /* Copy into the procData slot if necessary */
  if (putProc)
  {
    for (i = 0; i < pTrace->nRaw; i++)
      pTrace->procData[i] = pTrace->rawData[i];
    pTrace->nProc = pTrace->nRaw;
  }
  else 
  {
    /* Apply a time-domain taper to both ends of the time series */
    npoints = pSta->timeTaper / pTrace->delta;
    taper(pTrace->rawData, pTrace->nRaw, npoints);
  }


  return;
}

/*
 * cleanTrace: initialize the global trace buffer, freeing old GAP structures.
 *    Returns: nothing
 */
void cleanTrace( void )
{
  GAP *pGap;
  
  gTrace.nRaw = 0L;
  gTrace.nProc = 0L;
  gTrace.delta = 0.0;
  gTrace.starttime = 0.0;
  gTrace.endtime = 0.0;
  gTrace.nGaps = 0;
  
  /* Clear out the gap list */
  gTrace.nGaps = 0;
  while ( (pGap = gTrace.gapList) != (GAP *)NULL)
  {
    gTrace.gapList = pGap->next;
    free(pGap);
  }
  return;
}

/*
 * initBufs: allocate the three arrays needed for handling trace data
 *           The argument reqLen is the reqeusted length; this value
 *           is used for the raw data array. That value, or larger
 *           if needed to find a multiple of the FFT factors, is used
 *           for the size of the processed data array. The work array
 *           is sized as needed by the convertWave routine.
 *           This routine is intended to be called at startup instead
 *           of later in the process life, to minimize memory growth
 *           during process lifetime.
 *   Returns: 0 on success
 *           -1 when out of memory
 */
int initBufs( long reqLen, int rawOnly )
{
  long nfft;
  FACT *pF;
  
  if ( (gTrace.rawData = (double *)malloc(reqLen * sizeof(double))) ==
       (double*)NULL)
  {
    logit("", "initBuffs: out of memory for rawData\n");
    return -1;
  }
  gTrace.lenRaw = reqLen;
  
  if (rawOnly)
    return 0;
  
  if ( (nfft = prepFFT( reqLen, &pF)) < 0)
  {
    logit("", "initBuffs: out of memory for FFT factors\n");
    return -1;
  }
  
  if ( (gTrace.procData = (double *)malloc((nfft + 2) * sizeof(double))) ==
       (double*)NULL)
  {
    logit("", "initBuffs: out of memory for procData\n");
    return -1;
  }
  gTrace.lenProc = nfft;
  
  if ( (gWork = (double *)malloc((nfft + 2) * sizeof(double))) ==
       (double*)NULL)
  {
    logit("", "initBuffs: out of memory for gWork\n");
    return -1;
  }

  if ( (gWorkFFT = (double *)malloc((nfft + 1) * sizeof(double))) ==
       (double*)NULL)
  {
    logit("", "initBuffs: out of memory for gWorkFFT\n");
    return -1;
  }

  return 0;
}


/*
 * getPeakAmp: Find the maximum peak-to-peak amplitude within a sliding 
 *             window of the processed trace data.
 *             This function looks at the data within the "peak search"
 *             window, defined by peakWinStart and peakWinEnd in the COMP3
 *             structure. It looks for two peak values: the largest 
 *             peak-to-peak swing within a sliding window, and the largest
 *             zero-to-peak, positive or negative. The times, two for the
 *             peak-to-peak and one for the zero-to-peak, are saved along
 *             with the amplitudes in the COMP3 structure.
 *             Based on the ptp function in SAC v. 10.6f.
 */
void getPeakAmp(DATABUF *pTrace, COMP3 *pComp, STA *pSta, LMPARAMS *plmParams,
                EVENT *pEvt)
{
  long i;
  long iStart, iEnd;  /* start and end of peak-search window */
  long sStart, sStop; /* start and end of sliding window */
  long lmin, lmax;    /* indices of min and max within sliding window */
  long imin, imax;    /* indices of min and max in peak-search window */
  long iz2p = -1;     /* index of largest zero-to-peak in peak-search window */
  long wlen;          /* length of sliding window */
  double dmin, dmax;  /* amps for lmin, lmax */
  double p2pmax = 0.0;  /* peak-to-peak maximum in peak-search window */
  double z2pmax = 0.0;  /* zero-to-peak extremum in peak-search window */
  double preZ2P = 0.0;  /* zero-to-peak extremum before P-arrival */
  long iPZ;             /* index of largest preZ2P */
  
  imin = imax = 0;
  
  /* Set some "no-amp" values until we have useful amplitudes */
  pComp->z2pAmp = 0.0;
  pComp->z2pTime = -1.0;
  pComp->p2pAmp = -1.0;
  pComp->p2pMin = 0.0;
  pComp->p2pMax = 0.0;
  pComp->p2pMinTime = -1.0;
  pComp->p2pMaxTime = -1.0;
  
  /* See how much noise is in the pre-P-arrival data.       *
   * We hope and pray it isn't the coda of a previous event */
  iStart = (long)(0.5 + pSta->timeTaper / pTrace->delta );
  if (iStart < 0) iStart = 0;
  iEnd = (long)( 0.5 + (pSta->p_est - pTrace->starttime) * 0.9 / 
                   pTrace->delta );
  if (iEnd > pTrace->nProc)
    iEnd = pTrace->nProc;   /* be sure we don't walk off the end */
  for (i = iStart; i < iEnd; i++)
  {
    if (fabs(pTrace->procData[i]) > preZ2P)
    {
      preZ2P = fabs(pTrace->procData[i]);
      iPZ = i;
    }
  }
  
  iStart = (long)( 0.5 + (pComp->peakWinStart - pTrace->starttime) / 
                   pTrace->delta );
  if (iStart < 0 )
    iStart = 0;
  iEnd = (long)( 0.5 + (pComp->peakWinEnd - pTrace->starttime ) / 
                 pTrace->delta);
  if (iEnd > pTrace->nProc)
    iEnd = pTrace->nProc;
  
  if (plmParams->debug & LM_DBG_TIME)
  {
    logit("", "trace start: %10.4lf end: %10.4lf (%ld)\n", 
          pTrace->starttime - pEvt->origin_time,
          pTrace->endtime - pEvt->origin_time, pTrace->nProc);
    logit("", "search start: %10.4lf (%ld) end: %10.4lf (%ld)\n", 
          pComp->peakWinStart - pEvt->origin_time, iStart, 
          pComp->peakWinEnd - pEvt->origin_time, iEnd);
  }

  if (iEnd <= iStart) {
    logit("e", "trace <%s.%s.%s.%s> too short to process\n", 
          pSta->sta, pComp->name, pSta->net, pComp->loc);
    if ( ! (plmParams->debug & LM_DBG_TIME))
    {
      logit("", "trace start: %10.4lf end: %10.4lf (%ld)\n", 
            pTrace->starttime - pEvt->origin_time,
            pTrace->endtime - pEvt->origin_time, pTrace->nProc);
      logit("", "search start: %10.4lf (%ld) end: %10.4lf (%ld)\n", 
            pComp->peakWinStart - pEvt->origin_time, iStart, 
            pComp->peakWinEnd - pEvt->origin_time, iEnd);
    }
    return;
  }

  /* Search for the zero-to-peak extremum */
  for (i = iStart; i < iEnd; i++)
  {
    if (fabs(pTrace->procData[i]) > z2pmax)
    {
      z2pmax = fabs(pTrace->procData[i]);
      iz2p = i;
    }
  }

  /* Trace has all zero values */
  if(iz2p == -1) {
    logit("", "getPeakAmp: <%s.%s.%s.%s> trace has all zero values.\n",
          pSta->sta, pComp->name, pSta->net, pComp->loc);

    pComp->BadBitmap |= LM_BAD_ZEROS;
  } else {

  if (z2pmax < plmParams->z2pThresh * preZ2P)
  {
    logit("", "getPeakAmp: <%s.%s.%s.%s> z2p max (%5g) too small for pre-event threshold (%5g)\n",
          pSta->sta, pComp->name, pSta->net, pComp->loc, z2pmax, 
          plmParams->z2pThresh * preZ2P);
    pComp->BadBitmap |= LM_BAD_Z2P;
  }
  else
  {
    pComp->z2pAmp = pTrace->procData[iz2p];
    pComp->z2pTime = pTrace->starttime + pTrace->delta * iz2p;
  
    /* Do sliding-window peak-to-peak search if desired */
    if (plmParams->slideLength > 0.0)
    {
      /* sliding-window search within search window: set the window limits */
      wlen = (long)( 0.5 + plmParams->slideLength / pTrace->delta );
      if (wlen > iEnd - iStart)
        wlen = iEnd - iStart;
      else if (wlen <= 0)
        wlen = (iStart - iEnd)/2;
      
      /* Slide the window along the data */
      for (sStart = iStart, sStop = wlen + iStart; sStop <= iEnd; sStart++, sStop++)
      {
        /* Search within the window for the maximum peak-to-peak */
        dmin = 1.0e+10;
        dmax = -1.0e+10;
        for (i = sStart; i < sStop; i++)
        {
          if (pTrace->procData[i] > dmax)
          {
            dmax = pTrace->procData[i];
            lmax = i;
          }
          if (pTrace->procData[i] < dmin)
          {
            dmin = pTrace->procData[i];
            lmin = i;
          }
        }
        if (dmax - dmin > p2pmax)
        {
          p2pmax = dmax - dmin;
          imax = lmax;
          imin = lmin;
        }
      }
      pComp->p2pAmp = p2pmax;
      pComp->p2pMin = pTrace->procData[imin];
      pComp->p2pMax = pTrace->procData[imax];
      pComp->p2pMinTime = pTrace->starttime + pTrace->delta * imin;
      pComp->p2pMaxTime = pTrace->starttime + pTrace->delta * imax;
    }
  }

  }

  return;
}

/*
 * saveWATrace: a selector function for dispatching the desired trace-save
 *              procedure.
 */
void saveWATrace( DATABUF *pTrace, STA *pSta, COMP3 *pComp, EVENT *pEvt, 
                 LMPARAMS *plmParams)
{
  switch (plmParams->saveTrace)
  {
  case LM_ST_SAC:
    saveSACWATrace( pTrace, pSta, pComp, pEvt, plmParams);
    break;
#ifdef UW
  case LM_ST_UW:
    saveUWWATrace(pTrace, pSta, pComp, plmParams);
    break;
#endif
  default:
    logit("", "saveWATrace: unknown option (%d)\n", plmParams->saveTrace);
  }
  return;
}

/*
 * getAmpFromSource: get the Wood-Anderson amplitude directly from the source
 *        instead of synthesizing the Wood-Anderson from raw trace data.
 *        This is a selector for the desired source procedure.
 *   Returns: 0 on success
 *           -1 on fatal errors
 *           +1 on non-fatal errors
 */
int getAmpFromSource( EVENT *pEv, LMPARAMS *plmParams)
{
  int rc;
  
  switch (plmParams->traceSource)
  {
  case LM_TS_SAC:
    rc = getAmpFromSAC(pEv, plmParams, &gTrace);
    break;
#ifdef UW
  case LM_TS_UW:
    rc = getAmpDirectFromUW(pEv, plmParams);
    break;
#endif
  default:
    logit("", "localmag getAmpFromSource: unknown source: %d\n",
          plmParams->traceSource);
    rc = -1;
  }
  return rc;
}

/*
 * initLogA0: read in the LogA0 table from file.
 *    Returns: 0 on success
 *            -1 on failure: memory, parse, file-reading errors
 */
int initLogA0(LMPARAMS *plmParams)
{
  int nfiles, i = 0;
  char *com, *str;
  
  nfiles = k_open (plmParams->loga0_file); 
  if (nfiles == 0) 
  {
    logit("", "initLogA0: Error opening file <%s>\n", 
             plmParams->loga0_file);
    return -1;
  }

  while (k_rd ())        /* Read next line from active file  */
  {  
    com = k_str ();         /* Get the first token from line */

    /* Ignore blank lines & comments */
    if (!com)
      continue;
    if (com[0] == '#')
      continue;

    /* Process anything else as a command */
    if (k_its ("Dist")) 
    {
      if ( (str = k_str()) != (char *)NULL )
      {
        if (k_its("epi") )
          plmParams->fDist = LM_LD_EPI;
        else if (k_its("hypo") )
          plmParams->fDist = LM_LD_HYPO;
        else
        {
          logit("", "initLogA0: unknown Dist flag <%s>\n", str);
          k_close();
          return -1;
        }
      }
      else
      {
        logit("", "initLogA0: Dist missing argument in logA0 file\n");
        return -1;
      }
    }
      
    else if (k_its ("nDist") )
    {
      plmParams->nLtab = k_int();
      if ( (gLa0tab = (LOGA0 *)calloc(sizeof(LOGA0), plmParams->nLtab)) == 
           (LOGA0 *)NULL)
      {
        logit("", "initLogA0: out of memory for LogA0 table\n");
        k_close();
        return -1;
      }
    }

    else 
    {
      if (i >= plmParams->nLtab )
      {
        if (plmParams->nLtab == 0)
        {
          logit("", "initLogA0: nDist command must appear before table entries\n");
          k_close();
          return -1;
        }
        else
        {
          logit("", "initLogA0: too many LogA0 table entries\n");
          k_close();
          return -1;
        }
      }
      gLa0tab[i].val = gLa0tab[i].val2 = NOINITVAL;
      gLa0tab[i].dist = atoi(com);
      gLa0tab[i].val = k_val();  /* horiz attenuation */
      gLa0tab[i].val2 = k_val(); /* z attenuation, new */
      if (gLa0tab[i].val2 == 0.0) gLa0tab[i].val2 = NOINITVAL;
      k_err(); /* clear any errors here because Z relationship might not be given*/
      i++;
    }
      
    /* See if there were any errors processing the command */
    if (k_err ()) 
    {
      logit("", "initLogA0: Bad command in <%s>\n\t%s\n",
            plmParams->loga0_file, k_com());
      k_close();
      return -1;
    }

  } /** while k_rd() **/
    
  k_close ();
    
  plmParams->nLtab = i;  /* In case there are fewer entries than claimed */
  return 0;
}

/*
 * getMagFromAmp: compute the magnitude for each individual component, 
 *                using one half of the peak-to-peak amplitude found
 *                from the sliding window in GetPeak.
 *                When both horizontal components ('E' and 'N') are present, 
 *                we also compute a combined component magnitude from 
 *                one half of the average peak-to-peak amplitudes of these
 *                two horizontals.
 */
void getMagFromAmp(EVENT *pEvt, LMPARAMS *plmParams)
{
  STA *pSta;
  COMP1 *pC1;
  COMP3 *pComp;
  int i, dir;
  LOGA0 *la0;
  double dist, distTerm=0;
  double sumMagSta, sumMagEvt, ave;
  double sumChanMag, sumChanAmp;
  int nStaMags, nEvtMags, nChans;
  double *magArray = gTrace.rawData; /* A suitable hunk of space that isn't *
                                      * being used right now.               */
  
  pEvt->mag = 0.0;
  pEvt->sdev = 0.0;
  
  /* Run through all the stations we know about */
  nEvtMags = 0;
  sumMagEvt = 0.0;
  for (i = 0; i < pEvt->numSta; i++)
  {
    pSta = &pEvt->Sta[i];
    pSta->mag = NO_MAG; /* initialize this here, just in case */

    /* Initialize component fields with default values */
    pC1 = pSta->comp;
    /* Loop through the COMP1 list for this station */
    while ( pC1 != (COMP1 *)NULL)
    {
	pC1->mag = NO_MAG;
	pC1->notUsed = 1;

	pC1 = pC1->next;
    }

    switch (plmParams->fDist)
    {
    case LM_LD_EPI:
      dist = pSta->dist;
      break;
    case LM_LD_HYPO:
      dist = sqrt(pSta->dist * pSta->dist + pEvt->depth * pEvt->depth);
      break;
    default:
      /* This case is unlikely, since we already test for it in Configure */
      logit("", "getMagFromAmp: unknown value (%d) for LogA0 distance rule\n",
            plmParams->fDist);
      logit("", "\tMAGNITUDES COMPUTED WITHOUT LOGA0\n");
      pEvt->mag = NO_MAG;
      dist = 0.0;
    }
    
    if ( (la0 = logA0( dist, plmParams->nLtab)) == NULL)
    {
      logit("", "getMagFromAmp: <%s.%s> distance <%f) too close for logA0 table\n",
            pSta->sta, pSta->net, dist);
      continue;
    }

    
    /* Walk the list of components for this station */
    nStaMags = 0;
    sumMagSta = 0.0;
    pC1 = pSta->comp;
    /* Loop through the COMP1 list for this station */
    while ( pC1 != (COMP1 *)NULL)
    {
      sumChanMag = sumChanAmp = 0.0;
      nChans = 0;
      /* Calculate magnitude for each component/direction */
      for (dir = 0; dir < 3; dir++)
      {
        pComp = &pC1->c3[dir];
        if (pComp->name[0] == 0)
          continue;

        if ( dir == LM_Z ) {
            if ( la0->val2 != NOINITVAL ) {
                distTerm = la0->val2;
            } else {
                /* The table has no value for in the Z direction    */
                /* log and continue Make sure you put in a z column */
                /* in the Richter Column.                           */
                logit( "e", "warning: getMagFromAmp has no Z attenuation value at distance %d, using horizontal logA0\n", la0->dist );
                distTerm = la0->val;
            }
        } else {
            distTerm = la0->val;
        }


        /* Use half of peak-to-peak amplitude if we have one */
        if (pComp->p2pAmp > 0.0)
        {
          pComp->mag = log10(pComp->p2pAmp / 2.0) + distTerm;
          if (pComp->pSCNLPar != (SCNLPAR *)NULL)
            pComp->mag += pComp->pSCNLPar->magCorr;
          sumChanAmp += pComp->p2pAmp / 2.0;
          sumChanMag += pComp->mag;
          nChans++;
        }
        /* no peak-to-peak; use the zero-to-peak amplitude */
        else if (pComp->z2pAmp != 0.0) 
	/* this was wrong before, z2p was never used if the z2p was neg */
        {
          pComp->mag = log10(fabs(pComp->z2pAmp)) + distTerm;
          if (pComp->pSCNLPar != (SCNLPAR *)NULL)
            pComp->mag += pComp->pSCNLPar->magCorr;
          sumChanAmp += fabs(pComp->z2pAmp); 
          sumChanMag += pComp->mag;
          nChans++;
        }
        else
          pComp->mag = NO_MAG;

/* commented this out as Jim Pechmann found that this caused 2x magCorrection...this
	is a left over from a previous edition and should have long been removed.

        if (pComp->pSCNLPar != (SCNLPAR *)NULL)
          pComp->mag += pComp->pSCNLPar->magCorr;
*/
        if (plmParams->debug & LM_DBG_LA0 && pComp->pSCNLPar != (SCNLPAR *)NULL)
        {
            logit("", "%s.%s.%s chan mag: %5.3f dist: %4.2lf -LogA0: %4.2lf magCorr: %5.3f\n", 
			pSta->sta, pSta->net, pComp->name, pComp->mag,
                  	dist, distTerm, pComp->pSCNLPar->magCorr);
	}
        if (plmParams->debug & LM_DBG_LA0 && pComp->pSCNLPar == (SCNLPAR *)NULL) 
        {
            logit("", "%s.%s.%s chan mag: %5.3f dist: %4.2lf -LogA0: %4.2lf\n", 
			pSta->sta, pSta->net, pComp->name, pComp->mag, dist, distTerm);
        }
      }
      
      /* now check to make sure that both horizontals are available if configured to do so */
      if ( (nChans > 0 && plmParams->require2Horizontals == 0) || 
	   (nChans >= 2 && plmParams->require2Horizontals > 0))
      {
        if (plmParams->fMeanCompMags == TRUE)
        {  /* Take the mean of component magnitudes */
          pC1->mag = sumChanMag / nChans;
        }
        else
        { /* take the mean of component amplitudes; find magnitude from that */
          pC1->mag = log10( sumChanAmp / nChans ) + distTerm;

          /* Apply the station correction if there is one; assume it is *
           * the same for East and North components!                    */
          if (pC1->c3[LM_E].pSCNLPar != (SCNLPAR *)NULL)
            pC1->mag += pC1->c3[LM_E].pSCNLPar->magCorr;
        }
        
        pC1->notUsed = 0;
        sumMagSta += pC1->mag;
        nStaMags++;
#ifdef DEBUG
        logit("", "DEBUG: adding component group %2s.%2s for station %2s.%s to magnitude calculation\n", pC1->n2, pC1->loc, pSta->net, pSta->sta);
#endif /* DEBUG */
      }
      else
      {
        pC1->mag = NO_MAG;
        pC1->notUsed = 1;
      }
      
      pC1 = pC1->next;
    }

    if (nStaMags > 0)
    {
      pSta->mag = sumMagSta / nStaMags;
      sumMagEvt += pSta->mag;
      magArray[nEvtMags] = pSta->mag;
      nEvtMags++;
    }
    else
      pSta->mag = NO_MAG;
  }
        
  if (nEvtMags > 0)
  {
    ave = sumMagEvt / nEvtMags;
    if (pEvt->mag != NO_MAG)
      pEvt->mag = ave;
    pEvt->nMags = nEvtMags;
    qsort(magArray, nEvtMags, sizeof(double), CompareDoubles);
    if (nEvtMags % 2 == 0) 
    {
	/* average the two middle values  */
    	pEvt->magMed = (magArray[nEvtMags/2]+magArray[(nEvtMags/2)-1])/2.0;
    }
    else
    {
	/* just grab the middle value */
    	pEvt->magMed = magArray[nEvtMags/2];
    }

  }
  else
  {
    pEvt->mag = NO_MAG;
    pEvt->nMags = 0;
    pEvt->magMed = NO_MAG;
  }

  /* Standard deviation */
  if (nEvtMags > 1)
  {
    sumMagEvt = 0.0;
    for (i = 0; i < pEvt->numSta; i++)
    {
      pSta = &pEvt->Sta[i];
      if ( pSta->mag != NO_MAG )
        sumMagEvt += (pSta->mag - ave) * (pSta->mag - ave);
    }
    pEvt->sdev = sqrt(sumMagEvt / (nEvtMags - 1));
  }
  return;
}


/*
 * endEvent: Terminate the event and clean up. Calls cleanup routines
 *           for those data sinks that need it, then cleans up the
 *           EVENT structure in preparation for the next event.
 */
void endEvent(EVENT *pEvt, LMPARAMS *plmParams)
{
  int i;
  COMP1 *pC1;

  /* Call `endEvent' for those data sinks that may need it: EWDB? */
#ifdef UW
  if (plmParams->outputFormat == LM_OM_UW)
    writeUWEvent(pEvt, plmParams);
#endif
  
  /* Clean up the EVENT structure in preparation for next event */
  for (i = 0; i < plmParams->maxSta; i++)
  {
    while ( (pC1 = pEvt->Sta[i].comp) != (COMP1 *)NULL)
    {
      pEvt->Sta[i].comp = pC1->next;
      free(pC1);
    }
  }
  pEvt->numSta = 0;
  memset(pEvt->Sta, 0, plmParams->maxSta * sizeof(STA));

  return;
}

/*
 * procArc: parse the hyp2000 archive message into the EVENT structure
 *   Returns: 0 on success
 *           -1 on parse errors
 */
int procArc( char *msg, EVENT *pEvt)
{
  struct Hsum Sum;          /* Hyp2000 summary data    */
  char *cur_msg = NULL;
  char *cur_sdw = NULL;
  int error = 0;
  int nline = 0;

  cur_msg = msg;
  cur_sdw = parse_arc_next_shadow(cur_msg);
  while( error != -1 && cur_msg != NULL && cur_sdw != NULL ) {

    if(cur_msg[0] != '$') {
      cur_sdw = parse_arc_next_shadow(cur_msg);
      nline++;

      if(cur_sdw[0] != '$') {
	logit("", "procArc: error reading arc shadow line:\n%s\n", cur_sdw);
	error = -1;
      } else {

	if (nline == 1) {
	  /* Summary ARC line */

	  /* Process the hypocenter card (1st line of msg) */
	  if ( read_hyp( cur_msg, cur_sdw, &Sum ) < 0 )
	  {  /* Should we send an error message? */
	    logit( "", "procArc: error parsing HYPO summary message.\n" );
	    error = -1;
	  } else {

	    logit( "", "procArc: parsed HYPO summary message.\n" );

	    sprintf(pEvt->eventId, "%lu", Sum.qid);
	    pEvt->lon = (double)Sum.lon;
	    pEvt->lat = (double)Sum.lat;
	    pEvt->depth = (double)Sum.z;
	    pEvt->origin_time = Sum.ot - GSEC1970; /* 1600 to 1970, in seconds */
	    pEvt->origin_version = (long) Sum.version;
	    pEvt->qdds_version = 0;

	    pEvt->numArcPck = 0;
	  }

	} else {
	  /* Station ARC line */
	  if(pEvt->numArcPck < pEvt->maxArcPck) {
	    if (read_phs (cur_msg, cur_sdw, &(pEvt->ArcPck[pEvt->numArcPck])) < 0) {
	      logit("", "procArc: error reading phase line info.\n");
	      error = -1;
	    } else {
	      if(pEvt->ArcPck[pEvt->numArcPck].site[0] != 0 ) {
		logit( "", "procArc: parsed HYPO phase line (%s.%s).\n", pEvt->ArcPck[pEvt->numArcPck].site, pEvt->ArcPck[pEvt->numArcPck].comp );
		pEvt->numArcPck++;
	      } else {
		logit( "", "procArc: parsing HYPO phase lines is terminated.\n");
		cur_msg = NULL;
		cur_sdw = NULL;
	      }
	    }
	  } else {
	    logit("", "procArc: WARNING number of arc phases execeds number of allocated %d\n", pEvt->maxArcPck );
	    cur_msg = NULL;
	    cur_sdw = NULL;
	  }
	}

      }
    } else {
      logit("", "procArc: error reading arc no shadow line.\n");
      error = -1;
    }

    if(error != -1) {
      cur_msg = parse_arc_next_shadow(cur_sdw);
      nline++;
    }
  }

  if(error != -1) {
      logit("", "procArc: read %d lines and %d phases of the arc message.\n", nline, pEvt->numArcPck);
  } else {
      logit("", "procArc: error reading arc message (%d lines and %d phases).\n", nline, pEvt->numArcPck);
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

/*
 * getArc: Read content of a hyp2000 archive message and 
 *     fill in part of the EVENT structure.
 *  Arguments: filename: archive file name; if filename is empty or NULL,
 *                   archive message will be read from standard input.
 *             pEvt: pointer to the EVENT structure which is to be filled in.
 *                   The EVENT structure must be allocated by the caller;
 *  Returns: 0 on success;
 *          -1 on failure such as memory or read errors.
 * PRELIMINARY: may change method of geting ARC summary line
 */
static int getArc(char *filename, EVENT *pEvt)
{
  int rc = 0;
  char line[MAX_ARC_LINE+1];
  char arcmsg[MAX_BYTES_PER_EQ+1];
  int open_file = 0;
  int error = 0;
  FILE *fp;

  /* Prevent over bound null terminated string */
  line[MAX_ARC_LINE] = 0;
  arcmsg[MAX_BYTES_PER_EQ] = 0;

  if (filename == NULL || strlen(filename) == 0)
  {
    fp = stdin;
  }
  else
  {
    if ( (fp = fopen(filename, "r")) == NULL)
    {
      logit("", "Error opening <%f>: %d\n", filename, errno);
      rc = -1;
      goto Exit;
    }
    open_file = 1;
  }
  
  /* Put all file content into a string, since procArc has been changed */
  arcmsg[0] = 0;
  while( fgets(line, MAX_ARC_LINE, fp) != NULL  &&  error == 0) {

    /* Make sure we got a whole line */
    if (line[strlen(line)-1] != '\n')
    {
      logit("", "Warning: line from archive message too long, truncated;"
	  "\nFirst 40 chars: %40s\n", line);
      line[strlen(line)-1] = '\n';
    }

    if (strlen(line) + 1 > MAX_BYTES_PER_EQ - strlen(arcmsg)) {
      logit("e", "Arc messge would be truncated.\n");
      error = -1;
    } else {
      strncat(arcmsg, line, MAX_BYTES_PER_EQ - strlen(arcmsg) - 1);
    }

  }

  if ( error == 0 )
  {
    if (procArc( arcmsg, pEvt) < 0)
    {  /* Don't complain, procArc already did */
      rc = -1;
      goto Exit;
    }
  }
  else
  {
    logit("", "getArc: nothing to read from event message\n");
    rc = -1;
  }
  
 Exit:
  /* Done reading the message; close the file if we opened it. */
  if (open_file)
    fclose(fp);
  return rc;
}


/*
 * GetRespPZ: get response function (as poles and zeros) for the
 *   instrument sta, comp, net from the location specified in LMPARAMS.
 *   Response is filed in static responseStruct gScnlPZ for later use
 *   by makeWA().
 *  Resturns: 0 on success
 *           -1 on fatal errors
 *           +1 on non-fatal errors
 */
static int getRespPZ(char *sta, char *comp, char *net, char *loc, LMPARAMS *plmParams, 
              EVENT *pEvt)
{
  char respfile[PATH_MAX];
  int len, rc;
  
  /* Make sure the respone structure doesn't have any old memory allocated */
  cleanPZ(&gScnlPZ);
  memset(respfile, 0, PATH_MAX);
  
  switch(plmParams->respSource)
  {
#ifdef EWDB
  case LM_RS_EWDB:
    rc = getRespEWDB(sta, comp, net, loc, &gScnlPZ);
    return rc;
    break;
#endif
#ifdef UW
  case LM_RS_UW:
    rc = getUWResp(sta, comp, net, loc, &gScnlPZ, pEvt);
    return rc;
    break;
#endif    
  case LM_RS_SAC:  /* fill in directory; rest of action is after switch() */
    strcpy(respfile, plmParams->sacInDir);
    break;
  case LM_RS_FILE:  /* fill in directory; rest of action is after switch() */
    strcpy(respfile, plmParams->respDir);
    break;
  default:
    logit("", "getRespPZ: unknown response source <%d>\n", 
          plmParams->respSource);
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
                  plmParams->respNameFormat) < 0)
  {
    logit("", "getRespPZ: error formating respfile <%s>\n", respfile);
    return -1;
  }
  if ( (rc = readPZ(respfile, &gScnlPZ)) < 0)
  {
    switch(rc)
    {
    case -1:
      logit("", "getRespPZ: out of memory building pole-zero struct\n");
      break;
    case -2:
      logit("", "getRespPZ: error parsing pole-zero file <%s> for <%s.%s.%s.%s>\n", 
            respfile, sta, comp, net, loc);
      return +1;  /* Maybe the other resp files will work */
      break;
    case -3:
      logit("", "getRespPZ: invalid arguments passed to readPZ\n");
      break;
    case -4:
      logit("", "getRespPZ: error opening pole-zero file <%s> for <%s.%s.%s>\n", 
            respfile, sta, comp, net);
      return +1;  /* Maybe the other resp files will work */
      break;
    default:
      logit("", "getRespPZ: unknown error <%d> from readPZ(%s)\n", rc,
            respfile);
    }
    return -1;
  }
  /* all done! */
  return 0;
}

/*
 * initSaveTrace: initialize the trace-saving system for a new event.
 *                This is a simple selector for the specific type of file.
 *                Note that we don't return anything; if saving doesn't
 *                work, we don't want to hold up the show.
 */
static void initSaveTrace(EVENT *pEvt, LMPARAMS *plmParams)
{
  switch(plmParams->saveTrace)
  {
  case LM_ST_SAC:
    initSACSave(pEvt, plmParams);
    break;
#ifdef UW
  case LM_ST_UW:
    initUWSave(pEvt, plmParams);
    break;
#endif
  default:
    break;
  }
  return;
}

/*
 * termSaveTrace: Complete the trace-saving mechanism for an event.
 *                This is a simple selector for the specific type of file.
 *                Note that we don't return anything; if saving doesn't
 *                work, we don't want to hold up the show.
 */
static void termSaveTrace(EVENT *pEvt, LMPARAMS *plmParams)
{
  switch(plmParams->saveTrace)
  {
  case LM_ST_SAC:
    termSACSave(pEvt, plmParams);
    break;
#ifdef UW
  case LM_ST_UW:
    termUWSave(pEvt, plmParams);
    break;
#endif
  default:
    break;
  }
  return;
}

/*
 * logA0: look up the LogA0 value for given distance in the table of size nMax.
 *        If dist is less than the first entry, NULL is returned;
 *        if dist exceeds the last table entry, the LogA0 value for the
 *        last entry is returned.
 *        Otherwise, the nearest table entry to the given distance is
 *        returned.
 */
static LOGA0  *logA0( double dist, int nMax )
{
  int i;
  
  if ((int)dist < gLa0tab[0].dist)
    return NULL;
  
  for (i = 1; i < nMax; i++)
  {
    if ( (double)(gLa0tab[i].dist) > dist)
    {
      if ((double)(gLa0tab[i].dist) - dist < 
          dist - (double)(gLa0tab[i-1].dist))
        return &gLa0tab[i];
      else
        return &gLa0tab[i-1];
    }
  }
  return &gLa0tab[nMax-1];
}


/*************************************************************
 *                   CompareDoubles()                        *
 *                                                           *
 *  This function can be passed to qsort() and bsearch().    *
 *  Compare two doubles for ordering in increasing order     *
 *************************************************************/
static int CompareDoubles( const void *s1, const void *s2 )
{
  double *d1 = (double *)s1;
  double *d2 = (double *)s2;

  if (*d1 < *d2)
     return -1;
  else if (*d1 > *d2)
    return 1;
  
  return 0;
}


