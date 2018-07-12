/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_coda2k.c 5921 2013-09-11 16:09:57Z paulf $
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
       *                     to_coda2k()                     *
       *  Convert a coda of TYPE_CODA_SCNL to a coda of      *
       *  TYPE_CODA2K.                                       *
       *******************************************************/


int to_coda2k( char *coda_scnl, char *coda2k, unsigned char newMsgType )
{
    EWCODA cd;
    S2S s;

   if (rd_coda_scnl(coda_scnl, strlen(coda_scnl), &cd) != EW_SUCCESS) {
       logit("et", "to_coda2k: error reading coda_scnl msg\n");
       return -2;
   }
   
   memset(&s, 0, sizeof(S2S));
   s.scnl.s = cd.site;
   s.scnl.c = cd.comp;
   s.scnl.n = cd.net;
   s.scnl.l = cd.loc;
   if (scnl2scn(&s) == 0) return 1;  /* not selected */

/* Convert coda to ASCII (based on v6.1 pick_ew/report.c)
 ********************************************************/
   sprintf( coda2k,    "%3d",  (int) newMsgType );
   sprintf( coda2k+3,  "%3d",  (int) cd.modid );
   sprintf( coda2k+6,  "%3d ", (int) cd.instid );
   sprintf( coda2k+10, "%4d ", cd.seq);
   sprintf( coda2k+15, "%-5s", s.scn.s );
   sprintf( coda2k+20, "%-2s", s.scn.n );
   sprintf( coda2k+22, "%-3s", s.scn.c );
   sprintf( coda2k+25, "%8ld",  cd.caav[0] );
   sprintf( coda2k+33, "%8ld",  cd.caav[1] );
   sprintf( coda2k+41, "%8ld",  cd.caav[2] );
   sprintf( coda2k+49, "%8ld",  cd.caav[3] );
   sprintf( coda2k+57, "%8ld",  cd.caav[4] );
   sprintf( coda2k+65, "%8ld",  cd.caav[5] );
   sprintf( coda2k+73, "%4d \n", cd.dur );

   return 0;
}
