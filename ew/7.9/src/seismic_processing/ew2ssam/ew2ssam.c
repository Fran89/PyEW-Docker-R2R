/*
 * ew2ssam.c:  
 *
 *    This module was developed by modifying a copy of ew2rsam.c 
 *    (initial version by Lucky Vidmar).  
 *
 *    This module reads trace messages from the InRing and computes
 *  ssam values over time periods defined in the configuration file.
 *  The ssam values are written out to the OutRing as a ssambuf
 *  message (TYPE_VTABULARDATA), containing one value, with endtime 
 *  and starttime both equal to the time of the last sample in the
 *  ssam computation period; the sampling rate is the inverse of 
 *  the ssam averaging period.
 *    The channel name in the S-C-N of the newly produced message  
 *  is modified to reflect the Ssam nature of the message. The 
 *  original channel code is appended the string _Sx, where x is 
 *  the index of the ssam computation period (0 - 4). 
 *  If the newly generated channel name exceedes the maximum 
 *  length of the chan string, an error is reported and that message
 *  is NOT written out to the OutRing.
 *      
 *  FFT functions, four1 and realft, were taken from Numerical Recipes 
 *  in C: The Art of Scientific Computing.  Text copyright 1988-1992
 *  by Cambridge University Press.  Programs copyright 1988-1992 by
 *  Numerical Recipes Software.  
 *      
 */

#ifdef _OS2
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#endif

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

void four1(double data[], unsigned long nn, int isign);
void realft(double data[], unsigned long nn, int isign);
void taper(double data[], unsigned long nn, double width);

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

#include "vtabular_buf.h"

static  SHM_INFO  InRegion;      /* shared memory region to use for i/o    */
static  SHM_INFO  OutRegion;     /* shared memory region to use for i/o    */


#define   NUM_COMMANDS 	10 	/* how many required commands in the config file */

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;
 

#define SCN_INCREMENT   10	 /* how many more are allocated each time we run out */
#define STATION_LEN      6   /* max string-length of station code     */
#define CHAN_LEN         8   /* max string-length of component code   */
#define NETWORK_LEN      8   /* max string-length of network code     */


/* The message queue
 *******************/
#define	QUEUE_SIZE		1000   	/* How many msgs can we queue */
QUEUE 	MsgQueue;				/* from queue.h */

/* Thread stuff */
#define THREAD_STACK 8192
static unsigned tidProcessor;    /* Processor thread id */
static unsigned tidStacker;      /* Thread moving messages from InRing */
                                 /* to MsgQueue */
int MessageStackerStatus = 0;      /* 0=> Stacker thread ok. <0 => dead */
int ProcessorStatus = 0;           /* 0=> Processor thread ok. <0 => dead */



/* Combination of the SCN codes and logos 
 *****************************************/
typedef struct scn_struct {

	char		sta[STATION_LEN];         /* Site name */
	char		chan[CHAN_LEN];           /* Component/channel code */
	char		net[NETWORK_LEN];         /* Network name */

} SCNstruct;

SCNstruct 	*IncludeSCN = NULL; 		/* which SCNs to get */
int     	num_IncludeSCN = 0;			/* how many do we have */
int     	max_IncludeSCN = 0;				/* how many are allocated so far */

SCNstruct 	*ExcludeSCN = NULL; 		/* which SCNs NOT to get */
int     	num_ExcludeSCN = 0;			/* how many do we have */
int     	max_ExcludeSCN = 0;				/* how many are allocated so far */


/* SSAM book-keeping constants and structures */

#define	DC_ARRAY_ENTRIES	20
#define	DC_TIME_DIFF		3.0
#define	DC_STARTUP_ENTRIES	4
#define	DC_OFFSET_INVALID	-1.00
#define	TIME_INVALID        -1.00
#define	MAX_TIME_PERIODS	5
#define	MAX_CHAN_LEN		5 	
#define FFT_NN                512
#define num_freqs             256

typedef struct time_period_struct
{
	double	time_period; 	/* time period in seconds */
	double	ssam_value[num_freqs];		/* currently kept ssam total */
	int		ssam_nsamp;		/* samples in the current total */
	double	ssam_starttime;	/* when did we start counting ? */
	char	outchan[VTABULAR_CHAN_LEN];	/* outgoing channel label */

} Tstruct;

typedef struct fft_struct
{
       double fft_data[FFT_NN+1];
       double frequency[num_freqs];
       int   fft_nsamp;
} Fstruct;

/* there will be one for each requested channel */
/* with the maximum read from the configuration file */

typedef struct ssam_values_struct
{

	/* START DC offset section */

	double	DC_offset;			/* DC offset value */

	double 	DC_cur_val;			/* Current DC total */
	int		DC_cur_nsamp;		/* Number of samples in current total */
	double	DC_starttime;		/* When did we start counting ? */

	double	DC_array[DC_ARRAY_ENTRIES];	/* Averages for previous time slices */
	int		DC_cur_pos;			/* index of the current position in the array */
	int		DC_start_pos;		/* index of the starting position in the array */

	int		DC_startup;			/* TRUE if we are still starting out */
	/* END DC offset section */

        /* Start data gap detection section */
        
        double                          last_packet_endtime;

	/* START time period section */
	Tstruct		TP[MAX_TIME_PERIODS];
        Fstruct         FFT;

} SSAM_val;


typedef struct ssam_struct
{

	char	sta[7];
	char	chan[9];
	char	net[9];
	SSAM_val *values;

} SSAM;

float           SsamTaper;                               /* taper width for data */
int     	num_periods = 0;				/* how many ssam time periods do we keep */
double		SsamPeriod[MAX_TIME_PERIODS];
int     	MaxSCN = 0;				 	/* max number of SCNs to track */
        
SSAM		*Ssam;						/* Ssam struct, one for each SCN to track */
int			num_ssam;					/* How many have been allocated */


/* Things to read or derive from configuration file
 **************************************************/
static char    InRingName[20];      /* name of transport ring for i/o    */
static char    OutRingName[20];     /* name of transport ring for i/o    */
static char    MyModName[32];       /* this module's given name          */
static char    MyProgName[256];     /* this module's program name        */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */
static int     Debug = 0;           /* Debug flag, 0-no debug, 
												   1-basic debug,
												   2-super debug */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static long          OutKey;        /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeWaveform;
static unsigned char TypeError;
static unsigned char InstWild;
static unsigned char ModWild;

/* Error messages used by ew2ssam 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/


/* Functions in this source file 
 *******************************/
static	int  	ew2ssam_config (char *);
static	int  	ew2ssam_lookup (void);
static	void  	ew2ssam_status (unsigned char, short, char *);
static 	int 	matchSCN (char *, char *, char *, SCNstruct *, int, SCNstruct *, int, int *);
thr_ret			MessageStacker (void *);
thr_ret			Processor (void *);





int main (int argc, char **argv)
{
	time_t			timeNow;		   /* current time                  */ 
	time_t			timeLastBeat;	   /* time last heartbeat was sent  */
	long			recsize;		   /* size of retrieved message     */
	MSG_LOGO		reclogo;		   /* logo of retrieved message     */
	char			*flushmsg;
	int				i;


	/* Check command line arguments 
	 ******************************/
	if (argc != 2)
	{
		fprintf (stderr, "Usage: ew2ssam <configfile>\n");
		return EW_FAILURE;
	}

	/* Check that either SPARC or INTEL were defined during compilation */
	/* We want to make sure before we get too far */

#if defined _SPARC
	;
#elif defined _INTEL
	;
#else
	logit ("e", "ew2ssam: Must have either SPARC or INTEL defined during compilation; exiting!");
	return EW_FAILURE;
#endif
	

	/* To be used in loging functions
	 ********************************/
	if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
	{
		fprintf (stderr, "ew2ssam: Call to get_prog_name failed.\n");
		return EW_FAILURE;
	}
		
	/* Initialize name of log-file & open it 
	 ***************************************/
	logit_init (argv[1], 0, 256, 1);

	/* Read the configuration file(s)
	 ********************************/
	if (ew2ssam_config(argv[1]) != EW_SUCCESS)
	{
		fprintf (stderr, "ew2ssam: Call to ew2ssam_config failed \n");
		return EW_FAILURE;
	}
	logit ("" , "%s(%s): Read command file <%s>\n", 
						MyProgName, MyModName, argv[1]);

	/* Look up important info from earthworm.h tables
	 ************************************************/
	if (ew2ssam_lookup() != EW_SUCCESS)
	{
		fprintf (stderr, "%s(%s): Call to ew2ssam_lookup failed \n",
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
		logit ("e", "%s(%s): Call to getpid failed. Exiting.\n",
										MyProgName, MyModName);
		return (EW_FAILURE);
	}



	/* Attach to Input shared memory ring 
	 *******************************************/
	tport_attach (&InRegion, InKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, InRingName, InKey);

	/* Attach to Output shared memory ring 
	 *******************************************/
	tport_attach (&OutRegion, OutKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, OutRingName, OutKey);

	/* Force a heartbeat to be issued in first pass thru main loop
	 *************************************************************/
	timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

	/* Flush the incomming transport ring 
	 *************************************/
	if ((flushmsg = (char *) malloc (MAX_BYTES_PER_EQ)) ==  NULL)
	{
		logit ("e", "ew2ssam: can't allocate flushmsg; exiting.\n");
		return EW_FAILURE;
	}

	while (tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
			&recsize, flushmsg, (MAX_BYTES_PER_EQ - 1)) != GET_NONE)

        ;


	/* Initialize the Ssam structure */
	num_ssam = 0;
	
	if ((Ssam = (SSAM *) malloc (MaxSCN * sizeof (SSAM))) == NULL)
	{
		logit ("e", "ew2ssam: Could not malloc Ssam; exiting!\n");
		return EW_FAILURE;
	}

	for (i = 0; i < MaxSCN; i++)
	{
		Ssam->values = NULL;
	}


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
			"ew2ssam: Error starting MessageStacker thread.  Exiting.\n");
		tport_detach (&InRegion);
		tport_detach (&OutRegion);
		return EW_FAILURE;
	}

	MessageStackerStatus = 0; /*assume the best*/


	/* Start decimator thread which will read messages from
	 * the Queue, process them and write them to the OutRing
	 *******************************************************/
	if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
	{
		logit( "e", 
			"ew2ssam: Error starting Processor thread.  Exiting.\n");
		tport_detach (&InRegion);
		tport_detach (&OutRegion);
		return EW_FAILURE;
	}

	ProcessorStatus = 0; /*assume the best*/

/*--------------------- setup done; start main loop -------------------------*/

	while (tport_getflag (&InRegion) != TERMINATE  &&
               tport_getflag (&InRegion) != MyPid )
	{

		/* send ew2ssam' heartbeat
		***************************/
		if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
		{
			timeLastBeat = timeNow;
			ew2ssam_status (TypeHeartBeat, 0, ""); 
		}

		/* Check on our threads */
		if (MessageStackerStatus < 0)
		{
			logit ("et", 
				"ew2ssam: MessageStacker thread died. Exiting\n");
			return EW_FAILURE;
		}

		if (ProcessorStatus < 0)
		{
			logit ("et", 
				"ew2ssam: Processor thread died. Exiting\n");
			return EW_FAILURE;
		}

		sleep_ew (1000);

	} /* wait until TERMINATE is raised  */  

	/* Termination has been requested
	 ********************************/
	tport_detach (&InRegion);
	tport_detach (&OutRegion);
	logit ("t", "ew2ssam: Termination requested; exiting!\n" );
	return EW_SUCCESS;

}

/******************************************************************************
 *  ew2ssam_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int ew2ssam_config (char *configfile)
{
	char     		init[NUM_COMMANDS];     
						/* init flags, one byte for each required command */
	int      		nmiss;
						/* number of required commands that were missed   */
	char    		*com;
	char    		*str;
	int      		nfiles;
	int      		success;
	int      		i;


	/* Set to zero one init flag for each required command 
	*****************************************************/   
	for (i = 0; i < NUM_COMMANDS; i++)
		init[i] = 0;

	nLogo = 0;
	num_periods = 0;
	num_IncludeSCN = 0;
	num_ExcludeSCN = 0;

	/* Open the main configuration file 
	**********************************/
	nfiles = k_open (configfile); 
	if (nfiles == 0) 
	{
		logit ("e",
			"ew2ssam: Error opening command file <%s>; exiting!\n", 
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
					logit ("e", 
				  	  "ew2ssam: Error opening command file <%s>; exiting!\n", &com[1]);
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
	/*2*/	else if (k_its ("OutRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (OutRingName, str);
					init[2] = 1;
				}
			}
	/*3*/	else if (k_its ("HeartBeatInterval")) 
			{
				HeartBeatInterval = k_long ();
				init[3] = 1;
			}
	/*4*/	else if (k_its ("LogFile"))
			{
				LogSwitch = k_int();
				init[4] = 1;
			}

			/* Enter installation & module types to get
			 *******************************************/
	/*5*/	else if (k_its ("GetWavesFrom")) 
			{
				if (nLogo >= MAXLOGO) 
				{
					logit ("e", "ew2ssam: Too many <GetMsgLogo> commands in <%s>; "
										"; max=%d; exiting!\n", configfile, (int) MAXLOGO);
					return EW_FAILURE;
				}
				if ((str = k_str()) != NULL)
				{
					if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
					{
						logit ("e", "ew2ssam: Invalid installation name <%s> in "
											"<GetWavesFrom> cmd; exiting!\n", str);
						return EW_FAILURE;
					}
                }
				if ((str = k_str()) != NULL)
				{
					if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
					{
						logit ("e", "ew2ssam: Invalid module name <%s> in <GetWavesFrom> "
										"cmd; exiting!\n", str);
						return EW_FAILURE;
					}
				}
				/* We'll always fetch trace messages */
				if (GetType ("TYPE_TRACEBUF", &GetLogo[nLogo].type) != 0) 
				{
					logit ("e", "ew2ssam: Invalid msgtype <%s> in <GetWavesFrom> "
										"cmd; exiting!\n", str);
					return EW_FAILURE;
				}
				nLogo++;
				init[5] = 1;
			}


			/* Enter SCNs to be included 
			*******************************/
	/*6*/	else if (k_its ("IncludeSCN")) 
			{
				if (num_IncludeSCN >= max_IncludeSCN) 
				{
					/* Need to allocate more */
					max_IncludeSCN = max_IncludeSCN + SCN_INCREMENT;
					if ((IncludeSCN = (SCNstruct *) realloc (IncludeSCN, 
							(max_IncludeSCN * sizeof (SCNstruct)))) == NULL)
					{
						logit ("e", "ew2ssam: Call to realloc failed; exiting!\n");
						return EW_FAILURE;
					}
				}

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) STATION_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].sta, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) CHAN_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].chan, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) NETWORK_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].net, str);


				num_IncludeSCN = num_IncludeSCN + 1;
				init[6] = 1;

			} 
			/* Enter SCNs to be excluded 
			*******************************/
	/*NR*/	else if (k_its ("ExcludeSCN")) 
			{
				if (num_ExcludeSCN >= max_ExcludeSCN) 
				{
					/* Need to allocate more */
					max_ExcludeSCN = max_ExcludeSCN + SCN_INCREMENT;
					if ((ExcludeSCN = (SCNstruct *) realloc (ExcludeSCN, 
							(max_ExcludeSCN * sizeof (SCNstruct)))) == NULL)
					{
						logit ("e", "ew2ssam: Call to realloc failed; exiting!\n");
						return EW_FAILURE;
					}
				}

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) STATION_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].sta, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) CHAN_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].chan, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) NETWORK_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].net, str);


				num_ExcludeSCN = num_ExcludeSCN + 1;

			}
	/*7*/	else if (k_its ("MaxSCN")) 
			{
				MaxSCN = k_int ();
				init[7] = 1;
			}
	/*8*/	else if (k_its ("SsamPeriod")) 
			{
				if (num_periods >= MAX_TIME_PERIODS)
				{
					logit ("e", 
						"ew2ssam: Maximum number of <SsamPeriod> commands exceeded; exiting!\n");
									
					return EW_FAILURE;
				}
				
				SsamPeriod[num_periods] = k_val ();
				num_periods = num_periods + 1;
				
				init[8] = 1;
			}
        /*9*/   else if (k_its ("SsamTaper"))
                        {
                                SsamTaper = (float) k_val ();
                                init[9] = 1;
                         }
	/*NR*/	else if (k_its ("Debug"))
			{
				Debug = k_int();
			}

			/* Unknown command
			*****************/ 
			else 
			{
				logit ("e", "ew2ssam: <%s> Unknown command in <%s>.\n", 
								com, configfile);
				continue;
			}

			/* See if there were any errors processing the command 
			*****************************************************/
			if (k_err ()) 
			{
				logit ("e", 
					"ew2ssam: Bad <%s> command in <%s>; exiting!\n",
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
		logit ("e", "ew2ssam: ERROR, no ");
		if (!init[0])  logit ("e", "<MyModId> "        );
		if (!init[1])  logit ("e", "<InRing> "          );
		if (!init[2])  logit ("e", "<OutRing> " );
		if (!init[3])  logit ("e", "<HeartBeatInterval> "     );
		if (!init[4])  logit ("e", "<LogFile> "     );
		if (!init[5])  logit ("e", "<GetWavesFrom> "     );
		if (!init[6])  logit ("e", "<IncludeSCN> "     );
		if (!init[7])  logit ("e", "<MaxSCN> "     );
		if (!init[8])  logit ("e", "<SsamPeriod> "     );

		logit ("e", "command(s) in <%s>; exiting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  ew2ssam_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int ew2ssam_lookup( void )
{

	/* Look up keys to shared memory regions
	*************************************/
	if ((InKey = GetKey (InRingName)) == -1) 
	{
		fprintf (stderr,
				"ew2ssam:  Invalid ring name <%s>; exiting!\n", InRingName);
		return EW_FAILURE;
	}

	if ((OutKey = GetKey (OutRingName) ) == -1) 
	{
		fprintf (stderr,
			"ew2ssam:  Invalid ring name <%s>; exiting!\n", OutRingName);
		return EW_FAILURE;
	}

	/* Look up installations of interest
	*********************************/
	if (GetLocalInst (&InstId) != 0) 
	{
		fprintf (stderr, 
			"ew2ssam: error getting local installation id; exiting!\n");
		return EW_FAILURE;
	}


	if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
	{ 
		fprintf (stderr, 
			"ew2ssam: error getting wildcard installation id; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up modules of interest
	******************************/
	if (GetModId (MyModName, &MyModId) != 0) 
	{
		fprintf (stderr, 
			"ew2ssam: Invalid module name <%s>; exiting!\n", MyModName);
		return EW_FAILURE;
	}

	if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
	{
		fprintf (stderr, 
			"ew2ssam: Invalid module name <MOD_WILDCARD>; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up message types of interest
	*********************************/
	if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
	{
		fprintf (stderr, 
			"ew2ssam: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_ERROR", &TypeError) != 0) 
	{
		fprintf (stderr, 
			"ew2ssam: Invalid message type <TYPE_ERROR>; exiting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_TRACEBUF", &TypeWaveform) != 0) 
	{
		fprintf (stderr, 
			"ew2ssam: Invalid message type <TYPE_TRACEBUF>; exiting!\n");
		return EW_FAILURE;
	}

        if (GetType ("TYPE_VTABULARDATA", &TypeWaveform) !=0)
        {
               fprintf (stderr,
                        "ew2ssam: Invalid message type <TYPE_VTABULARDATA>; exiting!\n");
               return EW_FAILURE;
        }

	return EW_SUCCESS;

} 

/******************************************************************************
 * ew2ssam_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void ew2ssam_status( unsigned char type, short ierr, char *note )
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
		sprintf (msg, "%ld %d\n", (long) t, (int)MyPid);
	}
	else if (type == TypeError)
	{
		sprintf (msg, "%ld %hd %s\n", (long) t, ierr, note);
		logit ("et", "%s(%s): %s\n", MyProgName, MyModName, note);
	}

	size = (long)strlen (msg);   /* don't include the null byte in the message */

	/* Write the message to shared memory
	************************************/
	if (tport_putmsg (&OutRegion, &logo, size, msg) != PUT_OK)
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



/******************************************************************************
 * matchSCN () - set retind to the index of the SCN matching sta.cha.net      *
 *               found in the SCNlist array. Otherwise, set retind to -1      *
 *                                                                            *
 ******************************************************************************/
static int matchSCN (char *sta, char *chan, char *net, 
					SCNstruct *Include, int numInclude, 
					SCNstruct *Exclude, int numExclude, int *retind)
{
	int i;
	int j;

	int staWild = 0;
	int netWild = 0;
	int chanWild = 0;

	int staMatch = 0;
	int netMatch = 0;
	int chanMatch = 0;


	if ((sta == NULL) || (chan == NULL) || (net == NULL) || 
								(Include == NULL) || (numInclude < 0))
	{
		logit ("et",  "ew2ssam: invalid parameters to check_scn_logo\n");
		return (EW_FAILURE);
	}

	*retind = -1;

	/* Is the SCN in the include list */
	for (i = 0; i < numInclude; i++)
	{
		staWild = 0;
		netWild = 0;
		chanWild = 0;

		staMatch = 0;
		netMatch = 0;
		chanMatch = 0;

		/* Any wild cards ?*/
		if (strcmp (Include[i].sta, "*") == 0)
			staWild = 1;
		if (strcmp (Include[i].chan, "*") == 0)
			chanWild = 1;
		if (strcmp (Include[i].net, "*") == 0)
			netWild = 1;

		/* try to match explicitly */
		if (strcmp (sta, Include[i].sta) == 0)
			staMatch = 1;
		if (strcmp (chan, Include[i].chan) == 0)
			chanMatch = 1;
		if (strcmp (net, Include[i].net) == 0)
			netMatch = 1;

		if ((staWild == 1) ||  (staMatch == 1))  
			staMatch = 1;
		if ((netWild == 1) ||  (netMatch == 1)) 
			netMatch = 1;
		if ((chanWild == 1) || (chanMatch == 1))
			chanMatch = 1;

		/* If all 3 components match, set SCN was found in the 
 		 * Include list - now we have to check Exclude list */
		if (staMatch + netMatch + chanMatch == 3)
		{

			/* Is the SCN in the exclude list */
			for (j = 0; j < numExclude; j++)
			{
				staWild = 0;
				netWild = 0;
				chanWild = 0;

				staMatch = 0;
				netMatch = 0;
				chanMatch = 0;

				/* Any wild cards ? */
				if (strcmp (Exclude[j].sta, "*") == 0)
					staWild = 1;
				if (strcmp (Exclude[j].chan, "*") == 0)
					chanWild = 1;
				if (strcmp (Exclude[j].net, "*") == 0)
					netWild = 1;

				/* try to match explicitly */
				if (strcmp (sta, Exclude[j].sta) == 0)
					staMatch = 1;
				if (strcmp (chan, Exclude[j].chan) == 0)
					chanMatch = 1;
				if (strcmp (net, Exclude[j].net) == 0)
					netMatch = 1;

				if ((staWild == 1) ||  (staMatch == 1))  
					staMatch = 1;
				if ((netWild == 1) ||  (netMatch == 1)) 
					netMatch = 1;
				if ((chanWild == 1) || (chanMatch == 1))
					chanMatch = 1;

				/* If all 3 components match, set SCN was found in the 
 				 * Exclude list, we don't want it -   return -1 */
				if (staMatch + netMatch + chanMatch == 3)
				{
					*retind = -1;
					return EW_SUCCESS;
				}

			} /* loop over exclude list */

			/* If we made it this far - SCN was not
			 * in the Exclude list - return i */

			*retind = i;
			return EW_SUCCESS;
			
		} /* if SCN was found in the include list */

	}
		
	return EW_SUCCESS;
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
	TRACE_HEADER	*WaveHeader; 	
	int          	ret;

	/* Allocate space for input/output messages
	 *******************************************/
	if ((msg = (char *) malloc (MAX_BYTES_PER_EQ)) == (char *) NULL)
	{
		logit ("e", "ew2ssam: error allocating msg; exiting!\n");
		goto error;
	}

	WaveHeader = (TRACE_HEADER *) msg;


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
				ew2ssam_status (TypeError, ERR_TOOBIG, errText);
				continue;
			}
			else if (res == GET_MISS)
			{
				sprintf (errText, "missed msg(s) i%d m%d t%d in %s",
						(int) reclogo.instid, (int) reclogo.mod, 
						(int)reclogo.type, InRingName);
				ew2ssam_status (TypeError, ERR_MISSMSG, errText);
			}
			else if (res == GET_NOTRACK)
			{
				sprintf (errText, "no tracking for logo i%d m%d t%d in %s",
						(int) reclogo.instid, (int) reclogo.mod, 
						(int)reclogo.type, InRingName);
				ew2ssam_status (TypeError, ERR_NOTRACK, errText);
			}
		}

		/* If necessary, swap bytes in the wave message
		 **********************************************/
		if (WaveMsgMakeLocal (WaveHeader) < 0)
		{
			logit ("et", "%s(%s): Unknown waveform type.\n",
								MyProgName, MyModName);
			continue;
		}


		/* Check to see if msg's SCN code is desired
		 *********************************************/
		if (matchSCN (WaveHeader->sta, WaveHeader->chan, WaveHeader->net, 
					IncludeSCN, num_IncludeSCN, ExcludeSCN,
					num_ExcludeSCN, &res) != EW_SUCCESS)
		{
			logit ("et", "%s(%s): Call to matchSCN failed\n",
										MyProgName, MyModName);
			goto error;
		}

		/* If the message matches one of desired SCNs, queue it 
		 **********************************************************/
		if (res >= 0)
		{
/*
logit ("e", "Queuing %s.%s.%s\n", WaveHeader->sta, WaveHeader->chan,
							WaveHeader->net);
*/

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
					ew2ssam_status (TypeError, ERR_QUEUE, errText);
					goto error;
				}
				if (ret == -1)
				{
					sprintf (errText, 
						"queue cannot allocate memory. Lost message.");
					ew2ssam_status (TypeError, ERR_QUEUE, errText);
					continue;
				}
				if (ret == -3) 
				{
					sprintf (errText, "Queue full. Message lost.");
					ew2ssam_status (TypeError, ERR_QUEUE, errText);
					continue;
				}
			} /* problem from enqueue */

		} /* if we have a desired message */

	} /* while (1) */

	/* we're quitting
	 *****************/
	error:
	MessageStackerStatus = -1; /* file a complaint to the main thread */
	KillSelfThread (); /* main thread will restart us */
	return THR_NULL_RET; /* should never be reached */
}



/********************** Message Processing Thread ****************
 ******************************************************************/
thr_ret		Processor (void *dummy)
{


	int				ret;
	int				tmp = 0;
	int				i, j, index, found;
	int				sec_into_min, last_min;
        int                             nf,npts=0,k,nfreqs;
	double 			        packet_time_int,temp_time_period;
        double                          fft_real,fft_imaginary, modulus;
	double			tmpf;
	int				gotMsg;
    MSG_LOGO        reclogo;           /* logo of retrieved message */

	TRACE_HEADER	*WaveHeader;	   /* ptr to Wave data buffer       */
	int32_t			*WaveLong;		   /* if wave data is int32s         */
	short			*WaveShort;		   /* if wave data is shorts        */
	char 			*WaveBuf;          /* string to hold wave message   */
	long 			WaveBufLen;        /* length of WaveBuf             */

	VTABULAR_HEADER	*SsamHeader;	   /* ptr to Wave data buffer       */
	int32_t	*SsamLong;		   /* if wave data is int32s        */

	short			*SsamShort;		   /* if wave data is shorts        */
	char 			*SsamBuf;          /* string to hold wave message   */
	long 			SsamBufLen;        /* length of SsamBuf             */

	char			scn[256];


	/* Allocate the waveform buffer
     *******************************/
	WaveBufLen = (MAX_TRACEBUF_SIZ * sizeof (int32_t)) + sizeof (TRACE_HEADER);
	WaveBuf = (char *) malloc ((size_t) WaveBufLen);

	if (WaveBuf == NULL)
	{
		logit ("et", "ew2ssam: Cannot allocate waveform buffer\n");
		ProcessorStatus = -1;
		KillSelfThread();
	}


	/* Point to header and data portions of waveform message
	 *****************************************************/
	WaveHeader  = (TRACE_HEADER *) WaveBuf;
	WaveLong  = (int32_t *) (WaveBuf + sizeof (TRACE_HEADER));
	WaveShort = (short *) (WaveBuf + sizeof (TRACE_HEADER));


	/* Now, do the same for the Ssam trace message 
     ************************************************/
	/* We need room for 256 samples */
	SsamBufLen = ( num_freqs * sizeof (int32_t)) + sizeof (VTABULAR_HEADER);
	SsamBuf = (char *) malloc ((size_t) SsamBufLen);


	if (SsamBuf == NULL)
	{
		logit ("et", "ew2ssam: Cannot allocate Ssam waveform buffer\n");
		ProcessorStatus = -1;
		KillSelfThread();
	}


	/* Point to header and data portions of waveform message
	 *****************************************************/

        SsamHeader  = (VTABULAR_HEADER *) SsamBuf;
        SsamLong  = (int32_t *) (SsamBuf + sizeof (VTABULAR_HEADER) );
	SsamShort = (short *) (SsamBuf + sizeof (VTABULAR_HEADER) ); 



	while (1)
	{
		gotMsg = FALSE;
		while (gotMsg == FALSE)
		{
		
			RequestMutex ();
			ret = dequeue (&MsgQueue, WaveBuf, &WaveBufLen, &reclogo);
			ReleaseMutex_ew ();

			if (ret < 0)
			{
				/* empty queue */
				sleep_ew (1000);
			}
				else
				gotMsg = TRUE;
		} /* while no messages are dequeued */

		/* Just a little typing help */
		sprintf (scn, "%s.%s.%s", 
			WaveHeader->sta, WaveHeader->chan, WaveHeader->net);

		/* try to find this SCN in the SSAM list */
		i = 0; 
		index = -1; 
		found = FALSE; 
		while ((i < num_ssam) && (found == FALSE))
		{
			if ((strcmp (Ssam[i].sta, WaveHeader->sta) == 0) &&
			    (strcmp (Ssam[i].chan, WaveHeader->chan) == 0) &&
			    (strcmp (Ssam[i].net, WaveHeader->net) == 0))
			{
				index = i;
				found = TRUE;
			}

			i = i + 1;
		}

		/* if index not found, allocate a new struct */
		if (index == -1)
		{

			if (Debug > 2)
				logit ("e", "Got new SCN %s\n", scn);

			if (num_ssam >= MaxSCN)
			{
				logit ("e", "ew2ssam: Maximum number of SCNs reached; %s will be ignored.\n", scn);
			}
			else if ((strlen (WaveHeader->chan)) > MAX_CHAN_LEN)
			{
				logit ("e", "ew2ssam: Channel name too long; %s will be ignored.\n", scn);
			}
			else
			{
				/* allocate and initialize a new structure */

				/* SCN */
				strcpy (Ssam[num_ssam].sta, WaveHeader->sta);
				strcpy (Ssam[num_ssam].chan, WaveHeader->chan);
				strcpy (Ssam[num_ssam].net, WaveHeader->net);

				if ((Ssam[num_ssam].values = (SSAM_val *) 
							malloc (sizeof (SSAM_val))) == NULL)
				{
					logit ("e", 
						"ew2ssam: Cannot malloc Ssam->values; exiting!\n");
#ifdef _WINNT
                                        return THR_NULL_RET;
#else
					return NULL/* EW_FAILURE */;
#endif
				}

   /* cheryl - The time period to average over needs to be longer than  */
   /* the time period represented by the number of data samples sent to */
   /* the fft calculator.                                               */
          for (i=0; i<num_periods; i++)
          {
            if (SsamPeriod[i] < FFT_NN/WaveHeader->samprate)
            {
              logit ("e", "ew2ssam: Averaging time period must be greater than %f seconds, defaulting ...\n", FFT_NN/WaveHeader->samprate);

            }
          }


            
                                /* cheryl - minute syncing */
                                npts = 0;
				last_min = 0;
				sec_into_min = -1;
                                temp_time_period = -1;
                                Ssam[num_ssam].values->last_packet_endtime = 0;

				/* DC offset stuff */
				Ssam[num_ssam].values->DC_offset = DC_OFFSET_INVALID;
				Ssam[num_ssam].values->DC_cur_val = 0.0;
				Ssam[num_ssam].values->DC_cur_nsamp = 0;
				Ssam[num_ssam].values->DC_starttime = TIME_INVALID;

				Ssam[num_ssam].values->DC_cur_pos = 0;
				Ssam[num_ssam].values->DC_start_pos = 0;
				Ssam[num_ssam].values->DC_startup = TRUE;


				/* Ssam time interval stuff */
				for (i = 0; i < num_periods; i++)
				{

					Ssam[num_ssam].values->TP[i].time_period = SsamPeriod[i];
                                        for (nf = 0; nf < num_freqs+1; nf++)
                                        {
					   Ssam[num_ssam].values->TP[i].ssam_value[nf] = 0.0;
                                           Ssam[num_ssam].values->FFT.fft_data[nf] = 0.0;
                                           Ssam[num_ssam].values->FFT.frequency[nf] = WaveHeader->samprate*nf/FFT_NN;

                                        }
                                        Ssam[num_ssam].values->FFT.fft_nsamp = 1;
					Ssam[num_ssam].values->TP[i].ssam_nsamp = 0;
					Ssam[num_ssam].values->TP[i].ssam_starttime = TIME_INVALID;


					/* Build the outgoing channel name */
					sprintf (Ssam[num_ssam].values->TP[i].outchan, "%s_S%d", 
								Ssam[num_ssam].chan, i);

				}

				num_ssam = num_ssam + 1;
			}

		}

		if (index != -1)
		{
			if (Debug > 3)
				logit ("e", "%s: found at index %d\n", scn, index);

			/* SSAM PROCESSING HERE */

                        /* cheryl */
                        if (Debug > 3)
				logit ("e", "packet start time = %d, packet end time = %d\n", (int)WaveHeader->starttime,(int)WaveHeader->endtime);
			
                        /* cheryl */
                       packet_time_int = (int)WaveHeader->endtime - (int)WaveHeader->starttime;

                        if ((int)Ssam[index].values->last_packet_endtime != (int)WaveHeader->starttime && Ssam[index].values->last_packet_endtime != 0)

                        {
                              if (Debug > 2)
                                  logit ("e", "data stream interupted - reinitializing\n");
                              Ssam[index].values->DC_offset = DC_OFFSET_INVALID;
                              Ssam[index].values->DC_cur_val = 0.0;
                              Ssam[index].values->DC_cur_nsamp = 0;
                              Ssam[index].values->DC_starttime = TIME_INVALID;
                              Ssam[index].values->DC_cur_pos = 0;
                              Ssam[index].values->DC_start_pos = 0;
                              Ssam[index].values->DC_startup = TRUE;

                              for (i = 0; i < num_periods; i++)
                              {
                               for (nf = 0; nf < num_freqs-1; nf++)
                               {
                                Ssam[index].values->TP[i].ssam_value[nf] = 0.0;
                                Ssam[index].values->FFT.fft_data[nf] = 0.0;
                               }

                                Ssam[index].values->TP[i].ssam_nsamp = 0;
                                Ssam[index].values->TP[i].ssam_starttime = TIME_INVALID;
                              }
                        } /* data stream interuption */


			/* Add samples to the DC offset computation */
			if (Ssam[index].values->DC_cur_nsamp == 0)
			{
				Ssam[index].values->DC_starttime = WaveHeader->starttime;
			}

			for (i = 0; i < WaveHeader->nsamp; i++)
			{
				if ((strcmp (WaveHeader->datatype, "i2") == 0) ||
						(strcmp (WaveHeader->datatype, "s2") == 0))
				{
					tmp = (int) WaveShort[i];
				}
				else if ((strcmp (WaveHeader->datatype, "i4") == 0) ||
						(strcmp (WaveHeader->datatype, "s4") == 0))
				{
					tmp = (int) WaveLong[i];
				}

				Ssam[index].values->DC_cur_val = 
							Ssam[index].values->DC_cur_val + tmp;

				Ssam[index].values->DC_cur_nsamp = 
							Ssam[index].values->DC_cur_nsamp + 1;

			}

			/*
			 * If time difference between endtime and starttime 
			 * is bigger than DC_TIME_DIFF, compute the next 
			 * entry in the DC_array, reset counters, and compute
			 * an updated DC_offset value.
			 */
			if ((WaveHeader->endtime - 
					Ssam[index].values->DC_starttime) > DC_TIME_DIFF)
			{


				j = Ssam[index].values->DC_cur_pos;
				Ssam[index].values->DC_array[j] = 
						Ssam[index].values->DC_cur_val /
						Ssam[index].values->DC_cur_nsamp;
  
				Ssam[index].values->DC_cur_val = 0.0;
				Ssam[index].values->DC_cur_nsamp = 0;

				if (Debug > 2)
					logit ("e", "%s: DC offset portion at index %d = %f\n", scn, j, Ssam[index].values->DC_array[j]);

				/* 
				 * Do we have enough to compute a DC offset? 
				 * This only matters if this is one of the first packets of 
				 * this SCN, i.e. if startup is TRUE 
				 */
				if (Ssam[index].values->DC_startup == TRUE)
				{
					if (Debug > 2)
						logit ("e", "%s: WAIT: still in startup phase \n", scn);
					/*
					 * Next time around we will have enough to compute 
					 * a valid DC offset 
					 */
					if (Ssam[index].values->DC_cur_pos >= DC_STARTUP_ENTRIES)
					{
						Ssam[index].values->DC_startup = FALSE;
					}
				}
				else
				{

					j = Ssam[index].values->DC_start_pos;
					tmpf = 0.0;
					tmp = 0;
					while ((j < DC_ARRAY_ENTRIES) && 
						(j != Ssam[index].values->DC_cur_pos))
					{
						if (Debug > 2)
							logit ("e", "%s: Adding index %d = %f\n", scn, j, Ssam[index].values->DC_array[j]);

						tmpf = tmpf + Ssam[index].values->DC_array[j];
						tmp = tmp + 1;
						j = j + 1;
					}

					/* Wrap around the array, if we reach the right end */
					if ((j == DC_ARRAY_ENTRIES) && 
							(j != Ssam[index].values->DC_cur_pos))
					{

						j = 0;
						while (j != Ssam[index].values->DC_cur_pos)
						{

							if (Debug > 2)
								logit ("e", "%s: Adding index %d = %f\n", scn, j, Ssam[index].values->DC_array[j]);


							tmpf = tmpf + Ssam[index].values->DC_array[j];
							tmp = tmp + 1;
							j = j + 1;
						}
					}

					/* add the current portion */
					j = Ssam[index].values->DC_cur_pos;
					tmpf = tmpf + Ssam[index].values->DC_array[j];
					tmp = tmp + 1;

					if (Debug > 2)
						logit ("e", "%s: Adding index %d = %f\n", scn, j, Ssam[index].values->DC_array[j]);

						
					Ssam[index].values->DC_offset = tmpf / tmp;

					if (Debug > 1)
						logit ("e", "%s: New offset (%d-%d) ==> %f\n", scn,
							Ssam[index].values->DC_start_pos,
							Ssam[index].values->DC_cur_pos,
							Ssam[index].values->DC_offset);


				}


				/* increment position pointers */
				if ((Ssam[index].values->DC_cur_pos =  
							Ssam[index].values->DC_cur_pos + 1) >= DC_ARRAY_ENTRIES)
					Ssam[index].values->DC_cur_pos = 0;

				/* Increment start_position only if we have filled the array */
				if (Ssam[index].values->DC_cur_pos == Ssam[index].values->DC_start_pos)
				{
					if ((Ssam[index].values->DC_start_pos =  
							Ssam[index].values->DC_start_pos + 1) >= DC_ARRAY_ENTRIES)
						Ssam[index].values->DC_start_pos = 0;
				}

			} /* End of DC offset computation */
			
			/*cheryl*/
			last_min = (int)((int)WaveHeader->starttime/60);
 			sec_into_min = (int)WaveHeader->starttime - last_min * 60;
                        Ssam[index].values->last_packet_endtime = WaveHeader->endtime;

                        if (Debug > 4)
			  logit ("e", "last_min = %d, sec_into_min = %d, packet_time_int = %f \n", last_min, sec_into_min, packet_time_int);
			

			/* If a valid DC offset exists, continue */
			if (Ssam[index].values->DC_offset != DC_OFFSET_INVALID)	
			{

				if (Debug > 2)
					logit ("e", "%s: Got a valid offset %f - continuing\n", scn, Ssam[index].values->DC_offset);


				for (j = 0; j < num_periods; j++)
				{
					/* Set start time for a new ssam period */
					if (Ssam[index].values->TP[j].ssam_starttime == TIME_INVALID)
					{
						Ssam[index].values->TP[j].ssam_starttime = 
													WaveHeader->starttime;
					}

				}


                              if (Debug > 2)
                             logit("e","second = %d, ndata in fft_data= %d, nsamps=%d\n",sec_into_min, npts, Ssam[index].values->TP[0].ssam_nsamp);

				for (i = 0; i < WaveHeader->nsamp; i++)
				{

					if ((strcmp (WaveHeader->datatype, "i2") == 0) ||
							(strcmp (WaveHeader->datatype, "s2") == 0))
					{
						tmp = (int) WaveShort[i];
					}
					else if ((strcmp (WaveHeader->datatype, "i4") == 0) ||
							(strcmp (WaveHeader->datatype, "s4") == 0))
					{
						tmp = (int) WaveLong[i];
					}

					tmpf = (double) tmp - Ssam[index].values->DC_offset;
                                        npts = Ssam[index].values->FFT.fft_nsamp;
                                        Ssam[index].values->FFT.fft_data[npts] = tmpf;

                                        if (npts == FFT_NN)
                                        {
                                         taper(Ssam[index].values->FFT.fft_data,npts,SsamTaper);
                                         realft(Ssam[index].values->FFT.fft_data,npts,1);

                                         for (j = 0; j < num_periods; j++)
                                         {
                                            Ssam[index].values->TP[j].ssam_nsamp = Ssam[index].values->TP[j].ssam_nsamp + 1;
                                            Ssam[index].values->TP[j].ssam_value[0] += abs(Ssam[index].values->FFT.fft_data[1]);
                                            nfreqs = 1;
                                            for (k = 3; k < FFT_NN; k+=2)
                                            {
                                              fft_real = Ssam[index].values->FFT.fft_data[k];
                                              fft_imaginary = Ssam[index].values->FFT.fft_data[k+1]; 
                                              modulus = sqrt(fft_real*fft_real + fft_imaginary*fft_imaginary);
                                               
                                              Ssam[index].values->TP[j].ssam_value[nfreqs] = Ssam[index].values->TP[j].ssam_value[nfreqs] + modulus;

                                               nfreqs += 1;
                                            }
                                          }
                                          Ssam[index].values->FFT.fft_nsamp = 1;
                                        }
                                        else
                                        {
                                           Ssam[index].values->FFT.fft_nsamp = Ssam[index].values->FFT.fft_nsamp + 1;  
                                        }
				}

				if (Debug > 1)
					logit ("e", "%s: %f %d\n", scn, Ssam[index].values->TP[0].ssam_value[1],
						Ssam[index].values->TP[0].ssam_nsamp);



				/*
				 * Check elapsed time - if over time_period 
				 * prepare a new message a write it out
				 */
					
				for (j = 0; j < num_periods; j++)
				{
					if (Debug > 4)
						logit ("e", "%s: Period %f - %f-%f\n", scn,
							Ssam[index].values->TP[j].time_period,
							Ssam[index].values->TP[j].ssam_starttime, 
							WaveHeader->endtime);

			/* cheryl */
					if (Debug > 2)
                                                 logit ("e", "SSAM running total: %f, Number of samples: %d\n", Ssam[index].values->TP[j].ssam_value[1],Ssam[index].values->TP[j].ssam_nsamp);


                        /* cheryl added time syncing condition to the */
                        /*  following if statement.  If the the data  */
                        /*  packet is at a new minute mark but less   */
                        /*  than 60 seconds has elapsed, the buffers  */
                        /*  are written out and the averaging sums    */
                        /*  reinitialized. This has only been tested  */
                        /*  for 1 second data packets.                */
                        /* For Ssam:  Added condition that the time   */
                        /*  period averaged over must be greater than */
                        /*  the time period represented by the numbrer*/
                        /*  of data points sent to the fft calculator.*/
                        /*  If requested time period is too small the */
                        /*  module will default to 512 samples.       */ 

					temp_time_period = Ssam[index].values->TP[j].time_period; 

					if  ( ( (((temp_time_period * 
						(WaveHeader->endtime/temp_time_period -
						(int)(WaveHeader->endtime/temp_time_period)))
						< packet_time_int) && (temp_time_period >= 60.0)) ||
						(((int)WaveHeader->endtime - 
						(int)Ssam[index].values->TP[j].ssam_starttime) >=
						(Ssam[index].values->TP[j].time_period)) ) && (Ssam[index].values->TP[j].ssam_nsamp >= 1) )  
 
					{

					/* Fill the header of the message */

						strcpy (SsamHeader->sta, WaveHeader->sta);
						strcpy (SsamHeader->chan, Ssam[index].values->TP[j].outchan);
						strcpy (SsamHeader->net, WaveHeader->net);
                        strcpy (SsamHeader->tbl_code, "SS");
                        SsamHeader->nrows = 1;
                        time((time_t *) &SsamHeader->msgtime);  
                        strcpy (SsamHeader->RowHeader.rowpad,"      ");
						SsamHeader->RowHeader.endtime = WaveHeader->endtime;
						SsamHeader->RowHeader.samprate = WaveHeader->samprate;
                        SsamHeader->RowHeader.ncol = -1;  
						/* changed by CJB 4/23/01 to store interval length */
                        /* SsamHeader->RowHeader.opt = 0; */
						SsamHeader->RowHeader.opt = (long) Ssam[index].values->TP[j].time_period;


                        if ((strcmp(WaveHeader->datatype, "i4") == 0) || (strcmp(WaveHeader->datatype, "s4") == 0))
                        {
							SsamHeader->RowHeader.row_size = 2 * sizeof (int) + sizeof (int32_t) + 2 * sizeof (double) + sizeof (SsamHeader->RowHeader.rowpad) + num_freqs * sizeof (int32_t);
#if defined _SPARC
                        strcpy (SsamHeader->datatype, "s4");
							
#elif defined _INTEL

                        strcpy(SsamHeader->datatype, "i4");
#else
						logit ("e", "ew2ssam: Must have either SPARC or INTEL defined during compilation; exiting!");
						ProcessorStatus = -1;
						KillSelfThread();
#endif
						}
 
                        else if ((strcmp(WaveHeader->datatype , "i2") == 0) || (strcmp(WaveHeader->datatype , "s2") == 0 ))
                        {
							SsamHeader->RowHeader.row_size = 2 * sizeof (int) + sizeof (int32_t) + 2 * sizeof (double) + sizeof (SsamHeader->RowHeader.rowpad) + num_freqs * sizeof (short);
#if defined _SPARC
                        strcpy (SsamHeader->datatype, "s2");
							
#elif defined _INTEL

                        strcpy(SsamHeader->datatype, "i2");
#else
						logit ("e", "ew2ssam: Must have either SPARC or INTEL defined during compilation; exiting!");
						ProcessorStatus = -1;
						KillSelfThread();
#endif
						}
                        else
                        {
							logit ("e", "ew2ssam: Must have datatype defined\n"); 
                        }


                        for (nf = 0; nf < num_freqs-1; nf++)
                        {
							if ((strcmp (WaveHeader->datatype,"i4") == 0) || (strcmp (WaveHeader->datatype,"s4") == 0))
							{

								SsamLong[nf] =  (int32_t) (0.01 *
								(Ssam[index].values->TP[j].ssam_value[nf] / (float) Ssam[index].values->TP[j].ssam_nsamp));
								if (Debug > 0)
									logit ("e", "%s: SSAM-%f (%f/%d) = %d\n", 
										scn, Ssam[index].values->TP[j].time_period, 
										Ssam[index].values->TP[j].ssam_value[nf], 
										Ssam[index].values->TP[j].ssam_nsamp, 
										SsamLong[nf]);
                            }
							else if ((strcmp (WaveHeader->datatype, "i2") == 0) || (strcmp (WaveHeader->datatype, "s2") == 0))
							{
								SsamShort[nf] = (short) (0.01 * 
									(Ssam[index].values->TP[j].ssam_value[nf] / (float) Ssam[index].values->TP[j].ssam_nsamp));
								if (Debug > 0)
									logit ("e", "%s: SSAM-%f (%f/%d) = %d\n", 
										scn, Ssam[index].values->TP[j].time_period, 
										Ssam[index].values->TP[j].ssam_value[nf], 
										Ssam[index].values->TP[j].ssam_nsamp, 
										SsamShort[nf]);
                            }
							else
                            {
								logit("e", "Error - ew2ssam must have datatype defined\n"); 
                            } 
                                             
						} /* for nf */


						/* Reset all book-keeping counters */
                        for (i = 0; i < num_freqs-1; i++)
                        {
							Ssam[index].values->TP[j].ssam_value[i] = 0.0;
                        }
						Ssam[index].values->TP[j].ssam_nsamp = 0;
						Ssam[index].values->TP[j].ssam_starttime = TIME_INVALID;
	
	

						/* Write it OUT */
						reclogo.instid = InstId;
						reclogo.mod = MyModId;
						reclogo.type = TypeWaveform;

						
						if (tport_putmsg (&OutRegion, &reclogo, 
							SsamBufLen, SsamBuf) != PUT_OK)
						{
							logit ("et", "ew2ssam:  Error sending message: %d-%d-%d.\n",
								reclogo.instid, reclogo.mod, reclogo.type);
							ProcessorStatus = -1;
							KillSelfThread();
						}                          
					} /* is it time to write this ssam value out? */
				} /* loop over time periods */
			} /* If we have a valid DC offset */

		} /* If we have a valid index */

		ProcessorStatus = 0;

	} /* while 1*/
	return THR_NULL_RET; /* should never be reached */
}


/************************************************************/
void realft(double data[], unsigned long n, int isign)
/*  Calculates the fourier transform of a set of n real-valued  */
/*  data points.  Replaces this data (which is stored in array  */
/*  data[1,...,n]) by the positive frequency half of it's       */
/*  complex fourier transform.  The real-valued first and last  */
/*  components of the complex transform are returned as elements*/
/*  data[1] and data[2], respectively.  n must be a power of 2. */
/*  This routine also calculates the inverse transform of a     */
/*  complex data array if it is the transform of real data.     */
/*  (Result in this case must be multiplied by N/2.)            */

{
   void four1(double data[], unsigned long nn, int isign);
   unsigned long i, i1, i2, i3, i4, np3;
   double c1=0.5, c2, h1r, h1i, h2r, h2i;
   double wr, wi, wpr, wpi, wtemp, theta;

   theta = 3.141592653589793/(double) (n>>1);
   if (isign ==1)
   {
      c2 = -0.5;
      four1(data, n>>1,1);
   }
   else
   {
      c2 = 0.5;
      theta = -theta;
   }

   wtemp = sin(0.5 * theta);
   wpr = -2.0 * wtemp * wtemp;
   wpi = sin(theta);
   wr = 1.0 + wpr;
   wi = wpi;
   np3 = n + 3;
   for (i=2; i<=(n>>2); i++)
   {
      i4 = 1 + (i3=np3 - (i2=1 + (i1=i+i-1)));
      h1r = c1 * (data[i1] + data[i3]);
      h1i = c1 * (data[i2] - data[i4]);
      h2r = -c2 * (data[i2] + data[i4]);
      h2i = c2 * (data[i1] - data[i3]);
      data[i1] = h1r + wr * h2r - wi * h2i;
      data[i2] = h1i + wr * h2i + wi * h2r;
      data[i3] = h1r - wr * h2r + wi * h2i;
      data[i4] = -h1i + wr * h2i + wi * h2r;
      wr = (wtemp=wr) * wpr - wi * wpi + wr;
      wi = wi * wpr + wtemp * wpi + wi;
    }
    if (isign == 1)
    {
      data[1] = (h1r=data[1]) + data[2];
      data[2] = h1r - data[2];
    }
    else
    {
      data[1] = c1 * ((h1r=data[1]) + data[2]);
      data[2] = c1 * (h1r - data[2]);
      four1(data, n>>1, -1);
    }

    return;
}


/************************************************************/
void four1(double data[], unsigned long nn, int isign)
/* Replaces data[1...2*nn] by it's discreet Fourier transform,   */
/* if isign is input as 1; or replaces data[1..2*nn] by nn times */
/* it's inverse discreet Fourier transform, if isign is input    */
/* as -1.  Data array is a complex array of length nn or,        */
/* equivalently, a real array of length 2*nn.  nn must be an    */
/* integer power of 2 (this is not checked for!).               */

{
    unsigned long n, mmax,m,j,istep,i;
    double wtemp,wr,wpr,wpi,wi,theta;
    double tempr,tempi;

    n = nn << 1;
/*    printf("nn = %d\n",nn); */
    j = 1;
    for (i = 1; i < n; i+= 2)
    {
        if (j > i)
        {
            SWAP(data[j],data[i]);
            SWAP(data[j+1],data[i+1]);
        }
        m = n >> 1;
        while (m >= 2 && j > m)
        {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

/* Danielson-Lanczos section. */
    mmax = 2;
    while (n > mmax)
    {
        istep = mmax << 1;
        theta = isign*(6.28318530717959/mmax);
        wtemp = sin(0.5 * theta);
        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m = 1; m < mmax; m += 2)
        {
            for (i = m; i <= n; i += istep)
            {
                j = i + mmax;
                tempr = wr * data[j] - wi * data[j+1];
                tempi = wr * data[j+1] + wi * data[j];
                data[j] = data[i] - tempr;
                data[j+1] = data[i+1] - tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
        }
        mmax = istep;
    }

    return;
}


/*******************************************************************/

void taper(double data[], unsigned long nn, double width)

/*  Taper applies a Hanning taper to the data. 0 < width =< 0.5.    */
/*  No taper is applied if width = 0.                               */

{
  double pi, f0, f1, omega = 0.0, factor;
  int i, ntaper;

  ntaper = (int) (nn * width);
  pi = 3.14159265;
  f0 = f1 = 0.5;
  if (ntaper != 0)
  {
     omega = pi/ntaper;
  }

  for (i = 0; i<ntaper; i++)
  {
     factor = f0 - f1*cos(omega*i);
     data[i] = data[i] * factor;
     data[nn-1-i] = data[nn-1-i] * factor;
  }
  return;
}


