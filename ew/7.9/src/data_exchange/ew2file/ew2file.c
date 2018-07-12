/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ew2file.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.12  2010/04/19 18:56:44  quintiliani
 *     Fixed compilation problem for declaration of MAXPATHLEN
 *
 *     Revision 1.11  2008/04/15 20:56:33  paulf
 *     readKeeper had a minor formatting bug that could cause a memory fault
 *
 *     Revision 1.10  2007/03/28 17:14:47  paulf
 *     removed malloc.h since it is in earthworm.h
 *
 *     Revision 1.9  2007/03/28 15:19:16  paulf
 *     MACOSX check added
 *
 *     Revision 1.8  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.7  2005/11/15 21:23:44  dietz
 *     Modified format of timestring in filenames to "yyyymmdd-hhmmss."
 *
 *     Revision 1.6  2005/11/11 00:12:23  dietz
 *     Added optional command, TimeBasedFilename, to control whether output
 *     filenames include a time string derived from message contents.
 *     Currently coded to do this for: TYPE_PICK2K, TYPE_PICK_SCNL,
 *     TYPE_HYP2000ARC.
 *
 *     Revision 1.5  2005/07/25 15:58:21  friberg
 *     added in _LINUX directive to include sys/param.h
 *
 *     Revision 1.4  2004/07/26 17:20:44  dietz
 *     added msglen argument to writeFile() instead of using strlen() within
 *     writeFile(). This allows writeFile to be used with binary msgs too.
 *
 *     Revision 1.3  2003/11/25 00:45:56  dietz
 *     Fixed bug in timing of heartbeat file generation.
 *     Had been using HeartBeatInt instead of HeartFileInt to control
 *     the production loop.  Caused grief when HeartFileInt==0.
 *
 *     Revision 1.2  2003/01/31 17:21:52  dietz
 *     Changed timestamp in all error messages from human-readable to
 *     sec-since-1970 so that statmgr could interpret them.
 *
 *     Revision 1.1  2002/12/20 02:36:33  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 *   ew2file.c
 *  
 *   Program to read messages (with user given logos) from a ring
 *   and write them verbatim to files in one or more directories.
 *
 *   Pete Lombard - 11/29/2002
 *
 */

#define EW2FILE_VERSION "2.0.2 - May 31, 2016"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <time_ew.h>
#include <errno.h>
#include <signal.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <mem_circ_queue.h> 
#include <read_arc.h> 

#ifdef _LINUX
#include <sys/param.h> 	/* needed for MAXPATHLEN */
#endif

#ifdef _MACOSX
#include <sys/param.h> 	/* needed for MAXPATHLEN */
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

#define MAX_STR 512


/* Functions in this source file 
*******************************/
void ew2file_config  ( char * );
void ew2file_lookup  ( void );
void ew2file_status  ( MSG_LOGO *, char * );
int  writeFile(char *, size_t, char *, char *, char *, int, char *);
int writeOneFile(char *msg, size_t msglen, char *dir, char *suffix, char *errText);
void increment( int *);
int  readKeeper( char *filename);
void buildTimeString( char *msg, size_t msglen, unsigned char msgtype, 
                      char *tstring );

void buildQID(char *msg, int msgtype, char *tstring);

/* Thread things
***************/
#define THREAD_STACK 8192
static unsigned tidDup;              /* Dup thread id */
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /*   to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue; 		     /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret WriteThd( void * );
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and Dup thread */
/* Message Buffers to be allocated
*********************************/
static char *Rdmsg = NULL;           /* msg retrieved from transport      */ 
static char *Wrmsg = NULL;           /*  message to write to file   */

/* Timers
******/
time_t MyLastBeat;            /* time of last local (into Earthworm) hearbeat */

char *cmdname;          /* pointer to executable name */ 
pid_t MyPid;		/* Our own pid, sent with heartbeat for restart purposes */
   
#define DEF_HEARTFILE_MSG "Alive\n"
#define DEF_HEARTFILE_SFX "hrt"

#define MAX_SERIAL     10000000
#define HEART_SERIAL    9999999
#define SERIAL_FORMAT  "%07d"

/* Things to read or derive from configuration file
**************************************************/
char      InRing[MAX_RING_STR];    /* name of transport ring for input  */
char      MyModName[MAX_MOD_STR];  /* speak as this module name/id      */
char      Keeper[256];      /* filename for keeping serial number       */
int       LogSwitch;        /* 0 if no logfile should be written        */
int       HeartBeatInt;     /* seconds between heartbeat messages       */
int       HeartFileInt;     /* seconds between heartbeat files          */
char      *HeartFileMsg;    /* string for heartbeat file                */
char      *HeartFileSuffix; /* suffix for heartbeat file                */
long      MaxMsgSize;       /* max size for input/output msgs           */
int       QueueSize;	    /* max messages in output circular buffer   */
SHM_INFO  InRegion;         /* shared memory region to use for input    */
MSG_LOGO  *GetLogo;         /* array for requesting module,type,instid  */
short 	  nLogo;            /* number of logos to get                   */
int       TimeBasedFilename;/* flag to control if filename includes time*/ 
int       QIDinFilename;    /* flag to control if qid should be put in filename */
int       NoKeeperInFilename;    /* flag to control if keeper num should be in filename */
int       OneFilePerMessage;/* flag to control one or multifile output  */ 
char      **Suffix;	    /* array of suffix strings                  */
char      *tempDir;         /* Directory for creating and writing files */
char      **SendDirs;       /* array of destination directories         */
int       nDirs;
int       fileCount;

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static MSG_LOGO HeartLogo;          /* logo of heartbeat message          */
static MSG_LOGO ErrorLogo;          /* logo of error message              */
static unsigned char TypeError;
static unsigned char TypePick2K;
static unsigned char TypePickSCNL;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeMagnitude;

/* Error messages used by ew2file 
***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE         3   /* queue error                             */
#define  ERR_WRITE         4   /* error writing message file              */

int main( int argc, char **argv )
{
    /* Other variables: */
    long     recsize;	/* size of retrieved message             */
    MSG_LOGO reclogo;	/* logo of retrieved message             */
    time_t   now;	/* current time, used for timing heartbeats */
    char     errText[256];     /* string for log/error/heartbeat messages           */

    /* Check command line arguments 
     ******************************/
    cmdname = argv[0];
    if ( argc != 2 )
    {
	fprintf( stderr, "Usage: %s <configfile>\n", cmdname );
	fprintf( stderr, "Version: %s\n", EW2FILE_VERSION );
	return( 0 );
    }

    /* Initialize name of log-file & open it 
     ****************************************/
    logit_init( argv[1], 0, 512, 1 );
  
    /* Read the configuration file(s)
     ********************************/
    ew2file_config( argv[1] );
    logit( "et" , "%s(%s): Read command file <%s>\n", 
           cmdname, MyModName, argv[1] );

    /* Look up important info from earthworm.h tables
     *************************************************/
    ew2file_lookup();

    /* Reinitialize the logging level
     *********************************/
    logit_init( argv[1], 0, 512, LogSwitch );

    /* Get our own Pid for restart purposes
     ***************************************/
    MyPid = getpid();
    if(MyPid == -1)
    {
	logit("e", "%s(%s): Cannot get pid; exiting!\n", cmdname, MyModName);
	return(0);
    }

    /* Allocate space for input/output messages for all threads
     ***********************************************************/
    /* Buffer for Read thread: */
    if ( ( Rdmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL ) 
    {
	logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n", 
	       cmdname, MyModName );
	return( -1 );
    }

    /* Buffers for Write thread: */
    if ( ( Wrmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL ) 
    {
	logit( "e", "%s(%s): error allocating SSmsg; exiting!\n",
	       cmdname, MyModName );
	return( -1 );
    }

    /* Create a Mutex to control access to queue
     ********************************************/
    CreateMutex_ew();

    /* Initialize the message queue
     *******************************/
    initqueue( &OutQueue, (unsigned long)QueueSize,(unsigned long)(MaxMsgSize+1));
	   	
    /* Attach to Input/Output shared memory ring 
     ********************************************/
    tport_attach( &InRegion, InRingKey );

    /* step over all messages from transport ring
     *********************************************/
    while( tport_getmsg( &InRegion, GetLogo, nLogo, 
			 &reclogo, &recsize, Rdmsg, MaxMsgSize ) != GET_NONE);
   

    /* One heartbeat to announce ourselves to statmgr
     ************************************************/
    time(&MyLastBeat);
    sprintf( errText, "%ld %ld\n", (long) MyLastBeat, (long) MyPid);
    ew2file_status( &HeartLogo, errText);

    /* Start the message stacking thread if it isn't already running.
     ****************************************************************/
    if (MessageStackerStatus != MSGSTK_ALIVE )
    {
	if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
	{
	    logit( "e", 
		   "%s(%s): Error starting  MessageStacker thread; exiting!\n",
		   cmdname, MyModName );
	    tport_detach( &InRegion );
	    exit( -1 );
	}
	MessageStackerStatus = MSGSTK_ALIVE;
    }

    /* Start the file writing thread
     ***********************************/
    if ( StartThread(  WriteThd, (unsigned)THREAD_STACK, &tidDup ) == -1 )
    {
	logit( "e", "%s(%s): Error starting Write thread; exiting!\n",
	       cmdname, MyModName );
	tport_detach( &InRegion );
	exit( -1 );
    }

    /* Start main ew2file service loop, which aimlessly beats its heart.
     **********************************/
    while( tport_getflag( &InRegion ) != TERMINATE  &&
	   tport_getflag( &InRegion ) != MyPid         )
    {
	/* Beat the heart into the transport ring
         ****************************************/
	time(&now);
	if (difftime(now,MyLastBeat) > (double)HeartBeatInt ) 
	{
	    sprintf( errText, "%ld %ld\n", (long) now, (long) MyPid);
	    ew2file_status( &HeartLogo, errText);
	    MyLastBeat = now;
	}

	/* take a brief nap; added 970624:ldd 
         ************************************/
	sleep_ew(500);
    } /*end while of monitoring loop */

    /* Shut it down
     ***************/
    tport_detach( &InRegion );
    logit("t", "%s(%s): termination requested; exiting!\n", 
          cmdname, MyModName );
    exit( 0 );	
}
/* *******************  end of main *******************************
******************************************************************/


/**************************  Main Write Thread   ***********************
 *          Pull a messsage from the queue, and write it to a FILE!  *
 **********************************************************************/

thr_ret WriteThd( void *dummy )
{
    MSG_LOGO reclogo;
    time_t   now;
    time_t   lastHeartFile = 0;
    int      ret;
    int      i, index;
    long     msgSize;
    int      twoInARow = 0;
    char     errText[256];
    char     timestring[17];  /* format: "yyyymmdd-hhmmss." */

    while (1) {   /* main loop */
	/* Get message from queue
         *************************/
	if (HeartFileInt > 0) {
	    time(&now);
	    if (difftime(now,lastHeartFile) > (double)HeartFileInt ) {
		for (i = 0; i < nDirs; i++) {
                    timestring[0] = 0; /* null time string */
		    if (writeFile(HeartFileMsg, strlen(HeartFileMsg),
                                  SendDirs[i], timestring, HeartFileSuffix, 
				  HEART_SERIAL, 
				  errText) < 0) {
			ew2file_status( &ErrorLogo, errText );
		    }
		}
		lastHeartFile = now;
	    }
	}

	RequestMutex();
	ret=dequeue( &OutQueue, Wrmsg, &msgSize, &reclogo);
	ReleaseMutex_ew();
	
	if (ret < 0 )
	{ /* -1 means empty queue */
	    sleep_ew(500);
	    continue;
	}
     
	/* Determine which GetLogo this message used */
	for(i = 0; i < nLogo; i++) {
	    if ( (GetLogo[i].type   == WILD || 
		  GetLogo[i].type   == reclogo.type) &&
		 (GetLogo[i].mod    == WILD || 
		  GetLogo[i].mod    == reclogo.mod) &&
		 (GetLogo[i].instid == WILD || 
		  GetLogo[i].instid == reclogo.instid) )
		break;
	}
	if (i == nLogo) {
	    logit("et", "%s error: logo <%d.%d.%d> not found\n",
		  cmdname, reclogo.instid, reclogo.mod, reclogo.type);
	    continue;
	}
	index = i;

        /* Create time-string for filenaming (if so configured ) */
        timestring[0] = 0; /* nullify the existing string */
        if( TimeBasedFilename ) {
            buildTimeString( Wrmsg, msgSize, reclogo.type, timestring );
        }

        /* put QID at end of string. */
        if ( QIDinFilename ) {
            buildQID( Wrmsg, reclogo.type, timestring );
        }
  
        /* Write the message to disk file(s) */
	for (i = 0; i < nDirs; i++) {
	    if (OneFilePerMessage) {
              if( writeOneFile(Wrmsg, msgSize, SendDirs[i], Suffix[index], errText)< 0) {
		ew2file_status( &ErrorLogo, errText );
              } 
            }
            else if (writeFile(Wrmsg, msgSize, SendDirs[i], timestring, Suffix[index], 
                          fileCount, errText) < 0) {
		ew2file_status( &ErrorLogo, errText );
		twoInARow++;
		if (twoInARow == 2)
		    exit( -1 );
	      } else {
		twoInARow = 0;
              }
	}
	increment(&fileCount);
    }   /* End of main loop */
   
}

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
    long         recsize;	/* size of retrieved message             */
    MSG_LOGO     reclogo;       /* logo of retrieved message             */
    time_t       now;
    char         errText[256]; /* string for log/error/heartbeat messages */
    int          ret;
    int          NumOfTimesQueueLapped= 0; /* number of messages lost due to 
					      queue lap */

    /* Tell the main thread we're ok
     ********************************/
    MessageStackerStatus = MSGSTK_ALIVE;

    /* Start main service loop for current connection
     ************************************************/
    while( 1 )
    {
	/* Get a message from transport ring
         ************************************/
	ret = tport_getmsg( &InRegion, GetLogo, nLogo, 
			    &reclogo, &recsize, Rdmsg, MaxMsgSize );

	switch (ret) {
	case GET_NONE:
	    /* Wait if no messages for us */
	    sleep_ew(100); 
	    continue;
	    break;
	case GET_TOOBIG:
	    time(&now);
	    sprintf( errText, "%ld %hd msg[%ld] i%d m%d t%d too long for target",
		     (long)now, (short)ERR_TOOBIG, recsize, (int) reclogo.instid,
		     (int) reclogo.mod, (int)reclogo.type );
	    ew2file_status( &ErrorLogo, errText );
	    continue;
	    break;
	case GET_MISS:
	    time(&now);
	    sprintf( errText, "%ld %hd missed msg(s) i%d m%d t%d in %s",
		     (long)now, (short)ERR_MISSMSG, (int) reclogo.instid,
		     (int) reclogo.mod, (int)reclogo.type, InRing );
            ew2file_status( &ErrorLogo, errText );
	    break;
	case GET_NOTRACK:
	    time(&now);
	    sprintf( errText, "%ld %hd no tracking for logo i%d m%d t%d in %s",
		     (long)now, (short)ERR_NOTRACK, (int) reclogo.instid,
		     (int) reclogo.mod, (int)reclogo.type, InRing );
            ew2file_status( &ErrorLogo, errText );
	    break;
	}

                     
	/* Process retrieved msg (ret==GET_OK,GET_MISS,GET_NOTRACK) 
         ***********************************************************/
	Rdmsg[recsize] = '\0';
      
	/* put it into the 'to be shipped' queue */
	/* the Write thread is in the biz of de-queueng and writing */
	RequestMutex();
	ret=enqueue( &OutQueue, Rdmsg, recsize, reclogo ); 
	ReleaseMutex_ew();

	switch(ret) {
	case -2:
	    /* Serious: quit */
	    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
	    time(&now);
	    sprintf(errText,"%ld %hd internal queue error. Terminating.", (long)now, (short)ERR_QUEUE);
	    ew2file_status( &ErrorLogo, errText );
	    goto error;
	    break;
	case -1:
	    time(&now);
	    sprintf(errText,"%ld %hd message too big for queue.", (long)now,
		    (short)ERR_QUEUE);
	    ew2file_status( &ErrorLogo, errText );
	    continue;
	    break;
	case -3:
	    NumOfTimesQueueLapped++;
	    if (!(NumOfTimesQueueLapped % 5)) {
		logit("t", 
		      "%s(%s): Circular queue lapped 5 times. Messages lost.\n",
		      cmdname, MyModName);
		if (!(NumOfTimesQueueLapped % 100)) {
		    logit( "et", 
			   "%s(%s): Circular queue lapped 100 times. Messages lost.\n",
			   cmdname, MyModName);
		}
	    }
	    continue; 
	}
    } /* end of while */

    /* we're quitting 
     *****************/
 error:
    MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
    KillSelfThread(); /* main thread will not restart us */
    return THR_NULL_RET; /* Should never get here */
}

/*****************************************************************************
 *  ew2file_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.	             *
 *****************************************************************************/
#define NCOMMANDS 10
void ew2file_config( char *configfile )
{
    int      ncommand;     /* # of required commands you expect to process   */ 
    char     init[NCOMMANDS];     /* init flags, one byte for each required command */
    int      nmiss;        /* number of required commands that were missed   */
    char    *com;
    int      nfiles;
    int      success;
    int      i;	
    char*    str;

    /* Set to zero one init flag for each required command 
     *****************************************************/   
    ncommand = NCOMMANDS;
    for( i=0; i<ncommand; i++ )  init[i] = 0;
    nLogo = 0;
    GetLogo = NULL;
    tempDir = NULL;
    SendDirs = NULL;
    Suffix = NULL;
    nDirs = 0;
    HeartFileInt = 0;
    HeartFileMsg = DEF_HEARTFILE_MSG;
    HeartFileSuffix = DEF_HEARTFILE_SFX;
    TimeBasedFilename = 0;
    QIDinFilename = 0;
    NoKeeperInFilename = 0;
    OneFilePerMessage = 0;
   
    /* Open the main configuration file 
     **********************************/
    nfiles = k_open( configfile ); 
    if ( nfiles == 0 ) {
	logit( "e" ,
	       "%s: Error opening command file <%s>; exiting!\n", 
	       cmdname, configfile );
	exit( -1 );
    }

    /* Process all command files
     ***************************/
    while(nfiles > 0)   /* While there are command files open */
    {
        while(k_rd())        /* Read next line from active file  */
        {  
	    com = k_str();         /* Get the first token from line */

	    /* Ignore blank lines & comments
             *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

	    /* Open a nested configuration file 
             **********************************/
            if( com[0] == '@' ) {
		success = nfiles+1;
		nfiles  = k_open(&com[1]);
		if ( nfiles != success ) {
		    logit( "e" , 
			   "%s: Error opening command file <%s>; exiting!\n",
                           cmdname, &com[1] );
		    exit( -1 );
		}
		continue;
            }

	    /* Process anything else as a command 
             ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("InRing") ) {
                str = k_str();
                if(str) strcpy( InRing, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_int();
                init[3] = 1;
            }

	    /* Enter installation & module & message types to get
             ****************************************************/
  /*4*/     else if( k_its("GetMsgLogo") ) {
		if ((GetLogo = (MSG_LOGO*)realloc(GetLogo, (nLogo+1) * sizeof(MSG_LOGO))) 
		    == NULL) {
		    logit( "e" , 
			   "%s: out of memory for Logos\n",cmdname );
		    exit( -1 );
		}		
		if ((Suffix = (char **)realloc(Suffix, (nLogo+1) * sizeof(char *))) 
		    == NULL) {
		    logit( "e" , 
			   "%s: out of memory for suffixes\n",cmdname );
		    exit( -1 );
		}		
		if( ( str=k_str() ) != NULL ) {
		    if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
			logit( "e" , 
			       "%s: Invalid installation name <%s>", cmdname, str ); 
			logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
			exit( -1 );
		    }
		}
                if( ( str=k_str() ) != NULL ) {
		    if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
			logit( "e" , 
                               "%s: Invalid module name <%s>", cmdname, str ); 
			logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
			exit( -1 );
		    }
                }
                if( ( str=k_str() ) != NULL ) {
		    if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
			logit( "e" , 
                               "%s: Invalid msgtype <%s>", cmdname, str ); 
			logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
			exit( -1 );
		    }
                }
                if( ( str=k_str() ) != NULL ) {
		    if ((Suffix[nLogo] = strdup(str)) == NULL) {
			logit( "e" , 
			       "%s: out of memory for suffix strings\n", cmdname ); 
			exit( -1 );
		    }
		    if (strchr(str, '.') != NULL) {
			logit( "e", "%s: invalid suffix <%s>: `.' not allowed\n",
			       cmdname, str);
			exit( -1 );
		    }
                }
		nLogo++;
                init[4] = 1;
            }
			
	    /* Maximum size (bytes) for incoming/outgoing messages
             *****************************************************/ 
  /*5*/     else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[5] = 1;
            }

	    /* Maximum number of messages in outgoing circular buffer
             ********************************************************/ 
  /*6*/     else if( k_its("QueueSize") ) {
                QueueSize = k_long();
                init[6] = 1;
            }

	    /* Temporary directory, for creating and writing files
             ********************************************************/ 
  /*7*/     else if( k_its("TempDir") ) {
                if( ( str=k_str() ) != NULL ) {
		    if ( (tempDir = strdup(str)) == NULL) {
			logit( "e" , 
                               "%s: out of memory for TempDir\n", cmdname ); 
			exit( -1 );
		    }
                }
                init[7] = 1;
            }

	    /* Maximum number of messages in outgoing circular buffer
             ********************************************************/ 
  /*8*/     else if( k_its("SendDir") ) {
                if ((SendDirs = (char **)realloc(SendDirs, (nLogo+1) * sizeof(char *))) 
		    == NULL) {
		    logit( "e" , "%s: out of memory for SendDir\n",cmdname );
		    exit( -1 );
		}		
		if( ( str=k_str() ) != NULL ) {
		    if ( (SendDirs[nDirs] = strdup(str)) == NULL) {
			logit( "e" , 
                               "%s: out of memory for SendDir\n", cmdname ); 
			exit( -1 );
		    }
		}
		nDirs++;
                init[8] = 1;
            }

	    /* Serial keeper file
             ********************/
    /*9*/   else if( k_its("SerialKeeper") ) {
		if ( (str = k_str() ) != NULL) {
		    strncpy(Keeper, str, 255);
		    Keeper[255] = '\0';
		    init[9] = 1;
		}
	    }
		
	 /* Optional commands */

/*opt*/     else if( k_its("OneFilePerMessage") ) {
		OneFilePerMessage = k_int();
	    }

/*opt*/     else if( k_its("TimeBasedFilename") ) {
		TimeBasedFilename = k_int();
	    }
/*opt*/     else if( k_its("QIDinFilename") ) {
		QIDinFilename = k_int();
	    }
/*opt*/     else if( k_its("NoKeeperInFilename") ) {
		NoKeeperInFilename = k_int();
	    }

	    /* Interval for creating heartbeat files
             ***************************************/
/*opt*/     else if( k_its("HeartFileInt") ) {
		HeartFileInt = k_int();
	    }

	    /* Heartbeat File message
             ********************************************************/ 
/*opt*/     else if( k_its("HeartFileMsg") ) {
                if( ( str=k_str() ) != NULL ) {
		    if ( (HeartFileMsg = malloc(strlen(str)+2)) == NULL) {
			logit( "e" , 
                               "%s: out of memory for HeartFileMsg\n", cmdname ); 
			exit( -1 );
		    }
		    sprintf(HeartFileMsg, "%s\n", str);
                }
            }

	    /* Heartbeat File suffix
             ********************************************************/ 
/*opt*/     else if( k_its("HeartFileSuffix") ) {
                if( ( str=k_str() ) != NULL ) {
		    if ( (HeartFileSuffix = strdup(str)) == NULL) {
			logit( "e" , 
                               "%s: out of memory for HeartFileSuffix\n", cmdname ); 
			exit( -1 );
		    }
		    if (strchr(str, '.') != NULL) {
			logit("e", "%s: invalid HeartFileSuffix <%s>: `.' not allowed\n",
			      cmdname, str);
			exit( -1 );
		    }
                }
            }

	    /* Unknown command
             *****************/ 
	    else {
                logit( "e" , "%s: <%s> Unknown command in <%s>.\n", 
		       cmdname, com, configfile );
                continue;
            }
 
	    /* See if there were any errors processing the command 
             *****************************************************/
            if( k_err() ) {
		logit( "e" , 
                       "%s: Bad <%s> command  in <%s>; exiting!\n",
		       cmdname, com, configfile );
		exit( -1 );
            }
	}
	nfiles = k_close();
    }

    /* After all files are closed, check init flags for missed commands
     ******************************************************************/
    nmiss = 0;
    for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
    if ( nmiss ) {
	logit( "e", "%s: ERROR, no ", cmdname );
	if ( !init[0] )  logit( "e", "<LogFile> "      );
	if ( !init[1] )  logit( "e", "<MyModuleId> "   );
	if ( !init[2] )  logit( "e", "<InRing> "     );
	if ( !init[3] )  logit( "e", "<HeartBeatInt> " );
	if ( !init[4] )  logit( "e", "<GetMsgLogo> "   );
	if ( !init[5] )  logit( "e", "<MaxMsgSize> "  );
	if ( !init[6] )  logit( "e", "<Queue>"   );
	if ( !init[7] )  logit( "e", "<TempDir>" );
	if ( !init[8] )  logit( "e", "<SendDir>" );
	if ( !init[9] )  logit( "e", "<serialKeeper>" );
	logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
	exit( -1 );
    }

    /* Make sure our directories exist */
    if (RecursiveCreateDir(tempDir) != EW_SUCCESS) {
	logit("e", "%s error creating TempDir: %s\n", cmdname, strerror(errno));
	exit( -1 );
    }
    for (i = 0; i < nDirs; i++) {
	if (RecursiveCreateDir(SendDirs[i]) != EW_SUCCESS) {
	    logit("e", "%s error creating SendDir %s: %s\n", cmdname, SendDirs[i],
		  strerror(errno));
	    exit( -1 );
	}
    }
    if (readKeeper(Keeper) < 0) {
	exit( -1 );  /* readKeeper alreay complained */
    }
    
    return;
}

/****************************************************************************
 *  ew2file_lookup( )   Look up important info from earthworm.h tables      *
 ****************************************************************************/
void ew2file_lookup( void )
{
    /* Look up keys to shared memory regions
     *************************************/
    if( ( InRingKey = GetKey(InRing) ) == -1 ) {
	logit( "e",
               "%s:  Invalid ring name <%s>; exiting!\n", 
               cmdname, InRing);
	exit( -1 );
    }   

    /* Look up installations of interest
     *********************************/
    if ( GetLocalInst( &HeartLogo.instid ) != 0 ) {
	logit( "e",
               "%s: error getting local installation id; exiting!\n",
               cmdname );
	exit( -1 );
    }
    ErrorLogo.instid = HeartLogo.instid;
   
    /* Look up modules of interest
     ***************************/
    if ( GetModId( MyModName, &HeartLogo.mod ) != 0 ) {
	logit( "e",
               "%s: Invalid module name <%s>; exiting!\n", 
               cmdname, MyModName );
	exit( -1 );
    }
    ErrorLogo.mod = HeartLogo.mod;

    /* Look up message types of interest
     *********************************/
    if ( GetType( "TYPE_HEARTBEAT", &HeartLogo.type ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", cmdname );
	exit( -1 );
    }
    if ( GetType( "TYPE_ERROR", &ErrorLogo.type ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_ERROR>; exiting!\n", cmdname );
	exit( -1 );
    }
    TypeError = ErrorLogo.type;

    if ( GetType( "TYPE_PICK2K", &TypePick2K ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_PICK2K>; exiting!\n", cmdname );
	exit( -1 );
    }
    if ( GetType( "TYPE_PICK_SCNL", &TypePickSCNL ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_PICK_SCNL>; exiting!\n", cmdname );
	exit( -1 );
    }
    if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_HYP2000ARC>; exiting!\n", cmdname );
	exit( -1 );
    }
    if ( GetType( "TYPE_MAGNITUDE", &TypeMagnitude ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_MAGNITUDE>; exiting!\n", cmdname );
	exit( -1 );
    }

    
    return;
} 

/***************************************************************************
 * ew2file_status() sends an error or heartbeat message to transport       *
 *    If the message is of TYPE_ERROR, the text will also be logged.       *
 *    Since ew2file_status is called be more than one thread, the logo     *
 *    and message must be constructed by the caller.                       *
 ***************************************************************************/
void ew2file_status( MSG_LOGO *logo, char *msg)
{
    size_t size;
    
    if( logo->type == TypeError )
	logit( "et", "%s\n", msg );

    size = strlen( msg );   /* don't include the null byte in the message */ 	

    /* Write the message to shared memory
     ************************************/
    if( tport_putmsg( &InRegion, logo, (long)size, msg ) != PUT_OK ) {
	logit("et", "%s(%s):  Error sending message to transport.\n", 
	      cmdname, MyModName );
    }
    return;
}

/* the function to write multiple messages to a named file 
	returns 0 upon success or -1 and sets errText upon failure
*/

int writeOneFile(char *msg, size_t msglen, char *dir, char *suffix, char *errText)
{
    char outFile[MAXPATHLEN];
    time_t now;
    int ret;
    FILE *fp;

    sprintf( outFile,  "%s/%s", dir, suffix);
    if ( (fp = fopen(outFile, "ab")) == NULL) {
	time(&now);
	sprintf(errText, "%ld %hd error creating file %s: %s", (long)now, (short)ERR_WRITE,
		outFile, strerror(errno));
	return( -1 );
    }
    ret = (int)fwrite(msg, msglen, 1, fp);
    if ( ret != 1) {
	time(&now);
	sprintf(errText, "%ld %hd error writing file %s: %s", (long)now, (short)ERR_WRITE,
		outFile, strerror(errno));
	fclose(fp);
	return( -1 );
    }
    if (fclose(fp) != 0) {
	time(&now);
	sprintf(errText, "%ld %hd error completing file %s: %s", (long)now, (short)ERR_WRITE,
		outFile, strerror(errno));
	fclose(fp);
	return( -1 );
    }
    return( 0 );
}

/* the function to write ONE message to an individually named file 

	returns 0 upon success or -1 and sets errText upon failure
*/
int writeFile(char *msg, size_t msglen, char *dir, char *tstring, char *suffix, 
              int serial, char *errText)
{
    char tempFile[MAXPATHLEN];
    char outFile[MAXPATHLEN];
    char format[40];
    int ret;
    FILE *fp;
    time_t now;
    
    if (NoKeeperInFilename) {
        sprintf( tempFile, "%s/%s.%s", tempDir, tstring, suffix);
        sprintf( outFile, "%s/%s.%s", dir, tstring, suffix);
    } else {
        sprintf( format, "%%s/%%s%s.%%s", SERIAL_FORMAT);
        sprintf( tempFile, format, tempDir, tstring, serial, suffix);
        sprintf( outFile, format, dir, tstring, serial, suffix);
    }
    if ( (fp = fopen(tempFile, "wb")) == NULL) {
	time(&now);
	sprintf(errText, "%ld %hd error creating file %s: %s", (long)now, (short)ERR_WRITE,
		tempFile, strerror(errno));
	return( -1 );
    }
    ret = (int)fwrite(msg, msglen, 1, fp);
    if ( ret != 1) {
	time(&now);
	sprintf(errText, "%ld %hd error writing file %s: %s", (long)now, (short)ERR_WRITE,
		tempFile, strerror(errno));
	fclose(fp);
	return( -1 );
    }
    if (fclose(fp) != 0) {
	time(&now);
	sprintf(errText, "%ld %hd error completing file %s: %s", (long)now, (short)ERR_WRITE,
		tempFile, strerror(errno));
	fclose(fp);
	return( -1 );
    }
    if (rename_ew(tempFile, outFile) != 0) {
	time(&now);
	sprintf(errText, "%ld %hd error moving file %s to %s: %s", (long)now, (short)ERR_WRITE,
		tempFile, outFile, strerror(errno));
	return( -1 );
    }
    
    return( 0 );
}

void increment(int *count)
{
    FILE *fp;
    char format[20];
    
    *count = ((*count) + 1) % MAX_SERIAL;
    
    if ( (fp = fopen(Keeper, "w")) == NULL) {
	logit("et", "%s: error opening %s\n", cmdname, Keeper);
	return;
    }
    fprintf(fp, "# last serial number used by ew2file:\n");
    sprintf(format, "%s\n", SERIAL_FORMAT);
    fprintf(fp, format, *count);
    if (fclose(fp) != 0) {
	logit("et", "%s: error writing %s\n", cmdname, Keeper);
    }
    return;
}

int readKeeper( char *filename)
{
    FILE *fp;
    char line[80];
    char format[20];
    
    if ( (fp = fopen(filename, "r")) == NULL) {
	if (errno == ENOENT) {
	    logit("e", "%s: Keeper file <%s> not found, creating a new one\n",
		  cmdname, filename);
	    if ( (fp = fopen(Keeper, "w")) == NULL) {
		logit("e", "%s: error creating Keeper file %s: %s\n",
		      cmdname, filename, strerror(errno));
		return( -1 );
	    }
	    fileCount = 0;
	    fprintf(fp, "# last serial number used by ew2file:\n");
	    sprintf(format, "%s\n", SERIAL_FORMAT);
	    fprintf(fp, format, fileCount);
	    if (fclose(fp) != 0) {
		logit("et", "%s: error writing %s: %s\n", cmdname, filename,
		      strerror(errno));
		return( -1 );
	    }
	    return( 0 );
	} else {
	    logit("e", "%s: error reading Keeper file %s: %s\n", cmdname,
		  filename, strerror(errno));
	    return( -1 );
	}
    }
    fgets(line, 79, fp);  /* read the comment */
    if (fgets(line, 79, fp) == NULL) {
	if (feof(fp)) {
	    logit("e", "%s: Keeper file %s damaged\n", cmdname, filename);
	    fclose(fp);
	    return( -1 );
	} else {
	    logit("e", "%s: error reading Keeper file %s: %s\n", cmdname,
		  filename, strerror(errno));
	    fclose(fp);
	    return( -1 );
	}
    }
    sscanf(line, "%d", &fileCount);
    logit("e", "%s Initial serial: ", cmdname);
    sprintf(format, "%s\n", SERIAL_FORMAT);
    logit("e", format, fileCount);
    fclose(fp);
    return( 0 );
}
void buildQID(char *msg, int msgtype, char *tstring) {
struct Hsum Sum;               /* Hyp2000 summary data                   */
char     *in;             /* working pointer to archive message    */
char      line[MAX_STR];  /* to store lines from msg               */
char      shdw[MAX_STR];  /* to store shadow cards from msg        */
char      qid_str[MAX_STR];
int       qid;


   qid_str[0]=0;
   if( msgtype == TypeHyp2000Arc ) {
	in = msg;
        sscanf( in, "%[^\n]", line );
        in += strlen( line ) + 1;
        sscanf( in, "%[^\n]", shdw );
        in += strlen( shdw ) + 1;
        read_hyp( line, shdw, &Sum );
	sprintf(qid_str, "qid%08ld.", Sum.qid);
   } else if ( msgtype == TypeMagnitude ) {
        sscanf(msg, "%d MAG", &qid); /* should be the first item on the first line */
	sprintf(qid_str, "qid%08d.", qid);
   }
   strcat(tstring, qid_str);
}

void buildTimeString( char *msg, size_t msglen, unsigned char msgtype, char *tstring )
{
   char tmpstr[15];    /* temporary storage for yyyymmddhhmmss */
   int  datelen = 14;  /* length of yyyymmddhhmmss */
   int  i;

   tstring[0] = 0;     /* make it a null string! */

   if( msgtype == TypePick2K ) {
      if( msglen < 31+datelen ) return;
      strncpy( tmpstr, &msg[30], datelen );
      tmpstr[datelen]=0;
   }
   else if( msgtype == TypePickSCNL ) {
      char tmp[40];
      sscanf( msg, "%*d %*d %*d %*d %*s %*s %s", tmp );
      if( strlen(tmp) < datelen ) return;
      strncpy( tmpstr, tmp, datelen );
      tmpstr[datelen]=0;
   }
   else if( msgtype == TypeHyp2000Arc ) {
      if( msglen < datelen ) return;
      strncpy( tmpstr, msg, datelen );
      tmpstr[datelen]=0;
   }
   else { /* not-coded-to or cannot get tstring from other msg types */
      return;
   }
   
/* make sure there are no blanks in the temporary time string */
   for( i=0; i<strlen(tmpstr); i++ ) if( tmpstr[i]==' ' ) tmpstr[i]='0';

/* build final-format time string */
   strncpy( tstring, tmpstr, 8 );         /* yyyymmdd */
   tstring[8]  = '-';                     /* -        */
   strncpy( &tstring[9], &tmpstr[8], 6 ); /* hhmmss   */
   tstring[15] = '.';                     /* .        */
   tstring[16] =  0;                      /* null     */

   return;
}
