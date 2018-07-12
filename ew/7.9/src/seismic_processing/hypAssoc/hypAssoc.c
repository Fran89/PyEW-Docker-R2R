
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hypAssoc.c,v 1.1 2010/01/01 00:00:00 jmsaurel Exp $
 *
 *    Revision history:
 *     $Log: hypAssoc.c,v $
 *     Revision 1.1 2010/01/01 00:00:00 jmsaurel
 *     Initial revision
 *
 *
 */


#include "hypAssoc.h"

/* Functions in this source file 
*******************************/
static int     hypAssoc_config ( char * );
static int     hypAssoc_lookup ( void );
static void    hypAssoc_status ( MSG_LOGO *, short, char *, long );
static thr_ret MessageStacker( void * );
static thr_ret EventAssociator( void * );
static thr_ret EventOutput( void * );
static EVENT  *NewEvent ( EVENT * , struct Hsum );
static int     UpdateEvent ( EVENT * , struct Hpck );
static int     OutputEvent ( EVENT * );
static int     PhaseComp (struct Hpck , struct Hpck );
static void    UpdateHypo (EVENT *, struct Hsum );
static int     write_hyp_( struct Hsum , char ** );
static int     write_phs_( struct Hpck , char ** );
int write_term_( struct Hsum SumP, char **outptr);

/* The message queue
*******************/
static	QUEUE OutQueue;             /* from queue.h, queue.c; sets up linked */

/* Thread things
***************/
#define THREAD_STACK 8192
static unsigned tidAssociator;   /* Message processing thread id */
static unsigned tidOutput;       /* Message processing thread id */
static unsigned tidStacker;      /* Thread moving messages from transport *
                                  *   to queue */
static	int MessageStackerStatus=0;     /* 0=> Stacker thread ok. <0 => dead */
static	int EventAssociatorStatus=0;    /* 0=> Snipper thread ok. <0 => dead */
static	int EventOutputStatus=0;        /* 0=> Snipper thread ok. <0 => dead */

/* Transport global variables
****************************/
static  SHM_INFO  InRegion;    /* shared memory region to use for input    */
static  SHM_INFO  OutRegion;   /* shared memory region to use for output   */
#define MAXLOGO   10
static	MSG_LOGO  GetLogo[MAXLOGO];   /* array for requesting module,type,instid */
static  MSG_LOGO  errLogo;        /* Error logo                    */
static	short     nLogo;
static  MSG_LOGO  putlogo;       /* logo of sent message     */

        
/* Globals to set from configuration file
****************************************/
static int     QueueSize = 10;     /* max messages in output circular buffer */
static char    InRing[MAX_RING_STR+1];     /* name of transport ring for i/o */
static char    OutRing[MAX_RING_STR+1];    /* name of transport ring for i/o */
static char    MyModName[MAX_MOD_STR+1];   /* speak as this module name/id   */
static int     LogSwitch;           /* 0 if no logfile should be written   */
static long    HeartBeatInt;        /* seconds between heartbeats          */
static char   *QueueFile = NULL;    /* optional file to saveread queue */
static double  DelayTime = 0;       /* seconds to wait to process trigger */
static time_t  WaitTime = 0;        /* seconds to wait to process trigger */
static int     CheckLocCode = 0;    /* seconds to wait to process trigger */

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          InRingKey;     /* key of transport ring for i/o      */
static long          OutRingKey;    /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeHyp2000Arc;

/* Error messages used by hypAssoc 
***************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         3   /* error queueing message for sending     */
#define  ERR_FILE          4   /* error writing queue dump file          */
static char Text[MAXTXT];      /* string for log/error messages          */

/* Things to be malloc'd
***********************/
/* story: Some of these things don't have to be global; they are only
   so that we can malloc them from main at startup time. Having them 
   malloc'd by the routine which uses them presents the danger that the 
   module would start ok, but die (if unable to malloc) only when the 
   first event hits. Which could be much later, at a bad time...*/

EVENT	*Event;
size_t	maxEvent=MAX_EVENT;
size_t	maxPhases=MAX_PHASES;

/* Debug debug DEBUG */
static int Debug = 0;  

static pid_t      MyPid;

int main( int argc, char **argv )
{
    time_t       timeNow;            /* current time                   */       
    time_t       timeLastBeat;       /* time last heartbeat was sent   */
    MSG_LOGO     hrtLogo;            /* heartbeat logo                 */
    int          i, rc = 0;
   
    /* Initialize name of log-file & open it initially
       defaults to disk logging. */
    logit_init( argv[1], 0, (DB_MAX_TRIG_BYTES + 256), 1 );

    /* Check command line arguments */
    if ( argc != 2 ) {
	fprintf( stderr, "Usage: hypAssoc <configfile>\n" );
	return( 1 );
    }

    /* Read the configuration file(s) */
    if (hypAssoc_config (argv[1]) != EW_SUCCESS) {
	logit( "e", "hypAssoc: Call to hypAssoc_config failed \n");
	return( 1 );
    }
    
    /* Reset logit to the desired level of logging */
    logit_init( argv[1], 0, (DB_MAX_TRIG_BYTES + 256), LogSwitch );
    logit( "" , "hypAssoc: Read command file <%s>\n", argv[1] );

    /* Look up important info from earthworm.h tables */
    if (hypAssoc_lookup () != EW_SUCCESS) {
	logit( "e", "hypAssoc: Call to hypAssoc_lookup failed \n");
	return( 1 );
    }
 
    /* initialize the socket system  */
    SocketSysInit();

    /* get pid for restart */
    MyPid = getpid();

    hrtLogo.instid = InstId;
    hrtLogo.mod    = MyModId;
    hrtLogo.type   = TypeHeartBeat;

    /* Initialize outgoing logo */
    putlogo.instid = InstId;
    putlogo.mod    = MyModId;
    putlogo.type = TypeHyp2000Arc;

    /* Allocate space for Quake FIFO, Pick FIFO, sorting array 
     *********************************************************/
    Event = (EVENT *) calloc( maxEvent, sizeof( EVENT ) );
    if( Event == (EVENT *) NULL ) {
	logit( "et","hypAssoc: Error allocating Event stack at length=%ld; "
		"exiting!\n", (long)maxEvent );
	exit( -1 );
    }
    logit( "", "hypAssoc: Allocated event stack (length=%ld)\n", (long)maxEvent );

    for ( i=0; i < maxEvent; i++ ) {
	Event[i].Phases = (struct Hpck *) calloc( maxPhases, sizeof( struct Hpck ) );
	if( Event[i].Phases == (struct Hpck *) NULL ) {
		logit( "et","hypAssoc: Error allocating Event Phases stack at length=%ld; "
			"exiting!\n", (long)maxPhases );
		free( Event );
		exit( -1 );
	    }
	Event[i].TimeCreated = 0;
    }
    logit( "", "hypAssoc: Allocated event phases stacks (length=%ld)\n", (long)maxPhases );

    /* Force a heartbeat to be issued in first pass thru main loop */
    timeLastBeat = time(&timeNow) - HeartBeatInt - 1;
   
    /* Create a Mutex to control access to queue */
    CreateMutex_ew();

    /* Initialize the message queue */
    initqueue ( &OutQueue, (unsigned long)QueueSize,
		(unsigned long)DB_MAX_BYTES_PER_EQ + sizeof(time_t));

    /* Load the queue from the dump file */
    if (QueueFile != NULL) {
	rc = undumpqueue(&OutQueue, QueueFile);
	switch( rc ) {
	case 0:   /* All went well */
	    if (Debug)
		logit("", "successfully read %d msgs into queue\n", 
		      getNumOfElementsInQueue( &OutQueue));
	    break;
	case +1:  /* File doesn't exist */
	    logit("e", "hypAssoc: QueueFile <%s> doesn't exist\n", QueueFile);
	    break;
	case -1:  /* Error reading file */
	    logit("e", "hypAssoc: error reading <%s>: %s\n", QueueFile, 
		  strerror(errno));
	    break;
	case -2:  /* Too many entries */
	    logit("e", "hypAssoc: too many entries in %s for queue; exiting\n",
		  QueueFile);
	    return( 1 );
	case -3:   /* entry too large */
	    logit("e", "hypAssoc: entry too large in %s; exiting\n", QueueFile);
	    return( 1 );
	case -4:   /* Time stamps don't match */
	    logit("e", "hypAssoc: timestamp mismatch in %s; queue starts empty\n",
		  QueueFile);
	    break;
	default:
	    logit("e", "hypAssoc: unknown return (%d) from undumpqueue\n", rc);
	}
    }

    /* Attach to shared memory rings
     *******************************/
    tport_attach( &InRegion, InRingKey );
    logit( "", "trig2hyp: Attached to public memory region: %ld\n",
	InRingKey );
    tport_attach( &OutRegion, OutRingKey );
    logit( "", "trig2hyp: Attached to public memory region: %ld\n",
	OutRingKey );

    if(Debug == 1) 
	logit("e","starting to watch for hyp2000arc messages\n");

    /* Start the  message stacking thread */
    if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 ) {
	logit( "e", "Error starting  MessageStacker thread. Exiting.\n" );
	tport_detach( &InRegion );
	return( 1 );
    }
    MessageStackerStatus=0; /*assume the best*/

    /* Start the  event associator thread */
    if ( StartThread(  EventAssociator, (unsigned)THREAD_STACK, &tidAssociator ) == -1 ) {
	logit( "e", "Error starting  MessageStacker thread. Exiting.\n" );
	return( 1 );    
    }
    EventAssociatorStatus=0; /*assume the best*/

    /* Start the  event output thread */
    if ( StartThread(  EventOutput, (unsigned)THREAD_STACK, &tidOutput ) == -1 ) {
	logit( "e", "Error starting  MessageStacker thread. Exiting.\n" );
	tport_detach( &OutRegion );
	return( 1 );    
    }
    EventOutputStatus=0; /*assume the best*/

    /* Having delegated message collecting, and message processing, there's *
     * not much for us left to do: watch thread status, and beat the heart  */

    /* begin main loop till Earthworm shutdown (level 1) */
    while( tport_getflag(&InRegion) != TERMINATE  &&
	   tport_getflag(&InRegion) != MyPid         ) {
	/* send hypAssoc's heartbeat */
	if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInt ) {
	    timeLastBeat = timeNow;
	    hypAssoc_status( &hrtLogo, 0, "", MyPid ); 
	}

	/* see how our threads are feeling */
	if ( EventOutputStatus < 0) {
	    logit("et","Event output thread died. Exiting\n");
	    return( 1 );
	}
	if ( EventAssociatorStatus < 0) {
	    logit("et","Event associator processing thread died. Exiting\n");
	    return( 1 );
	}
	if ( MessageStackerStatus < 0) {
	    logit("et","Message stacking thread died. Exiting\n");
	    return( 1 );
	}

	sleep_ew( 3000 );   
    }  /* end of until shutdown requested */  

    /* Tell our threads to die */
    MessageStackerStatus = 1;
    EventAssociatorStatus = 1;
    EventOutputStatus = 1;
    sleep_ew(1000);
  
    /* Termination has been requested */
    tport_detach( &InRegion ); /* detach from shared memory */
    tport_detach( &OutRegion ); /* detach from shared memory */
    logit( "t", "Termination requested; exiting!\n" );

    /* clean up after ourselves */
    
    return( 0 );
}
/*------------------------ end of main() -----------------------------*/


/***********************************************************************
 *  hypAssoc_config() processes command file(s) using kom.c functions;*
 *                  exits if any errors are encountered.               *
 ***********************************************************************/
int hypAssoc_config (char *configfile)
{
    int      ncommand;     /* # of required commands you expect to process   */ 
    char     init[20];     /* init flags, one byte for each required command */
    int      nmiss;        /* number of required commands that were missed   */
    char    *com;
    char    *str;
    int      nfiles;
    int      success;
    int      i;

    /* Set to zero one init flag for each required command  */
    ncommand = 8;
    for( i=0; i < ncommand; i++ )  
	init[i] = 0;
    nLogo = 0;
    Debug = 0;

    /* Open the main configuration file  */
    nfiles = k_open( configfile ); 
    if ( nfiles == 0 ) {
	logit( "e",
	       "hypAssoc: Error opening command file <%s>; exiting!\n", 
	       configfile );
	return EW_FAILURE;
    }

    /* Process all command files */
    while(nfiles > 0) {   /* While there are command files open */
	while(k_rd()) {        /* Read next line from active file  */
	    com = k_str();         /* Get the first token from line */
	    
	    /* Ignore blank lines & comments */
	    if( !com )           continue;
	    if( com[0] == '#' )  continue;
	    
	    /* Open a nested configuration file  */
	    if( com[0] == '@' ) {
		success = nfiles+1;
		nfiles  = k_open(&com[1]);
		if ( nfiles != success ) {
		    logit( "e", 
			   "hypAssoc: Error opening command file <%s>; exiting!\n",
			   &com[1] );
		    return EW_FAILURE;
		}
		continue;
	    }
	    
	    /* Process anything else as a command  */
	    /*0*/     if( k_its("LogFile") ) {
		LogSwitch = k_int();
		init[0] = 1;
	    }
	    /*1*/     else if( k_its("MyModuleId") ) {
		str = k_str();
		if(str) {
		    strncpy( MyModName, str, MAX_MOD_STR-1 );
		    MyModName[MAX_MOD_STR-1] = '\0';
		}
		init[1] = 1;
	    }
	    /*2*/     else if( k_its("InRing") ) {
		str = k_str();
		if(str) {
		    strncpy( InRing, str, MAX_RING_STR-1 );
		    InRing[MAX_RING_STR-1] = '\0';
		}
		init[2] = 1;
	    }
	    /*3*/     else if( k_its("OutRing") ) {
		str = k_str();
		if(str) {
		    strncpy( OutRing, str, MAX_RING_STR-1 );
		    OutRing[MAX_RING_STR-1] = '\0';
		}
		init[3] = 1;
	    }
	    /*4*/     else if( k_its("HeartBeatInt") ) {
		HeartBeatInt = k_long();
		init[4] = 1;
	    }
	    
	    /* Enter installation & module to get event messages from */
	    /*5*/     else if( k_its("GetEventsFrom") ) {
		if ( nLogo >= MAXLOGO ) {
		    logit( "e", 
			   "hypAssoc: Too many <GetEventsFrom> commands in <%s>", 
			   configfile );
		    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO );
		    return EW_FAILURE;
		}
		if( ( str=k_str() ) ) {
		    if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
			logit( "e", 
			       "hypAssoc: Invalid installation name <%s>", str ); 
			logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
			return EW_FAILURE;
		    }
		    GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
		}
		if( ( str=k_str() ) ) {
		    if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
			logit( "e", 
			       "hypAssoc: Invalid module name <%s>", str ); 
			logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
			return EW_FAILURE;
		    }
		    GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
		}
		if( GetType( "TYPE_HYP2000ARC", &GetLogo[nLogo].type ) != 0 ) {
		    logit( "e", 
			   "hypAssoc: Invalid message type <TYPE_HYP2000ARC>" ); 
		    logit( "e", "; exiting!\n" );
		    return EW_FAILURE;
		}
		nLogo++;
		init[5] = 1;
	    }

	    /*6*/   else if (k_its("DelayTime") ) {
		DelayTime = (time_t)k_int();
		init[6] = 1;
	    }
	    
	    /*7*/   else if (k_its("WaitTime") ) {
		WaitTime = (time_t)k_int();
		init[7] = 1;
	    }
	    
	    /* Optional Commands */
	    /*NR*/    else if( k_its("Debug") ) {
		Debug = 1;
	    }
	    
	    /*NR*/    else if( k_its("CheckLocCode") ) {
		CheckLocCode = 1;
	    }
	    
	    /*NR*/   else if (k_its("QueueSize") ) {
		QueueSize = k_int();
	    }
	    
	    /*NR*/   else if (k_its("QueueFile") ) {
		if ( (str = k_str() ) )
		    {
			if ( (QueueFile = strdup(str)) == NULL) {
			    logit( "e", "hypAssoc: out of memory for QueueFile name\n");
			    return EW_FAILURE;
			}
		    }
	    }

	    /* Unknown command */
	    else {
		logit( "e", "hypAssoc: <%s> Unknown command in <%s>.\n", 
		       com, configfile );
		continue;
	    }
	    
	    /* See if there were any errors processing the command  */
	    if( k_err() ) {
		logit( "e",
		       "hypAssoc: Bad <%s> command in <%s>; exiting!\n",
		       com, configfile );
		return EW_FAILURE;
	    }
	} /* while k_rd() */
	
	nfiles = k_close();
    } /* while nfiles > 0 */
    
    /* After all files are closed, check init flags for missed commands */
    nmiss = 0;
    for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
    if ( nmiss ) {
	logit( "e", "hypAssoc: ERROR, no " );
	if ( !init[0] )  logit( "e", "<LogFile> "       );
	if ( !init[1] )  logit( "e", "<MyModuleId> "    );
	if ( !init[2] )  logit( "e", "<InRing> "      );
	if ( !init[3] )  logit( "e", "<OutRing> "   );
	if ( !init[4] )  logit( "e", "<HeartBeatInt> "  );
	if ( !init[5] )  logit( "e", "<GetEventsFrom> " );
	if ( !init[6] )  logit( "e", "<DelayTime> "   );
	if ( !init[7] )  logit( "e", "<WaitTime> "   );
	logit( "e", "command(s) in <%s>; exiting!\n", configfile );
	return EW_FAILURE;
    }

    return EW_SUCCESS;
}

/************************************************************************
 *  hypAssoc_lookup( )   Look up important info from earthworm.h tables*
 ************************************************************************/
int hypAssoc_lookup( void )
{
    /* Look up keys to shared memory regions */
    if( ( InRingKey = GetKey(InRing) ) == -1 ) {
	fprintf( stderr,
		 "hypAssoc:  Invalid ring name <%s>; exiting!\n", InRing);
	return EW_FAILURE;
    }

    /* Look up keys to shared memory regions */
    if( ( OutRingKey = GetKey(OutRing) ) == -1 ) {
	fprintf( stderr,
		 "hypAssoc:  Invalid ring name <%s>; exiting!\n", OutRing);
	return EW_FAILURE;
    }

    /* Look up installations of interest */
    if ( GetLocalInst( &InstId ) != 0 ) {
	fprintf( stderr, 
		 "hypAssoc: error getting local installation id; exiting!\n" );
	return EW_FAILURE;
    }

    /* Look up modules of interest */
    if ( GetModId( MyModName, &MyModId ) != 0 ) {
	fprintf( stderr, 
		 "hypAssoc: Invalid module name <%s>; exiting!\n", MyModName );
	return EW_FAILURE;
    }

    /* Look up message types of interest */
    if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
	fprintf( stderr, 
		 "hypAssoc: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
	return EW_FAILURE;
    }
    if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
	fprintf( stderr, 
		 "hypAssoc: Invalid message type <TYPE_ERROR>; exiting!\n" );
	return EW_FAILURE;
    }
    if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
	fprintf( stderr, 
		 "hypAssoc: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
	return EW_FAILURE;
    }
    return EW_SUCCESS;
} 

/*************************************************************************
 * hypAssoc_status() builds a heartbeat or error message & puts it into *
 *                 shared memory.  Writes errors to log file & screen.   *
 *************************************************************************/
void hypAssoc_status( MSG_LOGO *pLogo, short ierr, char *note, long MyPid )
{
    char        msg[256];
    long        size;
    time_t      t;
 
    time( &t );

    if( pLogo->type == TypeHeartBeat ) {
	sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
    }
    else if( pLogo->type == TypeError ) {
	sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
	logit( "et", "hypAssoc: %s\n", note );
    }
    
    size = strlen( msg );   /* don't include the null byte in the message */     

    /* Write the message to shared memory */
    if( tport_putmsg( &InRegion, pLogo, size, msg ) != PUT_OK ) {
	if( pLogo->type == TypeHeartBeat ) {
	    logit("et","hypAssoc:  Error sending heartbeat.\n" );
	}
	else if( pLogo->type == TypeError ) {
	    logit("et","hypAssoc:  Error sending error:%d.\n", ierr );
	}
    }
}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
    char        *msg;           /* message buffer, including receipt time  */
    char        *msg_p;         /* the actual trigger message    */
    int          res;
    long         recsize;       /* size of retrieved message     */
    MSG_LOGO     reclogo;       /* logo of retrieved message     */
    int          ret, rc, err;
    time_t       recvd;         /* time we received this message */

    /* Allocate space for input/output messages */
    if ( ( msg = (char *) malloc(DB_MAX_BYTES_PER_EQ) ) == (char *) NULL ) {
	logit( "e", "hypAssoc: error allocating msg; exiting!\n" );
	goto error;
    }

    /* In the queue, we put the timestamp followed by the trigger message */
    msg_p = msg + sizeof(time_t);
  
    errLogo.instid = InstId;
    errLogo.mod    = MyModId;
    errLogo.type   = TypeError;

    /* Tell the main thread we're ok */
    MessageStackerStatus=0;

    /* flush the input ring buffer */
    while( tport_getmsg( &InRegion, GetLogo, nLogo, &reclogo, &recsize, 
			 msg_p, DB_MAX_BYTES_PER_EQ-1) != GET_NONE )
	;

    /* Start service loop, picking up trigger messages */
    while( 1 ) {
	if (MessageStackerStatus > 0)
	    break;  /* We're terminated! */
	
	/* Get a message from transport ring */
	res = tport_getmsg( &InRegion, GetLogo, nLogo, &reclogo, 
			    &recsize, msg_p, DB_MAX_BYTES_PER_EQ-1 );
	if(Debug == 1) 
	    if(res==GET_OK)
		logit("et","Got message from transport of %ld bytes, res=%d\n",
		      recsize,res); 
	
	if( res == GET_NONE ) {
	    sleep_ew(1000); 
	    continue;
	} /*wait if no messages for us */
	
	/* Check return code; report errors */
	switch ( res ) {
	case GET_OK:
	    break;
	case GET_TOOBIG:
	    sprintf( Text, "msg[%ld] i%d m%d t%d too long for target",
		     recsize, (int) reclogo.instid,
		     (int) reclogo.mod, (int)reclogo.type );
	    hypAssoc_status( &errLogo, ERR_TOOBIG, Text, MyPid );
	    continue;
	case GET_MISS:
	    sprintf( Text, "missed msg(s) i%d m%d t%d in %s",
		     (int) reclogo.instid, (int) reclogo.mod, 
		     (int)reclogo.type, InRing );
	    hypAssoc_status( &errLogo, ERR_MISSMSG, Text, MyPid );
	    break;
	case GET_NOTRACK:
	    sprintf( Text, "no tracking for logo i%d m%d t%d in %s",
		     (int) reclogo.instid, (int) reclogo.mod, 
		     (int)reclogo.type, InRing );
	    hypAssoc_status( &errLogo, ERR_NOTRACK, Text, MyPid );
	    break;
	}

	recvd = time(0);
	memcpy(msg, (char *)&recvd, sizeof(time_t));    
	recsize += (long)sizeof(time_t);
    
	/* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK) */
	RequestMutex();
	rc = 0;
	ret = enqueue( &OutQueue, msg, recsize, reclogo );
	if (ret == 0 && QueueFile != NULL) {
	    rc = dumpqueue( &OutQueue, QueueFile);
	    err = errno;  /* save the error */
	}
	ReleaseMutex_ew();

	if(Debug == 1) 
	    logit("","Queued a message\n");
	switch ( ret ) {
	case 0:
	    break;
	case -2:   /* Serious: quit */
	    sprintf(Text,"internal queue error. Terminating.");
	    hypAssoc_status( &errLogo, ERR_QUEUE, Text, MyPid );
	    goto error;
	case -1:
	    sprintf(Text,"queue cannot allocate memory. Lost message.");
	    hypAssoc_status( &errLogo, ERR_QUEUE, Text, MyPid );
	    continue;
	case -3:
	    sprintf(Text,"Queue full. Message lost.");
	    hypAssoc_status( &errLogo, ERR_QUEUE, Text, MyPid );
	    continue;
	}

	if (rc < 0) {
	    sprintf(Text,"Error writing queue file: %s.", strerror(errno));
	    hypAssoc_status( &errLogo, ERR_FILE, Text, MyPid );
	}
	else if (Debug && QueueFile != NULL) {
	    logit("t", "MessageStacker: dumped %d queue messages to %s\n", 
		  getNumOfElementsInQueue( &OutQueue), QueueFile);
	}
    
	if(Debug == 1) 
	    logit("et", "stacker thread: queued msg len:%ld\n",recsize);
    }

    /* we're quitting */
 error:
    MessageStackerStatus = -1; /* file a complaint to the main thread */
    free(msg);
  
    KillSelfThread(); 
    return(NULL);
}

/******************** Message Processing Thread *******************
 *           Take messages from memory queue, process             *
 ******************************************************************/

thr_ret EventAssociator( void *dummy )
{
    static char  trgMsg[DB_MAX_TRIG_BYTES + sizeof(time_t)]; 
    char        *msg_p;          /* pointer to the actual trigger message */
    char	*in;             /* working pointer to archive message    */
    char	 line[MAX_STR];  /* to store lines from msg               */
    char	 shdw[MAX_STR];  /* to store shadow cards from msg        */
    int		 nline;          /* number of lines (not shadows) so far  */
    struct	 Hsum Sum;       /* Hyp2000 summary data                   */
    struct	 Hpck Pick;      /* Hyp2000 pick structure                 */
    int          i;
    long         msgSize;        /* size of retrieved message             */
    int          ret, rc;
    MSG_LOGO     reclogo;        /* logo of retrieved message             */
    EVENT	*new_event;
    double	 time_diff;


    msg_p = trgMsg + sizeof(time_t);

    /* Tell the main thread we're ok */
    EventAssociatorStatus = 0;
    while (1) {
	if (EventAssociatorStatus > 0)
	    break;
	
	/* Get message from queue */
	rc = 0;
	RequestMutex();
	ret = dequeue( &OutQueue, trgMsg, &msgSize, &reclogo);
	if (ret == 0 && QueueFile != NULL)
	    rc = dumpqueue(&OutQueue, QueueFile);
	ReleaseMutex_ew();

	/* -1 means empty queue */
	if (ret < 0 ) { 
	    sleep_ew(500); /* wait a bit then try again */
	    continue;
	}

	if (rc < 0) {
	    sprintf(Text,"Error writing queue file: %s.", strerror(errno));
	    hypAssoc_status( &errLogo, ERR_FILE, Text, MyPid );
	}
	else if (Debug && QueueFile != NULL) {
	    logit("t", "EventAssociator: dumped %d queue messages to %s\n",
		  getNumOfElementsInQueue( &OutQueue), QueueFile);
	}
	
	if (reclogo.type != TypeHyp2000Arc) {
	    logit("","illegal (non-hyp2000arc) message in queue; exiting!\n");
	    EventAssociatorStatus = -1;
	    KillSelfThread();
	}
	nline  = 0;
	trgMsg[msgSize + sizeof(time_t)] = '\0';   /*null terminate the message*/
	msgSize = strlen( msg_p );
	in     = msg_p;
	new_event = NULL;

	
	logit ("", "*** processing new hyp2000arc message (%d bytes) ***\n",
	       msgSize);
	logit ("", "%s\n", msg_p);
	
	/* begin loop over lines in message     */
	
	if(Debug == 1) 
	    logit("","Parsing hyp2000arc message\n");

	/* Read one data line and its shadow at a time from arcmsg; process them
	***********************************************************************/
	while( in < msg_p+msgSize )
	{
		if ( sscanf( in, "%[^\n]", line ) != 1 )
		{
			logit( "et", "HypoArc Translation : zerolength line message\n");
			goto EndMessage;
		}
		in += strlen( line ) + 1;
		if ( sscanf( in, "%[^\n]", shdw ) != 1 )
		{
			logit( "et", "HypoArc Translation : zerolength line shadow\n");
			goto EndMessage;
		}
		in += strlen( shdw ) + 1;
		nline++;
		logit( "e", "%s\n", line );  /*DEBUG*/
		logit( "e", "%s\n", shdw );  /*DEBUG*/

	/* Process the hypocenter card (1st line of msg) & its shadow
	************************************************************/
		if( nline == 1 )                /* hypocenter is 1st line in msg  */
		{
			read_hyp( line, shdw, &Sum );
			if (Sum.version == 0)
				break;
			for(i=0; i<maxEvent ;i++) {
				time_diff = Event[i].Origin.ot - Sum.ot - Sum.dmin / 5;
				if(ABS(time_diff) < DelayTime) {
					new_event = &Event[i];
					UpdateHypo(new_event,Sum);
					break;
				}
			}
			if (new_event == NULL)
					new_event = NewEvent(Event,Sum);
			continue;
		}

	/* Process all the phase cards & their shadows
	*********************************************/
		if( strlen(line) < (size_t) 75 )	/* found the terminator line      */
			break;
		read_phs( line, shdw, &Pick );		/* load info into Pick structure   */
		UpdateEvent(new_event,Pick);
	} /*end while over reading message*/

	if (EventAssociatorStatus > 0)
	    goto ShutdownEventAssociator;
 
 EndMessage:
	logit ("", "*** Done processing message (%d bytes) ***\n", msgSize);
	sleep_ew(500);
    }  /* while(1) loop forever; the main thread will kill us */
  
 ShutdownEventAssociator:
    logit("e", "hypAssoc:  Self-Terminating EventAssociator thread!\n");
    EventAssociatorStatus = -1;
    KillSelfThread();
    return(NULL);

}  /* end EventAssociator Thread */

/******************** Message Processing Thread *******************
 *           Take messages from memory queue,                     *
 *             output message if timeout has been reached         *
 ******************************************************************/

thr_ret EventOutput( void *dummy )
{
    int          i;
    time_t       now;            /* Current time, according to the clock */

    /* Tell the main thread we're ok */
    EventOutputStatus = 0;
    while (1) {
	if (EventOutputStatus > 0)
		goto ShutdownEventOutput;

	now=time(0);
	now -= WaitTime;
	for (i=0; i< maxEvent ; i++) {
		if ( (now > Event[i].TimeCreated) && (Event[i].TimeCreated != 0) )
			OutputEvent(&Event[i]);
	}
	sleep_ew(10000);
    }  /* while(1) loop forever; the main thread will kill us */
  
 ShutdownEventOutput:
    logit("e", "hypAssoc:  Self-Terminating EventOutput thread!\n");
    EventOutputStatus = -1;
    KillSelfThread();
    return(NULL);
}  /* end EventOutput Thread */

/****************************************************************** 
 *  Look into the Snippet, and return strings which may be        *
 ******************************************************************/
EVENT *NewEvent (EVENT *EventStack, struct Hsum NewEventSum)
{
    int		i;
    EVENT	*new_event;
    time_t	now;

    if (EventStack == NULL) {
	logit ("e", "Invalid argument passed in; exiting!\n");
    }

    for (i=0; i < maxEvent; i++) {
	if (EventStack[i].Origin.ot ==0) {
		now=time(0);
		EventStack[i].Origin = NewEventSum;
		EventStack[i].TimeCreated = now;
		new_event=&EventStack[i];
		logit ("e", "NewEvent : %d\n",i);
		break;
	}
    }

    return (new_event);	
}

/****************************************************************** 
 *  Updates phases informations if necessary (new ones)           *
 ******************************************************************/
int UpdateEvent (EVENT *ExistingEvent, struct Hpck NewPhase)
{
    int	i,test;

    for (i=0; i < ExistingEvent->NSta; i++) {
	test = PhaseComp(NewPhase,ExistingEvent->Phases[i]);
	if( test == 0)
		return(0);
	else if ( test == -1 ) {
		ExistingEvent->Phases[i] = NewPhase;
		return(0);
	}
    }
    ExistingEvent->Phases[ExistingEvent->NSta] = NewPhase;
    ExistingEvent->NSta += 1;
    return(1);
}

/****************************************************************** 
 *  Updates Hypocenter informations if necessary                  *
 ******************************************************************/
void UpdateHypo (EVENT *ExistingEvent, struct Hsum NewHypo)
{
    char  oldhypo[200];
    char  newhypo[200];
    char *strptr;

    if (NewHypo.nph > ExistingEvent->Origin.nph) {
	strptr = newhypo;
	write_hyp_(NewHypo, &strptr);
	strptr = oldhypo;
	write_hyp_(ExistingEvent->Origin, &strptr);
	ExistingEvent->Origin = NewHypo;
    }
}

/******************************************************************** 
 *  Compare two Hpck structure to determine if it is the same phase *
 *  Returns 0 if it already exists, -1 if it needs to be updated    *
 *  positive value if it is a new phase                             *
 ********************************************************************/
int PhaseComp (struct Hpck PhaseTest, struct Hpck PhaseRef)
{
    int	sta,net,loc,P,S;

    sta = ABS(strcmp(PhaseTest.site,PhaseRef.site));	/* Compare station code */
    net = ABS(strcmp(PhaseTest.net,PhaseRef.net));	/* Compare network code */
    if (CheckLocCode)
	loc = ABS(strcmp(PhaseTest.loc,PhaseRef.loc));	/* Compare location code */
    else
	loc = 0;
    P = ABS(PhaseTest.Plabel - PhaseRef.Plabel);	/* Compare P Phase code */
    S = ABS(PhaseTest.Slabel - PhaseRef.Slabel);	/* Compare S Phase code */
    if ((sta + net + loc + P + S) == 0 ) {		/* This is the same phase, check arrival times */
	if ((PhaseTest.Plabel == 'P') && PhaseTest.Pat != 0 ) {
		if (PhaseTest.Pat < PhaseRef.Pat) {
			return(-1);			/* We choose the first P Phase */
		}
	} else if ((PhaseTest.Slabel == 'S') && PhaseTest.Sat != 0 ) {
		if (PhaseTest.Sat < PhaseRef.Sat) {
			return(-1);			/* We choose the first S Phase */
  		}
	} else if ( PhaseTest.Plabel == ' ' )
		return(0);
	else if ( PhaseTest.Slabel == ' ' )
		return(0);
	}
    return(sta + net + loc + P + S);
}

/****************************************************************** 
 *  Create output HYP2000_ARC message and send it                 *
 ******************************************************************/
int OutputEvent (EVENT *OutEvent)
{
    int		 i;
    char        *outmsg;           /* message buffer, including receipt time  */
    char        *outmsg_p;         /* the actual trigger message */
    long         putsize;          /* size of sent message */
    long         linesize;         /* size of line */

    putsize = 0;
    /* Allocate space for input/output messages */
    if ( ( outmsg = (char *) malloc(DB_MAX_BYTES_PER_EQ) ) == (char *) NULL ) {
	logit( "e", "hypAssoc: error allocating outmsg; exiting!\n" );
	return(0);
    }
    outmsg_p = outmsg;

    linesize = write_hyp_(OutEvent->Origin, &outmsg_p);
    putsize += linesize;
    if (Debug == 1)
	logit("e","%s",outmsg_p - linesize);

    for (i=0; i< OutEvent->NSta; i++) {
	linesize = write_phs_(OutEvent->Phases[i], &outmsg_p);
	putsize += linesize;
	if (Debug == 1)
		logit("e","%s",outmsg_p - linesize);
    }
    linesize = write_term_(OutEvent->Origin, &outmsg_p);
    putsize += linesize;

    if( tport_putmsg( &OutRegion, &putlogo, putsize, outmsg ) != PUT_OK ) {
	logit("et","hypAssoc: Error writing %d-byte msg to ring(i%u m%u t%u)\n",
		putsize, putlogo.instid, putlogo.mod, putlogo.type );
    }
    if (Debug == 1)
	logit ("o", "***Output Event***\n%s",outmsg);

    OutEvent->TimeCreated = 0;
    OutEvent->Origin.ot = 0;
    OutEvent->NSta = 0;
    return(1);
}

/**************************************************************
 * write_hyp_() writes the hypocenter line from an Hsum structure
 *		(defined in read_arch.h) 

 *      Inputs: outptr - pointer to the output buffer
 *              SumP - pointer to Hsum structure provided by the caller
 *
 ****************************************************************/

int write_hyp_( struct Hsum SumP, char **outptr)
{
    float  deg, min;
    char   sign;
    char   *outptr_begin;

/*------------------------------------------------------------------------
Sample HYP2000ARC archive summary line and its shadow.  The summary line
may be up to 188 characters long.  Its full format is described in
documentation (hyp2000.shadow.doc) by Fred Klein.
199204290117039536 2577120 2407  475  0 18 98 17  16 5975 128175 6  58343COA  38    0  57 124 21   0 218  0  8COA WW D 24X   0  0L  0  0     10123D343 218Z  0   0  \n
YYYYMMDDHHMMSSssLLsLLLLlllsllllDDDDDMMMpppGGGnnnRRRRAAADDEEEEAAADDMMMMCCCrrreeeerrssshhhhvvvvpppssssddddsssdddmmmapdacpppsaeeewwwmaaawwwIIIIIIIIIIlmmmwwwwammmwwwwVvN
$1                                                                                0343   0   0\n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 12345
0         1         2         3         4         5         6         7         8         9        10        11        12        13        14        15        16
------------------------------------------------------------------------*/
    outptr_begin = *outptr;

    sprintf(*outptr,"%14.14s%2.2s",SumP.cdate,SumP.cdate+15);
    *outptr = *outptr + 16;

    deg = floor( fabs(SumP.lat) );
    min = ( fabs(SumP.lat) - deg ) * 60 * 100;
    if (SumP.lat < 0)
	sign = 'S';
    else
	sign = ' ';
    sprintf(*outptr,"%-2.0f%c%4.0f",deg,sign,min);
    *outptr = *outptr + 7;

    deg = floor( fabs(SumP.lon) );
    min = ( fabs(SumP.lon) - deg ) * 60 * 100;
    if (SumP.lon > 0)
	sign = 'E';
    else
	sign = ' ';
    sprintf(*outptr,"%3.0f%c%4.0f",deg,sign,min);
    *outptr = *outptr + 8;

    sprintf(*outptr,"%5.0f",SumP.z*100);
    *outptr = *outptr + 5;
    if ((SumP.labelpref == 'L') || (SumP.labelpref == 'X'))	/* If preferred Magnitude is Amplitude, update correct field */
	sprintf(*outptr,"%3.0f%3d%3d%3d%4.0f",SumP.Mpref*100,SumP.nph,SumP.gap,SumP.dmin,SumP.rms*100);
    else
	sprintf(*outptr,"   %3d%3d%3d%4.0f",SumP.nph,SumP.gap,SumP.dmin,SumP.rms*100);
    *outptr = *outptr + 16;

    sprintf(*outptr,"%3d%2d%4.0f%3d%2d%4.0f",SumP.e0az,SumP.e0dp,SumP.e0*100,SumP.e1az,SumP.e1dp,SumP.e1*100);
    *outptr = *outptr + 18;

    sprintf(*outptr,"%3.0f%3s%4.0f  %3d%4.0f%4.0f",SumP.Md*100,SumP.reg,SumP.e2*100,SumP.nphS,SumP.erh*100,SumP.erz*100);
    *outptr = *outptr + 23;

    if( SumP.mdtype == '\0')
	SumP.mdtype = ' ';
    sprintf(*outptr,"%3d    %4.0f   %3.0f       %c",SumP.nPfm,SumP.mdwt*10,SumP.mdmad*100,SumP.mdtype);
    *outptr = *outptr + 25;

    sprintf(*outptr,"%3d               %10ld",SumP.nphtot,SumP.qid);
    *outptr = *outptr + 28;

    sprintf(*outptr,"%c%3.0f%4.0f        %1ld \n$1\n",SumP.labelpref,SumP.Mpref*100,SumP.wtpref*10,SumP.version);
    *outptr = *outptr + 22;

    strcpy(*outptr,"\0");
    return ( strlen(outptr_begin) );
}

/**************************************************************
 * write_phs_() writes the phase line from an Hpck structure
 *		(defined in read_arch.h) 

 *      Inputs: outptr - pointer to the output buffer
 *              PckP - pointer to Hpck structure provided by the caller
 *
 ****************************************************************/

int write_phs_( struct Hpck PckP, char **outptr)
{
    char   *outptr_begin;
    float   Psec, Ssec;
    double  Cat; /* common arrival time up to yyyymmddhhmm */
   
/*------------------------------------------------------------------------
Sample TYPE_HYP2000ARC station archive card (P-arrival) and its shadow. 
  Phase card is 116 chars (including newline); 
  Shadow is up to 96 chars (including newline).
PWM  NC VVHZ  PD0199204290117  877  -8136    0   0   0      0 0  0  61   0 169 8400  0   77 88325  0 932   0WD 01  \n
SSSSSNNxcCCCxrrFWYYYYMMDDHHMMPPPPPRRRRWWWSSSSSrrxWRRRRAAAAAAAuuWWWDDDDddddEEEEAAAwwppprDDDDAAAMMMmmmppppssssSDALLaa\
$   6 5.49 1.80 7.91 3.30 0.10 PSN0   77 PHP3 1853 39 340 47 245 55 230 63  86 71  70 77  48   \n
$xnnnAA.AAQQ.QQaa.aaqq.qqRR.RRxccccDDDDDxPHpwAAAAATTTAAAAtttaaaaTTTAAAAtttaaaaTTTAAAAtttaaaaMMM\
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456
--------------------------------------------------------------------------*/
    outptr_begin = *outptr;

    sprintf(*outptr,"%-5.5s%2.2s  %3.3s ",PckP.site,PckP.net,PckP.comp);
    *outptr = *outptr + 13;

    sprintf(*outptr,"%c%c%c%1d",PckP.Ponset,PckP.Plabel,PckP.Pfm,PckP.Pqual);
    *outptr = *outptr + 4;

    Cat = julsec17( PckP.cdate );
    Psec = PckP.Pat - Cat;
    sprintf(*outptr,"%12.12s%5.0f%4.0f%3.0f",PckP.cdate,Psec*100,PckP.Pres*100,PckP.Pwt*100);
    *outptr = *outptr + 24;

    Ssec = PckP.Sat - Cat;
    sprintf(*outptr,"%5.0f%c%c%c%1d%4.0f         %3.0f",
		Ssec*100,PckP.Sonset,PckP.Slabel,PckP.Sfm,PckP.Squal,PckP.Sres*100,PckP.Swt*100);
    *outptr = *outptr + 25;

    sprintf(*outptr,"        %4.0f%3d %1d    ",PckP.dist*10,PckP.takeoff,PckP.codawt);
    *outptr = *outptr + 21;

    sprintf(*outptr,"%4d%3d%3.0f           %c  %2.2s\n",PckP.codalen,PckP.azm,PckP.Md*100,PckP.datasrc,PckP.loc);
    *outptr = *outptr + 27;

    sprintf(*outptr,"$\n");
    *outptr = *outptr + 2;

    strcpy(*outptr,"\0");
    return ( strlen(outptr_begin) );
}

/**************************************************************
 * write_term_() writes the terminator line from an Hsum structure
 *		(defined in read_arch.h) 

 *      Inputs: outptr - pointer to the output buffer
 *              SumP - pointer to Hsum structure provided by the caller
 *
 ****************************************************************/

int write_term_( struct Hsum SumP, char **outptr)
{
    char   *outptr_begin;
    int     i;
   
/*------------------------------------------------------------------------
Sample TYPE_HYP2000ARC station terminal card and its shadow. 
  Terminal card is 73 chars (including newline); 
  Shadow is 73 chars (including newline).
      1853422114S4758 61E5236   20                                 12368\n
      HHMMSSSSLLsLLLLlllsllllDDDDDX                           IIIIIIIIII\
$     1853422114S4758 61E5236   20                                 12368\n
$     HHMMSSSSLLsLLLLlllsllllDDDDDX                           IIIIIIIIII\
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
--------------------------------------------------------------------------*/
    outptr_begin = *outptr;

    for (i=0; i<62; i++)
	sprintf(*outptr+i," ");
    *outptr = *outptr + i;
    sprintf(*outptr,"%10ld\n$",SumP.qid);
    *outptr = *outptr + 12;

    for (i=0; i<61; i++)
	sprintf(*outptr+i," ");
    *outptr = *outptr + i;
    sprintf(*outptr,"%10ld\n",SumP.qid);
    *outptr = *outptr + 11;

    strcpy(*outptr,"\0");
    return ( strlen(outptr_begin) );
}
