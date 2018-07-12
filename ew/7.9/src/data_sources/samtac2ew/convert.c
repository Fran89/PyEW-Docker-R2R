/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *   
 *    $Id: convert.c 3561 2009-02-13 00:11:48Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2009/02/13 00:11:48  tim
 *     Fix issue with 2s complement negative decision
 *
 *     Revision 1.5  2009/01/21 16:17:28  tim
 *     cleaned up and adjusted data collection for minimal latency
 *
 *     Revision 1.4  2009/01/15 18:15:31  tim
 *     Clean up externs.h, k2comif.h and k2ewerrs.h
 *
 *     Revision 1.3  2008/11/10 16:30:07  tim
 *     Cleaned up logging verbosity
 *
 *     Revision 1.2  2008/10/29 16:00:50  tim
 *     Added timestamp support and corrected a problem with separating channels
 *
 *     Revision 1.1  2008/10/23 21:00:03  tim
 *     Updating to use SCNL, and ewtrace
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace_buf.h>
#include "scnl_map.h"
#include "convert.h"
#include "glbvars.h"
#include <time.h>
#include <time_ew.h>

/*
	Returns:
		UPON SUCCESS (trace data converted):
			-the address of the static memory buffer containing the TRACE MESSAGE
		UPON FAILURE:
			-NULL pointer;
			-sets the value pointed to by the long ptr to ZERO.
*/

static TracePacket trace_buffer;

char *convert_samtac_to_ewtrace(char *samtac_packet, SCN *scn, long *out_message_size, int channel) {

	TRACE2_HEADER	*trace2_hdr;		/* ew trace header */
	int sample_rate = ((0xff & samtac_packet[9]) << 8) + (0xff & samtac_packet[10]);
	int i;
	char demux_channel_data[400*4];  //Max sample rate is 400hz, 4 bytes per sample
	struct tm dt = {0,};
	time_t epoch;
	int channels = samtac_packet[8];
	
	*out_message_size = sizeof(TRACE2_HEADER)+sizeof(int)*sample_rate;
	trace2_hdr = (TRACE2_HEADER *) &trace_buffer.trh2;
	memset((void*) trace2_hdr, 0, sizeof(TRACE2_HEADER));
	
	//Copy the channel's data to the trace_buffer.msg[sizeof(TRACE2_HEADER)]
	//Also make it little endian
	//little endian int = 1 == [0x01, 0x00, 0x00, 0x00]
	//big endian array = [0x00, 0x00, 0x01] == little endian array = [0x01, 0x00, 0x00, 0x00]
	
	if (gcfg_debug > 3)
	{
		logit("et", "convert.c: channel: %d iterator: %d sample hex: %02X%02X%02X\n", channel, 
				17 + ( 0 * channels * 3) + (( channel - 1) * 3) + 0, 
				0xff & samtac_packet[17 + (0 * channels * 3) + ((channel - 1) * 3) + 0], 
				0xff & samtac_packet[17 + (0 * channels * 3) + ((channel - 1) * 3) + 1], 
				0xff & samtac_packet[17 + (0 * channels * 3) + ((channel - 1) * 3) + 2]);
	}

	for (i=0;i<sample_rate;i++) {
		demux_channel_data[(i * 4)] = samtac_packet[17 + (i * channels * 3) + ((channel - 1) * 3) + 2];
		demux_channel_data[(i * 4) + 1] = samtac_packet[17 + (i * channels * 3) + ((channel - 1) * 3) + 1];
		demux_channel_data[(i * 4) + 2] = samtac_packet[17 + (i * channels * 3) + ((channel - 1) * 3) + 0];
		if ((0x80 & demux_channel_data[(i * 4) + 2]) == 0x80) {
		//samtac_packet[17 + (i * channel * 3) + 0]
			//logit("et", "convert.c: 0x80 & packet: %02X\n", 
			demux_channel_data[(i * 4) + 3] = 0xff;
			//logit("et", "convert.c: hex: %02x%02x%02x%02x, dec: %d\n", demux_channel_data[(i * 4)], demux_channel_data[(i * 4)+ 1],
				//demux_channel_data[(i * 4)+2], demux_channel_data[(i * 4)+3], demux_channel_data[(i * 4)]);
			} else {
			demux_channel_data[(i * 4) + 3] = 0x00;
		}
		if (gcfg_debug > 3)
		{
			logit("et", "convert.c: channel: %d iterator: %d sample hex: %02X%02X%02X%02X\n", channel, 
				i, 
				0xff & demux_channel_data[(i * 4) + 3],
				0xff & demux_channel_data[(i * 4) + 2], 
				0xff & demux_channel_data[(i * 4) + 1],
				0xff & demux_channel_data[(i * 4) + 0]
				);
		}
	}
	//Copy demuxed and expanded SAMTAC data(for the channel specified) to the trace buffer
	memcpy(&trace_buffer.msg[sizeof(TRACE2_HEADER)], demux_channel_data, sample_rate*4*sizeof(char));
	
	trace2_hdr->pinno = 0;		/* Unknown item */
	trace2_hdr->nsamp = sample_rate;
	
	//turn start time to epoch
	dt.tm_year = samtac_packet[11] + 100;
	dt.tm_mon = samtac_packet[12] - 1;
	dt.tm_mday = samtac_packet[13];
	dt.tm_hour = samtac_packet[14];
	dt.tm_min = samtac_packet[15];
	dt.tm_sec = samtac_packet[16];
	epoch = timegm_ew( &dt );

	if (gcfg_debug > 2)
	{
		logit("et", "convert.c: epoch: %d\n", epoch);
	}

	trace2_hdr->starttime = epoch;

	if (gcfg_debug > 2)
	{
		logit("et", "convert.c: hrtime: %f\n", trace2_hdr->starttime);
	}
	
	trace2_hdr->endtime = trace2_hdr->starttime+ (double)(sample_rate-1) / sample_rate;
	trace2_hdr->samprate = (double)sample_rate;
	
	if (gcfg_debug > 2)
	{
		logit("et", "convert.c: sample_rate: %f\n", trace2_hdr->samprate);
	}

	strcpy(trace2_hdr->sta,scn->site);
	strcpy(trace2_hdr->net,scn->net);
	strcpy(trace2_hdr->chan,scn->chan);
	strcpy(trace2_hdr->loc,scn->loc);

	//Set data type
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
