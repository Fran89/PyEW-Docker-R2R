/*
 * pick_FP.h
 * modified from pick_ew.h, Revision 1.7 in src/seismic_sampling/pick_ew
 * 
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 *
 * (C) 2008-2012 Claudio Satriano <satriano@ipgp.fr>,
 * under the same license terms of the Earthworm software system. 
 */

/******************************************************************
 *                         File pick_FP.h                         *
 ******************************************************************/

#include "picker/ew_bridge.h"
#include "picker/PickData.h"
#include "picker/FilterPicker5_Memory.h"
#include "picker/FilterPicker5.h"

#define LINELEN 200         /* Size of char arrays to hold picks and codas */

/* Error bits */
#define PK_RESTART 1        /* Set when time series was broken, picker restarted */

// TODO: the PICK structure is replaced by the PickData structure - shall we remove this?
/* Pick variables */
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

// TODO: we don't have a coda implementation
/* Coda variables */
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

/* Picking parameters */
typedef struct {
	double filterWindow;
	double longTermWindow;
	double threshold1;
	double threshold2;
	double tUpEvent;
} PARM;

/* Station list parameters */
typedef struct {
	char   sta[6];           /* Station name */
	char   chan[4];          /* Component code */
	char   net[3];           /* Network code */
	char   loc[3];           /* Location code */
	CODA   Coda;             /* Coda structure */
	PICK   Pick;             /* Pick structure */
	PARM   Parm;             /* Configuration file parameters */
	long   enddata;          /* Last data value of previous message */
	double endtime;          /* Stop time of previous message */
	int    first;            /* 1 the first time this channel is found */
	int    ns_restart;       /* Number of samples since restart */
	
	FilterPicker5_Memory *mem;
	int 	phase_number;
} STATION;

#define STAFILE_LEN 64
typedef struct {
	char   name[STAFILE_LEN]; /* Name of station file */
	int    nsta;              /* number of channels configure in this file */
} STAFILE;

typedef struct {
	STAFILE  *StaFile;       /* Name of file(s) with SCNL info */
        char     *PickIndexDir;  /* an optional directory to place pick index files, to get them out of the param dir */
	int       nStaFile;      /* Number of StaFile commands given */
	long      InKey;         /* Key to ring where waveforms live */
	long      OutKey;        /* Key to ring where picks will live */
	int       HeartbeatInt;  /* Heartbeat interval in seconds */
	int       RestartLength; /* Number of samples to process for restart */
	int       MaxGap;        /* Maximum gap to interpolate */
	int       Debug;         /* If 1, print debug messages */
	int       NoCoda;        /* If 1, just do picks, no coda's */
	float     WeightTable[4];/* Pick weight table (maximum error in seconds for weight 0 up to weight 3) */
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
