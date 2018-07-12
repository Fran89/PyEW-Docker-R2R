/*
 * runwws.c : Runwws is an Earthworm front-end module that runs either a
 *            Winston import program or a WWS server program.  The WWS
 *            programs are java modules that know nothing about Earthworm
 *            shared memory rings.  Runwws starts one WWS program as a
 *            child process,  When Earthworm shuts down, runwws kills the
 *            WWS child program. If the WWS child program dies on its own,
 *            runwws exits.  Statmgr can be configured to restart runwws
 *            if it exits, and this will cause the WWS module to restart
 *            as well.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <chron3.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>

// Functions in this source file
// -----------------------------
void GetConfig( char *configfile );
void LogConfig( void );
void Lookup( void );
int  SendStatus( unsigned char type, short ierr, char *note );

// Pipe functions
// --------------
int  Pipe_Init( char *child );
int  Pipe_Put( char *chBuf );
int  Pipe_Close( void );
int  ChildDead( void );
int  KillChildProcess( void );
void WaitForChildToDie( int ChildWaitTime );

// Errors reported by Earthworm
// ----------------------------
#define ERR_CHILD_DIED  0
#define ERR_CHILD_ALIVE 1

// Things to read from configuration file
// --------------------------------------
static char RingName[MAX_RING_STR];  // Transport ring to read from
static char MyModName[MAX_MOD_STR];  // Speak as this module name/id
static int  LogSwitch;               // 0 if no logging should be done to disk
static char Child[150];              // Command to start child process
static int  HeartbeatInt;            // Heartbeat interval (s)
static int  ChildWaitTime;           // After this many msec, kill the child process

// Things to look up in the earthworm.h tables with getutil.c functions
// --------------------------------------------------------------------
static long          RingKey;        // Key of shared memory ring
static unsigned char InstId;         // Installation id
static unsigned char MyModId;        // Module Id for this program
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static pid_t myPid = 0;              // My process id

#define MAXSTRLEN    256
static char     Text[MAXSTRLEN];     // String for log/error messages
static SHM_INFO region;              // Shared memory region to use for i/o


int main( int argc, char *argv[] )
{
   time_t NextHeartbeat = 0;

// Check command line arguments
// ----------------------------
   if ( argc != 2 )
   {
        printf( "Usage: runwws <configfile>\n" );
        exit( 0 );
   }

// Read the configuration file(s)
// ------------------------------
   GetConfig( argv[1] );

// Initialize logit at desired logging level
// -----------------------------------------
   logit_init( argv[1], 0, MAXSTRLEN*2, LogSwitch );
   LogConfig();

// Look up info from earthworm.h tables and fill in message logos
// --------------------------------------------------------------
   Lookup();

// Get my own processid
// --------------------
   myPid = getpid();

// Attach to shared memory ring
// ----------------------------
   tport_attach( &region, RingKey );

// Start the child program & open pipe to it
// -----------------------------------------
   if ( Pipe_Init( Child ) < 0 )
   {
        logit( "et", "pipe_init() error starting child program <%s>\n", Child );
        tport_detach( &region );
        return -1;
   }
   logit( "et", "Started program <%s>\n", Child );

/*------------------- setup done; start main loop -------------------------*/

   while (1)
   {
      time_t now = time(NULL);

   // If the child process died, exit the parent program.
   // If desired, configure statmgr to restart the parent.
   // ---------------------------------------------------
      if ( ChildDead() )
      {
         sprintf( Text, "Child process died." );
         logit( "et", "runwws: %s\n", Text );
         if ( SendStatus( TypeError, ERR_CHILD_DIED, Text ) == PUT_OK )
            logit( "et", "runwws: Error message sent to ring.\n" );
         else
            logit( "et", "runwws: Error sending error message to ring.\n");
         logit( "et", "runwws: Exiting.\n" );
         Pipe_Close();
         tport_detach( &region );
         return -1;
      }

   // Send heartbeat to statmgr
   // -------------------------
      if ( now >= NextHeartbeat )
      {
         if ( SendStatus( TypeHeartBeat, 0, "" ) != PUT_OK )
            logit( "t", "runwws: Error sending heartbeat to ring.\n");
         NextHeartbeat = now + HeartbeatInt;
      }

   // See if a termination has been requested
   // ---------------------------------------
      if ( tport_getflag( &region ) == TERMINATE  ||
           tport_getflag( &region ) == myPid ) break;

      sleep_ew( 1000 );
   }
/*-----------------------------end of main loop-------------------------------*/

// Send a quit command to kill the child process and wait for it to die
// --------------------------------------------------------------------
   if ( Pipe_Put("q\n") < 0 )
      logit( "et", "runwws: Error sending quit command to child process.\n" );
   else
      logit( "et", "runwws: Sent quit command to child process.\n" );

   WaitForChildToDie( ChildWaitTime );    // Wait up to ChildWaitTime msec

// If the child process did not die, bonk it on the head
// -----------------------------------------------------
   Pipe_Close();
   if ( ! ChildDead() )
   {
      sprintf( Text, "Child process did not die after being sent a quit command." );
      logit( "et", "runwws: %s\n", Text );
      if ( SendStatus( TypeError, ERR_CHILD_ALIVE, Text ) == PUT_OK )
         logit( "et", "runwws: Error message sent to ring.\n" );
      else
         logit( "et", "runwws: Error sending error message to ring.\n");
      if ( ! KillChildProcess() )
         logit( "et", "runwws: Error killing the child process.\n" );
   }

// Exit program
// ------------
   tport_detach( &region );
   logit( "et", "runwws: Exiting.\n" );
   return 0;
}


/*****************************************************************************/
/*      GetConfig() processes command file(s) using kom.c functions          */
/*                  exits if any errors are encountered                      */
/*****************************************************************************/

void GetConfig( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* Init flags, one for each required command */
   int      nmiss;        /* Number of required commands that were missed   */
   char    *com;
   char    *str;
   char     processor[15];
   int      nfiles;
   int      success;
   int      i;

// Set to zero one init flag for each required command
// ---------------------------------------------------
   ncommand = 6;
   for ( i=0; i<ncommand; i++ ) init[i] = 0;

// Open the main configuration file
// --------------------------------
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        printf( "runwws: Error opening command file <%s>. Exiting.\n",
                configfile );
        exit( -1 );
   }

// Process all command files
// -------------------------
   while ( nfiles > 0 )      // While there are command files open
   {
        while (k_rd())       // Read next line from active file
        {
            com = k_str();   // Get the first token from line

        // Ignore blank lines & comments
        // -----------------------------
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        // Open a nested configuration file
        // --------------------------------
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  printf( "runwws: Error opening command file <%s>. Exiting.\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }
            strcpy( processor, "GetConfig" );

        // Process anything else as a command
        // ----------------------------------
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("PipeTo") ) {
                str = k_str();
                if(str) strcpy( Child, str );
                init[3] = 1;
            }
  /*4*/     else if( k_its("HeartbeatInt") ) {
                HeartbeatInt = k_int();
                init[4] = 1;
            }
  /*5*/     else if( k_its("ChildWaitTime") ) {
                ChildWaitTime = k_int();
                init[5] = 1;
            }
            else
            {
                printf( "runwws: <%s> Unknown command in <%s>. Exiting.\n",
                         com, configfile );
                exit( -1 );
            }

        // See if there were any errors processing the command
        // ---------------------------------------------------
            if( k_err() ) {
               printf( "runwws: Bad <%s> command for %s() in <%s>. Exiting.\n",
                        com, processor, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

// After all files are closed, check init flags for missed commands
// ----------------------------------------------------------------
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       printf( "runwws: ERROR, no " );
       if ( !init[0] ) printf( "<LogFile> "       );
       if ( !init[1] ) printf( "<MyModuleId> "    );
       if ( !init[2] ) printf( "<RingName> "      );
       if ( !init[3] ) printf( "<PipeTo> "        );
       if ( !init[4] ) printf( "<HeartbeatInt> "  );
       if ( !init[5] ) printf( "<ChildWaitTime> " );
       printf( "command(s) in <%s>.  Exiting.\n", configfile );
       exit( -1 );
   }
   return;
}


/************************************************************************/
/*  LogConfig()                                                         */
/************************************************************************/

void LogConfig( void )
{
   logit( "", "PipeTo:        %s\n", Child );
   logit( "", "LogFile:       %d\n", LogSwitch );
   logit( "", "MyModuleId:    %s\n", MyModName );
   logit( "", "RingName:      %s\n", RingName );
   logit( "", "HeartbeatInt   %d\n", HeartbeatInt );
   logit( "", "ChildWaitTime  %d\n", ChildWaitTime );
   logit( "", "\n" );
   return;
}

/************************************************************************/
/*  Lookup()  Look up important info from earthworm.h tables            */
/************************************************************************/

void Lookup( void )
{
// Look up key to shared memory region
// -----------------------------------
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        printf( "runwws: Invalid ring name <%s>. Exiting.\n", RingName);
        exit( -1 );
   }

// Look up local installation id
// -----------------------------
   if ( GetLocalInst( &InstId ) != 0 ) {
      printf( "runwws: Error getting local installation id. Exiting.\n" );
      exit( -1 );
   }

// Look up module id
// -----------------
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      printf( "runwws: Invalid module name <%s>. Exiting.\n", MyModName );
      exit( -1 );
   }

// Look up message types of interest
// ---------------------------------
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      printf( "runwws: Invalid message type <TYPE_HEARTBEAT>. Exiting.\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      printf( "runwws: Invalid message type <TYPE_ERROR>. Exiting.\n" );
      exit( -1 );
   }
   return;
}


/*********************************************************************/
/* SendStatus() builds a heartbeat or error message and              */
/* puts it into shared memory                                        */
/*********************************************************************/

int SendStatus( unsigned char type, short ierr, char *note )
{
   MSG_LOGO logo;
   char     msg[256];
   long     size;
   time_t   now = time(0);       // Current time

   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   if ( type == TypeHeartBeat )
   {
      sprintf( msg, "%ld %ld\n", (long)now, myPid );
   }
   else if ( type == TypeError )
   {
      sprintf( msg, "%ld %d %s\n", (long)now, ierr, note);
   }

   size = strlen( msg );  // Don't include null byte in message
   return tport_putmsg( &region, &logo, size, msg );
}
