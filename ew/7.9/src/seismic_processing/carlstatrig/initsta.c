
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initsta.c 1532 2004-05-31 17:57:19Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/31 17:57:18  lombard
 *     Minor bug fixes
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
 * initsta.c: Initialize a station structure.
 *              1) Initialize members of the STATION structure.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: InitializeStation                                     */
/*                                                                      */
/*      Inputs:         Pointer to a CarlStaTrig station structure      */
/*                                                                      */
/*      Outputs:        Updated station structure(above)                */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* malloc, memset                               */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: InitializeStation                                     */
int InitializeStation( STATION* station )
{
  int   retVal = 0;     /* Return value for this function.              */

  /*    Initialize the members of station                               */
  station->compCode[0] = '\0';
  station->netCode[0] = '\0';
  station->staCode[0] = '\0';
  station->locCode[0] = '\0';
  station->dataType = UNKNOWN;

  /*    Zero out the trace buffer                                       */
  memset( station->traceBuf, 0, BUFFSIZE );

  /*    Zero numerical members                                          */
  station->holdLTA = 0.0;
  station->holdSTA = 0.0;
  station->sampleRate = 0.0;
  station->channelNum = 0;
  station->dataSize = 0;
  station->buffSamps = 0;
  station->buffRefTime = 0.0;
  station->calcSamps = 0;
  station->calcSecs = 0;
  station->numSSR = 0;
  station->trigger = TRIG_OFF;
  station->trigOnTime = 0.0;
  station->trigOffTime = 0.0;
  station->trigEta = 0.0;
  station->trigCount = 0;
  
  return ( retVal );
}
