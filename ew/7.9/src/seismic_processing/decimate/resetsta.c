
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: resetsta.c 6 2000-02-14 17:02:31Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * resetsta.c: Reset station buffers for one STATION structure
 *              1) Reset station parameters and buffer pointers
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ResetStation                                          */
/*                                                                      */
/*      Inputs:         Pointer to Station Structure                    */
/*                                                                      */
/*      Outputs:        Updated Station structures                      */
/*                                                                      */
/*      Returns:        nothing                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/

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
void ResetStation( STATION *sta )
{
  PSTAGE this;              /* pointer to current stage                 */
  
  this = sta->pStage;
  sta->inEndtime = 0.0;
    
  /* Reset the stage buffers */
  while (this->next != (PSTAGE) NULL)
  {
    this->inBuff.starttime = 0.0;
    this->inBuff.samplerate = 1.0;
    this->inBuff.read = 0;
    this->inBuff.write = 0;
    this = this->next;
  }
    
  return;
}
