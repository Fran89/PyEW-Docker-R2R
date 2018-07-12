/******************************************************************************
 *
 *	File:			ewnotify.c
 *
 *	Function:		Earthworm module for acting upon receipt of an
 *                  ACTIVATE_MODULE, THRESH_ALERT, ACCEL_ALERT, or SHEAR_ALERT message;
 *                  respose includes sending email message(s) and posting
 *                  an ACTIVATE_MODULE message for the activated_scripts
 *                  module.
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			Based on activated_scripts.c
 *
 *	Notes:			
 *
 *	Change History:
 *			5/19/11	Started source
 *	
 *****************************************************************************/

#define VERSION "0.1 2016-09-28"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <earthworm.h>
#include <math.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <ew_spectra_io.h>

#define MAXTXT 1024
char *Argv0 = "ewnotify";
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static char    ASModName[MAX_MOD_STR];       /* speak as activated_scripts module name/id      */
#define MAX_ARGS_PLUS_1 10

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
#define PROCESS_OFF    0              /* Process thread has not been started      */
#define PROCESS_ALIVE  1              /* Process thread alive and well            */
#define PROCESS_ERR   -1              /* Process thread encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;
volatile int ProcessStatus = PROCESS_OFF;

QUEUE OutQueue;              /* from queue.h, queue.c; sets up linked    */
                                     /*    list via calloc and free              */
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
static unsigned char ASModId;       /* Module Id for activated_scripts    */

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

int		num_logos;
MSG_LOGO  GetLogo[5];     	/* requesting module,type,instid */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */
	
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */

static int     HeartBeatInt = 30;        /* seconds between heartbeats        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */
static char *SSmsg = NULL;           /* Process's incoming message buffer   */
static MSG_LOGO Logo;        /* logo of message to re-send */

static long    MaxMsgSize = 1000;          /* DEFAULT max size for input msgs    */
static int     RingSize = 10;        /* max messages in output circular buffer       */

#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport */
static unsigned tidProcess;          /* Processor thread id */

char inRing[MAX_RING_STR];			/* name of input ring */
char outRing[MAX_RING_STR];			/* name of output ring */
#define MAX_RECIP 60
char *recip = NULL;
int nRecip = 0, nRecipAlloc = 5;
char *alertSubject = NULL;
char *testSubject = NULL;
char *mailServer = NULL;
char *mailProg = NULL;
char *sender = NULL;
int DebugInfo = 0;

#ifdef _WINNT
#define NUMREQ 9
/* Handle deprecation of strdup in windows */
static char* mystrdup( const char* src ) {
	char* dest = calloc( 1, strlen(src) + 1 );
	if ( dest != NULL )
		strcpy( dest, src );
	return dest;
}
#else
#define NUMREQ 8
#define mystrdup strdup
#endif

static char* skipFirstToken( char *str ) {
	while ( *str != 0 && isspace(*str) )
		str++;
	while ( *str != 0 && !isspace(*str) )
		str++;
	while ( *str != 0 && isspace(*str) )
		str++;
	return str;
}

/***********************************************************************
 * ReadConfig () - handle config file commands not handled in ws2ts
 *		return 0 if handled, 1 if unknown or an error
 ***********************************************************************/
int ReadConfig(char *configfile) 
{
  char     init[10];  	/* init flags, one byte for each required command */
  int      nmiss;       /* number of required commands that were missed   */
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
	  } else if (k_its("InRing")) {
		strcpy( inRing, k_str() );
		if ( ( InRingKey = GetKey(inRing) ) == -1 ) {
			logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
					 Argv0, inRing);
			return 1;
		}
		init[1] = 1;
	  } else if (k_its("HeartbeatInt")) {
		/* optional, but set to 30 seconds by default */
                HeartBeatInt = k_int();
	  } else if (k_its("OutRing")) {
		strcpy( outRing, k_str() );
		if ( ( OutRingKey = GetKey(outRing) ) == -1 ) {
			logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
					 Argv0, outRing);
			return 1;
		}
		init[2] = 1;
	  } else if (k_its("SendMailTo")) {
		str = skipFirstToken( k_com() );
		if ( str == NULL || *str==0) {
			logit( "e", "%s: <SendMailTo> command with no argument; ignoring\n",
					 Argv0);
			continue;
		}
		if ( recip == NULL || nRecip == nRecipAlloc ) {
			nRecipAlloc *= 2;
			recip = (char*)realloc( recip, 60 * nRecipAlloc );
			if ( recip == NULL ) {
				logit( "e", "%s: Failed to allocate memory for mail recipient '%s'; exiting\n",
					Argv0, str );
				return 1;
			}
		}
		strncpy( recip+60*nRecip, str, 60 );
		nRecip++;
		recip[nRecip*60-1] = 0;
		init[3] = 1;
      } else if( k_its("ScriptModule") ) {
		strcpy( ASModName, k_str() );
		if ( GetModId( ASModName, &ASModId ) != 0 ) {
		  logit( "e", "%s: Invalid module name <%s>; exiting!\n",
				   Argv0, ASModName );
		}
		init[4] = 1;
      } else if( k_its("AlertSubject") ) {
      	str = skipFirstToken( k_com() );
		if ( str == NULL || *str==0) {
			logit( "e", "%s: <AlertSubject> command with no argument; ignoring\n",
					 Argv0);
			continue;
		}
		alertSubject = mystrdup( str );
		init[5] = 1;
      } else if( k_its("TestSubject") ) {
      	str = skipFirstToken( k_com() );
		if ( str == NULL || *str==0) {
			logit( "e", "%s: <TestSubject> command with no argument; ignoring\n",
					 Argv0);
			continue;
		}
		testSubject = mystrdup( str );
		init[6] = 1;
      } else if( k_its("MailProg") ) {
		mailProg = mystrdup( k_str() );
      } else if( k_its("MailServer") ) {
		mailServer = mystrdup( k_str() );
		init[7] = 1;
      } else if( k_its("SendMailFrom") ) {
#ifdef _WINNT
		sender = mystrdup( k_str() );
		init[8] = 1;
#else
		logit( "", "%s: <SendMailFrom> only used in Windows; ignoring.\n",
				Argv0 );
		continue;
#endif
      } else if( k_its("RingSize") ) {
		RingSize = k_long();
      } else if( k_its("MaxMsgSize") ) {
		MaxMsgSize = k_long();
      } else if( k_its("Debug") ) {
        logit( "", "Logging of debugging info enabled.\n" );
		DebugInfo = 1;
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
    fprintf(stderr, "ewnotify: ERROR, no ");
    if (!init[0])  fprintf(stderr, "<MyModuleId> ");
    if (!init[1])  fprintf(stderr, "<InRing> ");
    if (!init[2])  fprintf(stderr, "<OutRing> ");
    if (!init[3])  fprintf(stderr, "<SendMailTo> ");
    if (!init[4])  fprintf(stderr, "<ScriptModule> ");
    if (!init[5])  fprintf(stderr, "<AlertSubject> ");
    if (!init[6])  fprintf(stderr, "<TestSubject> ");
    if (!init[7])  fprintf(stderr, "<MailServer> ");
#ifdef _WINNT
    if (!init[8])  fprintf(stderr, "<SendMailFrom> ");
#endif    
    fprintf(stderr, "command(s) in <%s>; exitting!\n", configfile);
    return -1;
  }

  return err;
}

/***************************************************************************
 * ewnotify_status() builds a heartbeat or error message & puts it into    *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void ewnotify_status( unsigned char type, short ierr, char *note )
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
    sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
   else if( type == TypeError )
   {
    sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);

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
   char *subject;

   ProcessStatus = PROCESS_ALIVE;
   while (1)   /* main loop */
   {
     char *outBody, *kind;

     /* Get message from queue
      *************************/
     RequestMutex();
     ret=dequeue( &OutQueue, SSmsg, &msgSize, &Logo);
     if ( DebugInfo )
         logit("et", "Message dequeued in process thread, %d bytes in size\n", msgSize);
     ReleaseMutex_ew();
     if(ret < 0 )
     { /* -1 means empty queue */
       sleep_ew(500); /* wait a bit (changed from 1000 to 500 on 970624:ldd) */
       continue;
     }

     if ( (outBody = calloc( 1, msgSize+30 )) == NULL) {
	logit("et", "Process() Thread unable to allocate memory for message body, Fatal error exiting\n");
        ProcessStatus = PROCESS_ERR; /* file a complaint to the main thread */
        KillSelfThread(); 
	return (NULL);
     }

     /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
     ***********************************************************/ 
     if ( Logo.type == GetLogo[0].type ) {
      	/* Test message */
      	subject = testSubject;
      	kind = "TEST";
     } else {
      	subject = alertSubject;
      	if ( Logo.type == GetLogo[1].type ) {
			kind = "THRESH_ALERT";
		} else if (Logo.type == GetLogo[2].type) {
			kind = "ACCEL_ALERT";
		} else if (Logo.type == GetLogo[3].type) {
			kind = "SHEAR_ALERT";
      	        } else
      	         	kind = "RSAM_ALARM";
     }
     
     /* Build message: incomming module ID, incomming type, original body */
	 sprintf( outBody, "%3d %s %s%c", ASModId, kind, SSmsg, 0 );
	 
	 /* Send message as email */
     ret = SendMail( recip, nRecip, mailProg, subject, outBody+4, NULL, NULL,  
      				mailServer, sender );
      
     /* Post ACTIVATE_MODULE message to ring */
     Logo.type = GetLogo[0].type;
     Logo.mod = MyModId;
     if ( tport_putmsg( &OutRegion, &Logo, strlen(outBody), outBody ) != PUT_OK ) {
	logit("et", "%s(%s): Fatal Error sending ACTIVATE_SCRIPT message for %s observed.\n", Argv0, MyModName, kind );
        ProcessStatus = PROCESS_ERR; /* file a complaint to the main thread */
        free( outBody );
        KillSelfThread(); 
	return(NULL);
     }
     
     free( outBody );
   }  /* End of main loop */

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
      recsize = 0;
      res = tport_getmsg( &InRegion, GetLogo, num_logos,
                          &reclogo, &recsize, MSrawmsg, MaxMsgSize );
      MSrawmsg[recsize<MaxMsgSize?recsize:MaxMsgSize] = '\0';

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
            ewnotify_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, inRing );
            ewnotify_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     inRing );
            ewnotify_status( TypeError, ERR_NOTRACK, errText );
         }
      }
      
      if ( reclogo.type == GetLogo[0].type ) {
      	/* Verify that A_M message is for us */
		  res = sscanf( MSrawmsg, "%d", &modid );
		  
		  msgName = "ACTIVATE_MODULE";
		  okMsg = (res == 1);
		  if ( !okMsg ) {
				sprintf( errText, "malformed %s msg i%d m%d t%d in %s",msgName,
						(int) reclogo.instid,
						(int) reclogo.mod, (int)reclogo.type, inRing );
				ewnotify_status( TypeError, ERR_BADMSG, errText );
				continue;
		  }
		  if ( modid != MyModId )
			continue;
	  }

      RequestMutex();
      ret=enqueue( &OutQueue, MSrawmsg, recsize+1, reclogo );
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
         if (ret==-2)  /* Serious: quit */
         {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
        sprintf(errText,"internal queue error. Terminating.");
            ewnotify_status( TypeError, ERR_QUEUE, errText );
        goto error;
         }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            ewnotify_status( TypeError, ERR_QUEUE, errText );
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
   return(NULL);
}

int main(int argc, char **argv)
{
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

	if ((progname= strrchr(*argv, (int) '/')) != (char *) 0)
		progname++;
	else
		progname= *argv;
	
    logit_init (progname, 0, 1024, 1);

    /* Check command line arguments */
    if (argc != 2) {
		fprintf (stderr, "Usage: %s <configfile>\n", progname);
		fprintf (stderr, "Version: %s\n", VERSION);
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
	   	int gett1, gett2, gett3, gett4;
	   	GetLogo[1] = GetLogo[2] = GetLogo[3] = GetLogo[4] = GetLogo[0];
	   	gett1 = GetType( "TYPE_THRESH_ALERT", &(GetLogo[1].type) );
	   	gett2 = GetType( "TYPE_ACCEL_ALERT", &(GetLogo[2].type) );
	   	gett3 = GetType( "TYPE_SHEAR_ALERT", &(GetLogo[3].type) );
	   	gett4 = GetType( "TYPE_RSAM_ALARM", &(GetLogo[4].type) );
	   	MSrawmsg = (char *) calloc( 1,MaxMsgSize+1 );
	   	if ( geti || getm || gett || gett1 || gett2 ||gett3 || (MSrawmsg==NULL) ) {
	   		if ( geti )
	   			logit( "e", "%s: INST_WILDCARD unknown; exiting!\n", Argv0 );
	   		if ( getm )
	   			logit( "e", "%s: MOD_WILDCARD unknown; exiting!\n", Argv0 );
	   		if ( gett )
	   			logit( "e", "%s: TYPE_ACTIVATE_MODULE unknown; exiting!\n", Argv0 );
	   		if ( gett1 )
	   			logit( "e", "%s: TYPE_THRESH_ALERT unknown; exiting!\n", Argv0 );
	   		if ( gett2 )
	   			logit( "e", "%s: TYPE_ACCEL_ALERT unknown; exiting!\n", Argv0 );
	   		if ( gett3 )
	   			logit( "e", "%s: TYPE_SHEAR_ALERT unknown; exiting!\n", Argv0 );
	   		if ( MSrawmsg==NULL )
			    logit( "e", "%s: error allocating MSrawmsg; exiting!\n", Argv0 );
	   		exit(1);
	   	}
		if ( gett4 )
			num_logos = 4;
		else
			num_logos = 5;
		tport_attach( &InRegion, InRingKey );		
	}
	tport_attach( &OutRegion, OutRingKey );


	/* step over all messages from transport ring
	*********************************************/
	tport_flush( &InRegion, GetLogo, num_logos, &reclogo );
	
	/* One heartbeat to announce ourselves to statmgr
	************************************************/
	ewnotify_status( TypeHeartBeat, 0, "" );
	time(&MyLastBeat);
	
	
	/* Start the message stacking thread if it isn't already running.
	***************************************************************/
	if (MessageStackerStatus != MSGSTK_ALIVE )
	{
		 if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
		 {
		   logit( "e",
				  "%s(%s): Error starting  MessageStacker thread; exiting!\n",
			  Argv0, MyModName );
		   tport_detach( &InRegion );
		   tport_detach( &OutRegion );
		   free( MSrawmsg );
		   return( -1 );
		 }
		 MessageStackerStatus = MSGSTK_ALIVE;
	}
	

	/* Buffers for Process thread: */
	if ( ( SSmsg = (char *) calloc( 1, MaxMsgSize+1 ) ) ==  NULL )
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
		  free( MSrawmsg );
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
			  ewnotify_status( TypeHeartBeat, 0, "" );
		  time(&MyLastBeat);
		  }
	
		  /* take a brief nap
		   ******************/
		  sleep_ew(500);
                  if (MessageStackerStatus == MSGSTK_ERR) {
                     logit("et", "MessageStackerThread died, exiting for now\n");
		     break;
                  }
                  if (ProcessStatus == PROCESS_ERR) {
			/* may do more with this some day */
                     logit("et", "Process Thread died, exiting for now\n");
		     break;
                  }

	   } /*end while of monitoring loop */
		
		
	/* Clean up after ourselves */
	tport_detach( &InRegion );
	tport_detach( &OutRegion );
        if (SSmsg != NULL) free(SSmsg);
        if (MSrawmsg != NULL) free(MSrawmsg);
	return(0);
}
