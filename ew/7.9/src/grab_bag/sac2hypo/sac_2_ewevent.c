/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sac_2_ewevent.c,v 1.1 2002/09/10 17:20:59 dhanych Exp $
 *
 *    Revision history:
 *     $Log: sac_2_ewevent.c,v $
 *     Revision 1.1  2002/09/10 17:20:59  dhanych
 *     Initial revision
 *
 *     Revision 1.6  2002/03/22 20:02:38  lucky
 *     Pre v6.1 checkin
 *
 *     Revision 1.5  2001/07/27 20:41:58  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2001/07/01 21:55:23  davidk
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
 *     Revision 1.3  2001/06/21 21:25:00  lucky
 *     State of the code after LocalMag review was implemented and partially tested.
 *
 *     Revision 1.2  2001/06/06 20:54:46  lucky
 *     Changes made to support multitude magnitudes, as well as amplitude picks. This is w
 *     in progress - checkin in for the sake of safety.
 *
 *     Revision 1.1  2001/05/15 02:15:27  davidk
 *     Initial revision
 *
 *     Revision 1.5  2001/04/17 17:30:00  davidk
 *     changed a bad subscript in a logging statement, due to a compiler warning on NT.
 *
 *     Revision 1.4  2001/04/16 18:31:41  lucky
 *     Added a check for a valid coda length before assining the SAC header
 *     value to a db structure entry.
 *
 *     Revision 1.3  2001/01/19 18:04:23  lucky
 *     Added a check for mag label "DurCoda" as well as "Md" before looking
 *     for a coda length
 *
 *     Revision 1.2  2001/01/08 19:05:06  lucky
 *     Fixed a bug in ProduceSAC_NextStationForEvent whereby only one
 *     pick out of many would be written to the SAC header.
 *
 *     Revision 1.1  2000/12/18 18:55:12  lucky
 *     Initial revision
 *
 *     Revision 1.12  2000/12/06 17:50:47  lucky
 *     We now correctly keep track of the pick onset
 *
 *     Revision 1.11  2000/11/08 18:46:33  lucky
 *     Modified SAC_NextStationforEvent: let's stop pretending - we only
 *     produce sac for the first Waveform of any SCN.
 *
 *     Revision 1.10  2000/09/13 15:36:51  lucky
 *     Fixed FirstMotion character handling;
 *     Fixed handling of the coda length
 *
 *     Revision 1.9  2000/09/07 21:17:45  lucky
 *     Final version after the Review pages have been demonstrated.
 *
 *     Revision 1.6  2000/08/31 16:10:24  lucky
 *     Before mallocing snippet buffer, we need to calculate correct iByteLen field.
 *
 *     Revision 1.5  2000/08/30 18:42:51  lucky
 *     Added malloc of binSnippet before data is copied from the sacdata buffer
 *     to the EWEvent structure.
 *
 *     Revision 1.4  2000/08/30 17:41:57  lucky
 *     InitEWEvent has been changed to include optional allocation of
 *     pChanInfo space. This must be optional because GetEWEventInfo mallocs
 *     space on the fly.
 *
 *     Revision 1.3  2000/08/25 18:24:45  lucky
 *     Fixed to work with S and P arrivals, as well as the new, explicitly
 *     allocated pChanInfo.
 *
 *     Revision 1.2  2000/08/19 00:10:12  lucky
 *     Made SAC_reftime and SAC_Ktostr non-static -- needed in other modules
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

#include <earthworm.h>
#include <sac_2_ewevent.h>

extern		time_t timegm_ew (struct tm *);

double	SAC_reftime (struct SAChead *);
size_t	SAC_Ktostr (char *, int, char *);


/* Global Variable */
float WriteSAC_dGapThresh=1.5;


/********************************************************
*
*  Sac2EWEvent:
*      Fills the EWEventInfoStruct structure pointed 
*      to by pEvent, with the data from the SAC header 
*      pointed to by sacheadp, with data for one
*      channel of trace data.
*
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    04/2000
*
********************************************************/
int 	Sac2EWEvent (EWEventInfoStruct *pEvent, struct SAChead *sacheadp,
					char *sacdatap, int CopyData)
{

	double 				tref;
	int 				i, tmp, toalloc;
	int					wt;
	EWDB_OriginStruct	*pOrigin;
	EWDB_ArrivalStruct	*pArrival;
	EWDB_StationMagStruct	*pStaMag;
	EWDB_WaveformStruct	*pWaveform;
	EWDB_StationStruct	*pStation;
	EWChannelDataStruct	*pChan;
	EWChannelDataStruct	*pOldChan;



	if ((pEvent == NULL) || (sacheadp == NULL))
	{
		logit ("", "Invalid parameters passed in.\n");
		return EW_FAILURE;
	}


	/* Check how many channels we processed */
	if (pEvent->iNumChans >= pEvent->iNumAllocChans)
	{
		toalloc = (int) (pEvent->iNumAllocChans + CHAN_ALLOC_INCR);

		logit ("", "Max Channels (%d) reached. Alloc to %d\n",
									pEvent->iNumAllocChans, toalloc);

		pOldChan = pEvent->pChanInfo;
		if ((pEvent->pChanInfo = (EWChannelDataStruct *) malloc
							(toalloc * sizeof (EWChannelDataStruct))) == NULL)
		{
			logit ("",  "Could not malloc %d pChanInfo entries.\n", toalloc);
			return  (EW_FAILURE);
		}

		memcpy (pEvent->pChanInfo, pOldChan,
				(pEvent->iNumAllocChans * sizeof (EWChannelDataStruct)));

		for (tmp = pEvent->iNumChans; tmp < toalloc; tmp++)
			InitEWChan(&pEvent->pChanInfo[tmp]);

		pEvent->iNumAllocChans = toalloc;

		free (pOldChan);

	}


	/* Origin structure
	 *********************/
	if ((pOrigin = &(pEvent->PrefOrigin)) == NULL)
	{
		logit ("", "Invalid pEvent -- can't get PrefOrigin.\n");
		return EW_FAILURE;
	}


	/* Make sure that we have origin time
	 **************************************/
	if (sacheadp->o == (float) SACUNDEF)
	{
		logit ("", "No origin time in SAC header.\n");
		return EW_FAILURE;
	}

	/* Absolute event origin time
	 ******************************/
	if ((tref = SAC_reftime (sacheadp)) < 0)
	{
		logit ("", "Call to SAC_reftime failed.\n");
		return EW_FAILURE;
	}

	pOrigin->tOrigin = (double) sacheadp->o + tref;

	/* Fill in the event location 
	 *****************************/

	/* Lattitude */
	if (sacheadp->evla != (float) SACUNDEF)
		pOrigin->dLat = sacheadp->evla;

	/* Longitude */
	if (sacheadp->evlo != (float) SACUNDEF)
		pOrigin->dLon = sacheadp->evlo;

	/* Depth */
	if (sacheadp->evdp != (float) SACUNDEF)
		pOrigin->dDepth = sacheadp->evdp;




	/******* Channel Data *******/

	if ((pChan = &(pEvent->pChanInfo[pEvent->iNumChans])) == NULL)
	{
		logit ("", "Invalid pEvent -- can't get pChanInfo[%d].\n", pEvent->iNumChans);
		return EW_FAILURE;
	}
	
	pChan->iNumArrivals = 0;
	pChan->iNumWaveforms = 1;
	
	
	/******* Station struct *******/
	
	if ((pStation = &(pChan->Station)) == NULL)
	{
		logit ("", "Invalid EWChannel -- can't get Station.\n");
		return EW_FAILURE;
	}

	SAC_Ktostr (pStation->Sta,  7, sacheadp->kstnm);
	SAC_Ktostr (pStation->Comp, 9, sacheadp->kcmpnm);
	SAC_Ktostr (pStation->Net,  9, sacheadp->knetwk);

	/* Station Latitude */
	if (sacheadp->stla != (float) SACUNDEF)
		pStation->Lat = sacheadp->stla;

	/* Station Longitude */
	if (sacheadp->stlo != (float) SACUNDEF)
		pStation->Lon = sacheadp->stlo;

	/* Station Elevation */
	if (sacheadp->stel != (float) SACUNDEF)
		pStation->Elev = sacheadp->stel;
	

	/* P-wave information */
	if (sacheadp->a != (float) SACUNDEF)
	{
		i = pChan->iNumArrivals;


		/******* Arrival struct *******/
		if ((pArrival = &(pChan->Arrivals[i])) == NULL)
		{
			logit ("", "Invalid EWChannel; can't get P-wave Arrivals.\n");
			return EW_FAILURE;
		}
	
		pArrival->pStation = pStation;

		/* Arrival time */
		if ((tref = SAC_reftime (sacheadp)) < 0)
		{
			logit ("", "Call to SAC_reftime failed.\n");
			return EW_FAILURE;
		}
	
		pArrival->tCalcPhase = (double) sacheadp->a + tref;
		pArrival->tObsPhase = (double) sacheadp->a + tref;
		pArrival->szObsPhase[0] = sacheadp->ka[0];
		pArrival->szObsPhase[1] = sacheadp->ka[1];
		pArrival->szObsPhase[2] = sacheadp->ka[2];
		pArrival->szObsPhase[3] = '\0';
		pArrival->cMotion = sacheadp->ka[1];
		pArrival->cOnset = sacheadp->ka[3];
	
		/* Convert weight code into dSigma */
		wt = sacheadp->ka[2] - '0';
	
		if (wt == 3)
			pArrival->dSigma = 0.08;
		else if (wt == 2)
			pArrival->dSigma = 0.05;
		else if (wt == 1)
			pArrival->dSigma = 0.03;
		else if (wt == 0)
			pArrival->dSigma = 0.02;
		else
			pArrival->dSigma = 99.99;
			
	
		pArrival->dDist = sacheadp->dist;
		pArrival->dAzm = sacheadp->az;
	
		/******* StaMag struct *******/
	
		if ((pStaMag = &(pChan->Stamags[i])) == NULL)
		{
			logit ("", "Invalid EWChannel -- can't get StaMags.\n");
			return EW_FAILURE;
		}
	
		if (sacheadp->f != (float) SACUNDEF)
		{
			pStaMag->StaMagUnion.CodaDur.tCodaDurXtp = sacheadp->f - pArrival->tCalcPhase + tref;
			pStaMag->StaMagUnion.CodaDur.tCodaTermXtp = pArrival->tCalcPhase + 
										pStaMag->StaMagUnion.CodaDur.tCodaDurXtp;
		}
		else
		{
			pStaMag->StaMagUnion.CodaDur.tCodaDurXtp = 0.0;
			pStaMag->StaMagUnion.CodaDur.tCodaTermXtp = pArrival->tCalcPhase;
		}
		
		pStaMag->iMagType = MAGTYPE_DURATION;

		pChan->iNumArrivals = pChan->iNumArrivals + 1;
		pChan->iNumStaMags = pChan->iNumStaMags + 1;
	}

	
	/* S-wave information */
	if (sacheadp->t0 != (float) SACUNDEF)
	{
		i = pChan->iNumArrivals;


		/******* Arrival struct *******/
		if ((pArrival = &(pChan->Arrivals[i])) == NULL)
		{
			logit ("", "Invalid EWChannel; can't get S-wave Arrivals.\n");
			return EW_FAILURE;
		}
	
		pArrival->pStation = pStation;

		/* Arrival time */
		if ((tref = SAC_reftime (sacheadp)) < 0)
		{
			logit ("", "Call to SAC_reftime failed.\n");
			return EW_FAILURE;
		}
	
		pArrival->tCalcPhase = (double) sacheadp->t0 + tref;
		pArrival->tObsPhase = (double) sacheadp->t0 + tref;
		pArrival->szObsPhase[0] = sacheadp->kt0[0];
		pArrival->szObsPhase[1] = sacheadp->kt0[1];
		pArrival->szObsPhase[2] = sacheadp->kt0[2];
		pArrival->szObsPhase[3] = '\0';
		pArrival->cMotion = sacheadp->kt0[1];
		pArrival->cOnset = sacheadp->kt0[3];
	
		/* Convert weight code into dSigma */
		wt = sacheadp->kt0[2] - '0';
	
		if (wt == 3)
			pArrival->dSigma = 0.08;
		else if (wt == 2)
			pArrival->dSigma = 0.05;
		else if (wt == 1)
			pArrival->dSigma = 0.03;
		else if (wt == 0)
			pArrival->dSigma = 0.02;
		else
			pArrival->dSigma = 99.99;
			
	
		pArrival->dDist = sacheadp->dist;
		pArrival->dAzm = sacheadp->az;
	

		/******* StaMag struct *******/
	
		if ((pStaMag = &(pChan->Stamags[i])) == NULL)
		{
			logit ("", "Invalid EWChannel -- can't get StaMags.\n");
			return EW_FAILURE;
		}
	
		if (sacheadp->f != (float) SACUNDEF)
		{
			pStaMag->StaMagUnion.CodaDur.tCodaDurXtp = sacheadp->f - pArrival->tCalcPhase + tref;
			pStaMag->StaMagUnion.CodaDur.tCodaTermXtp = pArrival->tCalcPhase + 
										pStaMag->StaMagUnion.CodaDur.tCodaDurXtp;
		}
		else
		{
			pStaMag->StaMagUnion.CodaDur.tCodaDurXtp = 0.0;
			pStaMag->StaMagUnion.CodaDur.tCodaTermXtp = 0.0;
		}

		pStaMag->iMagType = MAGTYPE_DURATION;
		
		pChan->iNumArrivals = pChan->iNumArrivals + 1;
		pChan->iNumStaMags = pChan->iNumStaMags + 1;

	}


	if (pChan->iNumArrivals == 0)
	{
		logit ("", "Warning: No picks found in SAC header.\n");
	}


	/******* Waveform struct *******/
	if (CopyData == 1)
	{
		if ((pWaveform = &(pChan->Waveforms[0])) == NULL)
		{
			logit ("", "Invalid EWChannel -- can't get Waveform.\n");
			return EW_FAILURE;
		}
	
		/* Allocate space of the snippet data */
		pWaveform->iByteLen = sacheadp->npts * sizeof (float);
        if ((pWaveform->binSnippet = malloc (pWaveform->iByteLen)) == NULL)
        {
            logit ("", "Could not malloc binSnippet.\n");
            return EW_FAILURE;
        }

		memcpy (pWaveform->binSnippet, sacdatap, ((size_t) sacheadp->npts * sizeof (float)));
	}


	return EW_SUCCESS;
}






/********************************************************
*
*  SacHeaderInit:
*      Initialize the SAC header structure pointed to 
*      by sacheadp -- be sure to malloc it before 
*      calling this function.
*
*      Also sets default (constant) values in the header
*   
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    04/2000
*
********************************************************/
int 	SacHeaderInit (struct SAChead *sacheadp)
{

	int i;
	struct SAChead2 *head2;   	/* use a simple structure here - we */
								/* don't care what * the variables are - */
								/* set them to 'undefined' */
	if (sacheadp == NULL)
	{
		logit ("", "Invalid parameters passed in.\n");
		return EW_FAILURE;
	}

	/* change to a simpler format */
	head2 = (struct SAChead2 *) sacheadp;

	/*    set all of the floats to 'undefined'    */
	for (i = 0; i < NUM_FLOAT; ++i) 
		head2->SACfloat[i] = SACUNDEF;

	/*    set all of the ints to 'undefined'  */
	for (i = 0; i < MAXINT-5; ++i) 
		head2->SACint[i] = SACUNDEF;

	/*    except for the logical integers - set them to 1 */
	for ( ; i < MAXINT; ++i) 
		head2->SACint[i] = 1;

	/*    set all of the strings to 'undefined'   */
	for (i = 0; i < MAXSTRING; ++i) 
		(void) strncpy(head2->SACstring[i], SACSTRUNDEF,K_LEN);

	/*    SAC I.D. number */
	head2->SACfloat[9] = SAC_I_D;

	/*    header version number */
	head2->SACint[6] = SACVERSION;


	/* Set default values that remain constant */
	sacheadp->b      = 0;				/* beginning time relative to reference time */
	sacheadp->iftype = SAC_ITIME;		/* File type is time series */
	sacheadp->leven  = 1;				/* evenly spaced data */
	sacheadp->idep   = SAC_IUNKN;		/* unknown independent data type */
	sacheadp->iztype = SAC_IBEGINTIME;	/* Reference time is Begin time */
	sacheadp->ievtyp = SAC_IUNKN;		/* event type */

	/* Label strings */
	strncpy(sacheadp->ko, "Origin ", K_LEN);
	strncpy(sacheadp->ka, "Arrival ", K_LEN);
	strncpy(sacheadp->kt0, "S-Wave ", K_LEN);

	return EW_SUCCESS;

}



/******************************************************
 * SAC_reftime() reads the reference time in the SAC  *
 *   header and returns it in double seconds since    *
 *   January 1, 1970 00:00:00 GMT                     *
 *   Returns: -1 on error                             *
 ******************************************************/
double SAC_reftime( struct SAChead *psac )
{
   struct tm stm;
   time_t    tt;
   double    tref;

/* Find seconds from 1970.01.01 to Jan 1 of the correct year
 ***********************************************************/
   stm.tm_year  = (int) psac->nzyear - 1900l;  /* tm_year = yrs since 1900 */
   stm.tm_mon   = 0;                           /* January */
   stm.tm_mday  = 1;                           /* 1st */
   stm.tm_hour  = (int) psac->nzhour;
   stm.tm_min   = (int) psac->nzmin;
   stm.tm_sec   = (int) psac->nzsec;
   stm.tm_isdst = 0;
   stm.tm_wday  = 0;                           /* not used by timegm */
   stm.tm_yday  = 0;                           /* not used by timegm */

	tt = timegm_ew (&stm);

	if (tt < 0) 
	{
		logit ("", "Call to timegm_ew failed.\n");
		return (double) tt;
	}

/* Then add the number of seconds for the day-of-year plus milliseconds.
 ***********************************************************************/
   tref = (double)tt +                  /* sec since 1970 to Jan 1 this year */
          86400.*(psac->nzjday-1) +     /* sec since Jan 1 in this year      */
          (double)psac->nzmsec/1000.;   /* milliseconds */
  
   return( tref );
}


/****************************************************************
 * SAC_Ktostr() convert a SAC header K-field (which is filled   *
 *   with white-space) into a null-terminated character string  *
 *   Returns the length of the new string.                      *
 ****************************************************************/
size_t SAC_Ktostr( char *s1, int len1, char *s2 )
{
   int i;
   char tmp[K_LEN+1];

/* NULL-fill the target
 **********************/
   for( i=0; i<len1; i++ ) s1[i]='\0';

/* Is the K-field NULL ? 
 ***********************/
   strncpy( tmp, s2, K_LEN ); 
   tmp[K_LEN] = '\0';
   if( strcmp( tmp, SACSTRUNDEF ) == 0 )  return( 0 );

/* Null terminate after last non-space character 
 ***********************************************/
   for( i=K_LEN-1; i>=0; i-- )     /* start at the end of the string */
   {
     if( tmp[i]!=' ' &&  tmp[i]!='\0' )  /* find last non-space char */
     {
        tmp[i+1]='\0';   /* null-terminate after last non-space char */
        break;
     }
   }
   if( i<0 ) return( 0 );  /* K-field was empty */

   strncpy( s1, tmp, len1-1 );
   return( strlen( s1 ) );
}



/************************************************************************* 
 *                            WriteSAC_init()                            *
 *       Initializes the SAC writing environment.  Uses the SACPABase    *
 *        routines for writing SAC data                                  *
 *************************************************************************/
int WriteSAC_init(char * szOutDir,char * szOutputFormat, int bDebug)
{
  /* Initialize SAC Putaway Base   environment */
  
   if(bDebug)
     SACPABase_Debug(TRUE);
   return(SACPABase_init(MAX_SNIPPET_LEN+SACHEADERSIZE, szOutDir,TRUE,szOutputFormat));
}


/************************************************************************* 
 *                            WriteSAC_shutdown()                        *
 *       Shuts down the SAC writing environment.  Uses the SACPABase     *
 *        routines for writing SAC data                                  *
 *************************************************************************/
int WriteSAC_shutdown(void)
{
  /* Close SAC Putaway Base   environment */
  
   return(SACPABase_close());
}

/************************************************************************* 
 *                            WriteSAC_StartEvent ()                     *
 *       Initializes a SAC event, and records data that is related to    *
 *        the entire event.                                              *
 *        Uses the SACPABase routines for writing SAC data.              *
 *************************************************************************/
int WriteSAC_Event_BAD (EWEventInfoStruct * pEvent, int WriteHtml)
{
  int rc,i;

  rc=WriteSAC_StartEvent(pEvent);
  if(rc != EW_SUCCESS)
  {
    logit("","WriteSAC_StartEvent() failed with rc=%d for idEvent %d\n",
          rc,pEvent->Event.idEvent);
    return(EW_FAILURE);
  }

  for(i=0;i<pEvent->iNumChans;i++)
  {

	if (WriteHtml == 1)
	{
		logit ("t", "STARTING CHANNEL %d of %d\n", 
					i, pEvent->iNumChans);
		printf (" ");
	}

    rc=ProduceSAC_NextStationForEvent(&(pEvent->pChanInfo[i]));
    if(rc != EW_SUCCESS)
    {
      logit("","ProduceSAC_NextStationForEvent() failed with rc=%d "
            "for chan record %d of %d.\n",
        rc,i,pEvent->iNumChans);
    }


    rc=WriteSAC_NextStationForEvent(&(pEvent->pChanInfo[i]));
    if(rc != EW_SUCCESS)
    {
      logit("","WriteSAC_NextStationForEvent() failed with rc=%d "
            "for chan record %d of %d.\n",
        rc,i,pEvent->iNumChans);
    }
  }  /* end for i < iNumChans */

  return(WriteSAC_EndEvent());
}

/************************************************************************* 
 *                            WriteSAC_StartEvent ()                     *
 *       Initializes a SAC event, and records data that is related to    *
 *        the entire event.                                              *
 *        Uses the SACPABase routines for writing SAC data.              *
 *************************************************************************/
int WriteSAC_StartEvent(EWEventInfoStruct * pEvent)
{
  char szEventID[20];
  SAC_OriginStruct SAC_Origin;

  sprintf(szEventID,"%d",pEvent->Event.idEvent);
  
  SAC_Origin.dElev=pEvent->PrefOrigin.dDepth;
  SAC_Origin.dLat=pEvent->PrefOrigin.dLat;
  SAC_Origin.dLon=pEvent->PrefOrigin.dLon;
  SAC_Origin.tOrigin=pEvent->PrefOrigin.tOrigin;

  return(SACPABase_next_ev(szEventID,SAC_Origin.tOrigin,&SAC_Origin));
}

/************************************************************************* 
 *                            WriteSAC_EndEvent ()                       *
 *       Marks the end of a SAC event.                                   *
 *        Uses the SACPABase routines for writing SAC data.              *
 *************************************************************************/
int WriteSAC_EndEvent(void)
{
  return(SACPABase_end_ev());
}

/************************************************************************* 
 *                            WriteSAC_StartEvent ()                     *
 *       Initializes a SAC event, and records data that is related to    *
 *        the entire event.                                              *
 *        Uses the SACPABase routines for writing SAC data.              *
 *************************************************************************/
int ProduceSAC_NextStationForEvent(EWChannelDataStruct * pChannel)
{
  int i,j,rc;
  TRACE_REQ TR;
  SAC_ArrivalStruct AS;
  SAC_StationStruct SS;
  SAC_ResponseStruct RS;
  EWDB_ArrivalStruct * pArrival;
  EWDB_WaveformStruct * pWaveform;
  /*  SAC_AmpPickStruct AmpPick;
      EWDB_StationMagStruct * pStaMag; */


	EWDB_StationStruct * pStation=&(pChannel->Station);
		  

		  if(pChannel->iNumWaveforms < 1)
		  {
			logit("","ProduceSAC_NextStationForEvent() No waveforms for %s,%s,%s!  Not much"
					  " point in creating a SAC file for that station!\n",
					  pStation->Sta,pStation->Comp,pStation->Net);
			return(EW_FAILURE);
		  }


		  pWaveform = &(pChannel->Waveforms[0]);
		  if(pWaveform->iByteLen <= 0)
		  {
			logit("","ProduceSAC_NextStationForEvent() Waveform for "
				  "%s,%s,%s is 0 bytes long.  \nNot much"
			  " point in creating a SAC file for that station!\n",
			  pStation->Sta,pStation->Comp,pStation->Net);
			return(EW_FAILURE);
		  }

		  SACPABase_next_scnl(pStation->Sta,pStation->Comp,pStation->Net, "--");

		  strcpy(TR.sta,pStation->Sta);
		  strcpy(TR.chan,pStation->Comp);
		  strcpy(TR.net,pStation->Net);

		  TR.reqStarttime = TR.actStarttime = pWaveform->tStart;
		  TR.reqEndtime = TR.actEndtime = pWaveform->tEnd;
		  TR.pBuf=pWaveform->binSnippet;
		  TR.bufLen = TR.actLen = pWaveform->iByteLen;

		  /* Note we are not setting the samprate parameter!!!!!!
			 Hopefully this does not create a problem.  We don't
			 want to have to go through all of the header byte
			 swapping here.  davidk 000303
			 Maybe we could just get it from pResponse
		  *******************************************************/

		  rc=SACPABase_write_trace(&TR,WriteSAC_dGapThresh);

		  if(rc != EW_SUCCESS)
		  {
			logit("","SACPABase_write_trace() failed with rc=%d for %s,%s,%s\n",
				  rc,pStation->Sta,pStation->Comp,pStation->Net);
			return(EW_FAILURE);
		  }

		  /* Write arrival picks */
		  for (i = 0; i < pChannel->iNumArrivals; i++)
		  {
				if ((pArrival = &pChannel->Arrivals[i]) == NULL)
				{
					logit ("", "Invalid EWChannel; Can't get P-wave Arrival.\n");
					return (EW_FAILURE);
				}

				if ((pArrival->szObsPhase[0] == 'P') || 
						(pArrival->szObsPhase[0] == 'p'))
				{
				  AS.tPhase=pArrival->tObsPhase;
				  AS.cPhase='P';
				  AS.dDist=pArrival->dDist;
				  AS.dAzm=pArrival->dAzm;
				  AS.cFMotion=pArrival->cMotion;
				  AS.cOnset=pArrival->cOnset;
		  
				  if(pArrival->dSigma  <=0.02)
					AS.iPhaseWt=0;
				  else if(pArrival->dSigma  <=0.03)
					AS.iPhaseWt=1;
				  else if(pArrival->dSigma  <=0.05)
					AS.iPhaseWt=2;
				  else if(pArrival->dSigma  <=0.08)
					AS.iPhaseWt=3;
				  else
					AS.iPhaseWt=4;


					/* Find the coda length */
					AS.dCodaLen=0;
					for (j = 0; j < pChannel->iNumStaMags  && !AS.dCodaLen; j++)
					{
						if (pChannel->Stamags[j].iMagType == MAGTYPE_DURATION)
						{
							AS.dCodaLen = pChannel->Stamags[j].StaMagUnion.CodaDur.tCodaDurXtp;
						}
					}

					rc=SACPABase_write_parametric(&AS, PWAVE);
					if(rc != EW_SUCCESS)
					{
						logit("","SACPABase_write_parametric (PWAVE) \n"
								"failed with rc=%d for %s,%s,%s\n",
								rc,pStation->Sta,pStation->Comp,pStation->Net);
						return(EW_FAILURE);
					}

				} /* Processing Pwave */

				else if ((pArrival->szObsPhase[0] == 'S') || 
							(pArrival->szObsPhase[0] == 's'))
				{
				  AS.tPhase=pArrival->tObsPhase;
				  AS.cPhase='S';
				  AS.dDist=pArrival->dDist;
				  AS.dAzm=pArrival->dAzm;
				  AS.cFMotion=pArrival->cMotion;
				  AS.cOnset=pArrival->cOnset;
		  
				  if(pArrival->dSigma  <=0.02)
					AS.iPhaseWt=0;
				  else if(pArrival->dSigma  <=0.03)
					AS.iPhaseWt=1;
				  else if(pArrival->dSigma  <=0.05)
					AS.iPhaseWt=2;
				  else if(pArrival->dSigma  <=0.08)
					AS.iPhaseWt=3;
				  else
					AS.iPhaseWt=4;

					/* Find the coda length */
					AS.dCodaLen=0;
					for (j = 0; j < pChannel->iNumStaMags  && !AS.dCodaLen; j++)
					{
						if (pChannel->Stamags[j].iMagType == MAGTYPE_DURATION)
						{
							AS.dCodaLen = pChannel->Stamags[j].StaMagUnion.CodaDur.tCodaDurXtp;
						}
					}

					rc=SACPABase_write_parametric(&AS, SWAVE);
					if(rc != EW_SUCCESS)
					{
						logit("","SACPABase_write_parametric (SWAVE) \n"
								"failed with rc=%d for %s,%s,%s\n",
								rc,pStation->Sta,pStation->Comp,pStation->Net);
						return(EW_FAILURE);
					}

				} /* Processing Swave */

				else
				{
					logit ("", "Unknown arrival type ***%c***; continuing.\n",
									pArrival->szObsPhase[0]);
				}

		  } /* loop over arrivals */

		  SS.dLat=pStation->Lat;
		  SS.dLon=pStation->Lon;
		  SS.dElev=pStation->Elev;
		  SS.bResponseIsValid=pChannel->bResponseIsValid;

		  if(pChannel->bResponseIsValid)
		  {

			if ((pChannel->ResponseInfo.tfsFunc.iNumPoles > 0) || 
				(pChannel->ResponseInfo.tfsFunc.iNumZeroes > 0))
			{
				SS.pResponse=&RS;
				RS.dGain=pChannel->ResponseInfo.dGain;
				RS.iNumPoles=pChannel->ResponseInfo.tfsFunc.iNumPoles;
				RS.iNumZeroes=pChannel->ResponseInfo.tfsFunc.iNumZeroes;
				/* This assumes that SAC_PZNum and EWDBNum are of the
			       same size and layout
			    *****************************************/
			    memcpy(&(RS.Poles),&(pChannel->ResponseInfo.tfsFunc.Poles),
							           RS.iNumPoles * sizeof(RS.Poles[0]));
			    memcpy(&(RS.Zeroes),&(pChannel->ResponseInfo.tfsFunc.Zeroes),
									           RS.iNumZeroes * sizeof(RS.Zeroes[0]));
			}
		  }

  rc=SACPABase_write_stainfo(&SS);
  if(rc != EW_SUCCESS)
  {
    logit("","ProduceSAC_NextStationForEvent():SACPABase_write_stainfo() \n"
          "failed with rc=%d for %s,%s,%s\n",
          rc,pStation->Sta,pStation->Comp,pStation->Net);
    return(EW_FAILURE);
  }

  
  return(EW_SUCCESS);
}

/************************************************************************* 
 *                            WriteSACEvent ()                           *
 *        Write the information from the sac header to the file          *
 *************************************************************************/
int WriteSAC_NextStationForEvent(EWChannelDataStruct * pChannel)
{

	int		rc;

  rc=SACPABase_end_scnl();

  if(rc != EW_SUCCESS)
  {
    logit("","ProduceSAC_NextStationForEvent():SACPABase_end_scn() \n"
          "failed with rc=%d for %s,%s,%s\n",
          rc, pChannel->Station.Sta, pChannel->Station.Comp,
			pChannel->Station.Net);
    return(EW_FAILURE);
  }

	return EW_SUCCESS;
}


int WriteSAC_Set_dGapThresh(float IN_dGapThresh)
{
  WriteSAC_dGapThresh=IN_dGapThresh;
  return(EW_SUCCESS);
}

