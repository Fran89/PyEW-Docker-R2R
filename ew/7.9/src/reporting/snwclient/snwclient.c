  /****************************************************************************
   *                              snwclient                                   *
   *  This program sends data to a remote system running SNWCollectionAgent.  *
   ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "snwclient.h"

void SocketSysInit( void );
void GetConfig( char * );
void LogConfig( void );
void log_init( char *, char *, int, int );
void log( char *, char *, ... );
int  chdir_ew( char * );
int  GetFileName( char [] );
int  ConnectToSNWAgent( void );
int  SendBlockToSocket( char *, int );
void CloseSocketConnection( void );
void sleep_ew( unsigned );
int  getstr( char [], char [], int );
FILE *fopen_excl( const char *, const char * );


int main( int argc, char *argv[] )
{
   extern char OutDir[80];       /* Directory containing files to be sent */
   extern int RetryInterval;     /* Retry after this many seconds         */
   extern char LogFileName[80];  /* Full name of log files                */
   extern char TimeZone[40];     /* Time zone of system clock             */
   extern int  LogFile;          /* If 1, log to disk                     */
   extern int  Verbose;          /* If 1, log to disk                     */
   char fname[100];
   FILE *fp;
   char defaultConfig[] = "snwclient.d";
   char *configFileName = (argc > 1 ) ? argv[1] : &defaultConfig[0];
   char line[4096];

/* Read the configuration file
   ***************************/
	GetConfig( configFileName );

/* Set up logging and log the config file
	**************************************/
	log_init( LogFileName, TimeZone, 256, LogFile );
	LogConfig();

/* Initialize the socket system
   ****************************/
	SocketSysInit();

/* Change working directory to "OutDir"
   ***********************************/
	if ( chdir_ew( OutDir ) == -1 )
	{
		log( "e", "Error. Can't change working directory to %s\n Exiting.", OutDir );
		return -1;
	}

/* Get the name of a file to send to getfile.
   If rc==1, the directory is empty.
   *****************************************/
	while ( 1 )
	{
		int rc;
		int send_failed = 0;
		int read_error  = 0;
		
		rc = GetFileName( fname );
		
		if ( rc == 1 )       /* Try again */
		{
			sleep_ew( 200 );
			continue;
		}
		
		if( strcmp(fname,"core")==0 ) {
            if( remove( fname ) != 0) {
                log("et", "Error: Cannot delete core file <%s>; exiting!", fname );
                break;
            }
            continue;
		}

/* Open the file for reading only.  Since the file name
   is in the directory we know the file exists, but it
   may be in use by another process.  If so, fopen_excl()
   will return NULL.  In this case, wait a second for the
   other process to finish and try opening the file again.
   ******************************************************/
		fp = fopen_excl( fname, "rb" );
		if ( fp == NULL )
		{
			printf( "Error opening data file: %s\n", fname );
			sleep_ew( 1000 );
			continue;
		}

/* Connect to SNWCollectionAgent on remote system
   **********************************************/
		while ( ConnectToSNWAgent() == SENDFILE_FAILURE )
		{
			log( "et", "Can't connect to SNWCollectionAgent. Will retry in %d sec...\n", RetryInterval );
			sleep_ew( 1000 * RetryInterval );
		}

/* Read BUFLEN bytes at a time and send them to SNWCollectionAgent
   ***************************************************************/
		if(Verbose) log( "et", "Sending file %s\n", fname );
		int totalbytes = 0;
		while ( 1 )
		{
			int nbytes;
			
			fgets( line, 4096, fp );
			if ( ferror( fp ) ) 
			{
				read_error = 1;
				break;
			}
			if ( feof( fp ) )   break;
			nbytes = (int)strlen(line);
			totalbytes += nbytes;

			if ( totalbytes > 5 )
			{
				if(Verbose>1) log( "et", "Sending line: %s \n", line );
				if ( SendBlockToSocket( line, nbytes ) == SENDFILE_FAILURE )
				{
					send_failed = 1;
					break;
				}
			sleep_ew( 50 );
			}
         }

/* Finish up with this file
   ************************/
		CloseSocketConnection();
		fclose( fp );
		
		if ( read_error )              /* Exit this program */
		{
			log( "et", "fread() error on %s File partially sent. Exiting.\n", fname );
			return -1;
		}
		else if ( send_failed )        /* Sleep a while */
		{
			log( "et", "Send error. Will retry in %d sec...\n", RetryInterval );
			sleep_ew( 1000 * RetryInterval );
		}
		else                           /* Erase local copy of file */
		{
			log( "et", "File %s sent.\n", fname );
			if ( remove( fname ) == -1 )
				log( "et", "Error erasing: %s\n", fname );
		}
	}
	return(0);
}
