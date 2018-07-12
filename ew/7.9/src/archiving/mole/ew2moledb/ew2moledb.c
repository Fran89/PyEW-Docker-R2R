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

#include "ew2moledb_version.h"
#include "ew2moledb_tracebuf.h"
#include "ew2moledb_pickcoda.h"
#include "ew2moledb_quakelink.h"
#include "ew2moledb_arc.h"
#include "ew2moledb_magnitude.h"
#include "ew2moledb_strongmotionII.h"
#include "ew2moledb_sendmail.h"
#include "ew2moledb_mysql.h"
#include "ew2moledb_error.h"
#include "ew2moledb_heartbeat.h"
#include "ew2moledb_eb_picktwc.h"
#include "ew2moledb_eb_hypotwc.h"
#include "ew2moledb_eb_alarm.h"

#define FILENAME_MAXLEN 1024

/* Thread things
 ***************/
#define THREAD_STACK 8192

DECLARE_SPECIFIC_SEMAPHORE_EW(sema_CountAvail);
DECLARE_SPECIFIC_SEMAPHORE_EW(sema_CountBusy);
DECLARE_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);

 /* just to compile without warning from thread function returning */
#ifdef _WINNT
#define RETURN_THREAD_FUNCTION return
#else
#define RETURN_THREAD_FUNCTION return (thr_ret) NULL
#endif

thr_ret MessageStacker( void *p );    /* thread: messages from ring to queue */
thr_ret DBPopulate( void *p );        /* thread: messages from queue to database */
thr_ret QueueDumpFile( void *p );     /* thread: messages from queue to file */

void       inc_nStackerThread();
void       dec_nStackerThread();
mutex_t    mutex_nStackerThread;
int        nStackerRunning = 0;

mutex_t   mutex_OutQueue;            /* mutex for variable OutQueue */
QUEUE     OutQueue; 		     /* queue for saving messages read from ring */

#define   FLAG_TERM_NULL   0         /* flag value for null info */
#define   FLAG_TERM_REQ_DB 1         /* flag value for termination requested DB */
#define   FLAG_TERM_REQ_MS 2         /* flag value for termination requested Stackers */
#define   FLAG_TERM_REQ_QF 4         /* flag value for termination queue dump file */
#define   FLAG_TERM_THR_DB 8         /* flag value DB thread is terminated */
#define   FLAG_TERM_THR_MS 16        /* flag value MS thread is terminated */
#define   FLAG_TERM_THR_QF 32        /* flag value MS thread is terminated */
mutex_t   mutex_flags_term;          /* mutex for variable  flags_term */
int       flags_term = FLAG_TERM_NULL;          /* flags variable for handling termination */
void      setlck_flags_term(int flags_bitmap);  /* set flags_bitmap by lock mechanism */
int       testlck_flags_term(int flags_bitmap); /* test flags_bitmap by lock mechanism */

#define MAX_NUM_OF_RINGS 30
int remove_duplicated_ringname(char ringNames[MAX_NUM_OF_RINGS][MAX_RING_STR], int *nRings);

struct TYPE_PARAMS;

typedef struct {
  char      InRing[MAX_RING_STR];
  MSG_LOGO  *GetLogo;         /* array for requesting module,type,instid  */
  short     nLogo;            /* number of logos to get                   */
  struct TYPE_PARAMS *params;
} TYPE_PARAMS_STACKER;

typedef struct TYPE_PARAMS {
  char      *cmdname;          /* pointer to executable name argv[0] */ 

/* Variables set from ew2moledb_config() before starting threads
 **********************************************************************/
  int       nRings;
  char      InRings[MAX_NUM_OF_RINGS][MAX_RING_STR]; /* names of transport rings for input  */
  TYPE_PARAMS_STACKER pstacker[MAX_NUM_OF_RINGS];

  MSG_LOGO  *GetLogo;         /* array for requesting module,type,instid  */
  short     nLogo;            /* number of logos to get                   */

  char      MyModName[MAX_MOD_STR];  /* speak as this module name/id      */
  int       LogSwitch;        /* 0 if no logfile should be written        */
  long      MaxMsgSize;       /* max size for input/output msgs           */
  int       QueueSize;	    /* max messages in output circular buffer   */
  char      QueueFilename[FILENAME_MAXLEN];         /* Filename for dumping and undumping the queue */
  int       WaitSecAfterDBError; /* Milliseconds to wait after a DB error */
  char      db_hostname[256]; /* DB hostname */
  char      db_username[256]; /* DB username */
  char      db_password[256]; /* DB user password */
  char      db_name[256];     /* DB name */
  long      db_port;          /* DB port */
  char      ewinstancename[256];   /* EW instance name */

  /* Variables set from ew2moledb_lookup() before starting threads
   **********************************************************************/
  int        HeartBeatInt;     /* seconds between heartbeat messages       */
  char       HeartBeatRing[MAX_RING_STR];    /* name of transport ring for input  */
  SHM_INFO   HeartBeatRegion;         /* shared memory region to use for input    */
  long       HeartBeatRingKey;       /* key of transport ring for input    */
  MSG_LOGO   HeartLogo;       /* logo of heartbeat message          */
  MSG_LOGO   ErrorLogo;       /* logo of error message              */

} TYPE_PARAMS;

void ew2moledb_config( char *configfile, TYPE_PARAMS *params );
void ew2moledb_lookup( TYPE_PARAMS *params );
void ew2moledb_status( MSG_LOGO *logo, char *msg, TYPE_PARAMS *params);
void ew2moledb_mysql_status(int ret_ex_mysql_query, TYPE_PARAMS *params);

/* Error messages used by ew2moledb 
***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE         3   /* queue error                             */

/* maximum length for error message */
#define STR_LEN_ERRTEXT 2048

int main( int argc, char **argv ) {
    time_t       now;	                 /* current time, used for timing heartbeats */
    pid_t        MyPid;		         /* Our own pid, sent with heartbeat for restart purposes */
    time_t       MyLastBeat;             /* time of last local (into Earthworm) hearbeat */
    unsigned     tidStacker[MAX_NUM_OF_RINGS];   /* Thread moving messages from transport to queue */
    unsigned     tidDBPopulate;          /* Thread read message from queue and populate DB */
    unsigned     tidQueueDumpFile;       /* Thread read message from queue and populate DB */
    int          ret_dq;                 /* stores errors from dumpqueue() and undumpqueue() */
    int          cur_NumOfElements = 0;  /* Current value of number of elements in OutQueue */
    const int    wait_time_thr_tot   = 5000;  /* total of waiting time for joining all threads */
    const int    wait_time_thr_slice = wait_time_thr_tot / 10;   /* slice of waiting time for joining all threads */
    int          wait_time_thr_count = 0;     /* counter of waiting time for joining all threads */
    char         errText[STR_LEN_ERRTEXT];  /* string for log/error/heartbeat messages           */
    char         *configfile;       /* pointer to configuration file name argv[1] */ 
    MYSQL        *test_mysql_connection = NULL;

    int          r = 0;
    TYPE_PARAMS  params;

    /* set last character to zero */
    errText[STR_LEN_ERRTEXT-1]=0;

    /* Check command line arguments 
     ******************************/
    params.cmdname = argv[0];
    configfile = argv[1];
    if ( argc != 2 )
    {
	fprintf( stderr, "Usage: %s <configfile>\n", params.cmdname );
	fprintf( stderr, "Version %s\n", EW2MOLEDB_VERSION);
	return( 0 );
    }

    /* Initialize name of log-file & open it 
     ****************************************/
    logit_init( configfile, 0, 4096, 1 );

    /* Create Mutexes
     ********************************************/
    CreateSpecificMutex(&mutex_nStackerThread);
    CreateSpecificMutex(&mutex_OutQueue);
    CreateSpecificMutex(&mutex_flags_term);

    /* Read the configuration file(s)
     ********************************/
    ew2moledb_config( configfile, &params );
    logit("et", "ew2moledb: version %s\n", EW2MOLEDB_VERSION);
    logit("et" , "%s(%s): Read command file <%s>\n", 
           params.cmdname, params.MyModName, configfile );

    /* Remove duplicated ring names (Call function after logit_init()) */
    remove_duplicated_ringname(params.InRings, &(params.nRings));

    /* Init params_stacker array */
    for(r=0; r < params.nRings; r++) {
      strncpy(params.pstacker[r].InRing, params.InRings[r], MAX_RING_STR-1);
      params.pstacker[r].GetLogo = params.GetLogo;
      params.pstacker[r].nLogo = params.nLogo;
      params.pstacker[r].params = &params;
    }

    /* Look up important info from earthworm.h tables
     *************************************************/
    ew2moledb_lookup(&params);

    /* Initialize the message queue
     *******************************/
    RequestSpecificMutex(&mutex_OutQueue);
    initqueue( &OutQueue, (unsigned long)params.QueueSize,(unsigned long)(params.MaxMsgSize+1));
    ReleaseSpecificMutex(&mutex_OutQueue);

    /* Reinitialize the logging level
     *********************************/
    logit_init( configfile, 0, 4096, params.LogSwitch );

    /* Test MySQL connection before continuing
     *****************************************/
    if( ew2moledb_mysql_connect_p(&test_mysql_connection,
		    params.db_hostname, params.db_username, params.db_password, params.db_name, params.db_port) != NULL) {
	/* close MySQL connection */
	ew2moledb_mysql_close_p(&test_mysql_connection);
    } else {
	/* notify error status. exit */
	logit("e", "%s(%s): Error initializing connection to the database; exiting!\n", params.cmdname, params.MyModName);
	exit( -1 );
    }

    /* Get our own Pid for restart purposes
     ***************************************/
    MyPid = getpid();
    if(MyPid == -1)
    {
	logit("e", "%s(%s): Cannot get pid; exiting!\n", params.cmdname, params.MyModName);
	return(0);
    }

    /* Attach to shared memory for tport_getflag() and putting
     * heartbeat or status messages
     ********************************************/
    tport_attach( &params.HeartBeatRegion, params.HeartBeatRingKey );

    /* One heartbeat to announce ourselves to statmgr
     ************************************************/
    time(&MyLastBeat);
    snprintf( errText, STR_LEN_ERRTEXT-1, "%ld %ld\n%c", (long) MyLastBeat, (long) MyPid, 0);
    ew2moledb_status( &params.HeartLogo, errText, &params );

    /* Undump queue messages from file
     *******************************/
    RequestSpecificMutex(&mutex_OutQueue);
    ret_dq=undumpqueue( &OutQueue, params.QueueFilename);
    cur_NumOfElements = OutQueue.NumOfElements;
    ReleaseSpecificMutex(&mutex_OutQueue);

    /* Create Semaphores
     ********************************************/
    CREATE_SPECIFIC_SEMAPHORE_EW(sema_CountAvail,     params.QueueSize - cur_NumOfElements);
    CREATE_SPECIFIC_SEMAPHORE_EW(sema_CountBusy,      cur_NumOfElements );
    CREATE_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess, cur_NumOfElements);

    switch (ret_dq) {
	case 1:
	    logit("et", "dump file '%s' does not exist.\n", params.QueueFilename);
	    break;
	case 0:
	    logit("et", "read %d messages from queue dump file '%s'.\n", cur_NumOfElements, params.QueueFilename);
	    break;
	case -1:
	    logit("et", "errors opening or reading dump file '%s'.\n", params.QueueFilename);
	    break;
	case -2:
	    logit("et", "there are more entries in the dump file '%s' than will fit in queue. Read %d messages.\n", params.QueueFilename, cur_NumOfElements);
	    break;
	case -3:
	    logit("et", "dump file entry in '%s' is larger than will fit in queue.\n", params.QueueFilename);
	    break;
	case -4:
	    logit("et", "timestamps at each end of file '%s' don't match.\n", params.QueueFilename);
	    break;
	default:
	    logit("et", "Unknown error %d from undumpqueue() reading file '%s'.\n", ret_dq, params.QueueFilename);
	    break;
    }

    /* Start the DB populating thread
     ***********************************/
    if ( StartThreadWithArg(  DBPopulate, (void *) &params, (unsigned)THREAD_STACK, &tidDBPopulate ) == -1 )
    {
	logit( "e", "%s(%s): Error starting thread DBPopulate(); exiting!\n",
	       params.cmdname, params.MyModName );
	tport_detach( &params.HeartBeatRegion );
	exit( -1 );
    }

    /* Start the message stacking threads
     ****************************************************************/
    for(r=0; r < params.nRings; r++) {
      inc_nStackerThread();
    }
    for(r=0; r < params.nRings; r++) {
      if ( StartThreadWithArg(  MessageStacker, (void *) &(params.pstacker[r]),  (unsigned)THREAD_STACK, &(tidStacker[r]) ) == -1 )
      {
	logit( "e", 
	    "%s(%s): Error starting thread MessageStacker() on ring %s; exiting!\n",
	    params.cmdname, params.MyModName, params.pstacker[r].InRing );
	tport_detach( &params.HeartBeatRegion );
	exit( -1 );
      }
    }

    /* Start main ew2moledb service loop, which aimlessly beats its heart.
     **********************************/
    while( tport_getflag( &params.HeartBeatRegion ) != TERMINATE  &&
	   tport_getflag( &params.HeartBeatRegion ) != MyPid         )
    {
	/* Beat the heart into the transport ring
         ****************************************/
	time(&now);
	if (difftime(now,MyLastBeat) > (double)params.HeartBeatInt ) 
	{
	    snprintf( errText, STR_LEN_ERRTEXT-1, "%ld %ld\n%c", (long) now, (long) MyPid,0);
	    ew2moledb_status( &params.HeartLogo, errText, &params );
	    MyLastBeat = now;
	}

	/* take a brief nap
         ************************************/
	sleep_ew(100);
    } /*end while of monitoring loop */

    logit("t", "%s(%s): termination requested; waiting for all threads (max. %d sec.)!\n", 
          params.cmdname, params.MyModName, 3 * (wait_time_thr_tot/1000) );

    /* Ask to terminate to all Stacker threads */
    setlck_flags_term(FLAG_TERM_REQ_MS);
    wait_time_thr_count=0;
    while( wait_time_thr_count < wait_time_thr_tot && !testlck_flags_term(FLAG_TERM_THR_MS) ) {
	sleep_ew(wait_time_thr_slice);
	wait_time_thr_count+=wait_time_thr_slice;
    }
    
    /* The queue could be still full.
     * (i.e. DBPopulate() could not be able to  process messages in the queue. ) */
    if(!testlck_flags_term(FLAG_TERM_THR_MS)) {
	logit("et", "Warning: threads MessageStacker() not all terminated yet!\n");
    }

    /* Ask to terminate to DBPopulate thread */
    setlck_flags_term(FLAG_TERM_REQ_DB);
    wait_time_thr_count=0;
    while( wait_time_thr_count < wait_time_thr_tot && !testlck_flags_term(FLAG_TERM_THR_DB) ) {
	sleep_ew(wait_time_thr_slice);
	wait_time_thr_count+=wait_time_thr_slice;
    }

    /* The queue could be still empty. */
    if(!testlck_flags_term(FLAG_TERM_THR_DB)) {
	logit("et", "Warning: thread DBPopulate() not terminated yet!\n");
	/* One for DBPopulate which must quit since FLAG_TERM_REQ_DB */
	POST_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);
    }

    /* Start the Queue Dump File thread
     ***********************************/
    if ( StartThreadWithArg(  QueueDumpFile, (void *) &params, (unsigned)THREAD_STACK, &tidQueueDumpFile ) == -1 )
    {
	logit( "e", "%s(%s): Error starting thread QueueDumpFile(); exiting!\n",
	       params.cmdname, params.MyModName );
	tport_detach( &params.HeartBeatRegion );
	exit( -1 );
    }

    wait_time_thr_count=0;
    while( wait_time_thr_count < wait_time_thr_tot
	    && (   !testlck_flags_term(FLAG_TERM_THR_MS)
		|| !testlck_flags_term(FLAG_TERM_THR_DB)
		|| !testlck_flags_term(FLAG_TERM_THR_QF)
	       )
	 ) {
	if(!testlck_flags_term(FLAG_TERM_THR_QF)) {
	    /* One for QueueDumpFile which must quit since FLAG_TERM_REQ_DB */
	    POST_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);
	}
	sleep_ew(wait_time_thr_slice);
	wait_time_thr_count+=wait_time_thr_slice;
    }

    /* Still problem ??? */
    if(!testlck_flags_term(FLAG_TERM_THR_MS)) {
	logit("et", "Warning: threads MessageStacker() not all terminated yet!\n");
    }
    if(!testlck_flags_term(FLAG_TERM_THR_DB)) {
	logit("et", "Warning: thread DBPopulate() not terminated yet!\n");
    }
    if(!testlck_flags_term(FLAG_TERM_THR_QF)) {
	logit("et", "Warning: thread QueueDumpFile() not terminated yet!\n");
    }

    /* Detach from Input/Output shared memory ring 
     ********************************************/
    tport_detach( &params.HeartBeatRegion );

    /* Destroy Semaphores
     ********************************************/
    DESTROY_SPECIFIC_SEMAPHORE_EW(sema_CountAvail);
    DESTROY_SPECIFIC_SEMAPHORE_EW(sema_CountBusy);
    DESTROY_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);

    /* Shut it down
     ***************/
    logit("t", "%s(%s): termination requested; exiting!\n", 
          params.cmdname, params.MyModName );

    /* Deallocate memory from OutQueue */
    RequestSpecificMutex(&mutex_OutQueue);
    freequeue(&OutQueue);
    ReleaseSpecificMutex(&mutex_OutQueue);

    /* Destroy Mutexes
     ********************************************/
    CloseSpecificMutex(&mutex_nStackerThread);
    CloseSpecificMutex(&mutex_OutQueue);
    CloseSpecificMutex(&mutex_flags_term);

    logit("t", "%s(%s): exit!\n", 
          params.cmdname, params.MyModName );

    exit( 0 );	
}
/* *******************  end of main *******************************
******************************************************************/

#define MAX_SUB_STRPOS 4096
char *sub_strpos(char *iter_s[], char sep) {
    static char ret[MAX_SUB_STRPOS];
    char *p;
    int l;
    int i;

    ret[0] = 0;

    if(iter_s) {
	p = *iter_s;
	l = strlen(p);
	i = 0;

	if(p) {

	    /* Build ret */
	    while(i < l  &&  *p != sep  &&  *p != 0  &&  i < MAX_SUB_STRPOS-1) {
		ret[i] = *p;
		i++;
		p++;
	    }
	    if(*p == sep) {
		ret[i] = sep;
		i++;
		p++;
	    }
	    ret[i] = 0;

	    /* Check */
	    if(i >= MAX_SUB_STRPOS) {
		logit("e", "sub_strpos() returned value has not beeb completed, its length execeds %d bytes\n", MAX_SUB_STRPOS);
	    }

	    /* Set new value for *iter_s */
	    if(*p == 0  ||  i >= l) {
		*iter_s = NULL;
	    } else {
		*iter_s = p;
	    }

	}
    }

    return ret;
}


/* set flags_bitmap by lock mechanism 
 ***************************************************/
void       setlck_flags_term(int flags_bitmap) {
    RequestSpecificMutex(&mutex_flags_term);
    flags_term = flags_term | flags_bitmap;
    ReleaseSpecificMutex(&mutex_flags_term);
}


/* test flags_bitmap by lock mechanism
 ***************************************************/
int       testlck_flags_term(int flags_bitmap) {
    int ret = 0;
    RequestSpecificMutex(&mutex_flags_term);
    if(flags_bitmap == (flags_term & flags_bitmap)) {
	ret = 1;
    }
    ReleaseSpecificMutex(&mutex_flags_term);
    return ret;
}


/************* Main Thread for populating DB from queue  ***************
 *          Pull a messsage from the queue, and call stored procedure  *
 *          for inserting information in to the database               *
 **********************************************************************/
thr_ret DBPopulate( void *p )
{
    TYPE_PARAMS *params = (TYPE_PARAMS *) p;
    MSG_LOGO reclogo;
    int      ret;
    int      i;
    long     msgSize;

    /* TODO for future use */
    unsigned char inseq = 0;
    long     inkey;

    unsigned char TYPE_ERROR;
    unsigned char TYPE_HEARTBEAT;
    unsigned char TYPE_TRACEBUF2;
    unsigned char TYPE_PICK_SCNL;
    unsigned char TYPE_CODA_SCNL;
    unsigned char TYPE_QUAKE2K;
    unsigned char TYPE_LINK;
    unsigned char TYPE_HYP2000ARC;
    unsigned char TYPE_PICKTWC;
    unsigned char TYPE_HYPOTWC;
    unsigned char TYPE_ALARM;
    unsigned char TYPE_MAGNITUDE;
    unsigned char TYPE_STRONGMOTIONII;
    char    *sqlstr = NULL;
    char    *iter_sqlstr = NULL;
    char    *sub_sqlstr = NULL;
    int     ret_ex_mysql_query = EW2MOLEDB_OK;
    MYSQL   *mysql = NULL;
    char    *Wrmsg = NULL;           /* message to get from queue         */
    int count_sql_tot = 0;
    int count_sql_ok = 0;
    int count_sql_err_conndb = 0;
    int count_sql_err_exquery = 0;
    int count_sql_err_generic = 0;
    int flag_remove_last_msg = 0;

    logit( "t", "ew2moledb: thread DBPopulate() started.\n");

    GetType("TYPE_ERROR", &TYPE_ERROR);
    GetType("TYPE_HEARTBEAT", &TYPE_HEARTBEAT);
    GetType("TYPE_TRACEBUF2", &TYPE_TRACEBUF2);
    GetType("TYPE_PICK_SCNL", &TYPE_PICK_SCNL);
    GetType("TYPE_CODA_SCNL", &TYPE_CODA_SCNL);
    GetType("TYPE_QUAKE2K", &TYPE_QUAKE2K);
    GetType("TYPE_LINK", &TYPE_LINK);
    GetType("TYPE_HYP2000ARC", &TYPE_HYP2000ARC);
    GetType("TYPE_PICKTWC", &TYPE_PICKTWC);
    GetType("TYPE_HYPOTWC", &TYPE_HYPOTWC);
    GetType("TYPE_ALARM", &TYPE_ALARM);
    GetType("TYPE_MAGNITUDE", &TYPE_MAGNITUDE);
    GetType("TYPE_STRONGMOTIONII", &TYPE_STRONGMOTIONII);

    /* Allocate buffer for reading message from queue  */
    if ( ( Wrmsg = (char *) malloc(params->MaxMsgSize+1) ) ==  NULL ) 
    {
	logit( "e", "%s(%s): error allocating Wrmsg; exiting!\n",
	       params->cmdname, params->MyModName );
	exit( -1 );
    }

    while (!testlck_flags_term(FLAG_TERM_REQ_DB)) {   /* main loop */

	/* Get message from queue without removing it
         *************************/
	/* N.B. use sema_CountToProcess to synchronize receiving of messages
	 * and avoid to use sleep_ew() when queue is empty */
	WAIT_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);

	if(testlck_flags_term(FLAG_TERM_REQ_DB)) {
	  POST_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);
	  continue;
	}

	RequestSpecificMutex(&mutex_OutQueue);
	ret=cpqueuering( &OutQueue, Wrmsg, &msgSize, &reclogo, &inkey, &inseq);
	ReleaseSpecificMutex(&mutex_OutQueue);
	
	/* The following condition could occur only one time
	 * when the module quits.  */
	if (ret < 0 )
	{ /* -1 means empty queue */
	    logit("t", "DBPopulate: queue is empty!\n");
	    sleep_ew(100);
	    continue;
	}
     
	/* Determine which GetLogo this message used */
	for(i = 0; i < params->nLogo; i++) {
	    if ( (params->GetLogo[i].type   == WILD || 
		  params->GetLogo[i].type   == reclogo.type) &&
		 (params->GetLogo[i].mod    == WILD || 
		  params->GetLogo[i].mod    == reclogo.mod) &&
		 (params->GetLogo[i].instid == WILD || 
		  params->GetLogo[i].instid == reclogo.instid) )
		break;
	}
	if (i == params->nLogo) {
	    logit("et", "%s error: logo <%d.%d.%d> not found\n",
		  params->cmdname, reclogo.instid, reclogo.mod, reclogo.type);
	    continue;
	}

	/* Build the SQL string for calling the stored procedure related to each different message */
	Wrmsg[msgSize] = '\0';
	if(reclogo.type == TYPE_ERROR) {
		sqlstr = get_sqlstr_from_error_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_HEARTBEAT) {
		sqlstr = get_sqlstr_from_heartbeat_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_TRACEBUF2) {
		sqlstr = get_sqlstr_from_tracebuf_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_PICK_SCNL) {
		sqlstr = get_sqlstr_from_pick_scnl_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_CODA_SCNL) {
		sqlstr = get_sqlstr_from_coda_scnl_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_QUAKE2K) {
		sqlstr = get_sqlstr_from_quake2k_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_LINK) {
		sqlstr = get_sqlstr_from_link_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_HYP2000ARC) {
		sqlstr = get_sqlstr_from_arc_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_MAGNITUDE) {
		sqlstr = get_sqlstr_from_magnitude_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_STRONGMOTIONII) {
		sqlstr = get_sqlstr_from_strongmotionII_ew_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));

	/* Earlybird type messages */
	} else if(reclogo.type == TYPE_PICKTWC) {
		sqlstr = get_sqlstr_from_eb_picktwc_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_HYPOTWC) {
		sqlstr = get_sqlstr_from_eb_hypotwc_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));
	} else if(reclogo.type == TYPE_ALARM) {
		sqlstr = get_sqlstr_from_eb_alarm_msg(Wrmsg, msgSize, params->ewinstancename, GetModIdName(reclogo.mod));

	/* Unhandled type messages */
	} else {
		sqlstr = NULL;
		logit("e", "Unhandled message type %d!\n", reclogo.type);
	}

	if(sqlstr) {

	    count_sql_tot = 0;
	    count_sql_ok = 0;
	    count_sql_err_conndb = 0;
	    count_sql_err_exquery = 0;
	    count_sql_err_generic = 0;

	    iter_sqlstr = sqlstr;
	    while(iter_sqlstr != NULL) {
		count_sql_tot++;
		sub_sqlstr = sub_strpos(&iter_sqlstr, ';');
		logit("t", "DEBUG: sub_sqlstr=\"%s\"\n", sub_sqlstr);
		if( (ret_ex_mysql_query = ew2moledb_mysql_query_p(&mysql, sub_sqlstr,
				params->db_hostname, params->db_username, params->db_password, params->db_name, params->db_port)) == EW2MOLEDB_OK) {

		    /* sqlstr can contain more than one query! */

		    count_sql_ok++;

		} else {

		    /* Count error types */
		    switch(ret_ex_mysql_query) {
			case EW2MOLEDB_ERR_CONNDB:
			    count_sql_err_conndb++;
			    break;
			case EW2MOLEDB_ERR_EXQUERY:
			    count_sql_err_exquery++;
			    break;
			default:
			    count_sql_err_generic++;
			    break;
		    }

		    /* Report error on ring by ew2moledb_status() */
		    ew2moledb_mysql_status(ret_ex_mysql_query, params);

		    /* Wait a while after a DB error */
		    sleep_ew(params->WaitSecAfterDBError * 1000);

		} 
	    }

	    /* Check the kind of error and decide to remove or not the
	     * message from the Queue */
	    flag_remove_last_msg = 0;
	    if(count_sql_ok == count_sql_tot) {
		flag_remove_last_msg = 1;
	    } else {
		if(count_sql_err_conndb > 0) {
		    /* The item has not been successfully processed due to
		     * DB connection error, then re-increment
		     * sema_CountToProcess */
		    POST_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);
		} else {
		    flag_remove_last_msg = 1;
		}
	    }

	    if(flag_remove_last_msg) {
		/* Remove last message from queue
		 *************************/
		WAIT_SPECIFIC_SEMAPHORE_EW(sema_CountBusy);
		RequestSpecificMutex(&mutex_OutQueue);
		ret=dequeuering( &OutQueue, Wrmsg, &msgSize, &reclogo, &inkey, &inseq);
		ReleaseSpecificMutex(&mutex_OutQueue);
		POST_SPECIFIC_SEMAPHORE_EW(sema_CountAvail);

		if (ret < 0 )
		{ /* -1 means empty queue, then error */
		    logit("et", "DBPopulate: unexpected empty queue !\n");
		}
	    }

	} else {
	    logit("e", "sqlstr is NULL!\n");
	}

    }   /* End of main loop */


    /* At the end close possible open connection to DB server */
    ew2moledb_mysql_close_p(&mysql);
   
    /* we're quitting 
     *****************/
    logit("t", "ew2moledb: thread DBPopulate() terminated !\n");
    setlck_flags_term(FLAG_TERM_THR_DB);
    KillSelfThread(); /* main thread will not restart us */
    RETURN_THREAD_FUNCTION;
}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *p )
{
    TYPE_PARAMS_STACKER *params_stacker = (TYPE_PARAMS_STACKER *) p;
    SHM_INFO   InRegion;
    long       inkey;             /* Key to input ring            */
    unsigned char inseq = 0;      /* transport seq# in input ring */
    long       recsize;	/* size of retrieved message             */
    MSG_LOGO   reclogo;       /* logo of retrieved message             */
    time_t     now;
    int        ret;
    int        error_occurred = 0;
    int        NumOfTimesQueueLapped= 0; /* number of messages lost due to 
					      queue lap */

    /* Message Buffers to be allocated
     *********************************/
    char       *msgb = NULL;           /* msg retrieved from transport      */ 
    char       errText[STR_LEN_ERRTEXT]; /* string for log/error/heartbeat messages */

    /* set last character to zero */
    errText[STR_LEN_ERRTEXT-1]=0;

    /* Look up transport region keys earthworm.h tables
     ************************************************/
    if( ( inkey = GetKey(params_stacker->InRing) ) == -1 )
    {
      logit( "et", "ew2moledb: Invalid input ring name <%s>; skipping these ring!\n",
	  params_stacker->InRing );
      error_occurred = 1;
    } else {
      logit( "t", "ew2moledb: reading messages from input ring name <%s>.\n",
	  params_stacker->InRing );
    }

    if(!error_occurred) {

      /* Allocate space for messages buffer
       ***********************************************************/
      /* Buffer for Read thread: */
      if ( ( msgb = (char *) malloc(params_stacker->params->MaxMsgSize+1) ) ==  NULL ) 
      {
	logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n", 
	    params_stacker->params->cmdname, params_stacker->params->MyModName );
	exit( -1 );
      }

      /* Attach to input and output transport rings
       ******************************************/
      tport_attach( &InRegion,  inkey );

      /* Flush all old messages from the ring
       ************************************/
      while( tport_copyfrom( &InRegion, params_stacker->GetLogo, params_stacker->nLogo, &reclogo,
	    &recsize, msgb, params_stacker->params->MaxMsgSize, &inseq ) != GET_NONE );

      /* Start main service loop for current connection
       ************************************************/
      while( !testlck_flags_term(FLAG_TERM_REQ_MS) && !error_occurred)
      {
	/* Get a message from transport ring
	 ************************************/
	ret = tport_copyfrom( &InRegion, params_stacker->GetLogo, params_stacker->nLogo, &reclogo,
	    &recsize, msgb, params_stacker->params->MaxMsgSize, &inseq );

	switch (ret) {
	  case GET_NONE:
	    /* Wait if no messages for us */
	    sleep_ew(50); 
	    continue;
	    break;
	  case GET_TOOBIG:
	    time(&now);
	    snprintf( errText, STR_LEN_ERRTEXT-1, "%s(%s): %ld %hd msg[%ld] i%d m%d t%d too long for target",
		params_stacker->params->cmdname, params_stacker->params->MyModName,
		now, ERR_TOOBIG, recsize, (int) reclogo.instid,
		(int) reclogo.mod, (int)reclogo.type );
	    ew2moledb_status( &params_stacker->params->ErrorLogo, errText, params_stacker->params );
	    continue;
	    break;
	  case GET_MISS_LAPPED:
	    time(&now);
	    snprintf( errText, STR_LEN_ERRTEXT-1, "%s(%s): %ld %hd msg(s) overwritten i%d m%d t%d in %s",
		params_stacker->params->cmdname, params_stacker->params->MyModName,
		now, ERR_MISSMSG, (int) reclogo.instid,
		(int) reclogo.mod, (int)reclogo.type, params_stacker->InRing );
	    ew2moledb_status( &params_stacker->params->ErrorLogo, errText, params_stacker->params );
	    break;
	  case GET_MISS_SEQGAP:
	    time(&now);
	    snprintf( errText, STR_LEN_ERRTEXT-1, "%s(%s): %ld %hd gap in msg sequence i%d m%d t%d in %s",
		params_stacker->params->cmdname, params_stacker->params->MyModName,
		now, ERR_MISSMSG, (int) reclogo.instid,
		(int) reclogo.mod, (int)reclogo.type, params_stacker->InRing );
	    ew2moledb_status( &params_stacker->params->ErrorLogo, errText, params_stacker->params );
	    break;
	    /* only for tport_getmsg() 
	  case GET_MISS:
	    time(&now);
	    snprintf( errText, STR_LEN_ERRTEXT-1, "%s(%s): %ld %hd missed msg(s) i%d m%d t%d in %s",
		params_stacker->params->cmdname, params_stacker->params->MyModName,
		now, ERR_MISSMSG, (int) reclogo.instid,
		(int) reclogo.mod, (int)reclogo.type, params_stacker->InRing );
	    ew2moledb_status( &params_stacker->params->ErrorLogo, errText, params_stacker->params );
	    break;
	    */
	  case GET_NOTRACK:
	    time(&now);
	    snprintf( errText, STR_LEN_ERRTEXT-1, "%s(%s): %ld %hd no tracking for logo i%d m%d t%d in %s",
		params_stacker->params->cmdname, params_stacker->params->MyModName,
		now, ERR_NOTRACK, (int) reclogo.instid, (int) reclogo.mod, 
		(int)reclogo.type, params_stacker->InRing );
	    ew2moledb_status( &params_stacker->params->ErrorLogo, errText, params_stacker->params );
	    break;
	}


	/* Process retrieved msg (ret==GET_OK,GET_MISS,GET_NOTRACK) 
	 ***********************************************************/
	msgb[recsize] = '\0';

	/* put the message into the queue */
	WAIT_SPECIFIC_SEMAPHORE_EW(sema_CountAvail);
	RequestSpecificMutex(&mutex_OutQueue);
	ret=enqueuering( &OutQueue, msgb, recsize, reclogo, inkey, inseq); 
	ReleaseSpecificMutex(&mutex_OutQueue);
	POST_SPECIFIC_SEMAPHORE_EW(sema_CountBusy);

	/* Increment sema_CountToProcess */
	POST_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);

	switch(ret) {
	  case -2:
	    /* Serious: quit */
	    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
	    time(&now);
	    snprintf(errText, STR_LEN_ERRTEXT-1,"%ld %hd internal queue error. Terminating.", now, ERR_QUEUE);
	    ew2moledb_status( &params_stacker->params->ErrorLogo, errText, params_stacker->params );
	    error_occurred = 1;
	    break;
	  case -1:
	    time(&now);
	    snprintf(errText, STR_LEN_ERRTEXT-1,"%ld %hd message too big for queue.", now, 
		ERR_QUEUE);
	    ew2moledb_status( &params_stacker->params->ErrorLogo, errText, params_stacker->params );
	    continue;
	    break;
	  case -3:
	    NumOfTimesQueueLapped++;
	    logit("t", "%s(%s): Circular queue lapped. Messages lost.\n",
		    params_stacker->params->cmdname, params_stacker->params->MyModName);
	    continue; 
	}
      } /* end of while */

      /* Detach from shared memory regions and terminate
       ***********************************************/
      tport_detach( &InRegion );

      /* Deallocate space of messages buffer
       ***********************************************************/
      free(msgb);
      msgb=NULL;
    }

    /* we're quitting 
     *****************/
    logit("t", "ew2moledb: thread MessageStacker() on ring %s terminated !\n",
	params_stacker->InRing );
    dec_nStackerThread();
    KillSelfThread(); /* main thread will not restart us */
    RETURN_THREAD_FUNCTION;
}


/************* Main Thread for populating DB from queue  ***************
 *          Pull a messsage from the queue, and call stored procedure  *
 *          for inserting information in to the database               *
 **********************************************************************/
thr_ret QueueDumpFile( void *p )
{
    TYPE_PARAMS *params = (TYPE_PARAMS *) p;
    MSG_LOGO reclogo;
    int      ret;
    long     msgSize;
    char     errText[STR_LEN_ERRTEXT];
    char     *Wrmsg = NULL;           /* message to get from queue         */

    /* TODO for future use */
    unsigned char inseq = 0;
    long     inkey;

    int      ret_dq;                 /* stores errors from dumpqueue() and undumpqueue() */
    int      cur_NumOfElements = 0;  /* Current value of number of elements in OutQueue */
    QUEUE    LocOutQueue;  /* queue for saving messages read from global queue */
    int      flag_queue_is_empty = 0;

    logit( "t", "ew2moledb: thread QueueDumpFile() started.\n");

    initqueue( &LocOutQueue, (unsigned long)params->QueueSize,(unsigned long)(params->MaxMsgSize+1));

    /* set last character to zero */
    errText[STR_LEN_ERRTEXT-1]=0;

    /* Allocate buffer for reading message from queue  */
    if ( ( Wrmsg = (char *) malloc(params->MaxMsgSize+1) ) ==  NULL ) 
    {
	logit( "e", "%s(%s): error allocating Wrmsg; exiting!\n",
	       params->cmdname, params->MyModName );
	exit( -1 );
    }

    while (!testlck_flags_term(FLAG_TERM_REQ_QF) && flag_queue_is_empty == 0) {   /* main loop */

	/* Get message from queue without removing it
	 *************************/
	/* N.B. use sema_CountToProcess to synchronize receiving of messages
	 * and avoid to use sleep_ew() when queue is empty */
	WAIT_SPECIFIC_SEMAPHORE_EW(sema_CountToProcess);

	RequestSpecificMutex(&mutex_OutQueue);
	ret=cpqueuering( &OutQueue, Wrmsg, &msgSize, &reclogo, &inkey, &inseq);
	ReleaseSpecificMutex(&mutex_OutQueue);

	/* The following condition could occur only one time
	 * when the module quits.  */
	if (ret < 0 )
	{ /* -1 means empty queue */
	    logit("t", "QueueDumpFile: queue is empty!\n");
	    flag_queue_is_empty = 1;
	    continue;
	}

	/* Build the SQL string for calling the stored procedure related to each different message */
	Wrmsg[msgSize] = '\0';

	/* Remove last message from queue
	 *************************/
	WAIT_SPECIFIC_SEMAPHORE_EW(sema_CountBusy);
	RequestSpecificMutex(&mutex_OutQueue);
	ret=dequeuering( &OutQueue, Wrmsg, &msgSize, &reclogo, &inkey, &inseq);
	ReleaseSpecificMutex(&mutex_OutQueue);
	POST_SPECIFIC_SEMAPHORE_EW(sema_CountAvail);

	if(ret == 0) {
	  ret=enqueuering( &LocOutQueue, Wrmsg, msgSize, reclogo, inkey, inseq ); 
	}

    }   /* End of main loop */

    /* Dump queue messages to file */
    cur_NumOfElements = LocOutQueue.NumOfElements;
    ret_dq=dumpqueue( &LocOutQueue, params->QueueFilename);

    switch (ret_dq) {
	case -1:
	    logit("et", "errors opening or writing dump file '%s'; dump file is deleted.\n", params->QueueFilename);
	    break;
	case 0:
	    logit("et", "dumped %d queue messages to file '%s'.\n", cur_NumOfElements, params->QueueFilename);
	    break;
	default:
	    logit("et", "Unknown error %d from dumpqueue() writing file '%s'.\n", ret_dq, params->QueueFilename);
	    break;
    }

    freequeue(&LocOutQueue);

    /* we're quitting 
     *****************/
    logit("t", "ew2moledb: thread QueueDumpFile() terminated !\n");
    setlck_flags_term(FLAG_TERM_THR_QF);
    KillSelfThread(); /* main thread will not restart us */
    RETURN_THREAD_FUNCTION;
}


/*****************************************************************************
 *  ew2moledb_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.	             *
 *****************************************************************************/
#define NCOMMANDS 12
void ew2moledb_config( char *configfile, TYPE_PARAMS *params )
{
    int      ncommand;     /* # of required commands you expect to process   */ 
    char     init[NCOMMANDS];     /* init flags, one byte for each required command */
    int      nmiss;        /* number of required commands that were missed   */
    char    *com;
    int      nfiles;
    int      success;
    int      i;	
    int      arglen;	
    char*    str;
    unsigned char modid;

    params->HeartBeatRing[0] = 0;
    params->nRings = 0;
    params->WaitSecAfterDBError = 5; /* Seconds to wait after a DB error */

    /* Set to zero one init flag for each required command 
     *****************************************************/   
    ncommand = NCOMMANDS;
    for( i=0; i<ncommand; i++ )  init[i] = 0;
    params->nLogo = 0;
    params->GetLogo = NULL;

    params->db_hostname[0] = 0;
    params->db_username[0] = 0;
    params->db_password[0] = 0;
    params->db_name[0] = 0;
    params->db_port = 0;

    flag_debug_mysql = 0;

    params->ewinstancename[0] = 0;

    params->QueueFilename[0] = 0;
    params->QueueFilename[FILENAME_MAXLEN - 1] = 0;
   
    /* Open the main configuration file 
     **********************************/
    nfiles = k_open( configfile ); 
    if ( nfiles == 0 ) {
	logit( "e" ,
	       "%s: Error opening command file <%s>; exiting!\n", 
	       params->cmdname, configfile );
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
                           params->cmdname, &com[1] );
		    exit( -1 );
		}
		continue;
            }

	    /* Process anything else as a command 
             ************************************/
  /*0*/     if( k_its("LogFile") ) {
                params->LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strncpy( params->MyModName, str, MAX_MOD_STR-1 );
		if ( GetModId( params->MyModName, &modid ) != 0 ) {
		    logit( "e",
			    "%s: Invalid module name <%s>; exiting!\n", 
			    params->cmdname, params->MyModName );
		    exit( -1 );
		}
		snprintf(params->QueueFilename, FILENAME_MAXLEN - 1, "ew2moledb_%03d.queue", (int) modid);
                init[1] = 1;
            }
  /*2*/     else if( k_its("InRing") ) {
                str = k_str();
		if(str) {
		  /* First ring declared will be used as HeartBeat ring */
		  if(params->HeartBeatRing[0] == 0 ) {
		    strncpy( params->HeartBeatRing, str, MAX_RING_STR-1 );
		  }
		  strncpy( params->InRings[params->nRings], str, MAX_RING_STR-1 );
		  params->nRings++;
		}
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInt") ) {
                params->HeartBeatInt = k_int();
                init[3] = 1;
            }

	    /* Enter installation & module & message types to get
             ****************************************************/
  /*4*/     else if( k_its("GetMsgLogo") ) {
		if ((params->GetLogo = (MSG_LOGO*)realloc(params->GetLogo, (params->nLogo+1) * sizeof(MSG_LOGO))) 
		    == NULL) {
		    logit( "e" , 
			   "%s: out of memory for Logos\n", params->cmdname );
		    exit( -1 );
		}		
		if( ( str=k_str() ) ) {
		    if( GetInst( str, &params->GetLogo[params->nLogo].instid ) != 0 ) {
			logit( "e" , 
			       "%s: Invalid installation name <%s>", params->cmdname, str ); 
			logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
			exit( -1 );
		    }
		}
		/* TODO read RING NAME ??? */
                if( ( str=k_str() ) ) {
		    if( GetModId( str, &params->GetLogo[params->nLogo].mod ) != 0 ) {
			logit( "e" , 
                               "%s: Invalid module name <%s>", params->cmdname, str ); 
			logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
			exit( -1 );
		    }
                }
                if( ( str=k_str() ) ) {
		    if( GetType( str, &params->GetLogo[params->nLogo].type ) != 0 ) {
			logit( "e" , 
                               "%s: Invalid msgtype <%s>", params->cmdname, str ); 
			logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
			exit( -1 );
		    }
                }
		params->nLogo++;
                init[4] = 1;
            }
			
	    /* Maximum size (bytes) for incoming/outgoing messages
             *****************************************************/ 
  /*5*/     else if( k_its("MaxMsgSize") ) {
                params->MaxMsgSize = k_long();
                init[5] = 1;
            }

	    /* Maximum number of messages in outgoing circular buffer
             ********************************************************/ 
  /*6*/     else if( k_its("QueueSize") ) {
                params->QueueSize = k_long();
                init[6] = 1;
            }

	    /* DB hostname
             ********************/
    /*7*/   else if( k_its("DBHostname") ) {
		if ( (str = k_str() ) ) {
		    strncpy(params->db_hostname, str, 255);
		    params->db_hostname[255] = '\0';
		    init[7] = 1;
		}
	    }
		
	    /* DB username
             ********************/
    /*8*/   else if( k_its("DBUsername") ) {
		if ( (str = k_str() ) ) {
		    strncpy(params->db_username, str, 255);
		    params->db_username[255] = '\0';
		    init[8] = 1;
		}
	    }
		
	    /* DB password
             ********************/
    /*9*/   else if( k_its("DBPassword") ) {
		if ( (str = k_str() ) ) {
		    strncpy(params->db_password, str, 255);
		    params->db_password[255] = '\0';
		    init[9] = 1;
		}
	    }
		
	    /* DB name
             ********************/
    /*10*/   else if( k_its("DBName") ) {
		if ( (str = k_str() ) ) {
		    strncpy(params->db_name, str, 255);
		    params->db_name[255] = '\0';
		    init[10] = 1;
		}
	    }
		
	    /* EW instance name
             ********************/
    /*11*/   else if( k_its("EWInstanceName") ) {
		if ( (str = k_str() ) ) {
		    strncpy(params->ewinstancename, str, 255);
		    params->ewinstancename[255] = '\0';
		    init[11] = 1;
		}
	    }
		
	 /* Optional commands */

/*opt*/     else if( k_its("DBPort") ) {
		params->db_port = k_int();
	    }

	    /* Read the full path to the program used to send mail
	     *       ****************************************************/
	      else if( k_its( "MailProgram" ) )  /* optional */
	      { 
		  if( (str = k_str()) )
		  {
		      strncpy( ew2moledb_mailProg, str, MAX_SIZE_STRING_SENDMAIL );
		  }
	      } 

	      /* Read the name of the computer which serves mail
	       *       ***********************************************/
	      else if( k_its( "MailServer" ) )
	      {
		  if( (str = k_str()) )
		  {
		      strncpy( ew2moledb_mailServer, str, MAX_SIZE_STRING_SENDMAIL );
		  }
	      }

	      /* Set the sender/from field of the email messages
	       *       **************************************************/
	      else if( k_its( "mailFrom" ) )  /* optional */
	      {
		  if( (str = k_str()) )
		  {
		      strncpy( ew2moledb_mailFrom, str, MAX_SIZE_STRING_SENDMAIL );
		  }
	      }

	      /* Read mail recipients - overrides mail commands in ew2moledb config
	       *       *******************************************************************/
	      else if( k_its( "mail" ) )  /* optional */
	      {
		  if( ew2moledb_nmailRecipients >= EW2MOLEDB_MAXRECIP )
		  {
		      logit("e", "ew2moledb: Too many <mail> commands in Descriptor;"
			      " max=%d; exiting.\n", EW2MOLEDB_MAXRECIP );
		      exit( -1 );
		  }
		  str = k_str();
		  if( !str ) 
		  { 
		      logit("e","ew2moledb: No argument for <mail> command in Descriptor; "
			      "exiting.\n");
		      exit( -1 );
		  }
		  arglen = strlen(str);
		  if( arglen==0 || arglen>=EW2MOLEDB_MAXRECIPLEN )
		  {
		      logit("e","ew2moledb: Invalid length=%d for <mail> argument in "
			      "Descriptor; valid=1-%d; exiting.\n",
			      arglen, EW2MOLEDB_MAXRECIPLEN-1 );
		      exit( -1 );
		  }
		  strcpy( ew2moledb_mailRecipients[ew2moledb_nmailRecipients], str );
		  ew2moledb_nmailRecipients++;
	      }

	    /* Milliseconds to wait after a DB error
             ********************************************************/ 
            else if( k_its("WaitSecAfterDBError") ) {
                params->WaitSecAfterDBError = k_long();
            }

	    /* Flag to debug mysql instructions
             ********************************************************/ 
            else if( k_its("DebugMySql") ) {
                flag_debug_mysql = k_long();
            }

	    /* Unknown command
             *****************/ 
	    else {
                logit( "e" , "%s: <%s> Unknown command in <%s>.\n", 
		       params->cmdname, com, configfile );
                continue;
            }
 
	    /* See if there were any errors processing the command 
             *****************************************************/
            if( k_err() ) {
		logit( "e" , 
                       "%s: Bad <%s> command  in <%s>; exiting!\n",
		       params->cmdname, com, configfile );
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
	logit( "e", "%s: ERROR, no ", params->cmdname );
	if ( !init[0] )  logit( "e", "<LogFile> "      );
	if ( !init[1] )  logit( "e", "<MyModuleId> "   );
	if ( !init[2] )  logit( "e", "<InRing> "     );
	if ( !init[3] )  logit( "e", "<HeartBeatInt> " );
	if ( !init[4] )  logit( "e", "<GetMsgLogo> "   );
	if ( !init[5] )  logit( "e", "<MaxMsgSize> "  );
	if ( !init[6] )  logit( "e", "<Queue>"   );
	if ( !init[7] )  logit( "e", "<DBHostname>"   );
	if ( !init[8] )  logit( "e", "<DBUsername>"   );
	if ( !init[9] )  logit( "e", "<DBPassword>"   );
	if ( !init[10] )  logit( "e", "<DBName>"   );
	if ( !init[11] )  logit( "e", "<EWInstanceName>"   );
	logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
	exit( -1 );
    }

    return;
}


/****************************************************************************
 *  ew2moledb_lookup()   Look up important info from earthworm.h tables     *
 ****************************************************************************/
void ew2moledb_lookup( TYPE_PARAMS *params )
{
    /* Look up keys to shared memory regions
     *************************************/
    if( ( params->HeartBeatRingKey = GetKey(params->HeartBeatRing) ) == -1 ) {
	logit( "e",
               "%s:  Invalid ring name <%s>; exiting!\n", 
               params->cmdname, params->HeartBeatRing);
	exit( -1 );
    }   

    /* Look up installations of interest
     *********************************/
    if ( GetLocalInst( &params->HeartLogo.instid ) != 0 ) {
	logit( "e",
               "%s: error getting local installation id; exiting!\n",
               params->cmdname );
	exit( -1 );
    }
    params->ErrorLogo.instid = params->HeartLogo.instid;
   
    /* Look up modules of interest
     ***************************/
    if ( GetModId( params->MyModName, &params->HeartLogo.mod ) != 0 ) {
	logit( "e",
               "%s: Invalid module name <%s>; exiting!\n", 
               params->cmdname, params->MyModName );
	exit( -1 );
    }
    params->ErrorLogo.mod = params->HeartLogo.mod;

    /* Look up message types of interest
     *********************************/
    if ( GetType( "TYPE_HEARTBEAT", &params->HeartLogo.type ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", params->cmdname );
	exit( -1 );
    }
    if ( GetType( "TYPE_ERROR", &params->ErrorLogo.type ) != 0 ) {
	logit( "e",
               "%s: Invalid message type <TYPE_ERROR>; exiting!\n", params->cmdname );
	exit( -1 );
    }
    
} 


/***************************************************************************
 * ew2moledb_status() sends an error or heartbeat message to transport       *
 *    If the message is of TYPE_ERROR, the text will also be logged.       *
 *    Since ew2moledb_status is called be more than one thread, the logo     *
 *    and message must be constructed by the caller.                       *
 ***************************************************************************/
void ew2moledb_status( MSG_LOGO *logo, char *msg, TYPE_PARAMS *params)
{
    size_t size;
    
    if( logo->type == params->ErrorLogo.type)
	logit( "et", "%s\n", msg );

    size = strlen( msg );   /* don't include the null byte in the message */ 	

    /* Write the message to shared memory
     ************************************/
    if( tport_putmsg( &params->HeartBeatRegion, logo, size, msg ) != PUT_OK ) {
	logit("et", "%s(%s):  Error sending message to transport.\n", 
	      params->cmdname, params->MyModName );
    }
}

/***************************************************************************
 * ew2moledb_mysql_status()                                                *
 *    If ret_ex_mysql_query is not equal to EW2MOLEDB_OK then report error *
 *    message on ring by ew2moledb_status().                               *
 ***************************************************************************/
void ew2moledb_mysql_status(int ret_ex_mysql_query, TYPE_PARAMS *params) {
  time_t   now;	/* current time, used for error messages */
  char    errText[STR_LEN_ERRTEXT];

  /* set last character to zero */
  errText[STR_LEN_ERRTEXT-1]=0;

  if(ret_ex_mysql_query != EW2MOLEDB_OK) {
    time(&now);
    switch(ret_ex_mysql_query) {
      case EW2MOLEDB_ERR_CONNDB:
	snprintf( errText, STR_LEN_ERRTEXT-1, "%ld %hd %s Error connecting to %s:%ld.", now, 0, params->MyModName, params->db_hostname, params->db_port);
	break;
      case EW2MOLEDB_ERR_EXQUERY:
	snprintf( errText, STR_LEN_ERRTEXT-1, "%ld %hd %s Error executing query for %s:%ld.", now, 0, params->MyModName, params->db_hostname, params->db_port);
	break;
      default:
	snprintf( errText, STR_LEN_ERRTEXT-1, "%ld %hd %s Error generic for %s:%ld.", now, 0, params->MyModName, params->db_hostname, params->db_port);
	break;
    }
    ew2moledb_status( &params->ErrorLogo, errText, params );
  }
}

/* increment the number of running Stacker Threads
 ***************************************************/
void inc_nStackerThread() {
  RequestSpecificMutex(&mutex_nStackerThread);
  nStackerRunning++;
  ReleaseSpecificMutex(&mutex_nStackerThread);
}

/* decrement the number of running Stacker Threads and
 * in case they are zero set the flag FLAG_TERM_THR_STACKER
 ***************************************************/
void dec_nStackerThread() {
  RequestSpecificMutex(&mutex_nStackerThread);
  nStackerRunning--;
  if(nStackerRunning <= 0) {
    setlck_flags_term(FLAG_TERM_THR_MS);
  }
  ReleaseSpecificMutex(&mutex_nStackerThread);
}


/* Remove duplicated entries in ringNames
 ***************************************************/
int remove_duplicated_ringname(char ringNames[MAX_NUM_OF_RINGS][MAX_RING_STR], int *nRings) {
  int ret = 0;
  int r = 0;
  int k = 0;
  int j = 0;

  r = 0;
  while(r < *nRings) {
    k = r+1;

    while(k < *nRings) {
      if(strcmp(ringNames[r], ringNames[k]) == 0) {
	ret++;
	logit("et",  "ew2moledb: WARNING: ring name %s duplicated, skip!\n", ringNames[r]);
	/* override the string at k position */
	for(j=k; j < *nRings - 1; j++) {
	  strncpy(ringNames[j], ringNames[j+1], MAX_RING_STR-1);
	}
	*nRings = *nRings - 1;
      } else {
	k++;
      }
    }

    r++;
  }

  return ret;
}

