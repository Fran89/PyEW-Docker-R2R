/******************************************************************************
 *
 *	File:			activated_scripts.c
 *
 *	Function:		Earthworm module for running scripts upon receipt of an
 *                  ACTIVATE_MODULE message
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			Based on ewspectra.c
 *
 *	Notes:			
 *
 *	Change History:
 *			5/6/11	Started source
 *	
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <earthworm.h>
#include <math.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <ew_spectra_io.h>

#define ACT_VERSION "0.0.3 2012-12-20"

#define MAXTXT 1024
char *Argv0 = "activated_scripts";
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static int MaxArgs = 10;			 /* default max # of arguments to script */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue;              /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
thr_ret Process( void * );

/* Error messages used by export
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_BADMSG        3   /* message w/ bad timespan                 */
#define  ERR_QUEUE         4   /* error queueing message for sending      */
static char  errText[256];     /* string for log/error messages           */

#define backward 0

char *progname;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

MSG_LOGO  GetLogo[1];     			/* requesting module,type,instid */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */
	
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */

static int     HeartBeatInt = 30;        /* seconds between heartbeats        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeComplete;

static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */
static char *SSmsg = NULL;           /* Process's incoming message buffer   */

int static long    MaxMsgSize = 100; /* default max size for input msgs    */
static int     RingSize = 10;        /* max messages in output circular buffer       */

#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport */
static unsigned tidProcess;          /* Processor thread id */

#define NUMREQ 4
char inRing[MAX_RING_STR];			/* name of input ring */
char outRing[MAX_RING_STR];			/* name of output ring */
#define MAX_EXEC 20
char execCmd[MAX_EXEC][MAXTXT];
int nExec = 0;
int maxArg = 0;

/***********************************************************************
 * ReadConfig () - handle config file commands not handled in ws2ts
 *		return 0 if handled, 1 if unknown or an error
 ***********************************************************************/
int ReadConfig(char *configfile) 
{
  char     init[NUMREQ];  /* init flags, one byte for each required command */
  int      nmiss;         /* number of required commands that were missed   */
  char     *com;
  char     *str;
  char     *processor;
  int      nfiles, rc;
  int      i;
  int      err = 0;
  char configPath[MAXTXT], *paramsDir;

  /* Open the main configuration file 
**********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    if ( (paramsDir = getenv("EW_PARAMS")) == NULL)
    {
      fprintf(stderr, "%s: Error opening command file <%s>; EW_PARAMS not set\n", 
              Argv0, configfile);
      return -1;
    }
    strcpy(configPath, paramsDir);
    if (configPath[strlen(configPath)-1] != '/' || 
        configPath[strlen(configPath)-1] != '\\')
      strcat(configPath, "/");
    strcat(configPath, configfile);
    nfiles = k_open (configPath); 
    if (nfiles == 0) 
    {
      fprintf(stderr, "%s: Error opening command file <%s> or <%s>\n", 
              Argv0, configfile, configPath);
      return -1;
    }
  }

  /* Process all command files
***************************/
  while (nfiles > 0)   /* While there are command files open */
  {
    while (k_rd ())        /* Read next line from active file  */
    {  
      com = k_str ();         /* Get the first token from line */

      processor = "ReadConfig";
      
      /* Ignore blank lines & comments
*******************************/
      if (!com)
        continue;
      if (com[0] == '#')
        continue;

      /* Open a nested configuration file */
      if (com[0] == '@') 
      {
        if ( (rc = k_open (&com[1])) == 0)
        {
          fprintf(stderr, "%s: Error opening command file <%s>\n", 
                  Argv0, &com[1]);
          return -1;
        }
        nfiles = rc;
        continue;
      }

	  if (k_its("MyModuleId")) {
		strcpy( MyModName, k_str() );
		if ( GetModId( MyModName, &MyModId ) != 0 ) {
		  logit( "e", "%s: Invalid module name <%s>; exiting!\n",
				   Argv0, MyModName );
		}
		init[0] = 1;
	  } else if (k_its("HeartbeatInt")) {
		HeartBeatInt = k_int();	/* optional, defaults to 30 seconds */
	  } else if (k_its("InRing")) {
		strcpy( inRing, k_str() );
		if ( ( InRingKey = GetKey(inRing) ) == -1 ) {
			logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
					 Argv0, inRing);
			return 1;
		}
		init[1] = 1;
	  } else if (k_its("OutRing")) {
		strcpy( outRing, k_str() );
		if ( ( OutRingKey = GetKey(outRing) ) == -1 ) {
			logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
					 Argv0, outRing);
			return 1;
		}
		init[2] = 1;
	  } else if (k_its("Exec")) {
		if ( nExec == MAX_EXEC ) {
			logit( "e", "%s: Maximum number of <Exec> commands (%d) exceeded; exiting!\n",
					 Argv0, MAX_EXEC);
			return 1;
		}
		str = k_str();
		if ( str == NULL ) {
			logit( "e", "%s: <Exec> command with no argument; ignoring\n",
					 Argv0);
			continue;
		}
		for ( i=0; str[i]; i++ )
			if ( str[i]=='$' ) {
				if ( str[i+1] < '1' || str[i+1] > '9' ) {
					logit( "e", "%s: <Exec> has bad variable ($%c); ignoring\n",
							  Argv0, str[i+1] ? str[i+1] : ' ' );
					break;
				}
				i++;
				if ( str[i]-'0' > maxArg )
					maxArg = str[i]-'0';
			}
		if ( str[i] )
			continue;
		strcpy( execCmd[nExec], str );
		nExec++;
		init[3] = 1;
    } else if( k_its("MaxMsgSize") ) {
		MaxMsgSize = k_long();
    } else if( k_its("MaxScriptArgs") ) {
		MaxArgs = k_long();
    } else if( k_its("RingSize") ) {
		RingSize = k_long();
      } else {
        fprintf(stderr, "ReadConfig: <%s> Unknown command in <%s>.\n", 
                com, configfile);
        continue;
      }

      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        fprintf(stderr, 
                "%s: Bad <%s> command in <%s>\n\t%s\n",
                Argv0, processor, configfile, k_com());
        return -1;
      }

    } /** while k_rd() **/

    nfiles = k_close ();

  } /** while nfiles **/

  
  /* After all files are closed, check init flags for missed commands */
  nmiss = 0;
  for (i = 0; i < NUMREQ; i++)  
    if (!init[i]) 
      nmiss++;

  if (nmiss) 
  {
    fprintf(stderr, "gma: ERROR, no ");
    if (!init[0])  fprintf(stderr, "<MyModuleId> ");
    if (!init[1])  fprintf(stderr, "<InRing> ");
    if (!init[2])  fprintf(stderr, "<OutRing> ");
    if (!init[3])  fprintf(stderr, "<Exec> ");
    
    fprintf(stderr, "command(s) in <%s>; exitting!\n", configfile);
    return -1;
  }

  return err;
}

/***************************************************************************
 * actscr_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void actscr_status( unsigned char type, short ierr, char *note )
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

   time( &t );

   if( type == TypeHeartBeat )
    sprintf( msg, "%ld %ld\n%c", (long) t, (long) MyPid, (char)0);
   else if( type == TypeError )
   {
    sprintf( msg, "%ld %hd %s\n%c", (long) t, ierr, note, 0);

    logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","%s(%s):  Error sending heartbeat.\n",
                  Argv0, MyModName );
    }
    else if( type == TypeError ) {
           logit("et", "%s(%s):  Error sending error:%d.\n",
                  Argv0, MyModName, ierr );
    }
   }

   return;
}

/************************  Main Process Thread   *********************
*          Pull a messsage from the queue, parse & run scripts       *
**********************************************************************/

thr_ret Process( void *dummy )
{
   int      ret;
   long     msgSize;
   char     *msgName;
   MSG_LOGO Logo, outLogo; /* logos of retrieved message & result message */
   time_t   start, finish;
   char **argv = malloc( (MaxArgs+1)*sizeof(char*) );
   
   if ( argv == NULL ) {
   	logit( "et", "%s(%s): Could not allocate space for script arguments; exiting!\n",
   		Argv0, MyModName );
   	exit(1);
   }

   while (1)   /* main loop */
   {
      char *inputstring;
      char msgModId[20];
      int numArgs = 0;
      int i, rc;
      char startDate[20], finishDate[20];
      char msg[256];

     /* Get message from queue
      *************************/
     RequestMutex();
     ret=dequeue( &OutQueue, SSmsg, &msgSize, &Logo);
     ReleaseMutex_ew();
     if(ret < 0 )
     { /* -1 means empty queue */
       sleep_ew(500); /* wait a bit (changed from 1000 to 500 on 970624:ldd) */
       continue;
     }

	   time( &start );

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/      
      inputstring = SSmsg;
	  inputstring[msgSize] = 0;
	  
	  argv[0] = strtok( inputstring, " \t" );
	  if ( argv[0] == NULL )
	  	continue;
	  strcpy( msgModId, argv[0] );
	  argv[numArgs] = strtok( NULL, " \t" ); 
	  while ( argv[numArgs] ) {
	  	if ( numArgs >= MaxArgs )
	  		break;
	  	argv[++numArgs] = strtok( NULL, " \t" );
	  }
      msgName = "ACTIVATE_MODULE";

				
	  if ( numArgs < maxArg ) {
			sprintf( errText, "insufficient arguments in %s msg i%d m%d t%d in %s",msgName,
					(int) Logo.instid,
					(int) Logo.mod, (int)Logo.type, inRing );
			actscr_status( TypeError, ERR_BADMSG, errText );
			continue;
	  } else if ( numArgs >= MaxArgs ) {
			sprintf( errText, "too many arguments in %s msg i%d m%d t%d in %s",msgName,
					(int) Logo.instid,
					(int) Logo.mod, (int)Logo.type, inRing );
			actscr_status( TypeError, ERR_BADMSG, errText );
			continue;
	  }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/

	  for ( i=0; i<nExec; i++ ) {
	  	char cmd[MAXTXT], *orig = execCmd[i];
	  	int j=0;
	  	
	  	for ( ; *orig; orig++ )
	  		if ( *orig == '$' ) {
	  			char *arg = argv[*(orig+1)-'1'];
	  			for ( ; *arg; )
	  				cmd[j++] = *(arg++);
	  			orig++;
	  		} else
	  			cmd[j++] = *orig;
	  	cmd[j] = '\0';
	  	
	  	logit( "t", "%s(%s): Executing '%s'\n", Argv0, MyModName, cmd );
	  	rc = system( cmd );
	  }


	 /* Send completion message
	  *******************/
	   time( &finish );

	   outLogo.instid = InstId;
	   outLogo.mod    = MyModId;
	   outLogo.type   = TypeComplete;
	
	   EWSUnConvertTime( startDate, start );
	   EWSUnConvertTime( finishDate, finish );
	   
	   sprintf( msg, "Received on %s, from module %d, done at %s with return code %d\n%c",
			startDate, Logo.mod, finishDate, rc, 0 );
	
	   if( tport_putmsg( &OutRegion, &outLogo, strlen(msg)+1, msg ) != PUT_OK )
	   {
			logit("et","%s(%s):  Error sending completion message.\n",
					  Argv0, MyModName );
	   }
   }   /* End of main loop */

}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   int       res;
   int       ret;
   char      *msgName;
   int modid, okMsg;
   int           NumOfTimesQueueLapped= 0; /* number of messages lost due to
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
      res = tport_getmsg( &InRegion, GetLogo, 1,
                          &reclogo, &recsize, MSrawmsg, MaxMsgSize );

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
            actscr_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, inRing );
            actscr_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     inRing );
            actscr_status( TypeError, ERR_NOTRACK, errText );
         }
      }
      
      res = sscanf( MSrawmsg, "%d", &modid );
      
      msgName = "ACTIVATE_MODULE";
      okMsg = (res == 1);
	  if ( !okMsg ) {
			sprintf( errText, "malformed %s msg i%d m%d t%d in %s",msgName,
					(int) reclogo.instid,
					(int) reclogo.mod, (int)reclogo.type, inRing );
			actscr_status( TypeError, ERR_BADMSG, errText );
			continue;
      }
      if ( modid != MyModId )
      	continue;

      RequestMutex();
      ret=enqueue( &OutQueue, MSrawmsg, recsize, reclogo );
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
         if (ret==-2)  /* Serious: quit */
         {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
        sprintf(errText,"internal queue error. Terminating.");
            actscr_status( TypeError, ERR_QUEUE, errText );
        goto error;
         }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            actscr_status( TypeError, ERR_QUEUE, errText );
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
error:
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */
   return(NULL); /* should never reach here */
}

int main(int argc, char **argv)
{
/* Other variables: */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */
	
	/* Look up installations of interest
	*********************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
	  fprintf( stderr,
			  "%s: error getting local installation id; exiting!\n",
			   Argv0 );
	  exit( -1 );
	}
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_ACTIVATE_COMPLETE", &TypeComplete ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ACTIVATE_COMPLETE>; exiting!\n", Argv0 );
      exit( -1 );
   }

	if ((progname= strrchr(*argv, (int) '/')) != (char *) 0)
		progname++;
	else
		progname= *argv;
	
    logit_init (progname, 0, 1024, 1);

    /* Check command line arguments */
    if (argc != 2) {
		fprintf (stderr, "Usage: %s <configfile>\n", progname);
		fprintf (stderr, "Version: %s\n", ACT_VERSION);
		return EW_FAILURE;
    }
    
    
    	
	/* Get our own Pid for restart purposes
   	***************************************/
   	MyPid = getpid();
   	if(MyPid == -1)
   	{
      logit("e", "%s: Cannot get pid; exiting!\n", Argv0);
      return(0);
   	}

	/* Read-in & interpret config file */
	InRingKey = OutRingKey = -1;
	ReadConfig( argv[1] );
	
	if ( 1 ) {
	   	int geti = GetInst( "INST_WILDCARD", &(GetLogo[0].instid) );
	   	int getm = GetModId( "MOD_WILDCARD", &(GetLogo[0].mod) );
	   	int gett = GetType( "TYPE_ACTIVATE_MODULE", &(GetLogo[0].type) );
	   	int getMsg = 0;
		if ( ( MSrawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
		{
		  logit( "e", "%s: error allocating MSrawmsg; exiting!\n",
				 Argv0 );
		  getMsg = 1;
		}
	   	if ( geti || getm || gett || getMsg ) {
	   		if ( geti )
	   			logit( "e", "%s: INST_WILDCARD unknown; exiting!\n", Argv0 );
	   		if ( getm )
	   			logit( "e", "%s: MOD_WILDCARD unknown; exiting!\n", Argv0 );
	   		if ( gett )
	   			logit( "e", "%s: TYPE_ACTIVATE_MODULE unknown; exiting!\n", Argv0 );
	   		exit(1);
	   	}
		tport_attach( &InRegion, InRingKey );		
	}
	tport_attach( &OutRegion, OutRingKey );

	if ( 1 ) {

	   /* step over all messages from transport ring
	   *********************************************/
	   /* As Lynn pointed out: if we're restarted by startstop after hanging,
		  we should throw away any of our messages in the transport ring.
		  Else we could end up re-sending a previously sent message, causing
		  time to go backwards... */
	   do
	   {
		 res = tport_getmsg( &InRegion, GetLogo, 1,
							 &reclogo, &recsize, MSrawmsg, MaxMsgSize );
	   } while (res !=GET_NONE);
	
	   /* One heartbeat to announce ourselves to statmgr
	   ************************************************/
	   actscr_status( TypeHeartBeat, 0, "" );
	   time(&MyLastBeat);
	
	
	   /* Start the message stacking thread if it isn't already running.
		****************************************************************/
	   if (MessageStackerStatus != MSGSTK_ALIVE )
	   {
		 if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
		 {
		   logit( "e",
				  "%s(%s): Error starting  MessageStacker thread; exiting!\n",
			  Argv0, MyModName );
		   tport_detach( &InRegion );
		   tport_detach( &OutRegion );
		   return( -1 );
		 }
		 MessageStackerStatus = MSGSTK_ALIVE;
	   }
	

	   /* Buffers for Process thread: */
	   if ( ( SSmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
	   {
		  logit( "e", "%s(%s): error allocating SSmsg; exiting!\n",
				  Argv0, MyModName );
				tport_detach( &InRegion );
				tport_detach( &OutRegion );
				exit(1);
	   }
	
	   /* Create a Mutex to control access to queue
	   ********************************************/
	   CreateMutex_ew();
	
	   /* Initialize the message queue
	   *******************************/
	   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );
	
	   /* Start the socket writing thread
	   ***********************************/
	   if ( StartThread(  Process, (unsigned)THREAD_STACK, &tidProcess ) == -1 )
	   {
		  logit( "e", "%s(%s): Error starting Process thread; exiting!\n",
				  Argv0, MyModName );
		  tport_detach( &InRegion );
		  tport_detach( &OutRegion );
		  free( SSmsg );
		  exit( -1 );
	   }
	
	   /* Start main ringdup service loop
	   **********************************/
	   while( tport_getflag( &InRegion ) != TERMINATE  &&
			  tport_getflag( &InRegion ) != MyPid         )
	   {
		 /* Beat the heart into the transport ring
		  ****************************************/
		  time(&now);
		  if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
		  {
			  actscr_status( TypeHeartBeat, 0, "" );
		  time(&MyLastBeat);
		  }
	
		  /* take a brief nap; added 970624:ldd
		   ************************************/
		  sleep_ew(500);
	   } /*end while of monitoring loop */
		
	}
		
	/* Clean up after ourselves */
	tport_detach( &InRegion );
	tport_detach( &OutRegion );
	return(0);
}
