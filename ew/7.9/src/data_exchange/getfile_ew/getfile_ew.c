/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getfile_ew.c 5700 2013-08-05 00:05:51Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2007/02/26 18:56:06  paulf
 *     yet more warnings fixed related to time_t
 *
 *     Revision 1.5  2007/02/26 18:50:05  paulf
 *     more warnings fixed related to time_t
 *
 *     Revision 1.4  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.3  2003/06/24 20:26:43  withers
 *     withers added sleep_ew(1000) within HeartBeater loop
 *
 *     Revision 1.2  2002/11/03 19:00:48  lombard
 *     Added RCS header
 *
 *
 *
 */

 /****************************************************************
  *                           getfile_ew.c                       *
  *                                                              *
  *        Program to get files via a socket connection.         *
  *     Modified to attach to an earthworm ring and beat its     *
  *     heart there by Lucky Vidmar, November 2001.              *
  ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include "getfile_ew.h"

/* Functions declarations
   **********************/
void getfile_config( char * );
void getfile_lookup( void );
void getfile_status( unsigned char, short, char * );
void LogConfig( void );
void SocketSysInit( void );
void InitServerSocket( void );
int  AcceptConnection( int * );
int  GetBlockFromSocket( char [], int * );
void CloseReadSock( void );
thr_ret         HeartBeater (void *);



#define			MAX_CLIENTS		100

/* Thready stuff
****************/
#define THREAD_STACK 8192
static unsigned tidHeartBeater;    /* Heart Beat thread id */
int HeartBeaterStatus = 0;         /* 0=> thread ok. <0 => dead */


static SHM_INFO  OutRegion;     /* shared memory region to use for output */
static char      OutRingName[MAX_RING_STR];      /* name of transport ring for output */
static char      MyModName[MAX_MOD_STR];        /* speak as this module name/id */
static long      HeartBeatInterval;    /* seconds between heartbeats        */

static long          OutRingKey;     /* key of transport ring for output  */
static unsigned char MyModId;        /* Module Id for this program        */
static unsigned char InstId; 
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

char   ServerIP[20];           /* IP address of system to receive msg's */
int    ServerPort;             /* The well-known port number */
int    nClient = 0;            /* Number of trusted clients */
CLIENT Client[MAX_CLIENTS];     /* List if trusted clients */
int    TimeOut;                /* Send/receive timeout, in seconds */

static char   TempDir[80];            /* Name of temporary directory */
static char   LogFileDir[80];         /* Full name of log files */
static int    LogFile;                /* Flag value, 0 - 3 */
static int    EwLogSwitch;            /* Flag value, 0 - 3 */

static 	int     Earthworm; 
static 	int     Standalone;
static	time_t 	timeNow, timeLastBeat;


pid_t   MyPid;  /** Our process id is sent with heartbeat **/



int main( int argc, char *argv[] )
{
	int 	i;

      FILE   *fp;
      static char buf[BUFLEN];
      int    nbytes;
      char   fname[80];
      char   outfile[1024];
      int    total_bytes_received = 0;
      time_t tstart;
      int    rc, clientIndex;

	if (argc != 2)
    {
        fprintf( stderr, "Usage: getfile_ew <configfile>\n" );
        exit( 0 );
    }


	/* Read the configuration file
	***************************/
	getfile_config (argv[1]);

	if (Earthworm == TRUE)
	{
		/* We are in earthworm mode */

		/* Look up important info from earthworm.h tables
		************************************************/
		getfile_lookup ();

		/* Set up logging and log the config file
		**************************************/
		logit_init (argv[1], (short) MyModId, 256, EwLogSwitch);
		LogConfig ();

		/* Get our own process ID for restart purposes
		**********************************************/

		if ((MyPid = getpid ()) == -1)
		{
			logit ("e", "getfile: Call to getpid failed. Exiting.\n");
			exit (-1);
		}

		/* Attach to the output ring */
		tport_attach (&OutRegion, OutRingKey);
		logit ("", "getfile_ew: Attached to public memory region %s: %d\n",
									OutRingName, OutRingKey);

		/* Force a heartbeat to be issued in first pass thru main loop
		*************************************************************/
		timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

		/* Force a heartbeat to be issued in first pass thru main loop
		*************************************************************/
		if (StartThread (HeartBeater, (unsigned) THREAD_STACK, &tidHeartBeater) == -1)
		{
			logit( "e", "getfile_ew: Error starting Processor thread.  Exiting.\n");
			tport_detach (&OutRegion);
			exit (-1);
		}
		HeartBeaterStatus = 0; /*assume the best*/
	}
	else 
	{
		/* We are in standalone mode */

		/* Set up logging and log the config file
		**************************************/
		logit_init ( argv[1], 1, 256, LogFile );
		LogConfig ();
	}


/* Make sure all the necessary directories exist.
   The temporary directory becomes the current directory.
   *****************************************************/
      for ( i = 0; i < nClient; i++ )
      {

         if ( chdir_ew( Client[i].indir ) == -1 )
         {
            logit ("e", "ERROR. Can't change working directory to %s Exiting.\n",
                 Client[i].indir );
            return -1;
         }

         Client[i].ipint = inet_addr( Client[i].ip );
         if ( Client[i].ipint == 0xffffffff )
         {
            logit ( "e", "Error. Bad client ip: %s Exiting.\n", Client[i].ip );
            exit( -1 );
         }
      }

      if ( chdir_ew( TempDir ) == -1 )
      {
         logit ("e", "ERROR. Can't change working directory to %s Exiting.\n",
              TempDir );
         return -1;
      }

/* Initialize the socket system
   ****************************/
   SocketSysInit();

/* Initialize the server socket
   ****************************/
   InitServerSocket();

	/* Get file via socket connection.
	   AcceptConnection() blocks until a client connects.
   	   This is an infinite loop that is never broken.
	*************************************************/
	while ( 1 )
	{
		if (Earthworm == TRUE)
		{
			/* Check on our heartbeat process */
			if (HeartBeaterStatus < 0)
			{
				tport_detach (&OutRegion);
				logit( "t", "getfile_ew: Termination requested; exiting!\n" );
				fflush( stdout );
				exit (-1);
			}
		}


      if ( AcceptConnection( &clientIndex ) == GETFILE_FAILURE ) 
		continue;

      /* Get the name of the file being received,
         and the length of the file name.
       ***************************************/
      rc = GetBlockFromSocket( buf, &nbytes );

      if ( rc == GETFILE_DONE )
      {
         logit ("e", "Received zero-length block.\n" );
         CloseReadSock();
         continue;
      }

      if ( rc == GETFILE_FAILURE )
      {
         logit ("e", "Error getting file name from socket.\n" );
         CloseReadSock();
         continue;
      }

      buf[nbytes] = '\0';
      strcpy( fname, buf );

      logit ("et", "Receiving file %s\n", fname );

      /* Open a new file in the temporary directory
         to contain the incoming data
        ******************************************/
      fp = fopen( fname, "wb" );
      if ( fp == NULL )
      {
         logit ("e", "Error. Can't open new file %s\n", fname );
         CloseReadSock();
         continue;
      }

      /* Get the contents of the file, one block at a time,
         and write them to the local disk file.
         If a zero-length block is received, it means we
         have the whole file.
        *************************************************/
      time( &tstart );

      while ( 1 )
      {
         rc = GetBlockFromSocket( buf, &nbytes );

         if ( rc == GETFILE_DONE )
         {
            time_t telapsed = time(0) - tstart;

            logit ("e", "File <%s>, %d bytes, received in %ld seconds", 
							fname, total_bytes_received, (long) telapsed );
            if ( telapsed > 0 )
            {
               double rate = (double)total_bytes_received / telapsed / 1024.;
               logit ("e", " (%.1lf kbytes/sec)", rate );
            }
            logit ("e", "\n" );
            break;
         }

         if ( rc == GETFILE_FAILURE )
         {
            logit ("e", "GetBlockFromSocket() error.\n" );
            break;
         }

         if ( fwrite( buf, sizeof(char), nbytes, fp ) == 0 )
         {
            logit ("e", "Error writing new file.\n" );
            break;
         }
         total_bytes_received += nbytes;
      }
      fclose( fp );

		/* Move file to client directory
		 *****************************/

		sprintf (outfile, "%s/%s", Client[clientIndex].indir, fname);
		if ( rename_ew( fname, outfile ) == -1 )
			logit ("e", "Error. Can't move file to client directory.\n" );

		logit ("e", "File moved to <%s>\n", outfile);
		CloseReadSock();

	}   /* End of "while loop". Accept next connection. */
}



/******************************************************************************
 *  getfile_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/

#define NCOMMANDS 11         /* # of required commands you expect to process   */

void getfile_config( char *configfile )
{
   int      nCommonCmd;
   int      nStandaloneCmd;
   int      nEarthwormCmd;
   char     init[NCOMMANDS]; /* init flags, one byte for each required command */
   char    *com;
   char    *str;
   int      nfiles, success, i, nmiss;
   char 	envEW_LOG[1024];

/* Set to zero one init flag for each required command
 *****************************************************/
   nCommonCmd = 5;
   nStandaloneCmd = 2;
   nEarthwormCmd = 4;
   for( i=0; i<NCOMMANDS; i++ )  
		init[i] = 0;

	nClient = 0;
	Earthworm = FALSE;
	Standalone = FALSE;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        fprintf( stderr,
                "getfile: Error opening command file <%s>; exiting!\n",
                 configfile );
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
                          "getfile: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("ServerIP") ) 
			{
                str = k_str();
                if(str) strcpy( ServerIP, str );
                init[0] = 1;
            }
  /*1*/     else if( k_its("ServerPort") ) 
			{
                ServerPort = k_int();
                init[1] = 1;
            }
  /*2*/     else if( k_its("TimeOut") ) 
			{
                TimeOut = k_int();
                init[2] = 1;
            }
  /*3*/     else if( k_its("TempDir") ) 
			{
                str = k_str();
                if(str) strcpy( TempDir, str );
                init[3] = 1;
            }
  /*4*/     else if( k_its("Client") ) 
			{
                if ( nClient >= MAX_CLIENTS ) {
                    fprintf( stderr, "getfile: Too many <Client> commands in <%s>",
                            configfile );
                    fprintf( stderr, "; max=%d; exiting!\n", (int) MAX_CLIENTS );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   strcpy (Client[nClient].ip, str); 
                }
                if( ( str=k_str() ) ) {
                   strcpy (Client[nClient].indir, str);
                }

				nClient = nClient + 1;
                init[4] = 1;
			}

  /*5*/     else if( k_its("LogFile") ) {
                LogFile = k_int();
                init[5] = 1;
				Standalone = TRUE;
            }
  /*6*/     else if( k_its("LogFileDir") ) {
                str = k_str();
                if(str) strcpy( LogFileDir, str );
                init[6] = 1;
				Standalone = TRUE;

				/* Set the environment to reflect this */
				sprintf (envEW_LOG, "EW_LOG=%s", LogFileDir);
				if (putenv (envEW_LOG) != 0)
				{
					fprintf (stderr, "Warning: Could not set EW_LOG environment\n");
				}
            }


  /*7*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[7] = 1;
				Earthworm = TRUE;
            }
  /*8*/     else if( k_its("OutRingName") ) {
                str = k_str();
                if(str) strcpy( OutRingName, str );
                init[8] = 1;
				Earthworm = TRUE;
            }
  /*9*/     else if( k_its("EwLogSwitch") ) {
                EwLogSwitch = k_int();
                init[9] = 1;
				Earthworm = TRUE;
            }
  /*10*/    else if( k_its("HeartBeatInterval") ) {
                HeartBeatInterval = k_long();
                init[10] = 1;
				Earthworm = TRUE;
            }

         /* Unknown command
          *****************/
            else {
                fprintf( stderr, "getfile: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               fprintf( stderr,
                       "getfile: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/

   if ((Earthworm == FALSE) && (Standalone == FALSE))
   {
     fprintf (stderr, "Specify either Earthworm or Standalone configuration.\n");
     exit (-1);
   }
   if ((Earthworm == TRUE) && (Standalone == TRUE))
   {
     fprintf (stderr, "Specify only one:  Earthworm or Standalone configuration.\n");
     exit (-1);
   }

   nmiss = 0;
   for  (i = 0; i < nCommonCmd; i++ )  
		if( !init[i] ) nmiss++;
   if ( nmiss ) 
   {
       fprintf( stderr, "getfile: ERROR, no common " );
       if ( !init[0] )  fprintf( stderr, "<ServerIP> "           );
       if ( !init[1] )  fprintf( stderr, "<ServerPort> "        );
       if ( !init[2] )  fprintf( stderr, "<TimeOut> "        );
       if ( !init[3] )  fprintf( stderr, "<TempDir> "       );
       if ( !init[4] )  fprintf( stderr, "<Client> " );
       fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   if (Standalone == TRUE)
   {
	   nmiss = 0;
   	   for  (i = nCommonCmd; i < (nCommonCmd+nStandaloneCmd); i++ )  
			if( !init[i] ) nmiss++;
       if ( nmiss ) 
       {
              fprintf( stderr, "getfile: ERROR, no standalone " );
              if ( !init[5] )  fprintf( stderr, "<LogFile> "        );
              if ( !init[6] )  fprintf( stderr, "<LogFileDir> "        );
              fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
              exit( -1 );
       }
   }
   else
   {
	   nmiss = 0;
   	   for  (i = (nCommonCmd+nStandaloneCmd); i < NCOMMANDS; i++ )  
			if( !init[i] ) nmiss++;
       if ( nmiss ) 
       {
              fprintf( stderr, "getfile: ERROR, no Earthworm " );
              if ( !init[7] )  fprintf( stderr, "<MyModuleId> "           );
              if ( !init[8] )  fprintf( stderr, "<OutRingName> "        );
              if ( !init[9] )  fprintf( stderr, "<EwLogSwitch> "        );
              if ( !init[10] )  fprintf( stderr, "<HeartBeatInterval> "        );
              fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
              exit( -1 );
       }
   }
       
   return;
}

/******************************************************************************
 *  getfile_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
void getfile_lookup( void )
{

/* Look up keys to shared memory regions
   *************************************/
   if( ( OutRingKey = GetKey(OutRingName) ) == -1 ) {
        fprintf( stderr,
                "getfile:  Invalid ring name <%s>; exiting!\n", OutRingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "getfile: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "getfile: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "getfile: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "getfile: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

   return;
}

/******************************************************************************
 * getfile_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void getfile_status( unsigned char type, short ierr, char *note )
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
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "e", "getfile: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("e","getfile:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("e","getfile:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}



void LogConfig( void )
{
   int i;
   logit ("e", "COMMON CONFIGURATION:\n");
   logit ("e", "ServerIP:    %s\n", ServerIP );
   logit ("e", "ServerPort:  %d\n", ServerPort );
   logit ("e", "TimeOut:     %d\n", TimeOut );
   for (i = 0; i < nClient; i++ )
      logit ("e", "Client:    %15s   %s\n", Client[i].ip, Client[i].indir );

   if (Standalone == TRUE)
   {
      logit ("e", "STANDALONE CONFIGURATION:\n");
      logit ("e", "LogFileDir:  %s\n", LogFileDir );
      logit ("e", "LogFile:     %d\n", LogFile );
   }
   else
   {
      logit ("e", "EARTHWORM CONFIGURATION:\n");
      logit ("e", "MyModuleId:            %s\n", MyModName );
      logit ("e", "OutRingName:           %s\n", OutRingName );
      logit ("e", "EwLogSwitch:           %d\n", EwLogSwitch );
      logit ("e", "HeartBeatInterval:     %d\n", HeartBeatInterval );
   }

   return;
}


/********************** Heart Beating Thread ****************
 ************************************************************/
thr_ret     HeartBeater (void *dummy)
{

    /* Tell the main thread we're ok */
    HeartBeaterStatus = 0;

    while (1)
    {

        if (time(&timeNow) - timeLastBeat  >=  HeartBeatInterval)
        {
           timeLastBeat = timeNow;
           getfile_status (TypeHeartBeat, 0, "" );
        }

        /* see if a termination has been requested
        *****************************************/
        if ( tport_getflag( &OutRegion ) == TERMINATE ||
                    tport_getflag( &OutRegion ) == MyPid )
        {
            tport_detach (&OutRegion);
            logit( "t", "getfile_ew: Termination requested; exiting!\n" );
            fflush( stdout );
			HeartBeaterStatus = -1;
            KillSelfThread (); 
        }
        /* take a little nap; add 20030624: withers
        *******************************************/
        sleep_ew(1000);
     }
}
