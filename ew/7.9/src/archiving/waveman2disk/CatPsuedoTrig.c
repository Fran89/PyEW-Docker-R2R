
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: CatPsuedoTrig.c 1629 2004-07-16 20:47:13Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/07/16 20:47:13  lombard
 *     Modified to provide minimal support for SEED location codes.
 *     waveman2disk can now query both SCN (old) and SCNL (new) wave_serverV
 *
 *     Revision 1.2  2001/04/12 04:26:55  lombard
 *      Added checking for buffer overflow; cleaned up comments
 *
 *     Revision 1.1  2000/02/14 20:02:23  lucky
 *     Initial revision
 *
 *
 */

/******************************************************************************
  CatPsuedoTrig( char *TrgMsg, SCNL *Station, nStation, MinDur )

  subroutine to use pscnl strings from a station file to cat trigger messages
             onto the end of TrgMsg.  Suitable for feeding TrgMsg into
             parseSnippet.
*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <earthworm.h>
#include <parse_trig.h>
#include "waveman2disk.h"

int CatPsuedoTrig (char *TrgMsg, SCNL *Station, int nStation, int MinDur)
{
    SNIPPET Snppt, MaxSnppt; 
    char*  nxtSnippetPtr;
    char line[MAXTXT];
    int i;
    char phase[] = "P";
    double minStart;
    int maxDur;
    int len, lenLeft;
  
    nxtSnippetPtr = TrgMsg;

    minStart = -1000000;
    maxDur = MinDur;
    lenLeft = MAX_TRIG_BYTES - 1 - strlen(TrgMsg);
  
    /* parse the trig message so we can see the start times */
    while( parseSnippet(TrgMsg, &Snppt, &nxtSnippetPtr) == EW_SUCCESS ) {
	/* use least (earliest) start time of all triggers */
	if (minStart < Snppt.starttime) {
	    strcpy(MaxSnppt.startYYYYMMDD, Snppt.startYYYYMMDD);
	    strcpy(MaxSnppt.startHHMMSS, Snppt.startHHMMSS);
	    minStart = Snppt.starttime;
	}
	/* maxDur is a configured minimum duration which we'll replace if the
	   trigger is long enough */
	if (maxDur < Snppt.duration) maxDur = Snppt.duration;
    }
    
    if ( minStart < 0.0 ) {
	logit ("e", "CatPseudoTrig: failed to parse trigger message\n");
	return EW_FAILURE;
    }
    
    /* add on to the trigger message */
    for(i=0; i < nStation; i++) {
	sprintf( line, " %s %s %s %s %s %s %s UTC save: %s %s      %d\n",
		 Station[i].sta, Station[i].chan, Station[i].net, 
		 Station[i].loc, phase,
		 MaxSnppt.startYYYYMMDD, MaxSnppt.startHHMMSS,
		 MaxSnppt.startYYYYMMDD, MaxSnppt.startHHMMSS, maxDur);
	
	if ( (len = strlen(line)) < lenLeft) {
	    strcat(TrgMsg, line);
	    lenLeft -= len;
	}
	else {
	    logit("e", 
		  "CatPseudoTrig: out of space for TrigStations; proceding without them\n");
	    return( EW_SUCCESS);
	}
	
    }
    
    return(EW_SUCCESS);
}


