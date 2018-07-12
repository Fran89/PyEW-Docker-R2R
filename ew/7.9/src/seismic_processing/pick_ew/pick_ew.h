
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_ew.h 5333 2013-01-31 17:53:14Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2008/03/28 18:20:31  paulf
 *     added in PickIndexDir option to specify where pick indexes get stuffed
 *
 *     Revision 1.6  2007/12/16 19:18:43  paulf
 *     fixed an improper use of long for 4 byte sample data, some OS have long as 8bytes.
 *
 *     Revision 1.5  2007/02/26 13:59:08  paulf
 *     no coda changes (option)
 *
 *     Revision 1.4  2006/09/20 22:44:07  dietz
 *     Modified to be able to process multiple "StaFile" commands for setting
 *     per-channel picking parameters.
 *
 *     Revision 1.3  2005/04/08 23:57:19  dietz
 *     Added new config command "GetLogo" so pick_ew can select which logos
 *     to process. If no "GetLogo" commands are included, the default behavior
 *     is to process all TYPE_TRACEBUF and TYPE_TRACEBUF2 messages in InRing.
 *
 *     Revision 1.2  2004/04/29 22:44:51  kohler
 *     Pick_ew now produces new TYPE_PICK_SCNL and TYPE_CODA_SCNL messages.
 *     The station list file now contains SCNLs, rather than SCNs.
 *     Input waveform messages may be of either TYPE_TRACEBUF or TYPE_TRACEBUF2.
 *     If the input waveform message is of TYPE_TRACEBUF (without a location code),
 *     the location code is assumed to be "--".  WMK 4/29/04
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

/******************************************************************
 *                         File pick_ew.h                         *
 ******************************************************************/

#define LINELEN 200         /* Size of char arrays to hold picks and codas */

/* Error bits
   **********/
#define PK_RESTART 1        /* Set when time series was broken, picker restarted */

/* Pick variables
   **************/
typedef struct {
   double time;             /* Pick time */
   double xpk[3];           /* Absolute value of first three extrema after ipic */
   char   FirstMotion;      /* First motion  ?=Not determined  U=Up  D=Down */
   int    weight;           /* Pick weight (0-3) */
   int    status;           /* Pick status :
                               0 = picker is in idle mode
                               1 = pick active but not complete
                               2 = pick is complete but not reported
                               3 = pick has been reported */
} PICK;

/* Coda variables
   **************/
typedef struct {
   int    PickIndex;
   char   sta[6];           /* Station name */
   char   chan[4];          /* Component code */
   char   net[3];           /* Network code */
   char   loc[3];           /* Location code */
   int    aav[6];           /* aav of preferred windows */
   int    len_sec;          /* Coda length in seconds */
   int    len_out;          /* Coda length in seconds (sometimes * -1) */
   int    len_win;          /* Coda length in number of windows */
   int    status;           /* Coda status :
                               0 = picker is in idle mode
                               1 = coda active but not complete
                               2 = coda is complete but not reported
                               3 = coda has been reported */
} CODA;

/* Picking parameters
   ******************/
typedef struct {
   int    Itr1;             /* Parameter used to calculate itrm */
   int    MinSmallZC;       /* Minimum number of small zero crossings */
   int    MinBigZC;         /* Minimum number of big zero crossings */
   int    MinPeakSize;      /* Minimum size of 1'st three peaks */
   int    MaxMint;          /* Max interval between zero crossings in samples */
   int    MinCodaLen;       /* Mininum coda length in seconds */
   double RawDataFilt;      /* Filter parameter for raw data */
   double CharFuncFilt;     /* Filter parameter for characteristic function */
   double StaFilt;          /* Filter parameter for short-term average */
   double LtaFilt;          /* Filter parameter for long-term average */
   double EventThresh;      /* STA/LTA event threshold */
   double RmavFilt;         /* Filter parameter for running mean absolute value */
   double DeadSta;          /* Dead station threshold */
   double CodaTerm;         /* Coda termination threshold (60 mV, in counts) */
   double AltCoda;          /* Frac of c8 at which alt coda termination used */
   double PreEvent;         /* Frac of pre-event level for alt coda termination */
   double Erefs;            /* Event termination parameter */
} PARM;

/* Station list parameters
   ***********************/
typedef struct {
   char   sta[6];           /* Station name */
   char   chan[4];          /* Component code */
   char   net[3];           /* Network code */
   char   loc[3];           /* Location code */
   CODA   Coda;             /* Coda structure */
   PICK   Pick;             /* Pick structure */
   PARM   Parm;             /* Configuration file parameters */
   double cocrit;           /* Threshold at which to terminate coda measurement */
   double crtinc;           /* Increment added to ecrit at each zero crossing */
   double eabs;             /* Running mean absolute value (aav) of rdat */
   double ecrit;            /* Criterion level to determine if event is over */
   double elta;             /* Long-term average of edat */
   int    enddata;          /* Last data value of previous message */
   double endtime;          /* Stop time of previous message */
   double eref;             /* STA/LTA reference level */
   double esta;             /* Short-term average of edat */
   int    evlen;            /* Event length in samp */
   int    first;            /* 1 the first time this channel is found */
   int    isml;             /* Small zero-crossing counter */
   int    k;                /* Index to array of windows to push onto stack */
   int    m;                /* 0 if no event; otherwise, zero-crossing counter */
   int    mint;             /* Interval between zero crossings in samples */
   int    ndrt;             /* Coda length index within window */
   int    next;             /* Counter of zero crossings early in P-phase */
   int    nzero;            /* Big zero-crossing counter */
   int    old_sample;       /* Old value of integer data */
   int    ns_restart;       /* Number of samples since restart */
   double rdat;             /* Filtered data value */
   double rbig;             /* Threshold for big zero crossings */
   double rlast;            /* Size of last big zero crossing */
   double rold;             /* Previous value of filtered data */
   double rsrdat;           /* Running sum of rdat in coda calculation */
   int    sarray[10];       /* First 10 points after pick for 1'st motion determ */
   double tmax;             /* Instantaneous maximum in current half cycle */
   int    xdot;             /* First difference at pick time */
   double xfrz;             /* Used in first motion calculation */
} STATION;

#define STAFILE_LEN 64
typedef struct {
   char   name[STAFILE_LEN]; /* Name of station file */
   int    nsta;              /* number of channels configure in this file */
} STAFILE;

typedef struct {
   STAFILE  *StaFile;       /* Name of file(s) with SCNL info */
   char *PickIndexDir;      /* an optional directory to place pick index files, to get them out of the param dir */
   int       nStaFile;      /* Number of StaFile commands given */
   long      InKey;         /* Key to ring where waveforms live */
   long      OutKey;        /* Key to ring where picks will live */
   int       HeartbeatInt;  /* Heartbeat interval in seconds */
   int       RestartLength; /* Number of samples to process for restart */
   int       MaxGap;        /* Maximum gap to interpolate */
   int       Debug;         /* If 1, print debug messages */
   int       NoCoda;        /* If 1, just do picks, no coda's */
   int       NoCodaHorizontal;        /* If 1, just do coda's on vertical (Z) components */
   unsigned char MyModId;   /* Module id of this program */
   SHM_INFO  InRegion;      /* Info structure for input region */
   SHM_INFO  OutRegion;     /* Info structure for output region */
   int       nGetLogo;      /* Number of logos in GetLogo   */
   MSG_LOGO *GetLogo;       /* Logos of requested waveforms */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char InstIdWild;      /* Wildcard for inst id */
   unsigned char ModIdWild;       /* Wildcard for module id */
   unsigned char TypeHeartBeat;
   unsigned char TypeError;
   unsigned char TypePickScnl;
   unsigned char TypeCodaScnl;
   unsigned char TypeTracebuf;    /* Waveform buffer for data input (no loc code) */
   unsigned char TypeTracebuf2;   /* Waveform buffer for data input (w/loc code) */
} EWH;
