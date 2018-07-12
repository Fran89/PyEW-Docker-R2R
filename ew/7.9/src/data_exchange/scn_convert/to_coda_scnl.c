/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_coda_scnl.c 5921 2013-09-11 16:09:57Z paulf $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chron3.h>
#include <rdpickcoda.h>
#include "scn_convert.h"


      /*******************************************************
       *                   to_coda_scnl()                    *
       *  Convert a coda of TYPE_CODA2K to a coda of         *
       *  TYPE_CODA_SCNL.                                    *
       *  Returns 0, if no error.                            *
       *          1, if not selected for conversion          *
       *         -1, if an error or buffer overflow occured  *
       *         -2, if error in reading input message       *
       *******************************************************/


int to_coda_scnl( char *coda2k, char *coda_scnl, int outLen, unsigned char newMsgType )
{
    EWCODA cd;
    S2S s;
    int  rc;

   if (rd_coda2k(coda2k, strlen(coda2k), &cd) != EW_SUCCESS) {
       logit("et", "to_coda_scnl: error reading coda2k msg\n");
       return -2;
   }
   
   memset(&s, 0, sizeof(S2S));
   s.scn.s = cd.site;
   s.scn.c = cd.comp;
   s.scn.n = cd.net;
   if (scn2scnl(&s) == 0) return 1;  /* not selected */

   rc = snprintf( coda_scnl, outLen, "%d %d %d %d %s.%s.%s.%s %ld %ld %ld %ld %ld %ld %d\n",
                  (int)newMsgType, (int)cd.modid, (int)cd.instid, cd.seq,
                  s.scnl.s, s.scnl.c, s.scnl.n, s.scnl.l, 
		  cd.caav[0], cd.caav[1], cd.caav[2], cd.caav[3], cd.caav[4],
		  cd.caav[5], cd.dur );
   coda_scnl[outLen-1] = '\0';

   if ( rc < 0 )       return -1;   /* Error in Solaris and Windows */
   if ( rc >= outLen ) return -1;   /* Solaris only. Can't happen in Windows */

   return 0;
}
