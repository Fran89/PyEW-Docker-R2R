/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getfileII.c 4543 2011-08-10 14:54:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2007/02/26 20:51:24  paulf
 *     last windows warning for time_t squashed
 *
 *     Revision 1.3  2004/07/28 18:31:38  dietz
 *     changed unlink() to remove().
 *
 *     Revision 1.2  2004/07/28 17:59:41  dietz
 *     added more string-length checking to avoid array overflows
 *
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */

/****************************************************************
 *                           getfileII.c                        *
 *                                                              *
 *  Program to get files via a socket connection.               *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "getfileII.h"


int main( int argc, char *argv[] )
{
    extern int    nClient;               /* Number of trusted clients */
    extern CLIENT Client[MAXCLIENT];     /* Client list */
    extern char LogFileName[MAXPATHSTR]; /* Full name of log files */
    extern char TimeZone[40];            /* Time zone of system clock */
    extern int  LogFile;                 /* If 1, log to disk */
    extern char TempDir[MAXPATHSTR];     /* Name of temporary directory */
   
    int i;
    int rc;
    char defaultConfig[] = "getfileII.d";
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
    for ( i = 0; i < nClient; i++ )
    {
	if ( chdir_ew( Client[i].indir ) == -1 )
	{
            my_log( "et", "ERROR. Can't change working directory to %s Exiting.\n",
                 Client[i].indir );
            return -1;
	}
    }

    if ( chdir_ew( TempDir ) == -1 )
    {
	my_log( "et", "ERROR. Can't change working directory to %s Exiting.\n",
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
	FILE   *fp;
	static char buf[BUFLEN];
	int    nbytes;
	char   fname[MAXPATHSTR];
	int    total_bytes_received = 0;
	time_t tstart;
	int    clientIndex;

	if ( AcceptConnection( &clientIndex ) == GETFILE_FAILURE ) continue;

	/* Get the name of the file being received,
	   and the length of the file name.
	***************************************/
	rc = GetBlockFromSocket( buf, &nbytes );

	if ( rc == GETFILE_DONE )
	{
	    my_log( "et", "Received zero-length block.\n" );
	    CloseReadSock();
	    continue;
	}

	if ( rc == GETFILE_FAILURE )
	{
	    my_log( "et", "Error getting file name from socket.\n" );
	    CloseReadSock();
	    continue;
	}

	buf[nbytes] = '\0';
        if( nbytes < MAXPATHSTR )
        {
           strcpy( fname, buf );
	   my_log( "et", "Receiving file %s\n", fname );
        } 
        else
        {
           strncpy( fname, buf, MAXPATHSTR-1 );
           fname[MAXPATHSTR-1] = 0;
           my_log( "et", "Receiving file %s; filename being truncated to %s\n",
                 buf, fname );
        }

	/* Open a new file in the temporary directory
	   to contain the incoming data
	******************************************/
	fp = fopen( fname, "wb" );
	if ( fp == NULL )
	{
	    my_log( "et", "Error. Can't open new file %s\n", fname );
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

		my_log( "et", "File %s received\n", fname );
		my_log( "et", "%d bytes", total_bytes_received );
		my_log( "e", " in %ld second", (long) telapsed );
		if ( telapsed != 1 ) my_log( "e", "s" );
		if ( telapsed > 0 )
		{
		    double rate = (double)total_bytes_received / telapsed / 1024.;
		    my_log( "e", " (%.1lf kbytes/sec)", rate );
		}
		my_log( "e", "\n" );
		break;
	    }

	    if ( rc == GETFILE_FAILURE )
	    {
		my_log( "et", "GetBlockFromSocket() error.\n" );
		break;
	    }

	    if ( fwrite( buf, sizeof(char), nbytes, fp ) == 0 )
	    {
		my_log( "et", "Error writing new file.\n" );
		break;
	    }
	    total_bytes_received += nbytes;
	}
	if (fclose( fp ) != 0) {
	    my_log( "et", "Error closing new file\n");
	    rc = GETFILE_FAILURE;
	}
      

	/* Move file to client directory
         *****************************/
	if (rc != GETFILE_DONE) {
	    remove(fname);
	    my_log( "et", "Deleted incomplete file %s\n", fname);
	} else {
	    if ( SendAckToSocket() != GETFILE_SUCCESS)
		my_log( "et", "Error sending ACK to sender\n");
	    else
		my_log( "et", "ACK sent in reply\n");
	  
	    if ( rename_ew( fname, Client[clientIndex].indir ) == -1 )
		my_log( "et", "Error. Can't move file to client directory.\n" );
	}
      
	CloseReadSock();
    }                     /* End of "while loop". Accept next connection. */
}

