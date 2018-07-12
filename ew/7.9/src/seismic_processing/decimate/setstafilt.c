
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: setstafilt.c 1093 2002-10-25 17:59:44Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2002/10/25 17:59:44  dietz
 *     fixed spelling mistakes
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * setstafilt.c: Set up filter buffers for all stations
 *              1) Allocates linked list of buffer structures
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: SetStaFilters                                         */
/*                                                                      */
/*      Inputs:         Pointer to World Structure                      */
/*                                                                      */
/*      Outputs:        Updated Station structures                      */
/*                                                                      */
/*      Returns:        UW_SUCCESS on success, else UW_FAILURE          */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdlib.h>  /* For malloc                                      */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: SetStaFilters                                         */
int SetStaFilters( WORLD *pDcm )
{
  int iSta, jStage;
  STATION *sta;             /* pointer to array of stations             */
  PSTAGE this;              /* pointer to current stage                 */
  
  
  /* Set up filter stage structures for all stations we knw about       */
  for (iSta = 0; iSta < pDcm->nSta; iSta++)
  {
    sta = &(pDcm->stations[iSta]);
    
  /* Allocate the first stage */
    if ( (this = (PSTAGE) malloc( sizeof(STAGE))) == NULL )
      goto mem_abort;
    
    sta->pStage = this;
    sta->inEndtime = 0.0;
    
    /* Initialize current stage and allocate the next one */
    for (jStage = 0; jStage < pDcm->nStage; jStage++)
    {
      this->pFilt = &(pDcm->filter[jStage]);
      this->inBuff.starttime = 0.0;
      this->inBuff.samplerate = 1.0;
      this->inBuff.read = 0;
      this->inBuff.write = 0;
      if ( (this->next = (PSTAGE) malloc( sizeof(STAGE))) == NULL )
        goto mem_abort;
      this = this->next;
    }
    
    /* Initialize the final stage, only used for output of previous stage */
    this->pFilt = (FILTER*) NULL;
    this->inBuff.starttime = 0.0;
    this->inBuff.samplerate = 1.0;
    this->inBuff.read = 0;
    this->inBuff.write = 0;
    this->next = (PSTAGE) NULL;
    
  }
  
  return EW_SUCCESS;
  
 mem_abort:
  logit("e", "decimate: error allocating station stage; exiting\n");
  return EW_FAILURE;
}
