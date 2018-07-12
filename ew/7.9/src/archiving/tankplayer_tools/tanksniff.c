#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <trace_buf.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <time_ew.h>

/* tanksniff - a quick and dirty tank player tank  sniffing tool, shows output like sniffwave */

#define VERSION "0.0.4 - 2016-05-27"
int main( int argc, char *argv[] )
{
  TracePacket   pkt;              /* tracebuf message read from file      */
  FILE         *ifp;              /* file of waveform data to read from   */
  double 	hdtime;           /* time read from TRACEBUF2 header      */
  int32_t       nsamp;            /* #samples in this message             */
  char          byte_order;       /* byte order of this TYPE_TRACEBUF2 msg */
  int           byte_per_sample;  /* for TYPE_TRACEBUF2 msg               */
  char          orig_datatype[3]; /* data type BEFORE WaveMsg2Local() */
  long          datalen;
  int           rc;
  long 		totbytes = 0;
  char stime[30], etime[30];

  if ( argc != 2 )
  {
    fprintf(stderr, "Usage: tanksniff <tankfile>\n" );
    fprintf(stderr, " version %s\n", VERSION );
    return( 0 );
  }

  /* Open a waveform files
   ***********************/
  ifp = fopen( argv[1], "rb" );
  if ( ifp == NULL )
  {
    fprintf(stderr, "tanksniff: Cannot open tankfile <%s>\n", argv[1] );
    return(1);
  }

  /* Read thru file reading headers; gather info about all tracebuf messages
  **************************************************************************/
  while( fread( pkt.msg, sizeof(char), sizeof( TRACE2_HEADER ), ifp ) 
         == sizeof( TRACE2_HEADER ) )  
  {
     hdtime          = pkt.trh.endtime;
     nsamp           = pkt.trh.nsamp;
     byte_order      = pkt.trh.datatype[0];
     byte_per_sample = atoi(&pkt.trh.datatype[1]);
     strcpy(orig_datatype, pkt.trh.datatype);
#ifdef _SPARC
     if( byte_order == 'i' || byte_order == 'f' ) 
     {
       SwapInt32( &nsamp );
       SwapDouble( &hdtime );
     }
#endif
#ifdef _INTEL
     if( byte_order == 's' || byte_order == 't' )
     {
       SwapInt32( &nsamp );
       SwapDouble( &hdtime );
     }
#endif

     if ( (rc=WaveMsg2MakeLocal( &pkt.trh2 )) < 0 )
     {
       fprintf(stderr, "tanksniff: WaveMsg2MakeLocal() error.\n" );
       fprintf(stderr,"tbuf: %4s %3s %2s %2s  %13.2f+%4.2f %3d %3d %s %d at offset: %ld\n",
             pkt.trh2.sta, pkt.trh2.chan, pkt.trh2.net, pkt.trh2.loc,
             pkt.trh2.starttime, pkt.trh2.endtime-pkt.trh2.starttime,
             pkt.trh2.nsamp, (int)(pkt.trh2.samprate),
             "*** WM2ML BARFED:", rc, totbytes);
       if(rc == -1)
       {
         fprintf(stderr, "tanksniff: WaveMsg2MakeLocal() failed!  Processor hopelessly lost at offset %ld.  quitting!\n",
               totbytes);
	 exit(1);
       }
     }

     /* simulate sniffwave output */
      datestr23 (pkt.trh2.starttime, stime, 256);
      datestr23 (pkt.trh2.endtime,   etime, 256);
      datalen  = byte_per_sample * nsamp;
      fprintf( stdout, "%s.%s.%s.%s (%c %c) ",
                   pkt.trh2.sta, pkt.trh2.chan, pkt.trh2.net, pkt.trh2.loc, pkt.trh2.version[0], pkt.trh2.version[1] );
      if (pkt.trh2.samprate < 1.0) { /* more decimal places for slower sample rates */
          fprintf( stdout, "%d %s %4d %6.4f %s (%.4f) %s (%.4f) %ldbytes\n",
               pkt.trh2.pinno, orig_datatype, pkt.trh2.nsamp, pkt.trh2.samprate,
               stime, pkt.trh2.starttime,
               etime, pkt.trh2.endtime, datalen + (long)sizeof( TRACE2_HEADER ));
      } else {
          fprintf( stdout, "%d %s %4d %.1f %s (%.4f) %s (%.4f) %ldbytes ",
                         pkt.trh2.pinno, orig_datatype, pkt.trh2.nsamp, pkt.trh2.samprate,
                         stime, pkt.trh2.starttime,
                         etime, pkt.trh2.endtime, datalen + (long)sizeof( TRACE2_HEADER ));
      }
      fprintf(stdout, "\n");


     /* Store pertinent info about this tracebuf message
     **************************************************/
     totbytes += datalen + sizeof( TRACE2_HEADER );
     
     /* Skip over data samples
     ************************/
     fseek( ifp, datalen, SEEK_CUR );

  } /* end while first time thru */

  fclose( ifp );
  return( 0 );
}
