
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initpars.c,v 1.0  2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: initpars.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
 *     Initial revision
 *
 *
 */

/*
 * initpars.c: Initialize parameter structures used by Cont_Trig.
 *              1) Initialize members of the CONTEWH structure.
 *              2) Initialize members of the NETWORK structure.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: InitializeParameters					*/
/*									*/
/*	Inputs:		Pointer to a Cont_Trig network parameter	*/
/*			  structure					*/
/*									*/
/*	Outputs:	Updated inputs(above)				*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in cont_trig.h on 	*/
/*			failure						*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <string.h>

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/

/*******							*********/
/*	Cont_Trig Includes						*/
/*******							*********/
#include "cont_trig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: InitializeParameters					*/
int InitializeParameters( NETWORK* contNet )
{
  int	retVal = 0;	/* Return value for this function		*/

  /*	Initialize members of the CONTEWH structure			*/
  contNet->contEwh.myInstId = 0;
  contNet->contEwh.myModId = 0;
  contNet->contEwh.typeTrigList = 0;
  contNet->contEwh.typeError = 0;
  contNet->contEwh.typeHeartbeat = 0;

  /*	Initialize members of the NETWORK structure			*/
  contNet->MyPid = getpid();
  if( contNet->MyPid == -1 )
  {
    logit("e","cont_trig: Cannot get pid. Exiting.\n");
    return ERR_INIT;
  }
  contNet->stations = NULL;
  contNet->subnets = NULL;
  contNet->channels = NULL;
  contNet->nSta = 0;
  contNet->nSub = 0;
  contNet->nSlots = 0;
  contNet->nChan = 0;
  contNet->PreEventTime = 0;
  contNet->TrigDur = 60;
  contNet->subnetContrib = 30;
  contNet->DefaultStationDur = 120;
  contNet->MaxDur = 500;
  contNet->duration = 0;
  contNet->onTime = 0;
  contNet->latency = 5;
  contNet->numSubAll = 0;
  contNet->listSubnets = 0;
  contNet->eventID = 1;
  contNet->terminate = 0;
  contNet->numSub = 0;
  contNet->compAsWild = 0;
  
  /*	Initialize members of the CONTPARAM structure			*/
  contNet->contParam.debug = 0;
  contNet->contParam.ring[0] = '\0';
  contNet->contParam.staFile[0] = '\0';
  contNet->contParam.heartbeatInt = 15;
  contNet->contParam.ringKey = 0;
  strcpy(contNet->contParam.OriginName,"CONT");

  /* Set default length for the trigger message buffer			*/
  contNet->trigMsgBufLen = MAX_BYTES_PER_EQ;
  
  return ( retVal );
}
