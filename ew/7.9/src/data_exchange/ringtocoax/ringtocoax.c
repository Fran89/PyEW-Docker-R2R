
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ringtocoax.c 5920 2013-09-11 16:07:12Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.8  2007/02/23 16:28:19  paulf
 *     fixed long warning for time_t
 *
 *     Revision 1.7  2005/04/08 17:28:56  dietz
 *     Changed tport_getmsg to tport_copyfrom to get more info when messages
 *     are missed.
 *
 *     Revision 1.6  2002/03/18 17:56:56  patton
 *     Made logit changes.
 *
 *     Revision 1.5  2001/06/06 21:55:41  dietz
 *     Fixed minor harmless startup bug in ring flush tport_getmsg
 *     call, use nLogo instead of 1.
 *
 *     Revision 1.4  2001/05/08 22:45:18  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.3  2000/07/24 19:07:11  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/04/13 22:43:58  alex
 *     moved heartbeat so we keep beating during busy times. alex
 *
 *     Revision 1.1  2000/02/14 19:11:50  lucky
 *     Initial revision
 *
 *
 */

                /************************************************
                 *                ringtocoax.c                  *
                 *                                              *
                 *   Gets all messages from a transport ring    *
                 *   and broadcasts them to an ethernet port.   *
                 ************************************************/
/* 4/13/00 Alex:
 *   Moved heartbeat logic into working loop to prevent heart stoppage
 *   during very busy times.
 *
 * 9/30/99 Whitmore:
 *   Added Restart messages to list of messages not broadcast when
 *   configured as such.
 *
 * 4/19/99  Dietz:
 *   Added optional configuration command "BroadcastLogo" so that
 *   one can specify which logos to broadcast.  If this command
 *   is omitted, "broadcast everything" is the default.
 *
 * 11/19/98  V4.0 changes Lombard:
 *   0) no Y2k dates
 *   1) changed argument of logit_init to the config file name.
 *   2) process ID in heartbeat message
 *   3) flush input transport ring: not applicable
 *   4) add `restartMe' to .desc file
 *   5) multi-threaded logit: not applicable
 *
 * 3/29/96  Alex:
 *   Added switch to conrol whether heartbeats are passed or blocked
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <ew_packet.h>

/* Function prototypes
   *******************/
void ringtocoax_config( char * );
void ringtocoax_lookup( void );
void ringtocoax_status( unsigned char, short, char * );
void ringtocoax_shutdown( int );
void BroadcastMsg( MSG_LOGO, long, char * );
int  SocketInit( void );
int  SendPacket( PACKET *, long, char *, int );
void SocketShutdown( void );

static SHM_INFO  Region;               /* Shared memory to use for i/o     */
static char      Attached = 0;         /* Flag=1 when attached to sharedmem*/
static char     *Buffer = NULL;        /* Character string to hold msg     */
static MSG_LOGO *trackLogo = NULL;     /* Array of logos being tracked     */
static unsigned char *trackSeq = NULL; /* Array of msg sequence numbers    */
static MSG_LOGO *GetLogo = NULL;       /* logo(s) to get from shared memory*/
static short     nGetLogo = 0;         /* # logos we're configured to get  */

/* Things to read or derive from configuration file
   ************************************************/
static char RingName[MAX_RING_STR];          /* Name of transport ring for i/o     */
static char MyModName[MAX_MOD_STR];         /* Speak as this module name/id       */
static int  LogSwitch;             /* 0 if no log file should be written */
static long HeartBeatInterval;     /* Seconds between heartbeats         */
static long MsgMaxBytes;           /* Maximum message size in bytes      */
static char OutAddress[20];        /* IP address of line of broadcast on */
static int  OutPortNumber;         /* Well-known port number             */
static int  MaxLogo;               /* Maximum number of logos to track   */
static int  BurstInterval;         /* Msec between bursts of packets     */
static int  ScrnMsg;               /* If 1, print msg logos on screen    */
static int  CopyStatus;            /* If 1, copy heartbeats              */
static int  BurstCount;            /* Num of messages per timer interval */
static int  SqrtCount;             /* Num of sqrt roots between messages */

/* Things to look up in the earthworm.h tables with getutil.c functions
   ********************************************************************/
static long          RingKey;       /* Key of transport ring for i/o     */
static unsigned char InstId;        /* Local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeRestart;

/* Error messages used by this program
   ***********************************/
#define ERR_MISSMSG        0   /* Message missed in transport ring */
#define ERR_TOOBIG         1   /* Retreived msg too large for buffer */
#define ERR_NOTRACK        2   /* Msg retreived; tracking limit exceeded */
#define ERR_SENDPACKET     3   /* Error sending a packet to ethernet */
#define ERR_TOOMANYLOGO    4   /* Number of logos > MaxLogo from config file */
static char Text[150];         /* String for log/error messages */

/* Global to this file
   *******************/
static timer_t timerHandle;    /* Handle for timer object */
static pid_t myPid;            /* for restarts by startstop */

         /***************************************************
          *          The main program starts here           *
          ***************************************************/

int main( int argc, char *argv[] )
{
   time_t          timeNow;        /* Current time */
   time_t          timeLastBeat;   /* Time last heartbeat was sent */
   long          recsize;        /* Size of retrieved message */
   MSG_LOGO      reclogo;        /* Logo of retrieved message */
   unsigned char recseq;         /* transport seq# in input ring */
   int           res;
   int           i;

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: ringtocoax <configfile>\n" );
      return -1;
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read the configuration file(s)
   ******************************/
   ringtocoax_config( argv[1] );
   logit( "t" , "Read command file <%s>\n", argv[1] );

/* Look up important info from earthworm.h tables
   **********************************************/
   ringtocoax_lookup();

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, LogSwitch );

/* Malloc the message buffer
   *************************/
   Buffer = (char *)malloc( (size_t)(MsgMaxBytes+1) * sizeof(char) );
   if ( Buffer == NULL )
   {
      logit( "e", "ringtocoax: malloc error for message buffer; "
                       "exiting!\n" );
      ringtocoax_shutdown( -1 );
   }

/* Malloc the array of logos to track
   **********************************/
   trackLogo = (MSG_LOGO *)malloc( MaxLogo * sizeof(MSG_LOGO) );
   if ( trackLogo == NULL )
   {
      logit( "e", "ringtocoax: malloc error for trackLogo; exiting!\n" );
      ringtocoax_shutdown( -1 );
   }

   trackSeq = (unsigned char *)malloc( MaxLogo * sizeof(unsigned char) );
   if( trackSeq == NULL )
   {
      logit( "e", "ringtocoax: malloc error for trackSeq; exiting!\n" );
      ringtocoax_shutdown( -1 );
   }

/* Malloc the logos to broadcast
   (if no BroadcastLogo commands were in the config file)
   ******************************************************/
   if( nGetLogo == 0 )
   {
      GetLogo = (MSG_LOGO *) malloc( sizeof(MSG_LOGO) );
      if( GetLogo == NULL )
      {
         logit( "e", "ringtocoax: malloc error for GetLogo; exiting!\n" );
         ringtocoax_shutdown( -1 );
      }
      GetLogo[0].instid = 0;  /* default to 0 = wildcard */
      GetLogo[0].mod    = 0;
      GetLogo[0].type   = 0;
      nGetLogo = 1;
   }
   for( i=0; i<nGetLogo; i++ )
   {
     logit( "e",
            "ringtocoax: BroadcastLogo%3d:   inst:%3d   mid:%3d   typ:%3d\n",
             i+1, (int)GetLogo[i].instid, (int)GetLogo[i].mod,
             (int)GetLogo[i].type );
   }

/* Get process ID for heartbeat messages
   *************************************/
   myPid = getpid();
   if( myPid == -1 )
   {
      logit("e","ringtocoax: Cannot get pid; exiting!\n");
      ringtocoax_shutdown( -1 );
   }

/* Attach to Input/Output shared memory ring
   *****************************************/
   tport_attach( &Region, RingKey );
   Attached = 1;
   logit( "t", "Attached to public memory region %s: %d\n", RingName, RingKey );

/* Flush the incoming transport ring on startup
   ********************************************/
   while ( tport_copyfrom( &Region, GetLogo, nGetLogo, &reclogo, &recsize,
			   Buffer, MsgMaxBytes, &recseq ) != GET_NONE );
 
/* Force a heartbeat to be issued in first pass thru main loop
   ***********************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

/* Initialize the socket system for broadcasting
   *********************************************/
   if ( SocketInit() == -1 )
   {
      logit( "et", "SocketInit error; exiting!\n" );
      ringtocoax_shutdown( -1 );
   }

/* Setup done; start main loop
   ***************************/
   while ( 1 )
   {


      /* Process all new messages
       **************************/
      do
      {

        /* Send ringtocoax's heartbeat
        ***************************/
         if ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval )
         {
            timeLastBeat = timeNow;
            ringtocoax_status( TypeHeartBeat, 0, "" );
         }

        /* See if a termination has been requested
        ***************************************/
         if ( tport_getflag( &Region ) == TERMINATE ||
              tport_getflag( &Region ) == myPid       )
         {
            logit( "t", "ringotcoax: Termination requested; exiting!\n" );
            ringtocoax_shutdown( 0 );
         }

         /* Get a message from the transport ring
         *************************************/
         res = tport_copyfrom( &Region, GetLogo, nGetLogo, &reclogo,
                               &recsize, Buffer, MsgMaxBytes, &recseq );

         /* Check the return code from tport_copyfrom()
         **********************************************/
         if ( res == GET_NONE )          /* No more new messages     */
            break;

         else if ( res == GET_TOOBIG )   /* Next message was too big */
         {                               /* complain and try again   */
            sprintf( Text,
                    "Retrieved msg[%ld] (i%u m%u t%u seq%u) too big for Buffer[%ld]",
                    recsize, reclogo.instid, reclogo.mod, reclogo.type, recseq,
                    MsgMaxBytes );
            ringtocoax_status( TypeError, ERR_TOOBIG, Text );
            continue;
         }

         else if ( res ==  GET_MISS_LAPPED ) /* Got a msg, but missed some */
         {
            sprintf( Text,
              "Missed msgs (overwritten)  i%u m%u t%u seq%u  %s.",
               reclogo.instid, reclogo.mod, reclogo.type, recseq, RingName );
            ringtocoax_status( TypeError, ERR_MISSMSG, Text );
         }

         else if ( res ==  GET_MISS_SEQGAP ) /* Got a msg, but saw sequence gap */
         {
            sprintf( Text,
              "Missed msgs (sequence gap) i%u m%u t%u seq%u  %s.",
               reclogo.instid, reclogo.mod, reclogo.type, recseq, RingName );
            ringtocoax_status( TypeError, ERR_MISSMSG, Text );
         }

         else if ( res == GET_NOTRACK )  /* Got a msg, but can't tell */
         {                               /* if any were missed        */
            sprintf( Text,
              "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
               reclogo.instid, reclogo.mod, reclogo.type );
            ringtocoax_status( TypeError, ERR_NOTRACK, Text );
         }
         Buffer[recsize] = '\0';         /* Null-terminate the message */

         /* Is it a heartbeat or error, and should we pass them on
         *********************************************************/
         if ( CopyStatus == 0 && reclogo.type == TypeHeartBeat ) continue;
         if ( CopyStatus == 0 && reclogo.type == TypeError     ) continue;
         if ( CopyStatus == 0 && reclogo.type == TypeRestart   ) continue;

         /* Split the message into packets.
            Then, broadcast packets onto network.
          ***************************************/
         BroadcastMsg( reclogo, recsize, Buffer );

      } while( res != GET_NONE );        /* End of message-processing-loop */

      if( BurstInterval ) sleep_ew( 2*BurstInterval );  /* Wait for new messages to arrive */
      else                sleep_ew( 50 );
   }
}


 /************************************************************************
  *                          ringtocoax_config()                         *
  *                                                                      *
  *           Processes command file(s) using kom.c functions.           *
  *                  Exits if any errors are encountered.                *
  ************************************************************************/
#define ncommand 13      /* # of required commands you expect to process   */

void ringtocoax_config( char *configfile )
{
   char   init[ncommand]; /* Init flags, one byte for each required command */
   int    nmiss;          /* Number of required commands that were missed   */
   char  *com;
   char  *str;
   int    nfiles;
   int    success;
   int    i;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
        logit( "e", 
                "ringtocoax: Error opening command file <%s>; exiting!\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
   *************************/
   while ( nfiles > 0 )       /* While there are command files open */
   {
      while ( k_rd() )        /* Read next line from active file  */
      {
         com = k_str();       /* Get the first token from line */

 /* Ignore blank lines & comments
    *****************************/
         if ( !com )          continue;
         if ( com[0] == '#' ) continue;

 /* Open a nested configuration file
    ********************************/
         if ( com[0] == '@' )
         {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success )
            {
               logit( "e", 
                 "ringtocoax: Error opening command file <%s>; exiting!\n",
                  &com[1] );
               exit( -1 );
            }
            continue;
         }

/* Process anything else as a command
   **********************************/
         if ( k_its( "LogSwitch" ) )
         {
            LogSwitch = k_int();
            init[0] = 1;
         }
         else if ( k_its( "MyModName" ) )
         {
            str = k_str();
            if (str) strcpy( MyModName, str );
            init[1] = 1;
         }
         else if ( k_its( "RingName" ) )
         {
            str = k_str();
            if (str) strcpy( RingName, str );
            init[2] = 1;
         }
         else if ( k_its( "HeartBeatInterval" ) )
         {
            HeartBeatInterval = k_long();
            init[3] = 1;
         }
         else if ( k_its( "MsgMaxBytes" ) )
         {
            MsgMaxBytes = k_int();
            init[4] = 1;
         }
         else if ( k_its( "OutAddress" ) )
         {
            str = k_str();
            if (str) strcpy( OutAddress, str );
            init[5] = 1;
         }
         else if ( k_its( "OutPortNumber" ) )
         {
            OutPortNumber = k_int();
            init[6] = 1;
         }
         else if ( k_its( "MaxLogo" ) )
         {
            MaxLogo = k_int();
            init[7] = 1;
         }
         else if ( k_its( "BurstInterval" ) )
         {
            BurstInterval = k_int();
            init[8] = 1;
         }
         else if ( k_its( "ScrnMsg" ) )
         {
            ScrnMsg = k_int();
            init[9] = 1;
         }
         else if ( k_its( "CopyStatus" ) )
         {
            CopyStatus = k_int();
            if(CopyStatus == 0 || CopyStatus == 1) init[10] = 1;
         }
         else if ( k_its( "BurstCount" ) )
         {
            BurstCount = k_int();
            init[11] = 1;
         }
         else if ( k_its( "SqrtCount" ) )
         {
            SqrtCount = k_int();
            init[12] = 1;
         }

         else if ( k_its( "BroadcastLogo" ) )  /*OPTIONAL*/
         {
            MSG_LOGO *tlogo = NULL;
            tlogo = (MSG_LOGO *)realloc( GetLogo, (nGetLogo+1)*sizeof(MSG_LOGO) );
            if( tlogo == NULL )
            {
               logit( "e", "ringtocoax: BroadcastLogo: error reallocing"
                               " %d bytes; exiting!\n",
                       (nGetLogo+1)*sizeof(MSG_LOGO) );
               exit( -1 );
            }
            GetLogo = tlogo;

            if( ( str = k_str() ) )
            {
               if ( GetInst( str, &(GetLogo[nGetLogo].instid) ) != 0 )
               {
                  fprintf( stderr,
                          "ringtocoax: BroadcastLogo: invalid installation `%s'"
                          " in `%s'; exiting!\n", str, configfile );
                  exit( -1 );
               }
            }
            if( ( str = k_str() ) )
            {
               if ( GetModId( str, &(GetLogo[nGetLogo].mod) ) != 0 )
               {
                  fprintf( stderr,
                          "ringtocoax: BroadcastLogo: invalid module id `%s'"
                          " in `%s'; exiting!\n", str, configfile );
                  exit( -1 );
               }
            }
            if( ( str = k_str() ) )
            {
               if ( GetType( str, &(GetLogo[nGetLogo].type) ) != 0 )
               {
                  logit( "e",
                          "ringtocoax: BroadcastLogo: invalid msgtype `%s'"
                          " in `%s'; exiting!\n", str, configfile );
                  exit( -1 );
               }
            }
            nGetLogo++;
         }

/* Unknown command
   ***************/
         else
         {
             logit( "e", "ringtocoax: `%s' Unknown command in `%s'.\n",
                      com, configfile );
             continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "ringtocoax: Bad `%s' command in `%s'; exiting!\n",
                    com, configfile );
            exit( -1 );
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
   ****************************************************************/
   nmiss = 0;
   for ( i = 0; i < ncommand; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss )
   {
      logit( "e", "ringtocoax: ERROR, no " );
      if ( !init[0] )  logit( "e", "<LogSwitch> " );
      if ( !init[1] )  logit( "e", "<MyModName> " );
      if ( !init[2] )  logit( "e", "<RingName> " );
      if ( !init[3] )  logit( "e", "<HeartBeatInterval> " );
      if ( !init[4] )  logit( "e", "<MsgMaxBytes> " );
      if ( !init[5] )  logit( "e", "<OutAddress> " );
      if ( !init[6] )  logit( "e", "<OutPortNumber> " );
      if ( !init[7] )  logit( "e", "<MaxLogo> " );
      if ( !init[8] )  logit( "e", "<BurstInterval> " );
      if ( !init[9] )  logit( "e", "<ScrnMsg> " );
      if ( !init[10])  logit( "e", "<CopyStatus> " );
      if ( !init[11])  logit( "e", "<BurstCount> " );
      if ( !init[12])  logit( "e", "<SqrtCount> " );
      logit( "e", "command(s) in <%s>; exiting!\n", configfile );
      exit( -1 );
   }
   return;
}


  /****************************************************************
   *                      ringtocoax_lookup()                     *
   *                                                              *
   *        Look up important info from earthworm.h tables        *
   ****************************************************************/

void ringtocoax_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 )
   {
        fprintf( stderr,
          "ringtocoax:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      fprintf( stderr,
        "ringtocoax: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 )
   {
      fprintf( stderr,
        "ringtocoax: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      fprintf( stderr,
              "ringtocoax: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      fprintf( stderr,
              "ringtocoax: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_RESTART", &TypeRestart ) != 0 )
   {
      fprintf( stderr,
              "ringtocoax: Invalid message type <TYPE_RESTART>; exiting!\n" );
      exit( -1 );
   }
   return;
}


  /****************************************************************
   *                      ringtocoax_status()                     *
   *                                                              *
   *      Builds a heartbeat or error message & puts it into      *
   *      shared memory.  Writes errors to log file & screen.     *
   ****************************************************************/

void ringtocoax_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO logo;
   char     msg[256];
   long     size;
   time_t     t;

/* Build the message
   *****************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
      sprintf( msg, "%ld %d\n", (long) t, (int) myPid);

   else if( type == TypeError )
   {
      sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
      logit( "et", "ringtocoax: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
   **********************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
      if( type == TypeHeartBeat )
         logit("et","ringtocoax:  Error sending heartbeat.\n" );

      else if( type == TypeError )
         logit("et","ringtocoax:  Error sending error:%d.\n", ierr );
   }

   return;
}

 /************************************************************************
  *                        ringtocoax_shutdown()                         *
  *                                                                      *
  *    Shuts down politely (frees memory, detaches from rings, etc.)     *
  ************************************************************************/
void ringtocoax_shutdown( int estatus )
{
   if( Buffer    ) free( Buffer    );
   if( trackLogo ) free( trackLogo );
   if( trackSeq  ) free( trackSeq  );
   if( GetLogo   ) free( GetLogo   );
   if( Attached  ) tport_detach( &Region );   /* Detach from shared memory */
   SocketShutdown();                          /* close socket */
   exit( estatus );
}

  /*******************************************************************
   *                          BroadcastMsg()                         *
   *                                                                 *
   *   Split a message into packets.  Then, broadcast onto network.  *
   *   The sequence numbers of messages are stored by logo in the    *
   *   array trackSeq[].                                             *
   *******************************************************************/

void BroadcastMsg( MSG_LOGO logo, long msgSize, char *message )
{
   static int    packetCounter = 0;
   static int    nlogo = 0;
   int           i, j;
   int           lenSent;
   int           msgBytesToSend;
   int           packetBytes;
   int           packetDataBytes;
   PACKET        packet;
   unsigned char fragNum;
   unsigned char msgSeqNum;
   unsigned char lastOfMsg;
   char          *packetDataStart;

/* Get the index (j) of the next message
   sequence number for this logo
   *************************************/
   j = -1;                            /* -1 if not found */
   for ( i = 0; i < nlogo; i++ )
      if ( logo.type   == trackLogo[i].type &&
           logo.mod    == trackLogo[i].mod  &&
           logo.instid == trackLogo[i].instid )
      {
         j = i;
         break;
      }

/* We fell through the loop.
   This is a new logo.
   *************************/
   if ( j == -1 )
   {
      if ( nlogo == MaxLogo )
      {
         sprintf( Text, "Number of logo types exceeds the max= %d\n", MaxLogo );
         ringtocoax_status( TypeError, ERR_TOOMANYLOGO, Text );
         return;
      }

      j = nlogo;
      trackLogo[j] = logo;
      trackSeq[j]  = 0;
      nlogo++;
   }
   msgSeqNum = trackSeq[j]++;

/* Print some header values
   ************************/
   if ( ScrnMsg == 1 )
   {
      fprintf( stderr, "sending: ");
      fprintf( stderr, "nlogo:%d",  nlogo );
      fprintf( stderr, " inst:%u",  logo.instid );
      fprintf( stderr, " mid:%2u",  logo.mod );
      fprintf( stderr, " typ:%2u",  logo.type );
      fprintf( stderr, " len:%5ld", msgSize );
      fprintf( stderr, " cseq:%3u", msgSeqNum );
      fprintf( stderr, "\n" );
   }

/* Split the message into packets with a maximum size of 1472 bytes
   ****************************************************************/
   msgBytesToSend  = msgSize;
   fragNum         = 0;
   packetDataStart = message;

   do
   {

/* Set packet header values
   ************************/
      if ( msgBytesToSend > UDP_DAT )
      {
         packetDataBytes = UDP_DAT;
         lastOfMsg = 0;
      }
      else
      {
         packetDataBytes = msgBytesToSend;
         lastOfMsg = 1;
      }
      msgBytesToSend -= packetDataBytes;

      packet.msgInst   = logo.instid; /* Installation id */
      packet.msgType   = logo.type;   /* Message Type */
      packet.modId     = logo.mod;    /* Id of module originating message */
      packet.fragNum   = fragNum++;   /* Packet number of message; 0=>first */
      packet.msgSeqNum = msgSeqNum;   /* Message Sequence number */
      packet.lastOfMsg = lastOfMsg;   /* 1=> last packet of message, else 0 */

/* Copy data to the packet cargo bay
   *********************************/
      memcpy( packet.text, packetDataStart, packetDataBytes );
         packetDataStart += packetDataBytes;

/* Sleep for a while or else do some square roots
   **********************************************/
      if ( ++packetCounter == BurstCount )
      {
         packetCounter = 0;

         if ( BurstInterval > 0 )
            sleep_ew( BurstInterval );
      }
      else
      {
         int i;
         for ( i = 0; i < SqrtCount; i++ )
         {
            double a;
            a = sqrt( (double)i );
         }
      }

/* Broadcast the packet onto the network
   *************************************/
      packetBytes = packetDataBytes + UDP_HDR;

      lenSent = SendPacket( &packet, packetBytes, OutAddress, OutPortNumber );

      if ( lenSent != packetBytes )
      {
         sprintf( Text, "Error sending packet. lenSent: %d  packetBytes: %d\n",
                lenSent, packetBytes );
         ringtocoax_status( TypeError, ERR_SENDPACKET, Text );
      }

   } while ( msgBytesToSend > 0 );

   return;
}


