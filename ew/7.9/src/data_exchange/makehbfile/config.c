/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c 1157 2002-12-20 02:41:38Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/12/20 02:40:33  lombard
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "makehbfile.h"

char Path[PATHLEN];   /* Store heartbeat files here */
char HbName[80];      /* Name of heartbeat files to be created */
int  Interval;        /* Create a heartbeat file every "Interval" seconds */


void GetConfig( char *configFileName )
{
    FILE *fp;
    int  gotPath     = 0;
    int  gotHbName   = 0;
    int  gotInterval = 0;

    fp = fopen( configFileName, "r" );
    if ( fp == NULL )
    {
	printf( "Error opening configuration file: %s\n", configFileName );
	printf( "Usage: makehbfile <configFileName>\n" ); 
	printf( "Exiting.\n" );
	exit( -1 );
    }

    while ( 1 )
    {
	char line[80];
	char *rc;

	fgets( line, 80, fp );
	if ( ferror( fp ) ) break;
	if ( feof( fp ) )   break;
	rc = strtok( line, " \t\n" );
	if ( rc == NULL ) continue;
	if ( strcmp( rc, "-Path" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    strcpy( Path, rc );
	    gotPath = 1;
	}
	else if ( strcmp( rc, "-HbName" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    strcpy( HbName, rc );
	    gotHbName = 1;
	}
	else if ( strcmp( rc, "-Interval" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    Interval = atoi( rc );
	    gotInterval = 1;
	}
    }
    fclose( fp );

    if ( !gotPath )
    {
	printf( "No -Path commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotHbName )
    {
	printf( "No -HbName commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotInterval )
    {
	printf( "No -Interval commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    return;
}


void LogConfig( void )
{
    printf( "Path:     %s\n", Path );
    printf( "HbName:   %s\n", HbName );
    printf( "Interval: %d\n", Interval );
    putchar( '\n' );
    return;
}
