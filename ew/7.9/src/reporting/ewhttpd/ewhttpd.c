// Standard Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Earthworm Includes
#include <earthworm.h>
#include <kom.h>
#include <transport.h>

// SQLite Includes
#include <sqlite3.h>

// Mongoose Webserver Includes
#include "mongoose.h"
#include "webhandling.h"

// Defines
#define MAX_RINGS 10		// Maximum number of rings
#define MAX_MSG 16384		// Maximum message size
#define MAX_DBQUERY  20000	// Maximum length of a db query

// Constants
static const char* timeparser = "substr(datetime,1,4)||substr(datetime,6,2)||"
      "substr(datetime,9,2)||substr(datetime,12,2)||substr(datetime,15,2)||"
      "substr(datetime,18,2)";
      
// Structures
typedef struct {
   unsigned char	type;
   unsigned char	mod;
   unsigned char	instid;
   char*		msg;
} DBMSG;

// Functions in this file
void config(char*);
void lookup(void);
void status( unsigned char, short, char* );
int startDB( char *dbFile );
static void closeDB();
int cleanUpDB( unsigned int age );
int storeMsgDB( char* msg, MSG_LOGO logo );
static void selectDB( char* query, struct mg_connection *conn );
static void *callback( enum mg_event event, struct mg_connection *conn,
      const struct mg_request_info *request_info );
void ajax_response( struct mg_connection *conn,
      const struct mg_request_info *request_info );
int printStatusHTML( struct mg_connection *conn );

// Globals
static SHM_INFO   InRegion[MAX_RINGS];	// shared memory region to use for input
static pid_t      MyPid;		// Our process id is sent with heartbeat

// Database globals
static sqlite3_stmt	*stmt;			// Statement to fetch tables
static sqlite3		*dbhandle;		// Handle db connection
static char		*dbquery;		// Buffer for querie

// Things to read or derive from configuration file
static int	LogSwitch;		// 0 if no logfile should be written 
static long	HeartbeatInt;		// seconds between heartbeats        
static int	Debug = 0;		// 0=no debug msgs, non-zero=debug
static char	*WebServerOpt[100];	// Webserver options for mongoose
static int	nWebServerOpt = 0;	// Webserver options counter
static MSG_LOGO	*GetLogo = NULL;	// logo(s) to get from shared memory  
static short	nLogo = 0;		// # of different logos
static char	DataBaseFile[256];	// Name of the database file
static int	MsgLifeTime = 30;	// Number of days for each msg in db

// Things to look up in the earthworm.h tables with getutil.c functions
static long		InRingKey[MAX_RINGS];	// keys of transport input ring
static int		nRings = 0;		// Number of considered rings
static unsigned char	InstId;			// local installation id   
static unsigned char	MyModId;		// Module Id for this program 
static unsigned char	TypeHeartBeat;
static unsigned char	TypeError;


// Error messages
#define  ERR_MISSGAP       0   // sequence gap in transport ring         
#define  ERR_MISSLAP       1   // missed messages in transport ring      
#define  ERR_TOOBIG        2   // retreived msg too large for buffer     
#define  ERR_NOTRACK       3   // msg retreived; tracking limit exceeded 
static char  Text[150];        // string for log/error messages          


// Main program starts here
int main( int argc, char **argv ) 
{
   int           i;
   time_t        timeNow;          // current time               
   time_t        timeLastBeat;     // time last heartbeat was sent
   long          recsize;          // size of retrieved message    
   MSG_LOGO      reclogo;          // logo of retrieved message    
   unsigned char seq[MAX_RINGS];
   int           ret;
   struct mg_context *ctx;         // Pointer for mongoose server
   char         *msgbuf;		// buffer for msgs from ring  

   // Check command line arguments
   if( argc != 2 ) 
   {
      fprintf( stderr, "Usage: ewhttpd <configfile>\n" );
      return EW_FAILURE;
   }
   

   // Initialize name of log-file & open it
   logit_init( argv[1], 0, 256, 1 );
   

   // Read the configuration file(s)
   config( argv[1] );

   // Lookup important information from earthworm.d
   lookup();
   

   // Set logit to LogSwitch read from configfile
   logit_init( argv[1], 0, 256, LogSwitch );
   logit( "", "ewhttpd: Read command file <%s>\n", argv[1] );


   // Get our own process ID for restart purposes
   if( ( MyPid = getpid()) == -1 )
   {
      logit ( "e", "ewhttpd: Call to getpid failed. Exiting.\n" );
      for( i = 0; i<nWebServerOpt; i++ ) free( WebServerOpt[i] );
      clear_users();
      exit( -1 );
   }
   
   // Allocate the message input buffer
   if ( !( msgbuf = ( char* ) malloc( ( size_t ) MAX_MSG + 1 ) ) )
   {
      logit( "et",
            "ewhttpd: failed to allocate %d bytes"
            " for message buffer; exiting!\n", MAX_MSG + 1 );
      free( GetLogo );
      for( i = 0; i<nWebServerOpt; i++ ) free( WebServerOpt[i] );
      clear_users();
      exit( -1 );
   }
   
   // Attach to shared memory rings
   for( i=0; i<nRings; i++ )
   {
      tport_attach( &InRegion[i], InRingKey[i] );
      logit( "o", "ewhttpd: Attached to public memory region: %ld\n",
            InRingKey[i] );
      //Flush the input ring
      while( tport_copyfrom( &InRegion[i], GetLogo, nLogo, &reclogo,
               &recsize, msgbuf, MAX_MSG, &seq[i] ) != GET_NONE );
   }

   // Force a heartbeat to be issued in first pass thru main loop
   timeLastBeat = time( &timeNow ) - HeartbeatInt - 1;
   
   // Create the SQLite database if it does not, exit
   if( startDB(DataBaseFile) )
   {
      logit ( "e", "ewhttpd: Failed to open the database. Exiting.\n" );
      for( i = 0; i<nWebServerOpt; i++ ) free( WebServerOpt[i] );
      clear_users();
      free( msgbuf );
      exit( -1 );
   }
   logit( "o", "ewhttpd: Connected to database: %s\n",
      DataBaseFile );


   // Start webserver
   ctx = mg_start( &callback, NULL, ( const char** ) &WebServerOpt );
   if( ctx == NULL )
   {
      logit ( "e", "ewhttpd: Error starting webserver. Exiting.\n" );
      for( i = 0; i<nWebServerOpt; i++ ) free( WebServerOpt[i] );
      clear_users();
      free( msgbuf );
      closeDB();
      exit( -1 );
   }
   logit( "o", "ewhttpd: Started webserver connected to: %s\n",
         mg_get_option( ctx, "listening_ports" ) );
   
   // Loop while waiting for messages from the input rings
   // Recognized messages will be stored in the database
   while( tport_getflag( &InRegion[0] ) != TERMINATE  &&
      tport_getflag( &InRegion[0] ) != MyPid ) 
   {
      // send heartbeat
      if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt ) 
      {
         timeLastBeat = timeNow;
         status( TypeHeartBeat, 0, "" );
      }
      // Clean up old messages from database
      cleanUpDB( MsgLifeTime );
      
      // Check messages in all input rings sequencially
      for( i = 0; i < nRings; i++ )
      {
         ret = tport_copyfrom( &InRegion[i], GetLogo, nLogo, &reclogo,
               &recsize, msgbuf, MAX_MSG, &seq[i] );
         switch( ret )
         {
            case GET_OK:      // got a message, no errors or warnings
               break;
               
            case GET_NONE:    // no messages of interest, check again later
               continue;
               
            case GET_NOTRACK: // got a msg, but can't tell if any were missed
               sprintf( Text,
                     "Msg received (i%u m%u t%u); "
                     "transport.h NTRACK_GET exceeded",
                     reclogo.instid, reclogo.mod, reclogo.type );
               status( TypeError, ERR_NOTRACK, Text );
               break;

            case GET_MISS_LAPPED:     // got a msg, but also missed lots
               sprintf( Text,
                     "Missed msg(s) from logo (i%u m%u t%u)",
                     reclogo.instid, reclogo.mod, reclogo.type );
               status( TypeError, ERR_MISSLAP, Text );
               break;

            case GET_MISS_SEQGAP:     // got a msg, but seq gap
               sprintf( Text,
                     "Saw sequence# gap for logo (i%u m%u t%u s%u)",
                     reclogo.instid, reclogo.mod, reclogo.type, seq[i] );
               status( TypeError, ERR_MISSGAP, Text );
               break;

            case GET_TOOBIG:  // next message was too big, resize buffer
               sprintf( Text,
                     "Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%d]",
                     recsize, reclogo.instid, reclogo.mod, reclogo.type,
                     MAX_MSG );
               status( TypeError, ERR_TOOBIG, Text );
               continue;

            default:         // Unknown result
               sprintf( Text, "Unknown tport_copyfrom result:%d", ret );
               status( TypeError, ERR_TOOBIG, Text );
               continue;
         }
         
         // Process message
         msgbuf[recsize] = '\0'; // Null terminate for ease of printing
         
         if ( Debug )
   	    logit("ot", "received msg:\n%s\n", msgbuf);
         
         // Store message in database
         storeMsgDB( msgbuf, reclogo );
      }

      // Sleep a little
      sleep_ew( 1000 );
   }
   
   // Stop webserver
   mg_stop(ctx);
   
   // Close database
   closeDB();
   
   // free allocated memory
   free( GetLogo );
   free( msgbuf  );
   for( i = 0; i<nWebServerOpt; i++ ) free( WebServerOpt[i] );
   clear_users();
   
   // detach from shared memory
   for( i=0; i<nRings; i++ ) tport_detach( &InRegion[i] );
   
   // write a termination msg to log file
   logit( "t", "ewhttpd: Termination requested; exiting!\n" );
   fflush( stdout );
   return( 0 );
}

/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;                *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
#define ncommand 8        /* # of required commands you expect to process */
void config( char *configfile ) 
{
   char init[ncommand]; /* init flags, one byte for each required command */
   int nmiss; /* number of required commands that were missed   */
   char *com;
   char *str;
   int nfiles;
   int success;
   int i;

   // Set to zero one init flag for each required command
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

   // Open the main configuration file
   nfiles = k_open(configfile);
   if (nfiles == 0) 
   {
      logit("e",
         "ewhtmlemail: Error opening command file <%s>; exiting!\n",
         configfile);
      exit(-1);
   }

   // Process all command files
   while( nfiles > 0 ) // While there are command files open
   {
      while( k_rd() ) // Read next line from active file
      {
         com = k_str(); // Get the first token from line

         // Ignore blank lines & comments
         if( !com ) continue;
         if( com[0] == '#' ) continue;

         /* Open a nested configuration file
          **********************************/
         if( com[0] == '@' ) 
         {
            success = nfiles + 1;
            nfiles = k_open( &com[1] );
            if( nfiles != success ) 
            {
               logit("e",
                  "ewhtmlemail: Error opening command file <%s>; exiting!\n",
                  &com[1]);
               exit(-1);
            }
            continue;
         }

         // Process anything else as a command
         // 0
         if( k_its( "LogFile" ) ) 
         {
            LogSwitch = k_int();
            if( LogSwitch < 0 || LogSwitch > 2 ) 
            {
               logit( "e",
                  "ewhttpd: Invalid <LogFile> value %d; "
                  "must = 0, 1 or 2; exiting!\n", LogSwitch );
               exit( -1 );
            }
            init[0] = 1;
         } 
         // 1
         else if( k_its( "MyModuleId" ) )
         {
            if( (str = k_str()) != NULL ) 
            {
               if( GetModId( str, &MyModId ) != 0 ) 
               {
                   logit( "e",
                      "ewhttpd: Invalid module name <%s> "
                      "in <MyModuleId> command; exiting!\n", str );
                   exit( -1 );
               }
            }
            init[1] = 1;
         }
         // 2
         else if( k_its( "InRing" ) ) 
         {
            if( (str = k_str()) != NULL ) 
            {
               if( nRings >= MAX_RINGS )
               {
                  logit( "e",
                     "ewhttpd: Exceeded maximum number of input rings <%d> "
                     "in <InRing> command; exiting!\n", MAX_RINGS );
                  exit( -1 );
               }
               if( ( InRingKey[nRings++] = GetKey( str ) ) == -1 ) 
               {
                  logit( "e",
                     "ewhttpd: Invalid ring name <%s> "
                     "in <InRing> command; exiting!\n", str );
                  exit( -1 );
               }
            }
            init[2] = 1;
         }
         // 3
         else if( k_its( "HeartbeatInt" ) )
         {
            HeartbeatInt = k_long();
            init[3] = 1;
         }
         // 4
         else if( k_its( "Debug" ) ) 
         {
            Debug = k_int();
            init[4] = 1;
         }
         // 5 - Webserver options
         else if( k_its( "WebServerOpt" ) )
         {
          
            str = k_str();
            WebServerOpt[nWebServerOpt] = 
                  ( char* ) malloc( sizeof( char ) * strlen( str ) + 1 );
            if( WebServerOpt[nWebServerOpt] == NULL )
            {
               logit("e",
                     "ewhttpd: Error allocating memory for "
                     "web server parameters\n" );
               exit(-1);
            }
            strcpy( WebServerOpt[nWebServerOpt++], str );
            
            str = k_str();
            WebServerOpt[nWebServerOpt] = 
                  ( char* ) malloc( sizeof( char ) * strlen( str ) + 1 );
            if( WebServerOpt[nWebServerOpt] == NULL )
            {
               logit("e",
                     "ewhttpd: Error allocating memory for "
                     "web server parameters\n" );
               exit(-1);
            }
            strcpy( WebServerOpt[nWebServerOpt++], str );
            
            // Set NULL option
            WebServerOpt[nWebServerOpt] = NULL;
            
            init[5] = 1;
         }
         // 6 - DataBaseFile
         else if( k_its( "DataBaseFile" ) )
         {
            str = k_str();
            strcpy( DataBaseFile, str );
            init[6] = 1;
         }
         // 7 - GetLogo
         else if (k_its("GetLogo")) 
         {
            GetLogo = ( MSG_LOGO* ) realloc( GetLogo, ( nLogo + 1 ) * sizeof( MSG_LOGO ) );
            if( GetLogo == NULL )
            {
               logit("e",
                     "ewhttpd: Error allocating memory for "
                     "earthworm logos\n" );
               exit(-1);
            }
            int fail = 1;
            if ( (str = k_str()) != NULL)
            {
               GetInst(str, &GetLogo[nLogo].instid);
               if ((str = k_str()) != NULL)
               {
                  GetModId(str, &GetLogo[nLogo].mod);
                  if ((str = k_str()) != NULL)
                  {
                     GetType(str, &GetLogo[nLogo].type);
                     fail = 0;
                  }
               }
            }
            if( fail == 1 )
            {
               logit("e",
                     "ewhttpd: Error perceiving "
                     "earthworm logos\n" );
               if ( nLogo > 0 ) free( GetLogo );
               exit(-1);
            }
            nLogo++;
            init[7] = 1;
         }
         // Users
         else if( k_its( "User" ) )
         {
            char *user = k_str();
            char *password = k_str();
            if( Debug ) logit( "", "Registering user: %s\n", user );
            if( !add_user( user, password ) )
            {
               logit("e",
                     "ewhttpd: Error allocating memory for users\n" );
               exit(-1);
            }
         }
         // 9 - Message lifetime
         else if( k_its( "MsgLifeTime" ) )
         {
            MsgLifeTime = k_int();
         }
         // Unknown command
         else 
         {
            logit( "e", "ewhttpd: <%s> Unknown command in <%s>.\n",
               com, configfile );
            exit( -1 );
         }

         // See if there were any errors processing the command
         if( k_err() )
         {
            logit( "e",
               "ewhttpd: Bad <%s> command in <%s>; exiting!\n",
               com, configfile );
            exit( -1 );
         }
      }
      nfiles = k_close();
   }

   // After all files are closed, check init flags for missed commands
   nmiss = 0;
   for( i = 0; i < ncommand; i++ ) if ( !init[i] ) nmiss++;
   if( nmiss ) 
   {
      logit( "e", "ewhttpd: ERROR, no " );
      if( !init[0] ) logit( "e", "<LogFile> " );
      if( !init[1] ) logit( "e", "<MyModuleId> " );
      if( !init[2] ) logit( "e", "<InRing> " );
      if( !init[3] ) logit( "e", "<HeartbeatInt> " );
      if( !init[4] ) logit( "e", "<Debug> " );
      if( !init[5] ) logit( "e", "<WebServerOpt> " );
      if( !init[6] ) logit( "e", "<DataBaseFile> " );
      if( !init[7] ) logit( "e", "<GetLogo> " );
      logit( "e", "command(s) in <%s>; exiting!\n", configfile );
      exit( -1 );
   }
   return;
}

/*********************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 *********************************************************************/
void lookup( void ) 
{
    // Look up installations of interest
    if( GetLocalInst( &InstId ) != 0 ) 
    {
        logit( "e",
              "ewhttpd: error getting local installation id; exiting!\n" );
        exit( -1 );
    }

    //* Look up message types of interest
    if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
    {
        logit( "e",
              "ewhttpd: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
        exit( -1 );
    }
    if( GetType( "TYPE_ERROR", &TypeError) != 0 )
    {
        logit("e",
              "ewhttpd: Invalid message type <TYPE_ERROR>; exiting!\n" );
        exit( -1 );
    }
    return;
}

/******************************************************************************
 * status() builds a heartbeat or error message & puts it into                *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void status( unsigned char type, short ierr, char *note ) 
{
    MSG_LOGO logo;
    char msg[256];
    long size;
    time_t t;

    // Build the message
    logo.instid = InstId;
    logo.mod = MyModId;
    logo.type = type;

    time( &t );

    if( type == TypeHeartBeat ) 
       sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid );
    else if( type == TypeError )
    {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note );
        logit( "et", "ewhttpd: %s\n", note );
    }

    size = strlen( msg ); // don't include the null byte in the message

    // Write the message to shared memory
    if( tport_putmsg( &InRegion[0], &logo, size, msg) != PUT_OK )
    {
       if( type == TypeHeartBeat )
          logit( "et", "ewhttpd:  Error sending heartbeat.\n" );
       else if( type == TypeError )
          logit( "et", "ewhttpd:  Error sending error:%d.\n", ierr );
    }

    return;
}








/******************************************************************************
 * WebServer Functions                                                        * 
 *    - callback : Used to manage server requests and divert to the functions *
 ******************************************************************************/
 
// Functions within this section


// Callback
static void *callback( enum mg_event event, struct mg_connection *conn,
      const struct mg_request_info *request_info) 
{
   void *processed = "yes";

   if( event == MG_NEW_REQUEST )
   {
      // Check if this is an authorized connection
      if( user_count()>0 && !is_authorized( conn, request_info ) )
         // If not, open login page - hardcoded with ewhttpd_login.html
         redirect_to_login( conn, request_info );
      // Check if this is an authorization request
      else if( user_count()>0 && is_autorization_request( request_info ) ) 
         // Check authorization
         authorize( conn, request_info );
      else if( is_ajax_request( request_info ) ) 
         // Respond with AJAX
         ajax_response( conn, request_info );
         // Print a status report
      else if( strcmp( request_info->uri, "/status" ) == 0 )
         printStatusHTML( conn );
      else {
         // No suitable handler found, mark as not processed. Mongoose will
         // try to serve the request.
         processed = NULL;
      }
   } else {
      processed = NULL;
   }
   return processed;
}


// Print the status as an html
int printStatusHTML( struct mg_connection *conn )
{
   FILE *fp;
   char line[1024];
   
   // System call to ew's command: status
   fp = popen( "status 2>&1", "r" );
   if( fp == NULL )
   {
      mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "Error acquiring Earthworm status<br>");
      //
      return -1;
   }
   
   // Send connection header
   mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
   
   // Send some html stuff
   mg_printf(conn, "<html><head>"
         "<title>EW Status</title>"
         "<meta http-equiv=\"refresh\" content=\"60\">"
         "</head><body><pre>"
         "<h1>Earthworm Web Status Service</h1>");
      
   // Read the status ouput line by line and convert to valid html
   while( fgets( line, sizeof(line) - 1, fp ) != NULL )
      mg_printf( conn, "%s", line );

   // Send more html stuff
   mg_printf(conn, "</pre></body></html>");
   return 1;
}


// Respond to AJAX requests
// This function is responsible for retrieving data from the database
// and serving it to the clients
void ajax_response( struct mg_connection *conn,
      const struct mg_request_info *request_info )
{
   
   
   char buf[100];
   char reqtype[100];
   char* qs;
   
   // Extract request type
   // Available types are:
   //	- ewmsg		A JSON array of raw EW messages
   // A request without a valid type is rejected
   qs = request_info->query_string;
   mg_get_var( qs, strlen( qs == NULL ? "" : qs ), "reqtype", 
         reqtype, sizeof( reqtype ) );
         
   if( strcmp( reqtype, "ewmsg" ) == 0 )
   {
      // Produce ew messages from db query
      // Variables for the db query
      char	instIDstr[11];	// Installation ID
      char	modIDstr[10];	// Module ID
      char	typestr[9];	// Message type, 0 means all
      char	start[15];	// Start time, empty or 0 from begining
      char	end[15];	// End time, empty or 0 means NOW
      unsigned char aux;
      int	pc = 0;		// counter for parameters
      char      sqlquery[2000];	// to hold the query
      
      // Initialize strings
      instIDstr[0] = '\0';
      modIDstr[0] = '\0';
      typestr[0] = '\0';
      start[0] = '\0';
      end[0] = '\0';
      
      // Extract instID
      mg_get_var( qs, strlen( qs == NULL ? "" : qs ), "instid", 
            buf, sizeof( buf ) );
      if( strlen( buf ) > 0 )
      {
         if( GetInst( buf, &aux ) == 0 )
            if( aux == 0 ) //Wildcard
            	pc--;
            else
               sprintf( instIDstr, "instID=%d", aux );
         else
            printf( instIDstr, "instID=-1" );
         pc++;
      }
      
      
      // Extract modID
      mg_get_var( qs, strlen( qs == NULL ? "" : qs ), "modid", 
            buf, sizeof( buf ) );
      if( strlen( buf ) > 0 )
      {
         if( GetModId( buf, &aux ) == 0 )
            if( aux == 0 ) //Wildcard
            	pc--;
            else
               sprintf( modIDstr, "modID=%d", aux );
         else
            sprintf( modIDstr, "modID=-1" );;
         pc++;
      }    
           
      // Extract type
      mg_get_var( qs, strlen( qs == NULL ? "" : qs ), "type", 
            buf, sizeof( buf ) );
      if( strlen( buf ) > 0 )
      {
         
         if( GetType( buf, &aux ) == 0 )
            if( aux == 0 ) //Wildcard
            	pc--;
            else
            	sprintf( typestr, "type=%d", aux );
         else
            sprintf( typestr, "type=-1" );
         pc++;
      }
      
      
      // Extract times
      mg_get_var( qs, strlen( qs == NULL ? "" : qs ), "start", 
            start, sizeof( start ) );
      pc += strlen( start ) > 0 ? 1 : 0;
      mg_get_var( qs, strlen( qs == NULL ? "" : qs ), "end", 
            end, sizeof( end ) );
      pc += strlen( end ) > 0 ? 1 : 0;
      
      // Build query string      
      sprintf( sqlquery, "SELECT * FROM ewmsg %s ",
            ( pc > 0 || strlen( start ) > 0 || 
            strlen( end ) > 0 ) ? "WHERE" : "" );
      if( strlen( instIDstr ) > 0 ) 
      {
         strcat( sqlquery, instIDstr );
         if( --pc > 0 ) strcat( sqlquery, " AND " );
      }
      if( strlen( modIDstr ) > 0 )
      {
         strcat( sqlquery, modIDstr );
         if( --pc > 0 ) strcat( sqlquery, " AND " );
      }
      if( strlen( typestr ) > 0 ) 
      {
         strcat( sqlquery, typestr );
         if( --pc > 0 ) strcat( sqlquery, " AND " );
      }
      if( strlen( start ) > 0 || strlen( end ) > 0 )
         strcat( sqlquery, "substr(datetime,1,4)||substr(datetime,6,2)||"
               "substr(datetime,9,2)||substr(datetime,12,2)||"
               "substr(datetime,15,2)||substr(datetime,18,2)");
      if( strlen( start ) > 0 && strlen( end ) > 0 )
         sprintf( sqlquery, "%s BETWEEN '%s' AND '%s'", sqlquery, start, end );
      else if( strlen( start ) > 0 )
         sprintf( sqlquery, "%s > '%s'", sqlquery, start );
      else if( strlen( start ) > 0 )
         sprintf( sqlquery, "%s < '%s'", sqlquery, end );
      
      // Last part of the query
      strcat( sqlquery, ";" );
      
      if( Debug ) logit( "ot", "ewhttpd:\n%s\n", sqlquery );
      
      // Print AJAX header prior to making search
      print_ajax_header( conn );
      
      // Print JSON data
      selectDB( sqlquery, conn );
   }
   
   // Done
   return;
}



// Start the database
int startDB( char *dbFile )
{
   int ret;
   
   // Open database file or create it if nonexistant
   if( (ret = sqlite3_open( dbFile, &dbhandle ) ) )
   {
      return ret;
   }
      
   // Create table, if not existing
   sqlite3_exec( dbhandle, "CREATE TABLE IF NOT EXISTS "
         "ewmsg (datetime DATE,instID INTEGER,modID INTEGER,"
         "type INTEGER,raw BLOB);", 
         0, 0, 0 );
   
   // Reserve memory for query
   if( (dbquery = ( char* )malloc( MAX_DBQUERY * sizeof( char ) ) ) == NULL )
      return 0;
      
   return ret;
}

// Close the database
static void closeDB()
{
   // Free memory
   free( dbquery );
   
   // Close the database
   sqlite3_close( dbhandle );
}

// Store ew message in DB
int storeMsgDB( char* msg, MSG_LOGO logo )
{
   // Query too large, must be rejected
   if ( strlen( msg ) > ( MAX_DBQUERY + 100 ) ) return -1;
   // Store query
   sprintf( dbquery, "INSERT INTO ewmsg "
         "VALUES(DATETIME('now'),%d,%d,%d,'%s');",
         logo.instid, logo.mod, logo.type, msg );
         
   return sqlite3_exec( dbhandle, dbquery, 0, 0, 0 );
}

// Clean old messages from DB
// The argument age is the maximum number of days allowed 
int cleanUpDB( unsigned int age )
{
   sprintf( dbquery, "DELETE FROM ewmsg WHERE "
         "datetime < datetime('now', '-%d days');", age );
   return sqlite3_exec( dbhandle, dbquery, NULL, NULL, NULL );
}

// Conduct searches for messages in the database
static void selectDB( char* query, struct mg_connection *conn )
{
   int first = 1;
   int i;
   const char* rawmsg;

   sqlite3_prepare_v2( dbhandle, query, -1, &stmt, 0 );
   sqlite3_column_count( stmt );

   mg_printf( conn, "[" );
   while( sqlite3_step( stmt ) == SQLITE_ROW )
   {      
      if( first == 1 )
         first = 0;
      else
         mg_printf( conn, "," );
      
      mg_printf( conn, "{\"Time\":\"%s\",\"InstID\":%s,\"ModID\":%s,\""
            "MsgTyp\":%s,\"RawMsg\":\"",
            sqlite3_column_text( stmt, 0 ),
            sqlite3_column_text( stmt, 1 ),
            sqlite3_column_text( stmt, 2 ),
            sqlite3_column_text( stmt, 3 ) );
      
      // Send message with escaped newline characters
      rawmsg = sqlite3_column_text( stmt, 4 );
      for( i = 0; i < strlen( rawmsg ); i++ )
      {
         if( *(rawmsg+i) != '\n' )
            mg_printf( conn, "%c", *(rawmsg+i) );
         else
            mg_printf( conn, "\\n" );
      }
      mg_printf( conn, "\"}" );
   }
   
   mg_printf(conn,"]");
}








