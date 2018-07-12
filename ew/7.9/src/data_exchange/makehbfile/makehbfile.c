/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: makehbfile.c 5856 2013-08-16 16:06:03Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/12/20 02:40:33  lombard
 *     Initial revision
 *
 *
 *
 */

/*********************************************************************
 *                             makehbfile                            *
 *    Make heartbeat files that are sent by sendfile to getfile.     *
 *********************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef _WINNT
#include <unistd.h>
#endif
#include "makehbfile.h"

int main( int argc, char *argv[] )
{
    extern char Path[PATHLEN];        /* Store heartbeat files here */
    char   tempDirName[] = "temp.dir";
    char defaultConfig[] = "makehbfile.d"; 
    char *configFileName = (argc > 1 ) ? argv[1] : &defaultConfig[0];

/* Read the configuration file
***************************/
    GetConfig( configFileName );
    /*   LogConfig(); */
 
/* Change working directory to "Path"
*********************************/
    if ( chdir_ew( Path ) == -1 )
    {
	printf( "Error. Can't change working directory to %s\n", Path );
	printf( "Exiting.\n" );
	return -1;
    }

/* Make a directory to contain temporary copies of heartbeat
   files.  The directory is a subdirectory of "Path".
*********************************************************/
    if ( mkdir_ew( tempDirName ) == -1 )
    {
	printf( "Error. Can't create temporary directory %s\n", tempDirName );
	printf( "Exiting.\n" );
	return -1;
    }
 
/* Change working directory to the temporary directory
***************************************************/
    if ( chdir_ew( tempDirName ) == -1 )
    {
	printf( "Error. Can't change working directory to %s\n", tempDirName );
	printf( "Exiting.\n" );
	return -1;
    }


/* Open the file for writing only.
   If an error occurs, sleep a while and try again.
***********************************************/
    while ( 1 )
    {
	extern char HbName[80];    /* Name of heartbeat files */
	extern int Interval;

	FILE *fp = fopen( HbName, "wb" );

	if ( fp == NULL )
	{
	    printf( "Error opening heartbeat file %s\n", HbName );
	    printf( "Exiting.\n" );
	    return -1;
	}

/* Write current time to the heartbeat file
****************************************/
	if ( fprintf( fp, "%d\n", (int) time(0) ) < 0 )
	    printf( "Error writing heartbeat file.\n" );
	if ( fclose( fp ) < 0)
	    printf( "Error writing heartbeat file.\n" );
	  

/* Move the heartbeat file to the parent directory (ie "Path")
***********************************************************/
	if ( rename_ew( HbName, ".." ) == -1 )
	    printf( "Error. Can't move heartbeat file.\n" );

	sleep( Interval );
    }
}
