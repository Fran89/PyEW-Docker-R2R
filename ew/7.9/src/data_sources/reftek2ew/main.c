/* @(#)main.c	1.2 07/23/98 */
/*======================================================================
 *
 * Import data from a single Refraction Technology RTP server and load
 * them into Earthworm shared memory.  If a RAW_RING is specified, then
 * all packets which are received from the server are copied unaltered
 * as TYPE_RTP messages.  If a WAVE_RING is specified, then data (DT)
 * packets are reformatted into TYPE_TRACEBUF messages copied there.
 * Either or both rings may be defined.
 *
 * This program was written for Refraction Technology Inc. by
 * -------------------------------------------------------------------
 * Engineering Services & Software                 email: dec@essw.com
 * 3950 Sorrento Valley Blvd., Suite F             tel: (619) 587-2765
 * San Diego, CA 92121                             fax: (619) 587-0444
 *====================================================================*/
#include "import_rtp.h"
CHAR debugbuf[1024];

#define MY_MOD_ID IMPORT_RTP_MAIN

RTP *Rtp = (RTP *) NULL; /* global connection handle */

/* moved par from main() to global scope.  extern'd in import_rtp.h */
struct param par;

int main(int argc, char **argv)
{
UINT8 pkt[RTP_MAXMSGLEN];
INT32  nbytes;

    printf("%s version %s\n", argv[0], VERSION_ID);
    init((INT32) argc, (CHAR **) argv, &par);


/* Open connection to server */

    logit("t", "initiating connection to %s:%hu\n",
        par.host, par.port
    );

    Rtp = rtp_open(par.host, par.port, &par.attr, par.retry);

    if (Rtp == (RTP *) NULL) {
        logit("t", "FATAL ERROR: rtp_open: %s\n", strerror(errno));
        terminate(MY_MOD_ID + 1);
    }

    logit("t", "connected to %s:%hu\n", Rtp->peer, Rtp->port);

    while (1) {

    /* Get a packet */

        if (!rtp_daspkt(Rtp, pkt, &nbytes)) {

        /* If the read failed, retry if not too serious */

            if (rtp_errno(Rtp) > par.retry) {
                logit("t", "FATAL ERROR: rtp_daspkt: error %d\n",
                    rtp_errno(Rtp)
                );
                terminate(MY_MOD_ID + 2);
            } else {
                logit("t", "attempting to reconnect\n");
                notify_statmgr(SERVER_NOT_RESPONDING);
                rtp_close(Rtp);
                sleep(30);
                Rtp = rtp_open(par.host, par.port, &par.attr, par.retry);
                if (Rtp == (RTP *) NULL) {
                    logit("t", "FATAL ERROR: rtp_open: %s\n",
                        strerror(errno)
                    );
                    terminate(MY_MOD_ID + 3);
                }
                logit("t", "reconnected to %s:%hu\n", Rtp->peer, Rtp->port);
                notify_statmgr(SERVER_OK);
            }

    /* Process the packet */

        } else if (nbytes > 0 && rtp_want(Rtp, pkt)) {
            note_daspkt(); /* keep track of when we got last daspkt */
            if (par.RawRing.defined) send_rtp(&par.RawRing.shm, pkt);
            if (par.WavRing.defined) send_wav(&par.WavRing.shm, pkt);

        }
    }
    return (0);
}
