/* @(#)terminate.c	1.2 07/01/98 */
/*======================================================================
 *
 * Graceful exits.
 *
 *====================================================================*/
#include "import_rtp.h"

extern RTP *Rtp;

VOID terminate(INT32 status)
{

    RequestMutex(); /* So we only do this once */

    if (Rtp != (RTP *) NULL) {
        logit("t", "breaking connection with %s:%hu\n",
            Rtp->peer, Rtp->port
        );
        rtp_break(Rtp);
        rtp_close(Rtp);
    }

    logit("t", "exit %ld\n", status);
    exit((int) status);

}
