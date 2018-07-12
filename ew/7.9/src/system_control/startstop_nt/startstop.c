       /**********************************************************
        *                       startstop.c                      *
        *                                                        *
        *     Program to start and stop the Earthworm system     *
        *                   Windows NT version                   *
        **********************************************************/

#include <startstop_winlib.h>
#include <startstop_version.h>

#define PROGRAM_NAME "startstop_nt"


METARING metaring;
void Interactive( void * );        /* Interactive thread              */
static char ewstat[MAX_STATUS_LEN];
volatile static int done = 0;
boolean  service = 0;
volatile int checkpoint = 0;        /* Used for the service only    */

int     nChild;             /* number of children */
CHILD   child[MAX_CHILD];

int main( int argc, char *argv[] )
{
   unsigned  stackSize;
   unsigned  tid;
   int       p, err;
   char * tok;
   struct stat filestat;
   char * buffer;
   //char * filename = "startstop.exe";
   char * filename = argv[0];

   char * path_env = getenv( "PATH" );

/* set some default values that can't be set directly in the struct */
   metaring.statmgr_sleeptime = 1000;
   metaring.statmgr_location = (MAX_CHILD + 1);
   memset(metaring.ConfigFile, 0, FILENAME_MAX); /* this one is pre-allocated in METARING */
   strcpy ( metaring.ConfigFile, DEF_CONFIG );
   strcpy ( metaring.Version, STARTSTOP_VERSION );
   if (strlen(VERSION_APPEND_STR) > 0) {              /* if "64 bit" str then */
     strcat ( metaring.Version, VERSION_APPEND_STR ); /* add indicator string */
   }
   metaring.LogLocation = malloc( FILENAME_MAX + 1 );
   memset(metaring.LogLocation, 0, FILENAME_MAX + 1);
   strncpy ( metaring.LogLocation, getenv( "EW_LOG" ), FILENAME_MAX );
   if (strlen(getenv("EW_LOG")) > FILENAME_MAX) {
       fprintf(stderr, "startstop: Fatal Error: EW_LOG env var greater than allowed len %d (FILENAME_MAX)\n",
			FILENAME_MAX);
       exit(-1);
   }
   metaring.ParamLocation = malloc( FILENAME_MAX + 1 );
   memset(metaring.ParamLocation, 0, FILENAME_MAX + 1);
   if (strlen(getenv("EW_PARAMS")) > FILENAME_MAX) {
       fprintf(stderr, "startstop: Fatal Error: EW_PARAMS env var greater than allowed len %d (FILENAME_MAX)\n",
			FILENAME_MAX);
       exit(-1);
   }
   strncpy ( metaring.ParamLocation, getenv( "EW_PARAMS" ), FILENAME_MAX );
   metaring.BinLocation = malloc( FILENAME_MAX + 1 );
   memset(metaring.BinLocation, 0, FILENAME_MAX + 1);
   strcpy( metaring.BinLocation, "???" );
   tok = strtok(path_env, ";");

   if ((buffer=strrchr(filename,'\\')) != NULL)
   {  /* arg0 contains dir separator; use left part as path name */
       p = (int)(buffer-filename);     /* get pos of separator */
       if (p > 0)
       {  /* position (size) value OK */
           if (p > FILENAME_MAX)       /* if too large then */
               p = FILENAME_MAX;       /* limit size */
           strncpy ( metaring.BinLocation, filename, p );
       }
   }
   else
   {  /* arg0 does not contain dir separator; try old method */
       while (tok != NULL) {
           buffer = (char *)calloc(FILENAME_MAX, sizeof(char));
           strcat(buffer, tok);
           strcat(buffer, "\\");
           strcat(buffer, filename);
           if(stat(buffer, &filestat) == 0) {
               if(filestat.st_mode & _S_IFREG) {
                   //We are done
                   strncpy ( metaring.BinLocation, tok, FILENAME_MAX );
                   break;
               }
           }
           strcat(buffer, ".exe");
           if(stat(buffer, &filestat) == 0) {
               if(filestat.st_mode & _S_IFREG) {
                   //We are done
                   strncpy ( metaring.BinLocation, tok, FILENAME_MAX );
                   break;
               }
           }
           tok = strtok(NULL, ";");
       }
   }

   //strncpy ( metaring.BinLocation, buffer, FILENAME_MAX );


/* Set name of configuration file
   ******************************/
   if ( argc == 2 )
   {
        if ((strlen(argv[1]) == 2) /* checking for /v or -v or /h or -h */
                && ((argv[1][0] == '/')
                || (argv[1][0] == '-'))) {
            if ((argv[1][1] == 'v') || (argv[1][1] == 'V')) {
                printf("%s %s%s\n",PROGRAM_NAME, STARTSTOP_VERSION, VERSION_APPEND_STR);
            } else if ((argv[1][1] == 'h') || (argv[1][1] == 'H')) {
                printf("%s %s%s\n",PROGRAM_NAME, STARTSTOP_VERSION, VERSION_APPEND_STR);
                printf("usage: %s <config file name>\n", PROGRAM_NAME);
                printf("       If no config file name is used, then the default config file is used,\n");
                printf("       in this case %s.\n", DEF_CONFIG);
            }
            exit (0);
        } else {
            if (strlen(argv[1]) > FILENAME_MAX) {
                fprintf(stderr, "startstop: Fatal Error: %s len is greater than allowed len %d (FILENAME_MAX)\n",
			argv[1], FILENAME_MAX);
                exit(-1);
            }
            strcpy ( metaring.ConfigFile, argv[1] );
        }

   }
   else
   {
        fprintf ( stderr, "startstop: using default config file <%s>\n",
                  metaring.ConfigFile );
   }

   /* lock this instance, so another startstop can't startup using the same config file */
   LockStartstop(metaring.ConfigFile, argv[0]);

/* Initialize name of log-file & open it
   *************************************/
   logit_init( metaring.ConfigFile, 0, 1024, 1 );

   /* log startstop version string (with 64-bit indicator) */
   logit("et", "Startstop Version:  %s%s\n", STARTSTOP_VERSION, VERSION_APPEND_STR);

   err = StartstopSetup ( &metaring, &checkpoint, service, child, &nChild );
   if (err == -1) {
        UnlockStartstop();
        return -1;
    }

/* Start the interactive thread
   ****************************/
   stackSize = 0;
   StartThread( Interactive, stackSize, &tid );

   err = FinalLoop (&metaring, &done, ewstat, &checkpoint, service, child, &nChild );
   if (err == -1) {
            UnlockStartstop();
            return -1;
    }
    UnlockStartstop();
    return 0;
}


#pragma warning(disable : 4100)   /* suppress unreferenced formal parameter warning */

/************************ Interactive ****************************
*           Thread to get commands from keyboard                *
*****************************************************************/

void Interactive( void *dummy )
{
  char line[100];
  char param[100];
  char fish[100];
  int  i;
  MSG_LOGO  logo;
  char      message[512];

  EncodeStatus( ewstat, &metaring, child, &nChild );                   /* One free status */
  fprintf( stderr, "\n%s", ewstat );

  do
  {
     fprintf( stderr, "\n   Press return to print Earthworm status, or\n" );
     fprintf( stderr, "   type 'restart nnn<cr>' where nnn is either a module name or Process ID, or\n");
     fprintf( stderr, "   type 'quit<cr>' to stop Earthworm.\n\n" );
     line[0] = 0;
     param[0] = 0;
     fgets(fish,99,stdin);
     sscanf(fish,"%[^ \t\n]%*[ \t\n]%[^ \t\n]",line,param);
     if ( strlen( line ) == 0 )
     {
        EncodeStatus( ewstat, &metaring, child, &nChild  );
        fprintf( stderr, "\n%s", ewstat );
     }

     {
      char *ptr=line;
      while (*ptr != '\0')
        *(ptr++) = (char)tolower(*ptr);
     }

     if (strncmp( line, "restart", 7)==0)
     {
       int paramAsInt;
       sscanf( param, "%d", &paramAsInt );
       for( i = 0; i < nChild; i++ )
       {
         if( strcmp(child[i].progName, param)==0 || child[i].procInfo.dwProcessId==(DWORD)paramAsInt)
         {
	   		/* sending it to the ring rather than starting directly so statmgr can keep track */
           sprintf(param,"%d%c", child[i].procInfo.dwProcessId,0);
       	   fprintf( stderr, "sending restart message to the ring for %s\n", param);
       	   SendRestartReq(&metaring, param);
           break;
         }
       }
	   if ( i >= nChild ) {
       	fprintf( stderr, "Could not find process '%s' to restart; ignoring\n", param );
       }
     }
     if ((strncmp( line, "stop", 4)==0) || (strncmp( line, "stopmodule", 10)==0))
     {
       int paramAsInt;
       sscanf( param, "%d", &paramAsInt );
       for( i = 0; i < nChild; i++ )
       {
         if( strcmp(child[i].progName, param)==0 || child[i].procInfo.dwProcessId==(DWORD)paramAsInt)
         {
	   		/* sending it to the ring rather than stopping directly so statmgr can keep track */
           sprintf(param,"%d%c", child[i].procInfo.dwProcessId,0);
       	   fprintf( stderr, "sending stop message to the ring for %s\n", param);
       	   SendStopReq(&metaring, param);
           break;
         }
       }
	   if ( i >= nChild ) {
       	fprintf( stderr, "Could not find process '%s' to stop; ignoring\n", param );
       }
     }

     if ((strncmp( line, "recon", 5)==0) || (strncmp( line, "reconfigure", 11)==0)) { /* reconfigure */
        /* Send a message requesting reconfigure */
        /* message is just MyModId
           ****************************/
           sprintf(message,"%d\n", metaring.MyModId );
        /* Set logo values of message
           **************************/
           logo.type   = metaring.TypeReconfig;
           logo.mod    = metaring.MyModId;
           logo.instid = metaring.InstId;

        /* Send status message to transport ring
           **************************************/
           if ( tport_putmsg( &(metaring.Region[0]), &logo, (long)strlen(message), message ) != PUT_OK ) {
                logit( "e" , "status: Error sending message to transport region.\n" );
                return;
           }
     }
    if ( strcmp(line, "PAU") == 0 || strcmp(line, "pau") == 0 ) break;

  } while ( strcmp( line, "quit" ) != 0 );

  done = 1;                 /* The main thread checks this flag */

  sleep_ew( 1000000 );      /* Sleep a long time; let main thread exit */
}






