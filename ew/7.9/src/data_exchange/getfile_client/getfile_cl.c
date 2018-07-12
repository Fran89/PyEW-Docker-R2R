/****************************************************************
 *                           getfile_cl.c                        *
 *                                                              *
 *  Program to get files via a socket connection.               *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "getfile_cl.h"


int main( int argc, char *argv[] )
{
    extern int  RetryInterval;           /* Retry after this many seconds */
    extern int  ConnectInterval;         /* Check for available files after this many seconds */
    extern char InDir[MAXPATHSTR];       /* Where to store files on the client */
    extern char LogFileName[MAXPATHSTR]; /* Full name of log files */
    extern char TimeZone[40];            /* Time zone of system clock */
    extern int  LogFile;                 /* If 1, log to disk */
    extern char TempDir[MAXPATHSTR];     /* Name of temporary directory */
   
    int i;
    int rc;
    char defaultConfig[] = "getfile_cl.d";
    char *configFileName = (argc > 1 ) ? argv[1] : &defaultConfig[0];

    /* Read the configuration file
     ***************************/
    GetConfig( configFileName );

    /* Set up logging and log the config file
     **************************************/
    log_init( LogFileName, TimeZone, MAXPATHSTR*4, LogFile );
    LogConfig(VERSION);

    /* Make sure all the necessary directories exist.
       The temporary directory becomes the current directory.
    *****************************************************/
	if ( chdir_ew( InDir ) == -1 ) {
		log( "et", "ERROR. Can't change working directory to %s Exiting.\n", InDir );
		return -1;
	}

    if ( chdir_ew( TempDir ) == -1 ) {
		log( "et", "ERROR. Can't change working directory to %s Exiting.\n", TempDir );
		return -1;
    }

    /* Initialize the socket system
     ****************************/
    SocketSysInit();

    /* Get file via socket connection.
       This is an infinite loop that is never broken.
    *************************************************/
    while ( 1 ) {
		FILE   *fp;
		static char buf[BUFLEN];
		int    nbytes;
		char   fname[MAXPATHSTR];
		int    total_bytes_received = 0;
		time_t tstart;
	
		/* Connect to sendfile_srv program on remote system
		 **************************************************/
		while ( ConnectToSendfile() == GETFILE_FAILURE ) {
			log( "et", "Can't connect to sendfile_srv. Will retry in %d sec...\n", RetryInterval );
			sleep_ew( 1000 * RetryInterval );
		}

	/* Get the name of the file being received,
	   and the length of the file name.
	***************************************/
		rc = GetBlockFromSocket( buf, &nbytes );
	
		if ( rc == GETFILE_DONE ) {
			log( "et", "Received zero-length block.\n" );
			CloseReadSock();
			continue;
		}
	
		if ( rc == GETFILE_FAILURE ) {
			log( "et", "Error getting file name from socket.\n" );
			CloseReadSock();
			sleep_ew( 1000 * RetryInterval );
			continue;
		}
	
		buf[nbytes] = '\0';
        if(strcmp(buf, "No/Files") == 0) {
			log( "et", "No files available for transfer.\n" );
			CloseReadSock();
			sleep_ew( 1000 * ConnectInterval );
			continue;
        }
        if( nbytes < MAXPATHSTR ) {
           strcpy( fname, buf );
		   log( "et", "Receiving file %s\n", fname );
        } 
        else {
           strncpy( fname, buf, MAXPATHSTR-1 );
           fname[MAXPATHSTR-1] = 0;
           log( "et", "Receiving file %s; filename being truncated to %s\n", buf, fname );
        }

	/* Open a new file in the temporary directory
	   to contain the incoming data
	******************************************/
		fp = fopen( fname, "wb" );
		if ( fp == NULL ) {
			log( "et", "Error. Can't open new file %s\n", fname );
			CloseReadSock();
			continue;
		}

	/* Get the contents of the file, one block at a time,
	   and write them to the local disk file.
	   If a zero-length block is received, it means we
	   have the whole file.
	*************************************************/
		time( &tstart );
	
		while ( 1 ) {
			rc = GetBlockFromSocket( buf, &nbytes );
	
			if ( rc == GETFILE_DONE ) {
				time_t telapsed = time(0) - tstart;
		
				log( "et", "File %s received\n", fname );
				log( "et", "%d bytes", total_bytes_received );
				log( "e", " in %ld second", (long) telapsed );
				if ( telapsed != 1 ) log( "e", "s" );
				if ( telapsed > 0 ) {
					double rate = (double)total_bytes_received / telapsed / 1024.;
					log( "e", " (%.1lf kbytes/sec)", rate );
				}
				log( "e", "\n" );
				break;
			}
	
			if ( rc == GETFILE_FAILURE ) {
				log( "et", "GetBlockFromSocket() error.\n" );
				break;
			}
	
			if ( fwrite( buf, sizeof(char), nbytes, fp ) == 0 ) {
				log( "et", "Error writing new file.\n" );
				break;
			}
			total_bytes_received += nbytes;
		}
		if (fclose( fp ) != 0) {
			log( "et", "Error closing new file\n");
			rc = GETFILE_FAILURE;
		}
      
	/* Move file to client directory
         *****************************/
		if (rc != GETFILE_DONE) {
			remove(fname);
			log( "et", "Deleted incomplete file %s\n", fname);
		} else {
			if ( SendAckToSocket() != GETFILE_SUCCESS)
			log( "et", "Error sending ACK to sender\n");
			else
			log( "et", "ACK sent in reply\n");
		  
			if ( rename_ew( fname, InDir ) == -1 )
			log( "et", "Error. Can't move file to client directory.\n" );
		}
		  
		CloseReadSock();
    }                     /* End of "while loop". Accept next connection. */
}

