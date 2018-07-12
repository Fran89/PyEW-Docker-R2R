/* $Id: flushbuf.c 1449 2004-05-05 23:54:04Z lombard $ */
/*
 * flushbuf.c: Flush a station's trace data buffer.
 *              1) Empty some of the trace data from the buffer.
 */
/* Change: 2/19/99 count samples directly instead of based on times.  */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FlushBuffer                                           */
/*                                                                      */
/*      Inputs:         Pointer to a CarlStaTrig station structure      */
/*                      Flag for debugging                              */
/*                                                                      */
/*      Outputs:        Updated station structure(above)                */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure when station buffer not updated         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* memmove                                      */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: FlushBuffer                                           */
int FlushBuffer( STATION* station, int debug )
{
  char* bufPtr;         /* Pointer to the beginning of data to keep.    */
  int   retVal = 0;     /* Return value for this function.              */
  long  bytesToFlush;   /* Number of data points to remove.             */
  long  bytesToKeep;    /* Number of data points to keep.               */

  /*    Determine if the buffer has data to flush                       */
  if ( station->calcSamps > 0 )
  {
    /*  Calculate the amount of data to flush                           */
    bytesToFlush = station->calcSamps * station->dataSize;
    bytesToKeep = station->buffSamps * station->dataSize - bytesToFlush;
    
    if ( debug > 4 )
      logit("t", "flushing %d bytes from %s.%s.%s.%s\n", bytesToFlush,
            station->staCode, station->compCode, station->netCode,
	    station->locCode );
    
    /*  Determine the beginning position in the buffer for data to keep */
    bufPtr = station->traceBuf + bytesToFlush;
    
    /*  Flush the buffer                                                */
    memmove( station->traceBuf, bufPtr, bytesToKeep );

    /*  Update the buffer variables                                     */
    station->buffSamps -= station->calcSamps;
    station->calcSamps = 0;
  }
  else
  {
    /*  Calculations not up to date                                     */
    logit( "et", "carlStaTrig: Unable to flush station <%s.%s.%s.%s> trace buffer.\n\tReason: "
        "Trigger calculations not yet performed on existing data.\n",
           station->staCode, station->compCode, station->netCode,
	   station->locCode );
    retVal = ERR_PROC_MSG; /* return the error, let our caller deal with it */
  }

  return ( retVal );
}
