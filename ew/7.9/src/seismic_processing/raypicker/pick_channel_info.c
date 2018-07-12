/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_channel_info.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log: pick_channel_info.c,v $
 *     Revision 1.2  2009/02/13 20:40:36  mark
 *     Added SCNL parsing library
 *
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.2  2005/12/29 23:01:51  cjbryan
 *     revised bit flags and deleted PICKER_OFF
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.18  2005/06/20 21:34:24  cjbryan
 *     error cleanup and revise to use new library routines
 *
 *     Revision 1.17  2005/02/03 21:04:45  davidk
 *     Updated to SCNL.
 *
 *     Revision 1.16  2004/11/01 02:10:31  labcvs
 *     removed unused variables
 *
 *     Revision 1.15  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *
 *     Revision 1.14  2004/10/28 20:33:56  labcvs
 *     test all channel flags before turning them on or off while looking for triggers
 *
 *     Revision 1.13  2004/10/27 14:33:44  cjbryan
 *     revisions to period determination for amp calculations and collection of
 *     amp info between calculated arrival time and time trigger is declared
 *
 *     Revision 1.12  2004/09/15 16:08:01  cjbryan
 *     *** empty log message ***
 *
 *     Revision 1.10  2004/07/21 22:02:34  labcvs
 *     added a tolerance to channel samping rate check in SetChannelSampleRate
 *
 *     Revision 1.9  2004/07/21 18:00:04  cjbryan
 *     added max lifespan of a preliminary trigger turning on and off without confirmed trigger
 *
 *     Revision 1.8  2004/07/20 20:08:50  cjbryan
 *
 *     allowed for continuation of a preliminary trigger`
 *
 *     Revision 1.6  2004/07/16 19:27:22  cjbryan
 *     allowed continuation of a preliminary trigger
 *
 *     Revision 1.5  2004/07/13 19:22:08  cjbryan
 *     added code for debugging
 *
 *     Revision 1.4  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.3  2004/04/23 17:29:11  cjbryan
 *     changed bool to int
 *
 *     Revision 1.2  2004/04/22 21:11:56  patton
 *     Quick fixes to make raypicker compile.
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Functions used to implement a single sample channel for the raypicker.
 * 
 * see notes in pick_channel_info.h
 * 
 * @author Ray Buland, FORTRAN, circa 1979
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh C++ port from the original FORTRAN
 */

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* earthworm/hdyra includes */
#include <earthworm.h>
#include <global_msg.h>              /* TimeToDTString */
#include <rint_ew.h>                 /* nearest integer */
#include <watchdog_client.h>


/* raypicker includes */
#include "raypicker.h"
#include "pick_channel_info.h"
#include "channel_states.h"
#include "debug.h"
#include "macros.h"           /* MIN() */
#include "rp_messaging.h"     /* ReportPick(), ReportAmps(), SendErrorMessage() */
#include "returncodes.h"

/* Make these variables global static to reduce memory thrashing.
 * However, as a result, these functions are not thread-safe.*/
static RATE_CONSTANTS_DATA  *g_rateConstants;               /* set for channel in ProcessChannelMessage() */    
static double                g_sampleTime;                  /* time for each point in a sample buffer */

/* Global working memory
 * Initialized and Allocated by AllocChannelGlobals()
 * Freed by FreeChannelGlobals() */
static SERIES_DATA          *g_preFiltered = NULL;          /* broadband converted to short-period */
static SYMMETRY_TRACK_TYPE  *g_Symmetry = NULL;             /* Symmetry data [kr3()] */
static SERIES_DATA          *g_Filtered[PICKER_TYPE_COUNT]; /* IIR Filtered series (btn1, btn2) */
static SERIES_DATA          *g_Envelope[PICKER_TYPE_COUNT]; /* Waveform envelope; tries to follow rectified peaks (stn1, stn2) */
          
/*
 * PICKER_PARAM_DATA -- array of parameters for all picker series types.
 * 
 * The parameters MUST be ordered by frequency, highest to lowest.
 * That is, the left-most value in this array must start with the
 * highest number first, followed by the next-highest, etc.
 * 
 * The confirm duration is the number of seconds that the signal
 * must remain above some set level before confirmation as a pick. 
 * This level is calculated from the mid-level triggering value 
 * and a constant factor.  Since the mid-level and confirmation 
 * durations are updated until the trigger is confirmed, the result 
 * is that the confirmation duration tends to increase with the signal 
 * strength -- spikes which are identified as preliminary triggers 
 * will tend not to be sustained long enough to be confimed. 
 * Note, however, that the  calculated duration value is constrained 
 * by minimum and maximum times. 
 * Lowering the confirmation factor or times will tend to make the
 * picker more susceptible to triggering on narrow spikes.
 * 
 * A fundamental triggering value, which is derived from the
 * long-term average and a constant, is used in determining the
 * mid-level (actual) triggering level as well as a maximum constraint
 * on the STA (to prevent divergence from spikes).
 * This fundamental trigger value is calculated as the ratio of the STA to the 
 * LTA multiplied by a sensitivity factor which is defined here.
 * Raising the sensitivity factor will raise the triggering levels --
 * making the picker less sensitive.
 */
static const double PICKER_PARAM_DATA[PICKER_TYPE_COUNT][PICKER_VALUECOUNT] =
{
/*     corner    | confirm   | confirm duration | sensitivity */
/*  frequencies  | factor    | bounds (secs)    | factor      */
/*   high , low  ,           , low ,  high      ,             */
   { 20.2 , 19.2 , 0.3333333 , 4.0 ,  4.0       , 2.5 }
 , { 10.2 ,  9.2 , 0.3333333 , 1.6 ,  4.0       , 2.5 }
};



/************************************************************************
 *                     FreeChannelGlobals()                             *
 *                                                                      *
 * Releases all memory allocated by AllocGlobals().                     *
 *                                                                      *
 ************************************************************************/
void FreeChannelGlobals()
{
    int p;

    if (g_preFiltered != NULL)
    {
      free(g_preFiltered);
      g_preFiltered = NULL;
    }
    if (g_Symmetry != NULL)
    {
      free(g_Symmetry);
      g_Symmetry = NULL;
    }
    for (p = 0; p < PICKER_TYPE_COUNT; p++)
    {
      if (g_Filtered[p] != NULL)
      {
          free(g_Filtered[p]);
          g_Filtered[p] = NULL;
      }
      
      if (g_Envelope[p] != NULL)
      {
          free(g_Envelope[p]);
          g_Envelope[p] = NULL;
      }
    }
}

/*************************************************************************
 *                     AllocChannelGlobals()                             *
 *                                                                       *
 * Initialize and allocate global working buffers.                       *
 *                                                                       *
 * @return EW_SUCCESS                                                    *
 *         EW_WARNING = error calculating buffer size from maxSampleRate *
 *         EW_FAILURE = memory allocation error                          *
 *                                                                       *
 ************************************************************************/
int AllocChannelGlobals(const double maxSampleRate)
{
    int  p;                      /* filter index */
    int  retc = EW_SUCCESS;       /* return code  */
    long size;                   /* min number of samples for pre-filter averaging */
   
    /* Set channel global memory pointers to NULL to prevent memory errors
     * on delete if not allocated properly. */
    g_preFiltered = NULL;
    g_Symmetry    = NULL;
      
    for (p = 0; p < PICKER_TYPE_COUNT; p++)
    {
      g_Filtered[p] = NULL;
      g_Envelope[p] = NULL;
    }

    if ((size = GetWorkBufferAlloc(maxSampleRate)) <= 0)     
    {
      reportError(WD_WARNING_ERROR, NORESULT, "Allocated buffer size is 0!  Params max sample rate %lf \n", maxSampleRate);
      return EW_WARNING;
    }
    else
    {
      if ((g_preFiltered = (SERIES_DATA *)malloc(sizeof(SERIES_DATA) * size)) == NULL)
      {
          reportError(WD_FATAL_ERROR, GENFATERR, "Unable to allocate space for g_preFiltered array. \n");
          retc = EW_FAILURE;
      }
      else
      {
          if ((g_Symmetry = (SYMMETRY_TRACK_TYPE *)malloc(sizeof(SYMMETRY_TRACK_TYPE) * size)) == NULL)
          {
              reportError(WD_FATAL_ERROR, GENFATERR, "Unable to allocate space for g_Symmetry array. \n");
              retc = EW_FAILURE;
          }
          else
          {
              for (p = 0; p < PICKER_TYPE_COUNT; p++)
              {
                  if ((g_Filtered[p] = (SERIES_DATA *)malloc(sizeof(SERIES_DATA) * size)) == NULL)
                  {
                      reportError(WD_FATAL_ERROR, GENFATERR, 
                          "Unable to allocate space for g_Filtered[%d] array. \n", p);
                      retc = EW_FAILURE;
                      break;
                  }
                  
                  if ((g_Envelope[p] = (SERIES_DATA *)malloc(sizeof(SERIES_DATA) * size)) == NULL)
                  {
                      reportError(WD_FATAL_ERROR, GENFATERR, 
                          "Unable to allocate space for g_Envelope[%d] array. \n", p);
                      retc = EW_FAILURE;
                      break;
                  }
              }
         }
      }
    }
      
    /* encountered a problem allocating working buffers,
     * call FreeGlobals() to release anything that was allocated. */
    if (retc == EW_FAILURE)
      FreeChannelGlobals();
   
    return retc;
}

/************************************************************************
 *                        InitChannelInfo()                             *
 *                                                                      *
 * Initializes values and obtains memory needed by the pick filters     *
 * for one PICK_CHANNEL_INFO structure.                                 *
 * This function allocates buffer space for the pick series. It must be *
 * followed by FreeChannelInfo.                                         *
 *                                                                      *
 * @param maxSampleRate is nominal maximum sample values per second     *
 *                                                                      *
 * @return EW_SUCCESS = got a new channel                               *
 *         EW_WARNING = invalid parameter                               *
 *         EW_FAILURE = bad memory allocation                           *
 *                                                                      *
 ************************************************************************/
int InitChannelInfo(PICK_CHANNEL_INFO *channelinfo, 
					const double maxSampleRate)
{
    int p;
    int retc = EW_SUCCESS;


    channelinfo->state = PKCH_STATE_NEW;
    channelinfo->processed_points = 0;
    channelinfo->sampleRate = 0.0;
    channelinfo->rawAlloc     = 0;
    channelinfo->rawLength    = 0;
    channelinfo->rawBuffer    = NULL;
    channelinfo->prefiltmean  = 0.0; 
    channelinfo->rawLastTime  = PICK_CHAN_INVALID_TIME;
   
    if ((channelinfo->rawAlloc = GetWorkBufferAlloc(maxSampleRate)) == 0)
    {
      reportError(WD_WARNING_ERROR, NORESULT, "buffer allocation length is 0! \n");
      return EW_WARNING;
    }
    
    if ((channelinfo->rawBuffer =  
	   (SERIES_DATA *)malloc(channelinfo->rawAlloc * sizeof(SERIES_DATA))) == NULL)
    {
      /* Error allocating averaging buffer */
      reportError(WD_FATAL_ERROR, MEMALLOC, "error allocating channel data \n");
      channelinfo->rawAlloc = 0;
      channelinfo->state = PKCH_STATE_BAD;
      return EW_FAILURE;
    }
   
    channelinfo->isPrelim     = 0;
    channelinfo->isContinuing = 0;
    channelinfo->isTrigger    = 0;
    channelinfo->isConfirmed  = 0;
    channelinfo->isFinished   = 0;
   
    /* Initialize the persistence data for this channel's pick filters */
    for (p = 0; p < PICKER_TYPE_COUNT; p++)
    {
      if ((retc = InitPickSeriesData(maxSampleRate, &channelinfo->pick_series[p])) != EW_SUCCESS)
          break;
    }
   
    return retc;
}

/************************************************************************
 *                        FreeChannelInfo()                             *
 *                                                                      *
 * Frees any memory allocated for a single PICK_CHANNEL_INFO structure. *
 *                                                                      *
 * WARNING: ensure that InitChannelInfo() is called before calling this *
 * to  prevent attempting to deallocate some unknown memory.            *
 *                                                                      *
 * @return EW_SUCCESS (always)                                          *
 *                                                                      *
 ************************************************************************/
int FreeChannelInfo(PICK_CHANNEL_INFO *channelinfo)
{
    int p;

    if (channelinfo->rawBuffer != NULL)
    {
      free(channelinfo->rawBuffer);
      channelinfo->rawBuffer = NULL;
    }
    channelinfo->rawAlloc = 0;
    channelinfo->rawLength = 0;
   
    for (p = 0; p < PICKER_TYPE_COUNT; p++)
      FreePickSeries(&channelinfo->pick_series[p]);

    return EW_SUCCESS;
}

int CopyChannelInfo(PICK_CHANNEL_INFO **hNewInfo, PICK_CHANNEL_INFO *pOldInfo)
{

	*hNewInfo = (PICK_CHANNEL_INFO *)malloc(sizeof(PICK_CHANNEL_INFO));
	if (*hNewInfo == NULL)
	{
        reportError(WD_FATAL_ERROR, MEMALLOC, "raypicker: cannot allocate space for copying PICK_CHANNEL_INFO\n");
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/************************************************************************
 *                     SetChannelSampleRate()                           *
 *                                                                      *
 * Sets or checks the channel sample rate.                              *
 *                                                                      *
 * The first call to SetChannelSampleRate() sets the sample rate to the *
 * supplied value. Subsequent calls just compare the current rate to    *
 * the previous value.                                                  *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_WARNING - sampleRate not an acceptable value              *
 *                                                                      *
 ************************************************************************/
int SetChannelSampleRate(double traceSampleRate, RaypickerSCNL *thisScnl)
{
	PICK_CHANNEL_INFO *channelinfo = thisScnl->pchanInfo;
    double tolerance;
	int    retc = EW_SUCCESS;
    int    p;

    /* bad sampling rate */
    if (traceSampleRate <= 0.0)
    {
      reportError(WD_WARNING_ERROR, NORESULT, 
          "traceSampleRate %lf (<- 0.0) in SetChannelSampleRate. \n", traceSampleRate);
      return EW_WARNING;
    }
   
    /* First time sample rate is supplied */
    if (channelinfo->sampleRate == 0.0)
    {
      /* set sample rate */
      channelinfo->sampleRate = traceSampleRate;

      /* Set the sample rate for this channel's pick filters */
      for (p = 0; p < PICKER_TYPE_COUNT; p++)
      {
          if ((retc = SetPickFilterParams(traceSampleRate, p, 
			      &channelinfo->pick_series[p], PICKER_PARAM_DATA)) != EW_SUCCESS)
              break;
      }
    }
    else
    {
      /* not the first call, parameter rate must match stored rate */
      /* if (channelinfo->sampleRate != traceSampleRate) */
      tolerance = 0.05/channelinfo->sampleRate;
      if (fabs(channelinfo->sampleRate - traceSampleRate) >= tolerance)
      {
		  reportError(WD_WARNING_ERROR, NORESULT, "Sampling rate [%lf] for %s:%s:%s:%s does not match previous [%lf], rejecting\n", 
                            traceSampleRate, thisScnl->sta, thisScnl->chan, 
							thisScnl->net, thisScnl->loc, channelinfo->sampleRate);
          retc = EW_WARNING;
      }
    }
    return retc;
}

/************************************************************************
 *                     InitChannelForSeries()                           *
 *                                                                      *
 * Initializes the channel state when the channel is first encountered  *
 * or after a gap.  Essentially, drops any raw data that has been       *
 * accumulated (for initialization) and prepares to start collecting    *
 * the raw time series anew.                                            *
 *                                                                      *
 * The channel's state is changed to PKCH_STATE_WAIT, indicating that   *
 * the channel is waiting for enough raw data to perform the            *
 * initialization actions.                                              *
 *                                                                      *
 * Symmetry collection is not initialized here because it needs  data   *
 * from the series. This initialization will be handled in              *
 * SetInitialValues during the next pass through ProcessChannelMessage  *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE = a parameter passed in NULL                      *
 *                                                                      *
 ************************************************************************/
int InitChannelForSeries(double startTime, PICK_CHANNEL_INFO *channelinfo)
{
    if (channelinfo == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "channelinfo passed in NULL to InitChannelForSeries. \n");
      return EW_FAILURE;
    }
      
    /* initialize channel triggers */
	ClearChannelTriggers(channelinfo); 
   
    channelinfo->state = PKCH_STATE_WAIT;
             
    /* reinitialize the trigger suppression counter */
    channelinfo->processed_points = 0;
             
    /* restart the buffer from the beginning */
    channelinfo->rawLength = 0;
   
    channelinfo->rawStartTime = startTime;
   
    return EW_SUCCESS;
}

/************************************************************************
 *                       PreFilterSample()                              *
 *                                                                      *
 * Pre-filter data from broadband stations so they mimic data from      *
 * traditional short-period instruments and data from short-period      *
 * instruments in order to improve picking                              *
 *                                                                      *
 * Input data are stored in channel->rawbuffer[];                       *
 * Output data are stored in g_PreFiltered[]                            *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE = channel passed in NULL                          *
 *                                                                      *
 ************************************************************************/
static int PreFilterSample(PICK_CHANNEL_INFO *channel)
{
    static double prefilteredValue;
    int           s;
    long          ssz;


    if (channel == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "channel passed in NULL to PreFilterSample. \n");
      return EW_FAILURE;
    }
   
    reportError(WD_DEBUG, NORESULT, "pre-filtered mean: %f\n", channel->pre_filter.mean);
   
    /* Pre-filter broadband stations so that they mimic short-period instruments */
    for (s = 0, ssz = channel->rawLength; s < ssz; s++)
    {
      preFilterSamplePoint(channel->rawBuffer[s], &channel->pre_filter, 
                                  &prefilteredValue);
                                
      g_preFiltered[s] = prefilteredValue;
    }

    return EW_SUCCESS;
}
/************************************************************************
 *                      SetInitialValues()                              *
 *                                                                      *
 *  initializes filter trigger values                                   *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_WARNING = not enough data to process                      *
 *         EW_FAILURE = a parameter passed in NULL                      *
 *                                                                      *
 ************************************************************************/
static int SetInitialValues(RaypickerSCNL *scnl)
{  
    PICK_CHANNEL_INFO *channel = scnl->pchanInfo; /* pointer to this channel's data */
    PICK_SERIES_DATA  *workPicker;                /* pointer to series data for this channel */
    static double      val0;                      /* Buland's a0  */
    static double      val3;                      /* Buland's a3  */
    static double      val3b;                     /* Buland's a3b (a3, second pass) */
        
    static double      taperlen;                  /* Buland's iirrmp */
    static double      taperstt;  
     
    static double      rawmean;                   /* mean of raw sample */
    static double      rectmean;                  /* mean of rectified raw sample */
           int         i;                         /* index into raw data & filtered data */
           double      t;
           long        tsz;
           short       pickerIndex;               /* index into picker types */
           int         retc = EW_SUCCESS;
   
     if (channel == NULL)
     {
       reportError(WD_FATAL_ERROR, GENFATERR, "channel passed in NULL to SetInitialValues. \n");
       return EW_FAILURE;
     }

   
    /*  WARNING: DON'T CHANGE THE CHANNEL STATE;
     *           STILL NEED TO PERFORM SOME INITIALIZATION
     *           AFTER PROCESSING THE FIRST BUFFER (TO FOLLOW) */
      
    if (channel->rawLength < GetTriggerLevelCalcLength(channel->sampleRate))
    {
      /* first buffer in series must be at least GetTriggerLevelCalcLength(sampleRate) points long */
      reportError(WD_WARNING_ERROR, NORESULT, "SetInitialValues(): sample buffer size [%d] too short for channel initialization\n", 
                        channel->rawLength);
      return EW_WARNING;
    }

    val0     = 0.0;
    taperlen = floor(INIT_TAPER_LEN * channel->sampleRate + 0.5);
    taperstt = 0.25 * (taperlen * taperlen);
    val3     = taperstt;
    val3b    = 0.0;    
      
    i = 0;
    for (t = 0.0, tsz = MAX((long)taperlen, g_rateConstants->navetm); 
         t < tsz; t += 1.0, i++) 
    {
      /* Gather initialization values from raw data (FORTRAN do 5 i=navest,navetm) */
      if (g_rateConstants->navest <= i && i <= g_rateConstants->navetm) 
      {
          /* Sum of raw amplitudes */
          val0 += channel->rawBuffer[i];
            
          /* Sum of rectified raw amplitudes */
          val3 += fabs(channel->rawBuffer[i]);
      }
         
      if (t <= taperlen) 
      {
          /* Taper the series start to avoid transients due to big microseisms 
           * (FORTRAN do 94 i=1,iirrmp) */
          g_preFiltered[i] = ((t*t)/(taperstt + t*t)) * g_preFiltered[i];
      }
         
      /* FORTRAN do 6 i=navest,navetm */
      if (g_rateConstants->navest <= i && i <= g_rateConstants->navetm) 
      {
          /* Sum of rectified pre-filtered values */
          val3b += fabs(g_preFiltered[i]);
      }     
    } /* collecting sum values */
       
           
    /* Calculate initial mid-level triggering value */
    if ((channel->prefiltmean = g_rateConstants->aveln * val3b) < SNMIN)
         channel->prefiltmean = SNMIN;

    /* Calculate initial long-term average (lta)
	 * val3b = TRGLMI * channel->prefiltmean
	 * PICKER_PARAM_DATA[0][PICKER_SENSITIVITY] = TRGLM = 1/TRGLMI */
    val3b = channel->prefiltmean / PICKER_PARAM_DATA[0][PICKER_SENSITIVITY];
      
    /* set lta and mid-level trigger in all filters */
    for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
	{
      if ((retc = InitPickSeriesLevels(val3b, channel->prefiltmean, 
                PICKER_PARAM_DATA[0][PICKER_SENSITIVITY], &channel->pick_series[pickerIndex])) != EW_SUCCESS)
          return retc;
    }
  
    /*
     * dbh:
     *
     * The FORTRAN uses a variable, g1 to determine the s/n of the amplitude.
     * However, that value is determined from a mean of the rectified raw
     * sample, which is later adjusted by the rectified pre-filtered values.
     * 
     * This code just uses the rectified pre-filtered throughout.
     */
       
    /* mean of raw values */
    rawmean = g_rateConstants->aveln * val0; 

    /* mean of rectified raw values */
    if ((rectmean = g_rateConstants->aveln * val3) <  SNMIN)
      rectmean = SNMIN;
      
    /*  initialize symmetry data for the start of a new time series */
	if ((retc = InitSymmetryData(channel->rawBuffer[0], rawmean, rectmean, 
				SNMIN, channel->sampleRate, &channel->symmetry_data)) != EW_SUCCESS)
    {
      reportError(WD_WARNING_ERROR, NORESULT, "InitSymmetryData(%f, %f, %f, %f, %f) returned error %d\n", 
                    channel->rawBuffer[0], rawmean, rectmean, 
                    SNMIN, channel->sampleRate, retc);
      return EW_WARNING;
    }
                 
    /* part of pick series initialization */
    for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
    {
      workPicker = &channel->pick_series[pickerIndex];
        
      if ((retc = InitPickFilterForSeries(workPicker)) != EW_SUCCESS)
          return retc;
         
      workPicker->value_abs_limit = 2.0 * workPicker->trigger_value * workPicker->lta;
         
      /*  between FORTRAN 18-19: cmt2 ==> 1.0 / workPicker->mid_trigger;  */
      /*  after FORTRAN 20: cmt3 = BFAC * cmt2 ==> workPicker->confirm_factor =
	   *  PICKER_PARAM_DATA[pickerIndex][PICKER_CONFFACT] * cmt2; */
      workPicker->confirm_factor = PICKER_PARAM_DATA[pickerIndex][PICKER_CONFFACT]
                                   / workPicker->mid_trigger;
    }
          
    return EW_SUCCESS;
}

/************************************************************************
 *                   STASymmetryEnvelope()                              *
 *                                                                      *
 * Calculates the short-term average (STA), symmetry and envelope for   *
 * the entire span of channel->raw_buffer                               *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_WARNING = unacceptable return from SymmetryCheck          *
 *         EW_FAILURE = scnl passed in NULL                             *
 ************************************************************************/
static int STASymmetryEnvelope(RaypickerSCNL *scnl)
{
    PICK_CHANNEL_INFO *channel = scnl->pchanInfo;
    PICK_SERIES_DATA  *workPicker;      /* picker series data for this channel  */
    static double      sta_minimum;     /* minumum STA (Bullard's clt1)         */
    static double      rawdiff;         /* drift corrected signal value         */
    static double      filteredValue;   /* pick series' IIR filter output value */
   
    static int         s;
    static long        ssz;
           short       pickerIndex;
           int         retc = EW_SUCCESS;

    if (scnl == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "scnl passed in NULL to STASymmetryEnvelope. \n");
      return EW_FAILURE;
    }
   
    if (channel->state  == PKCH_STATE_INIT)
	  sta_minimum  = SNMIN * channel->pick_series[0].lta;
   
    /* initialize sample time to one point before start of buffer */
    g_sampleTime = channel->rawStartTime - g_rateConstants->sample_interval;
   
#ifdef DBG_WRITE_ENVELOPE
    sprintf(dbg_filename, "%s%s%s_envelope.txt", getenv("EW_LOG"), scnl->sta, scnl->chan);
    if ((g_debugFile = fopen(dbg_filename,"a")) == NULL)
    {
      logit("e", "failed to open envelope debug file: %s\n", dbg_filename);
    }
    else
    {
      if (dbg_write_header)
      {
          fprintf(g_debugFile, "time          time_str                 raw           ");
          for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
              fprintf(g_debugFile, "  env%d    ", pickerIndex);
          fprintf(g_debugFile, "\n");
      }
    }
#endif

#ifdef DBG_WRITE_SYMMCALC
    sprintf(dbg_filename, "%s%s%s_symmetrycalc.txt", getenv("EW_LOG"), scnl->sta, scnl->chan);
    if ((dbg_symcalcfile = fopen(dbg_filename,"a")) == NULL)
	{
        logit("e", "failed to open symmetry calc debug file: %s\n", dbg_filename);
	}
    else
	{
      if (dbg_write_header)
	  {
          fprintf(dbg_symcalcfile, "time           time_str            raw         ");
          for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
              fprintf(dbg_symcalcfile, "last%d   xmt%d    valLimit%d vAbsLim%d filtVal Sym ",
                     pickerIndex, pickerIndex, pickerIndex);
          fprintf(dbg_symcalcfile, "\n");
	  }
	}
#endif


#if defined(DBG_WRITE_ENVELOPE) || defined(DBG_WRITE_SYMMCALC)
/*    dbg_time = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0) - g_rateConstants->sample_interval; */
    dbg_time = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0);
#endif
     
    for (s = 0, ssz = channel->rawLength; s < ssz; s++)
    {
      g_sampleTime += g_rateConstants->sample_interval;     
      
#if defined(DBG_WRITE_ENVELOPE) || defined(DBG_WRITE_SYMMCALC)
      dbg_time += g_rateConstants->sample_interval;
      TimeToDTString(g_sampleTime, dbg_timestr);
#endif      

#ifdef DBG_WRITE_ENVELOPE
      if (g_debugFile != NULL)
          fprintf(g_debugFile, "%8.3f %s %f %lf %lf", dbg_time, dbg_timestr, g_preFiltered[s],
          channel->pre_filter.mean, channel->rawBuffer[s]);
#endif

#ifdef DBG_WRITE_SYMMCALC
      if (dbg_symcalcfile != NULL)
          fprintf(dbg_symcalcfile, "%8.3f %s %f", dbg_time, dbg_timestr, g_preFiltered[s]);
#endif


      /* remove the drift from the original signal 
       * c2(kl)=rng(nptr).nbuf(kl)-a1 */
      rawdiff = channel->rawBuffer[s] - channel->symmetry_data.rawmean;
      
      /* Adjust the mean of the symmetry data
       *
       * Use the first filter (the one with the highest frequency)
       * to determine if the simulated raw sample mean should be adjusted.
       * (last_value (= curent filtered value) < mid_trigger ==> no ongoing trigger)
       *
       * if(btm1.le.xmt1) a1=a1+c2(kl)*d10 between FORTRAN 98 and 8 */
      if (channel->pick_series[0].last_value <= channel->pick_series[0].mid_trigger)
		      channel->symmetry_data.rawmean += (rawdiff * g_rateConstants->lag10sec);

      for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
      {
          workPicker = &channel->pick_series[pickerIndex];
         
          BandpassSample(g_preFiltered[s], sta_minimum, g_rateConstants, 
							workPicker, &filteredValue);
                   
          /* On first (highest frequency) filter only, separate the positive 
           * and negative signals for symmetry tracking. */
          if (pickerIndex == 0)
          {
              switch ((retc = SymmetryCheck(rawdiff, filteredValue, workPicker->mid_trigger, 
                                            SNMIN, s, g_Symmetry, &channel->symmetry_data)))
              {
                  /* Current symmetry state is symmetric, adjust simulated mean of pre-filtered
                   * if(ir3.le.1) g1=g1+(abs(a2(kl))-g1)*d1 */
		      case SYMM_SYMMETRIC:
                      channel->prefiltmean += (fabs(g_preFiltered[s]) - channel->prefiltmean) 
                                                 * g_rateConstants->lag1sec;
                   
                      /* intentional fall-through */
                   
                  case SYMM_PARTSYMMETRIC:
                      retc = EW_SUCCESS;
                      break;
                   
                  case SYMM_ASYMMETRIC:
                      retc = EW_SUCCESS;
                      break;
                   
                  default:
                      reportError(WD_WARNING_ERROR, NORESULT, "SymmetryCheck(%f, %f, %f, %f, %f, %f, %d, ->(*), (*)->) returned error %d\n", 
                                    rawdiff, g_preFiltered[s], filteredValue, workPicker->mid_trigger, 
                                    g_rateConstants->lag1sec, SNMIN, s, retc);
                      return EW_WARNING;
              }            
          } /* first (highest frequency) filter */

          /* Calculate the envelope value
		   * Use a longer period low pass for the duration (envelope) signal 
           * sf1 = sf1 + (s1(kl) - sf1) * d2 */
          workPicker->last_envelope += ((workPicker->last_rect - workPicker->last_envelope) 
                                           * g_rateConstants->lag2sec);
         
          g_Envelope[pickerIndex][s] = workPicker->last_envelope;


#ifdef DBG_WRITE_ENVELOPE
          if (g_debugFile != NULL)
              fprintf (g_debugFile, "   %f", g_Envelope[pickerIndex][s]);
#endif

          /* Symmetry state is not asymmetric,
           * Update the floating maximum for the filtered value,
           * but require it to be at least value_abs_limit [2 * lta * trigger_value]
           * 
           * Note that the change rate is pegged to d1 */
          if (channel->symmetry_data.ir3 != SYMM_ASYMMETRIC)
          {   
              workPicker->value_limit += (workPicker->last_rect - workPicker->value_limit) * g_rateConstants->lag1sec;
              workPicker->value_limit = MIN(workPicker->value_limit, workPicker->value_abs_limit);  
          }
         
          /* Grab the filtered value
           * 
           * dbh: This MIN clips the short-term average to a maximum value of value_limit
           *      which is updated just above. 
           */
          g_Filtered[pickerIndex][s] = MIN(workPicker->value_limit, filteredValue);


#ifdef DBG_WRITE_SYMMCALC
          if (dbg_symcalcfile != NULL)
          { 
              fprintf(dbg_symcalcfile, "  %f  %f  %f %f %f %d", 
                           workPicker->last_value, workPicker->mid_trigger, 
                           workPicker->value_limit, workPicker->value_abs_limit,
                           filteredValue, channel->symmetry_data.ir3); 
          }
#endif

         
      } /* each filter */

#ifdef DBG_WRITE_SYMMCALC
      fprintf(dbg_symcalcfile, "\n");
#endif      
         
         
#ifdef DBG_WRITE_ENVELOPE   /* uncomment to write the envelope values */
      if (g_debugFile != NULL)
          fprintf(g_debugFile , "\n");
#endif
            
    } /* each point in the sample : determining short-term average */
   
#ifdef DBG_WRITE_ENVELOPE
    if (g_debugFile != NULL)
    {
        fclose(g_debugFile);
        g_debugFile = NULL;
    }
#endif

#ifdef DBG_WRITE_SYMMCALC
    if (dbg_symcalcfile != NULL)
    {
        fclose(dbg_symcalcfile);
        dbg_symcalcfile = NULL;
    }
#endif

#ifdef DBG_WRITE_FILTERED
    sprintf(dbg_filename, "%s%s%s_filtered.txt", 
				getenv("EW_LOG"), scnl->sta, scnl->chan);
    if ((g_debugFile = fopen(dbg_filename,"a")) == NULL)
        logit("e", "failed to open filtered debug file: %s\n", dbg_filename);
    else
    {
      if (dbg_write_header)
      {
          fprintf(g_debugFile, "time          time_str        ");
          for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
              fprintf(g_debugFile, "  F%d_Filt ", pickerIndex);

          fprintf(g_debugFile, "\n");
      }
      
      g_sampleTime = channel->rawStartTime - g_rateConstants->sample_interval;
/*      dbg_time    = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0) - g_rateConstants->sample_interval; */
      dbg_time    = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0);
      for (s = 0, ssz = channel->rawLength; s < ssz; s++)
      {
          g_sampleTime += g_rateConstants->sample_interval;
          dbg_time += g_rateConstants->sample_interval;
          TimeToDTString(g_sampleTime, dbg_timestr);
          fprintf(g_debugFile, "%8.3f %s", dbg_time, dbg_timestr); 
          for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
              fprintf(g_debugFile, "  %9.5f", g_Filtered[pickerIndex][s]);
          fprintf(g_debugFile , "\n");
      }
      fclose(g_debugFile);
      g_debugFile = NULL;
    }
#endif
     
#ifdef DBG_WRITE_SYMMETRY
    sprintf(dbg_filename, "%s%s%s_symmetry.txt", getenv("EW_LOG"), scnl->sta, scnl->chan);
    if ((g_debugFile = fopen(dbg_filename,"a")) == NULL)
      logit("e", "failed to open envelope debug file: %s\n", dbg_filename);
    else
    {
      if (dbg_write_header)
          fprintf(g_debugFile, "time         time_str      Symmetry\n");
      
      g_sampleTime = channel->rawStartTime - g_rateConstants->sample_interval;
/*      dbg_time = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0) - g_rateConstants->sample_interval; */
      dbg_time = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0);
      for (s = 0, ssz = channel->rawLength; s < ssz; s++)
      {
          g_sampleTime += g_rateConstants->sample_interval;
          dbg_time += g_rateConstants->sample_interval;
          TimeToDTString(g_sampleTime, dbg_timestr);
          fprintf(g_debugFile, "%8.3f %s  %d\n", dbg_time, dbg_timestr, g_Symmetry[s]);
      }
      fclose(g_debugFile);
      g_debugFile = NULL;
    }
#endif
   
    return retc;
}

/************************************************************************
 *                      InitializeLTA()                                 *
 *                                                                      *
 * Recalculates the long-term average as part of initialization.        *
 *                                                                      *
 * COMPLETES INITIALIZATION FOR THIS CHANNEL                            *
 *                                                                      *
 * NOTE: the assumption made that the length of the first buffer is     *
 *       both greater than navetm and greater than or equal to          *
 *       GetTriggerLevelCalcLength() [ntrlv].                           *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE = channel passed in NULL                          *
 ************************************************************************/
static int InitializeLTA(PICK_CHANNEL_INFO *channel)
{
    PICK_SERIES_DATA  *workPicker;  /* pick series data for this channel */
    static double      lta;         /* long-term average */
    static double      trgvl;       /* trigger level     */
    static double      clt4;   
    static double      taperlen;
           long        i;           /* loop counter      */
           long        isz;         /* loop upper bound  */
           short       pickerIndex;


    if (channel == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "channel passed in NULL to InitializeLTA. \n");
      return EW_FAILURE;
    }
      
    /* Recalculate the long term averages (now that we have a short-term averages) */   
    for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
    {
      workPicker = &channel->pick_series[pickerIndex];
      
      /* Adjust the long-term average */    
      lta = 0.0;
      trgvl = workPicker->trigger_value;
         
      /* Accumulate for the long-term average (FORTRAN do 80 i=navest,navetm) */
	  for (i = g_rateConstants->navest; i <= g_rateConstants->navetm; i++)
          lta  += g_Filtered[pickerIndex][i];

      workPicker->lta = MAX(g_rateConstants->aveln * lta, SNMIN);
         
      /* Adjust the trigger level */   
      clt4 = (PICKER_PARAM_DATA[pickerIndex][PICKER_SENSITIVITY] - 1.0) / workPicker->lta;
      
      /* calculate triggering level 
       * trgvl1=amax1(trgvl1+(clt4*b1(kl)-trgvl1)*d10,trglm) (FORTRAN 17) */
      for (i = 0, isz = GetTriggerLevelCalcLength(channel->sampleRate); i < isz;  i++)
      {
          trgvl += (clt4 * g_Filtered[pickerIndex][i] - trgvl) * g_rateConstants->lag10sec;
		  trgvl = MAX(trgvl, PICKER_PARAM_DATA[pickerIndex][PICKER_SENSITIVITY]);
      }
         
      workPicker->trigger_value = trgvl;
         
    }

    /* INITIALIZATION FINALLY FINISHED */
    channel->state  = PKCH_STATE_LOOK;
   
    return EW_SUCCESS;
}

/************************************************************************
 *                     ProcessChannelMessage()                          *
 *                                                                      *
 * Performs initialization if needed,                                   *
 * collects the sta & levels, performs picking, etc.                    *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE = channel passed in NULL or something else failed *
 *                                                                      *
 ************************************************************************/
int ProcessChannelMessage(RaypickerSCNL *Scnl, RParams rParams, EWParameters EwParams)
{
    PICK_CHANNEL_INFO   *channel = Scnl->pchanInfo; /* pointer to this channel's data              */
    PICK_SERIES_DATA    *workPicker;                /* pick series data for this channel           */
    static SERIES_DATA  *STA;                       /* 'global' STA work buffer, is set 
	                                                 *  = g_Filtered[pickerIndex]                  */
    static int          shutdownall;                /* flag indicating all triggers must shut down */
    static int          shutdownthis;               /* flag, 'this' trigger too long, shut it down */
           long          i;                         /* loop counter                                */
           short         pickerIndex;               /* loop counter                                */
           short         ph;                        /* loop counter                                */
           int           rc = EW_SUCCESS;           /* return code                                 */


    /* array to turn on (OR) the bit flag;
     * WARNING: THIS ARRAY MUST CONTAIN AS MANY ELEMENTS AS THERE ARE PICKER TYPES */
    static const PICKER_AGGR_STATE PICKER_ON[PICKER_TYPE_COUNT] =
    {
      (PICKER_AGGR_STATE)0x01,   /* 00000001 */
      (PICKER_AGGR_STATE)0x02    /* 00000010 */
    };

    if (channel == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "channel passed in NULL to ProcessChannelMessage. \n");
      return EW_FAILURE;
    }

    /* =======================================================================================
     * Get the constants for this sample rate
     */

    if ((rc = GetRateConstants(channel->sampleRate, &g_rateConstants, rParams.MaxPreFilters, 
                                rParams.Debug)) != EW_SUCCESS)
      return rc;

    /* =======================================================================================
     * Pre-Filter the raw data into array g_preFiltered[]
     */

      if ((rc = PreFilterSample(channel)) != EW_SUCCESS)
          return rc;
   
    /* =======================================================================================
     * INITIALIZATION, PART ONE
     */
    if (channel->state  == PKCH_STATE_INIT)
    {
      if ((rc = SetInitialValues(Scnl)) != EW_SUCCESS)
        return rc;
    }
   
    /* =======================================================================================
     * PROCESS THE RAW DATA INTO FILTERED [STA], CHECK SYMMETRY, ETC.
     */

    if ((rc = STASymmetryEnvelope(Scnl)) != EW_SUCCESS)
      return rc;
   
    /* =======================================================================================
     * INITIALIZATION, PART TWO
     */
    
    if (channel->state  == PKCH_STATE_INIT)
    {
      if ((rc = InitializeLTA(channel)) != EW_SUCCESS)
          return rc;
    }
   
    /*
     * The channel should arrive at this point in one of these states:
     * 
     *    PKCH_STATE_LOOK   -- looking for trigger
     *    PKCH_STATE_POSS   -- possible signal
     *    PKCH_STATE_PRELIM -- preliminary trigger
     *    PKCH_STATE_TRIG   -- trigger not yet confirmed
     *    PKCH_STATE_CONF   -- trigger confirmed, waiting shutdown
     */
    

    /* =======================================================================================
     * Initialization and preparatory loops completed,
     * 
     * Loop over all points finding trigger, reporting picks, collecting and reporting amps
     * 
     */ 

     
#ifdef DBG_WRITE_TEST
    sprintf(dbg_filename, "%s%s%s_test.txt", getenv("EW_LOG"), Scnl->sta, Scnl->chan);
    if ((g_testFile = fopen(dbg_filename,"a")) == NULL)
      logit("e", "failed to open test values file: %s\n", dbg_filename);
    else
    {
      if (dbg_write_header)
      {
          fprintf(g_testFile, "time          time_str      ");
          for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
          {
              workPicker = &channel->pick_series[pickerIndex];
              fprintf(g_testFile, "   sta%d  xmt%d  xlt%d  env%d  xmf%d ",
                  pickerIndex, pickerIndex, pickerIndex, pickerIndex, pickerIndex);
		  }
          fprintf(g_testFile, "\n");
	  }
	}
#endif

#ifdef DBG_WRITE_TRIGGER
    sprintf(dbg_filename, "%s%s%s_trigger.txt", getenv("EW_LOG"), Scnl->sta, Scnl->chan);
    if ((g_triggerFile = fopen(dbg_filename,"a")) == NULL)
      logit("e", "failed to open trigger values file: %s\n", dbg_filename);
    else
    {
      if (dbg_write_header)
      {
          fprintf(g_triggerFile, "time          time_str      ");
          for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
          {
              workPicker = &channel->pick_series[pickerIndex];
              fprintf(g_triggerFile, "isPrelim%d isTrig%d isConfirm%d",
                           pickerIndex, pickerIndex, pickerIndex);
		  }
          fprintf(g_triggerFile, "\n");
	  }
	}
#endif

      
    /* Initialize the time for the buffer; set to a notional point just before 
     * the first point in the buffer */    
    g_sampleTime = channel->rawStartTime - g_rateConstants->sample_interval;

    for (i = 0; i < channel->rawLength; i++)
    {
      g_sampleTime += g_rateConstants->sample_interval;   
      

#if defined(DBG_WRITE_TEST) || defined(DBG_WRITE_TRIGGER)
          TimeToDTString(g_sampleTime, dbg_timestr);
          dbg_time = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0);
#endif
     

      
/* #ifdef DBG_WRITE_TEST

      if (g_testFile != NULL)
      {
          TimeToDTString(g_sampleTime, dbg_timestr);
          dbg_time = g_sampleTime - (floor(g_sampleTime / 86400.0) * 86400.0) - g_rateConstants->sample_interval;
          fprintf(g_testFile, "%8.3f %s", dbg_time, dbg_timestr );
      }
#endif
*/

#ifdef DBG_WRITE_TEST
      if (g_testFile != NULL)
          fprintf(g_testFile, "%8.3f %s", dbg_time, dbg_timestr );
#endif

#ifdef DBG_WRITE_TRIGGER
      if (g_triggerFile != NULL)
          fprintf(g_triggerFile, "%8.3f %s", dbg_time, dbg_timestr );
#endif

      
      for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
      {   
          /* Get convenience pointers */
          STA  = g_Filtered[pickerIndex];
          workPicker = &channel->pick_series[pickerIndex];
         
          /* Update the STA series with the new STA value */
          NewSTAValue(g_sampleTime, STA[i], workPicker);
         
          /* Update the pick series triggering levels from the new STA point g_sampleTime */
          UpdateLevels(STA[i], g_rateConstants, workPicker);

#ifdef DBG_WRITE_TEST
          if (g_testFile != NULL)
              fprintf(g_testFile, "   %4.3f %4.3f %4.3f %4.3f %4.3f", 
                        STA[i], workPicker->mid_trigger, workPicker->lta, 
                        g_Envelope[pickerIndex][i], workPicker->shutdown_level);
#endif

          /* FORTRAN 23:  if(ivia1.ge.1) go to 24 */
          switch (workPicker->state)
          {
			  /* LOOKING FOR CONTINUATION OF A PRELIMINARY TRIGGER */
			  case PKCH_STATE_CONT:

                  /* increment arrival time maximum lifetime counter */
                  workPicker->max_life_counter++;

                  /* reset if have looked too long for continuation of a preliminary trigger 
                   * or if there have been too many preliminary triggers without a 
                   * confirmed trigger (currently set for about 64 secs) */
                  if ((workPicker->continue_counter > g_rateConstants->nlnktm) ||
                      (workPicker->max_life_counter > 8 * g_rateConstants->nlnktm))
				  {

                      if (rParams.Debug >= 2)
                          logit("", "turning off continuation of preliminary trigger for <%s:%s:%s:%s> \n", 
                                 Scnl->sta, Scnl->chan, Scnl->net, Scnl->loc);
					  
                      /* turn off continuation counter if too long since last preliminary
				       * trigger ended */
                      if (channel->isContinuing & PICKER_ON[pickerIndex])
                          channel->isContinuing ^= PICKER_ON[pickerIndex];
                      workPicker->continue_counter = 0;
                      
                      /* reset max time to live (for recurring prelim triggers) counter */
                      workPicker->max_life_counter = 0;

                      /* reset the arrival time */
                      workPicker->arrival = PKFL_NO_ARRIVAL;
                  }
				  
				  /* update continuation counter for this filter */
                  workPicker->continue_counter++;

                  /* Intentional fall through */
			  
              /* LOOKING FOR A PRELIMINARY TRIGGER */
              case PKCH_STATE_LOOK:
				 
				  /* Signal below triggering level; continue with next data point */
				  if (STA[i] <= workPicker->mid_trigger) 
					  break;

                  /* Signal is above initial triggering level;
                   * If too early in the time series to identify a trigger
                   * (trigger not yet allowed), just continue to next point */
                  if ((long)channel->processed_points < (long)g_rateConstants->nsettm)
                      break;
                
                  /* Sample point has exceeded level to identify preliminary triggers (xmt).
				   * and enough data for initialization;
                   * Change state and fall through */
                  workPicker->state = PKCH_STATE_PRELIM;

                  /* Adjust the channel's aggregate state */
                  if (!(channel->isPrelim & PICKER_ON[pickerIndex]))
                      channel->isPrelim |= PICKER_ON[pickerIndex];
                  if (channel->isContinuing & PICKER_ON[pickerIndex])
                      channel->isContinuing ^= PICKER_ON[pickerIndex];
                   
                  /* reset preliminary trigger time-out counter (prelim_counter)
                   * continuation counter (continue_counter)
                   * and trigger confirmation counter (confirm_counter) */
                  workPicker->prelim_counter   = 0;
                  workPicker->continue_counter = 0;
                  workPicker->confirm_counter  = 0;
               
                  /* Intentional fall-through */
                     
              case PKCH_STATE_PRELIM:

                  /* increment arrival time maximum lifetime counter */
                  workPicker->max_life_counter++;

                  /* Signal has fallen below xmt.
                   * This preliminary trigger has died.
                   * Go to next point and resume search for signal
                   * of appropriate strength. */
                  if (STA[i] <= workPicker->mid_trigger)
                  { 
                      workPicker->state  = PKCH_STATE_CONT;

                      if (channel->isPrelim & PICKER_ON[pickerIndex])
                          channel->isPrelim ^= PICKER_ON[pickerIndex];

                      if (!(channel->isContinuing & PICKER_ON[pickerIndex]))
                          channel->isContinuing |= PICKER_ON[pickerIndex];                      
                      workPicker->continue_counter = 0;
                      break;
                  }
                 
                  /* Signal level remains above triggering level;
                   * thus preliminary state continues, but trigger not yet confirmed. */
                

                  /* Signal level remains above triggering level and is asymmetric.
                   * Remain preliminary until signal turns symmetric (i.e., there is
				   * a zero crossing in signal) or time runs out. */
                  if (g_Symmetry[i] == SYMM_ASYMMETRIC)
                  {  

                      /* FORTRAN LINE 27 + 2 */
                   
                      /* increment preliminary trigger run counter */
                      workPicker->prelim_counter++;
                  
                      /* Preliminary trigger has continued too long without confirmation,
                       * (meaning that it remains asymmetric),
                       * discard preliminary identification and resume trigger search. 
                       * FORTRAN LINE 27  +~5 */
                      if (g_rateConstants->nlnktm < workPicker->prelim_counter)
                      {
                          workPicker->state  = PKCH_STATE_LOOK;
                          if (channel->isPrelim & PICKER_ON[pickerIndex])
                              channel->isPrelim ^= PICKER_ON[pickerIndex];

						  /*  reinitialize arrival time */
                          workPicker->arrival = PKFL_NO_ARRIVAL;

                          /* reset max time to live (for recurring prelim triggers) counter */
                          workPicker->max_life_counter = 0;

                          if (rParams.Debug >= 2)
                          {
                              /* print *,'----------> flt 1 unconf off',triggers_on,i */
                              logit("", "%s f[%d]  unconf off, asymmetric; mid-level %f  sta %f\n", 
                                         dbg_timestr, pickerIndex, workPicker->mid_trigger, STA[i]);
                          }
                      }
                      
                      /* Preliminary trigger not yet confirmed, continue to next point */
                      break;
                  }

                   
                  /* Signal is above preliminary triggering level and is symmetric;
                   * BEGINNING OF POSSIBLE SIGNAL */
                  if (rParams.Debug >= 2)
                      logit("", "%s f[%d] start of possible signal STA[i] %f > mid-level %f  ; lta %f\n", 
                                dbg_timestr, pickerIndex, STA[i], workPicker->mid_trigger, 
                                workPicker->lta);

                  /* Calculate an arrival time only if this is the first
                   * preliminary trigger for this possible arrival */
                  if (workPicker->arrival == PKFL_NO_ARRIVAL)
                  {
                      if (rParams.Debug >= 2)
                      {
                          /* print *,'----------> flt 1 unconf on',triggers_on,kl */
                          logit("", "%s f[%d] unconf on, calc arrival time\n", dbg_timestr, pickerIndex);
                      }   
                      NewTrigger(g_rateConstants, workPicker, rParams.Debug);
                  }
                  else
                  {
                      if (rParams.Debug >= 2)
                          logit("", "%s f[%d] resuming trigger\n", dbg_timestr, pickerIndex);
                  }

                  /* set the flags for a possible trigger; still need to wait until trigger
                   * has lasted sufficiently long to confirm it */
                  workPicker->state = PKCH_STATE_TRIG;
                  if (!(channel->isTrigger & PICKER_ON[pickerIndex]))
                      channel->isTrigger |= PICKER_ON[pickerIndex];

                  /* reset preliminary trigger counter */
                  workPicker->prelim_counter = 0;

                  /* reset max time to live (for recurring prelim triggers) counter */
                  workPicker->max_life_counter = 0;
               
                  /* Intentional fall-through */            
                                   
              case PKCH_STATE_TRIG:
           
                  /* Increment the counter for the trigger confirmation duration */
                  workPicker->confirm_counter++;
                   
                  /* Signal has fallen below mid_trigger, i.e., this trigger had died,
                   * Go to next point and resume search for signal of appropriate strength */
                  if (STA[i] <= workPicker->mid_trigger)
                  {
                      if (rParams.Debug >= 2)
                      {
                          logit("o", "%s f[%d] sta %f dropped below mid-level %f at dur %d/%d, resume search for triggering level\n", 
                                         dbg_timestr, pickerIndex, STA[i], workPicker->mid_trigger, 
                                         workPicker->confirm_counter, (int)workPicker->confirm_duration);
                      }
                   
                      /* change filter state */
                      workPicker->state   = PKCH_STATE_CONT;
                  
                      /* adjust channel aggregate state */
                      if (channel->isTrigger & PICKER_ON[pickerIndex])
                          channel->isTrigger ^= PICKER_ON[pickerIndex];

                      if (channel->isPrelim  & PICKER_ON[pickerIndex])
                          channel->isPrelim  ^= PICKER_ON[pickerIndex];

                      if (!(channel->isContinuing & PICKER_ON[pickerIndex]))
                          channel->isContinuing |= PICKER_ON[pickerIndex];
                   
                      /* restart continuation counter */
                      workPicker->continue_counter = 0;
                   
                      /* next point */
                      break;
                  }
                
                  /* Signal no longer symmetric; revert this trigger to preliminary state */
                  if (g_Symmetry[i] == SYMM_ASYMMETRIC)
                  {
                      if (rParams.Debug >= 2)
                      {
                          logit("o", "%s  f[%d] signal has gone asymmetric, revert to preliminary trigger (dur: %d)\n", 
                                          dbg_timestr, pickerIndex, workPicker->confirm_counter);
                      }
                   
                      /* change filter state */
                      workPicker->state   = PKCH_STATE_PRELIM;

                      /* increment preliminary counter */
                      workPicker->prelim_counter++;
                   
                      /* leave arrival time unchanged in case it returns to symmetric
                       * workPicker->arrival = PKFL_NO_ARRIVAL; */
                   
                      /* adjust channel aggregate state */
                      if (channel->isTrigger & PICKER_ON[pickerIndex])
                          channel->isTrigger ^= PICKER_ON[pickerIndex];
                   
                      /* Don't clear any collected amps, as the series may return
                       * to symmetry (before the preliminary is rejected), at
                       * which point the collected amps may be used as part of an
                       * ongoing event. */

                      /* next point */
                      break;
                  }
                
                
                  /* FORTRAN LINE 29 & FORTAN LINE 51 */
                
                  /* The required confirmation duration is determined by the
                   * maximum filtered value encountered after the time that
                   * the trigger starts.
                   * This means that the higher the signal encountered, and
				   * consequently, the larger the STA, the longer the required duration. */
                
                  /* Trigger has not yet persisted long enough to be confirmed; 
                   * go to next filter / next point */
                   if (workPicker->confirm_counter < GetConfirmDuration(STA[i], workPicker))
                      break;

                  if (rParams.Debug >= 1)
                      logit("o", "%s  f[%d] confirmed, duration %d; mid-level %f  lta %f\n", 
                            dbg_timestr, pickerIndex, workPicker->confirm_counter, 
                            workPicker->mid_trigger, workPicker->lta);
               
                  /* Trigger has persisted long enough, is now confirmed. */
                  if (channel->state == PKCH_STATE_LOOK)
                  {
                      /* A pick has not been previously been sent for the current
                       * trigger(s). Set the state to direct that the pick be sent.
                       * (The process of sending the pick will set the channel's
                       * state to PKCH_STATE_PICKSENT to prevent the pick from
                       * being sent again until the trigger ends and the
                       * channel's state returns to PKCH_STATE_LOOK). */
                      channel->state = PKCH_STATE_NEWPICK;
                  }              
                           
                  /* Signal has persisted, declare a trigger. */  
                  workPicker->state = PKCH_STATE_CONF;
                  channel->isConfirmed |= PICKER_ON[pickerIndex];
               
                  /* Calculate the shut-down value  (FORTRAN 29 + 10 & FORTRAN 51 +13)*/
                  workPicker->shutdown_level = 0.5 * (workPicker->lta + workPicker->mid_trigger);
               
                  /* go to 28 */
                
                  /* won't shut down on the same point that it was confirmed,
                   * break and go on to next filter or point
                   */
                  break;
                
              case PKCH_STATE_CONF:
            
                  /* If the trigger has gone on too long (i.e., exceeded the maximum 
                   * allowed length), shut it down */
                  if (rParams.MaxTriggerSecs < (g_sampleTime - channel->pickArrTime))
                  {
                      shutdownthis = TRUE;
                   
                      if (rParams.Debug >= 2)
                          logit("o" , "%s  f[%d] too long, force shutdown: %f < (%f - %f)[%f]\n", 
                                    dbg_timestr, pickerIndex, rParams.MaxTriggerSecs, 
                                    g_sampleTime, channel->pickArrTime, 
                                    g_sampleTime - channel->pickArrTime);
                  }
                  else
                      shutdownthis = FALSE;
           
                  /*   Look for a confirmed trigger shutting down */
      
                  /* FORTRAN LINE 24: if(sfn1(kl).gt.xmf1) go to 28 */
                  if (g_Envelope[pickerIndex][i] <= workPicker->shutdown_level 
							|| shutdownthis)
                  {
                      /* Signal level had dropped below the shutdown level */
                      shutdownall = FALSE;
                      
                      if (rParams.Debug >= 1)
                          logit("o", "%s  f[%d] trigger shutdown;  Envelope[%d]: %f   shutdown: %f\n", 
                                    dbg_timestr, pickerIndex, i, g_Envelope[pickerIndex][i], 
                                    workPicker->shutdown_level);
                   
                      /* Tell ReportAmps() that the trigger has shutdown,
                       * ready to send remaining amps if appropriate. */
                      workPicker->state = PKCH_STATE_ENDED;
                   
                      /* Mark the filter as finished in the aggregate state */
                      channel->isFinished  |= PICKER_ON[pickerIndex];
                      
                      /* Remove this filter from the channel's confirmed state
                       * to allow reporting (when all confirmed triggers are
                       * finished) 
                       * 
                       * DO NOT turn off isTrigger or isPrelim here.
                       * They are needed to track other filter's states. */
                      channel->isConfirmed ^= PICKER_ON[pickerIndex];

                    
                      if (pickerIndex == PICKER_ANNOINTED_TYPE)
                      {
                          /* shut down all other triggers */
                          shutdownall = TRUE;
                      
                          if (rParams.Debug >= 2)
                              logit("", "shutting down all triggers because annointed one (%d) is shutting down\n", 
                                        PICKER_ANNOINTED_TYPE);
                      } 
                      else
                      {
                          /* shut down all other triggers unless the annointed filter has been confirmed
                           * and has not yet shutdown */
                          shutdownall = (channel->pick_series[PICKER_ANNOINTED_TYPE].state != PKCH_STATE_CONF);
                      
                          if (rParams.Debug >= 2)
                          {
                              if (shutdownall)
                              {
                                  logit("", "f[%d] shutting down all other triggers because annointed one (%d) is not confirmed\n", 
										pickerIndex, PICKER_ANNOINTED_TYPE);
                              }
                              else
                              {
                                  logit("", "f[%d] NOT shutting down all other triggers because anointed one (%d) is confirmed\n", 
                                                pickerIndex, PICKER_ANNOINTED_TYPE);
                              }
                          }
                      }
                   
                      if (shutdownall)
                      {
                          /* no triggers waiting for shutdown */
                          channel->isConfirmed = 0;
                      
                          for (ph = 0; ph < PICKER_TYPE_COUNT; ph++)
                          {
                              if (pickerIndex == ph)
                                  continue;
                         
                              if (channel->pick_series[ph].state == PKCH_STATE_CONF)
                              {
                                  channel->pick_series[ph].state = PKCH_STATE_ENDED;
                                  channel->isFinished  |= PICKER_ON[ph];
                              }
                              else
                              {
                                  channel->pick_series[ph].state = PKCH_STATE_LOOK;
                              }
                          }
                      }
                  }
                
                  case PKCH_STATE_ENDED:
                  /* No action if trigger has ended,
                   * waiting for all filters on the channel to finish
                   * before reporting.
                   * (Handled after this filter loop, about 20 lines below)*/
                  break;

          }  /* switch (filter state) */         
      } /* Each filter  FORTRAN LINES 28 and 22 */


#ifdef DBG_WRITE_TRIGGER

      for (pickerIndex = 0; pickerIndex < PICKER_TYPE_COUNT; pickerIndex++)
      {
          workPicker = &channel->pick_series[pickerIndex];
          if (workPicker->state == PKCH_STATE_PRELIM)
              fprintf(g_triggerFile, "   1       ");
          else
              fprintf(g_triggerFile, "   0       ");

          if (workPicker->state == PKCH_STATE_TRIG)
              fprintf(g_triggerFile, "   1       ");
          else
              fprintf(g_triggerFile, "   0       ");

          if (workPicker->state == PKCH_STATE_CONF)
              fprintf(g_triggerFile, "   1       ");
          else
              fprintf(g_triggerFile, "   0       ");

              
      }
      fprintf(g_triggerFile, "\n");

#endif
       
      /* Channel activities based on aggregate picker state. */
      if (channel->state == PKCH_STATE_NEWPICK)
      {
          /* Trigger is newly confirmed, ready to report the pick
           * 
           * ReportPick must set channel->state = PKCH_STATE_PICKSENT
           * to prevent repeated pick messages.*/
          ReportPick(Scnl, rParams, EwParams);
         
          if (rParams.Debug >= 1)
              logit("ot", "%s  Reported Pick %d\n", dbg_timestr, channel->pickId);
         
          if (rParams.Debug >= 2)
          {
              static int idbg, jdbg, pdbg;
            
              sprintf(dbg_filename, "%sraypick_%ld_pick.txt", getenv("EW_LOG"), channel->pickId);
              if ((g_debugFile = fopen(dbg_filename,"w")) == NULL)
                  logit("e", "Failed to open pick report debug file %s\n", dbg_filename);
              else
              {
                  fprintf(g_debugFile, "Pick %ld for <%s:%s:%s:%s>\n", channel->pickId,
                            Scnl->sta, Scnl->chan, Scnl->net, Scnl->loc);
                  fprintf(g_debugFile, "Sample rate: %f\n", channel->sampleRate);
                  TimeToDTString(channel->rawStartTime, dbg_timestr);
                  fprintf(g_debugFile, "Raw data start time: %s\n", dbg_timestr);
                  TimeToDTString(g_sampleTime, dbg_timestr);
                  fprintf(g_debugFile, "Current processing time %s\n", dbg_timestr);
                  TimeToDTString(channel->pickArrTime, dbg_timestr);
                  fprintf(g_debugFile, "Arrival time: %s\n", dbg_timestr);
                  fprintf(g_debugFile, "Filter states:\n");
               
                  for (pdbg = 0; pdbg < PICKER_TYPE_COUNT; pdbg++)
                  {
                      workPicker = &channel->pick_series[pdbg];
                      fprintf(g_debugFile, "  %d: ", pdbg);
                      switch(workPicker->state)
                      {
                          case PKCH_STATE_LOOK:
                              fprintf(g_debugFile, "no trigger\n");
                              break;
						  case PKCH_STATE_CONT:
                              TimeToDTString(workPicker->arrival, dbg_arrstr);
                              fprintf(g_debugFile,"continuation of preliminary trigger, (arrival: %s)\n", dbg_arrstr);
                              break;
                          case PKCH_STATE_PRELIM:
                              TimeToDTString(workPicker->arrival, dbg_arrstr);
                              fprintf(g_debugFile,"preliminary trigger, (arrival: %s)\n", dbg_arrstr);
                              break;
                          case PKCH_STATE_TRIG:
                              TimeToDTString(workPicker->arrival , dbg_arrstr);
                              fprintf(g_debugFile, "trigger, waiting for confirmation (arrival: %s  dur %d)\n", 
                                        dbg_arrstr, workPicker->confirm_counter);
                              break;
                         case PKCH_STATE_CONF:
                              fprintf(g_debugFile,"confirmed trigger (This is the trigger resulting in the pick)\n");
                              break;
                          case PKCH_STATE_ENDED:
                              fprintf(g_debugFile, "trigger ended (SHOULD NOT BE HERE)\n");
                              break;
                          default:
                              fprintf(g_debugFile, "unhandled trigger status, ERROR\n");
                              break;
                      }
                  }           
                  fprintf(g_debugFile, "raypick_%ld_sta.txt contain short-term averages for filters <0-n>\n(earliest time first in file)\n",
                                          channel->pickId);
                  fclose(g_debugFile);
                  g_debugFile = NULL;
              }
            
              /* Write the STA values to files */
              sprintf(dbg_filename, "%sraypick_%ld_sta.txt", getenv("EW_LOG"), channel->pickId);
              if ((g_debugFile = fopen(dbg_filename,"w")) == NULL)
                  logit("e", "Failed to open pick station status debug file %s\n", dbg_filename);
              else
              {
                  static double dbastaarr;
                  fprintf(g_debugFile, "time time_str");
                  for (pdbg = 0; pdbg < PICKER_TYPE_COUNT; pdbg++)
                      fprintf(g_debugFile, "  sta%d", pdbg);

                  fprintf(g_debugFile, "\n");
               
                  dbastaarr = g_sampleTime - (((double)workPicker->sta_length) * g_rateConstants->sample_interval);
                  dbg_time   = dbastaarr - (floor(dbastaarr / 86400.0) * 86400.0) - g_rateConstants->sample_interval;
                  for (idbg = 0, jdbg = workPicker->sta_write_point; idbg < workPicker->sta_length; idbg++, jdbg++)
                  {
                      if  (jdbg == workPicker->sta_length)
                          jdbg = 0;

                      dbastaarr += g_rateConstants->sample_interval;
                      dbg_time   += g_rateConstants->sample_interval;
                      TimeToDTString(dbastaarr , dbg_timestr);
                      fprintf(g_debugFile , "%8.3f %s", dbg_time, dbg_timestr);
                      for (pdbg = 0; pdbg < PICKER_TYPE_COUNT; pdbg++)
                          fprintf(g_debugFile , "  %9.5f", channel->pick_series[pdbg].sta_buffer[jdbg]);

                      fprintf(g_debugFile, "\n");
                  }
                  fclose(g_debugFile);
                  g_debugFile = NULL;
              }     
          } /* write debug files */
      } /* new pick */


      if (channel->isFinished != 0     /* some trigger awaiting reporting */
          && channel->isConfirmed == 0 /*   no trigger waiting shutdown   */)
      {
          if (rParams.Debug >= 1)
             logit("o", "%s Pick %d shutdown, cleanup for start of next pick search \n", dbg_timestr, channel->pickId);
                                       
          /* Clear channel state (along with its filters) */
          ClearChannelTriggers(channel);          
      }
      
      
      /* Keep track of the number of points handled for this channel
       * (to prevent triggering too early in the time series). */
      channel->processed_points++;

      /* processed point count has reached highest relevant level,
       * prevent it from ever wrapping (which would happen at
       * about 498 days if the program were to run continuously
       * for that long -- which is a hope).*/
      if ((long)g_rateConstants->nsettm < (long)channel->processed_points)          
          channel->processed_points = g_rateConstants->nsettm;
      
#ifdef DBG_WRITE_TEST
      if (g_testFile != NULL)
          fprintf(g_testFile, "\n");
#endif
      
     
    } /* Entire sample array  FORTRAN LINE 22 */
   

#ifdef DBG_WRITE_TEST
    if (g_testFile != NULL)
    {
        fclose(g_testFile);
        g_testFile = NULL;
    }
#endif

#ifdef DBG_WRITE_TRIGGER
    if (g_triggerFile != NULL)
    {
        fclose(g_triggerFile);
        g_triggerFile = NULL;
    }
#endif


    dbg_write_header = FALSE;

   
    return rc;
}

/************************************************************************
 *                     ClearChannelTriggers()                           *
 *                                                                      *
 * Clears any existing trigger information for the channel to prepare   *
 * to begin looking for another trigger on the channel.  Usually called *
 * after a significant gap [by InitChannelSeries()], or when a pick     *
 * is terminated/shutdown.                                              *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE  = parameter passed in NULL                       *
 *                                                                      *
 ************************************************************************/
int ClearChannelTriggers(PICK_CHANNEL_INFO *channel)
{
    int p;
    int rc;

    if (channel == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "channel passed in NULL to ClearChannelTriggers. \n");
      return EW_FAILURE;
    }
      
    channel->state        = PKCH_STATE_LOOK;   
    channel->isPrelim     = 0;
    channel->isContinuing = 0;
    channel->isTrigger    = 0;
    channel->isConfirmed  = 0;
    channel->isFinished   = 0;

    for (p = 0; p < PICKER_TYPE_COUNT; p++)
    {
      if ((rc = ClearPickFilterTrigger(&channel->pick_series[p])) != EW_SUCCESS)
          return rc;
    }
   
    return EW_SUCCESS;
}
