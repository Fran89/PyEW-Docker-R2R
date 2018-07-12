/*=====================================================================
// Copyright (C) 2000,2001 Instrumental Software Technologies, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code, or portions of this source code,
//    must retain the above copyright notice, this list of conditions
//    and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgment:
//    "This product includes software developed by Instrumental
//    Software Technologies, Inc. (http://www.isti.com)"
// 4. If the software is provided with, or as part of a commercial
//    product, or is used in other commercial software products the
//    customer must be informed that "This product includes software
//    developed by Instrumental Software Technologies, Inc.
//    (http://www.isti.com)"
// 5. The names "Instrumental Software Technologies, Inc." and "ISTI"
//    must not be used to endorse or promote products derived from
//    this software without prior written permission. For written
//    permission, please contact "info@isti.com".
// 6. Products derived from this software may not be called "ISTI"
//    nor may "ISTI" appear in their names without prior written
//    permission of Instrumental Software Technologies, Inc.
// 7. Redistributions of any form whatsoever must retain the following
//    acknowledgment:
//    "This product includes software developed by Instrumental
//    Software Technologies, Inc. (http://www.isti.com/)."
// THIS SOFTWARE IS PROVIDED BY INSTRUMENTAL SOFTWARE
// TECHNOLOGIES, INC. "AS IS" AND ANY EXPRESSED OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL INSTRUMENTAL SOFTWARE TECHNOLOGIES,
// INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//=====================================================================
//  A current version of the software can be found at
//                http://www.isti.com
//  Bug reports and comments should be directed to
//  Instrumental Software Technologies, Inc. at info@isti.com
//=====================================================================
// This work was funded by the IRIS Data Management Center
// http://www.iris.washington.edu
//===================================================================== 
*/

/*
**   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
**   CHECKED IT OUT USING THE COMMAND CHECKOUT.
**
**    $Id: ew2mseed.h 6803 2016-09-09 06:06:39Z et $
** 
**	Revision history:
**	$Log$
**	Revision 1.1  2010/03/10 17:14:58  paulf
**	first check in of ew2mseed to EW proper
**
**	Revision 1.7  2007/04/12 19:21:04  hal
**	Added ew7.0+ support (-dSUPPORT_SCNL) and linux port (-D_LINUX)
**	
**	-Hal
**	
**	Revision 1.6  2005/03/23 03:24:25  ilya
**	Fixed alighnment of data in BUD volumes
**	
**	Revision 1.5  2002/05/13 16:54:03  ilya
**	More changes for not writing into the wrong file
**	
**	Revision 1.4  2002/05/03 02:35:09  ilya
**	BUG fix: MSEED records -> wrong files
**	
**	Revision 1.3  2002/04/11 20:07:13  ilya
**	Add disabling of catchup mechanism
**	
**	Revision 1.2  2002/04/02 14:42:01  ilya
**	Version 02-Apr-2002: catchup algorithm
**	
**	Revision 1.1.1.1  2002/01/24 18:32:05  ilya
**	Exporting only ew2mseed!
**	
**	Revision 1.1.1.1  2001/11/20 21:47:00  ilya
**	First CVS commit
**	
 * Revision 1.8  2001/03/15  16:36:45  comserv
 * Modifications to add configurable SocketReconnect and to
 * reduce a frequency of menu updates
 * 	Declared a new function ew2mseedUpdateTimeout(), updated RING structure
 * 	added two new constant definitions
 *
 * Revision 1.7  2001/02/27  22:57:40  comserv
 * Modifications to decrease socket connections:
 * 	new function declared: int updateMenu (RINGS *,  WS_MENU_QUEUE_REC *);
 * 	new error flag is defined  #define CANNOT_OPEN_FILE -807
 * 	declared int wsAppendMenuNoSocketReconnect(WS_MENU_REC *, int, int);
 *
 * Revision 1.6  2001/02/23  21:32:19  comserv
 * 	a call to wsServersReAttach () is removed since it has not been used
 *
 * Revision 1.5  2001/02/12  17:45:27  comserv
 * sample rate is redefined as doule instead of int
 *
 * Revision 1.5  2001/02/12  17:45:27  comserv
 * sample rate is redefined as doule instead of int
 *
 * Revision 1.4  2001/02/06  05:53:09  comserv
 * New declarations:    void logFailedSCN and int resetWsTrace
 *
 * Revision 1.3  2001/01/19  16:41:21  comserv
 * C++ comments are removed
 *
 * Revision 1.2  2001/01/08  22:56:41  comserv
 * new structure elements are added
 *
 * Revision 1.1  2000/11/17  06:57:27  comserv
 * Initial revision
 *
*/
#ifdef SUPPORT_SCNL
#include  <ws_clientII.h>
#else
#include  "ws_clientII_no_location.h"
#endif

#include "qlib2.h"
#include "chron3.h"

/* qlib include */
#include "data_hdr.h"


/* ------------------------DEFINES -------------------------*/
#ifndef EW2MSEED
#define EW2MSEED

#ifndef STEIM1			
#define STEIM1	10 	/*from qlib2 */
#endif

#ifndef STEIM2
#define STEIM2	11	/* from qlib2 */
#endif

#define MAX_ADRLEN 20
#define EW2MSEED_SMALLEST_MSEED 128

	/* Time conversion factor for moving between
   Carl Johnson's seconds-since-1600 routines (chron3.c)
   and Solaris' and OS/2's seconds-since-1970 routines
   *****************************************************/

#ifndef STR1970
#define   STR1970  "19700101000000.00"
#endif

#ifndef SEC1970
#define SEC1970  11676096000.0
#endif

/* ew2mseed Errors */
/*******************/
#define EW2MSEED_TIME2_UPDATE_MENU -808 /* IGD 03/15/01 the same as below,         */ 
					/* indicationg that we need to update menu */
#define EW2MSEED_TOO_EARLY -801 /* tank EndTime <  reqEndTime */

/* Next two errors indicate that not all requested data are obtained */
/* from the server						     */
/*********************************************************************/
#define EW2MSEED_ACT_START_TIME_AFTER_REQ -802 /* actual start time after requested*/
#define EW2MSEED_ACT_END_TIME_BEFORE_REQ -803 /*actual end time before requested */

/* Next two errors indicate that either a TRACE_REQ or WS data */
/* buffers are empty		  			       */
#define TRACE_REQ_EMPTY -804   /* Trace request buffer is empty */
#define WS_DATA_BUF_EMPTY -805 /* Wave server data buffer is empty */
#define WRONG_DATA_TYPE -806 /* unknown datatype of data from WS */
#define CANNOT_OPEN_FILE -807

/* STATE FLAGS */
#define EW2MSEED_OK 0
#define EW2MSEED_TERMINATE 1
#define EW2MSEED_MAKE_REPORT 2
#define EW2MSEED_ALARM	3

/* Extern variable */
 char *d20; /* used in the logit to output date: malloced once! */
 char *d20a;
 int ew_signal_flag ;
/* ---------Data structures -------------------------------*/

 struct WS_ring {
	char *wsIP; /* [MAX_ADRLEN]; */
	char *wsPort; /* [MAX_ADRLEN]; */
	struct WS_ring *next;
} ;



 
struct SCN_ring {
	TRACE_REQ traceRec;
	double prevStarttime;
	double prevEndtime;
	double lastRequestTime;	/* IGD 03/29/02 Average time 
				of the last request to compute priority */
	int priority;		/* 03/29/02 An empirical
					 number used to compute the 
					amount of data requested by this SCN */
        /* the  portion from configuration */
	char locId[3]; 		/* Location ID mapped to SCN */
	int logRec;             /* the size of MINISEED record */
	short CompAlg;          /* STEIM1 = 10 or STEIM2 = 11 */	
	double tankStarttime;
	double tankEndtime;
	double timeOnFiles;     /* the time : end of latest MSEED + 1 sample */	
	char *dir;
	char *curMseedFile;
	int snippetSize;        /* in bytes per the request*/
	double samprate;	/* IGD changed from int 02/12/01 */
	long mseedSeqNo;
	long gapsCount;
	double timeInterval; /* Interval between reqEndtime and reqStarttime */
	double newFileJulTime; /* julian time to change the file */
	struct SCN_ring *next;	
};  /* Keeps track of a single SCN */

 	/*  IGD 04/11/02 Included flag to use/not use priorities 
	 * IGD 03/29/02 Added LoopsBeforeService and
	 *	 PriorityHighWater to process late channels
	 */ 
typedef struct {
	char *configFile;         /* the name of this file */
	short verbosity;          /* level of verbosity 1-4 */
	char *LockFile;           /* lockfile == NULL by default */
	char *MseedDir;           /* root directory for data */
	short RecNum;		  /* Number of records we read at once */	
	short logSwitch;          /* Defines the streams for the log */ 
	short GapThresh;          /* Number of samples to declare a gap */
	int TimeoutSeconds;        /* Period of reconnection to the socket */
	int TravelTimeout;         /* Time for request to WS to be replied */	
	int LoopsBeforeService;	   /* How many loops we run before 
					resetting SCN priorities */
	int usePriority;
	int PriorityHighWater;     /* The highest priority for late SCNs */
	char *StartTime;           /* The cutoff time to start data retrieval */
	double julStartTime;       /* same but in julian seconds */
	int WS_num ;               /* Number of waveservers in config file */
        int WS_avail; 	           /* Number of avaialble waveservers */
	int SCN_num;               /* Number of SCN in config file */
	int SCN_avail;	           /* Number of available SCNs in waveservers */
	time_t updateMenuTime;	   /* The epoch time when the menu was updated last time */
	char *pBuf;		   /* The common data buffer */
	long bufLen;		   /* The size of the common data buffer */
	long reqBufLen;		   /* This is to determine the reqEndTime */	
	struct SCN_ring *scnRing;  /* Structure containing SCNs */
	struct WS_ring * wsRing;   /* Structure containing WaveServers*/
	
}   RINGS; /* main structure of the program */

  struct yjd
  { 
	short year;
	short jday;
  };




/*-------------- Function prototypes -------------------- */
int ew2mseed_config (RINGS *);
int ew2mseed_logconfig(RINGS *);
int processWsAppendMenu (RINGS *,  WS_MENU_QUEUE_REC *);
int updateMenu (RINGS *,  WS_MENU_QUEUE_REC *);
int fillPSCN (RINGS *, WS_MENU_QUEUE_REC *);
void LogWsErr (char[], int );
char * format_IP(char *);
int filesHandler(RINGS *);
int checkNetDirs(RINGS *);
int setMseedFiles(RINGS *);
int make_dir(char *, int);
char * getMostRecentMseedFile(char *, char *);
int isValidMseedName (char *, struct yjd *, char *); 
double getMostRecentMseedTime (char *, char *, char *, char *, char *, long *);
char * createFirstMseedFile (char *, char *, double);
short getJdayFromGreg (struct Greg *);
double getReqStartTime (double, double);
double getReqEndTime (double, double, int);
double getSampleRate(TRACE_REQ *, WS_MENU_QUEUE_REC *, int);
int getParamsFromWS (RINGS *, WS_MENU_QUEUE_REC *);
long int assumeSnippetSize (int, short, short, int);
void makeSNCLoop (RINGS *);
int processSingleSCN(RINGS *, WS_MENU_QUEUE_REC *);
int updateSingleSCNtimes (RINGS *, WS_MENU_QUEUE_REC *);
int createMseed (struct SCN_ring *scnRing,
					  short verbosity);
double checkForGaps (double, double, double);
long ew2mseedAddData (int *, char *, long, int, char, int);
int getBufferShift (double, double, double);
int checkCompilationFlags (char *);
char * strDate(double, char *);
int processWsGetTraceBin (TRACE_REQ *, WS_MENU_QUEUE_REC *, int);
double findNewFileJulTime (double);
char *ew2mseedUpdateFileName(TRACE_REQ *, char *, char *);
void finish_handler(RINGS *);
void ew2mseedGenReport(RINGS *);
void signal_handler(int);
int checkLock (char *);
void logFailedSCN (struct SCN_ring *, char *);	/* IGD 02/05/01 Add */
int resetWsTrace (TRACE_REQ*, double, double); /* IGD 02/05/01 Add */
void ew2mseedUpdateTimeout(RINGS *); /* IGD 03/14/01 */
/* Swapping routines from ew2mseed_swap.c */
int ew2mseed_SwapInt (int *, char);
int ew2mseed_SwapInt32 (long *, char);
int ew2mseed_SwapShort (short *, char);
int ew2mseed_SwapDouble (double *, char);
int wsAppendMenuNoSocketReconnect(WS_MENU_REC *, int, int);
void updatePriorityValues(RINGS *);
char *createBUDFileName (char * dir, char*begName, double jtime);
void ew2mseed_swapMiniSeedHeader(SDR_HDR *mseed_hdr );
#endif
