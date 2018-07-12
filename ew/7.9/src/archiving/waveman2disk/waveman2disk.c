/*

Thu Jul 22 14:56:37 MDT 1999 Lucky Vidmar


***************  FIX *************************

Story:  this is the manual, interactive, non-earthworm module 
waveman2disk.  Its purpose is to save trace data to disk in variuos
supported formats - currently ah, sac, and pc-suds. Like any 
good earthworm program, it has a configuration file waveman2disk.d
allowing for extensive configuration. This is where we tell it, among
other things, which waveservers to interrogate, where and it which format 
to store the trace output,

waveman2disk works in two modes:

(1) Emergency:  the config file lists the output format and location,
as well as the channels to save. The user is prompted for the
start and end time of trace data to be saved.

(2) Trigger-saving: a file, specified in the config file, contains
ascii text of one or more TYPE_TRIGLIST messages. These 
are processed one at a time, and the trace data of the channels
listed in the TRIGLIST message, are saved to disk.

Alex, 1/20/00: fix to accept string rather than integer EVENTID: on the 
TYPE_TRIGLIST message.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <time.h>
#include <signal.h>
#include <kom.h>
#include <transport.h>
#include <ws_clientII.h>
#include <parse_trig.h>
#include <putaway.h>
#include "waveman2disk.h"

#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff  /* should be in <netinet/in.h> */
#endif

    /* Functions in this source file */
    static int ConvertTime (char *, double *);
static int WriteEvent (void);
static int waveman2disk_config (char *);
static int matchSCNL( SNIPPET* pSnppt, WS_PSCNL pscnl);
static int duplicateSCNL( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq);
static int matchSCN( SNIPPET* pSnppt, WS_PSCNL pscnl);
static int duplicateSCN( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq);
static int GetInfoFromSnippet (SNIPPET);
static int snippet2trReq (WS_MENU_QUEUE_REC *, SNIPPET *, TRACE_REQ *, int, 
                          int *);


/* Functions from other source files */
extern int     t_atodbl (char*, char*, double*);

static	WS_MENU_QUEUE_REC MenuList; /* the list of menues */

/* Globals to set from configuration file*/
static int     LogSwitch;           /* 0 if no logfile should be written   */
static long    TimeoutSeconds;      /* seconds to wait for reply from ws */
static long    MaxTraces;           /* max traces per message we'll ever deal with  */
static int     nWaveServers;        /* number of wave servers we know about */
static char    wsIp[MAX_WAVESERVERS][MAX_ADRLEN];
static char    wsPort[MAX_WAVESERVERS][MAX_ADRLEN];
static char    OutDir[MAXTXT];      /*base output data directory*/
static double  GapThresh;  
static int     MinDuration;         /* minimum duration of data files for 
                                       stations not in triglist */
static char    OutputFormat[MAXTXT];/* intel or sparc */
static char    DataFormat[MAXTXT];  /* sac, suds and ah are supported */
static char    InputMethod[MAXTXT]; /* triglist or interactive */
static char    TrigFile[MAXTXT];    /* the optional trigger file name */


/** Snippet information section - Filled by GetInfoFromSnippet () **/
#define MOD_CHARS   3
#define INST_CHARS  3
#define TIME_CHARS  11

static char EventSubnet[MAXTXT];
static char EventID[MAXTXT];
static char EventDate[MAXTXT];
static char EventTime[MAXTXT + 4];

static	char	*StartTime = NULL;
static	double	reqDuration = 0;

/* Things to be malloc'd*/
/* story: Some of these things don't have to be global; they are only
   so that we can malloc them from main at startup time. Having them 
   malloc'd by the routine which uses them presents the danger that the 
   module would start ok, but die (if unable to malloc) only when the 
   first event hits. Which could be much later, at a bad time...*/

static char   *TraceBuffer;      /* where we store the trace snippet  */
static long    TraceBufferLen;   /* bytes of largest snippet we're up for - 
                                    from configuration file */
static TRACE_REQ *TraceReq;      /* request forms for wave server. Malloc'd at 
                                    startup. From ws_client.h */
static	int    nTrReq = 0;       /* number of TRACE_REQ structures
                                    from the entire message*/

static int     FormatIndex;       /* numerical value for our output format */
static long    OutBufferLen;      /* kilobytes of largest trace we'll save */

/* Debug debug DEBUG */
static int     Debug = 1;  

static SCNL     SaveStations[MAX_STATIONS];  /* SCNLs to save */
static SCNL     AddStations[MAX_STATIONS];   /* SCNLs to add to list */

static SNIPPET Snppt;              /* holds params for trace snippet. From
                                      parse_trig.h */

static int     n_add_stations;     /* number of stations read */
static int     n_save_stations;    /* number of stations read */

#define WM2D_VERSION "1.0.7 2016-08-10"

int main(int argc, char **argv)
{
    int          i;
    static char  trgMsg[MAX_TRIG_BYTES];     /* array to hold trigger message
						(size from earthworm.h)   */
    signed char  nextc;          /* must be signed, since EOF is -1 */
    int          ret;
    int          done, gotit;
    FILE        *fp;
    char        *nxtSnippetPtr;  /* pointer to next line in trigger message */
    int          iTrReq=0;       /* running index over TRACE_REQ structures */
    int          oldNTrRq=0;     /* temp for storing number of tr req's */
    WS_MENU      server = NULL;
    double       reqStart;
    int          log_to_disk_at_startup = 1;


    /* Check command line arguments */
    if (argc < 2) {
	fprintf (stderr, "Usage: waveman2disk <configfile> [n]\n");
	fprintf (stderr, "Version: %s\n", WM2D_VERSION);
	return EW_FAILURE;
    }
    if (argc == 3) {
        if (argv[2][0] == 'n') {
            log_to_disk_at_startup = 0;
        }
    }
    logit_init ("waveman2disk", 0, MAX_TRIG_BYTES+256, log_to_disk_at_startup);
    
    /* Zero the wave server arrays */
    for (i = 0; i < MAX_WAVESERVERS; i++) {
	memset (wsIp[i], 0, MAX_ADRLEN);
	memset (wsPort[i], 0, MAX_ADRLEN);
    }
       
#ifdef SIGPIPE
  (void)sigignore(SIGPIPE);
#endif

    /* Read the configuration file(s)*/
    if (waveman2disk_config (argv[1]) != EW_SUCCESS) {
	logit ("e", "Call to waveman2disk_config failed \n");
	return EW_FAILURE;
    }
    logit ("" , "waveman2disk: Read command file <%s>\n", argv[1]);

    /* Reset logging to desired level */
    logit_init ("waveman2disk", 0, MAX_TRIG_BYTES+256, LogSwitch);


    /* get the station list (if it doesnt exist, use whatever gets picked */
    if (n_add_stations > 0) {
	logit ("", "waveman2disk using appended station list of %d stations\n",
	       n_add_stations);
	if (Debug == 1) {
	    for (i = 0; i < n_add_stations; i++) {
		logit ("", "station[%d]: %s %s %s %s\n", i,
		       (AddStations + i)->sta,
		       (AddStations + i)->chan,
		       (AddStations + i)->net,
		       (AddStations + i)->loc);
	    }
	}
    }
    
    /* show the wave server info*/
    if (Debug == 1) {
	logit ("", "wave server stuff:\n TimeoutSeconds=%ld\n", TimeoutSeconds);
	for (i = 0; i < nWaveServers; i++) {
	    logit ("", " wsIp[%d]=.%s.",i,wsIp[i]);
	    logit ("", " wsPort[%d]=.%s.\n",i,wsPort[i]);
	}
    }
    
    /* Allocate the trace snippet buffer*/
    if ((TraceBuffer = malloc ((size_t) TraceBufferLen)) == NULL) {
	logit ("e",
	       "Cannot allocate snippet buffer of %ld bytes. Exitting\n",
	       TraceBufferLen);
	return EW_FAILURE;
    }

    /* Allocate the trace request structures */
    if ( (TraceReq = (TRACE_REQ *)calloc ((size_t)MaxTraces, sizeof(TRACE_REQ)))
	 == (TRACE_REQ *)NULL) {
	logit ("e",
	       "Out of memory for %ld TRACE_REQ structures.\n",
	       MaxTraces);
	return EW_FAILURE;
    }

    /* Initialize the disposal system*/
    if (PA_init (DataFormat, TraceBufferLen, &OutBufferLen, 
		 &FormatIndex, OutDir, OutputFormat, Debug) != EW_SUCCESS) {
	logit ("e", "Call to PA_init failed; exitting!\n");
	return EW_FAILURE;
    }
    
    /* initialize the socket system; this is a no-op for Unix */
    SocketSysInit();

    /* Build the current wave server menus*/
    for (i = 0; i < nWaveServers; i++) {
	if (Debug == 1)
	    logit("",
		  "calling wsAppendMenu with wsIp[%d]=%s, wsPort[%d]=%s," 
		  " Timout=%ld\n", i, wsIp[i], i, wsPort[i], TimeoutSeconds * 1000);
	
	ret = wsAppendMenu(wsIp[i], wsPort[i], &MenuList, TimeoutSeconds*1000);
	
	if (ret != WS_ERR_NONE) {
	    /* 
	       lots of possible errors, as from ws_clientII.h, but for now 
	       we don't care.  if things went wrong, we dont deal with 
	       that server. Someday we may get fussier...
	    */
	    
	    logit ("",
		   "WARNING: Failed to get menu from %s:%s; wsAppendMenu returned %d\n",
		   wsIp[i],wsPort[i], ret);
	}
    }
    
    /* Make sure that we have at least one menu */

    server = MenuList.head; /* which points to first server's menu*/

    if (server == NULL) {
	logit ("e", "wsAppendMenu built no menus; exitting!\n");
	return EW_FAILURE;
    } 
    
    /*Debug: show off our menu list*/
    if (Debug == 1) {
	logit ("", "\n Total Menu List:\n");
	while (server != NULL) {
	    WS_PSCNL tank = server->pscnl;
	    logit ("", "Server %s:%s:\n",server->addr,server->port);
	    while (tank != NULL) {
		logit ("", "%s %s %s %f %f\n", 
		       tank->sta,tank->chan,tank->net, 
		       tank->tankStarttime, tank->tankEndtime);
		tank = tank->next;
	    }
	    server = server->next;
	} 
	logit ("", "End of Menu\n");
    }

    if (strcmp (InputMethod, "triglist") == 0) {
	/* Read the file containing TYPE_TRIGLIST2K messages. */
	/* For each message, copy it into trgMsg array and    */
	/* proceed by filling the TraceReq array              */
	
	if ( (fp = fopen (TrigFile, "rt")) == NULL) {
	    logit ("e", "Can't open %s\n", TrigFile);
	    return EW_FAILURE;
	}

	/*
	 * Read the triglist message a byte at a time, looking for a 
	 * terminating '$' or EOF. Hope some fool doesn't use that in 
	 * a station or something. 
	 * Yes, this is slow IO, but what do you expect?
	 * The triglist may contain multiple events; each one is terminated by
	 * either '$' or EOF. We handle each event one at a time.
	 */
	done = FALSE;
	while (done == FALSE) {
	    /* Start a new event: */
	    gotit = FALSE;
	    i = 0;
	    while (gotit == FALSE) {
		if (i == MAX_TRIG_BYTES) {
		    logit("e", "trigger message <%s> too long; max is %d\n", TrigFile, 
			  MAX_TRIG_BYTES-1);
		    done = TRUE;
		    trgMsg[MAX_TRIG_BYTES-1] = '\0';
		    /* Maybe we should exit now, but we'll blunder on in the dark */
		    break;
		}
		nextc = (char)getc(fp);
		if (nextc == '$') {
		    gotit = TRUE;
		    trgMsg[i] ='\0';
		}
		else if (nextc == EOF) {
		    gotit = TRUE;
		    done = TRUE;
		    trgMsg[i] ='\0';
		}
		else {
		    trgMsg[i++] = nextc;
		}
	    }
	    /* Got one event; now deal with it. */
	    if (Debug == 1)
		{
		    logit ("", "Read trig message from file %s:\n", TrigFile);
		    logit ("", "%s\n", trgMsg);
		}
	    /* Add in any phony triggers we found in the config file */
	    if (n_add_stations > 0) {
		if (CatPsuedoTrig(trgMsg, AddStations, n_add_stations, MinDuration)
		    != EW_SUCCESS )
		    logit ("e", "CatPsuedoTrig failed to add stations \n");
		
		if (Debug == 1) 
		    logit("","return from CatPseudoTrig. New trgMsg size = %d\n",
			  (int)strlen(trgMsg) );
	    }
	    
	    /* begin loop over lines in event message */
	    nxtSnippetPtr = trgMsg;
	    nTrReq = 0;   /* total number of trace requests from all snippet lines */
	    
	    if (Debug == 1) 
		logit ("","Starting parseSnippet loop\n");
	    
	    while (parseSnippet (trgMsg, &Snppt, &nxtSnippetPtr) == EW_SUCCESS) {
		/* get next snippet params from msg */
		
		if (Debug == 1) 
		    logit ("", "After parseSnippet() in while loop\n");
		
		/* create requests implied by this snippet - ( it might include 
		   wildcards) */
		/* 
		   routine below will create the request structures. It has access to
		   the WaveServer Menu lists.
		   A little clumsiness: snippet2trReq only fills in the SCNL name
		   portion of the trace request.  The rest is done in the loop after
		   the call.
		*/ 
		
		oldNTrRq = nTrReq; /* save current value of trace reqeusts */ 
		ret = snippet2trReq (&MenuList, &Snppt, TraceReq, MaxTraces, &nTrReq); 
		if (ret != WS_ERR_NONE) {
		    switch (ret) {
		    case WS_ERR_BUFFER_OVERFLOW:
			/* then we've run out of request structures */
			logit ("e",
			       "MaxTraces (%ld) exceeded. Some traces not saved\n",
			       MaxTraces); 
			break;
		    case WS_ERR_INPUT:
		    case WS_ERR_SCNL_NOT_IN_MENU:
		    case WS_ERR_EMPTY_MENU:
			logit("e", "call to snippet2trReq failed\n");
			return EW_FAILURE;
		    }
		}
		
		if (Debug == 1)
		    logit ("",
			   "return from snippet2trReq: total of %d requests so far (was %d)\n",
			   nTrReq,oldNTrRq);
		
		if (oldNTrRq == nTrReq) /* then this snippet generated no requests. 
					   Suspicious. Log it */
		    logit ("e", "WARNING: in event %s %s %s %s %s was either "
			   "multiply requested, or not found in menu\n",
			   Snppt.eventId,Snppt.sta,Snppt.chan,Snppt.net,Snppt.loc);
		
		/* Fill in tracereq info that is common to this snippet */
		for (iTrReq = oldNTrRq; iTrReq < nTrReq; iTrReq++) {       
		    TraceReq[iTrReq].reqStarttime = Snppt.starttime;
		    TraceReq[iTrReq].reqEndtime = Snppt.starttime + Snppt.duration ;
		    TraceReq[iTrReq].pBuf = TraceBuffer;
		    TraceReq[iTrReq].bufLen = TraceBufferLen;
		    TraceReq[iTrReq].timeout = TimeoutSeconds;
		}
		
		if (Debug == 1)
		    logit ("", "Calling GetInfoFromSnippet\n");
		
		GetInfoFromSnippet (Snppt);
		
	    } /* end of while parseSnippet() loop over lines in message (level 1)*/
	    
	    if (Debug == 1) 
		logit ("","Done parseSnippet loop\n");
	    
	    /* write traces for this event */
	    if (WriteEvent () != EW_SUCCESS) {
		logit ("e", "Call to WriteEvent failed!\n");
		return EW_FAILURE;
	    }
	    
	} /* end of while not done reading the triglist file */
	fclose (fp);
    } /* InputMethod = triglist */
    
    else if (strcmp (InputMethod, "interactive") == 0) {
	/* If the start and end times are not set in the config file */
	/* prompt the user for them. Then, match the desired SCNLs    */
	/* given by SaveSCNL list agains the SCNLs that are available  */
	/* from the waveservers, build the request array, and call   */
	/* WriteEvent to produce output on disk                      */
	
	if ((StartTime == NULL) || (reqDuration == 0)) {
	    if ((StartTime = malloc (MAXTXT)) == NULL) {
		logit ("e", "out of memory for StartTime.\n");
		return EW_FAILURE;
	    }
	    
	    printf ("Enter Start Time - YYYYMMDDHHMMSS: ");
	    scanf ("%s", StartTime);
	    printf ("Enter Duration in seconds: ");
	    scanf ("%d", &i);
	    reqDuration = (double) i;
	}
	
	if (Debug == 1)
	    logit ("", "Start at %s for %f secs\n", StartTime, reqDuration);
	
	if (ConvertTime (StartTime, &reqStart) != EW_SUCCESS) {
	    logit ("e", "Call to ConvertTime failed.\n");
	    return EW_FAILURE;
	}
	
	nTrReq = 0;
	for (i = 0; i < n_save_stations; i++) {
	    strcpy (Snppt.sta, SaveStations[i].sta);
	    strcpy (Snppt.chan, SaveStations[i].chan);
	    strcpy (Snppt.net, SaveStations[i].net);
	    strcpy (Snppt.loc, SaveStations[i].loc);
	    
	    /* loop through available wave server menu choices */
	    server = MenuList.head; /* which points to first server's menu*/
	    while ((server != NULL) && (nTrReq < MaxTraces)) {
		WS_PSCNL tank = server->pscnl;
		
		while ((tank != NULL) && (nTrReq < MaxTraces)) {
		    if (strlen(tank->loc) > 0) {
			if ((matchSCNL(&Snppt, tank) == 1) && 
			    (duplicateSCNL(tank, TraceReq, nTrReq) == 0)) {
			    /* We have a match */
			    strcpy (TraceReq[nTrReq].sta, tank->sta);
			    strcpy (TraceReq[nTrReq].chan, tank->chan);
			    strcpy (TraceReq[nTrReq].net, tank->net);
			    strcpy (TraceReq[nTrReq].loc, tank->loc);
			    TraceReq[nTrReq].reqStarttime = reqStart;
			    TraceReq[nTrReq].reqEndtime = reqStart + reqDuration;
			    TraceReq[nTrReq].pBuf = TraceBuffer;
			    TraceReq[nTrReq].bufLen = TraceBufferLen;
			    TraceReq[nTrReq].timeout = TimeoutSeconds;
			    
			    nTrReq = nTrReq + 1;
			}
		    } else {
			if ((matchSCN(&Snppt, tank) == 1) && 
			    (duplicateSCN(tank, TraceReq, nTrReq) == 0)) {
			    /* We have a match */
			    strcpy (TraceReq[nTrReq].sta, tank->sta);
			    strcpy (TraceReq[nTrReq].chan, tank->chan);
			    strcpy (TraceReq[nTrReq].net, tank->net);
			    TraceReq[nTrReq].loc[0] = '\0';
			    TraceReq[nTrReq].reqStarttime = reqStart;
			    TraceReq[nTrReq].reqEndtime = reqStart + reqDuration;
			    TraceReq[nTrReq].pBuf = TraceBuffer;
			    TraceReq[nTrReq].bufLen = TraceBufferLen;
			    TraceReq[nTrReq].timeout = TimeoutSeconds;
		    
			    nTrReq = nTrReq + 1;
			    if (strcmp(Snppt.loc, "--") && strcmp(Snppt.loc, "*"))
				logit("et", "WARNING: specific location code <%s.%s.%s.%s>"
				      "served by SCN server\n", 
				      Snppt.sta,Snppt.chan, Snppt.net,Snppt.loc);
			}
		    }
		    tank = tank->next;
		} /* tank != NULL */

		server = server->next;
	    } /* server != NULL */ 
	} /* loop over requested channels */
	
	if (Debug == 1) {
	    logit ("", "Will request %d traces:\n", nTrReq);
	    for (i = 0; i < nTrReq; i++) {
		logit ("", "%s-%s-%s-%s: %0.2f %0.2f\n", 
		       TraceReq[i].sta,
		       TraceReq[i].chan,
		       TraceReq[i].net,
		       TraceReq[i].loc,
		       TraceReq[i].reqStarttime,
		       TraceReq[i].reqEndtime);
	    }
	}

	strncpy (EventDate, StartTime, 8);
	strncpy (EventTime, (StartTime + 8), 6);
	strcat (EventTime, ".00");
	strcpy (EventID, "MAN");
	strcpy (EventSubnet, "MAN");
	strcpy (Snppt.author, "MAN");
	strcpy(Snppt.eventId, "0");

	/* write traces for this event */
	if (WriteEvent () != EW_SUCCESS) {
	    logit ("e", "Call to WriteEvent failed!\n");
	    return EW_FAILURE;
	}
    } /* InputMethod = interactive */

    else {
	logit ("e", "Invalid InputMethod - %s!\n", InputMethod);
    }

    if (PA_close (FormatIndex, Debug) != EW_SUCCESS) {
	logit ("e", "Call to PA_close failed!\n");
    }

    return EW_SUCCESS;
}


/***********************************************************************
 * ConvertTime () - given pStart return a double representing          *
 *     number of seconds since 1970                                    *
 ***********************************************************************/
static int ConvertTime (char *pStart, double *start)
{
    char	YYYYMMDD[9];
    char	HHMMSS[12];

    if (pStart == NULL) {
	logit ("e", "Invalid parameters passed in.\n");
	return EW_FAILURE;
    }
    
    strncpy (YYYYMMDD, pStart, (size_t) 8);
    YYYYMMDD[8] = '\0';

    HHMMSS[0] = pStart[8];
    HHMMSS[1] = pStart[9];
    HHMMSS[2] = ':';
    HHMMSS[3] = pStart[10];
    HHMMSS[4] = pStart[11];
    HHMMSS[5] = ':';
    HHMMSS[6] = pStart[12];
    HHMMSS[7] = pStart[13];
    HHMMSS[8] = '.';
    HHMMSS[9] = '0';
    HHMMSS[10] = '0';
    HHMMSS[11] = '\0';

    if (t_atodbl (YYYYMMDD, HHMMSS, start) < 0) {
	logit ("e", "Can't convert StartTime %s -> %s %s: t_atodbl failed.\n",
	       pStart, YYYYMMDD, HHMMSS);
	return EW_FAILURE;
    }
    
    return EW_SUCCESS;
}

/***********************************************************************
 *  WriteEvent() - using global variables from main, retrieve and      *
 *       write event trace data in specified format out to disk        *
 ***********************************************************************/
static int WriteEvent (void)
{
    int  ret;
    int  iTrReq;       /* running index over TRACE_REQ structures */

    if (Debug == 1)
	logit ("", "calling PA_next_ev(TraceReq = %lu, nTrReq=%d,"
	       "Snppt.eventID = %s,  Snppt.author = %s, Debug = %d\n",
	       (unsigned long)TraceReq, nTrReq, EventID, Snppt.author, Debug);

    if ( PA_next_ev(EventID, TraceReq, nTrReq, FormatIndex, OutDir, 
		    EventDate, EventTime, EventSubnet, Debug) != EW_SUCCESS) {
	logit("e", "Call to PA_next_ev failed; exitting!\n");
	return EW_FAILURE;
    }

    if(Debug == 1) 
	logit ("", "returning from PA_next_ev\n");

    /* begin loop over retrieving and disposing of the trace snippets */
    for (iTrReq = 0; iTrReq < nTrReq; iTrReq++) {
	/* get this trace; rummage through all the servers we've been told about */
	if (Debug == 1)
	    logit ("", "calling wsGetTraceBin for request %d\n",iTrReq);
	
	if (strlen(TraceReq[iTrReq].loc) > 0) 
	    ret = wsGetTraceBinL( &(TraceReq[iTrReq]), &MenuList, TimeoutSeconds*1000 );
	else
	    ret = wsGetTraceBin( &(TraceReq[iTrReq]), &MenuList, TimeoutSeconds*1000 );
	
	if (Debug == 1) {
	    logit ("", "return from wsGetTraceBin(L): %d\n", ret);
	    logit ("", "actStarttime=%lf, actEndtime=%lf, actLen=%ld, samprate=%lf\n",
		   TraceReq[iTrReq].actStarttime, TraceReq[iTrReq].actEndtime,
		   TraceReq[iTrReq].actLen, TraceReq[iTrReq].samprate);
	}
	
	if (ret != WS_ERR_NONE ) {
	    logit ("e", "problem retrieving %s %s %s %s ", TraceReq[iTrReq].sta,
		   TraceReq[iTrReq].chan, TraceReq[iTrReq].net, 
		   TraceReq[iTrReq].loc); 
	    
	    switch( ret ) {
	    case WS_WRN_FLAGGED:
		logit ("e", "server has no data for period requested.\n");
		continue;
		break;
	    case WS_ERR_SCNL_NOT_IN_MENU:
		logit ("e", "SCNL not found in menu list; continuing.\n");
		continue;
		break;
		
		/* following errors will cause the socket to be closed - exit */
	    case WS_ERR_EMPTY_MENU:
		logit ("e", "no menu list found; continuing.\n");
		return EW_FAILURE;
		break;
	    case WS_ERR_BUFFER_OVERFLOW:
		logit ("e", "trace buffer overflow; exitting!\n");
		return EW_FAILURE;
		break;
	    case WS_ERR_PARSE:
		logit ("e", "error parsing server's reply; exitting!\n");
		return EW_FAILURE;
		break;
	    case WS_ERR_TIMEOUT:
		logit ("e", "timeout talking to wave server; exitting!\n");
		return EW_FAILURE;
		break;
	    case WS_ERR_BROKEN_CONNECTION:
		logit ("e", "connection to wave server broken; exitting!\n");
		return EW_FAILURE;
		break;
	    case WS_ERR_SOCKET:
		logit ("e", "error changing socket options; exitting!\n");
		return EW_FAILURE;
		break;
	    case WS_ERR_NO_CONNECTION:
		logit ("e", "socket to wave server already closed; exitting!\n");
		return EW_FAILURE;
		break;
	    default:
		logit ("e", "unknown error code %d; exitting!\n", ret);
		return EW_FAILURE;
	    }
	}
	if (Debug == 1)
	    logit ("",
		   "trace %s %s %s %s: went ok first time. Got %ld bytes\n",
		   TraceReq[iTrReq].sta, TraceReq[iTrReq].chan,
		   TraceReq[iTrReq].net, TraceReq[iTrReq].loc,
		   TraceReq[iTrReq].actLen); 
	
	/* Now call the snippet disposer*/
	if (Debug == 1)
	    logit ("",
		   "Calling PA_next (Snptt.eventId= %s, scnl %s,%s,%s,%s)\n",
		   Snppt.eventId, TraceReq[iTrReq].sta, TraceReq[iTrReq].chan, 
		   TraceReq[iTrReq].net, TraceReq[iTrReq].loc);  
	
	if (PA_next (&(TraceReq[iTrReq]), FormatIndex, GapThresh, 
		     OutBufferLen, Debug) != EW_SUCCESS) {
	    logit ("e", "Call to PA_next failed; exitting!\n");
	    return EW_FAILURE;
	}
	
	if (Debug == 1)
	    logit ("", "Returned from NextSnippetForAnEvent: %d\n", ret);
	
    } /* end of loop getting and disposing snippets */
    
    if (PA_end_ev (FormatIndex, Debug) != EW_SUCCESS) {
	logit ("e", "Call to PA_end_ev failed; exitting\n");
	return EW_FAILURE;
    }
    
    return EW_SUCCESS;
}

#define NUM_COMMANDS 	11

/***********************************************************************
 *  waveman2disk_config() processes command file(s) using kom.c functions;*
 *                  exits if any errors are encountered.               *
 ***********************************************************************/
static int waveman2disk_config (char *configfile)
{
    int   ncommand;     /* # of required commands you expect to process   */ 
    char  init[20];     /* init flags, one byte for each required command */
    int   nmiss;        /* number of required commands that were missed   */
    char *com;
    char *str;
    int   nfiles;
    int   success;
    int   i;
    int   SaveSCNLSet;
    int   TrigFileSet;

    struct hostent  *host;
    struct in_addr addr_s;


    /* Set to zero one init flag for each required command */
    ncommand = NUM_COMMANDS;
    for( i=0; i < ncommand; i++ )  
	init[i] = 0;
    nWaveServers = 0;
    n_add_stations = 0;
    n_save_stations = 0;
    SaveSCNLSet = 0;
    TrigFileSet = 0;
    Debug = 0;

    /* Open the main configuration file */
    nfiles = k_open( configfile ); 
    if ( nfiles == 0 ) {
	    logit ("e", "Error opening command file <%s>; exitting!\n",
		   configfile );
	    return EW_FAILURE;
	}

    /* Process all command files*/
    while(nfiles > 0) {  /* While there are command files open */
	    while(k_rd()) {       /* Read next line from active file  */
		    com = k_str();         /* Get the first token from line */

		    /* Ignore blank lines & comments*/
		    if( !com )           continue;
		    if( com[0] == '#' )  continue;

		    /* Open a nested configuration file */
		    if( com[0] == '@' ) {
			success = nfiles+1;
			nfiles  = k_open(&com[1]);
			if ( nfiles != success ) {
			    logit ("e",
				   "Error opening command file <%s>; exitting!\n",
				   &com[1] );
			    return EW_FAILURE;
			}
			continue;
		    }

		    /* Process anything else as a command */
		    /*0*/     if( k_its("LogFile") ) {
			    LogSwitch = k_int();
			    init[0] = 1;
			}

		    /*NR*/    else if( k_its("Debug") )  /*optional command*/
			{  /*optional command*/
			    Debug = 1;
			}

		    /* wave server addresses and port numbers to get trace snippets from*/
		    /*1*/     else if( k_its("WaveServer") ) {
			    if ( nWaveServers >= MAX_WAVESERVERS ) {
				    logit ("e", "Too many <WaveServer> commands in <%s>",
					   configfile );
				    logit ("e", "; max=%d; exitting!\n", (int) MAX_WAVESERVERS );
				    return EW_FAILURE;
				}
			    if( ( str=k_str() ) != NULL )
				strcpy(wsIp[nWaveServers],str);
			    if( ( str=k_str() ) != NULL )
				strcpy(wsPort[nWaveServers],str);
			    /* put in DNS check */
  				if ( inet_addr(wsIp[nWaveServers]) == INADDR_NONE )
    				{        /* it's not a dotted quad address */
      					if ( (host = gethostbyname(wsIp[nWaveServers])) == NULL)
        				{
          					logit("e", "bad server address <%s>\n", wsIp[nWaveServers] );
          					return EW_FAILURE;
        				}
      					memcpy((char *) &addr_s, host->h_addr,  host->h_length);
      					str = inet_ntoa(addr_s);
					strcpy(wsIp[nWaveServers],str);
    				}

			    nWaveServers++;
			    init[1] = 1;
			}

		    /*2*/     else if( k_its("TimeoutSeconds") ) {
			    TimeoutSeconds = k_int(); 
			    init[2] = 1;
			}

		    /*3*/     else if( k_its("MaxTraces") ) {
			    MaxTraces = k_int(); 
			    init[3] = 1;
			}

		    /*4*/     else if( k_its("TraceBufferLen") ) {
			    TraceBufferLen = k_int() * 1024; /* convert from kilobytes to bytes */
			    init[4] = 1;
			}

		    /*5*/     else if( k_its("GapThresh") ) {
			    GapThresh = k_val();
			    init[5]=1;
			}


		    /*NR*/    else if (k_its ("TrigStation")) { 
			    if (n_add_stations >= MAX_STATIONS)
				{
				    logit ("e", "Too many <TrigStation> commands in <%s>",
					   configfile);
				    logit ("e", "; max=%d; exitting!\n", (int) MAX_STATIONS);
				    return EW_FAILURE;
				}

			    if ((str = k_str ()) != NULL)
				strcpy ((AddStations + n_add_stations)->sta, str);
			    if ((str = k_str ()) != NULL)
				strcpy((AddStations + n_add_stations)->chan, str);
			    if ((str = k_str ()) != NULL)
				strcpy ((AddStations + n_add_stations)->net, str);
			    if ((str = k_str ()) != NULL)
				strcpy ((AddStations + n_add_stations)->loc, str);
			    n_add_stations++;
			}

		    /*6*/     else if( k_its("MinDuration") ) {
			    MinDuration = k_int();
			    init[6]=1;
			}

		    /*7*/     else if( k_its("DataFormat") ) {
			    if( ( str=k_str() ) != NULL )
				strcpy( DataFormat, str );

			    init[7] = 1;
			}
		    /*8*/     else if( k_its("OutDir") ) {
			    if( ( str=k_str() ) != NULL )
				strcpy( OutDir, str );
			    init[8] = 1;
			}
		    /*9*/    else if( k_its("OutputFormat") ) {
			    if ((str = k_str()) != NULL)
				strcpy (OutputFormat, str);

			    /* check validity */
			    if ((strcmp (OutputFormat, "intel") != 0) &&
				(strcmp (OutputFormat, "sparc") != 0)) {
				    fprintf (stderr, "Invalid OutputFormat %s\n", OutputFormat);
				    return EW_FAILURE;
				}
			    init[9] = 1;
			}
		    /*10*/    else if( k_its("InputMethod") ) {
			if ((str = k_str()) != NULL)
			    strcpy (InputMethod, str);
			init[10] = 1;
		    }


		    /*  */    else if (k_its ("TrigFile")) {
			if ((str = k_str ()) != NULL)
			    strcpy (TrigFile, str);
			TrigFileSet = 1;
		    }

		    /*  */    else if (k_its ("SaveSCNL")) { 
			if (n_save_stations >= MAX_STATIONS) {
			    logit ("e",
				   "Too many <SaveSCNL> commands in <%s>",
				   configfile);
			    logit ("e", "; max = %d; exitting!\n", (int) MAX_STATIONS);
			    return EW_FAILURE;
			}

			if ((str = k_str ()) != NULL)
			    strcpy ((SaveStations + n_save_stations)->sta, str);
			if ((str = k_str()) != NULL)
			    strcpy ((SaveStations + n_save_stations)->chan, str);
			if ((str = k_str()) != NULL)
			    strcpy ((SaveStations + n_save_stations)->net, str);
			if ((str = k_str()) != NULL)
			    strcpy ((SaveStations + n_save_stations)->loc, str);
			n_save_stations++;

			SaveSCNLSet = 1;
		    }

		    /*NR*/    else if (k_its ("StartTime")) {
			if ((str = k_str ()) != NULL) {
			    if ((StartTime = malloc (MAXTXT)) == NULL) {
				logit ("e", "Couldn't malloc StartTime.\n");
				return EW_FAILURE;
			    }
			    strcpy (StartTime, str);
			}
		    }
		    /*NR*/    else if( k_its("Duration") ) {
			reqDuration = k_val();
		    }

		    /* Unknown command*/
		    else {
			logit ("e", "<%s> Unknown command in <%s>.\n",
			       com, configfile );
			continue;
		    }

		    /* See if there were any errors processing the command */
		    if( k_err() ) 
			{
			    logit ("e",
				   "Bad <%s> command in <%s>; exitting!\n",
				   com, configfile );
			    return EW_FAILURE;
			}
	    } /* while k_rd() */

	    nfiles = k_close();
    } /* while nfiles > 0 */

    /* After all files are closed, check init flags for missed commands*/

    nmiss = 0;
    for (i = 0; i < ncommand; i++)  
	if (!init[i]) nmiss++;

    if (nmiss != 0) {
	logit ("e", "waveman2disk: ERROR, no " );
	if ( !init[0] )  logit ("e", "<LogFile> "       );
	if ( !init[1] )  logit ("e", "<WaveServer> "    );
	if ( !init[2] )  logit ("e", "<TimeoutSeconds> ");
	if ( !init[3] )  logit ("e", "<MaxTraces> "     );
	if ( !init[4] )  logit ("e", "<TraceBufferLen> ");
	if ( !init[5] )  logit ("e", "<GapThresh> "   );
	if ( !init[6] )  logit ("e", "<MinDuration> "   );
	if ( !init[7] )  logit ("e", "<DataFormat> "   );
	if ( !init[8] )  logit ("e", "<OutDir> "   );
	if ( !init[9] )  logit ("e", "<OutputFormat> "   );
	if ( !init[10] ) logit ("e", "<InputMethod> "   );
	logit ("e", "command(s) in <%s>; exitting!\n", configfile );
	return EW_FAILURE;
    }

    /* make sure that correct options are set, based on InputMethod */
    if (strcmp (InputMethod, "triglist") == 0) {
	if (TrigFileSet != 1) {
	    logit ("e", "ERROR, no <TrigFile> command in <%s>; exitting!\n",
		   configfile);
	    return EW_FAILURE;
	}
    }
    else if (strcmp (InputMethod, "interactive") == 0) {
	if (SaveSCNLSet != 1) {
	    logit ("e", "ERROR, no <SaveSCNL> command in <%s>; exitting!\n",
		   configfile);
	    return EW_FAILURE;
	}
    } else {
	logit ("e", "Invalid InputMethod %s\n", InputMethod);
	    return EW_FAILURE;
	}

    return EW_SUCCESS;
}

/****************************************************************************
 * given a pointer to a snippet structure (as parsed from the trigger       *
 * message) and the array of blank trace request structures, fill them with *
 * the requests implied by the snippet. Don't overflow, and return the      *
 * number of requests generated.                                            *
 *                                                                          *
 * args: pmenu: pointer to the client routine's menu list                   *
 *      pSnppt: pointer to the structure containing the parsed trigger line *
 *              from the trigger message.                                   *
 *      pTrReq: pointer to arrray of trace request forms                    *
 * maxTraceReq: size of this array                                          *
 *  pnTraceReq: pointer to count of currently used trace request forms      *
 ****************************************************************************/
static int snippet2trReq(WS_MENU_QUEUE_REC* pMenuList, SNIPPET* pSnppt, 
                         TRACE_REQ* pTrReq, int maxTraceReq, int* pnTraceReq )
{
    int ret =  WS_ERR_SCNL_NOT_IN_MENU;
    WS_MENU server; 

    if ((pMenuList == NULL) || (pSnppt == NULL) || (pTrReq == NULL) ||
	(pnTraceReq == NULL) || (maxTraceReq < 0)) {
	logit ("e", "Invalid arguments passed in\n");
	return WS_ERR_INPUT;
    }

    server = pMenuList->head;

    if(Debug == 1) 
	logit("","Entering snippet2trReq\n");      

    while ( server != NULL ) {
	WS_PSCNL tank = server->pscnl;

	if(Debug == 1) 
	    logit("","Searching through Server %s:%s:\n",server->addr,server->port);  
	while ( tank != NULL ) {
	    if (strlen(tank->loc) > 0) {
		if ( matchSCNL(pSnppt, tank) == 1 && 
		     duplicateSCNL(tank, pTrReq, *pnTraceReq) == 0 ) {
		    ret = WS_ERR_NONE;
		    strcpy( pTrReq[*pnTraceReq].sta, tank->sta  );
		    strcpy( pTrReq[*pnTraceReq].chan,tank->chan );
		    strcpy( pTrReq[*pnTraceReq].net, tank->net  );
		    strcpy( pTrReq[*pnTraceReq].loc, tank->loc );
		    (*pnTraceReq)++;
		    if( *pnTraceReq >= maxTraceReq) {
			logit("","snippet2trReq: overflowed trace request array\n");
			return WS_ERR_BUFFER_OVERFLOW;
		    }
		}
	    } else {
		if ( matchSCN(pSnppt, tank) == 1 && 
		     duplicateSCN(tank, pTrReq, *pnTraceReq) == 0 ) {
		    ret = WS_ERR_NONE;
		    strcpy( pTrReq[*pnTraceReq].sta, tank->sta  );
		    strcpy( pTrReq[*pnTraceReq].chan,tank->chan );
		    strcpy( pTrReq[*pnTraceReq].net, tank->net  );
		    pTrReq[*pnTraceReq].loc[0] = '\0';
		    (*pnTraceReq)++;
		    if (strcmp(pSnppt->loc, "--") && strcmp(pSnppt->loc, "*"))
			logit("et", "snippet2trReq WARNING: specific location code <%s>"
			      "served by SCN server\n", pSnppt->loc);
		    if( *pnTraceReq >= maxTraceReq) {
			logit("","snippet2trReq: overflowed trace request array\n");
			return WS_ERR_BUFFER_OVERFLOW;
		    }
		}
	    }
	    tank = tank->next;
	}  /* End while(Tank) */

	server = server->next;
    }    /* End While (server) */

    if ( ret == WS_ERR_EMPTY_MENU )   
	logit( "","snippet2trReq(): Empty menu\n" );

    return ret;
}


/***********************************************************************
 *   helper routine for snippet2trReq above: See if the SCNL name in the *
 *   Snippet structure matches the menu item. Wildcards `*'allowed      *
 ************************************************************************/
static int matchSCNL( SNIPPET* pSnppt, WS_PSCNL pscnl)
{
    int staMatch =0;
    int netMatch =0;
    int chanMatch=0;
    int locMatch =0;
    
    if (strcmp( pSnppt->sta , "*") == 0)  staMatch =1;
    if (strcmp( pSnppt->chan, "*") == 0)  chanMatch=1;
    if (strcmp( pSnppt->net , "*") == 0)  netMatch =1;
    if (strcmp( pSnppt->loc , "*") == 0)  locMatch =1;
    if (staMatch+netMatch+chanMatch+locMatch == 4) 
	return(1);

    if ( !staMatch  && strcmp( pSnppt->sta,  pscnl->sta  )==0 ) staMatch=1;
    if ( !chanMatch && strcmp( pSnppt->chan, pscnl->chan )==0 ) chanMatch=1;
    if ( !netMatch  && strcmp( pSnppt->net,  pscnl->net  )==0 ) netMatch=1;
    if ( !locMatch  && strcmp( pSnppt->loc,  pscnl->loc  )==0 ) locMatch=1;

    if (staMatch+netMatch+chanMatch+locMatch == 4) 
	return(1);
    else
	return(0);
}
static int matchSCN( SNIPPET* pSnppt, WS_PSCNL pscnl)
{
    int staMatch =0;
    int netMatch =0;
    int chanMatch=0;
    
    if (strcmp( pSnppt->sta , "*") == 0)  staMatch =1;
    if (strcmp( pSnppt->chan, "*") == 0)  chanMatch=1;
    if (strcmp( pSnppt->net , "*") == 0)  netMatch =1;
    if (staMatch+netMatch+chanMatch == 3) 
	return(1);

    if ( !staMatch  && strcmp( pSnppt->sta,  pscnl->sta  )==0 ) staMatch=1;
    if ( !chanMatch && strcmp( pSnppt->chan, pscnl->chan )==0 ) chanMatch=1;
    if ( !netMatch  && strcmp( pSnppt->net,  pscnl->net  )==0 ) netMatch=1;

    if (staMatch+netMatch+chanMatch == 3) 
	return(1);
    else
	return(0);
}


/************************************************************************
 *   helper routine for snippet2trReq above: See if the SCNL name in the *
 *   Snippet structure is a duplicate of any existing request           *
 ************************************************************************/
static int duplicateSCNL( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq)
{
    int i;

    for (i=0; i<nTraceReq; i++) {
	if( strcmp( pTrReq[i].sta, pscnl->sta  )==0 &&
	    strcmp( pTrReq[i].net, pscnl->net  )==0 &&
	    strcmp( pTrReq[i].chan,pscnl->chan )==0 &&
	    strcmp( pTrReq[i].loc, pscnl->loc  )==0 ) 
	    return(1); /* meaning: yes, it's a duplicate */
    }
    return(0); /* 'twas not a duplicate */
}
static int duplicateSCN( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq)
{
    int i;

    for (i=0; i<nTraceReq; i++) {
	if( strcmp( pTrReq[i].sta, pscnl->sta  )==0 &&
	    strcmp( pTrReq[i].net, pscnl->net  )==0 &&
	    strcmp( pTrReq[i].chan,pscnl->chan )==0 )
	    return(1); /* meaning: yes, it's a duplicate */
    }
    return(0); /* 'twas not a duplicate */
}



/* 
 *  Look into the Snippet, and return strings which may be   
 *  useful in file and directory names: 
 *    EventId, EventTime (maybe more in the future)
 */
static int GetInfoFromSnippet(SNIPPET snpptStruc)
{
    int	i, j;

    if (&snpptStruc == NULL) {
	logit ("e", "Invalid argument passed in; exitting!\n");
	return EW_FAILURE;
    }
		
    /**** EventID *****/
    i=EVENTID_SIZE;
    if(MAXTXT<i)i=MAXTXT;
    strncpy (EventID, snpptStruc.eventId,i);
    EventID[i-1]='\0';

    /**** EventDate *****/
    strcpy (EventDate, snpptStruc.startYYYYMMDD);


    /**** EventTime *****/
    /* must strip off : delimiters */
    j = 0;
    for (i = 0; i < TIME_CHARS; i++) {
	if (snpptStruc.startHHMMSS[i] != ':') {
	    EventTime[j] = snpptStruc.startHHMMSS[i];
	    j++;
	}
    }
    EventTime[j] = '\0';

    return EW_SUCCESS;
}
