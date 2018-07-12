/*
 * Standalone program to read MSEED data files and write
 * earthworm TRACE_BUF2 messages.
 * That file can then be made into a tankplayer file using remux_tbuf.
 *
 * reworked from Pete Lombard sac2tb program using read_mseed_data from Doug N's ms2ah program
 *  reworked by Paul Friberg, ISTI.com Aug 8, 2007
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "trace_buf.h"
#include "sachead.h"
#include "swap.h"
#include "time_ew.h"

#include "qlib2.h"


/* from Doug Neuhauser's ms2ah code */
int read_mseed_data 
   (DATA_HDR **phdr,            /* ptr to ptr to DATA_HDR (returned).   */
    int **pdata,                /* ptr to ptr to data buffer (returned).*/
    int stream_tol,             /* time tolerance for continuous data.  */
    FILE *fp);                  /* input FILE.                          */


#define DEF_MAX_SAMPS 100
#define TRACE2_UNDEF_STRING "  "

/* Internal Function Prototypes */
void usage( char * );
static int strib( char *string );

int main(int argc, char **argv)
{
  TRACE2_HEADER *trh;
  FILE *fp;
  char *MSfile, outbuf[MAX_TRACEBUF_SIZ];
  int32_t *lp;
  double sTime, sampInt;
  int arg;
  int i, npts, datalen;
  int max_samps = DEF_MAX_SAMPS;
  DATA_HDR *hdr;
  int *data, *current_data;
  int stream_tol = 0;
  trh = (TRACE2_HEADER *)outbuf;
  
  if (argc < 2)
    usage( argv[0] );

  arg = 1;
  while (arg < argc && argv[arg][0] == '-')
  {
    switch(argv[arg][1])
    {
    case 'n':
      arg++;
      max_samps = atoi(argv[arg]);
      break;
    default:
      usage( argv[0] );
    }
    arg++;
  }
  if (argc - arg != 1)
    usage( argv[0] );
  
  MSfile = argv[arg];
  arg++;
  
  if ( (fp = fopen(MSfile, "rb")) == (FILE *)NULL)
  {
    fprintf(stderr, "%s: error opening %s\n", argv[0], MSfile);
    exit( 1 );
  }

  npts = read_mseed_data (&hdr, &data, stream_tol, fp);
  if (npts <= 0)  
  {
    fprintf(stderr, "%s: no points opening %s\n", argv[0], MSfile);
    exit( 1 );
  }

  memset((void*)trh, 0, sizeof(TRACE2_HEADER));
  trh->version[0] = TRACE2_VERSION0;
  trh->version[1] = TRACE2_VERSION1;
  
  strncpy(trh->sta, hdr->station_id, TRACE2_STA_LEN);
  trh->sta[TRACE2_STA_LEN-1] = '\0';
  strib(trh->sta);
  
  strncpy(trh->chan, hdr->channel_id, TRACE2_CHAN_LEN);
  trh->chan[TRACE2_CHAN_LEN-1] = '\0';
  strib(trh->chan);
  
  strncpy(trh->net, hdr->network_id, TRACE2_NET_LEN);
  trh->net[TRACE2_NET_LEN-1] = '\0';
  strib(trh->net);
  
  
  if (strlen(hdr->location_id) != 0 ) {
      strncpy(trh->loc, hdr->location_id, TRACE2_LOC_LEN-1);
      trh->loc[TRACE2_LOC_LEN-1] = '\0';
      /* fprintf(stderr, "using NON default string for location code: '%s'\n", hdr->location_id); */
      /* strib(trh->loc); */
  } else {
      strcpy(trh->loc, LOC_NULL_STRING);
      /* fprintf(stderr, "using default string for location code: %s\n", trh->loc); */
  }
  
  trh->quality[0] = '\0';
  trh->samprate = sps_rate(hdr->sample_rate,hdr->sample_rate_mult);
  sampInt = 1.0/trh->samprate;
  

#ifdef _INTEL
  strcpy(trh->datatype, "i4");
#endif
#ifdef _SPARC
  strcpy(trh->datatype, "s4");
#endif

  sTime = (double)unix_time_from_int_time(hdr->begtime) +
            ((double)(hdr->begtime.usec)/USECS_PER_SEC);      /* start time */
  lp = (int32_t *)(trh+1);
  current_data = data;
  while (npts >= max_samps)
  {
    datalen = max_samps * sizeof(int);
    trh->starttime = sTime;
    sTime += sampInt * max_samps;
    trh->endtime = sTime - sampInt;
    trh->nsamp = max_samps;
    trh->pinno = 0;      
	
    for(i = 0; i < max_samps; i++)
      lp[i] = (int32_t)(current_data[i]);
    current_data += max_samps;
    
    npts -= max_samps;
    
    if (fwrite(trh, 1, sizeof(TRACE_HEADER) + datalen, stdout) 
        != sizeof(TRACE_HEADER) + datalen)
    {
      fprintf(stderr, "Error writing tankfile: %s\n", strerror(errno));
      exit( 1 );
    }
  }
  if (npts > 0)
  {  /* Get the last few crumbs */
    datalen = npts * sizeof(float);
    trh->starttime = sTime;
    sTime += sampInt * npts;
    trh->endtime = sTime - sampInt;
    trh->nsamp = npts;
    trh->pinno = 0;      
    
    for(i = 0; i < npts; i++)
      lp[i] = (int32_t)(current_data[i]);
    
    if (fwrite(trh, 1, sizeof(TRACE_HEADER) + datalen, stdout) 
        != sizeof(TRACE_HEADER) + datalen)
    {
      fprintf(stderr, "Error writing tankfile: %s\n", strerror(errno));
      exit( 1 );
    }
  }
  
  return( 0 );
}


void usage( char *argv0 )
{
  fprintf(stderr, "Usage: %s [-n max-samples] infile >> outfile\n", argv0);
  exit( 1 );
}
  
/*
 * strib: strips trailing blanks (space, tab, newline)
 *    Returns: resulting string length.
 */
static int strib( char *string )
{
  int i, length = 0;
  
  if ( string && (length = strlen( string )) > 0)
  {
    for ( i = length-1; i >= 0; i-- )
    {
      switch ( string[i])
      {
      case ' ':
      case '\n':
      case '\t':
        string[i] = '\0';
        break;
      default:
        return ( i+1 );
      }
    }
  }
  
  return length;
}
