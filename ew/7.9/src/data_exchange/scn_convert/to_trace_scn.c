/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_trace_scn.c 6081 2014-05-12 21:10:32Z paulf $
 *
 */

#include <string.h>

#include "trace_buf.h"
#include "scnl_convert.h"

      /*******************************************************
       *                   to_trace_scn()                    *
       *  Convert a pick of TYPE_TRACEBUF2 to a pick of      *
       *  TYPE_TRACEBUF.                                     *
       *  Returns 0, if no error.                            *
       *          1, if not selected for conversion          *
       *         -1, if an error or buffer overflow occured  *
       *******************************************************/

int to_trace_scn( char *msg )
{
    TRACE_HEADER  *TraceHeadP;     /* The tracebuf header  */
    TRACE2_HEADER Trace2Head;      /* The tracebuf2 header */
    S2S s = { 0 };

    Trace2Head = *((TRACE2_HEADER *) msg);

    s.scnl.s = Trace2Head.sta;
    s.scnl.c = Trace2Head.chan;
    s.scnl.n = Trace2Head.net;
    s.scnl.l = Trace2Head.loc;

    if (scnl2scn(&s) == 0) return 1; /* no match; don't send */

    TraceHeadP =   (TRACE_HEADER *)  msg;

    strncpy(TraceHeadP->sta,  s.scn.s, TRACE_STA_LEN);
    strncpy(TraceHeadP->chan, s.scn.c, TRACE_CHAN_LEN);
    strncpy(TraceHeadP->net,  s.scn.n, TRACE_NET_LEN);

    /* Make sure the last character is null; strncpy doesn't automatically append null if
     * count <= sizeof(strSource), and the TRACE_*_LEN defines include room for a null.
     */
    TraceHeadP->sta[TRACE_STA_LEN - 1]   = '\0';
    TraceHeadP->chan[TRACE_CHAN_LEN - 1] = '\0';
    TraceHeadP->net[TRACE_NET_LEN - 1]   = '\0';

    return 0;
}

	
    
