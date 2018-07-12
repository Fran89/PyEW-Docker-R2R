/*********************************************************************
     geqproc                        Mitch Withers, January 2005.
  
Our story:
geqproc is a first process in the earthquake-processing mega-module (sometimes
referred to as "the sausage") that produces earthquake locations for the
Earthworm system. geqproc not unlike eqroc, the primary difference being 
location, pick, and link messages are assumed to be assembled (and waited
for) by another module such as globalproc. geqproc, the first link in the
mega-module, is listed in startstop's configuration file to be started by
startstop. geqproc then starts the next process, specified in its "PipeTo"
command, and communicates with it via a one-directional pipe. Each newly
created sub-module starts the next link in the same way. From startstop's
point of view, the whole mega-module inherits the name of the first link
(geqproc) and that's the only name it displays. From statmgr's point of view,
all processes within the mega-module share one module id, one heartbeat, and
one descriptor file. However, each sub-module has its own configuration file
and its own log file. A typically the next module in the pipe will be
eqbuf followed by hyp2000_mgr.  Note that we do not generally run eqcoda
and eqverify since type_loc_global don't have coda picks.

We pick up TYPE_LOC_GLOBAL from the input ring (as output by global_proc),
convert to TYPE_HYP2000ARC, and then pass along the pipe.  There
is no throttle or waiting to make sure this is the "final" version
as that task is assumed to be performed elsewhere such as in global_proc.

Note also that we throw out any arrivals not suitable for Hypo (e.g. depth
phases).  This is dangerous and could result in a solution considerably
less accurate than the original.  It is concommitant upon the user to
assure that geqproc is attached to a ring that only has global loc
messages for areas with dense station coverage for which picks are
included in the incoming message.  eqfilterII may be used for this task.

*********************************************************************/

#ifdef _OS2
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <mem_circ_queue.h>
#include <sys/types.h>
#include <global_loc_rw.h>
#include <read_arc.h>
#include <ew_event_info.h>
#include <rw_glevt.h>

#include "geqproc.h"

static  SHM_INFO  InRegion;      /* shared memory region to use for i/o    */
static  SHM_INFO  OutRegion;     /* shared memory region to use for i/o    */

#define BUFLEN MAX_BYTES_PER_EQ

#define   NUM_COMMANDS 	7 	/* how many required commands in the config file */
#define   MAX_STR 255

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;
 

/* The message queue
 *******************/
#define	QUEUE_SIZE		1000	    /* How many msgs can we queue */
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
static char    OutRingName[MAX_RING_STR];     /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];       /* this module's given name          */
static char    MyProgName[256];     /* this module's program name        */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */
static int     Debug = 0;   			/* 1- print debug, 0- no debug       */
static char NextProc[150];        /* actual command to start next program   */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static long          OutKey;        /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char InstWild;
static unsigned char ModWild;

static unsigned char TypeLocGlobal, TypeHyp2000Arc;
static unsigned char TypeKill;

/* Error messages used by eqfilterII 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/

/* Functions in this source file 
 *******************************/
static	int  	geqproc_config (char *);
static	int  	geqproc_lookup (void);
static	void  	geqproc_status (unsigned char, short, char *);
thr_ret			MessageStacker (void *);
thr_ret			Processor (void *);
int glloc2hyp2k(char *, char *, long);

main (int argc, char **argv)
{
  time_t		timeNow;       /* current time                  */ 
  time_t		timeLastBeat;  /* time last heartbeat was sent  */
  long			recsize;       /* size of retrieved message     */
  MSG_LOGO		reclogo;       /* logo of retrieved message     */
  char			*flushmsg;
  int			i, j, k;

  /* Check command line arguments 
   ******************************/
  if (argc != 2)
  {
    fprintf (stderr, "Usage: geqproc <configfile>\n");
    return EW_FAILURE;
  }

  /* To be used in logging functions
   *********************************/
  if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
  {
    fprintf (stderr, "geqproc: Call to get_prog_name failed.\n");
    return EW_FAILURE;
  }

  /* Initialize name of log-file & open it 
   ***************************************/
  logit_init (argv[1], 0, 256, 1);

  /* Read the configuration file(s)
   ********************************/
  if (geqproc_config(argv[1]) != EW_SUCCESS)
  {
    logit( "e", "geqproc: Call to geqproc_config failed \n");
    return EW_FAILURE;
  }
  logit ("" , "%s(%s): Read command file <%s>\n", 
         MyProgName, MyModName, argv[1]);

  /* Look up important info from earthworm.h tables
   ************************************************/
  if (geqproc_lookup() != EW_SUCCESS)
  {
    logit( "e", "%s(%s): Call to geqproc_lookup failed \n",
             MyProgName, MyModName);
    return EW_FAILURE;
  }
  /* run through the logos and make sure only valid message types
     were configured.
   **************************************************************/
  for(i=0; i<nLogo; i++){
    if(GetLogo[i].type != TypeHyp2000Arc && 
           GetLogo[i].type != TypeLocGlobal){
      logit( "e", "%s(%s): Unsupported message type (%d)\n",
               MyProgName, MyModName, GetLogo[i].type);
      return EW_FAILURE;
    }
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

  /* Start the next processing program & open pipe to it
   *****************************************************/
   if( pipe_init( NextProc, 0 ) < 0 ) {
        logit( "et",
               "geqproc: Error opening pipe to %s; exiting!\n", NextProc);
        return EW_FAILURE;
   }
   logit( "e", "geqproc: piping output to <%s>\n", NextProc );

  /* Force a heartbeat to be issued in first pass thru main loop
   *************************************************************/
  timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

  /* Flush the incoming transport ring 
   ***********************************/
  if ((flushmsg = (char *) malloc (BUFLEN)) ==  NULL)
  {
    logit ("e", "geqproc: can't allocate flushmsg; exiting.\n");
    return EW_FAILURE;
  }

  while (tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
         &recsize, flushmsg, (BUFLEN - 1)) != GET_NONE);

  /* Create MsgQueue mutex */
  CreateMutex_ew();

  /* Allocate the message Queue
   ********************************/
  initqueue (&MsgQueue, QUEUE_SIZE, BUFLEN);

  /* Start message stacking thread which will read 
   * messages from the InRing and put them into the Queue 
   *******************************************************/
  if (StartThread (MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
  {
    logit( "e", "geqproc: Error starting MessageStacker thread.  Exiting.\n");
    tport_detach (&InRegion);
    tport_detach (&OutRegion);
    return EW_FAILURE;
  }

  MessageStackerStatus = 0; /*assume the best*/

  /* Start processing thread which will read messages from
   * the Queue, process them and write them to the OutRing
   *******************************************************/
  if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
  {
    logit( "e", "geqproc: Error starting Processor thread.  Exiting.\n");
    tport_detach (&InRegion);
    tport_detach (&OutRegion);
    return EW_FAILURE;
  }

  ProcessorStatus = 0; /*assume the best*/

/*--------------------- setup done; start main loop -------------------------*/

  /* We don't do much here - just beat our heart 
   * and check on our threads 
   **********************************************/
  while (tport_getflag (&InRegion) != TERMINATE  &&
         tport_getflag (&InRegion) !=  MyPid )
  {

    /* send geqproc's heartbeat
    ***************************/
    if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
    {
      timeLastBeat = timeNow;
      geqproc_status (TypeHeartBeat, 0, ""); 
    }

    /* Check on our threads */
    if (MessageStackerStatus < 0)
    {
      logit ("et", "geqproc: MessageStacker thread died. Exiting\n");
      return EW_FAILURE;
    }

    if (ProcessorStatus < 0)
    {
      logit ("et", "geqproc: Processor thread died. Exiting\n");
      return EW_FAILURE;
    }

    if ( pipe_error() )
    {
      logit( "et", "geqproc: Output pipe error. Exiting\n" );
      break;
    }

    sleep_ew (1000);

  } /* wait until TERMINATE is raised  */  

  if ( !pipe_error() )
    logit ("t", "geqproc: Termination requested; exiting!\n" );

  /* Termination has been requested
   ********************************/
   /* send a kill message down the pipe */
  pipe_put( "", TypeKill );

  tport_detach (&InRegion);
  tport_detach (&OutRegion);
  sleep_ew( 500 );  /* give time for msg to get through pipe */
  pipe_close();

  return EW_SUCCESS;

}

/******************************************************************************
 *   geqproc_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int geqproc_config (char *configfile)
{
  char  init[NUM_COMMANDS];  /*init flags, one byte for each required command */
  int  	nmiss;             /*number of required commands that were missed   */
  char 	*com;
  char 	*str;
  int  	nfiles;
  int  	success;
  int  	i, j, gotit, num_sides, tmpint;
  unsigned char	instid;

  /* Set to zero one init flag for each required command 
  *****************************************************/   
  for (i = 0; i < NUM_COMMANDS; i++)
    init[i] = 0;

  nLogo = 0;
  numInst = 0;

  /* Open the main configuration file 
   **********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    logit("e", "geqproc: Error opening command file <%s>; exiting!\n", configfile);
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
          logit("e", "geqproc: Error opening command file <%s>; exiting!\n",
                   &com[1]);
          return EW_FAILURE;
        }
        continue;
      }

      /* Process anything else as a command 
       ************************************/
/*0*/ if (k_its ("MyModuleId")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (MyModName, str);
          init[0] = 1;
        }
      }
/*1*/ else if (k_its ("InRing")) 
      {
      if ((str = k_str ()) != NULL)
        {
          strcpy (InRingName, str);
          init[1] = 1;
        }
      }
/*2*/ else if (k_its ("OutRing")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (OutRingName, str);
          init[2] = 1;
        }
      }
/*3*/ else if (k_its ("HeartBeatInt")) 
      {
        HeartBeatInterval = k_long ();
        init[3] = 1;
      }
/*4*/ else if (k_its ("LogFile"))
      {
        LogSwitch = k_int();
        init[4] = 1;
      }

/*NR*/else if (k_its ("Debug"))
      {
        Debug = 1;
      }

      /* Enter installation & module types to get
       *******************************************/
/*5*/ else if (k_its ("GetEventsFrom")) 
      {
        if (nLogo >= MAXLOGO) 
        {
          logit("e", "geqproc: Too many <GetMsgLogo> commands in <%s>; "
                   "; max=%d; exiting!\n", configfile, (int) MAXLOGO);
          return EW_FAILURE;
        }
        if ((str = k_str())) 
        {
          if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
          {
            logit("e", "geqproc: Invalid installation name <%s> in "
                     "<GetEventsFrom> cmd; exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if ((str = k_str())) 
        {
          if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
          {
            logit("e", "geqproc: Invalid module name <%s> in <GetEventsFrom> "
                     "cmd; exiting!\n", str);
            return EW_FAILURE;
          }
        }
        /* We'll always fetch loc_global messages */
        if (GetType ("TYPE_LOC_GLOBAL", &GetLogo[nLogo].type) != 0) {
              logit("e", "geqproc: Invalid msgtype <TYPE_LOC_GLOBAL> ");
              logit("e", "; exitting!!\n");
              return EW_FAILURE;
        }
        nLogo++;
        init[5] = 1;
      }
/*6*/ else if (k_its ("PipeTo"))
      {
        str = k_str();
        if(str) strcpy(NextProc, str);
        init[6] = 1;
      }
      /* Unknown command
       *****************/ 
      else 
      {
        logit("e", "geqproc: <%s> Unknown command in <%s>.\n", com, configfile);
        continue;
      }

      /* See if there were any errors processing the command 
       *****************************************************/
      if (k_err ()) 
      {
        logit("e", "geqproc: Bad <%s> command in <%s>; exiting!\n", com, configfile);
        return EW_FAILURE;
      }

    } /** while k_rd() **/

    nfiles = k_close ();

  } /** while nfiles **/

  /* After all files are closed, check init flags for missed commands
   ******************************************************************/
  nmiss = 0;
  for (i = 0; i < NUM_COMMANDS; i++)  
  {
    if (!init[i]) 
    {
      nmiss++;
    }
  }

  if (nmiss) 
  {
    logit("e", "geqproc: ERROR, no ");
    if (!init[0])  logit("e", "<MyModuleId> "    );
    if (!init[1])  logit("e", "<InRing> "        );
    if (!init[2])  logit("e", "<OutRing> "       );
    if (!init[3])  logit("e", "<HeartBeatInt> "  );
    if (!init[4])  logit("e", "<LogFile> "       );
    if (!init[5])  logit("e", "<GetEventsFrom> " );
    if (!init[6])  logit("e", "<PipeTo> " );

    logit("e", "command(s) in <%s>; exiting!\n", configfile);
    return EW_FAILURE;
  }

  return EW_SUCCESS;
}

/******************************************************************************
 *  geqproc_lookup( )   Look up important info from earthworm.h tables        *
 ******************************************************************************/
static int geqproc_lookup( void )
{

  /* Look up keys to shared memory regions
  *************************************/
  if ((InKey = GetKey (InRingName)) == -1) 
  {
    logit( "e", "geqproc:  Invalid ring name <%s>; exiting!\n", InRingName);
    return EW_FAILURE;
  }

  if ((OutKey = GetKey (OutRingName) ) == -1) 
  {
    logit( "e", "geqproc:  Invalid ring name <%s>; exiting!\n", OutRingName);
    return EW_FAILURE;
  }

  /* Look up installations of interest
  *********************************/
  if (GetLocalInst (&InstId) != 0) 
  {
    logit( "e", "geqproc: error getting local installation id; exiting!\n");
    return EW_FAILURE;
  }

  if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
  { 
    logit( "e", "geqproc: error getting wildcard installation id; exiting!\n");
    return EW_FAILURE;
  }

  /* Look up modules of interest
  ******************************/
  if (GetModId (MyModName, &MyModId) != 0) 
  {
    logit( "e", "geqproc: Invalid module name <%s>; exiting!\n", MyModName);
    return EW_FAILURE;
  }

  if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
  {
    logit( "e", "geqproc: Invalid module name <MOD_WILDCARD>; exiting!\n");
    return EW_FAILURE;
  }

  /* Look up message types of interest
  *********************************/
  if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
  {
    logit( "e", "geqproc: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_ERROR", &TypeError) != 0) 
  {
    logit( "e", "geqproc: Invalid message type <TYPE_ERROR>; exiting!\n");
    return EW_FAILURE;
  }
  if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
    logit( "e", "geqproc: Invalid message type <TYPE_KILL>; exiting!\n");
    return EW_FAILURE;
  }
  if (GetType ("TYPE_LOC_GLOBAL", &TypeLocGlobal) != 0) 
  {
    logit( "e", "geqproc: Invalid message type <TYPE_LOC_GLOBAL>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_HYP2000ARC", &TypeHyp2000Arc) != 0) 
  {
    logit( "e", "geqproc: Invalid message type <TYPE_HYP2000ARC>; exiting!\n");
    return EW_FAILURE;
  }
  return EW_SUCCESS;
} 

/******************************************************************************
 * geqproc_status() builds a heartbeat or error message & puts it into        *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void geqproc_status( unsigned char type, short ierr, char *note )
{
  MSG_LOGO    logo;
  char        msg[256];
  long        size;
  time_t        t;
 
  /* Build the message 
  *******************/ 
  logo.instid = InstId;
  logo.mod    = MyModId;
  logo.type   = type;

  time (&t);

  if (type == TypeHeartBeat)
  {
    sprintf (msg, "%ld %ld\n\0", (long) t, (long) MyPid);
  }
  else if (type == TypeError)
  {
    sprintf (msg, "%ld %hd %s\n\0", (long) t, ierr, note);
    logit ("et", "%s(%s): %s\n", MyProgName, MyModName, note);
  }

  size = strlen (msg);   /* don't include the null byte in the message */     

  /* Write the message to shared memory
  ************************************/
  if (tport_putmsg (&OutRegion, &logo, size, msg) != PUT_OK)
  {
    if (type == TypeHeartBeat) 
    {
      logit ("et","%s(%s):  Error sending heartbeat.\n", MyProgName, MyModName);
    }
    else if (type == TypeError) 
    {
      logit ("et","%s(%s):  Error sending error:%d.\n", MyProgName, MyModName, ierr);
    }
  }
}

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret	MessageStacker (void *dummy)
{
  char         	*msg;           /* "raw" retrieved message */
  int          	res;
  long         	recsize;        /* size of retrieved message */
  MSG_LOGO     	reclogo;        /* logo of retrieved message */
  int          	ret;

  /* Allocate space for input/output messages
   *******************************************/
  if ((msg = (char *) malloc (BUFLEN)) == (char *) NULL)
  {
    logit ("e", "geqproc: error allocating msg; exiting!\n");
    goto error;
  }

  /* Tell the main thread we're ok
   ********************************/
  MessageStackerStatus = 0;

  /* Start service loop, picking up messages
   *****************************************/
  while (1)
  {
    /* Get a message from transport ring, carefully checking error codes
     *******************************************************************/
    res = tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
                        &recsize, msg, BUFLEN-1);

    if (res == GET_NONE)  /* no messages for us now */
    {
      sleep_ew(100); 
      continue;
    } 

    if (res != GET_OK)    /* some kind of error code, speak */
    {
      if (res == GET_TOOBIG) /* msg too big for buffer */
      {
        sprintf (errText, "msg[%ld] i%d m%d t%d too long for target", recsize,
                           (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type);
        geqproc_status (TypeError, ERR_TOOBIG, errText);
        continue;
      }
      else if (res == GET_MISS)  /* got msg, but may have missed some */
      {
        sprintf (errText, "missed msg(s) i%d m%d t%d in %s", (int) reclogo.instid,
                           (int) reclogo.mod, (int)reclogo.type, InRingName);
        geqproc_status (TypeError, ERR_MISSMSG, errText);
      }
      else if (res == GET_NOTRACK) /* got msg, can't tell if any were missed */
      {
        sprintf (errText, "no tracking for logo i%d m%d t%d in %s", (int) reclogo.instid, 
                           (int) reclogo.mod, (int)reclogo.type, InRingName);
        geqproc_status (TypeError, ERR_NOTRACK, errText);
      }
    }

 /* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
  ********************************************************/
    RequestMutex ();
    ret = enqueue (&MsgQueue, msg, recsize, reclogo); 
    ReleaseMutex_ew ();

    if (ret != 0)
    {
      if (ret == -2)  /* Serious: quit */
      {
        sprintf (errText, "internal queue error. Terminating.");
        geqproc_status (TypeError, ERR_QUEUE, errText);
        goto error;
      }
      if (ret == -1)
      {
        sprintf (errText,"queue cannot allocate memory. Lost message.");
        geqproc_status (TypeError, ERR_QUEUE, errText);
        continue;
      }
      if (ret == -3) 
      {
        sprintf (errText, "Queue full. Message lost.");
        geqproc_status (TypeError, ERR_QUEUE, errText);
        continue;
      }
    } /* problem from enqueue */

  } /* while (1) */

  /* we're quitting
   *****************/
  error:
    MessageStackerStatus = -1; /* file a complaint to the main thread */
    KillSelfThread (); /* main thread will restart us */
 
}

/********************** Message Processing Thread ****************
 *
 * This is where all the action is: grab a message from the 
 * queue, determine the originating installation ID. Convert
 * to hyp2000arc, then send it down the pipe.
 *
 ******************************************************************/
thr_ret	Processor (void *dummy)
{

  int ret, i, j, k;
  int gotMsg, gotit, gotit2;
  long msgSize;
  MSG_LOGO reclogo;           /* logo of retrieved message */
  float	lat, lon;
  static char record[BUFLEN]; /* dare we assume global locs are biggest? */
  static char record2[BUFLEN]; /* StringToLoc isolator */
  unsigned char INST_WILDCARD;
  char hypmsg[MAX_BYTES_PER_EQ];
  int retVal = -1;

  GetInst("INST_WILDCARD",&INST_WILDCARD);
  while (1)
  {
    gotMsg = FALSE;
    while (gotMsg == FALSE)
      {
        strcpy(record,"\0");
        /* Grab the next message 
        ************************/
        RequestMutex ();
        ret = dequeue (&MsgQueue, record, &msgSize, &reclogo);

        /* Warning: once you call StringToLoc then your string is no
           no longer suitable for another call to it, whether in this
           routine out uploaded to the shared memory and called by
           another module.  So, just make a sacred copy, record2, for
           sending to putmsg if all tests pass.
         ***********************************************************/
        strncpy(record2,record,msgSize);
        ReleaseMutex_ew ();

        if (ret < 0)  /* empty queue */
        {
          sleep_ew (500);
        }
        else  /* for a message from queue */
        {
        /* call conversion to hyp2000arc, then sendit to pipe */
           if (glloc2hyp2k(record2, hypmsg, msgSize)!=EW_SUCCESS){
              logit("et","geqproc: Failed to convert gl to arcmsg\n");
           }else{
             if (pipe_put(hypmsg, (int)TypeHyp2000Arc ) != 0 )
               logit( "et","geqproc: Error writing msg to pipe.\n");
           }
        } /* if ret > 0 */

     } /* while no messages are dequeued */

     ProcessorStatus = 0;
   }
}

int glloc2hyp2k(char *in_msg, char *out_msg, long recsize)
/* get the incoming msg, parse into a loc_global struct,
   convert to the Hypo structs, then pack into an outgoing msg */
{
  int ret;
  EWEventInfoStruct             EventInfo;

  /************************************************************************
   we use GlEvt2EWEvent found in libsrc/util/glevt_2_ewevent.c to 
   first convert the global_loc msg into an EWEvent struct
   int  GlEvt2EWEvent (EWEventInfoStruct *pEWEvent, char *pGlEvt, int GlEvtLen)

   We then use EWEvent2ArcMsg found in libsrc/util/arc_2_ewevent.c
   to convert the EWEvent struct to an arc message.
   int  EWEvent2ArcMsg (EWEventInfoStruct *pEWEvent, char *pArc, int ArcLen)
   ***********************************************************************/

fprintf(stderr,"calling GlEvt2EWEvent\n");
  if (GlEvt2EWEvent (&EventInfo, in_msg, recsize) != EW_SUCCESS) {
     logit ("", "Call to GlEvt2EWEvent failed.\n");
     return(EW_FAILURE);
  }
fprintf(stderr,"calling EWEvent2ArcMsg\n");
  if (EWEvent2ArcMsg (&EventInfo, out_msg, DB_MAX_BYTES_PER_EQ) != EW_SUCCESS) {
     logit ("e", "Call to EWEvent2ArcMsg failed.\n");
     return(EW_FAILURE);
  }
fprintf(stderr,"returning to main\n");
  return(EW_SUCCESS);

}

