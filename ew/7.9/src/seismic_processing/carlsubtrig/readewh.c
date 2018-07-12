
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readewh.c 1458 2004-05-11 17:49:07Z lombard $
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
 * readcnfg.c: Read the Earthworm parameters.
 *              1) Set members of the CSUEWH structure.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: ReadEWH						*/
/*									*/
/*	Inputs:		Pointer to a CarlSubTrig Network parameter	*/
/*			  structure					*/
/*									*/
/*	Outputs:	Updated input structure(above)			*/
/*			Error messages to stderr			*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in carlsubtrig.h on 	*/
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
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: ReadEWH						*/
int ReadEWH( NETWORK* csuNet )
{
  int	retVal = 0;	/* Return value for this function		*/

  if ( CT_FAILED( GetLocalInst( &(csuNet->csuEwh.myInstId) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting myInstId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetModId( csuNet->csuParam.myModName, 
				 &(csuNet->csuEwh.myModId) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting myModId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetInst( csuNet->csuParam.readInstName,
	&(csuNet->csuEwh.readInstId) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting readInstId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetModId( csuNet->csuParam.readModName,
	&(csuNet->csuEwh.readModId) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting readModId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_CARLSTATRIG_SCNL",
				&(csuNet->csuEwh.typeCarlStaTrig) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting TYPE_CARLSTATRIG_SCNL.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_TRIGLIST_SCNL",
				&(csuNet->csuEwh.typeTrigList) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting TYPE_TRIGLIST_SCNL.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_ERROR", &(csuNet->csuEwh.typeError) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting typeError.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_HEARTBEAT",
				&(csuNet->csuEwh.typeHeartbeat) ) ) )
  {
    fprintf( stderr, "CarlSubTrig: Error getting typeHeartbeat.\n" );
    retVal = ERR_EWH_READ;
  }

  return ( retVal );
}
