
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readewh.c,v 1.0  2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: readewh.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
 *     Initial revision
 *
 *
 */

/*
 * readcnfg.c: Read the Earthworm parameters.
 *              1) Set members of the CONTEWH structure.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: ReadEWH						*/
/*									*/
/*	Inputs:		Pointer to a Cont_Trig Network parameter	*/
/*			  structure					*/
/*									*/
/*	Outputs:	Updated input structure(above)			*/
/*			Error messages to stderr			*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in cont_trig.h on 	*/
/*			  failure					*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* GetInst, GetLocalInst, GetModId, GetType	*/

/*******							*********/
/*	Cont_Trig Includes						*/
/*******							*********/
#include "cont_trig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: ReadEWH						*/
int ReadEWH( NETWORK* contNet )
{
  int	retVal = 0;	/* Return value for this function		*/

  if ( CT_FAILED( GetLocalInst( &(contNet->contEwh.myInstId) ) ) )
  {
    fprintf( stderr, "Cont_Trig: Error getting myInstId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetModId( contNet->contParam.myModName, 
				 &(contNet->contEwh.myModId) ) ) )
  {
    fprintf( stderr, "Cont_Trig: Error getting myModId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_TRIGLIST_SCNL",
				&(contNet->contEwh.typeTrigList) ) ) )
  {
    fprintf( stderr, "Cont_Trig: Error getting TYPE_TRIGLIST_SCNL.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_ERROR", &(contNet->contEwh.typeError) ) ) )
  {
    fprintf( stderr, "Cont_Trig: Error getting typeError.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_HEARTBEAT",
				&(contNet->contEwh.typeHeartbeat) ) ) )
  {
    fprintf( stderr, "Cont_Trig: Error getting typeHeartbeat.\n" );
    retVal = ERR_EWH_READ;
  }

  return ( retVal );
}
