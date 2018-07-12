
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initsta.c 1458 2004-05-11 17:49:07Z lombard $
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
 * initsta.c: Initialize a station structure.
 *              1) Initialize members of the STATION structure.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: InitializeSubtion					*/
/*									*/
/*	Inputs:		Pointer to a CarlSubTrig station structure	*/
/*									*/
/*	Outputs:	Updated station structure(above)		*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in carlsubtrig.h on	*/
/*			failure						*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>

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

/*	Function: InitializeStation					*/
int InitializeStation( STATION* station )
{
  int	retVal = 0;	/* Return value for this function.		*/

  /*	Initialize the members of station				*/
  station->compCode[0] = '\0';
  station->netCode[0] = '\0';
  station->staCode[0] = '\0';
  station->locCode[0] = '\0';
  
  /*	Zero numerical members						*/
  station->onTime = 0.0;
  station->saveOnTime = 0.0;
  station->onEta = 0.0;
  station->listMe = 0;
  station->saveOnEta = 0.0;
  station->countDown = 0;
  station->nextIn = 0;
  station->nextOut = 0;
  station->triggers = (STATRIG*) 0;
  
  return ( retVal );
}
