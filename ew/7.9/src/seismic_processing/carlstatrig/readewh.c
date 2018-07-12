
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readewh.c 1788 2005-04-12 22:43:08Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2005/04/12 22:43:08  dietz
 *     Added optional command "GetWavesFrom <instid> <module_id>"
 *
 *     Revision 1.2  2004/05/05 23:54:04  lombard
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
 * readcnfg.c: Read the Earthworm parameters.
 *              1) Set members of the CSTEWH structure.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadEWH                                               */
/*                                                                      */
/*      Inputs:         Pointer to a CarlStaTrig Earthworm parameter    */
/*                        structure                                     */
/*                                                                      */
/*      Outputs:        Updated input structure(above)                  */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* GetInst, GetLocalInst, GetModId, GetType     */

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ReadEWH                                               */
int ReadEWH( CSTPARAM* cstParam, CSTEWH* cstEwh )
{
  int   retVal = 0;     /* Return value for this function               */

  if ( CT_FAILED( GetLocalInst( &(cstEwh->myInstId) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting myInstId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetModId( cstParam->myModName, &(cstEwh->myModId) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting myModId.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetInst( "INST_WILDCARD", &(cstEwh->instWild) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting INST_WILDCARD.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetModId( "MOD_WILDCARD", &(cstEwh->modWild) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting MOD_WILDCARD.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_CARLSTATRIG_SCNL", 
                                &(cstEwh->typeCarlStaTrig) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting TYPE_CARLSTATRIG_SCNL.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_ERROR", &(cstEwh->typeError) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting TYPE_ERROR.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_HEARTBEAT", &(cstEwh->typeHeartbeat) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting TYPE_HEARTBEAT.\n" );
    retVal = ERR_EWH_READ;
  }
  else if ( CT_FAILED( GetType( "TYPE_TRACEBUF2", &(cstEwh->typeWaveform) ) ) )
  {
    logit( "e", "CarlStaTrig: Error getting TYPE_TRACEBUF2.\n" );
    retVal = ERR_EWH_READ;
  }

  return ( retVal );
}
