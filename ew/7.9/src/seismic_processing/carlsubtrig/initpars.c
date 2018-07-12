
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initpars.c 1458 2004-05-11 17:49:07Z lombard $
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
 * initpars.c: Initialize parameter structures used by CarlSubTrig.
 *              1) Initialize members of the CSUEWH structure.
 *              2) Initialize members of the NETWORK structure.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: InitializeParameters					*/
/*									*/
/*	Inputs:		Pointer to a CarlSubTrig network parameter	*/
/*			  structure					*/
/*									*/
/*	Outputs:	Updated inputs(above)				*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in carlsubtrig.h on 	*/
/*			failure						*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: InitializeParameters					*/
int InitializeParameters( NETWORK* csuNet )
{
  int	retVal = 0;	/* Return value for this function		*/

  /*	Initialize members of the CSUEWH structure			*/
  csuNet->csuEwh.myInstId = 0;
  csuNet->csuEwh.myModId = 0;
  csuNet->csuEwh.readInstId = 0;
  csuNet->csuEwh.readModId = 0;
  csuNet->csuEwh.typeCarlStaTrig = 0;
  csuNet->csuEwh.typeTrigList = 0;
  csuNet->csuEwh.typeError = 0;
  csuNet->csuEwh.typeHeartbeat = 0;

  /*	Initialize members of the NETWORK structure			*/
  csuNet->MyPid = getpid();
  if( csuNet->MyPid == -1 )
  {
    logit("e","carlsubtrig: Cannot get pid. Exiting.\n");
    return ERR_INIT;
  }
  csuNet->stations = NULL;
  csuNet->subnets = NULL;
  csuNet->channels = NULL;
  csuNet->nSta = 0;
  csuNet->nSub = 0;
  csuNet->nSlots = 0;
  csuNet->nChan = 0;
  csuNet->PreEventTime = 20;
  csuNet->NetTrigDur = 60;
  csuNet->subnetContrib = 30;
  csuNet->DefaultStationDur = 120;
  csuNet->MaxDur = 500;
  csuNet->duration = 0;
  csuNet->onTime = 0;
  csuNet->latency = 5;
  csuNet->numSubAll = 0;
  csuNet->listSubnets = 0;
  csuNet->eventID = 1;
  csuNet->terminate = 0;
  csuNet->numSub = 0;
  csuNet->compAsWild = 0;
  
  /*	Initialize members of the CSUPARAM structure			*/
  csuNet->csuParam.debug = 0;
  sprintf( csuNet->csuParam.readInstName, "INST_WILDCARD" );
  sprintf( csuNet->csuParam.readModName, "MOD_WILDCARD" );
  csuNet->csuParam.ringIn[0] = '\0';
  csuNet->csuParam.ringOut[0] = '\0';
  csuNet->csuParam.staFile[0] = '\0';
  csuNet->csuParam.subFile[0] = '\0';
  csuNet->csuParam.heartbeatInt = 15;
  csuNet->csuParam.ringInKey = 0;
  csuNet->csuParam.ringOutKey = 0;

  /* Set default length for the trigger message buffer			*/
  csuNet->trigMsgBufLen = MAX_BYTES_PER_EQ;
  
  return ( retVal );
}
