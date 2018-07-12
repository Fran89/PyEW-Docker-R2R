
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_ew.h,v 1.1 2000/02/14 19:06:49 lucky Exp $
 *
 *    Revision history:
 *		
 * 
 *     $Log: pick_ew.h,v $
 *
 *     Revision 1.3  2013/05/07  nromeu
 *     New parameters to PARM: MaxCodaLen, ProxiesFilt, Sensibility, Tau0Min, Tau0Inc, Tau0Max
 *     New structure: PROXIES with the variables to proxies computation
 * 
 *     Revision 1.2 2007/01/10 17:42:01 Núria Romeu
 *     Incorporació d'un nou camp a STATION per la reinicialització de LTA
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

/******************************************************************
 *                         File pick_ew.h                         *
 ******************************************************************/

/* Error bits
 ************/
#define PK_RESTART 1        /* Set when time series was broken, picker restarted */

/* Pick variables
   **************/
typedef struct {
   double time;             /* Pick time */
   double xpk[3];           /* Absolute value of first three extrema after ipic */
   char   FirstMotion;      /* First motion  ?=Not determined  U=Up  D=Down */
   int    weight;           /* Pick weight (0-3) */
   int    status;           /* Pick status :
                               0 = pick has been reported / scaning for event (picker in idle mode) 
                               1 = pick active but not complete (calculating)				
                               2 = pick is complete but not reported */						
} PICK;

/* Coda variables
   **************/
typedef struct {
   int    PickIndex;
   char   sta[6];           /* Station name */
   char   chan[4];          /* Component code */
   char   net[3];           /* Network code */
   int    aav[6];           /* aav of preferred windows */
   int    len_sec;          /* Coda length in seconds */
   int    len_out;          /* Coda length in seconds (sometimes * -1) */
   int    len_win;          /* Coda length in number of windows */
   int    status;           /* Coda status :
                               0 = coda has been reported / scaning for event (picker in idle mode) 
                               1 = coda active but not complete (calculating)				
                               2 = coda is complete but not reported */						
} CODA;



/* Proxies variables
   **************/
typedef struct {
   int    PickIndex;
   char   sta[6];           /* Station name */
   char   chan[4];          /* Component code */
   char   net[3];           /* Network code */
   double tau0;             /* Window of time to calculate the proxies, in seconds */
   double tauc;             /* Main period of the signal, in seconds */
   double pow_disp_noise;   /* Noise power of displacement, computation until trigger onset  */
   double pow_disp_signal;  /* Signal power of displacement, computation since trigger onset for each tau0 */
   double snr3s;			/* SNR at 3 seconds, signal to noise ration which decides if proxies are of enough good */
   double pd;               /* Maximum from displacement, in [cm] */
   int    status;           /* Proxy computation status :
                               0 = proxies is in idle mode 
							        or eabs is increasing for the first time => P pick
                               1 = eabs is decreasing for the first time => P pick is finishing
                               2 = eabs is increasing for the second time => S pick ?
							        time to complet the proxies
                               3 = proxies have been reported */
} PROXIES;


/* Picking parameters
   ******************/
typedef struct {
   int    Itr1;             /* Parameter used to calculate itrm */
   int    MinSmallZC;       /* Minimum number of small zero crossings */
   int    MinBigZC;         /* Minimum number of big zero crossings */
   long   MinPeakSize;      /* Minimum size of 1'st three peaks */
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
   // 
   int	  MaxCodaLen;		/* Maximum coda length in seconds */
   double ProxiesFilt;      /* Filter parameter for raw data to proxies computation*/
   double Sensibility;      /* Sensibility  */
   double Tau0Min;          /* Minimum TauO needed to report  */
   double Tau0Inc;          /* Increase in minimum TauO needed to report  */
   double Tau0Max;          /* Maximum TauO needed to report  */
   double PowDispFilt;      /* Filter parameter for running mean absolute value of power of displacement */
   double snrThreshold3s;	/* SNR at 3 seconds, signal to noise ration which decides if proxies are of enough good */
} PARM;

/* Station list parameters
   ***********************/
typedef struct {
   char   sta[6];           /* Station name */
   char   chan[4];          /* Component code */
   char   net[3];           /* Network code */
   CODA   Coda;             /* Coda structure */
   PICK   Pick;             /* Pick structure */
   PROXIES  Proxies;        /* Proxies structure */
   PARM   Parm;             /* Configuration file parameters */
   double cocrit;           /* Threshold at which to terminate coda measurement */
   double crtinc;           /* Increment added to ecrit at each zero crossing */
   double eabs;             /* Running mean absolute value (aav) of rdat */
   double ecrit;            /* Criterion level to determine if event is over */
   double elta;             /* Long-term average of edat */
   double old_elta;         /* Long-term average of edat at ONSET */  /***/ // Revision 1.2 Núria Romeu
   long   enddata;          /* Last data value of previous message */
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
   long   old_sample;       /* Old value of integer data */
   int    ns_restart;       /* Number of samples since restart */
   double rdat;             /* Filtered data value */
   double rbig;             /* Threshold for big zero crossings */
   double rlast;            /* Size of last big zero crossing */
   double rold;             /* Previous value of filtered data */
   double rsrdat;           /* Running sum of rdat in coda calculation */
   long   sarray[10];       /* First 10 points after pick for 1'st motion determ */
   double tmax;             /* Instantaneous maximum in current half cycle */
   long   xdot;             /* First difference at pick time */
   double xfrz;             /* Used in first motion calculation */
   /***/
   double acc;              /* Filtered acceleration to compute proxies. 
                                Only used with accelerometers */
   double old_acc;          /* Previous value of filtered acceleration. 
                                Only used with accelerometers */
   double vel;              /* Filtered velocity to compute proxies */
   double old_vel;          /* Previous value of filtered velocity */
   double disp_offset;		/* Filtered displacement to compute proxies */
   double old_disp_offset;	/* Previous value of filtered displacement */
   double disp;				/* Filtered displacement to compute proxies also filtered to erase the offset from the integration */
   double old_disp;			/* Previous value of filtered displacement also filtered to erase the offset from the integration */
   double pow_disp;			/* Power of displacement */
   double num;              /* Numerator used to calculate the proxy tauC */
   double den;              /* Denominator used to calculate the proxy tauC */
   double accum_eabs;       /* Accumulation of eabs to compute the best tau0 */
   double ave_eabs;         /* Average of eabs from the actual window */
   double old_ave_eabs;     /* Average of eabs from the last window */
   int    numeabs;             /* Proxy Tau0 index within window */
} STATION;

typedef struct {
   char StaFile[50];              /* Name of file with SCN info */
   long InKey;                    /* Key to ring where waveforms live */
   long OutKey;                   /* Key to ring where picks will live */
   int  HeartbeatInt;             /* Heartbeat interval in seconds */
   int  RestartLength;            /* Number of samples to process for restart */
   int  MaxGap;                   /* Maximum gap to interpolate */
   int  Debug;                    /* If 1, print debug messages */
   unsigned char MyModId;         /* Module id of this program */
   SHM_INFO InRegion;             /* Info structure for input region */
   SHM_INFO OutRegion;            /* Info structure for output region */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char GetThisInstId;   /* Get messages from this inst id */
   unsigned char GetThisModId;    /* Get messages from this module */
   unsigned char TypeHeartBeat;
   unsigned char TypeError;
   unsigned char TypePick2k;
   unsigned char TypeCoda2k;	/***/ // NR
   unsigned char TypeProxies;
   unsigned char TypeWaveform;    /* Waveform buffer for data input */
} EWH;
