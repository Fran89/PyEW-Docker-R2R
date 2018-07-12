/* @(#)watch.c	1.2 07/01/98 */
/*======================================================================
 *
 * Quick and dirty program to monitor the WAVE_RING and RAW_RING for
 * packets from import_rtp, as an aid in testing the same.
 *
 *====================================================================*/
#include "import_rtp.h"

typedef struct ringinfo {
    MSG_LOGO logo;
    SHM_INFO shm;
    CHAR    *msg;
    INT32     maxlen;
} RINGSTUFF;

/* create a tracepacket header summary string */

CHAR *trace_str(TracePacket *trace, CHAR *buf)
{
INT32 *data;
TRACE2_HEADER *trh2;
CHAR tb[UTIL_MAXTIMESTRLEN];

    data = (INT32 *) (trace->msg + sizeof(TRACE2_HEADER));
    trh2  = &(trace->trh2);
    buf[0] = 0;

    sprintf(buf+strlen(buf), "%4s:%s:%s:%s ",
        trh2->sta, trh2->chan, trh2->net, trh2->loc
    );

    sprintf(buf+strlen(buf), "%d ",    trh2->pinno);
    sprintf(buf+strlen(buf), "%s ",    util_dttostr(trh2->starttime, 0, tb));
    sprintf(buf+strlen(buf), "%s ",    util_dttostr(trh2->endtime, 0, tb));
    sprintf(buf+strlen(buf), "%7.2f ", trh2->samprate);
    sprintf(buf+strlen(buf), "%s ",    trh2->datatype);
    sprintf(buf+strlen(buf), "%d ",    trh2->nsamp);
    sprintf(buf+strlen(buf), "%7ld ",  (long)data[0]);
    sprintf(buf+strlen(buf), "%7ld ",  (long)data[trh2->nsamp-1]);

    return buf;
}

/* Connect to a ring */

VOID attach(RINGSTUFF *ring, CHAR *name, CHAR *type, CHAR *msg, INT32 maxlen)
{
INT32 key;
static CHAR *mod = "MOD_REFTEK2EW";

    if ((key = GetKey(name)) < 0) {
        fprintf(stderr, "can't get %s key!\n", name);
        exit(1);
    }

    ring->logo.instid = 0;

    if (GetType(type, &ring->logo.type) != 0) {
        printf("FATAL ERROR: invalid message type <%s>\n", type);
        exit(1);
    }

    if (GetModId(mod, &ring->logo.mod) != 0) {
        printf("FATAL ERROR: invalid module ID <%s>\n", mod);
        exit(1);
    }

    ring->msg = msg;
    ring->maxlen = maxlen;

    tport_attach(&ring->shm, key);
    printf("Attached to %s memory region %ld\n", name, key);
}

void main(int argc, char **argv)
{
int status;
long len;
RINGSTUFF raw, wave;
union {
    CHAR raw[RTP_DASPAKLEN];
    TracePacket trace;
} buf;
CHAR strbuf[1024];

/* Initialize connection to raw ring */

    attach(
        &raw,
        "RAW_RING",
        "TYPE_REFTEK",
        buf.raw,
        RTP_DASPAKLEN
    );

/* Initialize connection to wave ring */

    attach(
        &wave,
        "WAVE_RING",
        "TYPE_TRACEBUF2",
        buf.trace.msg,
        MAX_TRACEBUF_SIZ
    );

/* Poll the rings for messages, and print summaries */

    while (1) {

    /* raw ring */

        if (tport_getflag(&raw.shm) == TERMINATE) {
            printf("termination flag set (raw)... exiting\n");
            exit(0);
        }

        status = tport_getmsg(
            &raw.shm, &raw.logo, (short) 1, &raw.logo, &len,
            raw.msg, raw.maxlen
        );

        if (status == GET_OK) {
            printf("%s\n", reftek_str((unsigned char *)raw.msg, strbuf));
        } else if (status != GET_NONE) {
            printf("tport_getmsg (raw) returns status %d\n", status);
        }

    /* wave ring */

        if (tport_getflag(&wave.shm) == TERMINATE) {
            printf("termination flag set (wave)... exiting\n");
            exit(0);
        }

        status = tport_getmsg(
            &wave.shm, &wave.logo, (short) 1, &wave.logo, &len,
            wave.msg, wave.maxlen
        );

        if (status == GET_OK) {
            printf("%s\n", trace_str((TracePacket *) wave.msg, strbuf));
        } else if (status == GET_NONE) {
            sleep_ew(250);
        } else {
            printf("tport_getmsg (wave) returns status %d\n", status);
        }
    }
}
