/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: liss2ew_scnl.cpp 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2006/05/26 13:16:48  paulf
 *     reportErrorCleanup call commented out
 *
 *     Revision 1.3  2006/05/26 13:10:48  paulf
 *     patched up reportError.c hacks and liss2ew_scnl to match
 *
 *     Revision 1.2  2006/05/25 18:23:37  paulf
 *     fixes for winnt compiling
 *
 *     Revision 1.1  2006/05/25 15:49:51  paulf
 *     added for the first time
 *
 *     Revision 1.2  2006/03/11 00:00:06  davidk
 *     Fixed a bug where tracebuf version info was not being written to the
 *     trace header.
 *     Also initialized the trace_header prior to filling in values so all padding should
 *     be 0.
 *
 *     Revision 1.1  2005/06/30 20:40:12  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:51:30  mark
 *     Initial checkin
 *
 *     Revision 1.7  2003/06/18 20:03:30  patton
 *     Added warning message, see below.
 *
 *     Revision 1.6  2003/06/17 19:25:03  patton
 *     *** empty log message ***
 *
 *
 *     Revision 1.5  2003/06/16 22:07:35  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.4  2000/03/13 23:50:24  lombard
 *     Removed some DCC code that had a memory leak (Decode_SEED())
 *     Added initialization of decompression struct to Configure()
 *     Initialized Param.debug.
 *     Added soem error checking for Steim decompression.
 *
 *     Revision 1.3  2000/03/10 18:40:50  lombard
 *     moved memset of SCNL; best not to memset things after you have read
 *     into them!
 *
 *     Revision 1.2  2000/03/09 20:13:03  lombard
 *     Fixed  initialized memory in pscnB.
 *     Added check for `keepalive' record from LISS.
 *
 *     Revision 1.1  2000/03/05 21:45:50  lombard
 *     Initial revision
 *
 *
 *
 */

/****************************************************************************
 * WARNING: liss2ew has been observed intermittantly producing malformed    *
 * TRACE_BUF messages.  Currently the conditions for causing this problem   *
 * are unknown.  Due to this, liss2ew should be treated as suspect.  Use at *
 * your own risk.  JMP 6-18-2003                                            *
 ***************************************************************************/ 


/****************************************************************************
 * LISS2ew module: reads miniSEED from one LISS server and writes TRACE_BUF *
 * messages to earthworm transport ring. Pete Lombard, 1/31/2000            *
 *                                                                          *
 * WARNING: Much of this code is based on dumpseed from the DCC at ASL.     *
 * Their byte-swapping conventions are confusing at best! I make no         *
 * apologies for their code, either. PNL                                    *
 ****************************************************************************/

/* System includes */
#include <stdio.h>

/* Earthworm includes */
extern "C"
{
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <socket_ew.h>
#include <watchdog_client.h>
}
#include "liss2ew_scnl.h"

LISSConnection	*ConnInfo = NULL;
/*volatile*/ bool	*bThreadRunning;
volatile WORLD	 L2e;

volatile int	 ExitFlag;

int NumConnections;
int AllocedConnections;

thr_ret ReceiveThread(void *argptr);
void FreeLiss();

int main( int argc, char **argv)
{
	time_t  timeNow;                   /* current time                  */ 
	time_t  timeLastBeat;              /* time last heartbeat was sent  */
	int i;
  
	if (argc != 2)
	{
		fprintf (stderr, "Usage: %s <configfile>\n", argv[0]);
		return (EW_FAILURE);
	}

	reportErrorInit(MAXMESSAGELEN, 1, argv[0]); 

	/* Configure me! */
	if (Configure(argv) != EW_SUCCESS)
	{
		// reportErrorCleanup();
		return (EW_FAILURE);
	}

	if (L2e.Param.logSwitch != 1)
	{
		/* Close and restart logging, without file logging. */
		// reportErrorCleanup();
		reportErrorInit(MAXMESSAGELEN, L2e.Param.logSwitch, argv[0]); 
	}

	/* Initialize the socket system */
	SocketSysInit();

	/* Force a heartbeat to be issued in first pass thru main loop     */
	timeLastBeat = 0;

	ExitFlag = 0;
	for (i = 0; i < NumConnections; i++)
	{
		ConnInfo[i].thread_index = i;
		if (StartThreadWithArg(ReceiveThread, (void *)&ConnInfo[i], 0, &(ConnInfo[i].thread_id)) != 0)
		{
			reportError(WD_FATAL_ERROR, SYSERR, "Unable to start thread %d\n", i); 
			FreeLiss();
			return (EW_FAILURE);
		}
		reportError(WD_DEBUG, 0, "Started thread %d\n", i); 
		bThreadRunning[i] = true;
	}

	/*  Main message processing loop  */
	while ( tport_getflag( (SHM_INFO *)&(L2e.regionOut) ) != TERMINATE )
	{
		/*  Check for need to send heartbeat message  */
		if ( time( &timeNow ) - timeLastBeat >= L2e.Param.heartbeatInt )
		{
			timeLastBeat = timeNow;
			StatusReport( L2e.Ewh.typeHeartbeat, 0, "" ); 
		}

		/* Check that all our threads are running; if not, restart them. */
		for (i = 0; i < NumConnections; i++)
		{
			/* See if the thread has exited */
			if (bThreadRunning[i] == false)
			{
				/* Restart the thread. */
				ConnInfo[i].thread_index = i;
				if (StartThreadWithArg(ReceiveThread, (void *)&ConnInfo[i], 0, &(ConnInfo[i].thread_id)) != 0)
				{
					reportError(WD_FATAL_ERROR, SYSERR, "Unable to start thread %d\n", i);
					FreeLiss();
					return (EW_FAILURE);
				}
				bThreadRunning[i] = true;
			}
		}

		sleep_ew(1000);
	}  /* End of main loop; time to close up shop. */

	reportError(WD_INFO, 0, "Terminate message received\n"); 

	/* Exit all running threads, and free up memory. */
	FreeLiss();

	return EW_SUCCESS;
}

void FreeLiss()
{
	int i, j;

	ExitFlag = 1;
	sleep_ew(2000);

	for (i = 0; i < NumConnections; i++)
	{
		if (bThreadRunning[i] == true)
		{
			KillThread(ConnInfo[i].thread_id);
		}

		free(ConnInfo[i].inBuf);
		free(ConnInfo[i].dcp);
		for (j = 0; j < ConnInfo[i].Nscn; j++)
		{
			free(ConnInfo[i].pscnB[j].buf);
		}

		if (ConnInfo[i].pSocket)
			delete ConnInfo[i].pSocket;
	}

	delete []ConnInfo;
	delete []bThreadRunning;

	tport_detach((SHM_INFO *)&(L2e.regionOut));

	reportError(WD_DEBUG, 0, "Memory cleanup completed.\n"); 
	// reportErrorCleanup(); 
}

thr_ret ReceiveThread(void *argptr)
{
	LISSConnection *pConnInfo = (LISSConnection *)argptr;
	STATUS state;          /* current and reported state      */
	int retval, err;
	int retry = 0;
	int nread;

	if (pConnInfo->pSocket)
		delete pConnInfo->pSocket;

	pConnInfo->pSocket = new CTCPSocket(pConnInfo->IPaddr, pConnInfo->IPport);
	if (pConnInfo->pSocket->InitForConnect() == SOCKET_ERROR)
	{
		logit("t", "Error initializing socket for <%s:%d>\n", pConnInfo->IPaddr, pConnInfo->IPport);
		bThreadRunning[pConnInfo->thread_index] = false;
		return;
	}
	pConnInfo->pSocket->SetTimeout(L2e.sockTimeout);
	state = closed;

	while (ExitFlag == 0)
	{
		switch (state)
		{
		case closed:
			/* Open a nonblocking socket for the LISS connection */
			retval = pConnInfo->pSocket->Connect();
			if (retval != 0)
			{
				/* No connection; see if we merely timed out, or something more sinister
				 * is at work...
				 */
				if (retval != WOULDBLOCK_EW)
				{
					logit("t", "Error connecting to <%s:%d>; exiting thread.\n",
							pConnInfo->IPaddr, pConnInfo->IPport);
					bThreadRunning[pConnInfo->thread_index] = false;
					return;
				}

				retry++;
				state = closed;
				if (retry > RETRIES_EXIT)
				{
					logit("et", "Still no connection after %d tries, exiting thread.\n",
							retry);
					bThreadRunning[pConnInfo->thread_index] = false;
					return;
				}

				if (retry == RETRIES_LOG)
				{
					logit("et", "no connection after %d tries; still trying.\n",
						  retry);
				}
			} 
			else
			{
				logit("et", "connected to %s:%d\n", pConnInfo->IPaddr, pConnInfo->IPport);
				retry = 0;
				state = connected;
			}
			break;

		case connected:
			/* Got a connection, now try to use it */
			nread = pConnInfo->pSocket->ReceiveAll((unsigned char *)pConnInfo->inBuf, L2e.lenSEED);
			if (nread < L2e.lenSEED)
			{
				err = socketGetError_ew();
				retry++;
				if (err == WOULDBLOCK_EW)
				{
					if (retry == 1)
					{
						logit("et", "timed out reading from %s:%d; trying again.\n",
								pConnInfo->IPaddr, pConnInfo->IPport);
					}
					else if (retry == RETRIES_RECONN)
					{
						logit("et", "nothing read in %d minutes; reopening socket\n",
								retry * L2e.sockTimeout / 60000);
						pConnInfo->pSocket->CloseSocket();
						state = closed;
					}
					break;
				}
				else
				{
					logit("et", "fatal error occured while reading from %s:%d: %s\n",
							pConnInfo->IPaddr, pConnInfo->IPport, strerror(err));
					logit("t", "Exiting thread\n");
					pConnInfo->pSocket->CloseSocket();
					bThreadRunning[pConnInfo->thread_index] = false;
					return;
				}
			}

			/* We finally read something! */
			retry = 0;
			if (ProcessData(pConnInfo) != EW_SUCCESS)
			{
				logit("et", "fatal error processing LISS data.\n");
				reportError(WD_FATAL_ERROR, GENFATERR, "Exiting thread\n"); 
				pConnInfo->pSocket->CloseSocket();
				bThreadRunning[pConnInfo->thread_index] = false;
				return;
			}
			break;

		default:;
		}

/*		sleep_ew(25);*/
	}

	bThreadRunning[pConnInfo->thread_index] = false;
}

  
/* StatusReport: Send error and hearbeat messages to transport ring     */
void StatusReport( unsigned char type, short code, 
                   char* message )
{
	char          outMsg[MAXMESSAGELEN];  /* The outgoing message.        */
	long          msgTime;        /* Time of the message.                 */

	/*  Get the time of the message                                       */
	msgTime = (long)time( NULL );

	/*  Build & process the message based on the type                     */
	if ( L2e.Ewh.typeHeartbeat == type )
	{
		sprintf( outMsg, "%ld %ld\n\0", msgTime, L2e.MyPid );

		/*Write the message to the output region                            */
		if ( tport_putmsg( (SHM_INFO *)&(L2e.regionOut), (MSG_LOGO *)&(L2e.hrtLogo), 
						   (long) strlen( outMsg ), outMsg ) != PUT_OK )
		{
			/*     Log an error message                                       */
			logit( "et", "liss2ew: Failed to send a heartbeat message (%d).\n",
				 code );
		}
	}
	else
	{
		if ( message ) 
		{
			sprintf( outMsg, "%ld %hd %s\n\0", msgTime, code, message );
			logit("t","Error:%d (%s)\n", code, message );
		}
		else 
		{
			sprintf( outMsg, "%ld %hd\n\0", msgTime, code );
			logit("t","Error:%d (No description)\n", code );
		}

		/*Write the message to the output region                         */
		if ( tport_putmsg( (SHM_INFO *)&(L2e.regionOut), (MSG_LOGO *)&(L2e.errLogo), 
						   (long) strlen( outMsg ), outMsg ) != PUT_OK )
		{
			/*     Log an error message                                    */
			logit( "et", "liss2ew: Failed to send an error message (%d).\n",
				 code );
		}
	}
}


/* Jumbo configuration routine: read config file, look up EW parameters.
 * initialize a bunch of values, start up logit and attach to the transport
 * ring. Exits on failure, returns nothing. */
int Configure( char **argv)
{
	int i, j;

	/* Some important initial values or defaults */
	L2e.lenSEED = DEF_LEN_SEED;
	L2e.sockTimeout = 0;
	L2e.Param.debug = 0;
  
	/* Read config file and configure liss2ew */
	if (ReadConfig(argv[1]) == EW_FAILURE)
	{
		logit("e", "%s: configure() failed \n", argv[0]);
		return EW_FAILURE;
	}

	/*    Look up important info from earthworm tables                  */
	if ( ReadEWH() == EW_FAILURE )
	{
		logit("e", "%s error in ReadEWH.\n", argv[0] );
		return EW_FAILURE;
	}

	/* Set up logos for outgoing messages */
	L2e.hrtLogo.instid = L2e.Ewh.myInstId;
	L2e.hrtLogo.mod    = L2e.Ewh.myModId;
	L2e.hrtLogo.type   = L2e.Ewh.typeHeartbeat;

	L2e.errLogo.instid = L2e.Ewh.myInstId;
	L2e.errLogo.mod    = L2e.Ewh.myModId;
	L2e.errLogo.type   = L2e.Ewh.typeError;

	L2e.waveLogo.instid = L2e.Ewh.myInstId;
	L2e.waveLogo.mod    = L2e.Ewh.myModId;
	L2e.waveLogo.type   = L2e.Ewh.typeWaveform;

	/* Get my process ID so I can let statmgr restart me */
	L2e.MyPid = getpid();

	for (i = 0; i < NumConnections; i++)
	{
		/* Allocate memory for our input buffers */
		if ( (ConnInfo[i].inBuf = (char *)malloc(L2e.lenSEED)) == (char *)NULL)
		{
			logit("e", "%s: error allocating input buffer.\n", argv[0]);
			return EW_FAILURE;
		}

		/* Initialize SCN_BUFFER structures */
		for (j = 0; j < ConnInfo[i].Nscn; j++)
		{
			ConnInfo[i].pscnB[j].quality[0] = ConnInfo[i].pscnB[j].quality[1] = 0;
			ConnInfo[i].pscnB[j].starttime = 0.0;
			ConnInfo[i].pscnB[j].endtime = 0.0;
			ConnInfo[i].pscnB[j].samplerate = -1.0;
			ConnInfo[i].pscnB[j].writeP = 0;
			if ( (ConnInfo[i].pscnB[j].buf = (long *)malloc(L2e.traceLen * sizeof(long)))
					== (long *)NULL)
			{
				logit("e", "Error allocating memory for trace buffer; exiting\n");
				return EW_FAILURE;
			}
		}

		/* Intialize the decompression info.  This function will malloc the space for it;
		 * it's up to us to free said memory later.
		 */
		if ( (ConnInfo[i].dcp = init_generic_decompression()) == NULL)
		{
			logit("e", "Error allocating memory for decompression structure; exiting\n");
			exit( EW_FAILURE );
		}

		ConnInfo[i].thread_id = 0;
		ConnInfo[i].thread_index = -1;
		ConnInfo[i].pSocket = NULL;
	}

	/* Attach to Output transport ring */
	tport_attach( (SHM_INFO *)&(L2e.regionOut), L2e.Ewh.ringOutKey);

	return EW_SUCCESS;
}

 
/*  Read some values from earthworm master ID file */
int ReadEWH()
{
	if ( GetLocalInst( (unsigned char *)&(L2e.Ewh.myInstId)) != 0 )
	{
		logit("e", "liss2ew: Error getting myInstId.\n" );
		return EW_FAILURE;
	}

	if ( GetModId( (char *)L2e.Param.myModName, (unsigned char *)&(L2e.Ewh.myModId)) != 0 )
	{
		logit("e", "liss2ew: Error getting myModId.\n" );
		return EW_FAILURE;
	}

	if ((L2e.Ewh.ringOutKey = GetKey ((char *)L2e.Param.ringOut) ) == -1) 
	{
		logit("e", "liss2ew:  Invalid ring name <%s>; exiting!\n", 
				 L2e.Param.ringOut);
		return EW_FAILURE;
	}

	/* Look up message types of interest */
	if (GetType ("TYPE_HEARTBEAT", (unsigned char *)&(L2e.Ewh.typeHeartbeat)) != 0) 
	{
		logit("e", "liss2ew: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_ERROR", (unsigned char *)&(L2e.Ewh.typeError)) != 0) 
	{
		logit("e", "liss2ew: Invalid message type <TYPE_ERROR>; exiting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_TRACEBUF2", (unsigned char *)&(L2e.Ewh.typeWaveform)) != 0) 
	{
		logit("e", "liss2ew: Invalid message type <TYPE_TRACEBUF2>; exiting!\n");
		return EW_FAILURE;
	}

	return EW_SUCCESS;
} 

#define NUMREQ          5       /* Number of parameters that MUST be    */
                                /*   set from the config file.          */
/*      Function: ReadConfig                                            */
int ReadConfig (char *configfile )
{
	LISSConnection	*TempConnInfo;
	bool			*bTempThreadRunning;
	char    		*com;
	char    		*str;
	int      		nfiles;
	int      		success;
	int      		i;
	int				Iscn = 0;
	int				CurConnection = -1;
	/* number of required commands that were missed   */
	int      		nmiss;
	/* init flags, one byte for each required command */
	char     		init[NUMREQ];     

	AllocedConnections = 100;
	ConnInfo = new LISSConnection[AllocedConnections];
	bThreadRunning = new bool[AllocedConnections];

	// Initialize the new memory.
	for (i = 0; i < AllocedConnections; i++)
	{
		ConnInfo[i].Nscn = 0;
		ConnInfo[i].inBuf = NULL;
		ConnInfo[i].outBuf = NULL;
		ConnInfo[i].outBufLen = 0;

		bThreadRunning[i] = false;
	}
	NumConnections = 0;

	/* Set to zero one init flag for each required command */
	for (i = 0; i < NUMREQ; i++)
		init[i] = 0;

	/* Open the main configuration file 
	**********************************/
	nfiles = k_open (configfile); 
	if (nfiles == 0) 
	{
		logit("e", "liss2ew: Error opening command file <%s>; exiting!\n", 
				 configfile);
		return EW_FAILURE;
	}

	/* Process all command files
	***************************/
	while (nfiles > 0)   /* While there are command files open */
	{
		while (k_rd ())        /* Read next line from active file  */
		{  
			com = k_str ();         /* Get the first token from line */

			/* Ignore blank lines & comments
			*******************************/
			if (!com)
				continue;
			if (com[0] == '#')
				continue;

			/* Open a nested configuration file */
			if (com[0] == '@') 
			{
				success = nfiles + 1;
				nfiles  = k_open (&com[1]);
				if (nfiles != success) 
				{
					logit("e", "liss2ew: Error opening command file <%s>; exiting!\n", 
						   &com[1]);
					return EW_FAILURE;
				}
				continue;
			}

			/* Process anything else as a command */
			/*0*/ 	if (k_its ("MyModId")) 
			{
				if ( str = k_str () )
				{
					if (strlen(str) >= MAXMODNAMELEN)
					{
						logit("e", "MyModId too long; max is %d\n", MAXMODNAMELEN -1);
						return EW_FAILURE;
					}
					strcpy ((char *)L2e.Param.myModName, str);
					init[0] = 1;
				}
			}
			/*1*/	else if (k_its ("OutRing")) 
			{
				if ( str = k_str () )
				{
					if (strlen(str) >= MAXRINGNAMELEN)
					{
						logit("e", "OutRing name too long; max is %d\n", 
								MAXRINGNAMELEN - 1);
						return EW_FAILURE;
					}
					strcpy ((char *)L2e.Param.ringOut, str);
					init[1] = 1;
				}
			}
			/*2*/	else if (k_its ("HeartBeatInterval")) 
			{
				L2e.Param.heartbeatInt = k_long ();
				init[2] = 1;
			}

			/*3*/     else if (k_its ("LogFile"))
			{
				L2e.Param.logSwitch = k_int();
				init[3] = 1;
			}

			/*4*/ else if ( k_its ( "TraceLength"))
			{
				L2e.traceLen = k_int();
				if (L2e.traceLen < 10 && 
						L2e.traceLen > (MAX_TRACEBUF_SIZ - sizeof(TRACE2_HEADER))/sizeof(long))
				{
					logit("e", "TraceLength %d is outside of range (%d - %d.\n",
						  L2e.traceLen, 10, (MAX_TRACEBUF_SIZ 
											   - sizeof(TRACE2_HEADER))/sizeof(long));
					return EW_FAILURE;
				}
				init[4] = 1;
			}

			else if (k_its ("Address")) 
			{
				if (CurConnection >= AllocedConnections)
				{
					logit("", "Reallocating connection memory for %d connections...\n",
							AllocedConnections + 100);

					// We need to allocate more memory...
					TempConnInfo = new LISSConnection[AllocedConnections + 100];
					bTempThreadRunning = new bool[AllocedConnections + 100];
					
					// Copy the data from the old memory to the temporary memory.
					memcpy(TempConnInfo, ConnInfo, AllocedConnections * sizeof(LISSConnection));
					memcpy(bTempThreadRunning, bThreadRunning, AllocedConnections * sizeof(bool));

					// Delete the old memory, and move the pointers to the temporary memory.
					delete []ConnInfo;
					delete []bThreadRunning;
					ConnInfo = TempConnInfo;
					bThreadRunning = bTempThreadRunning;

					// Initialize the new, as-yet-unfilled memory.
					for (i = AllocedConnections; i < AllocedConnections + 100; i++)
					{
						ConnInfo[i].Nscn = 0;
						ConnInfo[i].inBuf = NULL;
						ConnInfo[i].outBuf = NULL;
						ConnInfo[i].outBufLen = 0;

						bThreadRunning[i] = false;
					}

					AllocedConnections += 100;
				}
				if (NumConnections > 0 && Iscn == 0)
				{
					logit("e", "No SCNL specified for %s:%d\n", ConnInfo[CurConnection].IPaddr,
							ConnInfo[CurConnection].IPport);
					return EW_FAILURE;
				}

				if ( str = k_str () )
				{
					if (strlen(str) >= MAXADDRLEN)
					{
						logit("e", "Address too long; max is %d\n", MAXADDRLEN);
						return EW_FAILURE;
					}

					CurConnection++;
					NumConnections++;
					Iscn = 0;

					strcpy (ConnInfo[CurConnection].IPaddr, str);
					ConnInfo[CurConnection].IPport = k_int();
					if (ConnInfo[CurConnection].IPport <= 0)
					{
						logit("e", "Invalid port for %s\n", ConnInfo[CurConnection].IPaddr);
						return EW_FAILURE;
					}
				}
			}
                
			else if( k_its("SCNL") ) 
			{
				if (CurConnection < 0)
				{
					logit("e", "No connections specified before SCNL list.\n");
					return EW_FAILURE;
				}
				if( Iscn >= MAX_SCN_PER_CONN )
				{
					logit("e", "Too many SCNLs for %s:%d (max %d)\n",
							ConnInfo[CurConnection].IPaddr, ConnInfo[CurConnection].IPport,
							MAX_SCN_PER_CONN);
					return EW_FAILURE;
				}

				/* read station code */
				str = k_str();
				if (str != (char *)NULL)
				{
					if( strlen(str) >= TRACE2_STA_LEN )
					{
						logit("e", "  sta code <%s> too long; maxchar=%d\n", str, TRACE2_STA_LEN-1 );
						return EW_FAILURE;
					}
					strcpy( ConnInfo[CurConnection].pscnB[Iscn].sta, str );
				}
				else
				{
					logit("e", "Invalid SCNL: missing <sta>\n" );
					return EW_FAILURE;
				}

				/* read component code */
				str = k_str();
				if (str != (char *)NULL)
				{
					if( strlen(str) >= TRACE2_CHAN_LEN )
					{
						logit("e", "  comp code <%s> too long; maxchar=%d\n", str, TRACE2_CHAN_LEN-1 );
						return EW_FAILURE;
					}
					strcpy( ConnInfo[CurConnection].pscnB[Iscn].chan, str );
				}
				else
				{
					logit("e", "Invalid SCNL: missing <sta>\n" );
					return EW_FAILURE;
				}

				/* read network code */
				str = k_str();
				if (str != (char *)NULL)
				{
					if( strlen(str) >= TRACE2_NET_LEN )
					{
						logit("e", "  net code <%s> too long; maxchar=%d\n", str, TRACE2_NET_LEN-1 );
						return EW_FAILURE;
					}
					strcpy( ConnInfo[CurConnection].pscnB[Iscn].net, str );
				}
				else
				{
					logit("e", "Invalid SCNL: missing <sta>\n" );
					return EW_FAILURE;
				}

				/* read location code */
				str = k_str();
				if (str != (char *)NULL)
				{
					if( strlen(str) >= TRACE2_LOC_LEN )
					{
						logit("e", "  loc code <%s> too long; maxchar=%d\n", str, TRACE2_LOC_LEN-1 );
						return EW_FAILURE;
					}
					strcpy( ConnInfo[CurConnection].pscnB[Iscn].lc, str );
				}
				else
				{
					logit("e", "Invalid SCNL: missing <sta>\n" );
					return EW_FAILURE;
				}

				ConnInfo[CurConnection].pscnB[Iscn].pinno = 0;

				/* Increment the SCNL count for this IP address. */
				Iscn++;
				ConnInfo[CurConnection].Nscn = Iscn;
			}

			else if ( k_its ( "lenSEED"))
			{
				L2e.lenSEED = k_int();
				if (L2e.lenSEED < 1)
				{
					logit("e", "lenSEED is 0; this won't work.\n");
					return EW_FAILURE;
				}
			}
      
			else if ( k_its ( "SocketTimeout"))
			{
				L2e.sockTimeout = k_int(); /* Convert to milliseconds below */
			}
      
			/* Optional: debug command */
			else if (k_its( "Debug") )
			{
				L2e.Param.debug = 1;
			}
      
			/* Unknown command */ 
			else 
			{
				logit("e", "Unknown command <%s> in config file.\n", com);
				continue;
			}

			/* See if there were any errors processing the command */
			if (k_err ()) 
			{
				logit("e", "Error in config file\n\t%s\n",
						 k_com());
				return EW_FAILURE;
			}

		} /** while k_rd() **/

		nfiles = k_close();

	} /** while nfiles **/

	/* After all files are closed, check init flags for missed commands */
	nmiss = 0;
	for (i = 0; i < NUMREQ; i++)  
		if (!init[i]) 
			nmiss++;

	if (nmiss) 
	{
		logit("e", "liss2ew: ERROR, no ");
		if (!init[0])  logit("e", "<MyModId> ");
		if (!init[1])  logit("e", "<OutRing> ");
		if (!init[2])  logit("e", "<HeartBeatInterval> ");
		if (!init[3])  logit("e", "<LogFile> ");
		if (!init[4])  logit("e", "<TraceLength> ");

		logit("e", "command(s) in <%s>; exiting!\n", configfile);
		return EW_FAILURE;
	}
	if (NumConnections <= 0)
	{
		logit("e", "Error: No IP addresses or SCNLs specified!\n");
		return EW_FAILURE;
	}

	if (L2e.sockTimeout > L2e.Param.heartbeatInt || L2e.sockTimeout <= 0)
		L2e.sockTimeout = L2e.Param.heartbeatInt;
  
	L2e.sockTimeout *= 1000; /* We use milliseconds internally */

	return EW_SUCCESS;
}


/* Process miniSEED data in buffer, fill TRACE_BUF message and send to 
   transport ring.
   Based on dumpseed from ASL DCC; modified for earthworm: PNL 2/2/2000 */

#define LW(a) LocGM68_WORD(a)
#define LL(a) LocGM68_LONG(a)
struct generic_blk {
  UDCC_WORD	blktype;
  UDCC_WORD	blkxref;
};

int ProcessData(LISSConnection *pConnInfo)
{
	static char tmpBuf[100];      /* Some scratch space                */
	SEED_DATA *seed_rec;          /* pointer to incoming miniSEED      */
	char n[10],s[10],l[10],c[10]; /* net, station, location, comp buffers */
	double starttime;             /* SEED start time with corrections  */
	double srate;                 /* sample rate                       */
	int a,b;
	struct generic_blk *blkhdr;   /* pointer to generic SEED blockette */
	int format = 0;               /* Format of data_only blockette     */
	int nsamps;                   /* Number of samples, from header    */
	int act_samps;                /* Number of samples decoded         */
	int Iscn;
	SCN_BUFFER *pScnBuffer;       /* Pointer to SCN buffer of interest */
	short level, dframes, firstframe, dstat;
	int rec_length;

	if (L2e.Param.debug)
		logit("t", "Processing data for <%s:%d>\n", pConnInfo->IPaddr, pConnInfo->IPport);

	seed_rec = (SEED_DATA*)pConnInfo->inBuf;

	/* Make sure we are looking at a Data packet */
	/* We might have lost sync with the miniSEED messages; if we die
		and restart, maybe we'll have better luck */
	if (seed_rec->Record_Type != 'D') 
	{
		/* Maybe a `keepalive' packet */
		if (seed_rec->Record_Type == ' ') 
		{
			if (L2e.Param.debug)
			logit("et", "liss2ew: received keepalive packet\n");
			return EW_SUCCESS;
		}

		logit("et", "liss2ew: non-data packet found: <%d>; exiting\n",
			  seed_rec->Record_Type);
		return EW_FAILURE;
	}

	SH_Get_Idents(seed_rec,n,s,l,c);
	TrimString(n);
	Upcase(n);
	TrimString(s);
	Upcase(s);
	TrimString(l);
	Upcase(l);
	TrimString(c);
	Upcase(c);

	/* Get index of this S-C-N in pscnB */
	for( Iscn=0; Iscn <pConnInfo->Nscn; Iscn++ )   
	{  /* The logic is: break on a match; continue on no match */
		if( strcmp( pConnInfo->pscnB[Iscn].sta, s) != 0 )
		{
			continue;
		}
		if( strcmp( pConnInfo->pscnB[Iscn].chan, c) != 0 )
		{
			continue;
		}
		if( strcmp( pConnInfo->pscnB[Iscn].net,  n) != 0 )
		{
			continue;
		}
		if( pConnInfo->pscnB[Iscn].lc[0] == LC_WILD )
		{
			break;   /* WILD matches any LC */
		}
		else if( strcmp( pConnInfo->pscnB[Iscn].lc, l ) != 0) 
		{
			continue;
		}

		break;
	}
	if( Iscn == pConnInfo->Nscn ) /* this SCN is not in our AcceptSCNL list */
	{
		if (L2e.Param.debug)
			logit("t", "Ignoring <%s %s %s %s>; not in SCNL list!\n", s, c, n, l );
		return EW_SUCCESS;
	}
	if (L2e.Param.debug)
		logit("t", "Processing <%s %s %s %s> packet\n", s,c,n,l);
   
	pScnBuffer = &(pConnInfo->pscnB[Iscn]);
	strcpy(pConnInfo->PacketSta, s);
	strcpy(pConnInfo->PacketChan, c);
	strcpy(pConnInfo->PacketNet, n);
	strcpy(pConnInfo->PacketLc, l);

	starttime = ST_GetDblUnix( SH_Start_Time(seed_rec) );

	/* Check for time correction */
	if ((seed_rec->Activity_Flags & ACTFLAG_CLKFIX) == 0)
		starttime += 0.0001 * LL(seed_rec->Time_Correction);
  
	/* Sample rate */
	a = LW(seed_rec->Rate_Factor);
	b = LW(seed_rec->Rate_Mult);

	if (b == 0)
	{
		logit("et", "liss2ew: illegal rate multiplier 0; skipping packet\n");
		return EW_SUCCESS;
	}
	if (a == 0)
	{
		logit("et", "liss2ew: illegal rate factor 0; skipping packet\n");
		return EW_SUCCESS;
	}
	if ( a < 0) 
		srate = -1.0 / (double) a;
	else 
		srate = (double) a;
	if ( b < 0) 
		srate /= -((double) b);
	else 
		srate *= (double) b;

	/* Check for changed sample rate; could be caused by different location code */
	if ( pScnBuffer->samplerate > 0.0  && pScnBuffer->samplerate != srate )
	{
		logit("et", "sample rate changed for <%s.%s.%s.%s>; current is %f; last is %f\n",
			  s,c,n,l, srate, pScnBuffer->samplerate);
	}
	pScnBuffer->samplerate = srate;

	nsamps = LW(seed_rec->Number_Samps);
	pScnBuffer->quality[0] = seed_rec->Qual_Flags; pScnBuffer->quality[1] = 0;
  
	/* Handle the individual miniSEED blockettes */
	a = LW(seed_rec->First_Blockette);  
	while(a > 0 && a < L2e.lenSEED)
	{                       /* walk the list of blockettes */
		blkhdr = (struct generic_blk *) (pConnInfo->inBuf + a);
		b = LW(blkhdr->blktype);
		switch (b)
		{
		case 1000:	
			format = ((struct Data_only *)blkhdr)->Encoding;
			rec_length = ((struct Data_only *)blkhdr)->Length;
			break;
		case 1001:	
			/* Time shim in microseconds */
			starttime += 0.000001 * ((struct Data_ext *)blkhdr)->Usec;
			break;
		default:
			if (L2e.Param.debug)
				logit("t", "skipping non-data blockette %d\n", b);
			break;
		}

		/* Point to the next blockette */
		a = LW(blkhdr->blkxref);
	}  /* End of while(a > 0...) */
  
	if (pConnInfo->outBufLen < nsamps)
	{
		if (L2e.Param.debug)
		{
			logit("t","ProcessData outBuf old %d new %d\n", pConnInfo->outBufLen, 
				nsamps + 100);
		}
		pConnInfo->outBufLen = nsamps + 100;   /* So we don't keep asking for a little more */
		if (pConnInfo->outBuf)
			free(pConnInfo->outBuf);
		if ( (pConnInfo->outBuf = (long *)malloc(pConnInfo->outBufLen * sizeof(long))) 
			 == (long *)NULL)
		{
			logit("et", "liss2ew: error alloacting memory for SEED output buffer.\n");
			return EW_FAILURE;
		}
	}
  
	/* And now for the trace data... */
	switch (format)
	{
	case FMT_STEIM_1:
		level = 1;
		break;
	case FMT_STEIM_2:
		level = 2;
		break;
	case FMT_STEIM_3:
		level = 3;
		break;
	default:
		logit("et", "Skipping unsupported data format %d\n", format);
		return (EW_SUCCESS);
	}
  
	switch (rec_length)
	{
	case 12:
		dframes = 63;
		break;
	case 11:
		dframes = 31;
		break;
	case 10:
		dframes = 15;
		break;
	case 9:
		dframes = 7;
		break;
	case 8:
		dframes = 3;
		break;
	case 7:
		dframes = 1;
		break;
	default:
		logit("et", "Skipping unsupported record length %d\n", rec_length);
		return EW_SUCCESS;
	}
  
	firstframe = LW(seed_rec->Data_Start) / sizeof(compressed_frame) - 1;
  
	act_samps = decompress_generic_record((generic_data_record *)seed_rec,
                                        pConnInfo->outBuf, &dstat, pConnInfo->dcp, 
                                        firstframe, nsamps, level, FLIP, 
                                        dframes);
	/* non-fatal advisories: see steim/steim.h */
	if (L2e.Param.debug)
	{
		if (dstat & EDF_INTEGRESYNC)
			logit("et","decompress frame: expansion error at integration constant check.\n") ;
		if (dstat & EDF_LASTERROR)
			logit("et","decompress record: last sample does not agree with decompression.\n") ;
		if (dstat & EDF_COUNTERROR)
			logit("et","decompress record: sample count disagreement.\n") ;
	}
	/* Fatal decompression errors: */
	if (dstat & EDF_SECSUBCODE)
		logit("et","decompress frame: illegal secondary subcode spanning two blocks.\n");
	if (dstat & EDF_TWOBLOCK)
		logit("et","decompress frame: illegal two-block code at end of frame.\n");
	if (dstat & EDF_OVERRUN)
		logit("et","decompress frame: unpacked buffer overrun.\n");
	if (dstat & EDF_REPLACEMENT)
		logit("et","decompress frame: illegal flag-word replacement subcode.\n");
	if (dstat & EDF_INTEGFAIL)
		logit("et","decompress frame: expansion failed. frame internally damaged.\n");

	if ((dstat & EDF_FATAL) != 0)
		return EW_SUCCESS;

	if (act_samps != nsamps)
	{
		logit("et", "liss2ew: sample mismatch: decoded (%d) advertised (%d)\n",
			  act_samps, nsamps);
		nsamps = act_samps;
	}
	if (act_samps > pConnInfo->outBufLen)
	{
		logit("et", "liss2ew: SEED decoder overflowed output buffer; results undefined.\n");
		return EW_FAILURE;
	}
  
	return( DisposePacket(pConnInfo, pScnBuffer, nsamps, starttime ));
}


/*
 * Dispose of the trace data into the assigned SCN buffer, fill up TRACE_BUF
 * packets, and send them on their way. */
int DisposePacket( LISSConnection *pConnInfo, SCN_BUFFER *pScnBuffer, int nsamps, 
                   double starttime)
{
  int i;   /* Sample counter */
  int hunk;
  
  /* SCN buffer has data and there's a gap, send the old data immediately */
  if (pScnBuffer->writeP)
  {
    if (starttime > pScnBuffer->endtime + 1.5/pScnBuffer->samplerate)
    {
      logit("et", "liss2ew: gap detected in <%s.%s.%s>\n", pScnBuffer->sta,
            pScnBuffer->chan, pScnBuffer->net);
      SendTrace( pConnInfo, pScnBuffer );
    }
    else if (starttime < pScnBuffer->endtime)
    {
      logit("et", "liss2ew: overlap detected in <%s.%s.%s>; dumping old data\n", 
            pScnBuffer->sta, pScnBuffer->chan, pScnBuffer->net);
      pScnBuffer->writeP = 0;
    }
  }

  /* Transfer trace data from outBuf into the SCN buffer */
  i = 0;
  while (i < nsamps)
  {
    hunk = L2e.traceLen - pScnBuffer->writeP;
    if (hunk > nsamps - i) hunk = nsamps - i;
    memcpy((void*)&(pScnBuffer->buf[pScnBuffer->writeP]), (void*)&(pConnInfo->outBuf[i]), 
           hunk * sizeof(long));
    if (pScnBuffer->writeP == 0)    /* SCN buffer was empty, need new starttime */
      pScnBuffer->starttime = starttime + i / pScnBuffer->samplerate;
    pScnBuffer->writeP += hunk;
    pScnBuffer->endtime = pScnBuffer->starttime + (pScnBuffer->writeP - 1) / pScnBuffer->samplerate;
    if ( pScnBuffer->writeP == L2e.traceLen)
      if ( SendTrace( pConnInfo, pScnBuffer ) == EW_FAILURE)
        return EW_FAILURE;    /* Bad problem; we need to die */
    i += hunk;
  }

  return EW_SUCCESS;
}


int SendTrace( LISSConnection *pConnInfo, SCN_BUFFER *pScnBuffer)
{
  TracePacket   tbuf;      /* Earthworm trace data buffer (trace_buf.h) */
  int len;
  long *waveBuf;
  
  waveBuf = (long *)((char *)&tbuf + sizeof(TRACE2_HEADER));
  
  /* init the header, for kicks and all */
  memset(&tbuf, 0, sizeof(TRACE2_HEADER));

  tbuf.trh2.pinno = pScnBuffer->pinno;
  strncpy(tbuf.trh2.sta, pConnInfo->PacketSta, TRACE2_STA_LEN);
  strncpy(tbuf.trh2.chan, pConnInfo->PacketChan, TRACE2_CHAN_LEN);
  strncpy(tbuf.trh2.net, pConnInfo->PacketNet, TRACE2_NET_LEN);
  strncpy(tbuf.trh2.loc, pConnInfo->PacketLc, TRACE2_LOC_LEN);
  tbuf.trh2.starttime = pScnBuffer->starttime;
  tbuf.trh2.endtime = pScnBuffer->endtime;
  tbuf.trh2.nsamp = L2e.traceLen;
  tbuf.trh2.samprate = pScnBuffer->samplerate;
  tbuf.trh2.version[0] = TRACE2_VERSION0;
  tbuf.trh2.version[1] = TRACE2_VERSION1;

  
  /* The decoders always write long integers */
#ifdef _SPARC
  strcpy(tbuf.trh2.datatype, "s4");
#endif
#ifdef _INTEL
  strcpy(tbuf.trh2.datatype, "i4");
#endif

  tbuf.trh2.quality[0] = pScnBuffer->quality[0];
  tbuf.trh2.quality[1] = pScnBuffer->quality[1];
  
  memcpy(waveBuf, pScnBuffer->buf, L2e.traceLen * sizeof(long));
  pScnBuffer->writeP = 0;  /* Mark the buffer as empty */
  
  len = L2e.traceLen * sizeof(long) + sizeof(TRACE2_HEADER);

  if ( tport_putmsg( (SHM_INFO *)&(L2e.regionOut), (MSG_LOGO *)&(L2e.waveLogo), len, (char*)&tbuf ) 
       != PUT_OK )
   {
     logit("et", "liss2ew: Error sending message via transport.\n");
     return EW_FAILURE;
   }
  
  return EW_SUCCESS;
}
