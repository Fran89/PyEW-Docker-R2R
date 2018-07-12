
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: prodstatrg.c 1534 2004-05-31 17:59:31Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/31 17:57:55  lombard
 *     Removed version string from CARLSTATRIG_SCNL message.
 *
 *     Revision 1.2  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * prodstatrg.c: Produce a station trigger message.
 *              1) Create the trigger message.
 *              2) Send the trigger message to the output ring.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ProduceStationTrigger                                 */
/*                                                                      */
/*      Inputs:         Pointer to a CarlTrig station list              */
/*                      Pointer to the World structure                  */
/*                                                                      */
/*      Outputs:        Message sent to the output ring                 */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure to produce station trigger message      */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* strcat, strcmp, strlen                       */
#include <sys/types.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */
#include <transport.h>  /* MSG_LOGO, SHM_INFO                           */

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ProduceStationTrigger                                 */
int ProduceStationTrigger( STATION* station, WORLD* cstWorld )
{
  char          staLine[MAXLINELEN];    /* Station output line.         */
  int           retVal = 0;     /* Return value for this function.      */

  sprintf( staLine, "%s %s %s %s %14.4lf %14.4lf %ld %.2lf", 
           station->staCode,
           station->compCode,
           station->netCode,
	   station->locCode,
           station->trigOnTime,
           station->trigOffTime,
           station->trigCount,
           station->trigEta);
  

  /*    Send the output message to the output ring              */
  cstWorld->outLogo.type   = cstWorld->cstEWH->typeCarlStaTrig;
  if ( tport_putmsg( &(cstWorld->regionOut), &(cstWorld->outLogo), 
                     strlen( staLine ), staLine ) != PUT_OK )
  {
    logit( "et", "carlStaTrig: Error producing a trigger message.\n" );
    /* This is bad; not much point in carlstatrig continuing */
    retVal = ERR_PROD_MSG;
  }

  if ( cstWorld->cstParam.debug > 2 )
    logit( "t", "trigger message: %s\n", staLine );
  else if ( cstWorld->cstParam.debug > 1 )
  {
    if ( station->trigger == TRIG_ON )
      logit("t", "%s.%s.%s.%s triggered on: %.1lf at %8.2lf count: %ld\n",
            station->staCode,
            station->compCode,
            station->netCode,
	    station->locCode,
            station->trigEta,
            station->trigOnTime,
            station->trigCount);
    else 
      logit("t", "%s.%s.%s.%s triggered off at: %8.2lf (on: %8.2lf) count: %ld\n",
            station->staCode,
            station->compCode,
            station->netCode,
	    station->locCode,
            station->trigOffTime,
            station->trigOnTime,
            station->trigCount);
  }
  return ( retVal );
}
