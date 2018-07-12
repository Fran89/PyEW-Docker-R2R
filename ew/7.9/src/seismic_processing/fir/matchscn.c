
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: matchscn.c 1652 2004-07-28 22:43:05Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/07/28 22:43:04  lombard
 *     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*****************************************************************************
 * matchSCNL () - Returns the index of the SCNL matching sta.cha.net.loc     *
 *               found in the SCNLlist array.                                *
 *               Otherwise, returns -1 (or -2 if an error occured            *
 *                                                                           *
 *****************************************************************************/
#include <string.h>
#include <earthworm.h>
#include "fir.h"

int matchSCNL (TRACE2_HEADER* WaveHead, WORLD* pDcm )
{
  int i;
  
  if ((WaveHead->sta == NULL) || (WaveHead->chan == NULL) || 
      (WaveHead->net == NULL) || (WaveHead->loc == NULL))
  {
    logit ("et",  "fir: invalid parameters to matchSCNL\n");
    return (-2);
  }
  
  for (i = 0; i < pDcm->nSta; i++ )
  {
    /* try to match explicitly */
    if ((strcmp (WaveHead->sta, pDcm->stations[i].inSta) == 0) &&
        (strcmp (WaveHead->chan, pDcm->stations[i].inChan) == 0) &&
        (strcmp (WaveHead->net, pDcm->stations[i].inNet) == 0) &&
	(strcmp (WaveHead->loc, pDcm->stations[i].inLoc) == 0))
      return (i);
  }
  /* No match */
  return -1;
}

