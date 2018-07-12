
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: report.c 5342 2013-02-04 14:23:25Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2008/03/28 18:20:31  paulf
 *     added in PickIndexDir option to specify where pick indexes get stuffed
 *
 *     Revision 1.5  2007/04/27 17:38:54  paulf
 *     patched report to not send ANY codas if NoCoda param is set
 *
 *     Revision 1.4  2006/11/17 18:24:02  dietz
 *     Changed index file name from pick_ew.ndx to pick_ew_MMM.ndx where
 *     MMM is the moduleid of pick_ew. This will allow multiple instances
 *     of pick_ew to run without competing for the same index file.
 *
 *     Revision 1.3  2004/05/28 18:46:06  kohler
 *     Pick times are now rounded to nearest hundred'th of a second, as in
 *     Earthworm v6.2.  The milliseconds are hard wired to 0 in the output
 *     pick value.  The new pick_scnl message type requires a milliseconds value.
 *
 *     Revision 1.2  2004/04/29 22:44:52  kohler
 *     Pick_ew now produces new TYPE_PICK_SCNL and TYPE_CODA_SCNL messages.
 *     The station list file now contains SCNLs, rather than SCNs.
 *     Input waveform messages may be of either TYPE_TRACEBUF or TYPE_TRACEBUF2.
 *     If the input waveform message is of TYPE_TRACEBUF (without a location code),
 *     the location code is assumed to be "--".  WMK 4/29/04
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

  /**********************************************************************
   *                              report.c                              *
   *                                                                    *
   *                 Pick and coda buffering functions                  *
   *                                                                    *
   *  This file contains functions ReportPick(), ReportCoda().          *
   **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <chron3.h>
#include <transport.h>
#include "pick_ew.h"

static char line[LINELEN];   /* Buffer to hold picks and codas */

/* Function prototypes
   *******************/
int GetPickIndex( unsigned char modid , char * dir);  /* function in index.c */


     /**************************************************************
      *               ReportPick() - Report one pick.              *
      **************************************************************/

void ReportPick( PICK *Pick, CODA *Coda, STATION *Sta, GPARM *Gparm, EWH *Ewh )
{
   MSG_LOGO    logo;      /* Logo of message to send to output ring */
   int         lineLen;
   struct Greg g;
   int         tsec, thun;
   int         PickIndex;
   char        firstMotion = Pick->FirstMotion;

/* Get the pick index and the SNC (station, network, component).
   They will be reported later, with the coda.
   ************************************************************/
   PickIndex = GetPickIndex( Gparm->MyModId , Gparm->PickIndexDir);
   Coda->PickIndex = PickIndex;
   strcpy( Coda->sta,  Sta->sta );
   strcpy( Coda->net,  Sta->net );
   strcpy( Coda->chan, Sta->chan );
   strcpy( Coda->loc,  Sta->loc );

/* Convert julian seconds to date and time.
   Round pick time to nearest hundred'th
   of a second.
   ***************************************/
   datime( Pick->time, &g );
   tsec = (int)floor( (double) g.second );
   thun = (int)((100.*(g.second - tsec)) + 0.5);
   if ( thun == 100 )
      tsec++, thun = 0;

/* First motions aren't allowed to be blank
   ****************************************/
   if ( firstMotion == ' ' ) firstMotion = '?';

/* Convert pick to space-delimited text string.
   This is a bit risky, since the buffer could overflow.
   Milliseconds are always set to zero.
   ****************************************************/
   sprintf( line,              "%d",  (int) Ewh->TypePickScnl );
   sprintf( line+strlen(line), " %d", (int) Gparm->MyModId );
   sprintf( line+strlen(line), " %d", (int) Ewh->MyInstId );
   sprintf( line+strlen(line), " %d", PickIndex );
   sprintf( line+strlen(line), " %s", Sta->sta );
   sprintf( line+strlen(line), ".%s", Sta->chan );
   sprintf( line+strlen(line), ".%s", Sta->net );
   sprintf( line+strlen(line), ".%s", Sta->loc );

   sprintf( line+strlen(line), " %c%d", firstMotion, Pick->weight );

   sprintf( line+strlen(line), " %4d%02d%02d%02d%02d%02d.%02d0",
            g.year, g.month, g.day, g.hour, g.minute, tsec, thun );

   sprintf( line+strlen(line), " %d", (int)(Pick->xpk[0] + 0.5) );
   sprintf( line+strlen(line), " %d", (int)(Pick->xpk[1] + 0.5) );
   sprintf( line+strlen(line), " %d", (int)(Pick->xpk[2] + 0.5) );
   strcat( line, "\n" );
   lineLen = strlen(line);

/* Print the pick
   **************/
 #if defined(_OS2) || defined(_WINNT)
   printf( "%s", line );
#endif

/* Send the pick to the output ring
   ********************************/
   logo.type   = Ewh->TypePickScnl;
   logo.mod    = Gparm->MyModId;
   logo.instid = Ewh->MyInstId;

   if ( tport_putmsg( &Gparm->OutRegion, &logo, lineLen, line ) != PUT_OK )
      logit( "et", "pick_ew: Error sending pick to output ring.\n" );
   return;
}


  /**************************************************************
   *               ReportCoda() - Report one coda.              *
   **************************************************************/

void ReportCoda( CODA *Coda, GPARM *Gparm, EWH *Ewh )
{
   MSG_LOGO logo;      /* Logo of message to send to output ring */
   int      lineLen;

   if (Gparm->NoCoda) {  
		/* do not pub any codas! */
		if (Gparm->Debug) {
			logit("t", "Debug: NoCoda set, not reporting %s.%s.%s.%s",
				Coda->sta, Coda->chan, Coda->net, Coda->loc);
		}
		return; 
   }
   if (Gparm->NoCodaHorizontal && Coda->chan[2] != 'Z') {  
		/* do not pub any codas on Horizontals! */
		if (Gparm->Debug) {
			logit("t", "Debug: NoCodaHorizontal set, not reporting %s.%s.%s.%s",
				Coda->sta, Coda->chan, Coda->net, Coda->loc);
		}
		return; 
   }

/* Convert coda to space-delimited text string.
   This is a bit risky, since the buffer could overflow.
   ****************************************************/
   sprintf( line,              "%d",  (int) Ewh->TypeCodaScnl );
   sprintf( line+strlen(line), " %d", (int) Gparm->MyModId );
   sprintf( line+strlen(line), " %d", (int) Ewh->MyInstId );
   sprintf( line+strlen(line), " %d", Coda->PickIndex );
   sprintf( line+strlen(line), " %s", Coda->sta );
   sprintf( line+strlen(line), ".%s", Coda->chan );
   sprintf( line+strlen(line), ".%s", Coda->net );
   sprintf( line+strlen(line), ".%s", Coda->loc );
   sprintf( line+strlen(line), " %d", Coda->aav[0] );
   sprintf( line+strlen(line), " %d", Coda->aav[1] );
   sprintf( line+strlen(line), " %d", Coda->aav[2] );
   sprintf( line+strlen(line), " %d", Coda->aav[3] );
   sprintf( line+strlen(line), " %d", Coda->aav[4] );
   sprintf( line+strlen(line), " %d", Coda->aav[5] );
   sprintf( line+strlen(line), " %d", Coda->len_out );
   strcat( line, "\n" );
   lineLen = strlen(line);

/* Print the coda
   **************/
#if defined(_OS2) || defined(_WINNT)
   printf( "%s", line );
#endif

/* Send the coda to the output ring
   ********************************/
   logo.type   = Ewh->TypeCodaScnl;
   logo.mod    = Gparm->MyModId;
   logo.instid = Ewh->MyInstId;

   if ( tport_putmsg( &Gparm->OutRegion, &logo, lineLen, line ) != PUT_OK )
      logit( "et", "pick_ew: Error sending coda to output ring.\n" );
   return;
}
