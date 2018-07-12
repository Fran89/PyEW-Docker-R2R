
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: protrace.c 6229 2015-01-23 16:45:37Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.2  2001/01/17 18:35:36  dietz
 *     Changed ProcessTraceMsg to return 0 when WaveMsgMakeLocal fails so that
 *     carlstatrig will not exit.  It will just skip that msg and continue.
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * protrace.c: Process a trace data message that came in from the input ring.
 *              1) Read the header information in the message.
 *              2) Append the trace data to the appropriate station.
 *              3) Update the subnet's that contain the station.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ProcessTraceMsg                                       */
/*                                                                      */
/*      Inputs:         Pointer to a World information structure        */
/*                      Pointer to a message buffer                     */
/*                                                                      */
/*      Outputs:        Updated station triggers                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure; this will cause carlstatrig to quit!   */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* strcmp                                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>
#include <swap.h>       /* WaveMsgMakeLocal                             */
#include <trace_buf.h>  /* TRACE_HEADER, TracePacket                    */
#include <transport.h>  /* SHM_INFO                                     */

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ProcessTraceMsg                                       */
int ProcessTraceMsg( WORLD* cstWorld, char* msg )
{
  int           retVal = 0;     /* Return value for this function.      */
  STATION*      station;        /* Pointer to a station.                */
  TracePacket*  packet;         /* Incoming trace data packet.          */

  /*    Convert the message to the local byte-ordering system           */
  packet = (TracePacket*) msg;
  if ( WaveMsg2MakeLocal( (TRACE2_HEADER *) packet ) < 0 )
  {
    logit( "et", "carlstatrig: Error in WaveMsgMakeLocal for data packet.\n" );
 /* Our caller will exit if we return an error, so make sure it's valid */
    return ( retVal );
  }
  else
  {
    if ( cstWorld->cstParam.debug > 4 )
    {
      logit( "t", "carlstatrig: Processing trace data packet.\n" );
      logit( "", "\tHeader Info:\n" );
      logit( "", "\t\tPin Number: %d\n", packet->trh2.pinno );
      logit( "", "\t\tNumber of Samples: %d\n", packet->trh2.nsamp );
      logit( "", "\t\tStart Time: %lf\n", packet->trh2.starttime );
      logit( "", "\t\tEnd Time: %lf\n", packet->trh2.endtime );
      logit( "", "\t\tSample Rate: %lf\n", packet->trh2.samprate );
      logit( "", "\t\tStation Code: %s\n", packet->trh2.sta );
      logit( "", "\t\tNetwork Code: %s\n", packet->trh2.net );
      logit( "", "\t\tComponent Code: %s\n", packet->trh2.chan );
      logit( "", "\t\tLocation Code: %s\n", packet->trh2.loc );
      logit( "", "\t\tData Format: %s\n", packet->trh2.datatype );
      switch (packet->trh2.version[1]) {
      case TRACE2_VERSION1:   /* version 20 */
         logit( "", "\t\tData Quality: %c %c\n", packet->trh2.quality[0], packet->trh2.quality[1] );
         break;
      case TRACE2_VERSION11:  /* version 21 */
         logit( "", "\t\tConversion Factor: %f\n", packet->trh2x.x.v21.conversion_factor );
         break;
      }
    }

    /*  Locate the appropriate station in the master list               */
    station = FindStation( packet->trh2.sta, packet->trh2.chan, 
			   packet->trh2.net, packet->trh2.loc, cstWorld );
    if ( station )
    {
      /*        Found station, append the new trace data                */
      retVal = AppendTraceData( packet, station, cstWorld );
    }
    else
    {
      if ( cstWorld->cstParam.debug > 3 )
        logit( "et", "carlstatrig: Unable to find station (%s.%s.%s.%s) for trace data "
               "append.\n", packet->trh2.sta, packet->trh2.chan, 
	       packet->trh2.net, packet->trh2.loc );
    }
  }
  /* Our caller will exit if we return an error, so make sure it's valid */
  return ( retVal );
}
