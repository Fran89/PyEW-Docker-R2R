/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pickwasher.c 6845 2016-10-18 19:11:11Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2007/04/03 14:47:18  stefan
 *     pickwasher, needed for glass
 *
 *     Revision 1.2  2006/06/12 14:42:41  patton
 *     Updated to reflect the changes for the new type_pick_scnl.  JMP
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2004/05/12 17:38:13  mark
 *     Changed ? to -- for unknown location and network codes
 *
 *
 */

/*
 *   pickwasher.c
 *  
 *   Program to read pick and amp messages (with hardcoded logos) from one ring,
 *   sanitize them for global use, and deposit them in another ring.  This program is mainly 
 *   copied from ringdup.
 *
 *   Patton
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <chron3.h>
#include <errno.h>
#include <signal.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <global_msg.h>
#include <global_pick_rw.h>
#include <global_amp_rw.h>

/* Functions in this source file 
 *******************************/
void  pickwash_config  (char *);
void  pickwash_lookup  (void);
void  pickwash_status  (unsigned char, short, char *);
void  pickwash_free    (void);
long  Get_Pick_Sequence(long);
int   Check_Char       (char);
void  Global_Msg_Errors(int);

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidWash;             /* Wash thread id */
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /* to queue */
#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue; 		             /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret Wash( void * );
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and Wash thread */
/* Message Buffers to be allocated
 *********************************/
static char *Rawmsg = NULL;          /* "raw" retrieved msg for main thread      */ 
static char *DirtyMsg = NULL;        /* Wash's incoming message buffer   */
static char *ParseMsg = NULL;		 /* Buffer to strtok off of */
static char *CleanMsg = NULL;        /* The message sent after wash has finished */
static MSG_LOGO Logo;	             /* logo of message to re-send */
static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */
static char *MSfilteredmsg = NULL;   /* MessageStacker's "filtered" message to   */
                                     /*    be sent to client                     */

/* Timers
   ******/
time_t now;						/* current time, used for timing heartbeats */
time_t MyLastBeat;				/* time of last local (into Earthworm) hearbeat */

extern int  errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short 	  nLogo;

MSG_LOGO  Pick_Logo;	        /* Logo to use when outputing global Picks */

char *Argv0;                    /* pointer to executable name */ 
pid_t MyPid;			        /* Our own pid, sent with heartbeat for restart purposes */
   
/* Things to read or derive from configuration file
 **************************************************/
static char    InRing[MAX_RING_STR];          /* name of transport ring for input  */
static char    OutRing[MAX_RING_STR];         /* name of transport ring for output */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     RingSize;			/* max messages in output circular buffer */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;

/* Error messages used by export 
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE         3   /* error queueing message for sending      */
static char  errText[256];     /* string for log/error messages           */

main( int argc, char **argv )
{
/* Other variables: */
   int           res;
   long          recsize;	/* size of retrieved message             */
   MSG_LOGO      reclogo;	/* logo of retrieved message             */

   char str[24];

   /* Check command line arguments 
    ******************************/
   Argv0 = argv[0];
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
      return( 0 );
   }

   /* Initialize name of log-file & open it 
    ****************************************/
   logit_init( argv[1], 0, 512, 1 );
  
   /* Read the configuration file(s)
    ********************************/
   pickwash_config( argv[1] );
   logit( "et" , "%s(%s): Read command file <%s>\n", 
           Argv0, MyModName, argv[1] );

   /* Look up important info from earthworm.h tables
    *************************************************/
   pickwash_lookup();

   /* Reinitialize the logging level
    *********************************/
   logit_init( argv[1], 0, 512, LogSwitch );

   /* Get our own Pid for restart purposes
    ***************************************/
   MyPid = getpid();
   if(MyPid == -1)
   {
      logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
      return(0);
   }

   /* HARDCODE Logos to wash
    ************************/
   nLogo = 0;

   /* Setup incoming Pick Logo
    **************************/
   /* Listen to any source */
   strcpy (str, "INST_WILDCARD");
   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) 
   {
  	   logit( "e" , "%s: Invalid input installation name <%s>\n", Argv0, str ); 
	   exit( -1 );
   }

   /* Listen to any module */
   strcpy (str, "MOD_WILDCARD");
   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) 
   {
 	   logit( "e" , "%s: Invalid input module name <%s>\n", Argv0, str ); 
	   exit( -1 );
   }

   /* Listen only to the TYPE_PICK2K messages */
   strcpy (str, "TYPE_PICK_SCNL");
   if( GetType( str, &GetLogo[nLogo].type ) != 0 ) 
   {
	   logit( "e" , "%s: Invalid input msgtype <%s>\n", Argv0, str ); 
	   exit( -1 );
   }
   nLogo++;

    /* Lookup our output logos
    *************************/
   Pick_Logo.instid = InstId;
   Pick_Logo.mod    = MyModId;

   strcpy (str, "TYPE_PICK_GLOBAL");
   if( GetType( str, &Pick_Logo.type ) != 0 ) 
   {
	   logit( "e" , "%s: Invalid output msgtype <%s>", Argv0, str ); 
	   exit( -1 );
   }
   nLogo++;

   /* Allocate space for input/output messages for all threads
    ***********************************************************/
   /* Buffer for main thread: */
   if ( ( Rawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL ) 
   {
      logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n", 
             Argv0, MyModName );
      pickwash_free();
      return( -1 );
   }

   /* Buffer for Wash thread: */
   if ( ( DirtyMsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL ) 
   {
      logit( "e", "%s(%s): error allocating DirtyMsg; exiting!\n",
              Argv0, MyModName );
      pickwash_free();
      return( -1 );
   }

   /* Buffer to strtok off of */
   if ( ( ParseMsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL ) 
   {
      logit( "e", "%s(%s): error allocating ParseMsg; exiting!\n",
              Argv0, MyModName );
      pickwash_free();
      return( -1 );
   }

   /* Buffer for the MessageStacker thread: */
   if ( ( MSrawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL ) 
   {
      logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n", 
             Argv0, MyModName );
      pickwash_free();
      return( -1 );
   }

   /* Create a Mutex to control access to queue
    ********************************************/
   CreateMutex_ew();

   /* Initialize the message queue
    *******************************/
   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );
	   	
   /* Attach to Input/Output shared memory rings 
    *********************************************/
   tport_attach( &InRegion, InRingKey );
   tport_attach( &OutRegion, OutRingKey );

   /* step over all old (pre startup) messages 
    * from transport ring
    *******************************************/
   do
   {
     res = tport_getmsg( &InRegion, GetLogo, nLogo, 
                         &reclogo, &recsize, Rawmsg, MaxMsgSize );
   } while (res !=GET_NONE);

   /* One heartbeat to announce ourselves to statmgr
   *************************************************/
   pickwash_status( TypeHeartBeat, 0, "" );
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

   /* Start the socket writing thread
   ***********************************/
   if ( StartThread(  Wash, (unsigned)THREAD_STACK, &tidWash ) == -1 )
   {
      logit( "e", "%s(%s): Error starting Wash thread; exiting!\n",
              Argv0, MyModName );
      tport_detach( &InRegion );
      tport_detach( &OutRegion );
      return( -1 );
   }

   /* Start main pickwash service loop 
   **********************************/
   while( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid         )
   {
     /* Beat the heart into the transport ring
      ****************************************/
      time(&now);
      if (difftime(now,MyLastBeat) > (double)HeartBeatInt ) 
      {
          pickwash_status( TypeHeartBeat, 0, "" );
	  time(&MyLastBeat);
      }

      /* take a brief nap
       ******************/
      sleep_ew(500);
   } /*end while of monitoring loop */

   /* Shut it down
   ***************/
   tport_detach( &InRegion );
   tport_detach( &OutRegion );

   pickwash_free();
   logit("t", "%s(%s): termination requested; exiting!\n", 
          Argv0, MyModName );
   return( 0 );	
}
/* *******************  end of main *******************************
 ******************************************************************/

/**************************  Get_Pick_Sequence  ***********************
 *         Problem:  Picks coming from the NSN have a sequence        *
 *         number that resets to 0 after 10000 picks.  This           *
 *         function ensures that the sequence number of the           *
 *         outgoing pick message (global) has an unique sequence      *
 *         number.                                                    *
 **********************************************************************/

long Get_Pick_Sequence(long in_seq)
{
	FILE *Seq_file;

	long file_number = 0;
	long sequence_number = 0;

	/* Open the local file containing the modifer  
	 * for the next sequence number.
	 ********************************************/
	if ( (Seq_file = fopen("sequence.txt", "r")) == NULL)
	{
		/* If it doesn't exist, create a new one */
		logit ("e", "Cannot open sequence file, trying to create new one\n");
		if (( Seq_file = fopen ("sequence.txt", "w")) == NULL)
		{
			logit ("e", "Cannot open sequence file, exiting\n");
			exit (-1);
		}

		/* Initialize the new file to 0 */
		fprintf(Seq_file,"%d", 0);
		fclose (Seq_file);

		/* If the sequence file still doesn't exist, 
		 * a serious error has occured, exit immediately.
		 ************************************************/
		if (( Seq_file = fopen ("sequence.txt", "r")) == NULL)
		{
			logit ("e", "Cannot open sequence file, exiting\n");
			exit (-1);
		}
	}

	/* Read sequence modifier from disk */
	fscanf(Seq_file, "%ld", &file_number);
	fclose(Seq_file);

	/* Sequence number (from NSN) is about to 
	 * wrap and reset to zero
	 ****************************************/
	if (in_seq == 9999)
	{
		/* Compute next sequence number */
		sequence_number = file_number + in_seq;

		/* Increase sequence modifer by 10000 */
		file_number += 10000;

		/* Save new sequence modifier to disk
		 ************************************/
		if (( Seq_file = fopen ("sequence.txt", "w")) == NULL)
		{
			logit ("e", "Cannot open sequence file, exiting\n");
			exit (-1);
		}
		fprintf(Seq_file, "%ld", file_number);
		fclose(Seq_file);
	}
	else
	{
		/* Compute next sequence number */
		sequence_number = file_number + in_seq;		
	}

	return (sequence_number);
}


/*****************************  Check_Char  ***************************
 *         This function takes a character from one of the            *
 *         input messages (pick or amp) and checks that               *
 *         character against a list of characters that are            *
 *         allowed in a pick or amp message, if the character         *
 *         is not any of these, then the character fails the test     *
 **********************************************************************/

int Check_Char(char input)
{
	if (input == 32)
	{   /* Space */
		return 0;
	}
	if (input == 43)
	{   /* + */
		return 0;
	}
	if (input == 45)
	{   /* - */
		return 0;
	}
	if (input == 46)
	{   /* . */
		return 0;
	}
	if (input == 63)
	{   /* ? */
		return 0;
	}
	if ((input > 64) && (input < 91))
	{   /* A -> Z */
		return 0;
	}
	if ((input > 96) && (input < 123))
	{   /* a -> a */
		return 0;
	}
	if ((input > 47) && (input < 58))
	{   /* 0 -> 9 */
		return 0;
	}
	if (input == '\n')
	{   /* newline */
		return 0;
	}
	if (input == 0)
	{   /* null termination */
		return 0;
	}
	return 1;
}

/*************************  Global_Msg_Errors  ************************
 *         This function prints out the error messages from the       * 
 *         global message functions                                   *
 **********************************************************************/

void Global_Msg_Errors(int input)
{
	switch(input)
	{
	case 1: 
		logit ("e", "Error 1:  GLOBAL_MSG_UNKNOWN\n");
		break;
	case 0: 
		logit ("e", "Message 0: GLOBAL_MSG_SUCCESS\n");
		break;
	case -1: 
		logit ("e", "Error -1: GLOBAL_MSG_NULL\n");
		break;		
	case -2: 
		logit ("e", "Error -2: GLOBAL_MSG_VERSINVALID\n");
		break;
	case -3: 
		logit ("e", "Error -3: GLOBAL_MSG_FORMATERROR\n");
		break;
	case -4: 
		logit ("e", "Error -4: GLOBAL_MSG_MAXCHILDREN\n");
		break;
	case -5: 
		logit ("e", "Error -5: GLOBAL_MSG_BADPARAM\n");
		break;
	case -6: 
		logit ("e", "Error -6: GLOBAL_MSG_TOOSMALL\n");
		break;
	case -7: 
		logit ("e", "Error -7: GLOBAL_MSG_BADCHILD\n");
		break;
	case -9: 
		logit ("e", "Error -9: GLOBAL_MSG_DEFINESMALLN\n");
		break;
	default:
		logit ("e", "Error ?:  GLOBAL_MSG_UNKNOWN\n");
		break;
	}
	return;
}

/**************************  Main Wash Thread   ***********************
 *          Pull a messsage from the queue, and put it on OutRing     *
 **********************************************************************/

thr_ret Wash( void *dummy )
{
	int     ret, test, error, weight;
	long	dirtymsgSize = 0;  
	long	count, sequence;
	double	arrival_time;
	char *  nextToken;

	/* Global Message buffers and structures */
	GLOBAL_PICK_STRUCT	Pick_Message;

	GLOBAL_PICK_BUFFER	Clean_Pick;

	/* Initialize global structures */
	if (error = InitGlobalPick( &Pick_Message ) != 0)
	{
		logit ("e", "Wash: Error initilizing Pick_Message, exiting\n");
		Global_Msg_Errors(error);
		exit (-1);
	}

	while (1)   /* main loop */
	{
	topOfLoop:

		/* Get a message from queue
	     ******((******************/
		RequestMutex();
		ret=dequeue( &OutQueue, DirtyMsg, &dirtymsgSize, &Logo);
		ReleaseMutex_ew();

		if(ret < 0 )
		{ /* -1 means empty queue */
			sleep_ew(500); 
			continue;
		}
     
		/* Is the message a pick?
		 ***********************************/
		if (Logo.type == GetLogo[0].type)
		{
			/* if the logo from the queue is of type Pick
			 *********************************************/
			DirtyMsg[dirtymsgSize] = '\0';

			/* Wash message */
			for (count = 0; count < dirtymsgSize; count++)
			{
				test = Check_Char(DirtyMsg[count]);
				if (test == 1)
				{
					/* error, don't pass this message */
					logit ("e", "Wash: Rejecting dirty Pick message: (%s) due to bad character [%c] at position: %d.\n", DirtyMsg, DirtyMsg[count], count);
					break;
				}
				test = 0;
			}
			if (test != 1)
			{
				/* Message is assumed to be good from
				 * this point on, except if the time check fails
				 ************************************/
				logit ("e", "Wash: Pick Parser Parsed: ");

				strcpy(ParseMsg, DirtyMsg);

				/* Startup + type */
				nextToken = strtok(ParseMsg, " ");

				/* ModID */
				nextToken = strtok(NULL, " ");

				/* InstID */
				nextToken = strtok(NULL, " ");

				/* Pick Index */
				nextToken = strtok(NULL, " ");

				sequence = atol(nextToken);
				Pick_Message.sequence = Get_Pick_Sequence(sequence);
				logit ("e", "<sequence> ");

				/* Station */
				nextToken = strtok(NULL, " .");

				strcpy(Pick_Message.station, nextToken);
				logit ("e", "<station> ");

				/* Channel */
				nextToken = strtok(NULL, " .");

				strcpy(Pick_Message.channel, nextToken);
				logit ("e", "<channel> ");

				/* Network */
				nextToken = strtok(NULL, " .");

				strcpy(Pick_Message.network, nextToken);
				logit ("e", "<network> ");

				/* Location */
				nextToken = strtok(NULL, " .");

				strcpy(Pick_Message.location, nextToken);
				logit ("e", "<location> ");

				/* First Motion + Weight */
				nextToken = strtok(NULL, " ");

				Pick_Message.polarity = nextToken[0];
				logit ("e", "<polarity> ");

				nextToken[0] = '0';
				weight = atoi(nextToken);

				/* dSigma calculator */
				if (weight == 3)
					Pick_Message.quality = 0.08;
				else if (weight == 2)
					Pick_Message.quality = 0.05;
				else if (weight == 1)
					Pick_Message.quality = 0.03;
				else if (weight == 0)
					Pick_Message.quality = 0.02;
				else
					Pick_Message.quality = 99.99;
				logit ("e", "<quality> ");


				/* Arrival Time */
				nextToken = strtok(NULL, " ");
				strcpy(Pick_Message.pick_time, nextToken);

				/* Convert time_str to float time, 
 				 * to ensure good timestamp
				 *********************************/
				if (epochsec17( &arrival_time, Pick_Message.pick_time ) != 0)
				{
					logit ("e", "Error decoding pick arrival time, rejecting message: %s.\n", DirtyMsg);
					goto topOfLoop;
				}

				logit ("e", "<pick_time> ");

				/* Phase Name */
				/* pick_scnl message doesn't currently 
				   contain this, inserting placeholder */
				strcpy(Pick_Message.phase_name, "?");
				logit ("e", "<phase_name> ");

				/* Message Logo */
				Pick_Message.logo = Pick_Logo;
				logit ("e", "<logo>\n");

				/* Version */
				/* Set by init function, don't set it Here!*/
		
				/* Write phase */
				logit ("e", "Wash: Prepareing to send pick message. ");
				if (error = WritePickToBuffer(&Pick_Message, Clean_Pick, sizeof(Clean_Pick)) != 0)
				{
					logit ("e", "Wash: Error writing Pick_Message to buffer, exiting\n");
					Global_Msg_Errors(error);
					exit (-1);
				}

				/* Send Phase */
				logit ("e", "Ready to send message. ");
				if (tport_putmsg (&OutRegion, &Pick_Logo, strlen(Clean_Pick), Clean_Pick) != PUT_OK)
					logit ("et", "Wash: Error sending message to out ring\n");
				logit ("e", "Sent phase message, with sequence: %ld.\n\n", Pick_Message.sequence);
			}
		}
	}   /* End of main loop */
}

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;	/* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   int		 res;
   int 		 ret;
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
      res = tport_getmsg( &InRegion, GetLogo, nLogo, 
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
            pickwash_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS ) 
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, InRing );
            pickwash_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK ) 
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     InRing );
            pickwash_status( TypeError, ERR_NOTRACK, errText );
         }
      }
                     
      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK) 
      ***********************************************************/
      MSrawmsg[recsize] = '\0';
 
      /* put it into the 'to be shipped' queue */
      /* the Wash thread is in the biz of de-queueng and sending */
      RequestMutex();
      ret=enqueue( &OutQueue, MSrawmsg, recsize, reclogo ); 
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {       
         if (ret==-2)  /* Serious: quit */
         {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
	    sprintf(errText,"internal queue error. Terminating.");
            pickwash_status( TypeError, ERR_QUEUE, errText );
	    goto error;
         }
         if (ret==-1) 
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            pickwash_status( TypeError, ERR_QUEUE, errText );
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
}

/*****************************************************************************
 *  pickwash_config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.	             *
 *****************************************************************************/
void pickwash_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */ 
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
/*   char     processor[15];*/
   int      nfiles;
   int      success;
   int      i;	
   char*    str;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   ncommand = 7;
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
	logit( "e" ,
                "%s: Error opening command file <%s>; exiting!\n", 
                Argv0, configfile );
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
                           Argv0, &com[1] );
                  exit( -1 );
               }
               continue;
            }
   /*         strcpy( processor, "pickwash_config" );*/

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
  /*3*/     else if( k_its("OutRing") ) {
                str = k_str();
                if(str) strcpy( OutRing, str );
                init[3] = 1;
            }
  /*4*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_int();
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
  /*6*/     else if( k_its("RingSize") ) {
                RingSize = k_long();
                init[6] = 1;
            }

         /* Unknown command
          *****************/ 
	    else {
                logit( "e" , "%s: <%s> Unknown command in <%s>.\n", 
                         Argv0, com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit( "e" , 
                       "%s: Bad <%s> command for pickwash_config() in <%s>; exiting!\n",
                        Argv0, com, configfile );
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
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit( "e", "<LogFile> "      );
       if ( !init[1] )  logit( "e", "<MyModuleId> "   );
       if ( !init[2] )  logit( "e", "<InRing> "     );
       if ( !init[3] )  logit( "e", "<OutRing> "     );
       if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
       if ( !init[5] )  logit( "e", "<MaxMsgSize> "  );
       if ( !init[6] )  logit( "e", "<RingSize> "   );
       logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   return;
}

/****************************************************************************
 *  pickwash_lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
void pickwash_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( InRingKey = GetKey(InRing) ) == -1 ) {
	fprintf( stderr,
 	        "%s:  Invalid ring name <%s>; exiting!\n", 
                 Argv0, InRing);
	exit( -1 );
   }   
   if( ( OutRingKey = GetKey(OutRing) ) == -1 ) {
	fprintf( stderr,
 	        "%s:  Invalid ring name <%s>; exiting!\n", 
                 Argv0, OutRing);
	exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr, 
              "%s: error getting local installation id; exiting!\n",
               Argv0 );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr, 
              "%s: Invalid module name <%s>; exiting!\n", 
               Argv0, MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
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
   return;
} 

/***************************************************************************
 * pickwash_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void pickwash_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char	       msg[256];
   long	       size;
   long        t;
 
/* Build the message
 *******************/ 
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
	sprintf( msg, "%ld %ld\n", t, (long) MyPid);
   else if( type == TypeError )
   {
	sprintf( msg, "%ld %hd %s\n", t, ierr, note);

	logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */ 	

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
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

/***************************************************************************
 * pickwash_free()  free all previously allocated memory                     *
 ***************************************************************************/
void pickwash_free( void )
{
   free (Rawmsg);               /* "raw" retrieved msg for main thread    */ 
   free (DirtyMsg);                /* Wash's incoming message buffer   */
   free (ParseMsg);				/* message tokened with */
   free (MSrawmsg);             /* MessageStacker's "raw" retrieved message */   
   
   return;
}
