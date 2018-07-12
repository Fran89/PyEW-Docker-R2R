
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: diskmgr.c 4673 2012-01-27 09:16:02Z quintiliani $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2007/02/26 16:32:20  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.4  2006/07/11 23:34:15  kohler
 *     Added new optional configuration parameter, DefDir.  At startup, diskmgr
 *     changes the default directory to DefDir.  Then, it determines the free
 *     space in DefDir.  This allows the user to specify the partition that
 *     diskmgr works with.  WMK 7/11/2006
 *
 *     Revision 1.3  2002/05/15 21:53:55  patton
 *     Made logit changes.
 *
 *     Revision 1.2  2001/05/09 17:54:57  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or myPid.
 *
 *     Revision 1.1  2000/02/14 17:00:08  lucky
 *     Initial revision
 *
 *
 */

 /**********************************************************************
  *                                                                    *
  *                         DiskMgr Program                            *
  *                                                                    *
  *  Report to the status manager if the disk is nearly full.          *
  *                                                                    *
  *  Note:  Changed 4/7/95 to run on Solaris 2.4 (from SunOS 4.1).     *
  *         The only changes required were to name of the system call  *
  *         for getting file-system statistics, to structure-type of   *
  *         its second argument, and to its required include file      *
  *                                                                    *
  **********************************************************************/

/* changes: 
  Lombard: 11/19/98: V4.0 changes: 
     0) no Y2k dates 
     1) changed argument of logit_init to the config file name.
     2) process ID in heartbeat message
     3) flush input transport ring: not applicable
     4) add `restartMe' to .desc file
     5) multi-threaded logit: not applicable
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>

#define ERR_DISKFULL  0

/* Function prototypes
 *********************/
void diskmgr_config( char * );
int  diskmgr_status( unsigned char, short, char * );

/* Things to read (or derive) from configuration file
   **************************************************/
static long          RingKey;     /* key of transport ring to write to */
static unsigned char MyModId;     /* diskmgr's module id               */
static int           LogSwitch;   /* If 0, no logging should be done to disk */
unsigned long        Min_kbytes;  /* complain when free disk space is  */
                                  /* below this number of kbytes       */
static int checkInterval = 15;    /* Interval in seconds for checking disk availability */
pid_t                myPid;	  /* for restarts by startstop         */
char                 DefDir[80];  /* Default directory */

/* Things to look up in the earthworm.h tables with getutil.c functions
   ********************************************************************/
static unsigned char InstId;        /* local installation id  */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

SHM_INFO Region1;

int main( int argc, char **argv )
{
   static int heartBeatInterval = 5;
   unsigned   DiskAvail;
   char       note[70];
   int        i;
   time_t       timeNow;          /* current time    */
   time_t       timeLastCheck;    /* time last check */

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: diskmgr <configfile>\n" );
        exit( 0 );
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read configuration file
   ***********************/
   diskmgr_config( argv[1] );
   logit( "" , "diskmgr: Read command file <%s>\n", argv[1] );


/* Look up local installation id
   *****************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "diskmgr: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up message types from earthworm.h tables
   *********************************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "diskmgr: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "diskmgr: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }


/* Reinitialize logit to the desired logging level
   ***********************************************/
   logit_init( argv[1], (short) MyModId, 256, LogSwitch );
   
/* Get process ID for heartbeat messages */
   myPid = getpid();
   if( myPid == -1 )
   {
     logit("e","diskmgr: Cannot get pid. Exiting.\n");
     exit (-1);
   }

/* Change default directory.  The GetDiskAvail() function 
   gets the space available on the partition that contains
   the default directory.
   *******************************************************/
   if ( chdir_ew( DefDir ) == -1 )
   {
      logit( "e", "diskmgr: Error changing default directory to %s\n", DefDir );
      exit( -1 );
   }
   logit( "et", "diskmgr: Default directory changed to %s\n", DefDir );

/* Get available disk space at program startup
   *******************************************/
   if ( GetDiskAvail( &DiskAvail ) == -1 )
   {
      logit( "et",
             "diskmgr:  Error getting file system statistics; exiting!\n" );
      exit( 1 );
   }
   logit( "et", "diskmgr: %u kbytes disk space available on startup.\n",
            DiskAvail );

/* Initialize timeLastCheck
   *************************************************************/
   timeLastCheck = time(&timeNow);

/* Attach to shared memory ring
   ****************************/
   tport_attach( &Region1, RingKey );


/* Loop until kill flag is set
   ***************************/
   while ( 1 )
   {

/* See if the disk is nearly full
   ******************************/
       if  ( time(&timeNow) - timeLastCheck  >=  checkInterval )
       {
	   timeLastCheck = timeNow;

	   if ( GetDiskAvail( &DiskAvail ) == -1 )
	       logit( "et", "diskmgr:  Error getting file system statistics.\n" );

	   if ( DiskAvail < Min_kbytes )
	   {
	       sprintf( note, "Disk of '%s' nearly full; %u kbytes available.",
		       DefDir, DiskAvail );
	       if ( diskmgr_status( TypeError, ERR_DISKFULL, note ) != PUT_OK )
		   logit( "t", "diskmgr:  Error sending error message to ring.\n");
	   }
       }

   /* Send heartbeat every heartBeatInterval seconds
      **********************************************/
      if ( diskmgr_status( TypeHeartBeat, 0, "" ) != PUT_OK )
      {
           logit( "t", "diskmgr:  Error sending heartbeat to ring.\n");
      }

   /* Check kill flag
      ***************/
      for ( i = 0; i < heartBeatInterval; i++ )
      {
         sleep_ew( 1000 );
         if ( tport_getflag( &Region1 ) == TERMINATE ||
              tport_getflag( &Region1 ) == myPid )
         {
                tport_detach( &Region1 );
                logit( "t", "diskmgr: Termination requested; exiting!\n" );
                return 0;
         }
      }
   }
}


         /*************************************************
          *                 diskmgr_status                *
          *  Builds heartbeat or error msg and puts it in *
          *  shared memory.                               *
          *************************************************/
int diskmgr_status( unsigned char type,
                    short         ierr,
                    char         *note )
{
        MSG_LOGO    logo;
        char        msg[1024];
        int         res;
        long        size;
        time_t      t;

        logo.instid = InstId;
        logo.mod    = MyModId;
        logo.type   = type;

        time( &t );

        if( type == TypeHeartBeat ) {
                sprintf ( msg, "%ld %d\n", (long) t, (int) myPid);
        }
        else if( type == TypeError ) {
                sprintf ( msg, "%ld %d %s\n", (long) t, ierr, note);
        }

        size = strlen( msg );  /* don't include null byte in message */
        res  = tport_putmsg( &Region1, &logo, size, msg );

        return( res );
}


/***********************************************************************
 * diskmgr_config()  processes command file using kom.c functions      *
 *                      exits if any errors are encountered            *
 ***********************************************************************/
void diskmgr_config(char *configfile)
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 4;
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Set default directory
   *********************/
   strcpy( DefDir, "." );

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "diskmgr: Error opening command file <%s>; exiting!\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  logit( "e",
                          "diskmgr: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
         /* Read module id for this program
          *********************************/
   /*0*/    if( k_its( "MyModuleId" ) ) {
                str = k_str();
                if (str) {
                   if ( GetModId( str, &MyModId ) < 0 ) {
                      logit( "e",
                              "diskmgr: Invalid MyModuleId <%s> in <%s>",
                               str, configfile );
                      logit( "e", "; exiting!\n" );
                      exit( -1 );
                   }
                }
                init[0] = 1;
            }

         /* Name of transport ring to write to
          ************************************/
   /*1*/    else if( k_its( "RingName" ) ) {
                str = k_str();
                if (str) {
                   if ( ( RingKey = GetKey(str) ) == -1 ) {
                      logit( "e",
                              "diskmgr: Invalid RingName <%s> in <%s>",
                               str, configfile );
                      logit( "e", "; exiting!\n" );
                      exit( -1 );
                   }
                }
                init[1] = 1;
            }
         /* Complain when free disk space drops lower than
          ************************************************/
   /*2*/    else if( k_its( "Min_kbytes" ) ) {
                Min_kbytes = (unsigned long) k_long();
                init[2] = 1;
            }
         /* Set Logfile switch
          ********************/
   /*3*/    else if( k_its( "LogFile" ) ) {
                LogSwitch = k_int();
                init[3] = 1;
            }
   /*opt*/  else if( k_its("CheckInterval") ) {
                checkInterval = k_long();
            }


/* Set default directory (optional)
   ********************************/
            else if( k_its( "DefDir" ) )
            {
                str = k_str();
                strcpy( DefDir, str );
            }

            else {
                logit( "e", "diskmgr: <%s> unknown command in <%s>.\n",
                        com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e", "diskmgr: Bad <%s> command in <%s>; \n",
                        com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "diskmgr: ERROR, no " );
       if ( !init[0] )  logit( "e", "<MyModuleId> " );
       if ( !init[1] )  logit( "e", "<RingName> "   );
       if ( !init[2] )  logit( "e", "<Min_kbytes> " );
       if ( !init[3] )  logit( "e", "<LogFile> "    );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}
