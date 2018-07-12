#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sendfile_srv.h"


char ServerIP[20];             /* IP address of system to receive msg's */
char ClientIP[20];             /* IP address of trusted client */
int  ServerPort;               /* The well-known port number */
int  TimeOut;                  /* Send timeout, in seconds */
int  AckTimeOut;               /* receive timeout, in seconds */
int  RetryInterval;            /* If an error occurs, retry after this many seconds */
char OutDir[MAXPATHSTR];       /* Directory containing files to be sent */
char LogFileName[MAXPATHSTR];  /* Full name of log files */
char TimeZone[40];             /* Time zone of system clock */
int  LogFile;                  /* Flag value, 0 - 3 */
int  SendPause;                /* milliseconds to pause before shipping file */


void GetConfig( char *configFileName )
{
    FILE *fp;
    int  gotServer        = 0;
    int  gotClient        = 0;
    int  gotPort          = 0;
    int  gotTimeOut       = 0;
    int  gotAckTimeOut    = 0;
    int  gotRetryInterval = 0;
    int  gotOutDir        = 0;
    int  gotLogFileName   = 0;
    int  gotTimeZone      = 0;
    int  gotLogFile       = 0;

    AckTimeOut = 0;
    SendPause  = 0;
   
    fp = fopen( configFileName, "r" );
    if ( fp == NULL ) {
		printf( "Error opening configuration file: %s\n", configFileName );
		printf( "Usage: sendfileII <configFileName>\n" );
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
			  printf( "Error. Length of -ClientIP exceeds 19 characters. Exiting.\n" );
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
		else if ( strcmp( rc, "-AckTimeOut" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			AckTimeOut = atoi( rc );
			gotAckTimeOut = 1;
		}
		else if ( strcmp( rc, "-RetryInterval" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			RetryInterval = atoi( rc );
			gotRetryInterval = 1;
		}
		else if ( strcmp( rc, "-OutDir" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			strcpy( OutDir, rc );
			gotOutDir = 1;
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
		else if ( strcmp( rc, "-SendPause" ) == 0 ) {
			rc = strtok( NULL, " \t\n" );
			if ( rc == NULL ) continue;
			SendPause = atoi( rc );
		}
    }
    fclose( fp );

    if ( !gotServer ) {
		printf( "No -ServerIP commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotClient ) {
		printf( "No -ClientIP commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotPort ) {
		printf( "No -ServerPort commands in %s\n", configFileName );
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
    if ( !gotOutDir ) {
		printf( "No -OutDir commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotLogFileName ) {
		printf( "No -LogFileName commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotTimeZone ) {
		printf( "No -TimeZone commands in %s\n", configFileName );
		printf( "Exiting.\n" );
		exit( -1 );
    }
    if ( !gotLogFile ) {
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
    log( "e", "ClientIP:      %s\n", ClientIP );
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
