
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: resetsta.c 1449 2004-05-05 23:54:04Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/05 23:54:03  lombard
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
 * resetsta.c: Reset a station structure due to an extended gap.
 *              1) Reset the members of the STATION structure.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ResetStation                                          */
/*                                                                      */
/*      Inputs:         Pointer to a CarlStaTrig station structure      */
/*                                                                      */
/*      Outputs:        Updated station structure(above)                */
/*                                                                      */
/*      Returns:        nothing                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <stdlib.h>     /* malloc                                       */
#include <string.h>     /* memset                                       */

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

/*      Function: ResetStation                                          */
void ResetStation( STATION* station )
{
  /*    Zero out the trace buffer                                       */
  memset( station->traceBuf, 0, MAX_TRACEBUF_SIZ );

  /*    Zero some of the numerical members                              */
  station->holdLTA = 0.0;
  station->holdSTA = 0.0;
  station->sampleRate = 0.0;
  station->dataSize = 0;
  station->buffSamps = 0;
  station->buffRefTime = 0.0;
  station->calcSamps = 0;
  station->calcSecs = 0;
  station->numSSR = 0;

  /*    Set the data type member                                        */
  station->dataType = UNKNOWN;

  /*    Initialize the current trigger to "off"                         */
  station->trigger = TRIG_OFF;

  return;
}
