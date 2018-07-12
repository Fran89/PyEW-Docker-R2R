/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_pick_scnl.c 5921 2013-09-11 16:09:57Z paulf $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chron3.h>
#include <math.h>
#include <rdpickcoda.h>
#include "scn_convert.h"


      /*******************************************************
       *                   to_pick_scnl()                    *
       *  Convert a pick of TYPE_PICK2K to a pick of         *
       *  TYPE_PICK_SCNL.                                    *
       *  Returns 0, if no error.                            *
       *          1, if not selected for conversion          *
       *         -1, if an error or buffer overflow occured  *
       *         -2, if error reading input message          *
       *******************************************************/


int to_pick_scnl( char *pick2k, char *pick_scnl, int outLen, 
		  unsigned char newMsgType )
{
   EWPICK      pk;
   S2S         s;
   struct Greg g;
   int         tsec, thun, rc;
   char        fm;

   if (rd_pick2k(pick2k, strlen(pick2k), &pk) != EW_SUCCESS) {
       logit("et", "to_pick_scnl: error reading pick2k msg\n");
       return -2;
   }
   if( pk.fm == ' ' ) fm = '?';
   else               fm = pk.fm;
   
   memset(&s, 0, sizeof(S2S));
   s.scn.s = pk.site;
   s.scn.c = pk.comp;
   s.scn.n = pk.net;
   if (scn2scnl(&s) == 0) return 1;  /* not selected */

/* Convert nominal epoch seconds to date and time.
   Round pick time to nearest hundred'th
   of a second.
   ***************************************/
   datime( pk.tpick + GSEC1970, &g );
   tsec = (int)floor( (double) g.second );
   thun = (int)((100.*(g.second - tsec)) + 0.5);
   if ( thun == 100 )
      tsec++, thun = 0;

/* Warning: Return code is different for Solaris and Windows
   *********************************************************/
   rc = snprintf( pick_scnl, outLen, 
		  "%d %d %d %d %s.%s.%s.%s %c%c %4d%02d%02d%02d%02d%02d.%02d0 %ld %ld %ld\n",
                  (int)newMsgType, (int)pk.modid, (int)pk.instid, pk.seq,
                  s.scnl.s, s.scnl.c, s.scnl.n, s.scnl.l, fm, pk.wt, 
		  g.year, g.month, g.day, g.hour, g.minute, tsec, thun, 
		  pk.pamp[0], pk.pamp[1], pk.pamp[2] );
   pick_scnl[outLen-1] = '\0';

   if ( rc < 0 )       return -1;   /* Error in Solaris and Windows */
   if ( rc >= outLen ) return -1;   /* Solaris only. Can't happen in Windows */

   return 0;
}
