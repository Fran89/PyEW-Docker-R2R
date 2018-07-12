
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rmstatrig.c 1458 2004-05-11 17:49:07Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * rmstatrig.c: Remove station trigger
 *              1) Removes a trigger message from the station's stack
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: RemoveStaTrig						*/
/*									*/
/*	Inputs:		Pointer to station				*/
/*			Pointer to trigger				*/
/*									*/
/*	Outputs:	nothing						*/
/*									*/
/*	Returns:	nothing						*/

/*******							*********/
/*	System Includes							*/
/*******							*********/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: RemoveStaTrig						*/
void RemoveStaTrig( STATION* sta, STATRIG* trig, int nSlots )
{
  trig->onTime = 0.0;
  trig->offTime = 0.0;
  trig->onEta = 0.0;
  trig->sequence = -1;
  sta->nextOut++;
  if ( sta->nextOut >= nSlots )
    sta->nextOut -= nSlots;
}
