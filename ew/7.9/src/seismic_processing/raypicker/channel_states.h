/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: channel_states.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: channel_states.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2004/07/16 19:27:21  cjbryan
 *     allowed continuation of a preliminary trigger
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * These are the states that a time series channel (pick_channel_info.h)
 * or a pick filter series derived from such a channel (sta_series.h)
 * can assume.
 * 
 * Neither channels nor filters can assume all of these states, but their
 * states are are so closely bound that it made sense to define the states in
 * the same file and in the same sequence.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 : August 2003, dbh
 */
#ifndef CHAN_STATES_H
#define CHAN_STATES_H

/* Channel only */
#define PKCH_STATE_BAD   -1  /* some problem prevents using this channel           */
#define PKCH_STATE_NEW    0  /* Never seen data before, initial state for channel  */
#define PKCH_STATE_WAIT   1  /* additional buffer needed for initial averaging     */
#define PKCH_STATE_INIT   2  /* First time into trigger processing for time series */

/*
 * For a channel, PKCH_STATE_LOOK means that time series initialization is complete
 *                and its filters are processing the series for triggers and amplitudes.
 *                The channel remains at this state unless a data gap is detected,
 *                in which case, it reverts to PKCH_STATE_NEW;
 *                or until a trigger is confirmed as a pick, then it goes to
 *                PKCH_STATE_NEWPICK until reported, then PKCH_STATE_PICKSENT
 *                before returning to PKCH_STATE_LOOK at shutdown.
 * 
 * For a filter, PKCH_STATE_LOOK is the initial state and means that no trigger is
 *               currently detected from the filter.
 */
#define PKCH_STATE_CONT   3    /* preliminary trigger turned off because signal level 
                                * too low, but before timing out                          */
#define PKCH_STATE_LOOK   4

/* Channel only */
#define PKCH_STATE_NEWPICK  5  /* first confirmed trigger for channel, need to send pick  */
#define PKCH_STATE_PICKSENT 6  /* pick already sent for trigger (prevents retransmission) */

/*
 * Trigger only
 * 
 * A channel contains aggregation variables that indicate the states of all of
 * its triggers to determine an overall state.  
 */
#define PKCH_STATE_PRELIM 5  /* preliminary trigger identified (see which_trigger), waiting for confirmation   */
#define PKCH_STATE_TRIG   6  /* trigger found, awaiting required persistence for confirmation                  */
#define PKCH_STATE_CONF   7  /* trigger confirmed, awaiting shutdown                                           */
#define PKCH_STATE_ENDED  8  /* trigger has shutdown, ready to report for this filter                          */


#endif  /*  CHAN_STATES_H  */
