/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: arc_2_ewevent.c 3470 2008-12-02 21:34:51Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2008/12/02 21:34:51  paulf
 *     oracle surgery performed, had to add in 2 .c files and modify the makefile.nt
 *
 *     Revision 1.8  2004/06/10 19:19:43  davidk
 *     Updated with Lucky's 2003 changes to the versions in ew/libsrc/util.
 *     Deleted the duplicates in ew/libsrc/util
 *
 *     Revision 1.2  2002/10/29 18:47:42  lucky
 *     Added origin version number
 *
 *     Revision 1.1  2002/06/28 21:06:22  lucky
 *     Initial revision
 *
 *     Revision 1.7  2001/07/20 17:40:05  lucky
 *     State of the code after much of v6.0 testing and debugging
 *
 *     Revision 1.6  2001/07/01 21:55:20  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.5  2001/06/26 19:55:31  lucky
 *     Fixed a bug where the number of stamags was not correctly incremented
 *     in the conversion from Arc msg to Event struct.
 *
 *     Revision 1.4  2001/06/21 21:25:00  lucky
 *     State of the code after LocalMag review was implemented and partially tested.
 *
 *     Revision 1.3  2001/06/06 20:54:46  lucky
 *     Changes made to support multitude magnitudes, as well as amplitude picks. This is w
 *     in progress - checkin in for the sake of safety.
 *
 *     Revision 1.2  2001/05/21 22:33:42  davidk
 *     removed internal definition of MAGTYPE_DURATION.  Using the one from rw_mag.h
 *     instead.
 *
 *     Revision 1.1  2001/05/15 02:15:27  davidk
 *     Initial revision
 *
 *     Revision 1.4  2001/04/17 17:22:36  davidk
 *     Removed some unneeded local variables to get rid of compiler warnings
 *     on NT.
 *
 *     Revision 1.2  2001/01/18 21:26:58  lucky
 *     Cleaned up debug messages
 *
 *     Revision 1.1  2000/12/18 18:55:12  lucky
 *     Initial revision
 *
 *     Revision 1.12  2000/12/06 17:50:47  lucky
 *     We now correctly keep track of the pick onset
 *
 *     Revision 1.11  2000/09/20 19:50:09  lucky
 *     Do not over-write coda length when filling in S-pick information -- it gets
 *     the wrong duration in the Arc file.
 *
 *     Revision 1.10  2000/09/13 15:36:51  lucky
 *     Fixed FirstMotion character handling;
 *     Fixed handling of the coda length
 *
 *     Revision 1.7  2000/09/07 21:17:45  lucky
 *     Final version after the Review pages have been demonstrated.
 *
 *     Revision 1.4  2000/08/30 17:41:57  lucky
 *     InitEWEvent has been changed to include optional allocation of
 *     pChanInfo space. This must be optional because GetEWEventInfo mallocs
 *     space on the fly.
 *
 *     Revision 1.3  2000/08/28 15:41:10  lucky
 *     get_EW_event_info.h -> EW_event_info.h
 *
 *     Revision 1.2  2000/08/25 18:23:42  lucky
 *     Made to work with P and S waves, as well as the new pChanInfo struct.
 *
 *     Revision 1.1  2000/08/16 17:18:11  lucky
 *     Initial revision
 *
 *     Revision 1.13  2000/06/29 17:24:42  lucky
 *     Added to Sac2EWEvent the translation of the coda duration from the sac header.
 *     The coda duration is written to EW structs.
 *
 *     Revision 1.12  2000/06/28 22:53:07  lucky
 *     Increased the maximum number of Phases per event to 1000 to accomodate
 *     extremely large Dewey-type events.
 *     If BuildPhs fails, we record the failure, but do not exit. This way,
 *     even if one arrival writing fails we still get the remaining ones
 *     in the arc message.
 *
 *     Revision 1.11  2000/06/28 22:22:34  lucky
 *     Added a check in EWEvent2ArcMsg so that only the arrival-type channels
 *     are written to Arc message. Recall:channels may be arrivals, station
 *     magnitudes, or even just waveform data.
 *
 *     Revision 1.10  2000/06/26 18:14:42  lucky
 *     Fixed creation of hypoinverse arc files with NSN locations.
 *
 *     Revision 1.9  2000/06/22 23:29:11  lucky
 *     Fixed BuildPhs to work with USNSN messages'
 *
 *     Revision 1.8  2000/06/20 21:42:51  lucky
 *     *** empty log message ***
 *
 *     Revision 1.7  2000/06/16 17:23:36  lucky
 *     Set tObsPhase to be same as tCalcPhase when read from sac headers.
 *
 *     Revision 1.6  2000/06/13 19:05:58  lucky
 *     Fixed to work with the new hypo2ora, as well as the review stuff.
 *
 *     Revision 1.5  2000/06/07 22:09:52  lucky
 *     Added ArcMsg2EWEvent calls
 *
 *
 *     Revision 1.1  2000/02/14 18:51:48  lucky
 *     Initial revision
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <earthworm.h>
#include <ws_clientII.h>
#include <chron3.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <read_arc.h>


/* Define max lengths of various lines in the Hypoinverse archive format
   (includes space for a newline & null-terminator on each line )
 ************************************************************************/
#define ARC_HYP_LEN   500
#define ARC_SHYP_LEN  500
#define ARC_PHS_LEN   500
#define ARC_SPHS_LEN  500
#define ARC_TRM_LEN    74
#define ARC_STRM_LEN   74


/* Define some simple "functions"
 ********************************/
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))        /* Smaller value  */
#endif
#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))           /* Absolute value */
#endif


extern		time_t timegm_ew (struct tm *);

int 	EWEvent2ArcMsg (EWEventInfoStruct *, char *, int);
int		BuildHyp (EWEventInfoStruct *, char *, int);
int		BuildPhs (EWEventInfoStruct *, int, char *, int);
int		BuildTerm (int, char *, int);






/********************************************************
*
*  EWEvent2ArcMsg:
*     Creates a Hypoinverse Archive message, stored in 
*     string pArc, from the data passed in the 
*     pEWEvent structure and its substructures.
*
*
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    04/2000
*
********************************************************/
int 	EWEvent2ArcMsg (EWEventInfoStruct *pEWEvent, char *pArc, int ArcLen)
{

	int		i;

	if ((pEWEvent == NULL) || (pArc == NULL) || (ArcLen < 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if (BuildHyp (pEWEvent, pArc, ArcLen) != EW_SUCCESS)
	{
		logit ("", "Call to BuildHyp failed.\n");
		return EW_FAILURE;
	}

	for (i = 0; i < pEWEvent->iNumChans; i++)
	{
		if (pEWEvent->pChanInfo[i].iNumArrivals > 0)
		{
			if (BuildPhs (pEWEvent, i, pArc, ArcLen) != EW_SUCCESS)
			{
				logit ("", "Call to BuildPhs failed for channel %d.\n", i);
			}
		}
	}

	if (BuildTerm (pEWEvent->PrefOrigin.idEvent, pArc, ArcLen) != EW_SUCCESS)
	{
		logit ("", "Call to BuildTerm failed.\n");
		return EW_FAILURE;
	}

	return EW_SUCCESS;

}


/********************************************************
*
*  ArcMsg2EWEvent:
*     Given a Hypoinverse Archive message stored in 
*     string pArc, fill the pEWEvent structure
*     and its substructures.
*
*
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    06/2000
*
********************************************************/
int 	ArcMsg2EWEvent (EWEventInfoStruct *pEWEvent, char *pArc, int ArcLen)
{

	int					j, rc, index;
	int					nline, tmp, toalloc;
	char				*in;
	char				line[ARC_HYP_LEN];
	char				shdw[ARC_SHYP_LEN];
	EWDB_EventStruct	*pEvent;
	EWDB_OriginStruct	*pOrigin;
	EWDB_MagStruct		*pMagnitude;
	EWDB_ArrivalStruct	*pArrival;
	EWDB_StationMagStruct	*pStaMag;
	EWDB_StationStruct	*pStation;
  EWChannelDataStruct	*pOldChan;
	struct Hsum 		HSum;
	struct Hpck 		HPck;
	/* EWDB_CodaAmplitudeStruct	*pCodaAmp; */


	if ((pEWEvent == NULL) || (pArc == NULL) || (ArcLen <= 0))
	{
		logit ("", "ArcMsg2EWEvent: Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	/* Initialize the Event structure */
	if (InitEWEvent (pEWEvent) != EW_SUCCESS)
	{
		logit ("", "Call to InitEWEvent failed.\n");
		return EW_FAILURE;
	}


	/* Initialize other stuff
	 ***********************/
	nline  = 0;
	in     = pArc;

	memset (&HSum, 0, sizeof (struct Hsum));
	
	pEvent = &pEWEvent->Event;
	memset (pEvent, 0, sizeof (EWDB_EventStruct));
	pEWEvent->iNumChans = 0;


	pOrigin = &pEWEvent->PrefOrigin;
	memset (pOrigin, 0, sizeof (EWDB_OriginStruct));
	
	pMagnitude = &pEWEvent->Mags[0];
	memset (pMagnitude, 0, sizeof (EWDB_MagStruct));


	/* Read one data line and its shadow at a time from arcmsg; process them
	 ***********************************************************************/
	index = 0;
	while (index < ArcLen)
	{
		j = 0;
        while (in[index] != '\n')
        {
            line[j] = in[index];
            index = index + 1;
            j = j + 1;
        }
        if (in[index] == '\n')
        {
            line[j] = '\0';
        }
		else
		{
			logit ("", "Max hyp length reached without newline\n");
			return EW_FAILURE;
		}

		j = 0;
		index = index + 1;
        while ((in[index] != '\n') && (j <  ARC_SPHS_LEN))
        {
            shdw[j] = in[index];
            index = index + 1;
            j = j + 1;
        }
        if (in[index] == '\n')
        {
            shdw[j] = '\0';
        }
		else
		{
			logit ("", "Max hyp length reached without newline\n");
			break;
		}

		index = index + 1;

		nline++;

		/* Process the hypocenter card (1st line of msg) & its shadow
		************************************************************/
		if (nline == 1) 
		{                  /* hypocenter */
			rc = read_hyp (line, NULL, &HSum); 
	
			pEvent->idEvent=HSum.qid;
   	    
			/* Fill in the Origin Struct */
			pOrigin->BindToEvent = TRUE;
			pOrigin->idEvent = pEvent->idEvent;
			pOrigin->SetPreferred = TRUE;
			pOrigin->tOrigin = HSum.ot - GSEC1970; 
	
			pOrigin->dLat = HSum.lat;
			pOrigin->dLon = HSum.lon;
			pOrigin->dDepth = HSum.z;
			pOrigin->dErrLat = HSum.erh;
			pOrigin->dErrLon = HSum.erh;
			pOrigin->dErZ = HSum.erz;
			pOrigin->iGap = HSum.gap;
			pOrigin->dDmin = (float)(HSum.dmin);
			pOrigin->dRms = HSum.rms;
			pOrigin->iUsedPh = HSum.nph;
			pOrigin->iE1Azm = HSum.e1az;
			pOrigin->iE1Dip = HSum.e1dp;
			pOrigin->iE2Azm = HSum.e0az;
			pOrigin->iE2Dip = HSum.e0dp;
			pOrigin->dE0 = HSum.e0;
			pOrigin->dE1 = HSum.e1;
			pOrigin->dE2 = HSum.e2;
			pOrigin->iFixedDepth = FALSE;
			pOrigin->iVersionNum = HSum.version;
	
	
			/* Since the Event/Origin insert succeeded, 
		  	   insert the associated Magnitude 
			   *******************************************/
	
			/* Fill in the Magnitude Struct */
			pMagnitude->iMagType = MAGTYPE_DURATION;
			pMagnitude->dMagAvg  = HSum.Md;
			pMagnitude->iNumMags = (int)(HSum.mdwt + 0.9);
			pMagnitude->dMagErr  = HSum.mdmad;
			pMagnitude->idEvent  = pEvent->idEvent;
			pMagnitude->bBindToEvent = TRUE;
			pMagnitude->bSetPreferred = TRUE;

			pEWEvent->iNumMags = 1;
			pEWEvent->iPrefMag = 0;
			pEWEvent->iMd = 0;

		}
		else
		{
			/* Process all the phase cards & their shadows
			   *********************************************/
			if (strlen (line) < (size_t) 75)  /* found the terminator line      */
				break;  

			/* re-blank the pick struct */
			memset (&HPck, 0, sizeof (struct Hpck));

			if (pEWEvent->iNumChans >= pEWEvent->iNumAllocChans)
			{

				toalloc = (int) (pEWEvent->iNumAllocChans + CHAN_ALLOC_INCR);

				logit ("", "Max Channels (%d) reached. Alloc to %d\n",
												pEWEvent->iNumAllocChans, toalloc);

				pOldChan = pEWEvent->pChanInfo;
				if ((pEWEvent->pChanInfo = (EWChannelDataStruct *) malloc
								(toalloc * sizeof (EWChannelDataStruct))) == NULL)
				{
					logit ("",  "Could not malloc %d pChanInfo entries.\n", toalloc);
					return  (EW_FAILURE);
				}

				memcpy (pEWEvent->pChanInfo, pOldChan,
						(pEWEvent->iNumAllocChans * sizeof (EWChannelDataStruct)));

				for (tmp = pEWEvent->iNumChans; tmp < toalloc; tmp++)
					InitEWChan(&pEWEvent->pChanInfo[tmp]);

				pEWEvent->iNumAllocChans = toalloc;

				free (pOldChan);
			}



			/* load info into Pck structure   */
			rc = read_phs (line, shdw, &HPck);

			pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumArrivals = 0;
			pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumStaMags = 0;

			pStation = &pEWEvent->pChanInfo[pEWEvent->iNumChans].Station;
			memset(pStation, 0, sizeof(EWDB_StationStruct));


			/* Fill in the common information */
			strncpy (pStation->Sta, HPck.site, sizeof (pStation->Sta)-1);
			strncpy (pStation->Comp, HPck.comp, sizeof (pStation->Comp)-1);
			strncpy (pStation->Net, HPck.net, sizeof (pStation->Net)-1);

			/* Do we have a P-pick?? */
			if ((HPck.Plabel == 'P') || (HPck.Plabel == 'p'))
			{
				/* Fill in the P-pick information */

				rc = pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumArrivals;

				pArrival = &pEWEvent->pChanInfo[pEWEvent->iNumChans].Arrivals[rc];
				memset(pArrival, 0, sizeof(EWDB_ArrivalStruct));
	
				pStaMag = &pEWEvent->pChanInfo[pEWEvent->iNumChans].Stamags[rc];
				memset(pStaMag, 0, sizeof(EWDB_StationMagStruct));
	

				/* Fill in the Arrival record */
				pArrival->idOrigin = pOrigin->idOrigin;

				pArrival->tObsPhase = HPck.Pat -  GSEC1970; 
				pArrival->tCalcPhase = pArrival->tObsPhase - HPck.Pres;

				if (pArrival->dWeight > 1000.0)
					pArrival->dWeight = 0.0;

				pArrival->dAzm = (float) HPck.azm;
				pArrival->dTakeoff = (float) HPck.takeoff;
				pArrival->tResPick = HPck.Pres;
				pArrival->szObsPhase[0] = 'P';
				pArrival->cMotion = HPck.Pfm;
				pArrival->cOnset = HPck.Ponset;
				pArrival->dDist = HPck.dist;
				pArrival->dWeight = HPck.Pwt;
	
				if (HPck.Pqual == 3)
					pArrival->dSigma = 0.08;
				else if (HPck.Pqual == 2)
					pArrival->dSigma = 0.05;
				else if (HPck.Pqual == 1)
					pArrival->dSigma = 0.03;
				else if (HPck.Pqual == 0)
					pArrival->dSigma = 0.02;
				else
					pArrival->dSigma = 99.99;
	
				/* create phase label */
				sprintf (pArrival->szCalcPhase, "P");

				pStaMag->idChan = pArrival->idChan;
				pStaMag->StaMagUnion.CodaDur.idPick = pArrival->idPick;
				pStaMag->idMagnitude = pMagnitude->idMag;
				pStaMag->iMagType = MAGTYPE_DURATION;
				pStaMag->dMag = HPck.Md;
				pStaMag->dWeight = (float) HPck.codawt;
				pStaMag->StaMagUnion.CodaDur.tCodaTermObs = pArrival->tObsPhase + HPck.codalenObs;
				pStaMag->StaMagUnion.CodaDur.tCodaTermXtp = pArrival->tObsPhase + HPck.codalen;
	
				pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumArrivals = 
					pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumArrivals + 1;

				pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumStaMags = 
					pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumStaMags + 1;

			}

			/* Do we have an S-pick */
			if ((HPck.Slabel == 'S') || (HPck.Slabel == 's'))
			{

				/* Fill in the S-pick information */

				rc = pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumArrivals;

				pArrival = &pEWEvent->pChanInfo[pEWEvent->iNumChans].Arrivals[rc];
				memset(pArrival, 0, sizeof(EWDB_ArrivalStruct));
	
				pStaMag = &pEWEvent->pChanInfo[pEWEvent->iNumChans].Stamags[rc];
				memset(pStaMag, 0, sizeof(EWDB_StationMagStruct));
	
				/* pPhaseAmp = &pEWEvent->pChanInfo[pEWEvent->iNumChans].PhaseAmps[rc];
				 * memset(pPhaseAmp, 0, sizeof(EWDB_PhaseAmpStruct)); */
				
				/* Fill in the Arrival record */
				pArrival->idOrigin = pOrigin->idOrigin;

				pArrival->tObsPhase = HPck.Sat -  GSEC1970; 
				pArrival->tCalcPhase = pArrival->tObsPhase - HPck.Sres;

				if (pArrival->dWeight > 1000.0)
					pArrival->dWeight = 0.0;

				pArrival->dAzm = (float) HPck.azm;
				pArrival->dTakeoff = (float) HPck.takeoff;
				pArrival->tResPick = HPck.Sres;
				pArrival->szObsPhase[0] = 'S';
				pArrival->cMotion = HPck.Sfm;
				pArrival->cOnset = HPck.Sonset;
				pArrival->dDist = HPck.dist;
				pArrival->dWeight = HPck.Swt;
	
				if (HPck.Squal == 3)
					pArrival->dSigma = 0.08;
				else if (HPck.Squal == 2)
					pArrival->dSigma = 0.05;
				else if (HPck.Squal == 1)
					pArrival->dSigma = 0.03;
				else if (HPck.Squal == 0)
					pArrival->dSigma = 0.02;
				else
					pArrival->dSigma = 99.99;
	
				/* create phase label */
				sprintf (pArrival->szCalcPhase, "S");

        /* This is horse pucky, you can't have a duration magnitude calc'd off 
           an S wave!  CLEANUP! DavidK */
				pStaMag->idChan = pArrival->idChan;
				pStaMag->StaMagUnion.CodaDur.idPick = pArrival->idPick;
				pStaMag->idMagnitude = pMagnitude->idMag;
				pStaMag->iMagType = MAGTYPE_DURATION;
				pStaMag->dMag = HPck.Md;
				pStaMag->dWeight = (float) HPck.codawt;
				pStaMag->StaMagUnion.CodaDur.tCodaTermObs = pArrival->tObsPhase + HPck.codalenObs;
				pStaMag->StaMagUnion.CodaDur.tCodaTermXtp = pArrival->tObsPhase + HPck.codalen;

				pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumArrivals = 
					pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumArrivals + 1;

				pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumStaMags = 
					pEWEvent->pChanInfo[pEWEvent->iNumChans].iNumStaMags + 1;

			}


			pEWEvent->iNumChans = pEWEvent->iNumChans + 1;

		}   /* else(line==1) */

	} /*end while over reading message*/

	return EW_SUCCESS;

}


/***************************************************************
 * BuildHyp () builds a hypoinverse summary card & its shadow  *
 ***************************************************************/
int		BuildHyp (EWEventInfoStruct *pEvent, char *arcmsg, int maxlen)
{

	char 		line[ARC_HYP_LEN];   /* temporary working place */
	char 		shdw[ARC_SHYP_LEN];  /* temporary shadow card   */
	struct tm	*otm;
	time_t     	ott;
	int  		tmp;
	int  		i;
	EWDB_OriginStruct	*pOrig;
	EWDB_MagStruct		*pMag;


	if ((pEvent == NULL) || (arcmsg == NULL))
	{
		fprintf (stderr, "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	if ((pOrig = &(pEvent->PrefOrigin)) == NULL)
	{
		fprintf (stderr, "Invalid pEvent -- can't get PrefOrigin.\n");
		return EW_FAILURE;
	}

	if ((pMag = &(pEvent->Mags[pEvent->iPrefMag])) == NULL)
	{
		fprintf (stderr, "Invalid pEvent -- can't get Pref Magnitude.\n");
		return EW_FAILURE;
	}

/* Put all blanks into line and shadow card
 ******************************************/
   for (i = 0; i < ARC_HYP_LEN; i++)
		line[i] = ' ';

   for (i = 0; i < ARC_SHYP_LEN; i++)
		shdw[i] = ' ';

/*----------------------------------------------------------------------------------
Sample HYPOINVERSE 2000 hypocenter and shadow card.  Event id from binder is stored in
  cols 129-138.  Many fields are blank due to lack of information from binder.
  (summary is 165 chars, including newline; shadow is 95 chars, including newline):
199912312359492936 2810120 2596  851    27 78 19  15                                                                                         10154                1 \n
$1                                                                              \n   
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123
-----------------------------------------------------------------------------------*/

	/* Origin time 
	 *************/
	ott = (time_t) pOrig->tOrigin;
	otm = gmtime( &ott );

	sprintf( line, "%04d%02d%02d%02d%02d%02d%02d",  
              (otm->tm_year + 1900),
              otm->tm_mon+1,
              otm->tm_mday,
              otm->tm_hour,
              otm->tm_min,
              otm->tm_sec,
              (int)((pOrig->tOrigin-(double)ott+.005)*100.) );
	line[16]=' ';

	/* Latitude, convert from decimal degrees to degrees/minutes*100 */
	if (pOrig->dLat >= 0)
		sprintf (line+16, "%2d ",  (int)pOrig->dLat);
	else
		sprintf (line+16, "%2d%c", -(int)pOrig->dLat, 'S');

	tmp = (int) ((pOrig->dLat - (int)pOrig->dLat) * 6000.);   
	sprintf( line+19,  "%4d",     (int) MIN( ABS(tmp), 9999 ));
	line[23]=' ';

	/* Longitude, convert from decimal degrees to degrees/minutes*100 */
	if (pOrig->dLon >= 0)
		sprintf( line+23, "%2d%c",  (int)pOrig->dLon, 'E' );
	else
		sprintf( line+23, "%2d ", (int) (-1.0 * pOrig->dLon));
    line[26] = ' ';

	tmp = (int)( (pOrig->dLon - (int)pOrig->dLon) * 6000.);   
	sprintf( line+27, "%4d", (int) MIN( ABS(tmp), 9999 ) );
	line[31]=' ';

	/* Depth */
	if (pOrig->dDepth != 0.0)
	{
		tmp = (int) (pOrig->dDepth * 100.0);
		sprintf( line+31,  "%5d", (int) MIN( ABS(tmp), 99999 ) );
		line[36]=' ';
	}


	/* All the other stuff */

	if (pOrig->iUsedPh != 0)
	{
		sprintf( line+39, "%3d", (int) MIN( pOrig->iUsedPh, 999 ) );
		line[42] = ' ';
	}

	if (pOrig->iGap != 0)
	{
		sprintf( line+42, "%3d", (int) MIN( pOrig->iGap, 999 ) );
		line[45] = ' ';
	}

	if (pOrig->dDmin != 0.0)
	{
		sprintf( line+45, "%3d", (int) MIN( pOrig->dDmin, 999 ) );
		line[48] = ' ';
	}

	if (pOrig->dRms != 0.0)
	{
		tmp = (int) (pOrig->dRms*100.);
		sprintf( line+48,  "%4d", (int) MIN( tmp, 9999 ) );
		line[52]  = ' ';
	}

	/* azimuth of largest principal error */
	if (pOrig->iE2Azm != 0.0)
	{
		sprintf( line+52,  "%3d", (int) MIN( pOrig->iE2Azm, 999 ) );
		line[55]  = ' ';
	}

	/* dip of largest principal error  */
	if (pOrig->iE2Dip != 0.0)
	{
		sprintf( line+55,  "%2d", (int) MIN( pOrig->iE2Dip, 99 ) );
		line[57]  = ' ';
	}

	/* magnitude (km) of largest principal error */
	if (pOrig->dE0 != 0.0)
	{
		tmp = (int) (pOrig->dE0 * 100.);
		sprintf( line+57,  "%4d", (int) MIN( tmp, 9999 ) );
		line[61]  = ' ';
	}

	/* azimuth of intermediate principal error */
	if (pOrig->iE1Azm != 0.0)
	{
		sprintf( line+61,  "%3d", (int) MIN( pOrig->iE1Azm, 999 ) );
		line[64]  = ' ';
	}

	/* dip of intermediate principal error  */
	if (pOrig->iE1Dip != 0.0)
	{
		sprintf( line+64,  "%2d", (int) MIN( pOrig->iE1Dip, 99 ) );
		line[66]  = ' ';
	}

	/* magnitude (km) of intermed principal error */
	if (pOrig->dE1 != 0.0)
	{
		tmp = (int) (pOrig->dE1 * 100.);
		sprintf( line+66,  "%4d", (int) MIN( tmp, 9999 ) );
		line[70]  = ' ';
	}
 

	if (pMag->dMagAvg != 0.0)
	{
		tmp = (int) (pMag->dMagAvg*100.);
		sprintf( line+70,  "%3d", (int) MIN (tmp, 999));
		line[73]  = ' ';
	}


	/** Region Code - UNKNOWN - leave blank for now */
	sprintf (line+73, "   ");
	line[76] = ' ';
	

	/* magnitude (km) of smallest principal error */
	if (pOrig->dE2 != 0.0)
	{
		tmp = (int) (pOrig->dE2 * 100.);
		sprintf( line+76,  "%4d", (int) MIN( tmp, 9999 ) );
		line[80]  = ' ';
	}

	if (pOrig->dErrLon != 0.0)
	{
		tmp = (int) (pOrig->dErrLon*100.);
		sprintf( line+85,  "%4d", (int) MIN( tmp, 9999 ) );
		line[89]  = ' ';
	}

	if (pOrig->dErZ != 0.0)
	{
		tmp = (int) (pOrig->dErZ*100.);
		sprintf( line+89,  "%4d", (int) MIN( tmp, 9999 ) );
		line[93]  = ' ';
	}

	if (pMag->iNumMags != 0)
	{
		sprintf( line+100, "%4d", (int) MIN (pMag->iNumMags, 9999));
		line[104] = ' ';
	}

	if (pMag->dMagErr != 0)
	{
		sprintf( line+107, "%3d", (int) MIN (pMag->dMagErr, 999));
		line[110] = ' ';
	}


	if (pEvent->Event.idEvent != 0)
	{
		sprintf( line+136, "%10ld",   pEvent->Event.idEvent);
		line[146] = ' ';
	}


	sprintf (line+164, "\n\0");

/* Write out summary shadow card
 *******************************/
	sprintf( shdw,     "$1");
	shdw[2]= ' ';
	sprintf (shdw+94,  "\n\0");

/* Copy both to the target address if there's room
 *************************************************/
	if ((strlen (arcmsg) + strlen (line) + strlen (shdw) + 1) > (size_t)maxlen ) 
	{
		fprintf (stderr, "Max arcmsg length exceeded\n");
		return EW_FAILURE;
	}

	strcpy (arcmsg, line);
	strcat (arcmsg, shdw);

	return EW_SUCCESS;

}




/***********************************************************************
 * BuildPhs() builds a hypoinverse archive phase card & its shadow     *
 ***********************************************************************/
int		BuildPhs (EWEventInfoStruct *pEvent, int NumPhs, char *arcmsg, int maxlen)
{

	char 		line[ARC_PHS_LEN];     /* temporary phase card    */
	char 		shdw[ARC_SPHS_LEN];    /* temporary shadow card   */
	char 		otime[50]; 
	int   		i, j;
	struct tm	*otm;
	time_t		ott;
	int   		tmpint, slen, clen, nlen, plen;
	double		Cat;
	EWDB_ArrivalStruct	*pArr;
	EWDB_StationStruct	*pStation;
	EWDB_StationMagStruct	*pStaMag;



	if ((pEvent == NULL) || (NumPhs < 0) || (arcmsg == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if ((pStation = &pEvent->pChanInfo[NumPhs].Station) == NULL)
	{
		logit ("", "Invalid EWEvent passed in -- can't get Station.\n");
		return EW_FAILURE;
	}

	/* Compute the common arrival time - yyyymmddhhmm */
	ott = (time_t) pEvent->PrefOrigin.tOrigin;
	otm = gmtime( &ott );

	sprintf (otime, "%04d%02d%02d%02d%02d00.00", 
                            (otm->tm_year + 1900),
                            otm->tm_mon+1,
                            otm->tm_mday,
                            otm->tm_hour,
                            otm->tm_min);

	epochsec17 (&Cat, otime);



	/* Put all blanks into line and shadow card
	 ******************************************/
	for( i=0; i<ARC_PHS_LEN; i++ )
		line[i] = ' ';

	for (i = 0; i < pEvent->pChanInfo[NumPhs].iNumArrivals; i++)
	{
		if ((pArr = &pEvent->pChanInfo[NumPhs].Arrivals[i]) == NULL)
		{
			logit ("", "Invalid EWEvent passed in -- can't get Arrivals.\n");
			return EW_FAILURE;
		}


		if ((pStaMag = &pEvent->pChanInfo[NumPhs].Stamags[i]) == NULL)
		{
			logit ("", "Invalid EWEvent passed in -- can't get StaMags.\n");
			return EW_FAILURE;
		}



/*----------------------------------------------------------------------------------------------------
Sample HYPOINVERSE station archive card and its shadow card.
  Many fields are blank due to lack of information from binder.
  (phase is 101 chars, including newline; shadow is 93 chars, including newline):
PWM  NC  VHZ  PD0199912312359 5341                                                0      77                 W  \n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 1
$   6 5.49 1.80 7.91 3.30 0.10 PSN0   77 PHP3 1853 39 340 47 245 55 230 63  86 71  70 77  48           \n
-----------------------------------------------------------------------------------------------------*/



		plen = strlen (pArr->szObsPhase);
		slen = strlen (pStation->Sta);
		clen = strlen (pStation->Comp);
		nlen = strlen (pStation->Net);

		/* Fill in the common parts */

		/* Station site code */
		if (slen >= 5) 
			strncpy (line, pStation->Sta, 5);
		else
			strncpy (line, pStation->Sta, slen );

		/* Network code */
		if (nlen >= 2) 
			strncpy (line+5, pStation->Net, 2);
		else
			strncpy (line+5, pStation->Net, nlen);

		line[7] = ' ';

		/* Component Code */
		if (clen == 1)
			strncpy(line+8, pStation->Comp, clen);
		else 
			line[8] = ' ';

		if (clen >= 3)
			strncpy (line+9, pStation->Comp, 3);
		else
			strncpy (line+9, pStation->Comp, clen);

		line[12] = ' ';

		/* Common time  - yyyymmddhhmm */
		sprintf (line+17, "%04d%02d%02d%02d%02d",
							(otm->tm_year + 1900),
							otm->tm_mon+1,
							otm->tm_mday,
							otm->tm_hour,
							otm->tm_min);
		line[29] = ' ';
								

		if ((pArr->szObsPhase[0] == 'P') || (pArr->szObsPhase[0] == 'p')) 
		{ 
			/* Fill in the P-wave information */
	
			/* Make sure there's a pick
			 **************************/
			if (pArr->tObsPhase == 0.0) 
			{
				logit ("", "No pick time found\n");
				return EW_FAILURE;
			}
	
			/* P onset (if any) */
			if (pArr->cOnset != '\0' ) 
				line[13] = pArr->cOnset;
	
			/* P label */
			line[14] = 'P';

			/* P first motion */
			if (pArr->cMotion != '\0' ) 
				line[15] = pArr->cMotion;
	
			/* Assigned P weight code */
			if (pArr->dSigma >= 0)
			{
				if (pArr->dSigma <= 0.02)
					tmpint = 0;
				else if (pArr->dSigma <= 0.03)
					tmpint = 1;
				else if (pArr->dSigma <= 0.05)
					tmpint = 2;
				else if (pArr->dSigma <= 0.08)
					tmpint = 3;
				else 
					tmpint = 4;
	
				line[16] = tmpint + '0';
			}
	
			/* Seconds of P arrival */
			sprintf (line+29, "%5d", (int) (100.0 * (pArr->tObsPhase - Cat)));
			line[34] = ' ';
	
			/* P travel time residual */
			sprintf (line+34, "%4d", (int) MIN((100.0* pArr->tResPick), 9999));
			line[38] = ' ';
	
			/* P weight actually used */
			sprintf (line+38, "%3d", (int) MIN((100.0 * pArr->dWeight), 999));
			line[41] = ' ';
	
			/* epicentral distance */
			if (pArr->dDist != 0.0)
			{
				sprintf (line+74, "%4d", (int) MIN((10.0 * pArr->dDist), 9999));
				line[78] = ' ';
			}
	
			/* emergence angle at source */
			if (pArr->dTakeoff != 0.0)
			{
				sprintf (line+78, "%3d", (int) MIN(pArr->dTakeoff, 999));
				line[81] = ' ';
			}
	
			/* duration magnitude weight code */
			sprintf (line+82, "%1d", (int) MIN(pStaMag->dWeight, 9));
			line[83] = ' ';
	
	
			/* coda duration in seconds */
			if (pStaMag->StaMagUnion.CodaDur.tCodaDurXtp != 0.0)
			{
				sprintf( line+87,  "%4d", (int) MIN(pStaMag->StaMagUnion.CodaDur.tCodaDurXtp, 9999 ) );
				line[91] = ' ';
			}
	
			/* azimuth to station */
			if (pArr->dAzm != 0.0)
			{
				sprintf (line+91, "%3d", (int) MIN(pArr->dAzm, 999));
				line[94] = ' ';
			}
	
			/* duration magnitude for this station */
			if (pStaMag->dMag != 0.0)
			{
				sprintf (line+94, "%3d", (int) MIN((100.0* pStaMag->dMag), 999));
				line[97] = ' ';
			}
	
			/* data source code */
			sprintf( line+108,  "%c",   ' ');
			line[109] = ' ';
	
			sprintf( line+111, "\n\0" );
	
	
	
			/* Build the shadow card
			 ***********************/
			for( j=0; j<ARC_SPHS_LEN; j++ )
				shdw[j] = ' ';
	
			strncpy (shdw, "$", 1);
	
			if (pStaMag->StaMagUnion.CodaDur.tCodaDurXtp != 0.0)
			{
				sprintf (shdw+35, "%5d",  (int) MIN( pStaMag->StaMagUnion.CodaDur.tCodaDurXtp, 99999 ) );
				shdw[40] = ' ';
			}
		
			sprintf (shdw+92, "\n\0");
	
		} 
		else if ((pArr->szObsPhase[0] == 'S') || (pArr->szObsPhase[0] == 's')) 
		{

			/* Fill in the S-wave information */

			/* Make sure there's a pick
			 **************************/
			if (pArr->tObsPhase == 0.0) 
			{
				logit ("", "No pick time found\n");
				return EW_FAILURE;
			}
	
			/* Seconds of S arrival */
			sprintf (line+41, "%5d", (int) (100.0 * (pArr->tObsPhase - Cat)));
			line[41+5] = ' ';

			/* S onset (if any) */
			if (pArr->cOnset != '\0' ) 
				line[46] = pArr->cOnset;
	
			/* S label */
			line[47] = 'S';

			/* blank  (first motion???) */
			line[48] = ' ';

			/* Assigned S weight code */
			if (pArr->dSigma >= 0)
			{
				if (pArr->dSigma <= 0.02)
					tmpint = 0;
				else if (pArr->dSigma <= 0.03)
					tmpint = 1;
				else if (pArr->dSigma <= 0.05)
					tmpint = 2;
				else if (pArr->dSigma <= 0.08)
					tmpint = 3;
				else 
					tmpint = 4;
	
				line[49] = tmpint + '0';
			}
	
	
			/* S travel time residual */
			sprintf (line+50, "%4d", (int) MIN((100.0* pArr->tResPick), 9999));
			line[50+4] = ' ';
	
			/* S weight actually used */
			sprintf (line+63, "%3d", (int) MIN((100.0 * pArr->dWeight), 999));
			line[63+3] = ' ';
	
			/* epicentral distance */
			if (pArr->dDist != 0.0)
			{
				sprintf (line+74, "%4d", (int) MIN((10.0 * pArr->dDist), 9999));
				line[78] = ' ';
			}
	
			/* emergence angle at source */
			if (pArr->dTakeoff != 0.0)
			{
				sprintf (line+78, "%3d", (int) MIN(pArr->dTakeoff, 999));
				line[81] = ' ';
			}
	
			/* duration magnitude weight code */
			sprintf (line+82, "%1d", (int) MIN(pStaMag->dWeight, 9));
			line[83] = ' ';
	
	
			/* azimuth to station */
			if (pArr->dAzm != 0.0)
			{
				sprintf (line+91, "%3d", (int) MIN(pArr->dAzm, 999));
				line[94] = ' ';
			}
	
			/* data source code */
			sprintf( line+108,  "%c",   ' ');
			line[109] = ' ';
	
			sprintf( line+111, "\n\0" );
	
		} /* S-wave portion */
		else
		{

			/* Fill in the UNKNOWN wave portion */
			/* THIS IS A HACK: to accomodate weird NSN 
			 * picks. In real life, we will not be using 
			 * the hypoinverse file format to do this
			 */
	
			/* Make sure there's a pick
			 **************************/
			if (pArr->tObsPhase == 0.0) 
			{
				logit ("", "No pick time found\n");
				return EW_FAILURE;
			}
	
	
			/* P remark such as "IP" */
			if (plen >= 2 )
				strncpy( line+13, pArr->szObsPhase, 2 );
			else if (plen == 1)
				strncpy( line+14, pArr->szObsPhase, plen );

	
			/* P first motion */
			if (pArr->cMotion != '\0' ) 
				line[15] = pArr->cMotion;
	
			/* Assigned P weight code */
			if (pArr->dSigma >= 0)
			{
				if (pArr->dSigma <= 0.02)
					tmpint = 0;
				else if (pArr->dSigma <= 0.03)
					tmpint = 1;
				else if (pArr->dSigma <= 0.05)
					tmpint = 2;
				else if (pArr->dSigma <= 0.08)
					tmpint = 3;
				else 
					tmpint = 4;
	
				line[16] = tmpint + '0';
			}
	
			/* Seconds of P arrival */
			sprintf (line+29, "%5d", (int) (100.0 * (pArr->tObsPhase - Cat)));
			line[34] = ' ';
	
			/* P travel time residual */
			sprintf (line+34, "%4d", (int) MIN((100.0* pArr->tResPick), 9999));
			line[38] = ' ';
	
			/* P weight actually used */
			sprintf (line+38, "%3d", (int) MIN((100.0 * pArr->dWeight), 999));
			line[41] = ' ';
	
			/* epicentral distance */
			if (pArr->dDist != 0.0)
			{
				sprintf (line+74, "%4d", (int) MIN((10.0 * pArr->dDist), 9999));
				line[78] = ' ';
			}
	
			/* emergence angle at source */
			if (pArr->dTakeoff != 0.0)
			{
				sprintf (line+78, "%3d", (int) MIN(pArr->dTakeoff, 999));
				line[81] = ' ';
			}
	
			/* duration magnitude weight code */
			sprintf (line+82, "%1d", (int) MIN(pStaMag->dWeight, 9));
			line[83] = ' ';
	
	
			/* coda duration in seconds */
			if (pStaMag->StaMagUnion.CodaDur.tCodaDurXtp != 0.0)
			{
				sprintf( line+87,  "%4d", (int) MIN(pStaMag->StaMagUnion.CodaDur.tCodaDurXtp, 9999 ) );
				line[91] = ' ';
			}
	
			/* azimuth to station */
			if (pArr->dAzm != 0.0)
			{
				sprintf (line+91, "%3d", (int) MIN(pArr->dAzm, 999));
				line[94] = ' ';
			}
	
			/* duration magnitude for this station */
			if (pStaMag->dMag != 0.0)
			{
				sprintf (line+94, "%3d", (int) MIN((100.0* pStaMag->dMag), 999));
				line[97] = ' ';
			}
	
			/* data source code */
			sprintf( line+108,  "%c",   ' ');
			line[109] = ' ';
	
			sprintf( line+111, "\n\0" );
	
	
			/* Build the shadow card
			 ***********************/
			for( j=0; j<ARC_SPHS_LEN; j++ )
				shdw[j] = ' ';
	
			strncpy (shdw, "$", 1);
	
			if (pStaMag->StaMagUnion.CodaDur.tCodaDurXtp != 0.0)
			{
				sprintf (shdw+35, "%5d",  (int) MIN( pStaMag->StaMagUnion.CodaDur.tCodaDurXtp, 99999 ) );
				shdw[40] = ' ';
			}
		
			sprintf (shdw+92, "\n\0");
		}

	} /* loop over arrivals */

 	 
	
	
	/* Copy both to the target address if there's room
	 *************************************************/
	if ((strlen (arcmsg) + strlen (line) + strlen (shdw) + 1) > (size_t)maxlen) 
	{
		logit ("", "Max arcmsg length exceeded.\n");
		return EW_FAILURE;
	}

	strcat (arcmsg, line);
	strcat (arcmsg, shdw);

	return EW_SUCCESS;

}





/***************************************************************************/
/* BuildTerm() builds a hypoinverse event terminator card & its shadow     */
/***************************************************************************/
int		BuildTerm (int eventid, char *arcmsg, int maxlen)
{
   char line[ARC_TRM_LEN];   /* temporary working place */
   char shdw[ARC_STRM_LEN];   /* temporary shadow card   */
   int  i;

/* Put all blanks into line and shadow card
 ******************************************/
	for (i = 0; i < ARC_TRM_LEN; i++)
		line[i] = ' ';
	for (i = 0; i < ARC_STRM_LEN; i++)
		shdw[i] = ' ';

/* Build terminator
 ******************/
	if (eventid != 0) 
		sprintf (line+62, "%10ld\n\0",  eventid );
	else
		sprintf (line+72, "\n\0");

/* Build shadow card
 *******************/
	sprintf (shdw, "$");
	shdw[1] = ' ';
	if (eventid != 0) 
		sprintf (shdw+62, "%10ld\n\0",  eventid);
	else
		sprintf (shdw+72, "\n\0");

/* Copy both to the target address if there's room
 *************************************************/
	if ((strlen (arcmsg) + strlen (line) + strlen (shdw) + 1) > (size_t)maxlen) 
	{
		logit ("", "Max length (%d) of ARC message exceeded.\n", maxlen);
		return EW_FAILURE;
	}

	strcat (arcmsg, line);
	strcat (arcmsg, shdw);
	return EW_SUCCESS;

}



