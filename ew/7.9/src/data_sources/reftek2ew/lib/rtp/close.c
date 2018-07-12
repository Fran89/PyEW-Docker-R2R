#pragma ident "$Id: close.c 1331 2004-03-16 23:32:39Z kohler $"
/*======================================================================
 * 
 * Close a connection, and free all resources.
 *
 *====================================================================*/
#include "rtp.h"

VOID rtp_close(RTP *rtp)
{
    if (rtp == (RTP *) NULL) return;

/* Shutdown the connection */

    if (rtp->sd >= 0) {
        shutdown(rtp->sd, 2);
#ifndef WINNT
        close(rtp->sd);
#else
        closesocket(rtp->sd);
#endif
    }

/* Free resources */

    if (rtp->soh.sh_nslot > 0) free(rtp->soh.sh_stat);
    free(rtp->peer);
    free(rtp);
}

/* Revision History
 *
 * $Log$
 * Revision 1.1  2004/03/16 23:19:55  kohler
 * Initial revision
 *
 * Revision 1.1.1.1  2000/06/22 19:13:09  nobody
 * Import existing sources into CVS
 *
 */
