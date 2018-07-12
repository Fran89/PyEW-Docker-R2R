/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_pick2k.c 5745 2013-08-07 16:27:24Z paulf $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chron3.h>
#include <math.h>
#include <rdpickcoda.h>
#include "scnl_convert.h"


      /*******************************************************
       *                    to_pick_2k()                     *
       *  Convert a pick of TYPE_PICK_SCNL to a pick of      *
       *  TYPE_PICK2K.                                       *
       *******************************************************/


int to_pick2k( char *pick_scnl, char *pick2k, unsigned char newMsgType )
{
   EWPICK      pk;
   S2S         s;
   struct Greg g;
   int         tsec, thun;
   char        fm;

   if (rd_pick_scnl(pick_scnl, strlen(pick_scnl), &pk) != EW_SUCCESS) {
       logit("et", "to_pick2k: error reading pick_scnl msg\n");
       return -2;
   }
   if( pk.fm == ' ' ) fm = '?';
   else               fm = pk.fm;
   
   memset(&s, 0, sizeof(S2S));
   s.scnl.s = pk.site;
   s.scnl.c = pk.comp;
   s.scnl.n = pk.net;
   s.scnl.l = pk.loc;
   if (scnl2scn(&s) == 0) return 1;  /* not selected */

/* Convert nominal epoch seconds to date and time.
   Round pick time to nearest hundred'th
   of a second.
   ***************************************/
   datime( pk.tpick + GSEC1970, &g );
   tsec = (int)floor( (double) g.second );
   thun = (int)((100.*(g.second - tsec)) + 0.5);
   if ( thun == 100 )
      tsec++, thun = 0;

/* Convert pick to ASCII (based on v6.1 pick_ew/report.c)
 ********************************************************/
   sprintf( pick2k,    "%3d",   (int) newMsgType );
   sprintf( pick2k+3,  "%3d",   (int) pk.modid );
   sprintf( pick2k+6,  "%3d ",  (int) pk.instid );
   sprintf( pick2k+10, "%4d ",  pk.seq );
   sprintf( pick2k+15, "%-5s",  s.scn.s );
   sprintf( pick2k+20, "%-2s",  s.scn.n );
   sprintf( pick2k+22, "%-3s",  s.scn.c );
   sprintf( pick2k+25, " %c ",  fm);
   sprintf( pick2k+27, "%1d  ", pk.wt );

   sprintf( pick2k+30, "%4d%02d%02d%02d%02d%02d.%02d", g.year,
            g.month, g.day, g.hour, g.minute, tsec, thun );

   sprintf( pick2k+47, "%ld %ld %ld\n", pk.pamp[0], pk.pamp[1], pk.pamp[2] );

   return 0;
}
