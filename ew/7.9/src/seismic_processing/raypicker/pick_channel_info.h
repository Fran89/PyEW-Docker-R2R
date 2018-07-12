/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_channel_info.h 4475 2011-08-04 15:17:09Z kevin $
 *
 *    Revision history:
 *     $Log: pick_channel_info.h,v $
 *     Revision 1.2  2009/02/13 20:40:36  mark
 *     Added SCNL parsing library
 *
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2005/06/20 21:33:55  cjbryan
 *     don't pass debug to some fns
 *
 *     Revision 1.2  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * pick_channel_info.h
 * 
 * Declarations for structures and functions used to contain,
 * initialize and free resources used to track a single sample
 * channel used for picking.
 * 
 * Included in the channel persistence data is the relevant
 * pre-filter (info need to obtain the pseudo short-period
 * series), symmetry tracking, the picking series, and a buffer
 * of the current raw data series.
 * 
 * NOTE: See pick_station.h, which manages the channels by station;
 *       and pick_series.h, which manages the STA series and picking.
 * 
 * @author Ray Buland, FORTRAN, 23 August 1979
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh from the original FORTRAN
 */
 
#ifndef PICK_CHANNEL_INFO_H
#define PICK_CHANNEL_INFO_H

#include "raypicker.h"
#include "ray_trigger.h"       /* SERIES_DATA */
#include "symmetry.h"          /* SYMMETRY_TRACK_TYPE, SYMMETRY_CYCLES */
#include "channel_states.h"
#include "pick_params.h"       /*  PICKER_TYPE_COUNT, PICKER_AGGR_STATE  */
#include "pick_series.h"


/* Initializing value for the channel's last_time */
#define PICK_CHAN_INVALID_TIME  -1.0


/* ================================================================================
 * ================================================================================
 * Functions to manage the globally-shared and reused channel memory.
 * 
 * For each tracebuf message that arrives, various operations are performed.
 * Most of this data must be available throughout multi_trigger processing,
 * but it is not needed to persist afterwards.  Therefore, it may be
 * reused from buffer-to-buffer.
 * See the static variables at the top of pick_channel for those shared
 * buffers and variables.
 * 
 * They are called in this manner:
 * 
 * - At program startup: InitChannelGlobals() 
 * 
 * - When the maximum sample rate is known: AllocChannelGlobals()
 * 
 * - When data from an arriving tracebuf is ready to process: multi_trigger()
 *   (Note that "ready to process" has a particular meaning
 *   the very first time that data for a channel arrives, or
 *   after a gap.  In such instances, a longer amount of data
 *   is needed for initialization.  Generally this must be at
 *   least LTRLV (see ray_trigger.h) seconds.  Since a tracebuf
 *   won't contain that much, it usually takes around 2-3 messages
 *   to initialize the channel. This preparation is handled
 *   in msg_handler.cpp/h.
 * 
 * - On program termination: FreeChannelGlobals()
 */
 
void FreeChannelGlobals();
int AllocChannelGlobals(const double maxSampleRate);

/* ================================================================================
 * ================================================================================
 * Functions used to manage individual channel processing.
 */

int InitChannelInfo(PICK_CHANNEL_INFO *channelinfo, const double maxSampleRate);
int FreeChannelInfo(PICK_CHANNEL_INFO *channelinfo);

int SetChannelSampleRate(double traceSampleRate, RaypickerSCNL *thisScnl);
int InitChannelForSeries(double startTime, PICK_CHANNEL_INFO *channelinfo);
int ProcessChannelMessage(RaypickerSCNL *Scnl, RParams rParams, EWParameters EwParams);
int ClearChannelTriggers(PICK_CHANNEL_INFO *channel);

/*
 * =========================================================================
 * STATIC METHODS ONLY AVAILABLE IN pick_channel_info.c
 * =========================================================================
 */
static int PreFilterSample(PICK_CHANNEL_INFO *channel);
static int SetInitialValues(RaypickerSCNL *scnl);
static int STASymmetryEnvelope(RaypickerSCNL *scnl);
static int InitializeLTA(PICK_CHANNEL_INFO *channel);

#endif  /* PICK_CHANNEL_INFO_H */
