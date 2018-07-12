/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *   
 *    $Id: convert.h 3535 2009-01-15 18:15:31Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2009/01/15 18:15:31  tim
 *     Clean up externs.h, k2comif.h and k2ewerrs.h
 *
 *     Revision 1.1  2008/10/29 16:00:59  tim
 *     *** empty log message ***
 *
 *     Revision 1.1  2008/10/23 21:00:03  tim
 *     Updating to use SCNL, and ewtrace
 *
 */

#ifndef CONVERT_EW
#define CONVERT_EW


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace_buf.h>
#include "scnl_map.h"
#include "convert.h"

/*
	Returns:
		UPON SUCCESS (trace data converted):
			-the address of the static memory buffer containing the TRACE MESSAGE
		UPON FAILURE:
			-NULL pointer;
			-sets the value pointed to by the long ptr to ZERO.
*/

//static TracePacket trace_buffer;

char *convert_samtac_to_ewtrace(char *samtac_packet, SCN *scn, long *out_message_size, int channel);

#endif
