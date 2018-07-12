/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c 5379 2013-02-20 19:27:08Z paulf $
 *
 *    Revision history:
 *     $Log: config.c,v $
 *     Revision 1.3  2006/03/23 00:40:59  dietz
 *     Modified test for illegal SendPause values from <=0 to <0 (zero is allowed!)
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sendfileII.h"


char ServerIP[20];     /* IP address of system to receive msg's */
int  ServerPort;       /* The well-known port number */
int  TimeOut;          /* Send timeout, in seconds */
int  AckTimeOut;       /* receive timeout, in seconds */
int  RetryInterval;    /* If an error occurs, retry after this many seconds */
char OutDir[80];       /* Directory containing files to be sent */
char LogFileName[80];  /* Full name of log files */
char TimeZone[40];     /* Time zone of system clock */
int  LogFile;          /* Flag value, 0 - 3 */
int  SendPause;        /* milliseconds to pause before shipping file */


void GetConfig( char *configFileName )
{
    FILE *fp;
    int  gotServer        = 0;
    int  gotPort          = 0;
    int  gotTimeOut       = 0;
    int  gotRetryInterval = 0;
    int  gotOutDir        = 0;
    int  gotLogFileName   = 0;
    int  gotTimeZone      = 0;
    int  gotLogFile       = 0;

    AckTimeOut = 0;
    SendPause  = 0;
   
    fp = fopen( configFileName, "r" );
    if ( fp == NULL )
    {
	printf( "Error opening configuration file: %s\n", configFileName );
	printf( "Usage: sendfileII <configFileName>\n" );
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
	if ( strcmp( rc, "-ServerIP" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    strcpy( ServerIP, rc );
	    gotServer = 1;
	}
	else if ( strcmp( rc, "-ServerPort" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    ServerPort = atoi( rc );
	    gotPort = 1;
	}
	else if ( strcmp( rc, "-TimeOut" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    TimeOut = atoi( rc );
	    gotTimeOut = 1;
	}
	else if ( strcmp( rc, "-AckTimeOut" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    AckTimeOut = atoi( rc );
	}
	else if ( strcmp( rc, "-RetryInterval" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    RetryInterval = atoi( rc );
	    gotRetryInterval = 1;
	}
	else if ( strcmp( rc, "-OutDir" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    strcpy( OutDir, rc );
	    gotOutDir = 1;
	}
	else if ( strcmp( rc, "-LogFileName" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    strncpy( LogFileName, rc, 80 );
	    if ( LogFileName[79] != '\0' )
	    {
		printf( "Length of -LogFileName exceeds 79 characters. Exiting.\n" );
		exit( -1 );
	    }
	    gotLogFileName = 1;
	}
	else if ( strcmp( rc, "-TimeZone" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    strncpy( TimeZone, rc, 40 );
	    if ( TimeZone[39] != '\0' )
	    {
		printf( "Length of -TimeZone exceeds 39 characters. Exiting.\n");
		exit( -1 );
	    }
	    gotTimeZone = 1;
	}
	else if ( strcmp( rc, "-LogFile" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    LogFile = atoi( rc );
	    gotLogFile = 1;
	}
	else if ( strcmp( rc, "-SendPause" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    SendPause = atoi( rc );
	}
    }
    fclose( fp );

    if ( !gotServer )
    {
	printf( "No -ServerIP commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotPort )
    {
	printf( "No -ServerPort commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotTimeOut )
    {
	printf( "No -TimeOut commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotRetryInterval )
    {
	printf( "No -RetryInterval commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotOutDir )
    {
	printf( "No -OutDir commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotLogFileName )
    {
	printf( "No -LogFileName commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotTimeZone )
    {
	printf( "No -TimeZone commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotLogFile )
    {
	printf( "No -LogFile commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if (AckTimeOut <= 0) {
	printf( "AckTimeOut (%d) must be a positive integer value\n", 
		AckTimeOut);
	exit(-1);
    }
    if (SendPause < 0) {
	printf( "SendPause (%d) must be a positive integer value\n", 
		SendPause);
	exit(-1);
    }

    return;
}


void LogConfig( char *version )
{
    log( "e", "This is %s\n", version);
    log( "e", "ServerIP:      %s\n", ServerIP );
    log( "e", "ServerPort:    %d\n", ServerPort );
    log( "e", "TimeOut:       %d\n", TimeOut );
    log( "e", "AckTimeOut:    %d\n", AckTimeOut );
    log( "e", "RetryInterval: %d\n", RetryInterval );
    log( "e", "OutDir:        %s\n", OutDir );
    log( "e", "SendPause(ms): %d\n", SendPause );
    log( "e", "LogFileName:   %s\n", LogFileName );
    log( "e", "TimeZone:      %s\n", TimeZone );
    log( "e", "LogFile:       %d\n", LogFile );

    return;
}
