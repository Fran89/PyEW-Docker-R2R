/*
 * Standalone program to read MSEED data files and write
 * earthworm TRACE_BUF2 messages.

 * this was based on ms2tb that used qlib2 functions, but was switched to libmseed (from Chad Trabant)
 * because that library is easily supported under windows.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#ifdef _WINNT
#include <io.h>
#endif

#include <errno.h>
#include "trace_buf.h"
#include "sachead.h"
#include "swap.h"
#include "time_ew.h"

#include "libmseed.h"

#define TRACE2_UNDEF_STRING "  "

int convert_mseed_to_tank(char * MSfile, char * tankfile, int max_samps, TRACE2X_HEADER *t2h)
{
  TRACE2_HEADER *trh;
  MSRecord *msr = 0;
  char outbuf[MAX_TRACEBUF_SIZ];
  FILE *fp;
  int32_t *lp;
  int retcode;
  long total_bytes_written=0;
  int writelen;
  int i, datalen;
  int lastrecord=0;
  int reclen=0;
#ifdef _WINNT
  int fn;
#endif
  trh = (TRACE2_HEADER *)outbuf;
  
  if (strcmp(tankfile, "stdout") == 0) {
	fp=stdout;
#ifdef _WINNT
        fn = _fileno(stdout);
        if  ( (retcode = _setmode(fn, _O_BINARY)) == -1 )
        {
          fprintf(stderr, "convert_mseed_to_tank: error setting binary mode for stdout for WINDOZE\n");
          return( 1 );
        }
#endif
  } else if ( (fp = fopen(tankfile, "ab")) == (FILE *)NULL) {
    fprintf(stderr, "convert_mseed_to_tank: error opening tankfile %s\n", tankfile);
    return( 1 );
  }

  if ( (retcode = ms_readmsr (&msr, MSfile, reclen, NULL, &lastrecord,
                                 1, 1, 0)) != MS_NOERROR )
  {
    fprintf(stderr, "convert_mseed_to_tank: error opening and reading first mseed record of %s\n", MSfile);
    return( 1 );
  }
   



/* set the pointer to the data section of the TBUF */
 lp = (int32_t *)(trh+1);
 do
  {
    int32_t *ds;
    double starttime;
    int actual_samples;		/* number of samples written to tracebuf */
    int current_sample;		/* pointer into msr mseed record data */

    memset((void*)trh, 0, sizeof(TRACE2_HEADER));
    trh->version[0] = TRACE2_VERSION0;
    trh->version[1] = TRACE2_VERSION1;
    
    if (t2h->sta[0] != 0) {
        strcpy(trh->sta, t2h->sta);
    } else {
        strcpy(trh->sta, msr->station);
    }

    if (t2h->chan[0] != 0) {
        strcpy(trh->chan, t2h->chan);
    } else {
        strcpy(trh->chan, msr->channel);
    }

    if (t2h->net[0] != 0) {
        strcpy(trh->net, t2h->net);
    } else {
        strcpy(trh->net, msr->network);
    }
    
    if (t2h->loc[0] != 0) {
        strcpy(trh->loc, t2h->loc);
    } else if (strlen(msr->location) != 0 && strcmp(msr->location, "  ")!=0) {
        strncpy(trh->loc, msr->location, TRACE2_LOC_LEN-1);
        trh->loc[TRACE2_LOC_LEN-1] = '\0';
    } else {
        strcpy(trh->loc, LOC_NULL_STRING);
    }
  
    trh->quality[0] = msr->dataquality;
    trh->samprate = msr->samprate;
  
#ifdef _INTEL
    strcpy(trh->datatype, "i4");
#endif
#ifdef _SPARC
    strcpy(trh->datatype, "s4");
#endif


    /* check encoding each time */
    if (msr->encoding == DE_STEIM1 || msr->encoding == DE_STEIM2 || msr->encoding == DE_INT32) {
	/* then we continue on */
    	trh->pinno = 0;      
    } else {
 	fprintf(stderr, "Error with mseed encoding type that is not supported in %s\n", MSfile);
        return( 1 );
    }

    /* should also really check that it is the same station from run to run, 
	but guess that is not critical yet */

    
    current_sample = 0;
    starttime = msr->starttime/1000000.; 
    while (current_sample < msr->numsamples)
    {
      if (msr->numsamples - current_sample > max_samps) 
      { 
         actual_samples = max_samps;
      } else {
         actual_samples = (int)(msr->numsamples-current_sample);
      }
      trh->starttime = starttime + current_sample/msr->samprate;
      trh->endtime = trh->starttime + (actual_samples-1)/msr->samprate;
      trh->nsamp = actual_samples;
      ds = (int32_t *) msr->datasamples;
      for(i = 0; i < actual_samples; i++)
      {
        lp[i] = (int32_t) *(ds+current_sample);
        current_sample++;
      }
      datalen = actual_samples*4;
      if ( (writelen=(int)fwrite(trh, 1, sizeof(TRACE_HEADER) + datalen, fp))
        != sizeof(TRACE_HEADER) + datalen)
      {
        fprintf(stderr, "Error writing tankfile: %s\n", strerror(errno));
        return( 1 );
      }

/*      fprintf(stderr, "DEBUG: wrote %d bytes\n", writelen); */
      total_bytes_written+= writelen;
    } /* end of while writing tracebufs out for a given msr */
  }
  while ( (retcode = ms_readmsr (&msr, MSfile, reclen, NULL, &lastrecord,
                                 1, 1, 0)) == MS_NOERROR );
  if (retcode != MS_ENDOFFILE) 
  {
    fprintf (stderr, "Error reading %s: %s\n", MSfile, ms_errorstr(retcode));
  }
/*
  fprintf(stderr, "DEBUG: wrote total %ld bytes\n", total_bytes_written);
*/
  ms_readmsr(&msr, NULL, 0, NULL, NULL,1, 0, 0);	/* cleans up memory and closes the file */
  fclose(fp);	/* close the output file */
  return( 0 );
}
