#include <stdio.h>
#include <string.h>
#include "qlib2.h"
#include "trace_buf.h"			/* earthworm TRACE message definition */
#include "scn_map.h"			/* earthworm TRACE message definition */
#include "externs.h"

/* this utility uses the Berkeley QLIB2 functions to convert mseed data
	into an Earthworm Trace data Message 

	Input:  1. byte array of mseed packet
		2. size of mseed smallest packet input
	 	3. pointer to a long which will contain the size of the trace buffer

	Returns:
		UPON SUCCESS (trace data converted):
			-the address of the static memory buffer containing the TRACE MESSAGE
			-updates the long ptr with the trace_buffer length
		UPON SUCCESS (log channel detected):
			-NULL pointer;
			-sets the value pointed to by the long ptr to the number of bytes of the LOG msg.
		UPON FAILURE:
			-NULL pointer;
			-sets the value pointed to by the long ptr to ZERO.

	CAVEATS: 
		Since this module was developed for ATWC and will be used
		in a mission critical application, one of the design requirements
		was to have as much memory pre-allocated as possible. Thus, the
		TracePacket union is pre-defined. From the trace_buf.h include
		file, this was 4096 bytes as of this writing of the code. Be fore
		warned for larger mseed implementations

*/

static TracePacket trace_buffer;

char *convert_mseed_to_ewtrace(void *bytes, int mseed_min_blk_size, long *out_message_size) {

DATA_HDR	*mseed_hdr;		/* qlib2 mseed_header */
TRACE_HEADER	*trace_hdr;		/* ew trace header */
TRACE2_HEADER	*trace2_hdr;		/* ew trace header */
int		num_samples;		/* number of samples returned from uncompression routine */
int		pin_from_scn;		/* return value to use from PINmap */

	/* note that the mseed_hdr is a dynamically allocated struct of approx. 100 bytes */
	if( (mseed_hdr=decode_hdr_sdr ((SDR_HDR *)bytes, mseed_min_blk_size)) == NULL) {
		*out_message_size=0;
		return NULL;
	}
	if (1 == UseTraceBuf2) 
		*out_message_size = sizeof(TRACE2_HEADER)+sizeof(int)*mseed_hdr->num_samples;
	else
		*out_message_size = sizeof(TRACE_HEADER)+sizeof(int)*mseed_hdr->num_samples;

	/* check to see if we have a log channel! */
	if (mseed_hdr->sample_rate == 0 || 
	    mseed_hdr->sample_rate_mult == 0 ||
	    mseed_hdr->num_samples == 0)  {
		free_data_hdr(mseed_hdr);
		/* not true message size! */
		*out_message_size = 80;
		return NULL;
	}


	/* uncompress the packet */
	if (1 == UseTraceBuf2) 
		num_samples = ms_unpack(mseed_hdr, mseed_hdr->num_samples, (char *)bytes, 
			&trace_buffer.msg[sizeof(TRACE2_HEADER)]);
	else
		num_samples = ms_unpack(mseed_hdr, mseed_hdr->num_samples, (char *)bytes, 
			&trace_buffer.msg[sizeof(TRACE_HEADER)]);

	if (num_samples != mseed_hdr->num_samples) {
		free_data_hdr(mseed_hdr);
		*out_message_size=0;
		return NULL;
	}
#ifdef DEBUG
	if (strcmp( trim(mseed_hdr->channel_id), "HHZ") == 0) {
		int *int_ptr;
		double f; 
		if (1 == UseTraceBuf2) 
			int_ptr = (int *) &(trace_buffer.msg[sizeof(TRACE2_HEADER)]);
		else
			int_ptr = (int *) &(trace_buffer.msg[sizeof(TRACE_HEADER)]);
		fprintf(stderr, "HHZ First sample of packet = %d \n", (int) *int_ptr);
		fprintf(stderr, "HHZ Second sample of packet = %d \n", (int) *(++int_ptr));
		fprintf(stderr, " X0= %d  XN = %d \n", mseed_hdr->x0, mseed_hdr->xn);
		f = num_samples/50.0;
		fprintf(stderr, " number of samples = %d, secs = %f\n", num_samples, f);
	}
#endif /* DEBUG */
	if (1 == UseTraceBuf2) {
		trace2_hdr = (TRACE2_HEADER *) &trace_buffer.trh;
		memset((void*) trace2_hdr, 0, sizeof(TRACE2_HEADER));
		trace2_hdr->pinno = 0;		/* Unknown item */
		trace2_hdr->nsamp = mseed_hdr->num_samples;
		/* note that unix_time_from_int_time() does not handle leap_seconds secs=60 should
			this miraculously occur on the start time of a data packet... 
		*/
		trace2_hdr->starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) +
		            ((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC);
		trace2_hdr->endtime = (double)unix_time_from_int_time(mseed_hdr->endtime) +
		            ((double)(mseed_hdr->endtime.usec)/USECS_PER_SEC);
		trace2_hdr->samprate = sps_rate(mseed_hdr->sample_rate,mseed_hdr->sample_rate_mult);
		strcpy(trace2_hdr->sta,trim(mseed_hdr->station_id));
		strcpy(trace2_hdr->net,trim(mseed_hdr->network_id));
		strcpy(trace2_hdr->chan,trim(mseed_hdr->channel_id));
		strcpy(trace2_hdr->loc,trim(mseed_hdr->location_id));
		if (0 == strncmp(trace2_hdr->loc, "  ", 2) || 0 == memcmp(trace2_hdr->loc, "\000\000", 2))
			strcpy(trace2_hdr->loc,"--");
		strcpy(trace2_hdr->datatype,(my_wordorder == SEED_BIG_ENDIAN) ? "s4" : "i4");
                trace2_hdr->version[0]=TRACE2_VERSION0;
                trace2_hdr->version[1]=TRACE2_VERSION1;
		trace2_hdr->quality[1] = (char)mseed_hdr->data_quality_flags;
	} else {
		trace_hdr = (TRACE_HEADER *) &trace_buffer.trh;
		memset((void*) trace_hdr, 0, sizeof(TRACE2_HEADER));
		trace_hdr->pinno = 0;		/* Unknown item */
		trace_hdr->nsamp = mseed_hdr->num_samples;
		/* note that unix_time_from_int_time() does not handle leap_seconds secs=60 should
			this miraculously occur on the start time of a data packet... 
		*/
		trace_hdr->starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) +
		            ((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC);
		trace_hdr->endtime = (double)unix_time_from_int_time(mseed_hdr->endtime) +
		            ((double)(mseed_hdr->endtime.usec)/USECS_PER_SEC);
		trace_hdr->samprate = sps_rate(mseed_hdr->sample_rate,mseed_hdr->sample_rate_mult);
		strcpy(trace_hdr->sta,trim(mseed_hdr->station_id));
		strcpy(trace_hdr->net,trim(mseed_hdr->network_id));
		strcpy(trace_hdr->chan,trim(mseed_hdr->channel_id));
		strcpy(trace_hdr->datatype,(my_wordorder == SEED_BIG_ENDIAN) ? "s4" : "i4");
		trace_hdr->quality[1] = (char)mseed_hdr->data_quality_flags;
	}

	/* finally, get the pinno from the linked list */
	if (1 == UseTraceBuf2) {
		if ( (pin_from_scn = getPinFromSCNL(trace2_hdr->sta,
			trace2_hdr->chan,trace2_hdr->net, trace2_hdr->loc)) != -1) {
			trace2_hdr->pinno = pin_from_scn;
		}	
	} else {
                if ( (pin_from_scn = getPinFromSCN(trace_hdr->sta,
                        trace_hdr->chan,trace_hdr->net)) != -1) {
                        trace_hdr->pinno = pin_from_scn;
                }
	}

	free_data_hdr(mseed_hdr);
	return &(trace_buffer.msg[0]);
} 
