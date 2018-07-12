
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <kom.h>
#include "sendfilemt.h"


int  LogFile;           // Flag value, 0 - 3
unsigned char MyModId;  // Module id
long RingKey;           // Transport ring key
int  HeartBeatInterval; // Seconds between heartbeats sent to statmgr
int  SendPause;         // Milliseconds to pause before shipping file
int  TimeOut;           // Send timeout, in seconds
int  AckTimeOut;        // Receive timeout, in seconds
int  RetryInterval;     // If an error occurs, retry after this many seconds
int  nSrv = 0;          // Number of Server lines in config file
SERVER *srv = NULL;


void GetConfig( char *configfile )
{
#define NCOMMAND 9
   const int ncommand = NCOMMAND;   // Number of commands to process
   char      init[NCOMMAND] = {0};  // Flags, one for each command
   int       nmiss = 0;             // Number of commands that were missed
   int       nfiles;
   int       i;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
        printf( "Error opening command file <%s>. Exiting.\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
   *************************/
   while ( nfiles > 0 )         // While there are command files open
   {
      while ( k_rd() )          // Read next line from active file
      {
         char *com = k_str();   // Get first token from line
         char *str;

/* Ignore blank lines & comments
   *****************************/
           if ( !com )          continue;
           if ( com[0] == '#' ) continue;

/* Open a nested configuration file
   ********************************/
           if ( com[0] == '@' )
           {
              int success = nfiles + 1;
              nfiles  = k_open( &com[1] );
              if ( nfiles != success )
              {
                 printf( "Error opening command file <%s>. Exiting.\n",
                     &com[1] );
                 exit( -1 );
              }
              continue;
           }

           if( k_its( "LogFile" ) )
           {
              LogFile = k_int();
              init[0] = 1;
           }

           else if ( k_its( "MyModuleId" ) )
           {
              str = k_str();
              if ( str )
              {
                 if ( GetModId( str, &MyModId ) < 0 )
                 {
                    printf( "Invalid MyModuleId <%s> in <%s>",
                             str, configfile );
                    printf( ". Exiting.\n" );
                    exit( -1 );
                 }
              }
              init[1] = 1;
           }

           else if( k_its( "RingName" ) )
           {
              str = k_str();
              if (str)
              {
                 if ( ( RingKey = GetKey(str) ) == -1 )
                 {
                    printf( "Invalid RingName <%s> in <%s>",
                             str, configfile );
                    printf( ". Exiting.\n" );
                    exit( -1 );
                 }
              }
              init[2] = 1;
           }

           else if( k_its( "HeartBeatInterval" ) )
           {
              HeartBeatInterval = k_int();
              init[3] = 1;
           }

           else if( k_its( "SendPause" ) )
           {
              SendPause = k_int();
              init[4] = 1;
           }

           else if( k_its( "TimeOut" ) )
           {
              TimeOut = k_int();
              init[5] = 1;
           }

           else if( k_its( "AckTimeOut" ) )
           {
              AckTimeOut = k_int();
              init[6] = 1;
           }

           else if( k_its( "RetryInterval" ) )
           {
              RetryInterval = k_int();
              init[7] = 1;
           }

           else if( k_its( "Server" ) )
           {
              SERVER *anotherSrv;

              nSrv++;
              anotherSrv = (SERVER *) realloc( srv, nSrv * sizeof(SERVER) );
              if ( anotherSrv != NULL )
              {
                 srv = anotherSrv;

                 str = k_str();
                 if (str)
                 {
                    if ( strlen(str) > 12 )
                    {
                       printf( "Error. Tag is more than 12 characters long.\n" );
                       printf( "Length: %d   Exiting.\n", strlen(str) );
                       exit( -1 );
                    }
                    strcpy( srv[nSrv-1].tag, str );
                 }

                 str = k_str();
                 if (str)
                 {
                    if ( strlen(str) > 20 )
                    {
                       printf( "Error. serverIP is more than 20 characters long.\n" );
                       printf( "Length: %d   Exiting.\n", strlen(str) );
                       exit( -1 );
                    }
                    strcpy( srv[nSrv-1].ip, str );
                 }

                 srv[nSrv-1].port = k_int();

                 str = k_str();
                 if (str)
                 {
                    if ( strlen(str) > 80 )
                    {
                       printf( "Error. outDir is more than 80 characters long.\n" );
                       printf( "Length: %d   Exiting.\n", strlen(str) );
                       exit( -1 );
                    }
                    strcpy( srv[nSrv-1].outDir, str );
                 }
                 init[8] = 1;
              }
              else
              {
                 free( srv );
                 printf( "Error reallocating memory. Exiting.\n" );
                 exit( -1 );
              }
           }
           else
           {
              printf( "<%s> unknown command in <%s>.\n",
                       com, configfile );
              continue;
           }

/* See if there were any errors processing the command
   ***************************************************/
           if ( k_err() )
           {
              printf( "Bad <%s> command in <%s>; \n",
                       com, configfile );
              exit( -1 );
           }
        }
        nfiles = k_close();
   }

/* Check flags for missed commands
   *******************************/
   for ( i = 0; i < ncommand; i++ )
      if( !init[i] ) nmiss++;

   if ( nmiss )
   {
      printf( "ERROR, no " );
      if ( !init[0] ) printf( "<LogFile> "           );
      if ( !init[1] ) printf( "<MyModuleId> "        );
      if ( !init[2] ) printf( "<RingName> "          );
      if ( !init[3] ) printf( "<HeartBeatInterval> " );
      if ( !init[4] ) printf( "<SendPause> "         );
      if ( !init[5] ) printf( "<TimeOut> "           );
      if ( !init[6] ) printf( "<AckTimeOut> "        );
      if ( !init[7] ) printf( "<RetryInterval> "     );
      if ( !init[8] ) printf( "<Server> "            );
      printf( "command(s) in <%s>. Exiting.\n", configfile );
      exit( -1 );
   }
   return;
}


void LogConfig( void )
{
   int i;

   logit( "e", "This is sendfilemt, V%s\n\n", SENDFILE_VERSION );

   logit( "e", "LogFile:              %d\n",   LogFile );
   logit( "e", "MyModId:              %u\n",   MyModId );
   logit( "e", "RingKey:              %d\n",   RingKey );
   logit( "e", "HeartBeatInterval(s): %d\n",   HeartBeatInterval );
   logit( "e", "SendPause(ms):        %d\n",   SendPause );
   logit( "e", "TimeOut(s):           %d\n",   TimeOut );
   logit( "e", "AckTimeOut(s):        %d\n",   AckTimeOut );
   logit( "e", "RetryInterval(s):     %d\n\n", RetryInterval );

   logit( "e", "    Tag" );
   logit( "e", "          ServerIP" );
   logit( "e", "      Port" );
   logit( "e", "           OutDir\n" );
   logit( "e", "    ---" );
   logit( "e", "          --------" );
   logit( "e", "      ----" );
   logit( "e", "           ------\n" );
   for ( i = 0; i < nSrv; i++ )
   {
      logit( "e", "%-12s", srv[i].tag );
      logit( "e", "   %-15s", srv[i].ip );
      logit( "e", " %d",      srv[i].port );
      logit( "e", "   %s\n",  srv[i].outDir );
   }
   logit( "e", "\n" );
   return;
}
