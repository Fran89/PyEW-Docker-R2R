
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: updtsta.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2006/10/20 15:44:37  paulf
 *     udpated with changes from Utah, STAtime now configurable
 *
 *     Revision 1.3  2004/05/05 23:54:03  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.2  2001/01/30 02:36:17  lombard
 *     Fixed calculation of newCalcSamps
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * updtsta.c: Update a station's STA/LTA and trigger information
 *              1) Check the trigger status of the station through the
 *                   station's current end time.
 *
 */
/* Change 2/19/99: Make carlstatrig less fussy about packet times       */
/*        Count buffer samples directly instead of by times.            */
/* Change 1/10/00: Made LTAtime an adjustable parameter instead of      */
/*        being hardwired to 8 seconds.                                 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: UpdateStation                                         */
/*                                                                      */
/*      Inputs:         Pointer to a CarlStaTrig station structure      */
/*                      Pointer to a CarlStaTrig network structure      */
/*                                                                      */
/*      Outputs:        Updated station structure                       */
/*                      Updated subnet structures(via the station)      */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure: unlikely logic error or error from     */
/*                      ProduceStationTrigger()                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <stdlib.h>     /* free, malloc                                 */
#include <math.h>       /* fabs                                         */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: UpdateStation                                         */
int UpdateStation( STATION* station, WORLD* cstWorld )
{
  char*         bufPtr;         /* Pointer to a trace data point in the */
                                /*    buffer.                           */
  short*        shortPtr;
  int32_t*      longPtr;
  float*        floatPtr;
  double*       doublePtr;
  
  double        sumSamps;       /* Sum of the samples in an STA period. */
  double        sumSampsR;      /* Sum of the samples in an STA period  */
  /*                                 (rectified).                       */
  double        eta;            /* Trigger value at calculation time.   */
  int           retVal = 0;     /* Return value for this function.      */
  long          addends;        /* Number of samples used in the STA.   */
  long          samp;           /* Loop counter.                        */
  long          dcm;            /* Trace data decimation factor.        */
  long          newCalcSecs;
  long          newCalcSamps;
  double        LTAtime, ltaM1;
  long          STAtime;
  
  dcm = cstWorld->decimation;
  LTAtime = (double) cstWorld->LTAtime;
  ltaM1 = LTAtime - 1.0;
  STAtime = cstWorld->STAtime;
  
  while ( ( newCalcSecs = station->calcSecs + STAtime ) <= 
          (long) station->buffRefTime )
  {
    newCalcSamps = station->buffSamps - (long)(station->sampleRate * 
      ( station->buffRefTime - (double) newCalcSecs ));

    if ( cstWorld->cstParam.debug > 4 )
      logit("t", "UpdateStation: %d samples (time %15.2lf), %d new samples (%d)\n",
            station->buffSamps, station->buffRefTime, newCalcSamps, 
            newCalcSecs );
    
    bufPtr = station->traceBuf + station->calcSamps * station->dataSize;
    
    /* Short-Term straight average */
    sumSamps = 0.0;
    switch ( station->dataType )
    {
    case CT_SHORT:
      shortPtr = ( short *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, shortPtr += dcm )
        sumSamps += *shortPtr;
      break;
    case CT_LONG:
      longPtr = ( int32_t *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, longPtr += dcm )
        sumSamps += *longPtr;
      break;
    case CT_FLOAT:
      floatPtr = ( float *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, floatPtr += dcm )
        sumSamps += *floatPtr;
      break;
    case CT_DOUBLE:
      doublePtr = ( double *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, doublePtr += dcm )
        sumSamps += *doublePtr;
      break;
    default:
      logit( "et", "carlStaTrig: Unknown data type in UpdateStation.\n" );
      /*    Quit now so only one message is printed                 */
      return( ERR_UNKNOWN );
    }
    if (addends == 0)	/* prevent divide by zero */
      return(retVal);
    /*        Determine the short-term average for this delta-t       */
    station->holdSTA = sumSamps / addends;
    
    /*        Calculate the rectified averages over the same period   */
    sumSampsR = 0.0;
    switch ( station->dataType )
    {
    case CT_SHORT:
      shortPtr = ( short *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, shortPtr += dcm )
        sumSampsR += fabs( *shortPtr - station->holdLTA );
      break;
    case CT_LONG:
      longPtr = ( int32_t *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, longPtr += dcm )
        sumSampsR += fabs( *longPtr - station->holdLTA );
      break;
    case CT_FLOAT:
      floatPtr = ( float *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, floatPtr += dcm )
        sumSampsR += fabs( *floatPtr - station->holdLTA );
      break;
    case CT_DOUBLE:
      doublePtr = ( double *) bufPtr;
      for ( samp = station->calcSamps, addends = 0; samp < newCalcSamps;
            samp += dcm, addends++, doublePtr += dcm )
        sumSampsR += fabs( *doublePtr - station->holdLTA );
      break;
    default:
      logit( "et", "carlStaTrig: Unknown data type in UpdateStation.\n" );
      /*      Quit now so only one message is printed                 */
      return( ERR_UNKNOWN );
    }

    if (addends == 0)	/* prevent divide by zero */
      return(retVal);
    /*        Determine the short-term rectified average for this     */
    /*          delta-t                                               */
    station->holdSTAR = sumSampsR / addends;

    /*  Do a trigger status chech based on the new STAs and old LTAs   */
    eta = station->holdSTAR -
      ( cstWorld->Ratio * station->holdLTAR ) - 
      fabs( station->holdSTA - station->holdLTA ) - cstWorld->Quiet;
    
    /*        Determine the long-term straight average for this       */
    /*           delta-t                                              */
    station->holdLTA = ( ltaM1 * station->holdLTA + station->holdSTA ) 
      / LTAtime;
    
    /*        Determine the long-term rectified average for this      */
    /*          delta-t                                               */
    station->holdLTAR = ( ltaM1 * station->holdLTAR + station->holdSTAR ) 
      / LTAtime;
    
    /*        Update the station parameters                           */
    station->numSSR += newCalcSamps - station->calcSamps;
    station->calcSamps = newCalcSamps;
    station->calcSecs = newCalcSecs;
    
    if ( cstWorld->cstParam.debug > 4 )
    {
      logit( "t", "Station: (%s.%s.%s.%s)\n", station->staCode,
             station->compCode, station->netCode, station->locCode );
      logit( "", "\tSample Sum: %lf  Num: %d\n", sumSamps, addends );
      logit( "", "\tSTA: %lf\n", station->holdSTA );
      logit( "", "\tLTA: %lf\n", station->holdLTA );
      logit( "", "\tSample SumR: %lf\n", sumSampsR );
      logit( "", "\tSTAR: %lf\n", station->holdSTAR );
      logit( "", "\tLTAR: %lf\n", station->holdLTAR );
      logit( "", "\tCalcSecs: %ld\n", station->calcSecs );
      logit( "", "\tCalcSamps: %ld\n", station->calcSamps );
      logit( "", "\tSSR: %ld\n", station->numSSR );
    }
    
    /*        If we have enough data, check for a trigger             */
    if ( station->numSSR > station->startupSamps )
    {
      if ( cstWorld->cstParam.debug > 2 )
      {
        logit( "t", "carlStaTrig: Station: (%s.%s.%s.%s)\n", station->staCode,
               station->compCode, station->netCode, station->locCode );
        logit( "", "\tAs of %d, trigger value is %lf\n", station->calcSecs,
               eta );
      }
      
      /*      If the trigger status changed, produce a station        */
      /*         trigger message                                      */
      /*      Trigger status logged in ProduceStationTrigger()        */
      if ( eta > 0.0  && station->trigger == TRIG_OFF )
      {   /* a new trigger  */
        station->trigger = TRIG_ON;
        station->trigOnTime = (double) station->calcSecs;
        station->trigOffTime = 0.0;
        station->trigEta = eta;
        station->trigCount++;
        retVal = ProduceStationTrigger( station, cstWorld );
      }
      else if ( eta <= 0.0 && station->trigger == TRIG_ON )
      {  /* this trigger just ended */
        station->trigger = TRIG_OFF;
        station->trigOffTime = (double) station->calcSecs;
        retVal = ProduceStationTrigger( station, cstWorld );
      }
    }
  }

  return ( retVal );
}
