
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: report.c,v 1.1 2000/02/14 19:06:49 lucky Exp $
 *
 *    Revision history:
 *     $Log: report.c,v $
 *
 *     Revision 1.3  2013/05/07  nromeu
 *     Reports proxies computation
 *     Get line length before encoding line. This avoids a buffer overflow 
 *     Logit of pick, coda and proxies messages 
 * 
 *     Revision 1.2  2013/04/26 16:09:00  nromeu
 *     ReportProxies
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 */

  /*********************************************************************************
   *                              report.c                                         *
   *                                                                               *
   *                 Pick and coda buffering functions                             *
   *                                                                               *
   *  This file contains functions ReportPick(), ReportCoda().ReportProxies()      *
   ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <chron3.h>
#include <transport.h>
#include "pick_ew.h"

/* Function prototypes
   *******************/
int GetPickIndex( void );

     /**************************************************************
      *                         ReportPick()                       *
      *                                                            *
      *                       Report one pick                      *
      **************************************************************/

void ReportPick( PICK *Pick, CODA *Coda, PROXIES *Proxies, STATION *Sta, GPARM *Gparm, EWH *Ewh )
{
   MSG_LOGO     logo;      /* Logo of message to send to the output ring */
   int          lineLen;
   struct Greg  g;
   int          tsec, thun;
   char         line[100];
   int          PickIndex;

/* Get the pick index and the SNC (station, network, component).
   They will be reported later, with the coda.
   ************************************************************/
   PickIndex = GetPickIndex();
   Coda->PickIndex = PickIndex;
   strcpy( Coda->sta,  Sta->sta );
   strcpy( Coda->net,  Sta->net );
   strcpy( Coda->chan, Sta->chan );
   // NR /***/
   Proxies->PickIndex = PickIndex;
   strcpy( Proxies->sta,  Sta->sta );
   strcpy( Proxies->net,  Sta->net );
   strcpy( Proxies->chan, Sta->chan );

/* Convert julian seconds to date and time
   ***************************************/
   datime( Pick->time, &g );
   tsec = (int)floor( (double) g.second );
   thun = (int)((100.*(g.second - tsec)) + 0.5);
   if ( thun == 100 )
      tsec++, thun = 0;

/* Convert pick to ASCII
   *********************/
   sprintf( line,              "%3d",   (int) Ewh->TypePick2k );
   sprintf( line+strlen(line), "%3d",   (int) Gparm->MyModId );
   sprintf( line+strlen(line), "%3d ",  (int) Ewh->MyInstId );
   sprintf( line+strlen(line), "%4d ",  PickIndex );
   sprintf( line+strlen(line), "%-5s",  Sta->sta );
   sprintf( line+strlen(line), "%-2s",  Sta->net );
   sprintf( line+strlen(line), "%-3s",  Sta->chan );
   sprintf( line+strlen(line), " %c",  Pick->FirstMotion ); 
   sprintf( line+strlen(line), "%1d  ", Pick->weight );

   sprintf( line+strlen(line), "%4d%02d%02d%02d%02d%02d.%02d", g.year,
            g.month, g.day, g.hour, g.minute, tsec, thun );

   sprintf( line+strlen(line), "%8.0lf",   Pick->xpk[0] );
   sprintf( line+strlen(line), "%8.0lf",   Pick->xpk[1] );
   sprintf( line+strlen(line), "%8.0lf\n", Pick->xpk[2] );

   lineLen = strlen(line);

/* Print the pick
   **************/
 #if defined(_OS2) || defined(_WINNT)
   printf( "%s", line );
#endif

/* Send the pick to the output ring
   ********************************/
   logo.type   = Ewh->TypePick2k;
   logo.mod    = Gparm->MyModId;
   logo.instid = Ewh->MyInstId;

   if ( tport_putmsg( &Gparm->OutRegion, &logo, lineLen, line ) != PUT_OK )
      logit( "et", "pick_ew: Error sending pick to output ring.\n" );

   // imprimir el MSG_PICK  //NR
   logit( "t", "pick: %s", line );
}


  /**************************************************************
   *                        ReportCoda()                        *
   *                                                            *
   *                       Report one coda                      *
   **************************************************************/

void ReportCoda( CODA *Coda, GPARM *Gparm, EWH *Ewh )
{
   MSG_LOGO     logo;      /* Logo of message to send to the output ring */
   int          lineLen;
   char         line[100];

/* Convert coda to ASCII
   *********************/
   sprintf( line,              "%3d",  (int) Ewh->TypeCoda2k );
   sprintf( line+strlen(line), "%3d",  (int) Gparm->MyModId );
   sprintf( line+strlen(line), "%3d ", (int) Ewh->MyInstId );
   sprintf( line+strlen(line), "%4d ", Coda->PickIndex );
   sprintf( line+strlen(line), "%-5s", Coda->sta );
   sprintf( line+strlen(line), "%-2s", Coda->net );
   sprintf( line+strlen(line), "%-3s", Coda->chan );
   sprintf( line+strlen(line), "%8d",  Coda->aav[0] );
   sprintf( line+strlen(line), "%8d",  Coda->aav[1] );
   sprintf( line+strlen(line), "%8d",  Coda->aav[2] );
   sprintf( line+strlen(line), "%8d",  Coda->aav[3] );
   sprintf( line+strlen(line), "%8d",  Coda->aav[4] );
   sprintf( line+strlen(line), "%8d",  Coda->aav[5] );
   sprintf( line+strlen(line), "%4d \n", Coda->len_out );
   lineLen = strlen(line);

/* Print the coda
   **************/
#if defined(_OS2) || defined(_WINNT)
   printf( "%s", line );
#endif

/* Send the coda to the output ring
   ********************************/
   logo.type   = Ewh->TypeCoda2k;
   logo.mod    = Gparm->MyModId;
   logo.instid = Ewh->MyInstId;

   if ( tport_putmsg( &Gparm->OutRegion, &logo, lineLen, line ) != PUT_OK )
      logit( "et", "pick_ew: Error sending coda to output ring.\n" );

   // imprimir el MSG_CODA  //NR
   logit( "t", "coda: %s", line );
}



  /**************************************************************
   *                        ReportProxies()                     *
   *                                                            *
   *                       Report proxies for one Tau0          *
   **************************************************************/

void ReportProxies( PROXIES *Proxies, GPARM *Gparm, EWH *Ewh ) /***/ //NR
{
   MSG_LOGO     logo;      /* Logo of message to send to the output ring */
   int          lineLen;
   char         line[100];

/* Convert proxies to ASCII
   *********************/
   sprintf( line,    "%3d%3d%3d %4d %-5s%-2s%-3s %3.0f %9.6lf %1.6E \n",  
			(int) Ewh->TypeProxies, (int) Gparm->MyModId, (int) Ewh->MyInstId, 
			Proxies->PickIndex, Proxies->sta, Proxies->net, Proxies->chan,
			Proxies->tau0, Proxies->tauc, Proxies->pd );

   lineLen = strlen(line);

/* Print the proxies
   **************/
#if defined(_OS2) || defined(_WINNT)
   printf( "%s", line );
#endif

/* Send the coda to the output ring
   ********************************/
   logo.type   = Ewh->TypeProxies;
   logo.mod    = Gparm->MyModId;
   logo.instid = Ewh->MyInstId;

   if ( tport_putmsg( &Gparm->OutRegion, &logo, lineLen, line ) != PUT_OK )
      logit( "et", "pick_ew: Error sending proxies to output ring.\n" );

   // imprimir el MSG_PROXIES  //NR
   logit( "t", "proxies: %s", line );
}
