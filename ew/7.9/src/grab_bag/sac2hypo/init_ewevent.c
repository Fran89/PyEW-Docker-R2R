/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: init_ewevent.c,v 1.2 2003/01/30 23:13:48 lucky Exp $
 *
 *    Revision history:
 *     $Log: init_ewevent.c,v $
 *     Revision 1.2  2003/01/30 23:13:48  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2002/06/28 21:06:22  lucky
 *     Initial revision
 *
 *     Revision 1.3  2001/06/18 18:42:30  lucky
 *     Changed Ml to ML
 *
 *     Revision 1.2  2001/06/06 20:54:46  lucky
 *     Changes made to support multitude magnitudes, as well as amplitude picks. This is w
 *     in progress - checkin in for the sake of safety.
 *
 *     Revision 1.1  2001/05/15 02:15:27  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/04/17 17:49:28  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/12/18 18:55:12  lucky
 *     Initial revision
 *
 *     Revision 1.6  2000/12/06 17:50:47  lucky
 *     We now correctly keep track of the pick onset
 *
 *     Revision 1.5  2000/09/07 21:17:45  lucky
 *     Final version after the Review pages have been demonstrated.
 *
 *     Revision 1.3  2000/08/30 17:41:57  lucky
 *     InitEWEvent has been changed to include optional allocation of
 *     pChanInfo space. This must be optional because GetEWEventInfo mallocs
 *     space on the fly.
 *
 *     Revision 1.2  2000/08/30 14:56:28  lucky
 *     pChanInfo will be allocated dynamically - no need to initialize
 *     statically allocated pChanInfo structures any more.
 *
 *     Revision 1.1  2000/08/29 18:09:31  lucky
 *     Initial revision
 *
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <earthworm.h>
#include <ew_event_info.h>

/******************************************************************
*
* Initialize the EventInfo struct -- set everything to NULL. 
*  Initialize the first chunk of pChan structs.
*
* WARNING!! This function uses the EW return codes not EWDB return codes!
*
******************************************************************/
int		InitEWEvent (EWEventInfoStruct *pEventInfo)
{

	int		i;

	if (pEventInfo == NULL)
	{
		logit ("", "Invalid arguments passed in\n");
		return EW_FAILURE;
	}

	pEventInfo->GotLocation = FALSE;
	pEventInfo->GotTrigger = FALSE;

	pEventInfo->iNumChans = 0;
	pEventInfo->iNumMags = 0;
	pEventInfo->iPrefMag = -10;
	pEventInfo->iMd = -1;
	pEventInfo->iML = -1;
	memset (&pEventInfo->Event, 0, sizeof(EWDB_EventStruct));
	memset (&pEventInfo->Mags, 0, MAX_MAGS_PER_ORIGIN * sizeof(EWDB_MagStruct));
	memset (&pEventInfo->PrefOrigin, 0, sizeof(EWDB_OriginStruct));
	memset (&pEventInfo->CoincEvt, 0, sizeof(EWDB_CoincEventStruct));


	if ((pEventInfo->pChanInfo = (EWChannelDataStruct *) malloc
				(INIT_NUM_CHANS * sizeof (EWChannelDataStruct))) == NULL)
	{
		logit ("",  "Could not malloc %d pChanInfo entries.\n", INIT_NUM_CHANS);
		return  (EW_FAILURE);
	}

	pEventInfo->iNumAllocChans = INIT_NUM_CHANS;
	for (i = 0; i < INIT_NUM_CHANS; i++)
	{
		InitEWChan(&pEventInfo->pChanInfo[i]);
	}

	return EW_SUCCESS;

}  /* end InitEWEvent() */


/***********************************************************
*
* Initialize a single EWChannelDataStruct structure.
*
* WARNING!! This function uses the EW return codes not EWDB return codes!
*
***********************************************************/
int	InitEWChan(EWChannelDataStruct *pChan)
{
	int		i;

	if (pChan == NULL)
	{
		logit ("", "Invalid argument passed in.\n");
		return EW_FAILURE;
	}

	pChan->idChan = 0;
	pChan->iNumArrivals = 0;
	pChan->iNumStaMags = 0;
	pChan->iNumTriggers = 0;
	pChan->iNumWaveforms = 0;
	pChan->bResponseIsValid = 0;

	memset (&pChan->Station, 0, sizeof(EWDB_StationStruct));
	memset (&pChan->ResponseInfo, 0, sizeof(EWDB_ChanTCTFStruct));

	for (i = 0; i < MDPCPE; i++)
	{
		memset (&pChan->Waveforms[i], 0, sizeof(EWDB_WaveformStruct));
		memset (&pChan->Arrivals[i], 0, sizeof(EWDB_ArrivalStruct));
		memset (&pChan->Stamags[i], 0, sizeof(EWDB_StationMagStruct));
		memset (&pChan->Triggers[i], 0, sizeof(EWDB_TriggerStruct));
	}

	return EW_SUCCESS;
}  /* end InitEWChan() */



void PrintEventInfo(EWEventInfoStruct *pEvent)
{
	int		i, j;


	if (pEvent == NULL)
	{
		logit ("", "Event is NULL.\n");
		exit (0);
	}

	if (pEvent->Event.iEventType == EWDB_EVENT_TYPE_COINCIDENCE)
	{
		logit ("", "PRINT EVENT %d: This is a coincidence event.\n",
				pEvent->Event.idEvent);

		logit ("", "idC=%d, time=%0.2f, source=<%s><%s>\n",
							pEvent->CoincEvt.idCoincidence,
							pEvent->CoincEvt.tCoincidence,
							pEvent->CoincEvt.szSource,
							pEvent->CoincEvt.szHumanReadable);
	}
	else
	{
		logit ("", "PRINT EVENT %d: This is a located event.\n",
				pEvent->Event.idEvent);

		logit ("", "Got %d magnitudes, iMd=%d, iML=%d, iPrefMag=%d\n", 
					pEvent->iNumMags, pEvent->iMd, pEvent->iML, pEvent->iPrefMag);
		for (i = 0; i < pEvent->iNumMags; i++)
			logit ("", "Mag %d (%d): type=%d, value=%f\n", i, 
					pEvent->Mags[i].idMag, 
					pEvent->Mags[i].iMagType, 
					pEvent->Mags[i].dMagAvg);
	}
	
		logit ("", "Got %d channels:\n", pEvent->iNumChans);
		for (i = 0; i < pEvent->iNumChans; i++)
		{
			logit ("", "Chan %d (%d) - %s.%s.%s: picks=%d, waves=%d, stamags=%d, triggers=%d\n", i,
   		                 pEvent->pChanInfo[i].Station.idChan,
   		                 pEvent->pChanInfo[i].Station.Sta,
   		                 pEvent->pChanInfo[i].Station.Comp,
   		                 pEvent->pChanInfo[i].Station.Net,
   		                 pEvent->pChanInfo[i].iNumArrivals,
   		                 pEvent->pChanInfo[i].iNumWaveforms,
   		                 pEvent->pChanInfo[i].iNumStaMags,
   		                 pEvent->pChanInfo[i].iNumTriggers);
		
			    for (j = 0; j < pEvent->pChanInfo[i].iNumArrivals; j++)
				{
					logit ("", "Arr %d (%d) -- %c: time=%f, sigma=%f\n", j,
									pEvent->pChanInfo[i].Arrivals[j].idPick,
									pEvent->pChanInfo[i].Arrivals[j].szObsPhase[0],
									pEvent->pChanInfo[i].Arrivals[j].tObsPhase,
									pEvent->pChanInfo[i].Arrivals[j].dSigma);
				}
			
			   	 for (j = 0; j < pEvent->pChanInfo[i].iNumStaMags; j++)
   				 {
   			    	logit ("", "StaMag %d: type=%d, mag=%0.1f ==> ", j,
   			                     pEvent->pChanInfo[i].Stamags[j].iMagType,
   			                     pEvent->pChanInfo[i].Stamags[j].dMag);
			
					if (pEvent->pChanInfo[i].Stamags[j].iMagType == MAGTYPE_DURATION)
						logit ("", "DURATION, codadur=%f\n", 
						pEvent->pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaDurXtp);
					else
						logit ("", "BODY, tAmp1=%f, dAmp1=%f, dAmpPeriod1=%f\n",
								pEvent->pChanInfo[i].Stamags[j].StaMagUnion.PeakAmp.tAmp1,
								pEvent->pChanInfo[i].Stamags[j].StaMagUnion.PeakAmp.dAmp1,
								pEvent->pChanInfo[i].Stamags[j].StaMagUnion.PeakAmp.dAmpPeriod1);
  			 	}
			   	 for (j = 0; j < pEvent->pChanInfo[i].iNumTriggers; j++)
   				 {
   			    	logit ("", "Trigger %d: time=%0.2f\n", j,
								pEvent->pChanInfo[i].Triggers[j].tTrigger);
				}
	
		} /* loop over chans */
}
