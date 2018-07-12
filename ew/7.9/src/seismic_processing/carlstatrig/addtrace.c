
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: addtrace.c 4128 2011-01-04 18:24:04Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2005/04/12 22:43:07  dietz
 *     Added optional command "GetWavesFrom <instid> <module_id>"
 *
 *     Revision 1.5  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.4  2001/01/30 02:36:17  lombard
 *     Fixed initialization of calcSecs and calcSamps in CopyToNew.
 *
 *     Revision 1.3  2001/01/17 18:35:14  dietz
 *     *** empty log message ***
 *
 *     Revision 1.2  2000/08/22 00:17:46  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/* $Id: addtrace.c 4128 2011-01-04 18:24:04Z kevin $ */
/*
 * addtrace.c: Append trace data to a station.
 *              1) Validate input trace data.
 *              2) Check for gaps and act accordingly based on gap size(s).
 *              3) Update the station's averages and critical times.
 */
/* Change: 11/25/98 to fix crashes on data rollback: PNL                */
/* Change: 2/4/99 Significant rewrite to fix many problems, eliminate   */
/*         recursion, handle errors more logically: PNL                 */
/* Change: 2/18/99 Make carlstatrig less fussy about packet times       */
/*         count buffer samples directly instead of by times            */
/*         delete InterpolateGap(): PNL                                 */
/* Change: 4/10/99 Fixed bug in gapSize calcluation that was causing    */
/*         LTA values to get filled with NaNs. PNL                      */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: AppendTraceData                                       */
/*                                                                      */
/*      Inputs:         Pointer to a trace packet structure             */
/*                      Pointer to a station structure                  */
/*                      Pointer to a network structure                  */
/*                                                                      */
/*      Outputs:        Updated station structure(above)                */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure;  this will cause carlstatrig to exit!  */
/*                                                                      */
/*      Function: CopyToNew: copies data into a new station buffer      */
/*      Function: CopyToOld: copies data into an old station buffer     */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <stdlib.h>     /* atoi, abs                                    */
#include <string.h>     /* memcpy                                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Internal Function prototypes                                    */
/*******                                                        *********/
int CopyToNew( TracePacket* data, int dataSize, DATATYPE dataType, 
                STATION* station, WORLD* cstWorld );
void CopyToOld( TracePacket* data, int dataSize, DATATYPE dataType, 
                STATION* station, WORLD* cstWorld );

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: AppendTraceData                                       */
int AppendTraceData( TracePacket* data, STATION* station, WORLD* cstWorld )
{
  DATATYPE      dataType;       /* Byte size of one data value.         */
  int           dataSize;       /* Byte size of one data value.         */
  int           retVal = 0;     /* Return value for this function.      */
  double        startLate;      /* Packet lateness in seconds           */
  long          gapSize;        /* Packet lateness in samples           */
  

  /* Make sure we have the expected version of TRACE2_HEADER */
  if (!TRACE2_HEADER_VERSION_IS_VALID(&data->trh2)) {
    logit( "et", "carlStaTrig: Unknown trace version <%c%c> in AppendTraceData.\n",
           data->trh2.version[0], data->trh2.version[1] );
    /*  Unknown version, quit now before major errors happen            */
    return( ERR_UNKNOWN );
  }
      
  

  /*    Determine the size and type of each data value                  */
  dataSize = atoi( &(data->trh2.datatype[1]) );
  switch ( dataSize )
  {
  case 2:
    dataType = CT_SHORT;
    break;
  case 4:
#ifdef _SPARC
    if ( 's' == data->trh2.datatype[0] )
#endif
#ifdef _INTEL
    if ( 'i' == data->trh2.datatype[0] )
#endif
      dataType = CT_LONG;
    else
      dataType = CT_FLOAT;
    break;
  case 8:
    dataType = CT_DOUBLE;
    break;
  default:
    logit( "et", "carlStaTrig: Unknown data size of %d in AppendTraceData.\n",
           dataSize );
    /*  Unknown size, quit now before major errors happen               */
    return( ERR_UNKNOWN );
  }

  /*    Check new data size                                             */
  if ( data->trh2.nsamp * dataSize > BUFFSIZE )
  {/* new data won't fit in buffer                                      */
    logit( "et",
           "carlStaTrig: <%s.%s.%s.%s> packet too large: %d  - skipping packet.\n",
           station->staCode, station->compCode, station->netCode, 
	   station->locCode, data->trh2.nsamp * dataSize );
    return 0;  /* so carlstatrig doesn't shut down                      */
  }
    
  /*  Check for existing data in the buffer                             */
  if ( station->buffSamps > 0 )
  {
    /*  Check for disqualifying conditions: changed data types          */
    if ( dataType != station->dataType )
    { /* data types don't match */
      logit( "t", "carlStaTrig: Change in data type to %s for station "
             "(%s.%s.%s.%s).\n", data->trh2.datatype, station->staCode,
               station->compCode, station->netCode, station->locCode );
      
      ResetStation( station );
      retVal = CopyToNew( data, dataSize, dataType, station, cstWorld );
      if ( CT_FAILED( retVal ) )
      {
        logit("et", "zero samplerate found for <%s.%s.%s.%s>, skipping\n",
              station->staCode, station->compCode, station->netCode,
	      station->locCode);
        return ( 0 );  /* non-zero would kill us */
      }
      retVal = UpdateStation( station, cstWorld );
      return ( retVal );
    }
    
    /* ... changed sampling rate                                        */
    if ( data->trh2.samprate != station->sampleRate )
    {  /* sample rates don't match */
      logit( "t", "carlStaTrig: Change in data sampling rate from %f to %f "
             "for station (%s.%s.%s.%s).\n", station->sampleRate,
             data->trh2.samprate, station->staCode, station->compCode,
             station->netCode, station->locCode );
      
      ResetStation( station );
      retVal = CopyToNew( data, dataSize, dataType, station, cstWorld );
      if ( CT_FAILED( retVal ) )
      {
        logit("et", "zero samplerate found for <%s.%s.%s.%s>, skipping\n",
              station->staCode, station->compCode, station->netCode,
	      station->locCode);
        return ( 0 );  /* non-zero would kill us */
      }
      retVal = UpdateStation( station, cstWorld );
      return ( retVal );
    }
    /* So far so good...                                                 */

    /* Calculate the gap (difference between the expected and actual     */
    /* new data start time; ideally this is zero                         */
    startLate = data->trh2.starttime - station->buffRefTime;
    gapSize = (long) (startLate * station->sampleRate);

    /* Check gap threshold                                               */
    if ( abs(gapSize) > cstWorld->maxGap )
    {
      if ( gapSize > cstWorld->maxGap )
      {
        if ( cstWorld->cstParam.debug )
        {
          logit( "t", "carlStaTrig: Station (%s.%s.%s.%s)" 
                 " reset due to prolonged gap: %d samples.\n",
                 station->staCode, station->compCode, 
                 station->netCode, station->locCode, gapSize );
        }
        /* Reset the station                                        */
        ResetStation( station );
        retVal = CopyToNew( data, dataSize, dataType, station, cstWorld );
        if ( CT_FAILED( retVal ) )
        {
          logit("et", "zero samplerate found for <%s.%s.%s>, skipping\n",
                station->staCode, station->compCode, station->netCode);
          return ( 0 );  /* non-zero would kill us */
        }
        retVal = UpdateStation( station, cstWorld );
        return ( retVal );
      }
      else  /* gapSize is < -maxGap */
      { /* Data overlaps; skip it                                  */
        logit( "t", "data overlap %d for %s.%s.%s.%s; skipping.\n",
                 -gapSize, station->staCode, station->compCode, 
                 station->netCode, station->locCode ); 
        return ( retVal );
      }
    }
    /* gap resolved, now insert new trace data                   */

    /* Check for buffer overflow                                 */
    if ( ( data->trh2.nsamp + station->buffSamps ) * dataSize >= BUFFSIZE )
    {
      /* Flush out the buffer                 */
      retVal = FlushBuffer( station, cstWorld->cstParam.debug );
      if ( CT_FAILED( retVal ) )
      { /* Buffer didn't flush; lets hope this is a onetime thing and reset it */
        ResetStation( station );
        retVal = CopyToNew( data, dataSize, dataType, station, cstWorld );
        if ( CT_FAILED( retVal ) )
        {
          logit("et", "zero samplerate found for <%s.%s.%s.%s>, skipping\n",
                station->staCode, station->compCode, station->netCode,
		station->locCode);
          return ( 0 );  /* non-zero would kill us */
        }
        retVal = UpdateStation( station, cstWorld );
        return ( retVal );
      }
    }
    CopyToOld( data, dataSize, dataType, station, cstWorld );
    retVal = UpdateStation( station, cstWorld );
    return ( retVal );
  }
  else  /* no existing data in station trace buffer */
  {
    retVal = CopyToNew( data, dataSize, dataType, station, cstWorld );
    if ( CT_FAILED( retVal ) )
    {
      logit("et", "zero samplerate found for <%s.%s.%s.%s>, skipping\n",
            station->staCode, station->compCode, station->netCode,
	    station->locCode);
      return ( 0 );  /* non-zero would kill us */
    }
    retVal = UpdateStation( station, cstWorld );
    return ( retVal );
  }
}

/* CopyToNew: Copies trace data into an empty station structure    */
/*       returns 0 on success                                      */
/*               ERR_PROC_MSG if samplerate is 0                   */
int CopyToNew( TracePacket* data, int dataSize, DATATYPE dataType, 
                STATION* station, WORLD* cstWorld )
{
  char* traceData;
  
  if ( data->trh2.samprate < DOUBLE_EQUAL )
    return ( ERR_PROC_MSG );  /* avoid div by 0 in many places */
  
  traceData = (char*) data + sizeof( TRACE2_HEADER );
  memcpy( station->traceBuf, traceData, data->trh2.nsamp * dataSize );
  
  /* Set the buffer information parameters                        */
  station->dataType = dataType;
  station->sampleRate = data->trh2.samprate;
  station->dataSize = dataSize;
  station->buffSamps = data->trh2.nsamp;
  station->buffRefTime = data->trh2.starttime + (double) data->trh2.nsamp /
    data->trh2.samprate;
/* Round up to the next whole second so we will be pointing at some data */
  station->calcSecs = (long) (data->trh2.starttime + 0.999);
/* calcSamps points to the sample that occured at calcSecs */
  station->calcSamps = (long)( (station->calcSecs - data->trh2.starttime) *
     station->sampleRate);
  station->startupSamps = (long) ( station->sampleRate * 
                                   cstWorld->startUp + 0.5 );
  
  if ( cstWorld->cstParam.debug > 4 )
    logit( "t", "Received first set of data for station "
           "(%s.%s.%s.%s)  buf start: %d end %15.2lf\n", station->staCode, 
           station->compCode, station->netCode, station->locCode,
	   station->calcSecs, station->buffRefTime );
  
  return ( 0 );
}


void CopyToOld( TracePacket* data, int dataSize, DATATYPE dataType, 
                STATION* station, WORLD* cstWorld )
{
  char* bufferEnd;
  char* traceData;
  
  bufferEnd = station->traceBuf + ( station->buffSamps * dataSize );
  traceData = (char*) data + sizeof( TRACE2_HEADER );
  memcpy( bufferEnd, traceData, data->trh2.nsamp * dataSize );
  
  /* Set the buffer information parameters      */
  station->buffRefTime =  data->trh2.starttime + (double) data->trh2.nsamp / 
    station->sampleRate;
  station->buffSamps += data->trh2.nsamp;
  
  if ( cstWorld->cstParam.debug > 4 )
    logit( "t", "Received another set of data for station "
           "(%s.%s.%s.%s)  tbuf start: %15.2lf end %15.2lf\n", 
	   station->staCode, station->compCode, station->netCode, 
	   station->locCode, data->trh2.starttime, data->trh2.endtime  );
  return;
}
