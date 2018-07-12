
/*****************************************************************
 *                            sendfilemt                         *
 *  This program sends files to the getfileII program.  In this  *
 *  multithreaded version of sendfile, each thread sends to      *
 *  another getfileII process.                                   *
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include "sendfilemt.h"

/* Global variables
   ****************/
unsigned char InstId;
pid_t         myPid;              // For restarts by startstop
SHM_INFO      Region;
unsigned char TypeHeartBeat;
unsigned char TypeError;
int           KillThreadFlag = 0;



int main( int argc, char *argv[] )
{
   extern int  LogFile;           // From configuration file
   extern long RingKey;           // Transport ring key
   extern int  HeartBeatInterval; // Seconds
   extern SERVER *srv;            // Structure of server info
   extern int  nSrv;              // Number of Server lines in config file

   char     defaultConfig[] = "sendfilemt.d";
   char     *configFileName = (argc > 1 ) ? argv[1] : &defaultConfig[0];
   int      i;                    // Passed to each thread
   time_t   tHeart = 0;           // When the last heartbeat was sent

/* Initialize the socket system
   ****************************/
   SocketSysInit();

/* Read the configuration file
   ***************************/
   GetConfig( configFileName );

/* Look up local installation id
   *****************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      printf( "%-12s Error getting installation id. Exiting.\n", argv[0] );
      return -1;
   }

/* Look up message types from earthworm.h tables
   *********************************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      printf( "%-12s Invalid message type <TYPE_HEARTBEAT>. Exiting.\n", argv[0] );
      return -1;
   }

   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      printf( "%-12s Invalid message type <TYPE_ERROR>. Exiting.\n", argv[0] );
      return -1;
   }

/* Initialize log file and log configuration parameters
   ****************************************************/
   logit_init( configFileName, 0, 256, LogFile );
   LogConfig();

/* Get process ID for heartbeat messages
   *************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "%-12s Can't get my process id. Exiting.\n", argv[0] );
      return -1;
   }

/* Attach to shared memory ring
   ****************************/
   tport_attach( &Region, RingKey );

/* Create mutex semaphore to allow threads to share resources
   **********************************************************/
   CreateMutex_ew();

/* Start a thread to communicate with each instance of getfileII
   *************************************************************/
   for ( i = 0; i < nSrv; i++ )
   {
      int rc = StartThreadWithArg( ThreadFunction, (void *)i, 0,
                                   &srv[i].threadId );
      if ( rc == -1 )
      {
         printf( "Error starting thread. Exiting.\n" );
         return -1;
      }
      srv[i].threadAlive = TRUE;
   }

/* Loop until kill flag is set in Earthworm transport ring
   *******************************************************/
   while ( 1 )
   {
      time_t now = time(0);   // Current time

/* Send a heartbeat to statmgr every HeartBeatInterval seconds
   ***********************************************************/
      if ( (now - tHeart) >= (time_t)HeartBeatInterval )
      {
         if ( ReportStatus( TypeHeartBeat, 0, "" ) != PUT_OK )
            logit( "t", "%-12s Error sending heartbeat to ring.\n", argv[0] );
         tHeart = now;
      }

/* Exit program if the kill flag was set in the transport ring
   ***********************************************************/
      if ( tport_getflag( &Region ) == TERMINATE ||
           tport_getflag( &Region ) == myPid ) break;

/* Have any threads died?
   *********************/
      for ( i = 0; i < nSrv; i++ )
      {
         char note[NOTESIZE];
         if ( !ThreadAlive(srv[i].threadId) )
         {
            if ( srv[i].threadAlive == TRUE )
            {
               sprintf( note, "%-12s Thread connected to %s port %d is dead.",
                        argv[0], srv[i].ip, srv[i].port );
               logit( "et", "%s\n", note );
               if ( ReportStatus( TypeError, ERR_THREAD_DEAD, note ) != PUT_OK )
                  logit( "et", "%-12s Error sending error message to ring.\n", argv[0] );
            }
            srv[i].threadAlive = FALSE;
         }
      }

      sleep_ew( 1000 );
   }

/* Set the kill flag and wait until all threads have exited.
   Do not use the Windows KillThread function because of
   bad side effects documented on the internet.
   ********************************************************/
   logit( "et", "%-12s Termination requested.\n", argv[0] );
   logit( "et", "%-12s Killing all program threads.\n", argv[0] );
   KillThreadFlag = 1;
   while ( 1 )
   {
      int nAliveThreads = 0;
      for ( i = 0; i < nSrv; i++ )
      {
         if ( ThreadAlive(srv[i].threadId) )
            nAliveThreads++;
      }
      if ( nAliveThreads == 0 ) break;
      sleep_ew( 1000 );
   }
   logit( "et", "%-12s All threads are now dead.\n", argv[0] );

   CloseMutex();
   tport_detach( &Region );
   logit( "et", "%-12s Exiting.\n", argv[0] );
   return 0;
}
