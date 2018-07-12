/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_data.h 1248 2003-06-16 22:08:11Z patton $
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

/* Additions made to FMT* macros: Pete Lombard, 2/4/2000 */
/*
 *	
 *	Misc structures used in the data block
 *	
 */

typedef struct	net_time {
	UDCC_WORD	year;
	UDCC_WORD	day;
	UDCC_BYTE	hour;
	UDCC_BYTE	minute;
	UDCC_BYTE	seconds;
	UDCC_BYTE	dummy;
	UDCC_WORD	fracs;		/* Number of .0001 seconds */
} NETTIME;

typedef UDCC_LONG NETFLT;

/*
 *	
 *	Fixed portion of the data block
 *	
 */

typedef struct _fixed_block {
	UDCC_BYTE	Seq_ID[6];		/* Sequence number of record */
	UDCC_BYTE	Record_Type;		/* Always a 'D' */
	UDCC_BYTE	Filler;			/* Always a space */

	UDCC_BYTE	Station_ID[5];		/* Station id (space filled) */
	UDCC_BYTE	Location_ID[2];		/* Array/Extended Station (filled) */
	UDCC_BYTE	Channel_ID[3];		/* Channel Id (space filled) */
	UDCC_BYTE	Network_ID[2];		/* Extended Network Type */

	NETTIME	Start_Time;		/* Start time of record */
	UDCC_WORD	Number_Samps;		/* Number of samples in record */

	DCC_WORD	Rate_Factor;		/* Sample rate factor */
	DCC_WORD	Rate_Mult;		/* Rate Multiplier */

	UDCC_BYTE	Activity_Flags;		/* Activity Information */

#define ACTFLAG_CALSIG 0x01		/* Calibration signals this record */
#define ACTFLAG_CLKFIX 0x02		/* Error caused by clock correction */
#define ACTFLAG_BEGEVT 0x04		/* Beginning of event */
#define ACTFLAG_ENDEVT 0x08		/* End of event */

	UDCC_BYTE	IO_Flags;		/* I/O Information */

#define IOFLAG_ORGPAR 0x01		/* Original tape had parity error */
#define IOFLAG_LONGRC 0x02		/* Original read long record */
#define IOFLAG_SHORTR 0x04		/* Original had short record */

	UDCC_BYTE	Qual_Flags;		/* Data Quality Information */

#define QULFLAG_AMPSAT 0x01		/* Amplifier saturation detected */
#define QULFLAG_SIGCLP 0x02		/* Signal clipping detected */
#define QULFLAG_SPIKES 0x04		/* Signal spiking detected */
#define QULFLAG_GLITCH 0x08		/* Signal glitch detected */
#define QULFLAG_PADDED 0x10		/* Data missing or padded */
#define QULFLAG_TLMSNC 0x20		/* Telemetry sync error/dropout */
#define QULFLAG_CHARGE 0x40		/* Digitial filter charging */
#define QULFLAG_TIMERR 0x80		/* Time tag questionable */

	UDCC_BYTE	Total_Blockettes;	/* Number blockettes to follow */
	DCC_LONG	Time_Correction;	/* Number of .0001 sec correction */
	UDCC_WORD	Data_Start;		/* Byte where data starts */
	UDCC_WORD	First_Blockette;	/* Byte of first blockette */
} SEED_DATA;


/*
 *
 *     Sample Rate Blockette
 * 
 */

#define BLK_SAMRATE 100
struct samp_rate {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	NETFLT  Samp_Rate;              /* Actual sample rate */

	UDCC_BYTE   Flags;                  /* Flags - reserved */
	UDCC_BYTE   Resrv1;
};
#define SAMRATE_SZ (sizeof (struct samp_rate))


/*
 *	
 *	Generic Event Detection Blockette [200]
 *	
 */

#define BLK_EVTDET 200
struct event_detect {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	NETFLT	Signal_Amplitude;	/* Event signal amplitude */
	NETFLT	Signal_Period;		/* Period of signal */
	NETFLT	Background_Estimate;	/* Estimation of background */

	UDCC_BYTE	Event_Flags;		/* Event detection flags */

#define EVENT_DILAT 0x01	/* Set is dilatation, unset is compression */
#define EVENT_DECON 0x02	/* When set results are deconvolved, if unset
					results are digital counts */

	UDCC_BYTE	Rsrvd[1];		/* Reserved (must be 0) */

	NETTIME	Onset_Time;		/* Onset Time of Event */
};
#define EVTDET_SZ (sizeof (struct event_detect))

/*
 *	
 *	Murdock Event Detection Blockette [201]
 *	
 */

#define BLK_MURDET 201

struct murdock_detect {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	NETFLT	Signal_Amplitude;	/* Event signal amplitude */
	NETFLT	Signal_Period;		/* Period of signal */
	NETFLT	Background_Estimate;	/* Estimation of background */

	UDCC_BYTE	Event_Flags;		/* Event detection flags */

#define MEVENT_DILAT 0x01	/* Set is dilatation, unset is compression */

	UDCC_BYTE	Rsrvd[1];		/* Reserved (must be 0) */

	NETTIME	Onset_Time;		/* Onset Time of Event */
	UDCC_BYTE	SNR_Qual[6];		/* Quality values */

	UDCC_BYTE	Lookback;		/* Lookback type */
	UDCC_BYTE	Pick;			/* Pick algorithm */

};
#define MURDET_SZ (sizeof (struct murdock_detect))

/*
 *	
 *	Step Cal Blockette [300]
 *	
 */

#define BLK_STEPCAL 300
struct step_cal {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	UDCC_WORD	Cal_In_Chan;		/* Channel ID containing cal input 
						zero denotes none */
	NETTIME	First_Cal;		/* Time of first cal in sequence 
					   (might be in previous records) */
	UDCC_LONG	Duration;		/* Number of .1ms step duration */
	UDCC_LONG	Interval;		/* Number of .1ms tween step on times */
	NETFLT	Amplitude;		/* Amp of signal in units */
	UDCC_BYTE	Number_Steps;		/* Number of cal steps in sequence */

	UDCC_BYTE	Cal_Flags;		/* Cal information */

#define STEP_POSITI 0x01	/* First step was positive */
#define STEP_ALTERN 0x02	/* Steps alternate sign */
#define STEP_AUTOMA 0x04	/* Cal was automatic, otherwise manual */
#define STEP_CONTIN 0x08	/* Cal begin in a previous record */

	UDCC_BYTE	Reserved[2];
};
#define STEPCAL_SZ (sizeof (struct step_cal))

/*
 *	
 *	Sine Wave Cal Blockette [310]
 *	
 */

#define BLK_SINECAL 310
struct sine_cal {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	UDCC_WORD	Cal_In_Chan;		/* Channel ID containing cal input 
						zero denotes none */
	NETTIME	Cal_Start;		/* Time when cal begins
					   (might be in previous records) */
	UDCC_LONG	Duration;		/* Number of .1ms cal duration */

	NETFLT	Period;			/* Sine period in seconds */
	NETFLT	Amplitude;		/* Amp of cal sig in units */

	UDCC_BYTE	Sine_Flags;		/* Sine cal flags */

#define SINE_AUTOMA 0x04	/* Cal automatic, otherwise manual */
#define SINE_CONTIN 0x08	/* Cal began in a previous record */
#define SINE_PKTOPK 0x10	/* Peak to peak amplitude */
#define SINE_ZRTOPK 0x20	/* Zero to peak amplitude */
#define SINE_RMSAMP 0x40	/* RMS amplitude */

	UDCC_BYTE	Reserved[3];
};
#define SINECAL_SZ (sizeof (struct sine_cal))

/*
 *	
 *	Pseudo Random Cal Blockette [320]
 *	
 */

#define BLK_RANDOMCAL 320
struct random_cal {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	UDCC_WORD	Cal_In_Chan;		/* Channel ID containing cal input 
						must be present */
	NETTIME	Cal_Start;		/* Time when cal begins
					   (might be in previous records) */
	UDCC_LONG	Duration;		/* Number of .1ms cal duration */
	NETFLT	Amplitude;		/* Amplitude of steps in units */

	UDCC_BYTE	Random_Flags;		/* Cal Flags */

#define RANDOM_AUTOMA 0x04	/* Automatic cal, otherwise manual */
#define RANDOM_CONTIN 0x08	/* Cal began in previous record */
#define RANDOM_RANAMP 0x10	/* Amps are random too, see cal in chan */

	UDCC_BYTE	Reserved[3];
};
#define RANDOMCAL_SZ (sizeof (struct random_cal))

/*
 *	
 *	Generic Cal Blockette [390]
 *	
 */

#define BLK_GENCALCAL 390
struct Generic_cal {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	UDCC_WORD	Cal_In_Chan;		/* Channel ID containing cal input 
						must be present */
	NETTIME	Cal_Start;		/* Time when cal begins
					   (might be in previous records) */
	UDCC_LONG	Duration;		/* Number of .1ms cal duration */
	NETFLT	Amplitude;		/* Amplitude of steps in units */

	UDCC_BYTE	Gencal_Flags;		/* Cal Flags */

#define GENCAL_AUTOMA 0x04	/* Automatic cal, otherwise manual */
#define GENCAL_CONTIN 0x08	/* Cal began in previous record */

	UDCC_BYTE	Reserved[3];
};
#define GENCALCAL_SZ (sizeof (struct Generic_cal))

/*
 *
 *      Data Only Blockette [1000]
 * 
 */

#define BLK_DATAONLY 1000
struct Data_only {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	UDCC_BYTE	Encoding;
	UDCC_BYTE   Order;
	UDCC_BYTE   Length;
	UDCC_BYTE   Resv1;
};
#define DATAONLY_SZ (sizeof (struct Data_only))

#define FMT_ASCII       0   /* Changed from LOG: PNL, 2/4/2000 */
#define	FMT_16_WORD	1
#define	FMT_24_WORD	2
#define	FMT_32_WORD	3
#define	FMT_IEEE_SP	4
#define	FMT_IEEE_DP	5
#define	FMT_STEIM_1	10
#define	FMT_STEIM_2	11
#define	FMT_GEOSCOPE_MPX24	12
#define	FMT_GEOSCOPE_16_3	13
#define	FMT_GEOSCOPE_16_4	14
#define	FMT_USNSN	15
#define	FMT_CDSN	16
#define	FMT_GRAEF	17
#define	FMT_IPG	        18
#define FMT_STEIM_3     19  /* Added PNL, 2/4/2000 */
#define	FMT_SRO_ASRO	30
#define	FMT_HGLP	31
#define	FMT_DWWSSN	32
#define	FMT_RSTN	33

/*
 *
 *      Quanterra Data Extension Blockette [1001]
 * 
 */

#define BLK_DATAEXT 1001
struct Data_ext {
	UDCC_WORD	Blockette_Type;		/* Blockette identifier */
	UDCC_WORD	Next_Begin;		/* Byte where next blockette begins */

	UDCC_BYTE	Timing;
	DCC_BYTE   Usec;
	UDCC_BYTE   Resv1;
	UDCC_BYTE   Frames;
};
#define DATAEXT_SZ (sizeof (struct Data_ext))

#include "seed_data_proto.h"
