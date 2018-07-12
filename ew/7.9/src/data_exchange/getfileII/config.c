/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c 4543 2011-08-10 14:54:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/07/28 17:59:41  dietz
 *     added more string-length checking to avoid array overflows
 *
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getfileII.h"

char   ServerIP[20];           /* IP address of system to receive msg's */
int    ServerPort;             /* The well-known port number */
int    nClient = 0;            /* Number of trusted clients */
CLIENT Client[MAXCLIENT];      /* List if trusted clients */
int    TimeOut;                /* Send/receive timeout, in seconds */
char   TempDir[MAXPATHSTR];    /* Name of temporary directory */
char   LogFileName[MAXPATHSTR];/* Full name of log files */
char   TimeZone[40];           /* Time zone of system clock */
int    LogFile;                /* Flag value, 0 - 3 */


void GetConfig( char *configFileName )
{
    FILE *fp;
    int  gotServer      = 0;
    int  gotPort        = 0;
    int  gotTimeOut     = 0;
    int  gotTempDir     = 0;
    int  gotLogFileName = 0;
    int  gotTimeZone    = 0;
    int  gotLogFile     = 0;

    fp = fopen( configFileName, "r" );
    if ( fp == NULL )
    {
	printf( "Error opening configuration file: %s\n", configFileName );
	printf( "Usage: getfileII <configFileName>\n" );
	printf( "Exiting.\n" );
	exit( -1 );
    }

    while ( 1 )
    {
	char line[MAXPATHSTR*2];
	char *rc;

	fgets( line, MAXPATHSTR*2, fp );
	if ( ferror( fp ) ) break;
	if ( feof( fp ) )   break;
	rc = strtok( line, " \t\n" );
	if ( rc == NULL ) continue;

	if ( strcmp( rc, "-ServerIP" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
            if ( strlen(rc) >= 19 )
            {
              printf( "Error. Length of -ServerIP exceeds 19 characters. Exiting.\n" );
              exit( -1 );
            } 
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
	else if ( strcmp( rc, "-Client" ) == 0 )
	{
	    if ( nClient < MAXCLIENT )
	    {
		rc = strtok( NULL, " \t\n" );
		if ( rc == NULL ) continue;
                if ( strlen(rc) > 19 )
                {
		  printf( "Error. IPaddr in -Client exceeds 19 characters. Exiting.\n" );
		  exit( -1 );
                } 
		strcpy( Client[nClient].ip, rc );

		rc = strtok( NULL, " \t\n" );
		if ( rc == NULL ) continue;
                if ( strlen(rc) > MAXPATHSTR-1 )
                {
		  printf( "Error. Directory in -Client exceeds %d characters. Exiting.\n",
                          MAXPATHSTR-1 );
		  exit( -1 );
                } 
		strcpy( Client[nClient].indir, rc );
 		nClient++;
	    }
	    else
	    {
		printf( "Error. Too many -Client lines. Max=%d", MAXCLIENT );
		printf( "Exiting.\n" );
		exit( -1 );
	    }
	}
	else if ( strcmp( rc, "-TimeOut" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    TimeOut = atoi( rc );
	    gotTimeOut = 1;
	}
	else if ( strcmp( rc, "-TempDir" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
            if ( strlen(rc) > MAXPATHSTR-1 )
            {
               printf( "Error. Length of -TempDir exceeds %d characters. Exiting.\n",
                        MAXPATHSTR-1 );
               exit( -1 );
            } 
            strcpy( TempDir, rc );
	    gotTempDir = 1;
	}
	else if ( strcmp( rc, "-LogFileName" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
            if ( strlen(rc) > MAXPATHSTR-1 )
            {
               printf( "Error. Length of -LogFileName exceeds %d characters. Exiting.\n",
                        MAXPATHSTR-1 );
               exit( -1 );
            } 
            strcpy( LogFileName, rc );
	    gotLogFileName = 1;
	}
	else if ( strcmp( rc, "-TimeZone" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
            if ( strlen(rc) > 39 )
	    {
		printf( "Error. Length of -TimeZone exceeds 39 characters. Exiting.\n");
		exit( -1 );
	    }
	    strcpy( TimeZone, rc );
	    gotTimeZone = 1;
	}
	else if ( strcmp( rc, "-LogFile" ) == 0 )
	{
	    rc = strtok( NULL, " \t\n" );
	    if ( rc == NULL ) continue;
	    LogFile = atoi( rc );
	    gotLogFile = 1;
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
    if ( nClient == 0 )
    {
	printf( "No -Client commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotTimeOut )
    {
	printf( "No -TimeOut commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotTempDir )
    {
	printf( "No -TempDir commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    if ( !gotLogFileName )
    {
	printf( "No -LogFileName commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
#ifndef __unix
    if ( !gotTimeZone )
    {
	printf( "No -TimeZone commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
#endif
    if ( !gotLogFile )
    {
	printf( "No -LogFile commands in %s\n", configFileName );
	printf( "Exiting.\n" );
	exit( -1 );
    }
    return;
}


void LogConfig( char *version )
{
    int i;
    my_log( "e", "This is %s\n", version);
    my_log( "e", "ServerIP:    %s\n", ServerIP );
    my_log( "e", "ServerPort:  %d\n", ServerPort );
    my_log( "e", "TimeOut:     %d\n", TimeOut );
    my_log( "e", "LogFileName: %s\n", LogFileName );
    my_log( "e", "TimeZone:    %s\n", TimeZone );
    my_log( "e", "LogFile:     %d\n", LogFile );
   
    for ( i = 0; i < nClient; i++ )
	my_log( "e", "Client:      %-20s  %s\n", Client[i].ip, Client[i].indir );
    return;
}
