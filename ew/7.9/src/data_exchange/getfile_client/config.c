#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getfile_cl.h"

char   ServerIP[20];           /* IP address of system to send msg's */
int    ServerPort;             /* The well-known port number */
int    TimeOut;                /* Send/receive timeout, in seconds */
int    RetryInterval;          /* If an error occurs, retry after this many seconds */
int    ConnectInterval;        /* Check for available files after this many seconds */
char   TempDir[MAXPATHSTR];    /* Name of temporary directory */
char   LogFileName[MAXPATHSTR];/* Full name of log files */
char   TimeZone[40];           /* Time zone of system clock */
int    LogFile;                /* Flag value, 0 - 3 */
char   ClientIP[20];           /* IP address of trusted client */
char   InDir[MAXPATHSTR];      /* Where to store files on the client */

/* config.c for getfile_cl
 ***************************/
void GetConfig( char *configFileName )
{
    FILE *fp;
    int  gotServer      = 0;
    int  gotClient      = 0;
    int  gotPort        = 0;
    int  gotTimeOut     = 0;
    int  gotRetryInterval   = 0;
    int  gotConnectInterval = 0;
    int  gotTempDir     = 0;
    int  gotInDir       = 0;
    int  gotLogFileName = 0;
    int  gotTimeZone    = 0;
    int  gotLogFile     = 0;

    fp = fopen( configFileName, "r" );
    if ( fp == NULL ) {
		printf( "Error opening configuration file: %s\n", configFileName );
		printf( "Usage: getfileII <configFileName>\n" );
		printf( "Exiting.\n" );
		exit( -1 );
    }

    while ( 1 ) {
		char line[MAXPATHSTR*2];
		char *rc;
	
		fgets( line, MAXPATHSTR*2, fp );
		if ( ferror( fp ) ) break;
		if ( feof( fp ) )   break;
		rc = strtok( line, " \t\n" );
		if ( rc == NULL ) continue;
	
		if ( strcmp( rc, "-ServerIP" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			if ( strlen(rc) >= 19 ) {
			  printf( "Error. Length of -ServerIP exceeds 19 characters. Exiting.\n" );
			  exit( -1 );
			} 
			strcpy( ServerIP, rc );
			gotServer = 1;
		}
		else if ( strcmp( rc, "-ServerPort" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			ServerPort = atoi( rc );
			gotPort = 1;
		}
		else if ( strcmp( rc, "-ClientIP" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			if ( strlen(rc) >= 19 ) {
			  printf( "Error. Length of -ClientIP exceeds 19 characters. Exiting.\n" );
			  exit( -1 );
			} 
			strcpy( ClientIP, rc );
			gotClient = 1;
		}
		else if ( strcmp( rc, "-TimeOut" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			TimeOut = atoi( rc );
			gotTimeOut = 1;
		}
		else if ( strcmp( rc, "-RetryInterval" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			RetryInterval = atoi( rc );
			gotRetryInterval = 1;
		}
		else if ( strcmp( rc, "-ConnectInterval" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			ConnectInterval = atoi( rc );
			gotConnectInterval = 1;
		}
		else if ( strcmp( rc, "-TempDir" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
				if ( strlen(rc) > MAXPATHSTR-1 ) {
				   printf( "Error. Length of -TempDir exceeds %d characters. Exiting.\n", MAXPATHSTR-1 );
				   exit( -1 );
				} 
				strcpy( TempDir, rc );
			gotTempDir = 1;
		}
		else if ( strcmp( rc, "-InDir" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
				if ( strlen(rc) > MAXPATHSTR-1 ) {
				   printf( "Error. Length of -InDir exceeds %d characters. Exiting.\n", MAXPATHSTR-1 );
				   exit( -1 );
				} 
				strcpy( InDir, rc );
			gotInDir = 1;
		}
		else if ( strcmp( rc, "-LogFileName" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
				if ( strlen(rc) > MAXPATHSTR-1 ) {
				   printf( "Error. Length of -LogFileName exceeds %d characters. Exiting.\n", MAXPATHSTR-1 );
				   exit( -1 );
				} 
				strcpy( LogFileName, rc );
			gotLogFileName = 1;
		}
		else if ( strcmp( rc, "-TimeZone" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			if ( strlen(rc) > 39 ) {
				printf( "Error. Length of -TimeZone exceeds 39 characters. Exiting.\n");
				exit( -1 );
			}
			strcpy( TimeZone, rc );
			gotTimeZone = 1;
		}
		else if ( strcmp( rc, "-LogFile" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			LogFile = atoi( rc );
			gotLogFile = 1;
		}
    }
    fclose( fp );

    if ( !gotServer ) {
		printf( "No -ServerIP commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotPort ) {
		printf( "No -ServerPort commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotClient ) {
		printf( "No -ClientIP commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotTimeOut ) {
		printf( "No -TimeOut commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotRetryInterval ) {
		printf( "No -RetryInterval commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotConnectInterval ) {
		printf( "No -ConnectInterval commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotTempDir ) {
		printf( "No -TempDir commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotInDir ) {
		printf( "No -InDir commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotLogFileName ) {
		printf( "No -LogFileName commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
#ifndef __unix
    if ( !gotTimeZone ) {
		printf( "No -TimeZone commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
#endif
    if ( !gotLogFile ) {
		printf( "No -LogFile commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    return;
}


void LogConfig( char *version )
{
    int i;
    log( "e", "This is %s\n", version);
    log( "e", "ServerIP:    %s\n", ServerIP );
    log( "e", "ServerPort:  %d\n", ServerPort );
    log( "e", "ClientIP:    %s\n", ClientIP );
    log( "e", "TimeOut:     %d\n", TimeOut );
    log( "e", "RetryInterval: %d\n", RetryInterval );
    log( "e", "ConnectInterval: %d\n", ConnectInterval );
    log( "e", "LogFile:     %d\n", LogFile );
    log( "e", "LogFileName: %s\n", LogFileName );
    log( "e", "TimeZone:    %s\n", TimeZone );
    log( "e", "TempDir:     %s\n", TempDir );
    log( "e", "InDir:       %s\n", InDir );
   
    return;
}
