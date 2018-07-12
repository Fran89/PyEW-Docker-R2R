
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initsta.c,v 1.0  2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: initsta.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
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
/*	Inputs:		Pointer to a Cont_Trig station structure	*/
/*									*/
/*	Outputs:	Updated station structure(above)		*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in cont_trig.h on	*/
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
/*	Cont_Trig Includes						*/
/*******							*********/
#include "cont_trig.h"

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
