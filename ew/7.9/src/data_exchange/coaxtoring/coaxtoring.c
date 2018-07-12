
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: coaxtoring.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2007/03/28 17:14:14  paulf
 *     removed malloc.h incl since it is in earthworm.h
 *
 *     Revision 1.9  2007/03/28 15:12:27  paulf
 *     added _MACOSX #define
 *
 *     Revision 1.8  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.7  2002/12/27 15:35:16  friberg
 *     Changed Heartbeat thread to send a first heartbeat when the
 *     program first starts up. Previously it waited HeartbeatInt seconds
 *     before sending the first heartbeat and this caused restart problems
 *     if the program was killed before beating at least once.
 *
 *     Revision 1.6  2002/02/11 21:56:33  patton
 *     *** empty log message ***
 *
 *     Revision 1.5  2002/02/11 21:22:41  patton
 *     Made logit changes due to new logit code.
 *
 *     Revision 1.4  2001/08/10 22:01:54  kohler
 *     Minor loogin
 *
 *
 *     Minor logging change.
 *
 *     Revision 1.3  2001/05/08 22:14:52  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2000/08/08 17:31:08  lucky
 *     Lint cleanup
 *
 *     Revision 1.1  2000/02/14 16:16:56  lucky
 *     Initial revision
 *
 *
 */

       /***********************************************************
        *                      coaxtoring.c                       *
        *                                                         *
        *       Ethernet to transport shared memory program.      *
        *                                                         *
        *  Incoming packets may be of any msgInst, msgType,       *
        *  and modId, but they must have the same port number,    *
        *  specified in the configuration file.  The program has  *
        *  three threads.  The first thread (MsgIn) collects      *
        *  packets and assembles them into messages.  The second  *
        *  thread writes messages to the transport shared memory  *
        *  ring buffer.  The third thread generates heartbeat     *
        *  messages.  Messages are transferred without            *
        *  translation of any kind.  This file is strictly        *
        *  ANSII C.  System specific features are contained in    *
        *  files threads_os2.c and threads_sol.c                  *
        *                                                         *
        *  Errors:                                                *
        *  1 Receiver error                                       *
        *  2 Message buffer overflow                              *
        *  3 Lost message(s)                                      *
        **********************************************************/

/* changes: 
  Lombard: 11/19/98: V4.0 changes: 
     0) no Y2k dates 
     1) changed argument of logit_init to the config file name.
     2) process ID in heartbeat message
     3) flush input transport ring: not applicable
     4) add `restartMe' to .desc file
     5) multi-threaded logit
*/

#define BUFFER_UNUSED       0
#define BUFFER_PARTLYFILLED 1
#define BUFFER_FULL         2

#define MAX_MSGTYPE 16               /* Max message types to track */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <ew_packet.h>

int  socketInit( char *, int, int ); /* In receiver.c */
int  receiver( PACKET * );           /* In receiver.c */
void coaxtoring_config(char * );     /* Read configuration file */
void LogConfig( void );              /* Log the configuration file */
int  MsgInInit( void );              /* Routine to initialize buffers, etc */
int  ReportError( int, char * );     /* Create an error message */
int  GetBuf( void );                 /* Get an empty buffer */

thr_ret Heartbeat( void * );
thr_ret MsgIn( void * );
thr_ret KillProcess( void * );

/* MSG_INFO is the buffer descriptor for incoming messages. There will
   be one of these structures for each buffer in the ring. */

typedef struct
{
   volatile char *start;        /* Buffer start */
   MSG_LOGO       logo;          /* msginstid, msgtype, modid */
   unsigned short length;        /* Number of bytes in message */
   unsigned char  cSeqNum;       /* Message sequence number on coax */
   unsigned char  pSeqNum;       /* Message sequence number in this program */
   short          status;        /* 0=available; 1=partly filled, 2=full */
   volatile char *nxtMsgBytePtr; /* Pointer to partially filled buffer */
   unsigned short msgByteCnt;    /* Message length so far */
   unsigned char  nxtFragNum;    /* Next expected packet fragment number */
} MSG_INFO;

volatile MSG_INFO *MsgPtr;       /* Array of buffer pointers */

volatile int nBufferUsed;        /* Keep track of buffer counts */
volatile int maxBufferUsed;

unsigned char pSeqNumA = 0;      /* Program sequence number received */
unsigned char pSeqNumB = 0;      /* Program sequence number sent */

SHM_INFO region;                 /* Transport region */
pid_t   myPid;                   /* Process ID for restarts by startstop */

/* Things to look up in the earthworm.h tables with getutil.c functions
   ********************************************************************/
static unsigned char InstId;         /* local installation id */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* The following variables are from the config file
   ************************************************/
int           nMsgBuf;           /* Number of message buffers in use */
unsigned      MsgMaxBytes;       /* Size of Message Buffers in bytes */
char          InAddress[20];     /* IP address of line to listen to */
int           PortNumber;        /* UDP Port to listen to */
int           ScrnMsg;           /* If 1, print messages on screen */
int           HeartbeatInt;      /* Heartbeat interval in seconds */
int           LogSwitch;         /* If 0, no logging should be done to disk */
long          RingKey;           /* Key to the transport ring to write to */
int           BufferReportInt;   /* Interval in seconds between buffer reports */
int           RcvBufSize;        /* Size of IP receive buffer */
static unsigned char MyModuleId; /* Module id for this instance of coaxtoring */

int main( int argc, char *argv[] )
{
   MSG_INFO *pMsgOut;            /* Pointer to output buffer */
   int      j;
   int      res;
   unsigned stackSize;           /* Stack size of heartbeat and MsgIn threads */
   unsigned tidHeart;            /* Thread id of the heartbeat thread */
   unsigned tidMsgIn;            /* Thread id of the MsgIn thread */
   unsigned tidKill;             /* Thread id of the KillProcess thread */

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );


/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
        printf( "Usage: coaxtoring <configfile>\n" );
        return -1;
   }

/* Read configuration parameters
   *****************************/
   coaxtoring_config( argv[1] );
   logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );

/* Look up other important info from earthworm.h tables
   ****************************************************/
   if ( GetLocalInst( &InstId ) < 0 ) {
      printf( "coaxtoring: Error getting local installation id. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      printf( "coaxtoring: Invalid message type <TYPE_HEARTBEAT>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      printf( "coaxtoring: Invalid message type <TYPE_ERROR>. Exiting.\n" );
      return -1;
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, LogSwitch );

/* Log the configuration file
   **************************/
   LogConfig();

/* Allocate message buffers
   ************************/
   if ( MsgInInit() < 0 )
   {
      logit( "e", "coaxtoring: MsgInInit error. Exiting.\n" );
      return -1;
   }

/* Initialize the socket system
   ****************************/
   SocketSysInit();

/* Initialize the socket receiver
   ******************************/
   if ( socketInit( InAddress, PortNumber, RcvBufSize ) < 0 )
   {
      logit( "et", "coaxtoring: socketInit error. Exiting.\n" );
      return -1;
   }

/* Attach to the transport layer
   *****************************/
   tport_attach( &region, RingKey );

/* Create semaphore which is set when a message is fully built
   ***********************************************************/
   CreateSemaphore_ew();

/* Start the Heartbeat thread
   **************************/
   myPid = getpid();
   if( myPid == -1 )
   {
    logit("e","coaxtoring: Cannot get pid. Exiting.\n");
    exit (-1);
  }
   
   stackSize = 4096;
   if ( StartThread( Heartbeat, stackSize, &tidHeart ) == -1 )
   {
      printf( "coaxtoring: Error starting Heartbeat thread. Exiting.\n" );
      exit (-1);
   }

/* Start the KillProcess thread
   ****************************/
   stackSize = 4096;
   if ( StartThread( KillProcess, stackSize, &tidKill ) == -1 )
   {
      printf( "coaxtoring: Error starting KillProcess thread. Exiting.\n" );
      return -1;
   }

/* Start the MsgIn thread
   **********************/
   stackSize = 8192;
   if ( StartThread( MsgIn, stackSize, &tidMsgIn ) == -1 )
   {
      printf( "coaxtoring: Error starting MsgIn thread. Exiting.\n" );
      return -1;
   }

/* Keep getting messages
   *********************/
   while ( 1 )
   {

/* Wait for MsgIn to post semaphore when a message is complete
   ***********************************************************/
      WaitSemPost();

/* Find the next full buffer
   *************************/
      for ( j = 0; j < nMsgBuf; j++ )
         if ( MsgPtr[j].status  == BUFFER_FULL &&
              MsgPtr[j].pSeqNum == pSeqNumB )
            break;

      if ( j == nMsgBuf )
         logit( "ot", "coaxtoring: Internal error\n" );

      pSeqNumB++;
      pMsgOut = (MSG_INFO *)&MsgPtr[j];

/* Send message to transport ring buffer
   *************************************/
      if ( pMsgOut->logo.mod == MyModuleId )
         res = tport_putmsg( &region, &pMsgOut->logo, pMsgOut->length,
                              (char *)pMsgOut->start );
      else
         res = tport_copyto( &region, &pMsgOut->logo, pMsgOut->length,
                              (char *)pMsgOut->start, pMsgOut->cSeqNum );

      if ( res != PUT_OK )
         logit( "et", "coaxtoring: Error sending msg to transport ring: region %d\n",
                 region.key);


/* Print some header values
   ************************/
      if ( ScrnMsg == 1 ) {
         if ( pMsgOut->logo.mod == MyModuleId ) {
            printf( "%s", strchr( (char *)pMsgOut->start, (int)'c' ) );
         }
         else
         {
            printf( "inst:%u",   pMsgOut->logo.instid );
            printf( " mid:%2u",  pMsgOut->logo.mod );
            printf( " typ:%2u",  pMsgOut->logo.type );
            printf( " len:%5u",  pMsgOut->length );
            printf( " cseq:%3u", pMsgOut->cSeqNum );
            printf( "\n" );
         }
      }

/* We're done with this buffer.  Flag it as unused.
   ***********************************************/
      pMsgOut->status = BUFFER_UNUSED;
      nBufferUsed--;
   }
}


/************************************************************************
 *                                MsgIn()                               *
 *                                                                      *
 * This function assembles Earthworm packets into messages.  It will    *
 * build messages in a bunch of buffers which are described by an array *
 * of structures called MsgPtr's.  The program assumes that packets and *
 * messages are received in order.  MessageIn runs as a separate thread.*
 * When it finishes assembling a message, it posts a semaphore.         *
 * The main program blocks on this semaphore.                           *
 ************************************************************************/

thr_ret MsgIn( void *dummy )
{
   static int nMsgType = 0;
   static struct                    /* Used to track lost messages */
   {
      MSG_LOGO      logo;
      unsigned char msgSeqNum;
   } trak[MAX_MSGTYPE];

   char     errMsg[100];
   MSG_INFO *pMsgIn = NULL;         /* Pointer to input buffer */
   PACKET   packet;                 /* The packet header */
   char     *nxtPacketBytePtr;
   unsigned textLen;                /* Bytes of text in incoming packet */
   int      packetLen;              /* Total length of incoming packet */
   int      j, k;
   int      nLostMsg;               /* Number of lost messages (+n*256) */
   int      startup;                /* 1 if we're synching up,
                                       0 if collecting packets */
   int      track;                  /* 1 if this logo is being tracked,
                                       0 if not */

/* Top of loop waiting for packets
   *******************************/
   while ( 1 )
   {

/* Get a packet (receiver() blocks the program)
   ********************************************/
      packetLen = receiver( &packet );

      if ( packetLen == -1 )
      {
         ReportError( 1, "Receiver error" );
         continue;                             /* Get another packet */
      }
      textLen = packetLen - UDP_HDR;           /* Bytes of cargo in packet */

/* Is a message with the logo of this packet already active?
   *********************************************************/
      startup = 1;
      for ( j = 0; j < nMsgBuf; j++ )
         if ( MsgPtr[j].status      == BUFFER_PARTLYFILLED &&
              MsgPtr[j].logo.type   == packet.msgType &&
              MsgPtr[j].logo.mod    == packet.modId &&
              MsgPtr[j].logo.instid == packet.msgInst )
         {
            if ( MsgPtr[j].cSeqNum == packet.msgSeqNum )
            {
               pMsgIn = (MSG_INFO *)&MsgPtr[j];
               startup = 0;                         /* This is the right message */
            }
            else                                    /* End of message lost */
            {
               MsgPtr[j].status = BUFFER_UNUSED;    /* Discard this message */
               nBufferUsed--;
            }
            break;
         }

/* Are we in startup mode?
   ***********************/
      if ( startup )              /* Waiting for the start of a new message */
      {
         if ( packet.fragNum != 0 )
            continue;                   /* Wait for first packet of message */
         startup = 0;                          /* No longer in startup mode */

/* Store header stuff in a message buffer header
   *********************************************/
         j = GetBuf();                       /* Grab an unused buffer */
         if ( j == -1 )                      /* No unused buffers available */
         {
            logit( "et", "coaxtoring/MsgIn: ERROR; Out of message buffers\n" );
            continue;
         }
         pMsgIn                = (MSG_INFO *)&MsgPtr[j];
         pMsgIn->logo.instid   = packet.msgInst;
         pMsgIn->logo.type     = packet.msgType;
         pMsgIn->logo.mod      = packet.modId;
         pMsgIn->nxtMsgBytePtr = (char *)pMsgIn->start;
         pMsgIn->cSeqNum       = packet.msgSeqNum;
         pMsgIn->msgByteCnt    = 0;                   /* Message length so far */
         pMsgIn->nxtFragNum    = 0;
         pMsgIn->status        = BUFFER_PARTLYFILLED;    /* Mark buffer in use */
         if ( ++nBufferUsed > maxBufferUsed )
            maxBufferUsed = nBufferUsed;
      }

/* Not in startup mode. Is this the expected packet fragment?
   **********************************************************/
      else
         if ( packet.fragNum != pMsgIn->nxtFragNum )
         {                                           /* Broken message */
            MsgPtr[j].status = BUFFER_UNUSED;        /* Discard this message */
            nBufferUsed--;
            continue;
         }
      pMsgIn->nxtFragNum++;           /* This was the next expected packet */

/* Is there room in the message buffer?
   If not, discard the message.
   ************************************/
      if ( pMsgIn->msgByteCnt + textLen > MsgMaxBytes )
      {
         sprintf( errMsg, "inst:%u mid:%2u typ:%2u  Message buffer overflow",
                  pMsgIn->logo.instid, pMsgIn->logo.mod, pMsgIn->logo.type );
         ReportError( 2, errMsg );
         MsgPtr[j].status = BUFFER_UNUSED;
         nBufferUsed--;
         continue;
      }

/* Tack on this packet's data to the message
   *****************************************/
                                                      /* Update msg length */
      pMsgIn->msgByteCnt = (unsigned short)(pMsgIn->msgByteCnt + textLen);
      nxtPacketBytePtr = packet.text;

      memcpy( (char *)pMsgIn->nxtMsgBytePtr, nxtPacketBytePtr, textLen );
      pMsgIn->nxtMsgBytePtr += textLen;

/* Was this the last packet of the message?
   ****************************************/
      if ( packet.lastOfMsg == 1 )             /* Yes! */
      {

/* Did we lose any messages?
   *************************/
         track = 0;
         for ( k = 0; k < nMsgType; k++ )              /* Look up this logo */
         {                                             /* in tracking list. */
            if ( trak[k].logo.type   == packet.msgType &&
                 trak[k].logo.mod    == packet.modId &&
                 trak[k].logo.instid == packet.msgInst )
            {
               track    = 1;
               nLostMsg = packet.msgSeqNum - trak[k].msgSeqNum;

               if ( nLostMsg != 0 )                   /* Report lost message */
               {
                  if ( nLostMsg < 0 )
                     nLostMsg += 256;
                  sprintf( errMsg, "inst:%u mid:%2u typ:%2u seq:%u  %d lost message",
                           pMsgIn->logo.instid, pMsgIn->logo.mod,
                           pMsgIn->logo.type, packet.msgSeqNum, nLostMsg );
                  if ( nLostMsg > 1 )                      /* Make it plural */
                     strcat( errMsg, "s" );
                  ReportError( 3, errMsg );
                  logit( "t", "%s\n", errMsg );
               }
               trak[k].msgSeqNum = packet.msgSeqNum;
               trak[k].msgSeqNum++;
               break;
            }
         }

         if ( !track && nMsgType < MAX_MSGTYPE )  /* Start tracking this logo */
         {
            trak[nMsgType].logo.type   = packet.msgType;
            trak[nMsgType].logo.mod    = packet.modId;
            trak[nMsgType].logo.instid = packet.msgInst;
            trak[nMsgType].msgSeqNum   = packet.msgSeqNum;
            trak[nMsgType].msgSeqNum++;
            nMsgType++;
         }

/* Update buffer values and post semaphore
   ***************************************/
         pMsgIn->length  = pMsgIn->msgByteCnt; /* Set the message length */
         pMsgIn->pSeqNum = pSeqNumA++;         /* Set the program seq number */
         pMsgIn->status  = BUFFER_FULL;        /* Mark message as complete */

         PostSemaphore();                      /* Post the event semaphore */
      }
   }
}


 /******************************************************************
  *                          MsgInInit()                           *
  ******************************************************************/

int MsgInInit( void )
{
   MSG_INFO *ptr;
   int i;

/* Allocate an array of message buffer headers
   *******************************************/
   MsgPtr = (volatile MSG_INFO *) malloc( nMsgBuf * sizeof(MSG_INFO) );
   if ( MsgPtr == (volatile MSG_INFO *)NULL )
   {
      logit( "et", "coaxtoring: Message buffer header allocation failure.\n" );
      return -1;
   }

/* Allocate space for message buffers
   **********************************/
   for ( i = 0; i < nMsgBuf; i++ )
   {
      ptr = (MSG_INFO *)&MsgPtr[i];
      ptr->start = (volatile char *) malloc( (unsigned long)MsgMaxBytes );
      if ( ptr->start == (volatile char *)NULL )
      {
         logit( "et", "coaxtoring: Message buffer allocation failure.\n" );
         return -1;
      }

/* Initialize buffer status to empty
   *********************************/
      ptr->status = BUFFER_UNUSED;
   }

/* Initialize the buffer status counters
   *************************************/
   nBufferUsed   = 0;
   maxBufferUsed = 0;
   return 0;
}


  /*******************************************************************
   *                        coaxtoring_config()                      *
   *                                                                 *
   *  Processes command file using kom.c functions.                  *
   *  Exits if any errors are encountered.                           *
   *******************************************************************/

void coaxtoring_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process */
   char     init[11];     /* Init flags, one byte for each required command */
   int      nmiss;        /* Number of required commands that were missed */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
   ***************************************************/
   ncommand = 11;
   for( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
        logit( "e", "coaxtoring: Error opening command file <%s>. Exiting.\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
   *************************/
   while ( nfiles > 0 )           /* While there are command files open */
   {
      while ( k_rd() )          /* Read next line from active file  */
      {
          com = k_str();        /* Get the first token from line */

/* Ignore blank lines & comments
   *****************************/
          if( !com )          continue;
          if( com[0] == '#' ) continue;

/* Open a nested configuration file
   ********************************/
          if( com[0] == '@' )
          {
             success = nfiles + 1;
             nfiles  = k_open( &com[1] );
             if ( nfiles != success )
             {
                logit( "e", "coaxtoring: Error opening command file <%s>. Exiting.\n",
                         &com[1] );
                exit( -1 );
             }
             continue;
          }

/* Process anything else as a command
   **********************************/
   /*0*/  if ( k_its( "nMsgBuf" ) ) {            /* number of buffers to allocate */
             nMsgBuf = k_int();
             init[0] = 1;
          }
   /*1*/  else if ( k_its( "MsgMaxBytes" ) ) {   /* maximum message size (bytes) */
             MsgMaxBytes = (unsigned) k_int();
             init[1] = 1;
          }
   /*2*/  else if ( k_its( "InAddress" ) ) {     /* IP address to read from */
             str = k_str();
             if (str) strcpy( InAddress, str );
             init[2] = 1;
          }
   /*3*/  else if ( k_its( "PortNumber" ) ) {    /* Port Number to read from */
             PortNumber = k_int();
             init[3] = 1;
          }
   /*4*/  else if ( k_its( "ScrnMsg" ) ) {       /* screen message flag */
             ScrnMsg = k_int();
             init[4] = 1;
          }
   /*5*/  else if ( k_its( "HeartbeatInt" ) ) {  /* heartbeat interval (sec) */
             HeartbeatInt = k_int();
             init[5] = 1;
          }
   /*6*/  else if ( k_its( "MyModuleId" ) ) {    /* set module id */
             str = k_str();
             if ( str )
             {
                if ( GetModId( str, &MyModuleId ) < 0 )
                {
                   logit( "e", "coaxtoring: Invalid MyModuleId <%s> in <%s>",
                            str, configfile );
                   printf( ". Exiting.\n" );
                   exit( -1 );
                }
             }
             init[6] = 1;
          }
   /*7*/  else if ( k_its( "LogFile" ) ) {    /* set log switch */
             LogSwitch = k_int();
             init[7] = 1;
          }
   /*8*/  else if ( k_its( "RingName" ) ) {   /* which transport ring to write to */
             str = k_str();
             if (str) {
                if ( ( RingKey = GetKey(str) ) == -1 ) {
                   logit( "e", "coaxtoring: Invalid RingName <%s> in <%s>",
                            str, configfile );
                   logit( "e", ". Exiting.\n" );
                   exit( -1 );
                }
             }
             init[8] = 1;
          }
   /*9*/  else if ( k_its( "BufferReportInt" ) )  /* Buffer status report interval */
          {
             BufferReportInt = k_int();
             init[9] = 1;
          }
/*10*/    else if ( k_its( "RcvBufSize" ) )       /* Size of IP receive buffer */
          {
             RcvBufSize = k_int();
             init[10] = 1;
          }
          else
          {
             logit( "e", "coaxtoring: <%s> unknown command in <%s>.\n",
                     com, configfile );
             continue;
          }

/* See if there were any errors processing the command
   ***************************************************/
          if( k_err() ) {
             logit( "e", "coaxtoring: Bad <%s> command in <%s>. Exiting.\n",
                      com, configfile );
             exit( -1 );
          }
      }
      nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
   ****************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss )
   {
       logit( "e", "coaxtoring: ERROR, no " );
       if ( !init[0] )  logit( "e", "<nMsgBuf> "         );
       if ( !init[1] )  logit( "e", "<MsgMaxBytes> "     );
       if ( !init[2] )  logit( "e", "<InAddress> "       );
       if ( !init[3] )  logit( "e", "<PortNumber> "      );
       if ( !init[4] )  logit( "e", "<ScrnMsg> "         );
       if ( !init[5] )  logit( "e", "<HeartbeatInt> "    );
       if ( !init[6] )  logit( "e", "<MyModuleId> "      );
       if ( !init[7] )  logit( "e", "<LogFile> "         );
       if ( !init[8] )  logit( "e", "<RingName> "        );
       if ( !init[9] )  logit( "e", "<BufferReportInt> " );
       if ( !init[10] ) logit( "e", "<RcvBufSize> "      );
       logit( "e", "command(s) in <%s>. Exiting.\n", configfile );
       exit( -1 );
   }
   return;
}


/******************************************************************
 *                            ReportError                         *
 ******************************************************************/

void LogConfig( void )
{
   logit( "",  "nMsgBuf:         %d\n", nMsgBuf );
   logit( "",  "MsgMaxBytes:     %u\n", MsgMaxBytes );
   logit( "",  "InAddress:       %s\n", InAddress );
   logit( "",  "PortNumber:      %d\n", PortNumber );
   logit( "",  "ScrnMsg:         %d\n", ScrnMsg );
   logit( "",  "HeartbeatInt:    %d\n", HeartbeatInt );
   logit( "",  "MyModuleId:      %u\n", MyModuleId );
   logit( "",  "LogFile:         %d\n", LogSwitch );
   logit( "",  "RingKey:         %d\n", RingKey );
   logit( "",  "BufferReportInt: %d\n", BufferReportInt );
   logit( "",  "RcvBufSize:      %d\n", RcvBufSize );
   return;
}


/******************************************************************
 *                            ReportError                         *
 ******************************************************************/

int ReportError( int errNum, char *errStr )
{
   int j;
   MSG_INFO *pMsg;                /* Pointer to buffer */
   time_t tstamp;

   j = GetBuf();                  /* Get an unused buffer */
   if ( j == -1 )                 /* No buffer is available */
   {
      logit( "et", "coaxtoring/ReportError: ERROR: Out of message buffers\n" );
      return -1;
   }

/* Build ASCII error message
   *************************/
   pMsg = (MSG_INFO *)&MsgPtr[j];
   time( &tstamp );
   sprintf( (char *)pMsg->start, "%ld %d %s\n", (long)tstamp, errNum, errStr );

   pMsg->logo.instid = InstId;
   pMsg->logo.mod    = MyModuleId;
   pMsg->logo.type   = TypeError;
   pMsg->length      = (unsigned short)strlen( (char *)pMsg->start );
   pMsg->cSeqNum     = 0;                      /* Set the coax seq number */
   pMsg->pSeqNum     = pSeqNumA++;             /* Set the program seq number */
   pMsg->status      = BUFFER_FULL;            /* Mark buffer full */
   if ( ++nBufferUsed > maxBufferUsed )
      maxBufferUsed = nBufferUsed;

/* Let the main thread know that a message is ready
   ************************************************/
   PostSemaphore();
   return j;
}


/*****************************************************************
 *                              GetBuf                           *
 *               Return the index of an unused buffer            *
 *****************************************************************/

int GetBuf( void )
{
   int i, j;

   j = -1;
   for ( i = 0; i < nMsgBuf; i++ )
      if ( MsgPtr[i].status == BUFFER_UNUSED )
      {
         j = i;
         break;
      }
   return j;
}


/*****************************************************************
 *                            Heartbeat                          *
 *              Send a heartbeat to the transport ring           *
 *****************************************************************/

thr_ret Heartbeat( void *dummy )
{
   char     msg[20];
   unsigned short length;
   time_t   ltime;
   MSG_LOGO logo;
   int      i = 0;
   int      j = 0;

/* Newly added in on Dec 27, 2002 to get a heartbeat out right at startup 
	There was a case where coaxtoring could get killed and not restarted
	by statmgr if it was killed before the first heartbeat.
*/
   time( &ltime );
   logo.instid = InstId;
   logo.mod    = MyModuleId;
   logo.type   = TypeHeartBeat;
   sprintf( msg, "%ld %d\n", (long) ltime, myPid );
   length = (unsigned short)strlen( msg );
   if ( tport_putmsg( &region, &logo, length, msg ) != PUT_OK )
   {
            logit( "et", "coaxtoring: Error sending heartbeat to transport ring\n" );
   }

   while( 1 )
   {
      sleep_ew( 1000 );

/* Log the buffer status counter
   *****************************/
      if ( ++j == BufferReportInt )
      {
         j = 0;
         logit( "et", "coaxtoring: High mark:%3d buffer", maxBufferUsed );
         if ( maxBufferUsed > 1 ) logit( "e", "s" );
         logit( "e", "\n" );
         maxBufferUsed = 0;
      }

/* Send a heartbeat
   ****************/
      if ( ++i == HeartbeatInt )
      {
         i = 0;
         time( &ltime );
         logo.instid = InstId;
         logo.mod    = MyModuleId;
         logo.type   = TypeHeartBeat;
         sprintf( msg, "%ld %d\n", (long) ltime, myPid );
         length = (unsigned short)strlen( msg );

         if ( tport_putmsg( &region, &logo, length, msg ) != PUT_OK )
            logit( "et", "coaxtoring: Error sending heartbeat to transport ring\n" );
      }
   }
}


/*****************************************************************
 *                          KillProcess                          *
 *          Watch the terminate flag in the memory ring.         *
 *  When it is set, kill the process.                            *
 *****************************************************************/

thr_ret KillProcess( void *dummy )
{
   while ( tport_getflag( &region ) != TERMINATE  &&
           tport_getflag( &region ) != myPid         )
      sleep_ew( 200 );

   logit( "t", "coaxtoring: Termination requested. Exiting.\n" );
   exit( 0 );
}
