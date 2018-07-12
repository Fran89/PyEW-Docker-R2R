/***************************************************************************
 * General utility routines
 *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libmseed.h>
#ifdef _WIN32
#include <libdali.h>
#endif

#include "util.h"


/***************************************************************************
 * safe_usleep():
 *
 * Sleep for a given number of microseconds using nanosleep which will not
 * affect threads/signals.
 ***************************************************************************/
extern void
safe_usleep (unsigned long int useconds)
{
#ifndef _WIN32
  struct timespec treq, trem;
  
  treq.tv_sec = (time_t) (useconds / 1e6);
  treq.tv_nsec = (long) ((useconds * 1e3) - (treq.tv_sec * 1e9));
  
  nanosleep (&treq, &trem);
#else                        /* if Windows build then        */
  dlp_usleep (useconds);     /* use portable fn from libdali */
#endif
}  /* End of safe_usleep() */


/***************************************************************************
 * Routines to byte swap TRACE header structures into the local byte order.
 *
 ***************************************************************************/

int WaveMsgMakeLocal (TRACE_HEADER *wvmsg)
{
  return WaveMsgVersionMakeLocal((TRACE2X_HEADER *)wvmsg, '1');
}
int WaveMsg2MakeLocal (TRACE2_HEADER *wvmsg)
{
  return WaveMsgVersionMakeLocal((TRACE2X_HEADER *)wvmsg, wvmsg->version[0]);
}
int WaveMsg2XMakeLocal (TRACE2X_HEADER *wvmsg)
{
  return WaveMsgVersionMakeLocal( wvmsg, wvmsg->version[0]);
}

/************************ WaveMsgMakeLocal **************************
*       Byte-swap a universal TYPE_TRACEBUF2X message in place.     *
*       Changes the 'datatype' field in the message header          *
*       Returns -1 if unknown data type.                            *
*       Returns -1 if more than max number of samples for tbuf      * 
*        size allowed  2000 or 1000 depending on data type          *
*       Returns -2 if checksumish calculation of header fails.      *
*       Elsewise (SUCCESS) returns 0.                               *
*********************************************************************/

int WaveMsgVersionMakeLocal (TRACE2X_HEADER *wvmsg, char version)
{
  int dataSize;  /* flag telling us how many bytes in the data */
  char  byteOrder;
  char  hostOrder;
  int*  intPtr;
  short* shortPtr;
  int i;
  int nsamp;
  double samprate,starttime,endtime;
  double tShouldEnd; 
  double dFudgeFactor;
  
  /* Check local machine order with htons() */
  if ( htons(0x1234) == 0x1234 )
    hostOrder = 's';
  else
    hostOrder = 'i';
  
  /* See what sort of data it carries */
  dataSize=0;
  if ( strcmp(wvmsg->datatype, "s4") == 0 )
    {
      dataSize=4; byteOrder='s';
    }
  else if ( strcmp(wvmsg->datatype, "i4") == 0 )
    {
      dataSize=4; byteOrder='i';
    }
  else if ( strcmp(wvmsg->datatype, "s2") == 0 )
    {
      dataSize=2; byteOrder='s';
    }
  else if ( strcmp(wvmsg->datatype, "i2") == 0 )
    {
      dataSize=2; byteOrder='i';
    }
  else
    return(-1); /* We don't know this message type*/
  
  /* Swap the header (if neccessary) */
  if ( byteOrder != hostOrder )
    {
      /* swap the header */
      ms_gswap4 ( &(wvmsg->pinno) );
      ms_gswap4 ( &(wvmsg->nsamp) );
      ms_gswap8 ( &(wvmsg->starttime) );
      ms_gswap8 ( &(wvmsg->endtime) );
      ms_gswap8 ( &(wvmsg->samprate) );
      
      if ( version == TRACE2_VERSION0 )
	{
	  switch ( wvmsg->version[1] )
	    {
	    case TRACE2_VERSION11:
	      ms_gswap4 ( &(wvmsg->x.v21.conversion_factor) );
	      break;
	    }
	}
    }
  
  if ( wvmsg->nsamp > 4032/dataSize )
    {
      ms_log (2, "WaveMsg2MakeLocal: packet from %s.%s.%s.%s has bad number of samples=%d datatype=%s\n",
	      wvmsg->sta, wvmsg->chan, wvmsg->net, wvmsg->loc, wvmsg->nsamp , wvmsg->datatype);
      return -1;
    }
  
  /* perform a CheckSumish kind of calculation on the header 
   * ensure that the tracebuf ends within 5 samples of the given endtime.
   * DK 2002/03/18
   *******************************************************************/
  
  /* moved nsamp memcpy to here to avoid byte-alignment with next statement */
  memcpy( &nsamp,     &wvmsg->nsamp,     sizeof(int) );
  memcpy( &samprate,  &wvmsg->samprate,  sizeof(double) );
  memcpy( &starttime, &wvmsg->starttime, sizeof(double) );
  memcpy( &endtime,   &wvmsg->endtime,   sizeof(double) );
  
  tShouldEnd = starttime + ((nsamp - 1) / samprate);
  dFudgeFactor = 5.0 / samprate;
  
  /* This is supposed to be a simple sanity check to ensure that the
   * endtime is within 5 samples of where it should be.  We're not
   * trying to be judgemental here, we're just trying to ensure that
   * we protect ourselves from complete garbage, so that we don't segfault
   * when allocating samples based on a bad nsamp
   ***********************************************************************/
  if ( endtime < (tShouldEnd-dFudgeFactor) ||  
       endtime > (tShouldEnd+dFudgeFactor) )
    {
      ms_log (1, "WaveMsgVersionMakeLocal: packet from %s.%s.%s.%s has inconsistent header values!\n",
	      wvmsg->sta, wvmsg->chan, wvmsg->net, wvmsg->loc );
      ms_log (1, "WaveMsgVersionMakeLocal: header.starttime  : %.4lf\n", starttime  );
      ms_log (1, "WaveMsgVersionMakeLocal: header.samplerate : %.1lf\n", samprate   );
      ms_log (1, "WaveMsgVersionMakeLocal: header.nsample    : %d\n",    nsamp      );
      ms_log (1, "WaveMsgVersionMakeLocal: header.endtime    : %.4lf\n", endtime    );
      ms_log (1, "WaveMsgVersionMakeLocal: computed.endtime  : %.4lf\n", tShouldEnd );
      ms_log (1, "WaveMsgVersionMakeLocal: header.endtime is not within 5 sample intervals "
	      "of computed.endtime!\n" );
      
      return -2;
    }
  
  /* Swap the data (if neccessary) */
  if ( byteOrder != hostOrder )
    {
      intPtr = (int*) ((char*)wvmsg + sizeof(TRACE2_HEADER) );
      shortPtr = (short*) ((char*)wvmsg + sizeof(TRACE2_HEADER) );
      
      for ( i=0; i < nsamp; i++ )
	{
	  if ( dataSize == 2 ) ms_gswap2 ( &shortPtr[i] );
	  if ( dataSize == 4 ) ms_gswap4 ( &intPtr[i] );
	}
      
      /* Re-write the data type field in the message */
      if ( dataSize == 2 ) sprintf (wvmsg->datatype, "%c2", hostOrder);
      if ( dataSize == 4 ) sprintf (wvmsg->datatype, "%c4", hostOrder);
    }
  
  return 0;
}
