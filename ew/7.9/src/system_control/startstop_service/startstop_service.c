
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: startstop_service.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2010/03/26 15:14:34  stefan
 *     just changed version number
 *
 *     Revision 1.7  2008/12/05 21:28:23  stefan
 *     version
 *
 *     Revision 1.6  2007/12/16 19:31:34  paulf
 *     upped version num of EW
 *
 *     Revision 1.5  2007/02/27 05:00:49  stefan
 *     sends stop and restart messages to accommodate new statmgr
 *
 *     Revision 1.4  2007/02/22 21:02:12  stefan
 *     lock changes
 *
 *     Revision 1.3  2006/06/06 21:14:39  stefan
 *     hydra console incorporation
 *
 *     Revision 1.4  2006/05/31 00:43:30  davidk
 *     Removed a free() of child, which is an array, not a pointer.
 *     The free was causing startstop_hydra to crash when the service was stopped.
 *
 *     Revision 1.3  2006/05/04 17:08:01  davidk
 *     removed unneccessary windows.h include, as it was causing winsock issues.
 *
 *     Revision 1.2  2005/12/28 21:17:04  mark
 *     Added Named Pipes for launching console windows
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:38  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *     $Log$
 *     Revision 1.8  2010/03/26 15:14:34  stefan
 *     just changed version number
 *
 *     Revision 1.7  2008/12/05 21:28:23  stefan
 *     version
 *
 *     Revision 1.6  2007/12/16 19:31:34  paulf
 *     upped version num of EW
 *
 *     Revision 1.5  2007/02/27 05:00:49  stefan
 *     sends stop and restart messages to accommodate new statmgr
 *
 *     Revision 1.4  2007/02/22 21:02:12  stefan
 *     lock changes
 *
 *     Revision 1.3  2006/06/06 21:14:39  stefan
 *     hydra console incorporation
 *
 *     Revision 1.1  2005/06/29 17:17:30  friberg
 *     new Windows Service version of startstop contributed from the Hydra group
 *
 *     Revision 1.6  2005/04/14 22:57:11  labcvs
 *     Alex fixed a bug in startstop where startstop was not looking for statmgr2
 *     to start before all modules, causing a situation where statmgr2 could miss
 *     first heartbeats from programs that died quickly after startup. AB 4-14-05
 *     Credit to John for figuring it out.
 *
 *     Revision 1.5  2004/08/12 21:33:20  mark
 *     Attempt to hide NoNew-console windows
 *
 *     Revision 1.4  2004/08/09 17:35:09  labcvs
 *     Fixed compile error in install_service
 *
 *     Revision 1.3  2004/07/15 22:59:21  mark
 *     Made sure new consoles are visible
 *
 *     Revision 1.2  2004/07/15 21:41:46  mark
 *     Fixes, tweaks, etc.  Still a ways to go...
 *
 *     Revision 1.1  2004/07/13 16:58:55  mark
 *     Initial checkin; changed ew's startstop to run as a Windows service
 *
 *     Revision 1.16  2002/07/16 19:01:33  davidk
 *     Fixed a bug in a sprintf statement that gets issued when startstop
 *     cannot restart a zombie process.  The bug involved trying to printf
 *     an integer using "%s", and caused startstop to crash on execution.
 *     Startstop now issues the message correctly and restarts the zombie
 *     process.
 *
 *     Revision 1.15  2002/04/22 17:17:34  lucky
 *     Fixed index (compile time) problem
 *
 *     Revision 1.14  2002/04/19 18:51:28  lucky
 *     Moved CloseHandle () calls out of TerminateChild. This was causing a slow
 *     handle leak resulting in system crash. Basically, when a child would shut
 *     itself down, i.e., it was not terminated by startstop, its handles would not be
 *     closed.
 *
 *     Revision 1.13  2002/03/20 16:56:03  patton
 *     made logit chagnes.
 *     JMP 03/20/2002
 *
 *     Revision 1.12  2001/07/16 17:01:02  patton
 *     Changed 'sleeptime' to 'statmgrDelay'.
 *
 *     Revision 1.11  2001/07/02 21:57:36  patton
 *     Modified Previous changes as per Lucky's suggestions.
 *
 *     Revision 1.9  2001/06/08 20:30:14  dietz
 *     Fixed original bug in logit call that caused startstop to crash when
 *     it couldn't start one of its children.  Removed SetErrorMode call and
 *     Alex's search for executable (previous attempts at fixin bug), added
 *     logging, made to shut down properly when CreateProcess fails.
 *
 *     Revision 1.8  2001/06/07 19:27:52  alex
 *     *** empty log message ***
 *
 *     Revision 1.5  2001/06/05 01:15:11  Alex
 *     Alex: added check for binary being in the earthworm bin path.
 *
 *     Revision 1.4  2001/05/08 20:48:53  dietz
 *     Changed RestartChild() so that it gives the child process a chance to
 *     shut down gracefully before it terminates it.
 *
 *     Revision 1.3  2000/08/28 22:45:13  kohler
 *     Added new "minimized console" display option.  WMK
 *
 *     Revision 1.2  2000/07/24 21:14:07  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 19:37:19  lucky
 *     Initial revision
 *
 *
 */

       /**********************************************************
        *                       startstop.c                      *
        *                                                        *
        *     Program to start and stop the Earthworm system     *
        *                   Windows NT version                   *
        **********************************************************/
/* Changes:
 Lombard: 11/19/98: V4.0 changes: no Y2K date problems
     1) changed argument of logit_init to the config file name
     2) process ID in heartbeat message: not applicable
     3) flush input transport ring: not applicable
     4) add `restartMe' to .desc file: not applicable
     5) multi-threaded logit

  Alex 6/7/1:
  Fix to issue issue warning and continue running if an executable
  is not found. If the executable is not in the traditional place, a warning
  will be issued, and the %PATH% be searched for the executable. TYP for
  finding SetErrorMode().
 */

#include <startstop_winlib.h>
#include <startstop_version.h>

#define PROGRAM_NAME "startstop_service"

METARING metaring;
void Interactive( void * );        /* Interactive thread              */
void startstop_service_main(int argc, char *argv[]);
void service_handler(unsigned int dwControl);
static char ewstat[MAX_STATUS_LEN];
volatile static int done = 0;
volatile int checkpoint = 0;        /* Used for the service only    */




/* Possible Error codes
 **********************/
char ErrText[512];

#define STARTSTOP_SERVICE_NAME  "ew_startstop"
#define STARTSTOP_DISPLAY_NAME  "Earthworm start-stop"

int     nChild;             /* number of children */
CHILD   child[MAX_CHILD];
boolean service;


int main( int argc, char *argv[] )
{
   char * tok;
   char * buffer;
   struct stat filestat;
   int p;
   char * path_env;
   char * filename = argv[0];

   service = 1;
         /* set default values (do it here because used in this fn): */
   strcpy ( metaring.ConfigFile, DEF_CONFIG );
   strcpy ( metaring.Version, STARTSTOP_VERSION );
   if (strlen(VERSION_APPEND_STR) > 0) {              /* if "64 bit" str then */
     strcat ( metaring.Version, VERSION_APPEND_STR ); /* add indicator string */
   }

   if ( argc == 2 )
   {
        if ((strlen(argv[1]) == 2) /* checking for /v or -v or /h or -h */
                && ((argv[1][0] == '/')
                || (argv[1][0] == '-'))) {
            if ((argv[1][1] == 'v') || (argv[1][1] == 'V')) {
                printf("%s %s%s\n",PROGRAM_NAME, STARTSTOP_VERSION, VERSION_APPEND_STR);
            } else if ((argv[1][1] == 'h') || (argv[1][1] == 'H')) {
                printf("%s %s%s\n",PROGRAM_NAME, STARTSTOP_VERSION, VERSION_APPEND_STR);
                printf("usage: %s -install\n", PROGRAM_NAME);
                printf("       or\n");
                printf("usage: %s -uninstall\n", PROGRAM_NAME);
                printf("       Automatically using this config file: %s.\n", DEF_CONFIG);
                printf("       Not to be run standalone, only once installed as a service.\n");
            }
            exit (0);
        } else {
            if (_stricmp(argv[1], "-install") == 0)
            {
                install_service(STARTSTOP_SERVICE_NAME, STARTSTOP_DISPLAY_NAME, NULL);
                return 0;
            }
            if (_stricmp(argv[1], "-uninstall") == 0)
            {
                uninstall_service(STARTSTOP_SERVICE_NAME);
                return 0;
            }
            else
            {
                strcpy ( metaring.ConfigFile, argv[1] );

            /* Initialize name of log-file & open it
               *************************************/
               logit_init( metaring.ConfigFile, 0, 1024, 1 );
            }
        }
   }
   else
   {
        fprintf ( stderr, "startstop: using default config file <%s>\n",
                  metaring.ConfigFile );
   }

    /* enter values for metaring ParamLocation and BinLocation so */
    /*  they will be displayed in the 'status' information: */
   metaring.ParamLocation = malloc( FILENAME_MAX + 1 );
   memset(metaring.ParamLocation, 0, FILENAME_MAX + 1);
   if (strlen(getenv("EW_PARAMS")) <= FILENAME_MAX)
       strncpy ( metaring.ParamLocation, getenv( "EW_PARAMS" ), FILENAME_MAX );
   else
       strcpy( metaring.ParamLocation, "???" );

   metaring.BinLocation = malloc( FILENAME_MAX + 1 );
   memset(metaring.BinLocation, 0, FILENAME_MAX + 1);
   strcpy( metaring.BinLocation, "???" );
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
       path_env = getenv( "PATH" );
       tok = strtok(path_env, ";");
       while (tok != NULL)
       {
           buffer = (char *)calloc(FILENAME_MAX, sizeof(char));
           strcat(buffer, tok);
           strcat(buffer, "\\");
           strcat(buffer, filename);
           if(stat(buffer, &filestat) == 0)
           {
               if(filestat.st_mode & _S_IFREG)
               {   /* We are done */
                   strncpy ( metaring.BinLocation, tok, FILENAME_MAX );
                   break;
               }
           }
           strcat(buffer, ".exe");
           if(stat(buffer, &filestat) == 0)
           {
               if(filestat.st_mode & _S_IFREG)
               {   /* We are done */
                   strncpy ( metaring.BinLocation, tok, FILENAME_MAX );
                   break;
               }
           }
           tok = strtok(NULL, ";");
       }
   }

   lockfile = ew_lockfile_path(metaring.ConfigFile);
   if ( (lockfile_fd = ew_lockfile(lockfile) ) == -1)
   {
       fprintf(stderr, "%s is already running, only one %s can run at the same time, exiting\n",
                                                          argv[0], argv[0]);
       exit(-1);
   }

   start_service(STARTSTOP_SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)startstop_service_main);
   logit("e","\n------------------------------------------" );

   return 0;
}

#pragma warning(disable : 4100)   /* suppress unreferenced formal parameter warning */

void startstop_service_main( int argc, char *argv[] )
{
   int       err;
/* set some default values that can't be set directly in the struct */
   metaring.statmgr_sleeptime = 1000;
   metaring.statmgr_location = (MAX_CHILD + 1);

   init_service(STARTSTOP_SERVICE_NAME, 0, (LPHANDLER_FUNCTION)service_handler);
   checkpoint = 0;
   set_service_status(SERVICE_START_PENDING, 0, checkpoint, 10000);

   checkpoint = checkpoint + 1;
   err = StartstopSetup ( &metaring, &checkpoint, service, child, &nChild );
   if (err == -1) {
        #ifdef STARTSTOP_SERVICE_NAME
            return;
        #else
            return -1;
        #endif
    }

/* Start the interactive thread
   ****************************/
/* Don't start this thread as a service; if there's no console to talk to, this thread will
 * eat up 100% of the CPU time.
   stackSize = 0;
   StartThread( Interactive, stackSize, &tid );
*/
   set_service_status(SERVICE_RUNNING, 0, 0, 0);

   /* log startstop version string (with 64-bit indicator) */
   logit("et", "Startstop (service) Version:  %s%s\n", STARTSTOP_VERSION, VERSION_APPEND_STR);

   err = FinalLoop (&metaring, &done, ewstat, &checkpoint, service, child, &nChild );

         /* if lock file was opened OK then unlock and delete it: */
   if (lockfile_fd > 0) {
       ew_unlockfile(lockfile_fd);
       ew_unlink_lockfile(lockfile);
   }
}

void service_handler(unsigned int dwControl)
{
    switch(dwControl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        logit("e", "Stop service message received.\n");

        checkpoint = 0;
        set_service_status(SERVICE_STOP_PENDING, 0, 0, 3000);
        checkpoint = checkpoint + 1;

       logit("et", "'done' set via service_handler.\n");
        done = 1;
        break;

    case SERVICE_CONTROL_INTERROGATE:
        logit("e", "Interrogate service message received.\n");
        set_service_status(0, 0, 0, 0);

    default:;
    }
}






