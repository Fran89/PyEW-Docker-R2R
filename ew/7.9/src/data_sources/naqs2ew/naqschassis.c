/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqschassis.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/02/26 17:16:53  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.2  2002/07/09 18:11:55  dietz
 *     logit changes
 *
 *     Revision 1.1  2002/03/15 23:10:09  dietz
 *     Initial revision
 *
 */

/*
 *   naqschassis.c:  Chassis for any data streams client of NaqsServer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <socket_ew.h>
#include <trace_buf.h>
#include "naqschassis.h"

/* Functions used in this source file
 ************************************/
void     naqschassis_config( char * );        
void     naqschassis_heart( void );

/* Things to read or derive from configuration file
 **************************************************/
char     RingName[30];    /* name of transport ring for output          */
char     MyModName[30];   /* speak as this module name/id               */
int      LogSwitch;       /* 0 if no logfile should be written          */
int      HeartBeatInt;    /* seconds between heartbeats                 */
                          /* (to local ring)                            */
int      MaxSamplePerMsg; /* max# samples to pack into a tracebuf msg   */
long     MaxMsgSize;      /* max size for input/output msgs             */
char     NaqsIPAddr[20];  /* NaqsServer's IP address, in dot notation   */
int      NaqsPort;        /* NaqsServer's well-known port number        */
int      Debug;           /* Debug logging flag (optionally configured) */
char     ProgName[40];    /* program name for logging purposes          */
SHM_INFO Region;          /* shared memory region to use for output     */

unsigned int SocketTimeoutLength;  /* Length of timeouts on SOCKET_ew calls */
int      SOCKET_ewDebug;           /* set to 1 in for socket debug messages */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
long          RingKey;       /* key of transport ring for i/o     */
unsigned char MyInstId;      /* local installation id             */
unsigned char MyModId;       /* Module Id for this program        */
unsigned char TypeHeartBeat;
unsigned char TypeErrorn;

/* Globals:
 **********/
static int   ValidConnect = 0;    /* 1 if connected to server, 0 not connected  */
static int   HeartReady   = 0;    /* initialization flag for heartbeat          */
static char *SockBuf   = NULL;    /* buffer for incoming & outgoing socket msgs */
static int   SockBufLen   = 0;    /* length (bytes) of SockBuf (may be dynamic) */
static pid_t MyPid        = 0;    /* processid - use in heartbeat & termination */

#define CONNECT_RETRY_DT 10       /* Seconds wait between connect attempts      */
#define LOGFAIL_INTERVAL 10       /* log connection failures every Nth failure  */


int main( int argc, char **argv )
{
   NMXHDR         naqshd;              /* NMX message header structure    */
   char           errtxt[230];
   int            nretry = 0;          /* number of connection attempts    */
   int            msglen;
   int            rc;
   long           rctype;
   int            rcclient;
   int            firstconnect = 1;
   int            flag;
   char          *c;

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: %s <configfile>\n", argv[0] );
        return -1;
   }
   strcpy( ProgName, argv[1] );
   c = strchr( ProgName, '.' );
   if( c ) *c = '\0';

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 1024, 1 );
 
/* Read the configuration file(s)
 ********************************/
   naqschassis_config( argv[1] );
   logit( "" , "%s: Read command file <%s>\n", ProgName, argv[1] );

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, 1024, LogSwitch );

/* Get the process id for restart purposes 
 *****************************************/
   MyPid = getpid();
   if( MyPid == -1 )
   {  
      logit("e", "%s: Cannot get PID; exiting.\n", ProgName);
      exit( -1 );
   }  

/* Attach to Output shared memory ring
 *************************************/
   tport_attach( &Region, RingKey );
   logit( "", "%s: Attached to public memory region %s: %d\n",
          ProgName, RingName, RingKey );

/* Put out one heartbeat early
 *****************************/
   naqschassis_heart();

/* Allocate chassis message buffers; initialize client
 *****************************************************/
   if( naqsclient_init() ) 
   {
     logit("e", "%s: Error initializing; exiting!\n", ProgName );
     naqschassis_shutdown( -1 );
   }
   SockBufLen = MAX_TRACEBUF_SIZ;
   if( ( SockBuf = (char *) malloc(SockBufLen) ) == (char *) NULL )
   {
     logit("e", "%s: Cannot allocate %ld bytes for SockBuf; exiting!\n",
           ProgName, SockBufLen );
     naqschassis_shutdown( -1 );
   }

/*---------------------Main loop-------------------------*/

ReConnect:

/* Get a connection to NaqsServer
 ********************************/
   nretry = 0;

   while( !ValidConnect )
   {
      rc = nmxsrv_connect( NaqsIPAddr, NaqsPort );

      if( rc == NMX_SUCCESS )
      {
         logit("et","%s: attempt %d; connected to server %s:%d\n",
                ProgName, nretry+1, NaqsIPAddr, NaqsPort );
         if( !firstconnect )
         {
            naqschassis_error( ERR_RECONNECT, 
                              "Connection NaqsServer reestablished",
                               time(NULL) );
         }
         firstconnect = 0;
         ValidConnect = 1;
      }
      else
      {
         if( nretry % LOGFAIL_INTERVAL == 0 )
         {
            logit("et","%s: attempt %d; failed to connect to server %s:%d; "
                       "will retry every %d seconds.\n",
                   ProgName, nretry+1, NaqsIPAddr, NaqsPort, CONNECT_RETRY_DT );
         }
         nretry++;
         sleep_ew( CONNECT_RETRY_DT * 1000 );
      }
      naqschassis_heart();
      flag = tport_getflag(&Region);
      if( flag == TERMINATE  ||  flag == MyPid ) naqschassis_shutdown( 0 );
   }

/* NAQS client startup protocol:
   1) send "connect" message
 **********************************/
   nmx_bld_connect( &SockBuf, &SockBufLen, &msglen );
   nmxsrv_sendto( SockBuf, msglen );

/* Loop on reading from NAQS;
   read a message from the socket and process it accordingly
 ***********************************************************/
   while( ValidConnect )
   {
       rctype = nmxsrv_recvmsg( &naqshd, &SockBuf, &SockBufLen );

     /* give client code first crack at the new packet
      ************************************************/
       rcclient = naqsclient_process( rctype, &naqshd, &SockBuf, &SockBufLen );
       if( rcclient )
       {
       /* the client processed the message in some manner 
          should add logging and error checking here...*/
       }

    /* error reported by NaqsServer
     ******************************/
       else if( rctype == NMXMSG_ERROR )
       {
          logit("et","%s: received error message from NaqsServer:\n", ProgName );
          logit("e", "         %s\n", SockBuf );
       }

    /* termination notice from NaqsServer
     ************************************/
       else if( rctype == NMXMSG_TERMINATE )
       {
          NMX_TERMINATE terminate;

          nmx_rd_terminate( &naqshd, SockBuf, &terminate );
          if( terminate.note != NULL ) {
             sprintf(errtxt,"NaqsServer terminated connection; reason:%d (%s)",
                     terminate.reason, terminate.note );
          }
          else {
             sprintf(errtxt,"NaqsServer terminated connection; reason:%d\n",
                     terminate.reason );
          }
          naqschassis_error( ERR_NOCONNECT, errtxt, time(NULL) );
          nmxsrv_disconnect();
          ValidConnect = 0;
          goto ReConnect;
       }

    /* some kind of trouble with socket
     **********************************/
       else if( rctype == NMX_FAILURE )
       {
          naqschassis_error( ERR_NOCONNECT, 
                        "trouble with socket to NaqsServer; disconnecting!",
                         time(NULL) );
          nmxsrv_disconnect();
          ValidConnect = 0;
          goto ReConnect;
       }

    /* unknown message type or return code
     *************************************/
       else
       {
          logit("et", "%s: unknown return (%d) from nmxsrv_recvmsg()\n",
                 ProgName, rctype );
       }

    /* Beat heart & see if an we've been asked to terminate
     ******************************************************/
       naqschassis_heart();
       flag = tport_getflag(&Region);
       if( flag == TERMINATE  ||  flag == MyPid ) break;

     /*sleep_ew(100);*/    /* sleep shouldn't be necessary because
                              we're blocking on the socket read */
   } /* end of while( ValidConnect ) */

   naqschassis_shutdown( 0 );
   return( 0 );
}

/*****************************************************************************
 *  naqschassis_config() processes command file(s) using kom.c functions;    *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
#define ncommand  5        /* # of required commands you expect to process   */

void naqschassis_config( char *configfile )
{
   char   init[ncommand];  /* init flags, one byte for each required command */
   int    nmiss;           /* number of required commands that were missed   */
   char  *com;
   char  *str;
   char   processor[20];
   int    nfiles;
   int    success;
   int    i;

/* Set variables to default values
 *********************************/
   Debug          = 0;
   SOCKET_ewDebug = 0;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
               "%s: Error opening command file <%s>; exiting!\n",
                ProgName, configfile );
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
                  logit( "e",
                         "%s: Error opening command file <%s>; exiting!\n",
                          ProgName, &com[1] );
                  exit( -1 );
               }
               continue;
            }
            strcpy( processor, "naqschassis_config" );

         /* Process anything else as a command
          ************************************/
  /*0*/     if( k_its("MyModuleId") ) {
                str=k_str();
                if(str) strcpy(MyModName,str);
                init[0] = 1;
            }
  /*1*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_int();
                init[2] = 1;
            }
  /*3*/     else if(k_its("LogFile") ) {
                LogSwitch=k_int();
                init[3]=1;
            }

  /*4*/  /* NaqsServer's IP address and port */
            else if(k_its("NaqsServer") ) {
                str=k_str();
                if(str) strcpy(NaqsIPAddr,str);
                NaqsPort = k_int();
                init[4]=1;
            }

         /* OPTIONAL: Debug flag for logging */
            else if(k_its("Debug") ) {
               Debug = k_int();
            }

         /* OPTIONAL: Socket timeout length */
            else if(k_its("SocketTimeout") ) {
               SocketTimeoutLength = k_int();
            }

         /* OPTIONAL:  Socket Debug Flag */
            else if(k_its("SocketDebug") ) {
                SOCKET_ewDebug = k_int();
                setSocket_ewDebug(SOCKET_ewDebug);
            }

         /* Pass it off to the filter's config processor
          **********************************************/
            else if( naqsclient_com( configfile ) ) {
                strcpy( processor, "naqsclient_com" );
            }

         /* Unknown command
          *****************/
            else {
                logit( "e", "%s: <%s> Unknown command in <%s>.\n",
                         ProgName, com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                      "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
                       ProgName, com, processor, configfile );
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
       logit( "e", "%s: ERROR, no ", ProgName );
       if ( !init[0] )   logit( "e", "<MyModuleId> "       );
       if ( !init[1] )   logit( "e", "<RingName> "         );
       if ( !init[2] )   logit( "e", "<HeartBeatInt> "     );
       if ( !init[3] )   logit( "e", "<LogFile> "          );
       if ( !init[4] )   logit( "e", "<NaqsServer> "       );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
      logit( "e",
             "%s:  Invalid ring name <%s>; exiting!\n", ProgName, RingName);
      exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &MyInstId ) != 0 ) {
      logit( "e",
             "%s: error getting local installation id; exiting!\n", ProgName);
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      logit( "e",
             "%s: Invalid module name <%s>; exiting!\n", ProgName, MyModName);
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e",
             "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", ProgName);
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeErrorn ) != 0 ) {
      logit( "e",
             "%s: Invalid message type <TYPE_ERROR>; exiting!\n", ProgName);
      exit( -1 );
   }

   return;
}

/*******************************************************************************
 * naqschassis_heart() puts a heartbeat msg into shared memory if it's time    *
 *******************************************************************************/
void naqschassis_heart( void )
{
/* Initialize these variables once: */
   static MSG_LOGO   hrtlogo;
   static time_t     timelastbeat;

/* Working variables: */
   time_t            timenow;
   char              msg[56];

/* Initialize static variables on first call
 *******************************************/
   if( !HeartReady )
   {
   /* Set up the heartbeat logo: */
      hrtlogo.instid = MyInstId;
      hrtlogo.mod    = MyModId;
      hrtlogo.type   = TypeHeartBeat;

   /* Force heartbeat on first call: */
      timelastbeat = 0;
      HeartReady   = 1;
   }

/* Skip out if it's too soon
 ***************************/
   time(&timenow);
   if( (timenow-timelastbeat) < HeartBeatInt )  return;

/* Otherwise, build the heartbeat & write it to shared memory
 ************************************************************/
   sprintf( msg, "%ld %ld\n\0", (long) timenow, (long) MyPid );
   if( tport_putmsg( &Region, &hrtlogo, (long)strlen(msg), msg ) != PUT_OK )
   {
      logit("et", "%s:  Error putting heartbeat in %s\n",
             ProgName, RingName );
   }
   timelastbeat = timenow;

   return;
}

/*******************************************************************************
 * naqschassis_error() puts error msg into shared memory and writes it to log  *
 *******************************************************************************/
void naqschassis_error( short ierr, char *note, time_t t )
{
   MSG_LOGO   errlogo;
   char       msg[256];

   errlogo.instid = MyInstId;
   errlogo.mod    = MyModId;
   errlogo.type   = TypeErrorn;

   sprintf( msg, "%ld %hd %s\n", (long)t, ierr, note );
   logit( "et", "%s: %s\n", ProgName, note );

   if( tport_putmsg( &Region, &errlogo, (long)strlen(msg), msg ) != PUT_OK )
   {
      logit("et","%s:  Error sending error:%d to ring\n", ProgName, ierr );
   }

   return;
}

/*******************************************************************************
 * naqschassis_shutdown()  frees malloc'd memory, detaches from shared memory, *
 *                     does other cleanup stuff, and exits                     *
 *******************************************************************************/
void naqschassis_shutdown( int exitstatus )
{
   int            msglen;
   NMX_TERMINATE terminate;

/* Let client do its shutdown things
 ***********************************/
   naqsclient_shutdown();

/* Send a termination message to NaqsServer
 ******************************************/
   if( ValidConnect )
   {
      if( exitstatus ) terminate.reason = NMX_ERROR_SHUTDOWN;
      else             terminate.reason = NMX_NORMAL_SHUTDOWN;
      terminate.note = NULL;

      nmx_bld_terminate( &SockBuf, &SockBufLen, &msglen, &terminate );
      nmxsrv_sendto( SockBuf, msglen );
      if(Debug) logit("et","%s: sent termination message to NaqsServer\n", ProgName);
      nmxsrv_disconnect();
   }

/* Free all allocated memory
 ***************************/
   free( SockBuf );

/* Detach from transport ring
 ****************************/
   tport_detach( &Region );

   if( exitstatus == 0 )
   {
      logit("t", "%s:  Termination requested; exiting.\n",
               ProgName );
   }
   else
   {
      logit("t", "%s:  fatal error %d; exiting.\n",
               ProgName, exitstatus );
   }

   exit( exitstatus );
}
