
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: matchscn.c 1459 2004-05-11 18:14:18Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/11 18:14:18  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*****************************************************************************
 * matchSCNL() - Returns the index of the SCNL matching sta.cha.net.loc      *
 *               found in the SCNLlist array.                                *
 *               Otherwise, returns -1 if no match is found                  *
 *                                  -2 if an error occured                   *
 *****************************************************************************/
#include <string.h>
#include <earthworm.h>
#include "decimate.h"

int matchSCNL( TracePacket* pPkt, unsigned char msgtype, WORLD* pDcm )
{
  int   i;
  
/* Look for match in TYPE_TRACEBUF2 packet 
 *****************************************/
  if( msgtype == pDcm->dcmEWH.typeTrace2 ) 
  {
     if( (pPkt->trh2.sta  == NULL) || 
         (pPkt->trh2.chan == NULL) || 
         (pPkt->trh2.net  == NULL) || 
         (pPkt->trh2.loc  == NULL)    )
     {
       logit ("et",  "decimate: invalid (NULL) parameters to matchSCNL\n");
       return( -2 );
     }
  
     for(i = 0; i < pDcm->nSta; i++ )
     {
     /* try to match explicitly */
       if ((strcmp(pPkt->trh2.sta,  pDcm->stations[i].inSta)  == 0) &&
           (strcmp(pPkt->trh2.chan, pDcm->stations[i].inChan) == 0) &&
           (strcmp(pPkt->trh2.net,  pDcm->stations[i].inNet)  == 0) &&
           (strcmp(pPkt->trh2.loc,  pDcm->stations[i].inLoc)  == 0)    )
         return( i );
     }
     return( -1 );  /* no match in SCNL for TYPE_TRACEBUF2 */
  }

/* Look for match in TYPE_TRACEBUF packet
 ****************************************/
  else if( msgtype == pDcm->dcmEWH.typeTrace )
  {
     if( (pPkt->trh.sta  == NULL) || 
         (pPkt->trh.chan == NULL) || 
         (pPkt->trh.net  == NULL)    )
     {
       logit ("et",  "decimate: invalid (NULL) parameters to matchSCNL\n");
       return( -2 );
     }
  
     for(i = 0; i < pDcm->nSta; i++ )
     {
     /* try to match explicitly */
       if ((strcmp(pPkt->trh.sta,   pDcm->stations[i].inSta)  == 0) &&
           (strcmp(pPkt->trh.chan,  pDcm->stations[i].inChan) == 0) &&
           (strcmp(pPkt->trh.net,   pDcm->stations[i].inNet)  == 0) &&
           (strcmp(LOC_NULL_STRING, pDcm->stations[i].inLoc)  == 0)    )
         return( i );
     }
     return( -1 );  /* no match in SCN for TYPE_TRACEBUF */
  }

/* Unknown Message Type
 **********************/
  return( -2 );
}

