DROP TABLE IF EXISTS `eb_picktwc`;
CREATE TABLE `eb_picktwc` (
  `id`               BIGINT   NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
  `fk_module`        BIGINT   NOT NULL COMMENT 'Foreign key to ew_module',
  `fk_scnl`          BIGINT   NOT NULL COMMENT 'Foreign key to ew_scnl',

  `lPickIndex`       BIGINT       DEFAULT NULL COMMENT 'Pick index; 0-10000->pick_wcatwc, 10000-20000
                                 ->develo; 20000-30000->hypo_display source',
  `iUseMe`           BIGINT       DEFAULT NULL COMMENT '2->This pick will not be removed by FindBadPs,
                                 1->Use this P in location; 0->Don t (auto KO)
                                 -1->Don t (manually knocked out)',
  `dPTime_dt`        DATETIME    DEFAULT NULL COMMENT 'P-time - DATETIME from dPTime',
  `dPTime_usec`      INT         DEFAULT NULL COMMENT 'P-time - usec from dPTime',
  `cFirstMotion`     CHAR(1)     DEFAULT NULL COMMENT '?=unknown, U=up, D=down',
  `szPhase`          VARCHAR(10) DEFAULT NULL COMMENT 'Pick Phase',
  `dMbAmpGM`         DOUBLE      DEFAULT NULL COMMENT 'Mb amplitude (ground motion in nm)',
  `dMbPer`           DOUBLE      DEFAULT NULL COMMENT 'Mb Per data, per of lMbAmp (sec)',
  `dMbTime_dt`       DATETIME    DEFAULT NULL COMMENT 'time at end of Mb T/A - DATETIME from dMbTime',
  `dMbTime_usec`     INT         DEFAULT NULL COMMENT 'time at end of Mb T/A - usec from dMbTime',
  `dMlAmpGM`         DOUBLE      DEFAULT NULL COMMENT 'Ml amplitude (ground motion in nm)',
  `dMlPer`           DOUBLE      DEFAULT NULL COMMENT 'Ml Per data, per of lMlAmp (sec)',
  `dMlTime_dt`       DATETIME    DEFAULT NULL COMMENT 'time at end of Ml T/A - DATETIME from dMlTime',
  `dMlTime_usec`     INT         DEFAULT NULL COMMENT 'time at end of Ml T/A - usec from dMlTime',
  `dMSAmpGM`         DOUBLE      DEFAULT NULL COMMENT 'MS amplitude (ground motion in um)',
  `dMSPer`           DOUBLE      DEFAULT NULL COMMENT 'MS Per data, per of lMSAmp (sec)',
  `dMSTime_dt`       DATETIME    DEFAULT NULL COMMENT 'time at end of MS T/A - DATETIME from dMSTime',
  `dMSTime_usec`     INT         DEFAULT NULL COMMENT 'time at end of MS T/A - usec from dMSTime',
  `dMwpIntDisp`      DOUBLE      DEFAULT NULL COMMENT 'Maximum integrated disp. peak-to-peak amp',
  `dMwpTime`         DOUBLE      DEFAULT NULL COMMENT 'Mwp window time in seconds',
  `szHypoID`         VARCHAR(33) DEFAULT NULL COMMENT 'If pick made in hypo_display, this is the
                                 associated hypocenter ID',
  `dPStrength`       DOUBLE      DEFAULT NULL COMMENT 'Ratio of P motion to background',
  `dFreq`            DOUBLE      DEFAULT NULL COMMENT 'Dominant frequency of this P-pick',

  `modified`         TIMESTAMP   DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',            
  PRIMARY KEY (`id`),
  FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  FOREIGN KEY (`fk_scnl`)   REFERENCES ew_scnl(`id`)   ON DELETE CASCADE ON UPDATE CASCADE,
  INDEX (`lPickIndex`),
  INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'TYPE_PICKTWC';

-- typedef struct {              /* STATION - station parameters and variables */
--    double  dLat;              /* Station geographic latitude (+=N, -=S) */
--    double  dLon;              /* Station geographic longitude (+=E, -=W) */
--    double  dCoslat;           /* These cos/sines are only used where the above*/
--    double  dSinlat;           /*  are in geocentric, which is only in the */
--    double  dCoslon;           /*  near station lookup table determination */
--    double  dSinlon;  
--    char    cFirstMotion;      /* ?=unknown, U=up, D=down */
--    double  dAlarmAmp;         /* Signal amplitude to exceedin m/s */
--    double  dAlarmDur;         /* Duration (sec) signal must exceed dAlarmDur */
--    double  dAlarmLastSamp;    /* Last samp to exceed threshold in Alarm */
--    double  dAlarmLastTriggerTime;/* Time when alarm was last triggered for
--                                     this station */
--    double  dAlarmMinFreq;     /* In hertz; low frequency limit condition */
--    double  dAlarmSamp;        /* Alarm sample value in m/s */
--    double  dAmp0;             /* Response scaling factor from calibs */
--    double  dAvAmp;            /* Average signal amp per 12 cycle */
--    double  dAveFiltNoise;     /* Moving average of RMS (filtered) */
--    double  dAveLDC;           /* Moving average of filtered DC offset */
--    double  dAveLDCRaw;        /* Moving average of unfilteredDC offset */
--    double  dAveLDCRawOrig;    /* Moving average of unfilteredDC offset when
--                                  Phase 1 passed */
--    double  dAveLTA;           /* Moving average of average signal amplitude */
--    double  dAveMDF;           /* Moving average of MDF */
--    double  dAveMDFOrig;       /* Moving avg of MDF when Phase 1 was passed */
--    double  dAveRawNoise;      /* Moving average of RMS */
--    double  dAveRawNoiseOrig;  /* Moving avg of RMS when Phase 1 was passed */
--    double  dAzimuth;          /* Epicenter/station azimuth */
--    double  dClipLevel;        /* Max Counts which signal can attain */
--    double  dCooze;            /* Variable used in locations */
--    double  dDataEndTime;      /* 1/1/70s of last non-zero data */
--    double  dDataStartTime;    /* 1/1/70s of 1st data != 0 */
--    double  dDelta;            /* Epicentral distance in degrees */
--    double  dElevation;        /* Station elevation in meters */
--    double  dEndTime;          /* Time at end of last packet (1/1/70 seconds) */
--    double  dExpectedPTime;    /* Expected P-time (1/1/70) for hypo in dummy */
--    double  dFiltX1[MAX_FILT_SECTIONS];   /* Saved data for filter */ 
--    double  dFiltX2[MAX_FILT_SECTIONS];   /* Saved data for filter */ 
--    double  dFiltY1[MAX_FILT_SECTIONS];   /* Saved data for filter */ 
--    double  dFiltY2[MAX_FILT_SECTIONS];   /* Saved data for filter */ 
--    double  dFiltX1LP[MAX_FILT_SECTIONS]; /* Saved data for filter */ 
--    double  dFiltX2LP[MAX_FILT_SECTIONS]; /* Saved data for filter */ 
--    double  dFiltY1LP[MAX_FILT_SECTIONS]; /* Saved data for filter */ 
--    double  dFiltY2LP[MAX_FILT_SECTIONS]; /* Saved data for filter */ 
--    double  dFracDelta;        /* Fractional part of epi. distance */
--    double  dFreq;             /* Dominant frequency of this P-pick */
--    double  dGainCalibration;  /* Factor to converts SW cal amplitude to gain */
--    double  dLTAThresh;        /* Phase 2 ampltude threshold */
--    double  dLTAThreshOrig;    /* dLTAThresh at time Phase 1 first passed */
--    double  dMaxD;             /* Maximum displacement value in signal */
--    double  dMaxID;            /* Max integrated displacement value in signal */
--    double  dMaxV;             /* Maximum velocity value in signal */
--    double  dMbAmpGM;          /* Mb amplitude (ground motion in nm) */
--    double  dMbMag;            /* Mb magnitude */
--    double  dMbPer;            /* Mb Per data, per of lMbAmp (sec) */
--    double  dMbTime;           /* 1/1/70 time (sec) at end of Mb T/A */
--    double  dMDFThresh;        /* MDF to exceed to pass Phase 1 */
--    double  dMDFThreshOrig;    /* dMDFThresh at time Phase 1 first passed */
--    double  dMlAmpGM;          /* Ml amplitude (ground motion in nm) */
--    double  dMlMag;            /* Ml magnitude */
--    double  dMlPer;            /* Ml Per data, per of lMlAmp (sec) */
--    double  dMlTime;           /* 1/1/70 time (sec) at end of Ml T/A */
--    double  dMSAmpGM;          /* MS amplitude (ground motion in um) */
--    double  dMSMag;            /* MS magnitude */
--    double  dMSPer;            /* MS Per data, per of lMSAmp (sec) */
--    double  dMSTime;           /* 1/1/70 time (sec) at end of MS T/A */
--    double  dMwpIntDisp;       /* Maximum integrated disp. peak-to-peak amp */
--    double  dMwpMag;           /* Mwp magnitude */
--    double  dMwpTime;          /* Mwp window time in seconds */
--    double  dMwAmpGM;          /* Mw amplitude (ground motion in um) */
--    double  dMwMag;            /* Mw magnitude */
--    double  dMwMagBG;          /* Mw magnitude based on background data */
--    double  dMwPer;            /* Mw Per data */
--    double  dMwTime;           /* 1/1/70 time (sec) at end of Mw T/A */
--    double  dMwAmpSp[MAX_SPECTRA];   /* Spectral amplitude */
--    double  dMwAmpSpBG[MAX_SPECTRA]; /* Background Spectral amplitude */
--    double  dMwMagSp[MAX_SPECTRA];   /* Spectral magnitude */
--    double  dMwMagSpBG[MAX_SPECTRA]; /* Background Spectral magnitude */
--    double  dMwPerSp[MAX_SPECTRA];   /* Spectral period */
--    double  dPEndTime;         /* End time of P wave window (1/1/70) */
--    double  dPerMax;           /* Period of the maximum Mm */
--    double  dPhaseTime;        /* Time (1/1/70 seconds) of picked phase */
--    double  dPhaseTT[MAX_PHASES]; /* IASPEI91 Phase travel time (since o-time) */
--    double  dPhaseTimeIasp[MAX_PHASES];/* 1/1/70 seconds time of Iaspei phase */
--    double  dPStartTime;       /* Start time of P wave window (1/1/70)*/
--    double  dPTravTime;        /* P wave travel time (in seconds) */
--    double  dPStrength;        /* Ratio of P motion to background */
--    double  dPTime;            /* P-time in seconds from 1/1/70 */
--    double  dRes;              /* Location residual for this station */
--    double  dResidualWeights;  /* Weighting factors for residual */
--    double  dREndTime;         /* End time of Rayleigh wave window (1/1/70) */
--    double  dRStartTime;       /* Start time of Rayleigh wave window (1/1/70) */
--    double  dRTravTime;        /* Rayleigh wave travel time (in seconds) */
--    double  dSampRate;         /* Sample rate in samps/sec */
--    double  dScaleFactor;      /* Empirical factor for screen trace scaling */
--    double  dScreenLat[MAX_SCREENS][2];/* Lat bounds of screen's stns. */
--    double  dScreenLon[MAX_SCREENS][2];/* Lon bounds of screen's stns. */
--    double  dScreenStart;      /* Time (1/1/70s) at trace start on screen */
--    double  dSens;             /* Station sensitivity at 1 Hz in cts/m/s */
--    double  dSnooze;           /* Variable used in locations */
--    double  dStartTime;        /* 1/1/70 second at index = 0 (in ANALYZE: oldest
--                                  time in buffer) */
--    double  dSumLDC;           /* Accumulator for average DC amplitude */
--    double  dSumLDCRaw;        /* Accumulator for average, unfiltered DC amp */
--    double  dSumLTA;           /* Accumulator for average signal amplitude */
--    double  dSumMDF;           /* Accumulator for MDF summation */
--    double  dSumRawNoise;      /* Accumulator for RMS summation */
--    double  dThetaEnergy;      /* Energy release calculated from this station */   
--    double  dThetaMoment;      /* Moment calculated from this stn for Theta */   
--    double  dTheta;            /* Theta for this station */   
--    double  dTimeCorrection;   /* Transmission time delay (sec) to subtract
--                                  from data */
--    double  dTrigTime;         /* 1/1/70 time (sec) that Phase1 was passed */
--    double  dVScale;           /* Vertical scaling factor */
--    int     iAgency;           /* Agency indicator (not used after 5/08) */
--    int     iAhead;            /* 1->data ahead of present; 0->data ok */
--    int     iAlarm;            /* 0->no alarm, 1->seismic alarm activated */								 
--    int     iAlarmStatus;      /* 1=Initialize digital alarm variables, 2=Process
--                                  data in alarm, 3=Alarm declared, 0=No alarms*/
--    int     iBin;              /* Associator bin for this pick */
--    int     iBytePerSamp;      /* # bytes/sample - normally 4 */
--    int     iCal;              /* 1 -> pick looks like calibration */
--    int     iClipIt;           /* 1 -> Clip display amplitude; 0 -> don't */
--    int     iComputeMwp;       /* 1=use this stn for Mwp, 0=don't */
--    int     iDisplayStatus;    /* 0=toggled off, 1=on, 2=always off */
--    int     iFirst;            /* 1=First packet for this station */
--    int     iFiltStatus;       /* 1=Run data through SP filter, 0=don't */
--    int     iHasWrapped;       /* 1 -> enough data recorded to fill buffer */
--    int     iLPAlarmSent;      /* 1 -> LP Alarm for this station recently sent */ 
--    int     iMbClip;           /* 1 if Clipped; 0 if not */       
--    int     iMlClip;           /* 1 if Clipped; 0 if not */      
--    int     iMSClip;           /* 1 if Clipped; 0 if not */
--    int     iMwClip;           /* 1 if Clipped; 0 if not */
--    int     iMwNumPers;        /* Number of frequencies with amps (Mm) */
--    int     iNearbyStnArray[5];/* Array of 5 closest stations */
--    int     iNPole;            /* # poles in response function */
--    int     iNumPhases;        /* # phases at this station */
--    int     iNZero;            /* # zeros in response function */
--    int     iPickCnt;          /* Pick Counter with locate buffer */
--    int     iPickStatus;       /* 0=don't pick, 1=initialize, 2=pick it
--                                  3=it's been picked, get mags */
--    int     iSignalToNoise;    /* S:N ratio to exceed for P-pick */
--    int     iStationDisp[MAX_SCREENS]; /* Flags to show if this stn on screen */
--    int     iStationSortIndex; /* Used by the qsort to obtain the original
--                                  station lineup dln 2/13/10 */
--    int     iStationType;      /* Model of seismometer:
--                                  1 = STS1     360s
--                                  2 = STS2     130s
--                                  3 = CMG-3NSN 30s
--                                  4 = CMG-3T   100s
--                                  5 = KS360i   360s
--                                  6 = KS5400   350s
--                                  7 = CMG-3    30s
--                                  8 = CMG-40T  30s
--                                  9 = CMG3TNSN 30s 
--                                  10 = KS-10   20s
--                                  11 = CMG3ESP_30  30s
--                                  12 = CMG3ESP_60  60s
-- 				 13 = Trillium 40 40s 
-- 				 14 = CMG3ESP_120 (100) 120s 
-- 				 15 = CMG40T_20   20s 
-- 				 16 = CMG3T_360  360s 
-- 				 17 = KS2000_120 120s 
-- 				 18 = CMG 6TD    30s 
-- 				 19 = Trillium 120 120s 
-- 				 20 = Trillium 240 240s 
-- 				 29 = unknown broadband (no cal) 
-- 				 30 = EpiSensor FBA ES-T 
-- 				 31 = Guralp-5T* 
-- 				 40 = GT_S13 
-- 				 41 = MP_L4 
-- 				 42 = Ranger SS1 
--                                  50 = generic LP (no cal)
--                                  51 = ATWC LP Response (Hi gain)
--                                  52 = ATWC LP Response (Low gain)
--                                  100 = Generic SP (no cal)
--                                  101 = ATWC SP Response (Hi gain)
--                                  102 = ATWC SP Response (Medium gain)
--                                  103 = ATWC SP Response (Low gain)
--                                  999 = Not determined */
--    int     iTrigger;          /* 1=triggered data, 0=continuous */
--    int     iUseMe;            /* 2->This pick will not be removed by FindBadPs,
--                                  1->Use this P in location; 0->Don't (auto KO)
--                                  -1->Don't (manually knocked out) */
--    long    lAlarmCycs;        /* # samples per half cycle */
--    long    lAlarmP1;          /* Alarm Phase 1 pass flag */
--    long    lAlarmSamps;       /* # samples ctr while alarm threshold exceeded */
--    long    lCurSign;          /* Sign of current MDF for Phase 3 */
--    long    lCycCnt;           /* Cycle ctr (if T/A in first MbCycles, 
--                                  this is associated with Mb magnitude) */
--    long    lCycCntLTA;        /* Cycle counter for LTAs */
--    long    lEndData;          /* Data at last point of previous buffer */								 
--    long    lFiltSamps;        /* Number of samples processed by filter per 
--                                  sequence */
--    int     lFirstMotionCtr;   /* Number of samples checked so far */
--    long    lHit;              /* Number of hits counter for Phases 2 & 3 */
--    long    lIndex;            /* Next index to write sample (on read, index of
-- 				 dStartTime) - ANALYZE */
--    long    lIndexToStartWrite;/* Used in ReadDiskNew - Index to write to */
--    long    lLastSign;         /* Sign of last MDF for Phase 3 */
--    long    lLTACtr;           /* Long term averages counter */
--    long    lMagAmp;           /* Summation of 1/2 cycle amplitudes */
--    double  dMaxPk;            /* Amp (p-p nm) for use in magnitude comp. */
--    long    lMbPer;            /* Mb Per data, per of dMbAmpGM doubled in sec*10 */
--    long    lMDFCnt;           /* Counter of cycles used in MDFTotal */
--    long    lMDFNew;           /* Present MDF value */
--    long    lMDFOld;           /* Last MDF value */
--    long    lMDFRunning;       /* Running total of sample differences */
--    long    lMDFRunningLTA;    /* Running total of sample differences for LTAs */
--    long    lMDFTotal;         /* Total of MDFs over several cycles */
--    long    lMis;              /* Num. of misses count for Phases 2 & 3 */
--    long    lMlPer;            /* Ml Per data, per of dMlAmpGM doubled in sec*10 */
--    long    lMwpCtr;           /* Index which counts samples from P for Mwp */
--    long    lNumOsc;           /* # of osc. counter for Phase 3 */
--    long    lNumToFilter;      /* Number of samples read in for partial read */
--    long    lPer;              /* Temporary period array - in seconds*10 */
--    long    lPhase1;           /* Phase 1 passed flag */
--    long    lPhase2;           /* Phase 2 passed flag */
--    long    lPhase3;           /* Phase 3 passed flag */
--    long    lPhase2Cnt;        /* Sample counter for timing Phase 2 */
--    long    lPhase3Cnt;        /* Sample counter for timing Phase 3 */
--    long    lPickBufIndex;     /* Present index of P adjustment buffer */
--    long    lPickBufRawIndex;  /* Present index of Neural net buffer */
--    long    lPickIndex;        /* Pick index; 0-10000->pick_wcatwc, 10000-20000
--                                  ->develo; 20000-30000->hypo_display source*/
--    long    lRawCircCtr;       /* Index counter for raw-data, circ buff */
--    long    lRawCircSize;      /* Size (# samples) in plRawCircBuff */
--    long    lRawNoise;         /* Max peak/trough signal difference */
--    long    lRawNoiseOrig;     /* Max peak/trough signal difference when Phase1
--                                  passed */
--    long    lSampIndexF;       /* Next index to write in filt data circ. buff */
--    long    lSampIndexR;       /* Next index to write in raw data circular buff */
--    long    lSampNew;          /* Present sample */
--    long    lSampOld;          /* Last sample */  
--    long    lSampRaw;          /* Un-filter present sample */
--    long    lSampsInLastPacket;/* # samps in last packet read in - ANALYZE */
--    long    lSampsPerCyc;      /* Number of samples per half cycle */
--    long    lSampsPerCycLTA;   /* Number of samples per half cycle in LTA */
--    long    lStartFilterIndex; /* For partial reads, start index of read data */
--    long    lSWSim;            /* Count of similarities to sin wave cal */
--    long    lTest1;            /* Phase 2 Test 1 passed */
--    long    lTest2;            /* Phase 2 Test 2 passed */
--    long    lTrigFlag;         /* Flag -> samp has passed MDF threshold */
--    long    l13sCnt;           /* 13s counter; this delays p-picks 
--                                  by 13 seconds after pick for Neural net */
--    double  *pdRawDispData;    /* Pointer to Mwp buffer of displacement signal */
--    double  *pdRawIDispData;   /* Pointer to integrated displacement signal */
--    long    *plFiltCircBuff;   /* Pointer to buffer of filtered signal */
--    long    *plPickBuf;        /* Pick buffer for P adjustment */
--    long    *plPickBufRaw;     /* Pick buffer for Neural net checker */
--    long    *plRawData;        /* Pointer to Mwp buffer of unfiltered signal */
--    long    *plRawCircBuff;    /* Pointer to buffer of unfiltered signal */
--    double  *pdTimeCircBuff;   /* Pointer to the buffer time -- JFP */
--    SYSTEMTIME stPTime;        /* Time at P-pick Index in structure */
--    SYSTEMTIME stStartTime;    /* Time of data at index 0 */
--    char    szChannel[TRACE_CHAN_LEN]; /* Channel identifier (SEED notation) */
--    char    szHypoID[32];      /* If pick made in hypo_display, this is the
--                                  associated hypocenter ID */
--    char    szNetID[TRACE_NET_LEN];    /* Network ID */
--    char    szPhase[PHASE_LENGTH];     /* Pick Phase */
--    char    szPhaseIasp[MAX_PHASES][PHASE_LENGTH]; /* Iaspei91 Phase name */
--    char    szStation[TRACE_STA_LEN];  /* Station name */
--    char    szLocation[TRACE_STA_LEN]; /* Location */
--    char    szStationName[64]; /* Seismometer name */
--    fcomplex zPoles[MAX_ZP];   /* Poles of response function */
--    fcomplex zZeros[MAX_ZP];   /* Zeros of response function */
-- } STATION;               
