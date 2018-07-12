
   /********************************************************************
    *                           remux_tbuf.c                           *
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



#ifdef _WINNT
/* this needs to get moved to a lib eventually if others will do this trick */
int setenv(const char * name, const char * value, int i) {
    return SetEnvironmentVariable(name, value) ? 0 : -1;
}
#endif

/* versioning added in the second incarnation which allows remux_tbuf to run without any EW_LOG var set */
#define VERSION_NUMBER "0.0.2 2012.12.07"

main( int argc, char *argv[] )
{
  int           rc;

  if ( getenv( "EW_LOG" ) == NULL )
      setenv("EW_LOG", ".", 1);

  logit_init("remux_tbuf", 0, 1024, 1);

  /* Check arguments
  *****************/
  if ( argc != 3 )
  {
    logit("et", "Usage: remux_tbuf <demuxed_file> <outputfile>\n" );
    logit("et", "Version %s\n", VERSION_NUMBER);
    return( 0 );
  }

  rc = remux_tracebufs(argv[1], argv[2], 0.0, 0.0);

  return( rc );
}
