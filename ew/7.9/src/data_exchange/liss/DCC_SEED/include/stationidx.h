/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: stationidx.h 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:04:58  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

#ifndef DEF_STATIONIDX
#define DEF_STATIONIDX

#ifndef TMLB_DEFINED
#include <dcc_time.h>
#endif

struct	station_index	{

	char	SI_station[5];		/* Station Code (SF) */
	char	SI_location[2];		/* Location Code (SF) */
	char	SI_channel[3];		/* Channel Code (SF) */

	UDCC_BYTE	SI_format_subclass;	/* Subcode for data format */

	STDTIME	SI_record_start_time;	/* Record starting time */
	STDTIME	SI_record_end_time;	/* Record ending time */

	UDCC_LONG	SI_sample_rate;		/* Sample rate * 10000 */
	UDCC_WORD	SI_number_samples;	/* Number of samples this record */

	UDCC_BYTE	SI_number_channels;	/* The number of channels 1-16 */

	DSKREC	SI_seed_blockette;	/* Pointer into blockette file */

	DCC_WORD	SI_time_correction;	/* delta change in milliseconds +/- */

	UDCC_LONG	SI_status_flags;	/* Flags for this record */

	DSKREC	SI_station_record;	/* Pointer to station record */

};

#define CORR_TIME  0x0001		/* Time is corrected */
#define CORR_DEL   0x0002		/* Record is deleted */
#define CORR_OUTS  0x0004		/* Record is out of sequence */
#define CORR_EMPTY 0x0008		/* Record contains empty space */
#define CORR_HARDE 0x0010		/* Hard parity error */
#define CORR_SHORT 0x0020		/* Short record (padded) */
#define CORR_LONG  0x0040		/* Long record (truncated) */
#define CORR_BFOR  0x0080		/* Header contains bad bits */
#define CORR_BTIM  0x0100		/* Bad time fields in header */
#define CORR_BEVEN 0x0200		/* Beginning of an event here */
#define CORR_EEVEN 0x0400		/* End of an event here */
#define CORR_CALI  0x0800		/* Calibration present */
#define CORR_NOACC 0x1000		/* No time accounting - log channel */
#define CORR_TEAR  0x2000		/* Time Correction is for a tear */

#endif
