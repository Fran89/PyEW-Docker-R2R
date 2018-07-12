/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dschassis.c 4512 2011-08-09 18:25:58Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/02/26 17:16:53  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.1  2003/01/30 23:11:44  lucky
 *     Initial revision
 *
 *     Revision 1.1  2002/03/15 23:10:09  dietz
 *     Initial revision
 *
 */

/*
 *   dschassis.c:  Chassis for any client of NMX DataServer.
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
#include "dschassis.h"

/* Functions used in this source file
 ************************************/
void     dschassis_config( char * );        
void     dschassis_heart( void );
void     dschassis_shutdown( int exitstatus );
void     dschassis_error( short ierr, char *note, time_t t );

/* Things to read or derive from configuration file
 **************************************************/
char     RingName[30];    /* name of transport ring for output          */
char     MyModName[30];   /* speak as this module name/id               */
int      LogSwitch;       /* 0 if no logfile should be written          */
int      HeartBeatInt;    /* seconds between heartbeats                 */
                          /* (to local ring)                            */
int      MaxSamplePerMsg; /* max# samples to pack into a tracebuf msg   */
long     MaxMsgSize;      /* max size for input/output msgs             */
char     DataServerIPAddr[20];  /* DataServer's IP address, in dot notation   */
int      DataServerPort;        /* DataServer's well-known port number        */
int      Debug;           /* Debug logging flag (optionally configured) */
char     ProgName[40];    /* program name for logging purposes          */
SHM_INFO Region;          /* shared memory region to use for output     */

unsigned int SocketTimeoutLength;  /* Length of timeouts on SOCKET_ew calls */
int      SOCKET_ewDebug;           /* set to 1 in for socket debug messages */
NMX_CONNECT_REQUEST ConReq; /* Connect Request structure */


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
   long           tconnect = 0;
   int            rcclient = 0;
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

/* Read the configuration file(s)
 ********************************/
   dschassis_config( argv[1] );  

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], (short) MyModId, 1024, LogSwitch );
   logit( "" , "%s: Read command file <%s>\n", ProgName, argv[1] );

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
   dschassis_heart();

/* Allocate chassis message buffers; initialize client
 *****************************************************/
/*  if( dsclient_init() ) 
   {
     logit("e", "%s: Error initializing; exiting!\n", ProgName );
     dschassis_shutdown( -1 ); 
   } */
   SockBufLen = MAX_TRACEBUF_SIZ;
   if( ( SockBuf = (char *) malloc(SockBufLen) ) == (char *) NULL )
   {
     logit("e", "%s: Cannot allocate %ld bytes for SockBuf; exiting!\n",
           ProgName, SockBufLen );
    dschassis_shutdown( -1 ); 
   }

/*---------------------Main loop-------------------------*/

ReConnect:

/* 1) Get a connection to DataServer
 ***********************************/
   nretry = 0;

   while( !ValidConnect )
   {
      rc = nmxsrv_connect( DataServerIPAddr, DataServerPort );

      if( rc == NMX_SUCCESS )
      {
         logit("et","%s: attempt %d; connected to server %s:%d\n",
                ProgName, nretry+1, DataServerIPAddr, DataServerPort );
         if( !firstconnect )
         {
            dschassis_error( ERR_RECONNECT, 
                             "Connection DataServer reestablished",
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
                   ProgName, nretry+1, DataServerIPAddr, 
                   DataServerPort, CONNECT_RETRY_DT );
         }
         nretry++;
         sleep_ew( CONNECT_RETRY_DT * 1000 );
      }
      dschassis_heart(); 
      flag = tport_getflag(&Region);
      if( flag == TERMINATE  ||  flag == MyPid ) dschassis_shutdown( 0 );
   }

/* DataServer client startup protocol:
   2) read connection time from socket (4-byte int)
 **************************************************/
   nmxsrv_recvconnecttime( &ConReq.tconnect );

/* DataServer client startup protocol:
   3) send ConnectRequest message to DataServer
 ***********************************************/
   if(Debug) {
     logit("e","DataServer User/Password: %s %s\n", ConReq.user, ConReq.pswd );
     logit("e","DataServer Protocol Version: %d\n", ConReq.DAPversion );
     logit("e","tconnect: %ld  %s", 
                ConReq.tconnect, ctime( (long*)&ConReq.tconnect) );
   }
   nmx_bld_connectrequest( &SockBuf, &SockBufLen, &msglen, &ConReq );
   nmxsrv_sendto( SockBuf, msglen );

/* Loop on reading/requesting from DataServer;
   read a message from the socket and process it accordingly.
   4) wait for a "Ready" msg from the DataServer
   5) send a "Request" msg to the DataServer,
   6) read & process the response msg(s), until receiving another 
      "Ready" msg. The "Ready" msg indicates the end of the requested
       data.  Each request may elicit 0 or more response msgs.
 *********************************************************************/
   while( ValidConnect )
   {
       rctype = nmxsrv_recvmsg( &naqshd, &SockBuf, &SockBufLen );

     /* give client code first crack at the new packet
      ************************************************/
/*       rcclient = dsclient_process( rctype, &naqshd, &SockBuf, &SockBufLen ); */
       if( rcclient )
       {
       /* the client processed the message in some manner 
          should add logging and error checking here...*/
       }

    /* error reported by DataServer
     ******************************/
       else if( rctype == NMXMSG_READY )
       {
          logit("et","%s: received ready message from DataServer:\n", ProgName );
          logit("e", "         %s\n", SockBuf );
       }

    /* error reported by DataServer
     ******************************/
       else if( rctype == NMXMSG_ERROR )
       {
          logit("et","%s: received error message from DataServer:\n", ProgName );
          logit("e", "         %s\n", SockBuf );
       }

    /* termination notice from DataServer
     ************************************/
       else if( rctype == NMXMSG_TERMINATE )
       {
          NMX_TERMINATE terminate;

          nmx_rd_terminate( &naqshd, SockBuf, &terminate );
          if( terminate.note != NULL ) {
             sprintf(errtxt,"DataServer terminated connection; reason:%d (%s)",
                     terminate.reason, terminate.note );
          }
          else {
             sprintf(errtxt,"DataServer terminated connection; reason:%d\n",
                     terminate.reason );
          }
          dschassis_error( ERR_NOCONNECT, errtxt, time(NULL) ); 
          nmxsrv_disconnect();
          ValidConnect = 0;
/*          goto ReConnect; */
          dschassis_shutdown( -1 );
       }

    /* some kind of trouble with socket
     **********************************/
       else if( rctype == NMX_FAILURE )
       {
          dschassis_error( ERR_NOCONNECT, 
                        "trouble with socket to DataServer; disconnecting!",
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
          if(Debug) {
             int im;
             logit("e","naqshd sig: 0x%x  msgtype: %d  msglen: %d\n",
                    naqshd.signature, naqshd.msgtype, naqshd.msglen );
             for(im=0;im<naqshd.msglen;im++) logit("e","%d ", (int)SockBuf[im]);
             logit("e","\n" );
          }
       }

    /* Beat heart & see if an we've been asked to terminate
     ******************************************************/
       dschassis_heart();
       flag = tport_getflag(&Region);
       if( flag == TERMINATE  ||  flag == MyPid ) break;

     /*sleep_ew(100);*/    /* sleep shouldn't be necessary because
                              we're blocking on the socket read */
   } /* end of while( ValidConnect ) */

   dschassis_shutdown( 0 );
   return( 0 );
}

/*****************************************************************************
 *  dschassis_config() processes command file(s) using kom.c functions;      *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
#define ncommand  6        /* # of required commands you expect to process   */

void dschassis_config( char *configfile )
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
        fprintf( stderr,
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
                  fprintf( stderr,
                          "%s: Error opening command file <%s>; exiting!\n",
                           ProgName, &com[1] );
                  exit( -1 );
               }
               continue;
            }
            strcpy( processor, "dschassis_config" );

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

  /*4*/  /* DataServer's IP address and port */
            else if(k_its("DataServer") ) {
                str=k_str();
                if(str) strcpy(DataServerIPAddr,str);
                DataServerPort = k_int();
                init[4]=1;
            }

  /*5*/  /* DataServer's User and Password */
            else if(k_its("DSuser") ) {
                memset( &ConReq, 0, sizeof(NMX_CONNECT_REQUEST) );
                ConReq.DAPversion = 0;
                if(str=k_str()) {
                  if( strlen(str) >= NMX_DS_LEN ) {
                     fprintf(stderr,"Bad DSuser command; exiting\n");
                     exit(-1);
                  }
                  strcpy(ConReq.user,str);
                  if(str=k_str()) {
                    if( strlen(str) >= NMX_DS_LEN ) {
                       fprintf(stderr,"Bad DSuser command; exiting\n");
                       exit(-1);
                    }
                    strcpy(ConReq.pswd,str);
                    init[5]=1;
                  }
                }
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
/*            else if( dsclient_com( configfile ) ) {
                strcpy( processor, "naqsclient_com" );
            } */

         /* Unknown command
          *****************/
            else {
                fprintf( stderr, "%s: <%s> Unknown command in <%s>.\n",
                         ProgName, com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               fprintf( stderr,
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
       fprintf( stderr, "%s: ERROR, no ", ProgName );
       if ( !init[0] )   fprintf( stderr, "<MyModuleId> "       );
       if ( !init[1] )   fprintf( stderr, "<RingName> "         );
       if ( !init[2] )   fprintf( stderr, "<HeartBeatInt> "     );
       if ( !init[3] )   fprintf( stderr, "<LogFile> "          );
       if ( !init[4] )   fprintf( stderr, "<DataServer> "       );
       fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
      fprintf( stderr,
              "%s:  Invalid ring name <%s>; exiting!\n", ProgName, RingName);
      exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &MyInstId ) != 0 ) {
      fprintf( stderr,
              "%s: error getting local installation id; exiting!\n", ProgName);
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid module name <%s>; exiting!\n", ProgName, MyModName);
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", ProgName);
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeErrorn ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ERROR>; exiting!\n", ProgName);
      exit( -1 );
   }

   return;
}

/*******************************************************************************
 * dschassis_heart() puts a heartbeat msg into shared memory if it's time    *
 *******************************************************************************/
void dschassis_heart( void )
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
   if( tport_putmsg( &Region, &hrtlogo, strlen(msg), msg ) != PUT_OK )
   {
      logit("et", "%s:  Error putting heartbeat in %s\n",
             ProgName, RingName );
   }
   timelastbeat = timenow;

   return;
}

/*******************************************************************************
 * dschassis_error() puts error msg into shared memory and writes it to log  *
 *******************************************************************************/
void dschassis_error( short ierr, char *note, time_t t )
{
   MSG_LOGO   errlogo;
   char       msg[256];

   errlogo.instid = MyInstId;
   errlogo.mod    = MyModId;
   errlogo.type   = TypeErrorn;

   sprintf( msg, "%ld %hd %s\n", t, ierr, note );
   logit( "et", "%s: %s\n", ProgName, note );

   if( tport_putmsg( &Region, &errlogo, strlen(msg), msg ) != PUT_OK )
   {
      logit("et","%s:  Error sending error:%d to ring\n", ProgName, ierr );
   }

   return;
}

/*******************************************************************************
 * dschassis_shutdown()  frees malloc'd memory, detaches from shared memory, *
 *                     does other cleanup stuff, and exits                     *
 *******************************************************************************/
void dschassis_shutdown( int exitstatus )
{
   int            msglen;
   NMX_TERMINATE terminate;

/* Let client do its shutdown things
 ***********************************/
/*   naqsclient_shutdown(); */

/* Send a termination message to DataServer
 ******************************************/
   if( ValidConnect )
   {
      if( exitstatus ) terminate.reason = NMX_ERROR_SHUTDOWN;
      else             terminate.reason = NMX_NORMAL_SHUTDOWN;
      terminate.note = NULL;

      nmx_bld_terminate( &SockBuf, &SockBufLen, &msglen, &terminate );
      nmxsrv_sendto( SockBuf, msglen );
      if(Debug) logit("et","%s: sent termination message to DataServer\n", ProgName);
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
