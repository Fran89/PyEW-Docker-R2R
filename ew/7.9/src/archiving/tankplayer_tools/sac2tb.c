/*
 * Standalone program to read SAC data files and write
 * earthworm TRACE_BUF2 messages.
 * That file can then be made into a tankplayer file using remux_tbuf.
 *
 * Pete Lombard; May 2001
 */

#define VERSION_NUM  "0.0.5 2016-08-31"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "trace_buf.h"
#include "sachead.h"
#include "swap.h"
#include "time_ew.h"

#define DEF_MAX_SAMPS 100

/* Internal Function Prototypes */
void usage( char * );
static int readSACHdr(FILE *, struct SAChead *);
static double sacRefTime( struct SAChead * );
static int strib( char *string );

int main(int argc, char **argv)
{
  TRACE2_HEADER *trh;
  FILE *fp;
  struct SAChead sh;
  char *SACfile=NULL, outbuf[MAX_TRACEBUF_SIZ];
  float *seis, *sp;       /* input trace buffer */
  int32_t *lp;
  double sTime, sampInt;
  int arg;
  int swapIsNeeded;
  int i, npts, datalen;
  int max_samps = DEF_MAX_SAMPS;
  const char sac_undef[] = SACSTRUNDEF;
  char net[3];
  char chan[4];
  char loc[3];
  char sta[6];
  int  seisan_chan_fix=0;
  FILE *fp_out = NULL;
  int appendOutput = 0;
  float multiplier = 1.0;
  trh = (TRACE2_HEADER *)outbuf;
  net[0]=0;
  sta[0]=0;
  chan[0]=0;
  loc[0]=0;
  SACfile = NULL;
  
  if (argc < 2)
    usage( argv[0] );

  arg = 1;
  while (arg < argc && argv[arg][0] == '-')
  {
    switch(argv[arg][1])
    {
    case 'N':
      arg++;
      strcpy(net, argv[arg]);
      break;
    case 'S':
      arg++;
      strcpy(sta, argv[arg]);
      break;
    case 'C':
      arg++;
      strcpy(chan, argv[arg]);
      break;
    case 'L':
      arg++;
      strcpy(loc, argv[arg]);
      break;

    case 'c':
      seisan_chan_fix=1;
      break;
    case 'n':
      arg++;
      max_samps = atoi(argv[arg]);
      break;
    case 'm':
      arg++;
      multiplier = (float)atof(argv[arg]);
      break;
    case 'a':
      appendOutput = 1;
      break;
    default:
      usage( argv[0] );
    }
    arg++;
  }
  if (argc - arg  == 1)
  {
#ifdef _WINNT
    usage( argv[0] );
    exit( 1 );
#else
    SACfile = argv[arg];
    arg++;
    fp_out = stdout;
#endif
  } else if ( argc - arg == 2)
  {
    SACfile = argv[arg];
    arg++;
  } else
    usage( argv[0] );
  
  if ( (fp = fopen(SACfile, "rb")) == (FILE *)NULL)
  {
    fprintf(stderr, "%s: error opening %s\n", argv[0], SACfile);
    exit( 1 );
  }
  if ( fp_out == NULL ) 
  {
    fp_out = fopen( argv[arg], appendOutput ? "ab" : "wb" );
    if ( fp_out == (FILE *)NULL )
    {
      fprintf(stderr, "%s: error opening %s\n", argv[0], argv[arg]);
      fclose( fp );
      exit( 1 );
    }
  }

  /* mtheo 2007/10/19 */
  swapIsNeeded = readSACHdr(fp, &sh);
  if ( swapIsNeeded < 0)
  {
    fclose(fp);
    exit( 1 );
  }

  npts = (int)sh.npts;
  datalen = npts * sizeof(float);
  if ( (seis = (float *) malloc((size_t)datalen)) == (float *)NULL)
  {
    fprintf(stderr, "%s: out of memory for %d SAC samples\n", argv[0], npts);
    exit( 1 );
  }

  if (sh.delta < 0.001)
  {
    fprintf(stderr, "SAC sample period too small: %f\n", sh.delta);
    exit( 1 );
  }

  memset((void*)trh, 0, sizeof(TRACE2_HEADER));
  
  strncpy(trh->sta, sh.kstnm, TRACE2_STA_LEN);
  trh->sta[TRACE2_STA_LEN-1] = '\0';
  strib(trh->sta);
  
  if (seisan_chan_fix) {
     /* some seisan chans look like: EH Z 
                                     0123
      */
     strncpy(trh->chan, sh.kcmpnm, TRACE2_CHAN_LEN);
     trh->chan[2] = trh->chan[3];
     trh->chan[3] = 0;
  } else {
     strncpy(trh->chan, sh.kcmpnm, TRACE2_CHAN_LEN-1);
     trh->chan[TRACE2_CHAN_LEN-1] = '\0';
     strib(trh->chan);
  }
  if ( chan[0] != 0 ) {
     strcpy(trh->chan, chan);
  }
  
  if (net[0] != 0) {
     strncpy(trh->net, net, TRACE2_NET_LEN-1);
     trh->net[TRACE2_NET_LEN-1] = '\0';
  } else {
     /* get the net code from the SAC file */
     strncpy(trh->net, sh.knetwk, TRACE2_NET_LEN-1);
     trh->net[TRACE2_NET_LEN-1] = '\0';
  }
  strib(trh->net);
  
  if (memcmp(sh.khole, sac_undef, TRACE2_LOC_LEN-1) != 0 && strncmp(sh.khole, "  ", 2) != 0 ) {
      strncpy(trh->loc, sh.khole, TRACE2_LOC_LEN);
      trh->loc[TRACE2_LOC_LEN-1] = '\0';
      strib(trh->loc);
  } else {
      strcpy(trh->loc, LOC_NULL_STRING);
  }
  if (loc[0] != 0) {
     strncpy(trh->loc, loc, TRACE2_LOC_LEN-1);
     trh->net[TRACE2_LOC_LEN-1] = '\0';
  } 
  if (sta[0] != 0) {
     strcpy(trh->sta, sta);
  } 
  trh->version[0] = TRACE2_VERSION0;
  trh->version[1] = TRACE2_VERSION1;

  
  trh->quality[0] = '\0';
  sampInt = (double)sh.delta;
  trh->samprate = 1.0/sampInt;

#ifdef _INTEL
  strcpy(trh->datatype, "i4");
#endif
#ifdef _SPARC
  strcpy(trh->datatype, "s4");
#endif

  sTime = (double)sh.b + sacRefTime( &sh );
  fprintf(stderr, "start %4.4d,%3.3d,%2.2d:%2.2d:%2.2d.%4.4d %f\n", (int)sh.nzyear,
          (int)sh.nzjday, (int)sh.nzhour, (int)sh.nzmin, (int)sh.nzsec, (int)sh.nzmsec, sTime);
  
  /* Read the sac data into a buffer */
  if ( (i = (int)fread(seis, sizeof(float), sh.npts, fp)) != npts)
  {
    fprintf(stderr, "error reading SAC data: %s\n", strerror(errno));
    exit( 1 );
  }
  fclose(fp);

  /* mtheo 2007/11/15 */
  if(swapIsNeeded == 1) {
      for (i = 0; i < npts; i++)
	  SwapInt32( (int32_t *) &(seis[i]));
  }
  
  sp = seis;
  lp = (int32_t *)(trh+1);
  while (npts >= max_samps)
  {
    datalen = max_samps * sizeof(float);
    trh->starttime = sTime;
    sTime += sampInt * max_samps;
    trh->endtime = sTime - sampInt;
    trh->nsamp = max_samps;
    trh->pinno = 0;      
    
    for(i = 0; i < max_samps; i++)
      lp[i] = (int32_t)((sp[i])*multiplier);
    sp += max_samps;
    npts -= max_samps;
    
    if (fwrite(trh, 1, sizeof(TRACE2_HEADER) + datalen, fp_out) 
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
      lp[i] = (int32_t)(sp[i]);
    
    if (fwrite(trh, 1, sizeof(TRACE_HEADER) + datalen, fp_out) 
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
#ifdef _WINNT
  fprintf(stderr, "Usage: %s [-c][-m multi] [-N NN] [-C CCC] [-S SSSSS] [-L LL] [-n max-samples] [-a] infile outfile\n", argv0);
#else
  fprintf(stderr, "Usage: %s [-c][-m multi] [-N NN] [-C CCC] [-S SSSSS] [-L LL] [-n max-samples] infile >> outfile\n", argv0);
  fprintf(stderr, "    or %s [-c][-m multi] [-N NN] [-n max-samples] [-a] infile outfile\n", argv0);
#endif
  fprintf(stderr, "    -N NN is the network code to use from the cmdline instead of SAC file\n");
  fprintf(stderr, "    -L LL is the location code to use from the cmdline instead of SAC file\n");
  fprintf(stderr, "    -C CCC is the chan code to use from the cmdline instead of SAC file\n");
  fprintf(stderr, "    -S SSSSS is the station name to use from the cmdline instead of SAC file\n");
  fprintf(stderr, "    -c is a flag to fix a SEISAN problem with chans written in as EH Z\n");
  fprintf(stderr, "    -m multi is a multiplier factor against the SAC float data\n");
  fprintf(stderr, "    Version: %s\n", VERSION_NUM);
  exit( 1 );
}

  
/*
 * readSACHdr: read the header portion of a SAC file into memory.
 *  arguments: file pointer: pointer to an open file from which to read
 *             filename: pathname of open file, for logging.
 * returns: 0 on success
 *          1 on success and if byte swapping is needed
 *         -1 on error reading file
 *     The file is left open in all cases.
 */
static int readSACHdr(FILE *fp, struct SAChead *pSH)
{
  int i;
  struct SAChead2 *pSH2;
  int fileSize;
  int ret = 0;

  // obtain file size:
  fseek (fp , 0 , SEEK_END);
  fileSize = ftell (fp);
  rewind (fp);

  pSH2 = (struct SAChead2 *)pSH;
  
  if (fread( pSH, sizeof(struct SAChead2), 1, fp) != 1)
  {
    fprintf(stderr, "readSACHdr: error reading SAC file: %s\n",
            strerror(errno));
    return -1;
  }
  
  /* mtheo 2007/10/19
   * Guessing if byte swapping is needed
   * fileSize should be equal to sizeof(struct SAChead) + (pSH->npts * sizeof(float)) */
  if(fileSize != (int)(sizeof(struct SAChead) + (pSH->npts * sizeof(float))) ) {
      ret = 1;
      fprintf(stderr, "WARNING: Swapping is needed! (fileSize %d, pSH.npts %ld)\n", fileSize, (long)(pSH->npts));
      for (i = 0; i < NUM_FLOAT; i++)
	  SwapInt32( (int32_t *) &(pSH2->SACfloat[i]));
      for (i = 0; i < MAXINT; i++)
	  SwapInt32( (int32_t *) &(pSH2->SACint[i]));
      if(fileSize != (int)(sizeof(struct SAChead) + (pSH->npts * sizeof(float))) ) {
	  fprintf(stderr, "ERROR: Swapping is needed again! (fileSize %d, pSH.npts %ld)\n",
		  fileSize, (long)(pSH->npts));
	  ret = -1;
      }
  } else {
      fprintf(stderr, "Swapping is not needed! (fileSize %d, pSH.npts %ld)\n", fileSize, (long)(pSH->npts));
  }
  
// /* SAC files are always in "_SPARC" byte order; swap if necessary */
// #ifdef _INTEL
//   for (i = 0; i < NUM_FLOAT; i++)
//     SwapLong( (long *) &(pSH2->SACfloat[i]));
//   for (i = 0; i < MAXINT; i++)
//     SwapLong( (long *) &(pSH2->SACint[i]));
// #endif
  
  return ret;
}

/*
 * sacRefTime: return SAC reference time as a double.
 *             Uses a trick of mktime() (called by timegm_ew): if tm_mday
 *             exceeds the normal range for the month, tm_mday and tm_mon
 *             get adjusted to the correct values. So while mktime() ignores
 *             tm_yday, we can still handle the julian day of the SAC header.
 *             This routine does NOT check for undefined values in the
 *             SAC header.
 *  Returns: SAC reference time as a double.
 */
static double sacRefTime( struct SAChead *pSH )
{
  struct tm tms;
  double sec;
  
  tms.tm_year = pSH->nzyear - 1900;
  tms.tm_mon = 0;    /* Force the month to January */
  tms.tm_mday = pSH->nzjday;  /* tm_mday is 1 - 31; nzjday is 1 - 366 */
  tms.tm_hour = pSH->nzhour;
  tms.tm_min = pSH->nzmin;
  tms.tm_sec = pSH->nzsec;
  tms.tm_isdst = 0;
  sec = (double)timegm_ew(&tms);
  
  return (sec + (pSH->nzmsec / 1000.0));
}

/*
 * strib: strips trailing blanks (space, tab, newline)
 *    Returns: resulting string length.
 */
static int strib( char *string )
{
  int i, length = 0;
  
  if ( string && (length = (int)strlen( string )) > 0)
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
