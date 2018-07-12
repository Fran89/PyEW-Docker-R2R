
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_recorder.c 5720 2013-08-05 20:18:25Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2007/02/26 16:37:01  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.9  2006/09/01 16:54:11  paulf
 *     fixed a bug in the null termination of the output picks
 *
 *     Revision 1.8  2006/09/01 16:09:33  paulf
 *     really fixed the filenameing to use %02d%02d sprintf for the month/day!
 *
 *     Revision 1.7  2006/09/01 15:28:03  paulf
 *     fixed the filenaming of pick_recorder pick files
 *
 *     Revision 1.6  2006/09/01 15:12:01  paulf
 *     really fixed the SCNL capability
 *
 *     Revision 1.5  2006/08/31 17:29:09  paulf
 *     added in SCNL capability
 *
 *     Revision 1.4  2002/05/15 21:38:30  patton
 *     Made Logit changes.
 *
 *     Revision 1.3  2001/05/09 17:48:36  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2001/05/04 15:50:43  lucky
 *     Cleaned up comments to reflect what this module actually does.
 *
 *     Revision 1.1  2000/07/24 20:15:42  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 17:18:52  lucky
 *     Initial revision
 *
 *
 */

/*
 * pick_recorder.c:  
 *
 *    This module reads messages of type TYPE_PICK2K from the InRing 
 *      and writes them to a file in the directory specified by
 *      TargetDir option.
 * 
 *    A new file is started for each day so that the result of a 
 *     continuous run of pick_recorder is a series of daily files 
 *     containing all pick messages from the InRing.
 */

#ifdef _OS2
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>
#include <mem_circ_queue.h>


static  SHM_INFO  InRegion;      /* shared memory region to use for i/o    */


#define   NUM_COMMANDS 		6 	/* how many required commands in the config file */

#define   MAX_PICKBUF_LEN 	50000

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;
 

#define SCN_INCREMENT   10	 /* how many more are allocated each time we run out */
#define STATION_LEN      6   /* max string-length of station code     */
#define CHAN_LEN         8   /* max string-length of component code   */
#define NETWORK_LEN      8   /* max string-length of network code     */


/* The message queue
 *******************/
#define	QUEUE_SIZE		500   	/* How many msgs can we queue */
QUEUE 	MsgQueue;				/* from queue.h */

/* Thread stuff */
#define THREAD_STACK 8192
static unsigned tidProcessor;    /* Processor thread id */
static unsigned tidStacker;      /* Thread moving messages from InRing */
                                 /* to MsgQueue */
int MessageStackerStatus = 0;      /* 0=> Stacker thread ok. <0 => dead */
int ProcessorStatus = 0;           /* 0=> Processor thread ok. <0 => dead */




/* Things to read or derive from configuration file
 **************************************************/
static char    InRingName[MAX_RING_STR];      /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];       /* this module's given name          */
static char    MyProgName[256];     /* this module's program name        */
static char    TargetDir[256];      /* where picks will be written out   */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */
static int     Debug = 0;           /* Debug flag, 0-no debug, 1-basic debug, 2-super debug */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char ModWild;

/* Error messages used by pick_recorder 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/


/* Functions in this source file 
 *******************************/
static	int  	pr_config (char *);
static	int  	pr_lookup (void);
static	void  	pr_status (unsigned char, short, char *);
thr_ret			MessageStacker (void *);
thr_ret			Processor (void *);





int main (int argc, char **argv)
{
	time_t			timeNow;		   /* current time */ 
	time_t			timeLastBeat;	   /* time last heartbeat was sent  */
	long			recsize;		   /* size of retrieved message     */
	MSG_LOGO		reclogo;		   /* logo of retrieved message     */
	char			*flushmsg;


	/* Check command line arguments 
	 ******************************/
	if (argc != 2)
	{
		fprintf (stderr, "Usage: pick_recorder <configfile>\n");
		return EW_FAILURE;
	}

	/* Check that either SPARC or INTEL were defined during compilation */
	/* We want to make sure before we get too far */

#if defined _SPARC
	;
#elif defined _INTEL
	;
#else
	logit ("e", "pick_recorder: Must have either SPARC or INTEL defined during compilation; exiting!");
	return EW_FAILURE;
#endif
	

	/* To be used in loging functions
	 ********************************/
	if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
	{
		fprintf (stderr, "pick_recorder: Call to get_prog_name failed.\n");
		return EW_FAILURE;
	}

	/* Initialize name of log-file & open it 
	 ***************************************/
	logit_init (argv[1], 0, 256, 1);

	/* Read the configuration file(s)
	 ********************************/
	if (pr_config(argv[1]) != EW_SUCCESS)
	{
		fprintf (stderr, "pick_recorder: Call to pr_config failed \n");
		return EW_FAILURE;
	}
    logit ("" , "%s(%s): Read command file <%s>\n", MyProgName, MyModName, argv[1]);

	/* Look up important info from earthworm.h tables
	 ************************************************/
	if (pr_lookup() != EW_SUCCESS)
	{
		fprintf (stderr, "%s(%s): Call to pr_lookup failed \n",
						MyProgName, MyModName);
		return EW_FAILURE;
	}


	/* Reinitialize logit to desired logging level 
	 *********************************************/
	logit_init (argv[1], 0, 256, LogSwitch);
	

	/* Get our process ID
	 **********************/
	if ((MyPid = getpid ()) == -1)
	{
		logit ("e", "%s(%s): Call to getpid failed. Exiting.\n", MyProgName, MyModName);
		return (EW_FAILURE);
	}



	/* Attach to Input shared memory ring 
	 *******************************************/
	tport_attach (&InRegion, InKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, InRingName, InKey);


	/* Force a heartbeat to be issued in first pass thru main loop
	 *************************************************************/
	timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

	/* Flush the incomming transport ring 
	 *************************************/
	if ((flushmsg = (char *) malloc (MAX_BYTES_PER_EQ)) ==  NULL)
	{
		logit ("e", "pick_recorder: can't allocate flushmsg; exiting.\n");
		return EW_FAILURE;
	}

	while (tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
			&recsize, flushmsg, (MAX_BYTES_PER_EQ - 1)) != GET_NONE)

        ;


	/* Create MsgQueue mutex */
	CreateMutex_ew();


	/* Allocate the message Queue
	 ********************************/
	initqueue (&MsgQueue, QUEUE_SIZE, MAX_BYTES_PER_EQ);

	/* Start message stacking thread which will read 
	 * messages from the InRing and put them into the Queue 
	 *******************************************************/
	if (StartThread (MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
	{
		logit( "e", 
			"pick_recorder: Error starting MessageStacker thread.  Exiting.\n");
		tport_detach (&InRegion);
		return EW_FAILURE;
	}

	MessageStackerStatus = 0; /*assume the best*/


	/* Start decimator thread which will read messages from
	 * the Queue, process them and write them to the OutRing
	 *******************************************************/
	if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
	{
		logit( "e", 
			"pick_recorder: Error starting Processor thread.  Exiting.\n");
		tport_detach (&InRegion);
		return EW_FAILURE;
	}

	ProcessorStatus = 0; /*assume the best*/

/*--------------------- setup done; start main loop -------------------------*/

	while (tport_getflag (&InRegion) != TERMINATE  &&
               tport_getflag (&InRegion) != MyPid  )
	{

		/* send pick_recorder' heartbeat
		***************************/
		if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
		{
			timeLastBeat = timeNow;
			pr_status (TypeHeartBeat, 0, ""); 
		}

		/* Check on our threads */
		if (MessageStackerStatus < 0)
		{
			logit ("et", 
				"pick_recorder: MessageStacker thread died. Exiting\n");
			return EW_FAILURE;
		}

		if (ProcessorStatus < 0)
		{
			logit ("et", 
				"pick_recorder: Processor thread died. Exiting\n");
			return EW_FAILURE;
		}

		sleep_ew (1000);

	} /* wait until TERMINATE is raised  */  

	/* Termination has been requested
	 ********************************/
	tport_detach (&InRegion);
	logit ("t", "pick_recorder: Termination requested; exiting!\n" );
	return EW_SUCCESS;

}

/******************************************************************************
 *  pr_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int pr_config (char *configfile)
{
	char     		init[NUM_COMMANDS];     
						/* init flags, one byte for each required command */
	int      		nmiss;
						/* number of required commands that were missed   */
	char    		*com;
	char    		*str;
	char 			*inst_type;
	char 			*mod_type;
	int      		nfiles;
	int      		success;
	int      		i;


	/* Set to zero one init flag for each required command 
	*****************************************************/   
	for (i = 0; i < NUM_COMMANDS; i++)
		init[i] = 0;

	nLogo = 0;

	/* Open the main configuration file 
	**********************************/
	nfiles = k_open (configfile); 
	if (nfiles == 0) 
	{
		logit ("e" ,
			"pick_recorder: Error opening command file <%s>; exiting!\n", 
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

			/* Open a nested configuration file 
			**********************************/
			if (com[0] == '@') 
			{
				success = nfiles + 1;
				nfiles  = k_open (&com[1]);
				if (nfiles != success) 
				{
					logit ("e" , 
				  	  "pick_recorder: Error opening command file <%s>; exiting!\n", &com[1]);
					return EW_FAILURE;
				}
				continue;
			}

			/* Process anything else as a command 
			************************************/
	/*0*/ 	if (k_its ("MyModId")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (MyModName, str);
					init[0] = 1;
				}
			}
	/*1*/	else if (k_its ("InRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (InRingName, str);
					init[1] = 1;
				}
			}
	/*2*/	else if (k_its ("HeartBeatInterval")) 
			{
				HeartBeatInterval = k_long ();
				init[2] = 1;
			}
	/*3*/	else if (k_its ("LogFile"))
			{
				LogSwitch = k_int();
				init[3] = 1;
			}

			/* Enter installation & module types to get
			 *******************************************/
	/*4*/	else if (k_its ("GetPicksFrom")) 
			{
				if (nLogo >= MAXLOGO) 
				{
					logit ("e" , "pick_recorder: Too many <GetPicksFrom> commands in <%s>; "
										"; max=%d; exiting!\n", configfile, (int) MAXLOGO);
					return EW_FAILURE;
				}
				if ((inst_type = k_str())) 
				{
					if (GetInst (inst_type, &GetLogo[nLogo].instid) != 0) 
					{
						logit ("e" , "pick_recorder: Invalid installation name <%s> in "
											"<GetPicksFrom> cmd; exiting!\n", inst_type);
						return EW_FAILURE;
					}
                 		}
				if ((mod_type = k_str())) 
				{
					if (GetModId (mod_type, &GetLogo[nLogo].mod) != 0) 
					{
						logit ("e" , "pick_recorder: Invalid module name <%s> in <GetPicksFrom> "
										"cmd; exiting!\n", mod_type);
						return EW_FAILURE;
					}
				}
				/* We'll always fetch pick messages */
				if (GetType ("TYPE_PICK2K", &GetLogo[nLogo].type) != 0) 
				{
					logit ("e" , "pick_recorder: Invalid msgtype <%s> in <GetPicksFrom> "
										"cmd; exiting!\n", str);
					return EW_FAILURE;
				}
				nLogo++;

				/* now add in the SCNL pick message */
				GetLogo[nLogo].instid = GetLogo[nLogo-1].instid;
				GetLogo[nLogo].mod = GetLogo[nLogo-1].mod;
				if (GetType ("TYPE_PICK_SCNL", &GetLogo[nLogo].type) != 0) 
				{
					logit ("e" , "pick_recorder: Invalid msgtype <%s> in <GetPicksFrom> "
										"cmd; exiting!\n", str);
					return EW_FAILURE;
				}
				nLogo++;
				init[4] = 1;
			}

	/*5*/	else if (k_its ("TargetDir")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (TargetDir, str);
					init[5] = 1;
				}
			}
	/*NR*/	else if (k_its ("Debug"))
			{
				Debug = k_int();
			}

			/* Unknown command
			*****************/ 
			else 
			{
				logit ("e" , "pick_recorder: <%s> Unknown command in <%s>.\n", 
								com, configfile);
				continue;
			}

			/* See if there were any errors processing the command 
			*****************************************************/
			if (k_err ()) 
			{
				logit ("e" , 
					"pick_recorder: Bad <%s> command in <%s>; exiting!\n",
						com, configfile);
				return EW_FAILURE;
			}

		} /** while k_rd() **/

		nfiles = k_close ();

	} /** while nfiles **/

	/* After all files are closed, check init flags for missed commands
	******************************************************************/
	nmiss = 0;
	for (i = 0; i < NUM_COMMANDS; i++)  
		if (!init[i]) 
			nmiss++;

	if (nmiss) 
	{
		logit ("e" , "pick_recorder: ERROR, no ");
		if (!init[0])  logit ("e" , "<MyModId> "        );
		if (!init[1])  logit ("e" , "<InRing> "          );
		if (!init[2])  logit ("e" , "<HeartBeatInterval> "     );
		if (!init[3])  logit ("e" , "<LogFile> "     );
		if (!init[4])  logit ("e" , "<GetPicksFrom> "     );
		if (!init[5])  logit ("e" , "<TargetDir> "     );

		logit ("e" , "command(s) in <%s>; exiting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  pr_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int pr_lookup( void )
{

	/* Look up keys to shared memory regions
	*************************************/
	if ((InKey = GetKey (InRingName)) == -1) 
	{
		fprintf (stderr,
				"pick_recorder:  Invalid ring name <%s>; exiting!\n", InRingName);
		return EW_FAILURE;
	}


	/* Look up installations of interest
	*********************************/
	if (GetLocalInst (&InstId) != 0) 
	{
		fprintf (stderr, 
			"pick_recorder: error getting local installation id; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up modules of interest
	******************************/
	if (GetModId (MyModName, &MyModId) != 0) 
	{
		fprintf (stderr, 
			"pick_recorder: Invalid module name <%s>; exiting!\n", MyModName);
		return EW_FAILURE;
	}

	if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
	{
		fprintf (stderr, 
			"pick_recorder: Invalid module name <MOD_WILDCARD>; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up message types of interest
	*********************************/
	if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
	{
		fprintf (stderr, 
			"pick_recorder: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_ERROR", &TypeError) != 0) 
	{
		fprintf (stderr, 
			"pick_recorder: Invalid message type <TYPE_ERROR>; exiting!\n");
		return EW_FAILURE;
	}
	return EW_SUCCESS;

} 

/******************************************************************************
 * pr_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void pr_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t      t;
 
	/* Build the message 
	*******************/ 
	logo.instid = InstId;
	logo.mod    = MyModId;
	logo.type   = type;

	time (&t);

	if (type == TypeHeartBeat)
	{
		sprintf (msg, "%ld %d\n", (long) t,  MyPid);
	}
	else if (type == TypeError)
	{
		sprintf (msg, "%ld %hd %s\n", (long) t, ierr, note);
		logit ("et", "%s(%s): %s\n", MyProgName, MyModName, note);
	}

	size = strlen (msg);   /* don't include the null byte in the message */     

	/* Write the message to shared memory
	************************************/
	if (tport_putmsg (&InRegion, &logo, size, msg) != PUT_OK)
	{
		if (type == TypeHeartBeat) 
		{
			logit ("et","%s(%s):  Error sending heartbeat.\n",
											MyProgName, MyModName);
		}
		else if (type == TypeError) 
		{
			logit ("et","%s(%s):  Error sending error:%d.\n", 
										MyProgName, MyModName, ierr);
		}
	}

}




/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret		MessageStacker (void *dummy)
{
	char         	*msg;           /* "raw" retrieved message */
	int          	res;
	long         	recsize;        /* size of retrieved message */
	MSG_LOGO     	reclogo;        /* logo of retrieved message */
	int          	ret;

	/* Allocate space for input/output messages
	 *******************************************/
	if ((msg = (char *) malloc (MAX_BYTES_PER_EQ)) == (char *) NULL)
	{
		logit ("e", "pick_recorder: error allocating msg; exiting!\n");
		goto error;
	}


	/* Tell the main thread we're ok
	 ********************************/
	MessageStackerStatus = 0;

	/* Start service loop, picking up trigger messages
	 **************************************************/
	while (1)
	{
		/* Get a message from transport ring
		 ************************************/
		res = tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo, 
								&recsize, msg, MAX_BYTES_PER_EQ-1);


		if (res == GET_NONE) 
		{
			sleep_ew(100); 
			continue;
		} /*wait if no messages for us */

		/* Check return code; report errors
		***********************************/
		if (res != GET_OK)
		{
			if (res == GET_TOOBIG)
			{
				sprintf (errText, "msg[%ld] i%d m%d t%d too long for target",
							recsize, (int) reclogo.instid,
							(int) reclogo.mod, (int)reclogo.type);
				pr_status (TypeError, ERR_TOOBIG, errText);
				continue;
			}
			else if (res == GET_MISS)
			{
				sprintf (errText, "missed msg(s) i%d m%d t%d in %s",
						(int) reclogo.instid, (int) reclogo.mod, 
						(int)reclogo.type, InRingName);
				pr_status (TypeError, ERR_MISSMSG, errText);
			}
			else if (res == GET_NOTRACK)
			{
				sprintf (errText, "no tracking for logo i%d m%d t%d in %s",
						(int) reclogo.instid, (int) reclogo.mod, 
						(int)reclogo.type, InRingName);
				pr_status (TypeError, ERR_NOTRACK, errText);
			}
		}



		/* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
		*********************************************************/
		RequestMutex ();
		/* put it into the queue */
		ret = enqueue (&MsgQueue, msg, recsize, reclogo); 
		ReleaseMutex_ew ();

		if (ret != 0)
		{
			if (ret == -2)  /* Serious: quit */
			{
				sprintf (errText, "internal queue error. Terminating.");
				pr_status (TypeError, ERR_QUEUE, errText);
				goto error;
			}
			if (ret == -1)
			{
				sprintf (errText, 
					"queue cannot allocate memory. Lost message.");
				pr_status (TypeError, ERR_QUEUE, errText);
				continue;
			}
			if (ret == -3) 
			{
				sprintf (errText, "Queue full. Message lost.");
				pr_status (TypeError, ERR_QUEUE, errText);
				continue;
			}
		} /* problem from enqueue */


	} /* while (1) */

	/* we're quitting
	 *****************/
	error:
	MessageStackerStatus = -1; /* file a complaint to the main thread */
	KillSelfThread (); /* main thread will restart us */
	return(NULL); /* should never reach here */

}



/********************** Message Processing Thread ****************
 ******************************************************************/
thr_ret		Processor (void *dummy)
{


	int				ret;
	long			PickBufLen;
	int				gotMsg;
    MSG_LOGO        reclogo;           /* logo of retrieved message */
	char 			*PickBuf;          /* string to hold wave message   */
	FILE			*fp;
	char			filename[256];
	time_t			timeNow;
	struct tm		ot;
	struct tm		*otp;
	int				curday;
	int				curmon;
	int				curyear;


	/* Allocate the waveform buffer
     *******************************/
	PickBuf = (char *) malloc ((size_t) MAX_PICKBUF_LEN);

	if (PickBuf == NULL)
	{
		logit ("et", "pick_recorder: Cannot allocate pick buffer\n");
		ProcessorStatus = -1;
		KillSelfThread();
	}


	/* Open the first file 
	 **********************/
	otp = &ot;
	timeNow = time (NULL);
	otp = localtime (&timeNow);


	sprintf (filename, "%s/picks.%4d%02d%02d", TargetDir,
				(1900 + otp->tm_year), (otp->tm_mon + 1), otp->tm_mday);

	curyear = otp->tm_year;
	curmon = otp->tm_mon;
	curday = otp->tm_mday;

	logit ("", "Opening %s\n", filename);

	if ((fp = fopen (filename, "wt")) == NULL)
	{
		logit ("e", "Could not open file %s\n", filename);
		ProcessorStatus = -1;
		KillSelfThread();
	}
		

	while (1)
	{
		gotMsg = FALSE;
		while (gotMsg == FALSE)
		{
		
			RequestMutex ();
			ret = dequeue (&MsgQueue, PickBuf, &PickBufLen, &reclogo);
			ReleaseMutex_ew ();

			if (ret < 0)
			{
				/* empty queue */
				sleep_ew (1000);
			}
			else
				gotMsg = TRUE;

		} /* while no messages are dequeued */

		/* terminate PickBuf as a string */
		PickBuf[PickBufLen] ='\0';

		logit ("", "Recording Pick: %s", PickBuf);
	
		/* Are we still in the same day 
	 	 *******************************/
		otp = &ot;
		timeNow = time (NULL);
		otp = localtime (&timeNow);

		if ((curday != otp->tm_mday) || (curmon != otp->tm_mon) ||
				(curyear != otp->tm_year))
		{

			logit ("", "Closing %s\n", filename);

			/* close the current file and start a new one */
			fclose (fp);

			sprintf (filename, "%s/picks.%4d%02d%02d", TargetDir,
				(1900 + otp->tm_year), (otp->tm_mon + 1), otp->tm_mday);

			curyear = otp->tm_year;
			curmon = otp->tm_mon;
			curday = otp->tm_mday;

			logit ("", "Opening %s\n", filename);

			if ((fp = fopen (filename, "wt")) == NULL)
			{
				logit ("e", "Could not open file %s\n", filename);
				ProcessorStatus = -1;
				KillSelfThread();
			}
		}
		
		fprintf (fp, "%s", PickBuf);
		fflush (fp);

		ProcessorStatus = 0;

	} /* while 1*/
	return(NULL); /* should never reach here */

}
