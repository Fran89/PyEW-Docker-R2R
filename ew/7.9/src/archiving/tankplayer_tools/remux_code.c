
   /********************************************************************
    *                           remux_tracebufs().c                           *
    *                                                                  *
    *      Reads a file of demultiplexed TYPE_TRACEBUF2 msgs -         *
    *  one that contains all msgs for a given SCNL, followed by all    * 
    *  msgs for the next SCNL, etc, as could be built from multiple    * 
    *  binary requests to wave_serverV.                                *
    *      Writes a new file containing all of the TRACEBUF2 messages  *
    *  arranged in chronological order. This new file can then be used *
    *  as input to tankplayer, allowing the QA of earthworm with data  *
    *  from wave_serverV. The data may not be in exactly the same      *
    *  order as it was originally, but it'll be close.                 *
    *                                                                  *  
    *  Command line arguments:                                         *
    *     arvg[1] = name of input file  (arranged by SCNL)             *
    *     argv[2] = name of output file (chronological)                *
    *                                                                  *
    *  Lynn Dietz  April 7, 1999                                       *
    ********************************************************************/

/* Paul Friberg - converted to a function so other modules can use it directly too 
	added in start_epoch, end_epoch for cutting functionality. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <trace_buf.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <time_ew.h>
#include "remux.h"

#define MAX_NUMBUF         500000        

typedef struct _TBUF {
    int    offset;    /* offset in bytes from beginning of input file */
    int    size;      /* length in bytes of this TRACEBUF2 message    */
    double time;      /* a time from the header of this TRACEBUF2 msg */
} TBUF;

int compare_time( const void *, const void * ); /* function for qsort()*/

int remux_tracebufs(char *intank, char *outtank, double start_epoch, double end_epoch)
{
  TracePacket   pkt;              /* tracebuf message read from file      */
  TBUF         *tbuf;             /* array of info structures             */
  FILE         *ifp;              /* file of waveform data to read from   */
  FILE         *ofp;              /* file of waveform data to write to    */
  double 	hdtime;           /* time read from TRACEBUF2 header      */
  int32_t       nsamp;            /* #samples in this message             */
  char          byte_order;       /* byte order of this TYPE_TRACEBUF2 msg */
  int           byte_per_sample;  /* for TYPE_TRACEBUF2 msg               */
  int           totbytes = 0;     /* total # bytes read from file so far  */
  int           nbuf = 0;         /* total # msgs read from file so far   */
  int           maxnumbuf;        /* number of buffers we can store       */
  long          datalen;
  int           ib;
  int           rc;

  maxnumbuf = MAX_NUMBUF;

  /* Open a waveform files
   ***********************/
  ifp = fopen( intank, "rb" );
  if ( ifp == NULL )
  {
    logit("et", "remux_tracebufs(): Cannot open demuxed wavefile <%s>\n", intank );
    return(1);
  }
  ofp = fopen( outtank, "wb" );
  if ( ofp == NULL )
  {
    logit("et", "remux_tracebufs(): Cannot open output wavefile <%s>\n", outtank);
    return(1);
  }

  /* allocate work space
   *********************/
  tbuf = (TBUF *) malloc( maxnumbuf*sizeof(TBUF) );
  if ( tbuf == NULL )
  {
    logit("et", "remux_tracebufs(): error allocating %d bytes\n", 
             maxnumbuf*sizeof(TBUF) );
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
       logit( "et", "trex_tbuf: WaveMsg2MakeLocal() error.\n" );
       logit("e","tbuf: %4s %3s %2s %2s  %13.2f+%4.2f %3d %3d %s %d at offset: %d\n",
             pkt.trh2.sta, pkt.trh2.chan, pkt.trh2.net, pkt.trh2.loc,
             pkt.trh2.starttime, pkt.trh2.endtime-pkt.trh2.starttime,
             pkt.trh2.nsamp, (int)(pkt.trh2.samprate),
             "*** WM2ML BARFED:", rc, totbytes);
       if(rc == -1)
       {
         logit("et", "trex_tbuf: WaveMsg2MakeLocal() failed!  Processor hopelessly lost at offset %d.  quitting!\n",
               totbytes);
         goto done;
       }
     }

     /* Store pertinent info about this tracebuf message
     **************************************************/
     datalen           = byte_per_sample * nsamp;
     tbuf[nbuf].offset = totbytes;
     tbuf[nbuf].size   = datalen + sizeof( TRACE2_HEADER );
     tbuf[nbuf].time   = hdtime;
     

     /* Skip over data samples
     ************************/
     if( tbuf[nbuf].size > MAX_TRACEBUF_SIZ ) {
       logit("et",
               "remux_tracebufs(): msg[%d] overflows internal buffer[%d]\n",
	        tbuf[nbuf].size, MAX_TRACEBUF_SIZ );
       goto done;
     }
     fseek( ifp, datalen, SEEK_CUR );

     /* now check to see if we store this packet at all */
     totbytes += tbuf[nbuf].size;
     if (start_epoch != 0.0) 
     {
	/* continue if the packet's endtime is BEFORE start_epoch */
	if (pkt.trh2.endtime < start_epoch)
          continue;
     }
     if (end_epoch != 0.0) 
     {
	/* continue if the packet's starttime is AFTER end_epoch */
	if (pkt.trh2.starttime > end_epoch)
          continue;
     }

     if(rc == 0)  /* only track this tracebuf if WaveMsg2MakeLocal() SUCCEEDED */
       nbuf++;

     /* Allocate more space if necessary 
      **********************************/
     if( nbuf >= maxnumbuf )
     {
        maxnumbuf += 100000;
        tbuf = realloc( tbuf, maxnumbuf*sizeof(TBUF) );
        if( tbuf == NULL )
        {
           logit("et", "remux_tracebufs(): Could not realloc tbuf to %d bytes\n", 
                    maxnumbuf*sizeof(TBUF) );
           goto done;
        }
     }
  } /* end while first time thru */

  /* Sort the TBUF structure on time
   *********************************/
  qsort( tbuf, nbuf, sizeof(TBUF), compare_time );
  
  /* Write chronological multiplexed output file 
   *********************************************/
  for( ib=0; ib<nbuf; ib++ )
  {
     if(!(ib%100))
       printf(".");
     fseek( ifp, tbuf[ib].offset, SEEK_SET );
     if( fread( pkt.msg, sizeof(char), tbuf[ib].size, ifp ) 
                != tbuf[ib].size )
     {
        logit("et", "remux_tracebufs(): error reading input file at byte %d\n",
                tbuf[ib].offset );
        goto done;
     }      
     if( fwrite( pkt.msg, sizeof(char), tbuf[ib].size, ofp )  
                != tbuf[ib].size )
     {
       logit("et", "remux_tracebufs(): error writing %d bytes to output file\n",
               tbuf[ib].size );
       goto done;
     }      
  }

  logit("et", "remux_tracebufs(): finished writing file %s\n", outtank );

done:
  fclose( ifp );
  fclose( ofp );
  free( tbuf );
  return( 0 );
}

/*************************************************************
 *  compare_time():  This function is passed to qsort()      *
 *************************************************************/
int compare_time( const void *s1, const void *s2 )
{
   TBUF *t1 = (TBUF *) s1;
   TBUF *t2 = (TBUF *) s2;

   if( t1->time > t2->time ) return( 1 );
   if( t1->time < t2->time ) return( -1 );
   return( 0 );
}
