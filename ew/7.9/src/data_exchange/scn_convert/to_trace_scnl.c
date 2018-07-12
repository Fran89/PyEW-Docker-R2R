/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_trace_scnl.c 6081 2014-05-12 21:10:32Z paulf $
 *
 */

#include <string.h>

#include "trace_buf.h"
#include "scn_convert.h"

      /*******************************************************
       *                   to_trace_scnl()                   *
       *  Convert a pick of TYPE_TRACEBUF to a pick of       *
       *  TYPE_TRACEBUF2.                                    *
       *  Returns 0, if no error.                            *
       *          1, if not selected for conversion          *
       *         -1, if an error or buffer overflow occured  *
       *******************************************************/

int to_trace_scnl( char *msg )
{
    TRACE_HEADER  TraceHead;       /* The tracebuf header  */
    TRACE2_HEADER *Trace2HeadP;    /* The tracebuf2 header */
    S2S s = { 0 };

    TraceHead   = *((TRACE_HEADER *)  msg);

    s.scn.s = TraceHead.sta;
    s.scn.c = TraceHead.chan;
    s.scn.n = TraceHead.net;

    if (scn2scnl(&s) == 0) return 1; /* no match; don't send */

    Trace2HeadP =   (TRACE2_HEADER *) msg;

    strncpy(Trace2HeadP->sta,  s.scnl.s, TRACE2_STA_LEN);
    strncpy(Trace2HeadP->chan, s.scnl.c, TRACE2_CHAN_LEN);
    strncpy(Trace2HeadP->net,  s.scnl.n, TRACE2_NET_LEN);
    strncpy(Trace2HeadP->loc,  s.scnl.l, TRACE2_LOC_LEN);

    /* Make sure the last character is null; strncpy doesn't automatically append null if
     * count <= sizeof(strSource), and the TRACE2_*_LEN defines include room for a null.
     */
    Trace2HeadP->sta[TRACE2_STA_LEN - 1]   = '\0';
    Trace2HeadP->chan[TRACE2_CHAN_LEN - 1] = '\0';
    Trace2HeadP->net[TRACE2_NET_LEN - 1]   = '\0';
    Trace2HeadP->loc[TRACE2_LOC_LEN - 1]   = '\0';

    Trace2HeadP->version[0] = TRACE2_VERSION0;
    Trace2HeadP->version[1] = TRACE2_VERSION1;

    return 0;
}

	
