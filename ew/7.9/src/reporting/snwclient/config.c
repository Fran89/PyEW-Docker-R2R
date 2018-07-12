
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void log( char *, char *, ... );

char ServerIP[20];     /* IP address of system to receive msg's */
int  ServerPort;       /* The well-known port number */
int  TimeOut;          /* Send/receive timeout, in seconds */
int  RetryInterval;    /* If an error occurs, retry after this many seconds */
char OutDir[80];       /* Directory containing files to be sent */
char LogFileName[80];  /* Full name of log files */
char TimeZone[40];     /* Time zone of system clock */
int  LogFile;          /* Flag value, 0 - 3 */
int  Verbose;          /* Log level, 0 - 3 */


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

   fp = fopen( configFileName, "r" );
   if ( fp == NULL )
   {
      printf( "Error opening configuration file: %s\n", configFileName );
      printf( "Usage: sendfile <configFileName>\n" );
      printf( "Exiting.\n" );
      exit( -1 );
   }

   Verbose = 0;
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
      else if ( strcmp( rc, "-Verbose" ) == 0 )
      {
         rc = strtok( NULL, " \t\n" );
         if ( rc == NULL ) continue;
         Verbose = atoi( rc );
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
   return;
}


void LogConfig( void )
{
   log( "e", "ServerIP:      %s\n", ServerIP );
   log( "e", "ServerPort:    %d\n", ServerPort );
   log( "e", "TimeOut:       %d\n", TimeOut );
   log( "e", "RetryInterval: %d\n", RetryInterval );
   log( "e", "OutDir:        %s\n", OutDir );
   log( "e", "LogFileName:   %s\n", LogFileName );
   log( "e", "TimeZone:      %s\n", TimeZone );
   log( "e", "LogFile:       %d\n", LogFile );
   return;
}
