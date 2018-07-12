/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sendfileII.c,v 1.2 2006/03/23 00:05:10 dietz Exp $
 *
 *    Revision history:
 *     $Log: sendfileII.c,v $
 *     Revision 1.2  2006/03/23 00:05:10  dietz
 *     Added optional config command "-SendPause msec". sendfileII will sleep
 *     SendPause milliseconds before trying to open a file for shipping. This
 *     gives file-writers of unknown behavior some time to complete writin the
 *     file and close it before sendfileII wisks it away. Default=0.
 *
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */

/*********************************************************************
 *                              sendfileII                           *
 *  This program sends a file to a remote system running getfileII.  *
 *********************************************************************/

#include <stdio.h>
#include <string.h>
#include "sendfileII.h"

int main( int argc, char *argv[] )
{
    extern char OutDir[80];       /* Directory containing files to be sent */
    extern int  RetryInterval;    /* Retry after this many seconds */
    extern char LogFileName[80];  /* Full name of log files */
    extern char TimeZone[40];     /* Time zone of system clock */
    extern int  LogFile;          /* If 1, log to disk */
    extern int  AckTimeOut;       /* Receive timeout */
    extern int  SendPause;        /* Pre-shipping pause (msec) */
    char buf[BUFLEN];             /* Work buffer */
    char fname[100];
    FILE *fp;
    char defaultConfig[] = "sendfileII.d";
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
    if ( chdir_ew( OutDir ) == -1 )
    {
	log( "e", "Error. Can't change working directory to %s\n Exiting.",
	     OutDir );
	return -1;
    }

    /* Get the name of a file to send to getfileII.
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
        if( SendPause ) sleep_ew( SendPause );

	/* Open the file for reading only.  Since the file name
	   is in the directory we know the file exists, but it
	   may be in use by another process.  If so, fopen_excl()
	   will return NULL.  In this case, wait a second for the
	   other process to finish and try opening the file again.
	******************************************************/
	fp = fopen_excl( fname, "rb" );
	if ( fp == NULL )
	{
	    sleep_ew( 1000 );
	    continue;
	}

	/* Connect to getfileII program on remote system
         *******************************************/
	while ( ConnectToGetfile() == SENDFILE_FAILURE )
	{
	    log( "et", "Can't connect to getfileII. Will retry in %d sec...\n",
		 RetryInterval );
	    sleep_ew( 1000 * RetryInterval );
	}

	/* Send the pathless file name to getfileII
         **************************************/
	if ( SendBlockToSocket( fname, strlen(fname) ) == SENDFILE_FAILURE )
	{
	    log( "et", "Can't send file name to getfileII. Will retry in %d sec...\n",
		 RetryInterval );
	    fclose( fp );
	    sleep_ew( 1000 * RetryInterval );
	    continue;
	}

	/* Read BUFLEN bytes at a time and send them to getfileII
         ****************************************************/
	log( "et", "Sending file %s\n", fname );
	while ( 1 )
	{
	    int nbytes = fread( buf, sizeof(char), BUFLEN, fp );

	    if ( nbytes > 0 )
	    {
		if ( SendBlockToSocket( buf, nbytes ) == SENDFILE_FAILURE )
		{
		    send_failed = 1;
		    break;
		}
	    }

	    /* End-of-file encountered on read.
	       Send a zero-length message to getfileII.
	    *************************************/
	    if ( feof( fp ) )
	    {
		if ( SendBlockToSocket( buf, 0 ) == SENDFILE_FAILURE )
		    send_failed = 1;
		break;
	    }

	    /* If an fread error occurs, exit program.  No heartbeats will
	       be sent, so downstream software will know something is wrong.
	    ************************************************************/
	    if ( (nbytes == 0) && ferror( fp ) )
	    {
		read_error = 1;
		break;
	    }
	}

	/* Wait for an acknowledgement from our receiver.
         ************************************************/
	if (GetAckFromSocket(AckTimeOut) == SENDFILE_FAILURE)
	    send_failed = 1;

	/* Finish up with this file
         ************************/
	CloseSocketConnection();

	fclose( fp );

	if ( read_error )              /* Exit this program */
	{
	    log( "et", "fread() error on %s File partially sent. Exiting.\n",
		 fname );
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
}
