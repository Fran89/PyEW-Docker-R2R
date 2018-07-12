#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trace_buf.h"
#include "scn_map.h"
#include "gcf.h"
#include "externs.h"


/*
	Returns:
		UPON SUCCESS (trace data converted):
			-the address of the static memory buffer containing the TRACE MESSAGE
		UPON FAILURE:
			-NULL pointer;
			-sets the value pointed to by the long ptr to ZERO.
*/

static TracePacket trace_buffer;

char *convert_gcf_to_ewtrace(GCFhdr *hdr, SCN *scn, long *out_message_size) {

TRACE_HEADER	*trace_hdr;		/* ew trace header */
TRACE2_HEADER	*trace2_hdr;		/* ew trace header */



    if (UseTraceBuf2 == 0) {
    	*out_message_size = sizeof(TRACE_HEADER)+sizeof(int)*hdr->num_samps;
	trace_hdr = (TRACE_HEADER *) &trace_buffer.trh;
	memset((void*) trace_hdr, 0, sizeof(TRACE_HEADER));
	memcpy(&trace_buffer.msg[sizeof(TRACE_HEADER)], hdr->data, hdr->num_samps*sizeof(int32_t));
	trace_hdr->pinno = 0;		/* Unknown item */
	trace_hdr->nsamp = hdr->num_samps;
	trace_hdr->starttime = gepoch2uepoch(&(hdr->epoch));
	trace_hdr->endtime = trace_hdr->starttime+ (double)(hdr->num_samps-1) / hdr->sample_rate;
	trace_hdr->samprate = hdr->sample_rate;
	strcpy(trace_hdr->sta,scn->site);
	strcpy(trace_hdr->net,scn->net);
	strcpy(trace_hdr->chan,scn->chan);
#ifdef _SPARC
	strcpy(trace_hdr->datatype, "s4");
#endif
#ifdef _INTEL
	strcpy(trace2_hdr->datatype, "i4");
#endif
	trace_hdr->quality[1] = 0;
	trace_hdr->pinno = 0;
	return &(trace_buffer.msg[0]);
    } else {
    	*out_message_size = sizeof(TRACE2_HEADER)+sizeof(int)*hdr->num_samps;
	trace2_hdr = (TRACE2_HEADER *) &trace_buffer.trh2;
	memset((void*) trace2_hdr, 0, sizeof(TRACE2_HEADER));
	memcpy(&trace_buffer.msg[sizeof(TRACE2_HEADER)], hdr->data, hdr->num_samps*sizeof(int32_t));
	trace2_hdr->pinno = 0;		/* Unknown item */
	trace2_hdr->nsamp = hdr->num_samps;
	trace2_hdr->starttime = gepoch2uepoch(&(hdr->epoch));
	trace2_hdr->endtime = trace2_hdr->starttime+ (double)(hdr->num_samps-1) / hdr->sample_rate;
	trace2_hdr->samprate = hdr->sample_rate;
	strcpy(trace2_hdr->sta,scn->site);
	strcpy(trace2_hdr->net,scn->net);
	strcpy(trace2_hdr->chan,scn->chan);
	strcpy(trace2_hdr->loc,scn->loc);
#ifdef _SPARC
	strcpy(trace2_hdr->datatype, "s4");
#endif
#ifdef _INTEL
	strcpy(trace2_hdr->datatype, "i4");
#endif
	trace2_hdr->quality[1] = 0;
	trace2_hdr->pinno = 0;
	trace2_hdr->version[0]=TRACE2_VERSION0;
	trace2_hdr->version[1]=TRACE2_VERSION1;
	return &(trace_buffer.msg[0]);
    }
}
