/**************************************************************************
 *                              sendfile_srv                              *
 *  This program sends a file to a remote system running getfile_client.  *
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include "sendfile_srv.h"

int main( int argc, char *argv[] )
{
    extern char OutDir[80];       /* Directory containing files to be sent */
    extern int  RetryInterval;    /* Retry after this many seconds */
    extern char LogFileName[80];  /* Full name of log files */
    extern char TimeZone[40];     /* Time zone of system clock */
    extern int  LogFile;          /* If 1, log to disk */
    extern int  AckTimeOut;       /* Receive timeout */
    extern int  SendPause;        /* Pre-shipping pause (msec) */
    char buf[BUFLEN], fname[100];
	int rc;
    FILE *fp;
    char defaultConfig[] = "sendfile_srv.d";
    char *configFileName = (argc > 1 ) ? argv[1] : &defaultConfig[0];

    /* Read the configuration file
     ***************************/
    GetConfig( configFileName );

    /* Set up logging and log the config file
     **************************************/
    log_init( LogFileName, TimeZone, 256, LogFile );
    LogConfig(VERSION);

    /* Initialize the socket system
     ****************************/
    SocketSysInit();

    /* Change working directory to "OutDir"
     ***********************************/
    if ( chdir_ew( OutDir ) == -1 ) {
		log( "e", "Error. Can't change working directory to %s\n Exiting.", OutDir );
		return -1;
    }

    /* Initialize the server socket
     ****************************/
    InitServerSocket();

    /* Send a file via socket connection.
       AcceptConnection() blocks until a client connects.
       This is an infinite loop that is never broken.
    *************************************************/
    while ( 1 ) {

		sleep_ew( 2000 );
		if ( AcceptConnection( ) == SENDFILE_FAILURE ) continue;
	
		/* Get the name of a file to send to getfile_client.
		   If rc==1, the directory is empty.
		*****************************************/
		while ( 1 ) {
			int send_failed = 0;
			int read_error  = 0;
		
			rc = GetFileName( fname );
		
			if ( rc == 1 ) {      /* Try again */
			/* Send a dummy file name to getfile_client
			   to indicate no files available.
				 **************************************/
				log( "et", "No files to be sent to getfile_client at this time.\n");
				strcpy(fname, "No/Files");
				if ( SendBlockToSocket( fname, strlen(fname) ) == SENDFILE_FAILURE ) {
					log( "et", "Can't send file name %s to getfile_client. Will retry in %d sec...\n", fname, RetryInterval );
					sleep_ew( 1000 * RetryInterval );
					continue;
				}
				sleep_ew( 200 );
				break;
			}
			if( SendPause ) sleep_ew( SendPause );
	
		/* Open the file for reading only.  Since the file name
		   is in the directory we know the file exists, but it
		   may be in use by another process.  If so, fopen_excl()
		   will return NULL.  In this case, wait a second for the
		   other process to finish and try opening the file again.
		******************************************************/
			fp = fopen_excl( fname, "rb" );
			if ( fp == NULL ) {
				sleep_ew( 1000 );
				continue;
			}
	
		/* Send the pathless file name to getfile_client
			 **************************************/
			if ( SendBlockToSocket( fname, strlen(fname) ) == SENDFILE_FAILURE ) {
				log( "et", "Can't send file name to getfile_client. Will retry in %d sec...\n", RetryInterval );
				fclose( fp );
				sleep_ew( 1000 * RetryInterval );
				continue;
			}
	
		/* Read BUFLEN bytes at a time and send them to getfile_client
			 ****************************************************/
			log( "et", "Sending file %s\n", fname );
			while ( 1 ) {
				int nbytes = fread( buf, sizeof(char), BUFLEN, fp );
		
				if ( nbytes > 0 ) {
					if ( SendBlockToSocket( buf, nbytes ) == SENDFILE_FAILURE ) {
						send_failed = 1;
						break;
					}
				}
	
			/* End-of-file encountered on read.
			   Send a zero-length message to getfile_client.
			*************************************/
				if ( feof( fp ) ) {
					if ( SendBlockToSocket( buf, 0 ) == SENDFILE_FAILURE ) send_failed = 1;
					break;
				}
	
			/* If an fread error occurs, exit program.  No heartbeats will
			   be sent, so downstream software will know something is wrong.
			************************************************************/
				if ( (nbytes == 0) && ferror( fp ) ) {
					read_error = 1;
					break;
				}
			}
	
		/* Wait for an acknowledgement from our receiver.
		 ************************************************/
			if (GetAckFromSocket(AckTimeOut) == SENDFILE_FAILURE)  send_failed = 1;
	
		/* Finish up with this file
		 **************************/
			fclose( fp );
	
			if ( read_error ) {             /* Exit this program */
				log( "et", "fread() error on %s File partially sent. Exiting.\n", fname );
				return -1;
			}
			else if ( send_failed ) {       /* Sleep a while */
				log( "et", "Send error. Will retry in %d sec...\n", RetryInterval );
				sleep_ew( 1000 * RetryInterval );
			}
			else {                          /* Erase local copy of file */
				log( "et", "File %s sent.\n", fname );
				if ( remove( fname ) == -1 )
				log( "et", "Error erasing: %s\n", fname );
			}
			break;
		}
		CloseSocketConnection();
    }
}
