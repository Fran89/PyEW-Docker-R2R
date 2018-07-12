
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: template.c 6843 2016-10-18 19:00:10Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2007/03/28 17:50:34  paulf
 *     fixed return of main
 *
 *     Revision 1.7  2007/02/26 14:57:30  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.6  2007/02/20 16:34:32  paulf
 *     fixed some windows bugs for time_t declarations being complained about
 *
 *     Revision 1.5  2007/02/20 13:29:20  paulf
 *     added lockfile testing to template module
 *
 *     Revision 1.4  2002/05/15 16:56:38  patton
 *     Made logit changes
 *
 *     Revision 1.3  2001/05/09 17:26:54  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or myPid.
 *
 *     Revision 1.2  2000/07/24 19:18:47  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 19:43:11  lucky
 *     Initial revision
 *
 *
 */

/*
 * template.c:  Sample code for a basic earthworm module which:
 *              1) reads a configuration file using kom.c routines 
 *                 (template_config).
 *              2) looks up shared memory keys, installation ids, 
 *                 module ids, message types from earthworm*.d tables 
 *                 using getutil.c functions (template_lookup).
 *              3) attaches to one public shared memory region for
 *                 input and output using transport.c functions.
 *              4) processes hard-wired message types from configuration-
 *                 file-given installations & module ids (This source
 *                 code expects to process TYPE_HINVARC & TYPE_H71SUM
 *                 messages).
 *              5) sends heartbeats and error messages back to the
 *                 shared memory region (template_status).
 *              6) writes to a log file using logit.c functions.
 */

/* changes: 
  Lombard: 11/19/98: V4.0 changes: 
     0) changed message types to Y2K-compliant ones
     1) changed argument of logit_init to the config file name.
     2) process ID in heartbeat message
     3) flush input transport ring
     4) add `restartMe' to .desc file
     5) multi-threaded logit: not applicable
*/

#ifdef _OS2
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <lockfile.h>

/* Functions in this source file 
 *******************************/
void  template_config  ( char * );
void  template_lookup  ( void );
void  template_status  ( unsigned char, short, char * );
void  template_hinvarc ( void );
void  template_h71sum  ( void );

static  SHM_INFO  Region;      /* shared memory region to use for i/o    */

#define   MAXLOGO   2
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;
pid_t     myPid;               /* for restarts by startstop               */

#define BUF_SIZE 60000          /* define maximum size for an event msg   */
static char Buffer[BUF_SIZE];   /* character string to hold event message */
        
/* Things to read or derive from configuration file
 **************************************************/
static char    RingName[MAX_RING_STR];        /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeHinvArc;
static unsigned char TypeH71Sum;

/* Error messages used by template 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
static char  Text[150];        /* string for log/error messages          */


int main( int argc, char **argv )
{
   time_t      timeNow;          /* current time                  */       
   time_t      timeLastBeat;     /* time last heartbeat was sent  */
   long      recsize;          /* size of retrieved message     */
   MSG_LOGO  reclogo;          /* logo of retrieved message     */
   int       res;
 
   char * lockfile;
   int lockfile_fd;

/* Check command line arguments 
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: template <configfile>\n" );
        exit( 0 );
   }
/* Initialize name of log-file & open it 
 ***************************************/
   logit_init( argv[1], 0, 256, 1 );
   
/* Read the configuration file(s)
 ********************************/
   template_config( argv[1] );
   logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );

/* Look up important info from earthworm*.d tables
 ************************************************/
   template_lookup();
 
/* Reinitialize logit to desired logging level 
 **********************************************/
   logit_init( argv[1], 0, 256, LogSwitch );


   lockfile = ew_lockfile_path(argv[1]); 
   if ( (lockfile_fd = ew_lockfile(lockfile) ) == -1) {
	fprintf(stderr, "one  instance of %s is already running, exiting\n", argv[0]);
	exit(-1);
   }
/*
   fprintf(stderr, "DEBUG: for %s, fd=%d for %s, LOCKED\n", argv[0], lockfile_fd, lockfile);
*/

/* Get process ID for heartbeat messages */
   myPid = getpid();
   if( myPid == -1 )
   {
     logit("e","template: Cannot get pid. Exiting.\n");
     exit (-1);
   }

/* Attach to Input/Output shared memory ring 
 *******************************************/
   tport_attach( &Region, RingKey );
   logit( "", "template: Attached to public memory region %s: %d\n", 
          RingName, RingKey );

/* Flush the transport ring */
   while( tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize, Buffer, 
			sizeof(Buffer)-1 ) != GET_NONE );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

/*----------------------- setup done; start main loop -------------------------*/

   while(1)
   {
     /* send template's heartbeat
      ***************************/
        if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval ) 
        {
            timeLastBeat = timeNow;
            template_status( TypeHeartBeat, 0, "" ); 
        }

     /* Process all new messages    
      **************************/
        do
        {
        /* see if a termination has been requested 
         *****************************************/
           if ( tport_getflag( &Region ) == TERMINATE ||
                tport_getflag( &Region ) == myPid )
           {
           /* detach from shared memory */
                tport_detach( &Region ); 
           /* write a termination msg to log file */
                logit( "et", "template: Termination requested; exiting!\n" );
                fflush( stdout );
	   /* should check the return of these if we really care */
/*
   		fprintf(stderr, "DEBUG: %s, fd=%d for %s\n", argv[0], lockfile_fd, lockfile);
*/
   		ew_unlockfile(lockfile_fd);
   		ew_unlink_lockfile(lockfile);
                exit( 0 );
           }

        /* Get msg & check the return code from transport
         ************************************************/
           res = tport_getmsg( &Region, GetLogo, nLogo,
                               &reclogo, &recsize, Buffer, sizeof(Buffer)-1 );

           if( res == GET_NONE )          /* no more new messages     */
           {
                break;
           }
           else if( res == GET_TOOBIG )   /* next message was too big */
           {                              /* complain and try again   */
                sprintf(Text, 
                        "Retrieved msg[%ld] (i%u m%u t%u) too big for Buffer[%ld]",
                        recsize, reclogo.instid, reclogo.mod, reclogo.type, 
                        (long)sizeof(Buffer)-1 );
                template_status( TypeError, ERR_TOOBIG, Text );
                continue;
           }
           else if( res == GET_MISS )     /* got a msg, but missed some */
           {
                sprintf( Text,
                        "Missed msg(s)  i%u m%u t%u  %s.",
                         reclogo.instid, reclogo.mod, reclogo.type, RingName );
                template_status( TypeError, ERR_MISSMSG, Text );
           }
           else if( res == GET_NOTRACK ) /* got a msg, but can't tell */
           {                             /* if any were missed        */
                sprintf( Text,
                         "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                          reclogo.instid, reclogo.mod, reclogo.type );
                template_status( TypeError, ERR_NOTRACK, Text );
           }

        /* Process the message 
         *********************/
           Buffer[recsize] = '\0';      /*null terminate the message*/
        /* logit( "", "%s", rec ); */   /*debug*/

           if( reclogo.type == TypeHinvArc ) 
           {
               template_hinvarc( );
           }
           else if( reclogo.type == TypeH71Sum ) 
           {
               template_h71sum( );
           }

           
        } while( res != GET_NONE );  /*end of message-processing-loop */
        
        sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */

   }  
/*-----------------------------end of main loop-------------------------------*/        
}

/******************************************************************************
 *  template_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
void template_config( char *configfile )
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
   ncommand = 5;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
        logit( "e",
                "template: Error opening command file <%s>; exiting!\n", 
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
                          "template: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInterval") ) {
                HeartBeatInterval = k_long();
                init[3] = 1;
            }


         /* Enter installation & module to get event messages from
          ********************************************************/
  /*4*/     else if( k_its("GetEventsFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit( "e", 
                            "template: Too many <GetEventsFrom> commands in <%s>", 
                             configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e", 
                               "template: Invalid installation name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e", 
                               "template: Invalid module name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
                       logit( "e", 
                               "template: Invalid type name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].type = GetLogo[nLogo].type;
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetType( str, &GetLogo[nLogo+1].type ) != 0 ) {
                       logit( "e", 
                               "template: Invalid type name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].type = GetLogo[nLogo].type;
                }

                nLogo  += 2;
                init[4] = 1;
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type ); */  /*DEBUG*/
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo+1, (int) GetLogo[nLogo+1].instid,
                               (int) GetLogo[nLogo+1].mod,
                               (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
            }

         /* Unknown command
          *****************/ 
            else {
                logit( "e", "template: <%s> Unknown command in <%s>.\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit( "e", 
                       "template: Bad <%s> command in <%s>; exiting!\n",
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
       logit( "e", "template: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "           );
       if ( !init[1] )  logit( "e", "<MyModuleId> "        );
       if ( !init[2] )  logit( "e", "<RingName> "          );
       if ( !init[3] )  logit( "e", "<HeartBeatInterval> " );
       if ( !init[4] )  logit( "e", "<GetEventsFrom> "     );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  template_lookup( )   Look up important info from earthworm*.d tables       *
 ******************************************************************************/
void template_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        fprintf( stderr,
                "template:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr, 
              "template: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHinvArc ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_H71SUM2K>; exiting!\n" );
      exit( -1 );
   }
   return;
} 

/******************************************************************************
 * template_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void template_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t        t;
 
/* Build the message
 *******************/ 
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) myPid);
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "template: %s\n", note );
   }

   size = (long)strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","template:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","template:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

/*******************************************************************************
 *  template_hinvarc( )  process a Hypoinverse archive message                 *
 *                       Fill this function with what you need to do           *
 *******************************************************************************/
void template_hinvarc( void )
{
}

/*******************************************************************************
 *  template_h71sum( )  process a Hypo71 summary message                       *
 *                      Fill this function with what you need to do            *
 *******************************************************************************/
void template_h71sum( void )
{
}

/*******************************************************************************
 *  template_msgtype( )  process a given msgtype message                       *
 *                       Fill this function with what you need to do           *
 *******************************************************************************/
void template_msgtype( void )
{
}
