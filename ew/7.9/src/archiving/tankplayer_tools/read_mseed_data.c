/************************************************************************/
/*  Routine to read data from MiniSEED data records.			*/
/*									*/
/*	Douglas Neuhauser						*/
/*	Seismological Laboratory					*/
/*	University of California, Berkeley				*/
/*	doug@seismo.berkeley.edu					*/
/*									*/
/************************************************************************/

/*
 * Copyright (c) 1996-2002 The Regents of the University of California.
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research and non-profit purposes,
 * without fee, and without a written agreement is hereby granted,
 * provided that the above copyright notice, this paragraph and the
 * following three paragraphs appear in all copies.
 * 
 * Permission to incorporate this software into commercial products may
 * be obtained from the Office of Technology Licensing, 2150 Shattuck
 * Avenue, Suite 510, Berkeley, CA  94704.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY
 * FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND
 * ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF
 * CALIFORNIA HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef lint
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* added in these next 2 directly instead of via procs.h, this is the only change made for ms2tb */
#include "qlib2.h"
#define FATAL(str) { fprintf (stderr, str); exit(1); }

#define	INCR	262144

/************************************************************************/
/*  read_mseed_data							*/
/*	Read MiniSEED data.						*/
/************************************************************************/
int read_mseed_data 
   (DATA_HDR **phdr,		/* ptr to ptr to DATA_HDR (returned).	*/
    int **pdata,		/* ptr to ptr to data buffer (returned).*/
    int stream_tol,		/* time tolerance for continuous data.	*/
    FILE *fp)			/* input FILE.				*/
{
    int *data = NULL;
    int npts = 0;
    int n_alloc = 0;
    DATA_HDR *ihdr = NULL;
    DATA_HDR *hdr = NULL;
    char *mseed = NULL;
    int seconds, usecs, n;
    double thresh, slew;
    int status, blksize;
    int	blknum = 0;

    while ((status = blksize = read_ms_record (&hdr, &mseed, fp)) > 0) {
	++blknum;
	if (!ihdr) {
	    ihdr = dup_data_hdr (hdr);
	    time_interval2 (1, hdr->sample_rate, hdr->sample_rate_mult, 
			    &seconds, &usecs);
	    thresh = (stream_tol > 0) ? stream_tol * USECS_PER_TICK : 
		.5 * (seconds * USECS_PER_SEC + usecs);
	}
	else {
	    /* Error checking on data stream.		    */
	    /* Ensure that this data is contiguous enough.  */
	    if (strcmp(ihdr->station_id,hdr->station_id)!=0 ||
		strcmp(ihdr->network_id,hdr->network_id)!=0 ||
		strcmp(ihdr->channel_id,hdr->channel_id)!=0) {
		FATAL("Header for wrong channel\n");
	    }
	    time_interval2 (npts, hdr->sample_rate, hdr->sample_rate_mult, 
			    &seconds, &usecs);
	    slew = tdiff (add_time(ihdr->begtime, seconds, usecs), hdr->begtime);
	    if (fabs(slew) > thresh) {
		fprintf(stderr,"Max slew of %.6lf seconds exceeded - time series prematurely terminated.\n",
			((double)thresh)/USECS_PER_SEC);
		status = EOF;
		break;
	    }
	}

	/* Ensure we have enough space for the unpacked data.		*/
	while (npts+hdr->num_samples>n_alloc) {
	    data = (data) ? (int *)realloc (data, (n_alloc+INCR) * sizeof(int)) :
		(int *)malloc ((n_alloc+INCR) * sizeof(int));
	    if (data) n_alloc += INCR;
	    else {
		FATAL("Unable to malloc/realloc for data\n");
	    }
	}

	n = ms_unpack (hdr, hdr->num_samples, mseed, data+npts);
	if (n != hdr->num_samples) {
	    FATAL("Error unpacking MiniSEED data.\n");
	}
	npts += n;
	free_data_hdr(hdr);
	hdr = NULL;
    }
    if (mseed) free (mseed);
    *pdata = data;
    *phdr = ihdr;
    ihdr->num_samples = npts;
    if (status != EOF) {
	fprintf (stderr, "Error reading miniSEED block %d.\n", ++blknum);
	return (status);
    }
    return (npts);
}
