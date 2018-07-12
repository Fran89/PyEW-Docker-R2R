/*****************************************************************
 * ewmseedarchiver.c
 *
 * Program to read (selected) MSEED messages from a transport ring
 * and write the miniSEED records to the file system.
 *
 * by Chad Trabant
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <swap.h>
#include <trace_buf.h>
#include <libmseed.h>

#include "exportfilter.h"
#include "dsarchive.h"

#define PACKAGE "ewmseedarchiver"
#define PROGRAM_VERSION "0.3 2015-03-04"

static int archiverecord (char *record, int reclen);
static thr_ret MessageStacker (void *); /* Read messages and add to queue */

static void config (char *);
static void lookup (void);
static void logstatus (unsigned char, short, char *);
static void freelocal (void);
static int  addarchive (const char *path, const char *layout);

/* Archive output structure definition containers */
typedef struct Archive_s {
  DataStream  datastream;
  struct Archive_s *next;
} Archive;

/* Thread things */
#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE MsgQueue;                      /* from queue.h, queue.c; sets up linked    */

/* Message Buffers to be allocated */
static char *Rawmsg = NULL;          /* "raw" retrieved message                  */
static char *Filteredmsg = NULL;     /* MessageStacker's "filtered" message      */
static char *Qmsg = NULL;            /* message from queue                       */
static MSG_LOGO Logo;                /* logo of message to re-send               */

/* Timers */
time_t now;                          /* current time, used for timing heartbeats */
time_t MyLastBeat;                   /* time of last local (into Earthworm) hearbeat */

extern int errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

char *Argv0;        /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */

/* Things to read or derive from configuration file */
static char    RingName[MAX_RING_STR];  /* name of transport ring for input  */
static char    MyModName[MAX_MOD_STR];  /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     QueueSize;           /* max messages in output circular buffer */

/* Things to look up in the earthworm.h tables with getutil.c functions */
static long          RingNameKey;   /* key of transport ring for input    */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeMseed;

/* Error messages used by export */
#define  ERR_MISSMSG     0     /* message missed in transport ring        */
#define  ERR_TOOBIG      1     /* retreived msg too large for buffer      */
#define  ERR_NOTRACK     2     /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE       3     /* error queueing message for sending      */
static char  errText[256];     /* string for log/error messages           */

static int verbose     = 0;

static Archive *archiveroot = 0;    /* Output file structures */

int
main ( int argc, char **argv )
{
  int        res;
  long       recsize;   /* size of retrieved message             */
  MSG_LOGO   reclogo;   /* logo of retrieved message             */
  
  int        ret;
  long       msgSize;
  int        count;
  
  /* Check command line arguments */
  Argv0 = argv[0];
  if ( argc != 2 )
    {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
      fprintf( stderr, "Version: V%s\n", PROGRAM_VERSION );
      exit( 0 );
    }
  
  /* Initialize name of log-file & open it */
  logit_init( argv[1], 0, 512, 1 );

  /* Read the configuration file(s) */
  config( argv[1] );
  logit( "et" , "%s(%s): Read command file <%s>\n Version %s\n",
	 Argv0, MyModName, argv[1], PROGRAM_VERSION );
  
  /* Look up important info from earthworm.h tables */
  lookup();
  
  /* Reinitialize the logging level */
  logit_init( argv[1], 0, 512, LogSwitch );

  /* Get our own Pid for restart purposes */
  MyPid = getpid();
  if ( MyPid == -1 )
    {
      logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
      exit(0);
    }
  
  /* Initialize export filter */
  if ( exportfilter_init() != 0 )
    {
      logit("e", "%s(%s): Error in exportfilter_init(); exiting!\n",
            Argv0, MyModName );
      exit(-1);
    }
  
  /* Allocate space for input/output messages for all threads */
  if ( ( Rawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
    {
      logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n",
             Argv0, MyModName );
      freelocal();
      exit(-1);
    }
  
  /* Buffers for message handling loop */
  if ( ( Qmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
    {
      logit( "e", "%s(%s): error allocating Qmsg; exiting!\n",
	     Argv0, MyModName );
      freelocal();
      exit(-1);
    }
  
  /* Buffers for the MessageStacker thread: */
  if ( ( Filteredmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
    {
      logit( "e", "%s(%s): error allocating Filteredmsg; exiting!\n",
	     Argv0, MyModName );
      freelocal();
      exit (-1);
    }
  
  /* Create a Mutex to control access to queue */
  CreateMutex_ew();
  
  /* Initialize the message queue */
  initqueue( &MsgQueue, (unsigned long)QueueSize,(unsigned long)MaxMsgSize+1 );
    
  /* Attach to Input shared memory ring */
  tport_attach( &InRegion, RingNameKey );
  
  /* step over all messages from transport ring */
  /* As Lynn pointed out: if we're restarted by startstop after hanging,
     we should throw away any of our messages in the transport ring.
     Else we could end up re-sending a previously sent message, causing
     time to go backwards... */
  do
    {
      res = tport_getmsg( &InRegion, GetLogo, nLogo,
			  &reclogo, &recsize, Rawmsg, MaxMsgSize );
    } while (res != GET_NONE);
  
  /* One heartbeat to announce ourselves to statmgr */
  logstatus( TypeHeartBeat, 0, "" );
  time(&MyLastBeat);
  
  /* Start the message stacking thread if it isn't already running. */
  if ( MessageStackerStatus != MSGSTK_ALIVE )
    {
      if ( StartThread( MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
	{
	  logit( "e",
		 "%s(%s): Error starting  MessageStacker thread; exiting!\n",
		 Argv0, MyModName );
	  tport_detach( &InRegion );
	  freelocal();
	  exit(-1);
	}
      MessageStackerStatus = MSGSTK_ALIVE;
    }
  
  /* Start main service loop */
  while ( tport_getflag( &InRegion ) != TERMINATE &&
          tport_getflag( &InRegion ) != MyPid )
    {
      /* Beat the heart into the transport ring */
      time(&now);
      if ( difftime(now,MyLastBeat) > (double)HeartBeatInt )
	{
	  logstatus ( TypeHeartBeat, 0, "" );
	  MyLastBeat = now;
	}
      
      /* Process up to 10 messages, then let outer loop have a pass */
      count = 0;
      while ( count < 10 )
	{
	  /* Get message from queue */
	  RequestMutex();
	  ret = dequeue (&MsgQueue, Qmsg, &msgSize, &Logo);
	  ReleaseMutex_ew();
	  
	  /* If queue is empty (ret=-1), sleep 1/2 second and break to outer loop */
	  if ( ret < 0 )
	    {
	      sleep_ew(500);
	      break;
	    }
	  
	  /* Archive miniSEED record */
	  if ( Logo.type == TypeMseed )
	    {
	      if ( archiverecord (Qmsg, msgSize) )
		{
		  logit ("et", "Error archiving record\n");
		}
	    }
	  else if ( verbose )
	    {
	      logit ("t", "Unrecognized message type (%d), skipping\n", Logo.type);
	    }
	  
	  count++;
	}
    } /* End of main service loop */
  
  /* Shut down */
  
  /* Flush message queue */
  while ( 1 )
    {
      RequestMutex();
      ret = dequeue (&MsgQueue, Qmsg, &msgSize, &Logo);
      ReleaseMutex_ew();
      
      /* If queue is empty (ret=-1) we are done */
      if ( ret < 0 )
	break;
      
      /* Archive miniSEED record */
      if ( Logo.type == TypeMseed )
	{
	  if ( archiverecord (Qmsg, msgSize) )
	    {
	      logit ("et", "Error archiving record\n");
	    }
	}
    }
  
  /* Free memory and close files for all Archive definitions */
  if ( archiveroot )
    {
      Archive *arch;
      
      arch = archiveroot;
      while ( arch )
	{
	  ds_streamproc (&arch->datastream, NULL, 0, verbose-1);
	  arch = arch->next;
	}
    }
  
  tport_detach( &InRegion );
  
  exportfilter_shutdown();
  
  freelocal();
  
  logit ("t", "%s(%s): termination requested; exiting!\n", Argv0, MyModName );
  
  return 0;
}  /* End of main() */


/******************************************************************
 * archiverecord:
 *
 * Write the supplied miniSEED record to all Archives.
 *
 * Returns 0 on success and non-zero otherwise.
 ******************************************************************/
static int
archiverecord ( char *record, int reclen )
{
  static MSRecord *msr = NULL;
  Archive *arch;
  int suffix = 0;
  int retval;
  
  retval = msr_unpack (record, reclen, &msr, 0, verbose);
  if ( retval != MS_NOERROR )
    {
      ms_log (2, "Cannot unpack Mini-SEED: %s\n", ms_errorstr(retval));
      
      return 1;
    }
  
  arch = archiveroot;
  while ( arch )
    {
      ds_streamproc (&arch->datastream, msr, suffix, verbose-1);
      arch = arch->next;
    }
  
  return 0;
}  /* End of archiverecord() */


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
static thr_ret
MessageStacker ( void *dummy )
{
   long      recsize;       /* size of retrieved message             */
   MSG_LOGO  reclogo;       /* logo of retrieved message             */
   long      filteredSize;  /* size of message after user-filtering  */
   unsigned char filteredType;  /* type of message after filtering   */
   int       res;
   int       ret;
   int       NumOfTimesQueueLapped= 0; /* number of messages lost due to
                                             queue lap */

   /* Tell the main thread we're ok
   ********************************/
   MessageStackerStatus = MSGSTK_ALIVE;

   /* Start main export service loop for current connection
   ********************************************************/
   while( 1 )
   {
      /* Get a message from transport ring
      ************************************/
      res = tport_getmsg( &InRegion, GetLogo, nLogo,
                          &reclogo, &recsize, Rawmsg, MaxMsgSize );

      /* Wait if no messages for us
       ****************************/
      if( res == GET_NONE ) {sleep_ew(100); continue;}

      /* Check return code; report errors
      ***********************************/
      if( res != GET_OK )
      {
         if( res==GET_TOOBIG )
         {
            sprintf( errText, "msg[%ld] i%d m%d t%d too long for target",
                            recsize, (int) reclogo.instid,
                (int) reclogo.mod, (int)reclogo.type );
            logstatus( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, RingName );
            logstatus( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     RingName );
            logstatus( TypeError, ERR_NOTRACK, errText );
         }
      }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/
      Rawmsg[recsize] = '\0';
      /* pass it through the filter routine: this may reformat,
         or reject as it chooses. */
      if ( exportfilter( Rawmsg, recsize, reclogo.type, &Filteredmsg,
                        &filteredSize, &filteredType ) == 0 )  continue;
      reclogo.type = filteredType;  /* note the new message type */
      
      /* put it into the 'to be shipped' queue */
      /* the main thread is in the biz of de-queueng and handling */
      RequestMutex();
      ret = enqueue (&MsgQueue, Filteredmsg, filteredSize, reclogo);
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
	 if (ret==-2)  /* Serious: quit */
	 {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
	   sprintf(errText,"internal queue error. Terminating.");
	   logstatus( TypeError, ERR_QUEUE, errText );
	   
	   break;
	 }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            logstatus( TypeError, ERR_QUEUE, errText );
            continue;
         }
         if (ret==-3)  /* Log only while client's connected */
         {
         /* Queue is lapped too often to be logged to screen.
          * Log circular queue laps to logfile.
          * Maybe queue laps should not be logged at all.
          */
            NumOfTimesQueueLapped++;
            if (!(NumOfTimesQueueLapped % 5))
            {
               logit("t",
                     "%s(%s): Circular queue lapped 5 times. Messages lost.\n",
                      Argv0, MyModName);
               if (!(NumOfTimesQueueLapped % 100))
               {
                  logit( "et",
                        "%s(%s): Circular queue lapped 100 times. Messages lost.\n",
                         Argv0, MyModName);
               }
            }
            continue;
         }
      }
   } /* end of while */

   /* we're quitting
   *****************/
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */

#ifndef _WINNT     /* function return type is 'void' under Windows */
   return NULL;
#endif
}


/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.               *
 *****************************************************************************/
static void
config ( char *configfile )
{
  int      ncommand;     /* # of required commands you expect to process   */
  char     init[10];     /* init flags, one byte for each required command */
  int      nmiss;        /* number of required commands that were missed   */
  char    *com;
  char     processor[20];
  int      nfiles;
  int      success;
  int      i;
  char*    str;

  /* Set to zero one init flag for each required command */
  ncommand = 9;
  for ( i=0; i < ncommand; i++ )  init[i] = 0;
  nLogo = 0;
  
  /* Open the main configuration file */
  nfiles = k_open( configfile );
  if ( nfiles == 0 )
    {
      logit( "e", "%s: Error opening command file <%s>; exiting!\n",
	     Argv0, configfile );
      exit(-1);
    }
  
  /* Process all command files */
  while ( nfiles > 0 )     /* While there are command files open */
    {
      while ( k_rd() )     /* Read next line from active file */
        {
	  com = k_str();   /* Get the first token from line */
	  
	  /* Ignore blank lines & comments */
	  if ( !com )           continue;
	  if ( com[0] == '#' )  continue;
	  
	  /* Open a nested configuration file */
	  if ( com[0] == '@' )
	    {
	      success = nfiles+1;
	      nfiles  = k_open (&com[1]);
	      if ( nfiles != success )
		{
                  logit( "e", "%s: Error opening command file <%s>; exiting!\n",
			 Argv0, &com[1] );
                  exit(-1);
		}
	      continue;
            }
	  strcpy ( processor, "config" );
	  
	  /* Process anything else as a command */
  /*0*/   if ( k_its("LogFile") )
	    {
	      LogSwitch = k_int();
	      init[0] = 1;
            }
  /*1*/   else if ( k_its("MyModuleId") )
	    {
	      str = k_str();
	      if (str) strcpy ( MyModName, str );
	      init[1] = 1;
            }
  /*2*/   else if ( k_its("RingName") )
	    {
	      str = k_str();
	      if (str) strcpy ( RingName, str );
	      init[2] = 1;
            }
  /*3*/   else if ( k_its("MaxOpenFiles") )
	    {
	      ds_maxopenfiles = k_long();
	      init[3] = 1;
            }
  /*4*/   else if ( k_its("HeartBeatInt") )
	    {
	      HeartBeatInt = k_int();
	      init[4] = 1;
            }
	  
	  /* Enter installation & module & message types to get */
  /*5*/   else if ( k_its("GetMsgLogo") )
	    {
	      if ( nLogo >= MAXLOGO )
		{
		  logit( "e",
			 "%s: Too many <GetMsgLogo> commands in <%s>", Argv0, configfile );
		  logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO );
		  exit(-1);
                }
	      if ( ( str=k_str() ) )
		{
		  if ( GetInst( str, &GetLogo[nLogo].instid ) != 0 )
		    {
		      logit( "e",
			     "%s: Invalid installation name <%s>", Argv0, str );
		      logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
		      exit(-1);
		    }
                }
	      if ( ( str=k_str() ) )
		{
		  if ( GetModId( str, &GetLogo[nLogo].mod ) != 0 )
		    {
		      logit( "e",
			     "%s: Invalid module name <%s>", Argv0, str );
		      logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
		      exit(-1);
		    }
                }
	      if ( ( str=k_str() ) )
		{
		  if ( GetType( str, &GetLogo[nLogo].type ) != 0 )
		    {
		      logit( "e",
			     "%s: Invalid msgtype <%s>", Argv0, str );
		      logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
		      exit(-1);
		    }
                }
	      nLogo++;
	      init[5] = 1;
            }
	  
	  /* Maximum size (bytes) for incoming/outgoing messages */
  /*6*/   else if ( k_its("MaxMsgSize") )
	    {
	      MaxMsgSize = k_long();
	      init[6] = 1;
            }

	  /* Maximum number of messages in outgoing circular buffer */
  /*7*/   else if ( k_its("QueueSize") )
	    {
	      QueueSize = k_long();
	      init[7] = 1;
            }
	  
  /*8*/   else if ( k_its("Archive") )
	    {
	      if ( ( str=k_str() ) )
		{
		  if ( addarchive(str, NULL) == -1 )
		    exit( -1 );
		  
		  init[8] = 1;
		}
            }
	  
	  /* Optional options */
	  else if ( k_its("Verbosity") )
	    {
	      verbose = k_long();
            }
	  
	  /* Pass it off to the filter's config processor */
	  else if ( exportfilter_com() )
	    strcpy ( processor, "exportfilter_com" );

	  /* Unknown command */
	  else
	    {
	      logit( "e", "%s: <%s> Unknown command in <%s>.\n",
		     Argv0, com, configfile );
	      continue;
            }
	  
	  /* See if there were any errors processing the command */
	  if ( k_err() )
	    {
	      logit( "e", "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
		     Argv0, com, processor, configfile );
	      exit( -1 );
            }
	}
      
      nfiles = k_close();
    }
  
  /* After all files are closed, check init flags for missed commands */
  nmiss = 0;
  for ( i=0; i < ncommand; i++ ) if ( !init[i] ) nmiss++;
  if ( nmiss )
    {
      logit( "e", "%s: ERROR, no ", Argv0 );
      if ( !init[0] )  logit( "e", "<LogFile> "      );
      if ( !init[1] )  logit( "e", "<MyModuleId> "   );
      if ( !init[2] )  logit( "e", "<RingName> "     );
      if ( !init[3] )  logit( "e", "<MaxOpenFiles> " );
      if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
      if ( !init[5] )  logit( "e", "<GetMsgLogo> "   );
      if ( !init[6] )  logit( "e", "<MaxMsgSize> "   );
      if ( !init[7] )  logit( "e", "<QueueSize> "    );
      if ( !init[8] )  logit( "e", "<Archive> "      );
      logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
      exit( -1 );
    }
  
  return;
}  /* End of config() */


/****************************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
static void
lookup ( void )
{
  /* Look up keys to shared memory regions */
  if ( ( RingNameKey = GetKey(RingName) ) == -1 )
    {
      fprintf ( stderr,
		"%s:  Invalid ring name <%s>; exiting!\n", Argv0, RingName );
      exit(-1);
    }
  
  /* Look up installations of interest */
  if ( GetLocalInst( &InstId ) != 0 )
    {
      fprintf ( stderr,
		"%s: error getting local installation id; exiting!\n", Argv0 );
      exit(-1);
    }
  
  /* Look up modules of interest */
  if ( GetModId( MyModName, &MyModId ) != 0 )
    {
      fprintf ( stderr,
		"%s: Invalid module name <%s>; exiting!\n", Argv0, MyModName );
      exit(-1);
    }
  
  /* Look up message types of interest */
  if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
    {
      fprintf ( stderr,
		"%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit(-1);
    }
  if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
    {
      fprintf ( stderr,
		"%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
      exit(-1);
    }
  if ( GetType( "TYPE_MSEED", &TypeMseed ) != 0 )
    {
      fprintf ( stderr,
		"%s: Critical message type <TYPE_MSEED> is missing; exiting!\n", Argv0 );
      exit(-1);
    }
  
  return;
}


/***************************************************************************
 * logstatus() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
static void
logstatus ( unsigned char type, short ierr, char *note )
{
  MSG_LOGO    logo;
  char        msg[256];
  long        size;
  time_t      t;

  /* Build the message */
  logo.instid = InstId;
  logo.mod    = MyModId;
  logo.type   = type;
  
  time ( &t );

  if ( type == TypeHeartBeat )
    sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
  else if ( type == TypeError )
    {
      sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
      
      logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
    }
  
  size = strlen( msg );   /* don't include the null byte in the message */

  /* Write the message to shared memory */
  if ( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK )
    {
      if ( type == TypeHeartBeat )
	{
	  logit ("et","%s(%s):  Error sending heartbeat.\n",
		Argv0, MyModName );
      }
      else if ( type == TypeError )
	{
	  logit ("et", "%s(%s):  Error sending error:%d.\n",
		 Argv0, MyModName, ierr );
      }
    }
  
  return;
}


/***************************************************************************
 * freelocal()  free all previously allocated memory                      *
 ***************************************************************************/
static void
freelocal ( void )
{
   free (Rawmsg);               /* "raw" retrieved message   */
   free (Qmsg);                 /* Incoming message buffer   */
   free (Filteredmsg);          /* MessageStacker's "filtered" message to  */
                                /*  be sent to destination                 */
   return;
}


/***************************************************************************
 * addarchive:
 * Add entry to the data stream archive chain.  'layout' if defined
 * will be appended to 'path'.
 *
 * Returns 0 on success, and -1 on failure
 ***************************************************************************/
static int
addarchive ( const char *path, const char *layout )
{
  Archive *newarch;
  int pathlayout;
#ifdef _WINNT
  char *cptr;
#endif
  
  if ( ! path )
    {
      ms_log (2, "addarchive(): cannot add archive with empty path\n");
      return -1;
    }
  
  if ( ! (newarch = (Archive *) malloc (sizeof (Archive))) )
    {
      ms_log (2, "addarchive(): cannot allocate memory for new archive definition\n");
      return -1;
    }
  
  /* Setup new entry and add it to the front of the chain */
  pathlayout = strlen (path) + 2;
  if ( layout )
    pathlayout += strlen (layout);

  if ( ! (newarch->datastream.path = (char *) malloc (pathlayout)) )
    {
      ms_log (2, "addarchive(): cannot allocate memory for new archive path\n");
      if ( newarch )
        free (newarch);
      return -1;
    }
  
  if ( layout )
    snprintf (newarch->datastream.path, pathlayout, "%s/%s", path, layout);
  else
    snprintf (newarch->datastream.path, pathlayout, "%s", path);
  
#ifdef _WINNT                          /* if Windows then */
  cptr = newarch->datastream.path;     /*  replace all backslashes */
  while(*cptr != '\0')                 /*  with forward slashes */
    {
      if(*cptr == '\\')
        *cptr = '/';
      ++cptr;
    }
#endif

  newarch->datastream.idletimeout = 60;
  newarch->datastream.grouproot = NULL;
  
  newarch->next = archiveroot;
  archiveroot = newarch;
  
  return 0;
}  /* End of addarchive() */
