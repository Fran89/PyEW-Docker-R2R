
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: menlo_report.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2007/02/26 14:47:38  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.7  2001/09/26 17:11:11  patton
 *     Made changes due to slightly reworked logit.
 *     JMP 9/26/200
 *     1.
 *
 *     Revision 1.6  2001/08/28 05:19:59  patton
 *     *** empty log message ***
 *
 *     Revision 1.5  2001/08/28 04:49:57  patton
 *     Made Logit changes due to new logit code.
 *     JMP 8/27/2001.
 *
 *     Revision 1.4  2001/05/07 23:53:05  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.3  2000/07/24 18:46:58  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/07/24 18:39:05  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 18:56:41  lucky
 *     Initial revision
 *
 *
 */

/*
 * menlo_report.c : Grabs final event messages and writes files to
 *                  a remote machine.
 */

/*
  All commands in this program will be run on the remote machine
  as "userid".  Make sure that the following files are set up
  properly on the remote machine:
     /etc/hosts         must contain address and localhostname
     /etc/hosts.equiv   must contain local_hostname
     .rhosts            in userid's home directory must contain a line:
                        local_hostname local_username
                        describing who will invoke this program.
  Also make sure that entries for the remote host are in the
  local machine's \tcpip\etc\hosts file.


  Thu Nov 12 19:01:29 MST 1998 lucky

  	- Name of the config file passed to logit_init().
 	- Incoming transport ring flushed at startup.
 	- Process ID sent on with the heartbeat msgs for restart
 	  purposes.
    - Y2K compliance:
       o changed message type names to their y2k equivalents
       o modified menlo_report_hinvarc() to y2k standard
       o modified menlo_report_h71sum() to y2k standard


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>

/* Functions in this source file
 *******************************/
void  menlo_report_hinvarc ( char * );
void  menlo_report_h71sum  ( char * );
void  menlo_report_config  ( char * );
void  menlo_report_lookup  ( void );
void  menlo_report_status  ( unsigned char, short, char * );

static  SHM_INFO  Region;      /* shared memory region to use for i/o    */

#define   MAXLOGO   2
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;

#define BUF_SIZE MAX_BYTES_PER_EQ   /* define maximum size for event msg  */

/* Things to read from configuration file
 ****************************************/
static char    RingName[MAX_RING_STR];        /* name of transport ring for i/o     */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id       */
static int     LogSwitch;           /* 0 if no logfile should be written  */
static long    HeartBeatInterval;   /* seconds between heartbeats         */
static char    RemoteHost[30];      /* name of computer to copy files to  */
static char    RemoteUser[15];      /* username to use on remote machine  */
static char    RemotePasswd[15];    /* password of username on RemoteHost */
static char    TmpFile[20];         /* temporary remote file name         */
static char    RemoteDir[50];       /* remote directory to place files in */
static char    LocalDir[50];        /* local directory to write tmp files */
static char    ArcSuffix[6];        /* suffix to use for archive files    */
static char    SumSuffix[6];        /* suffix to use for summary files    */
static int     SendArc   = 1;       /* =0 if ArcSuffix="none" or "NONE"   */
static int     SendSum   = 1;       /* =0 if SumSuffix="none" or "NONE"   */
static int     KeepLocal = 0;       /* =1 if KeepLocalCopy command is in  */
                                    /*    configuration file              */
static int     LongFileName = 0;    /* =1 if EnableLongFileName command   */
                                    /*    is in configuration file        */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeH71Sum2K;

/* Error messages used by menlo_report
 *************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_COPYFILE      3   /* error copying file to remote machine   */
#define  ERR_LOCALFILE     4   /* error creating/writing local temp file */
#define  ERR_RMFILE        5   /* error removing temporary file          */
static char  Text[150];        /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/


int main( int argc, char **argv )
{
   static char  eqmsg[BUF_SIZE];  /* array to hold event message    */
   time_t       timeNow;          /* current time                   */
   time_t       timeLastBeat;     /* time last heartbeat was sent   */
   long         recsize;          /* size of retrieved message      */
   MSG_LOGO     reclogo;          /* logo of retrieved message      */
   int          res;


/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        logit( "e" , "Usage: menlo_report <configfile>\n" );
        exit( 0 );
   }

/* Read the configuration file(s)
 ********************************/
   menlo_report_config( argv[1] );

/* Set logit to LogSwitch read from 
 * configfile.
 ***********************************/
   logit_init( argv[1], 0, 256, LogSwitch );
   logit( "" , "menlo_report: Read command file <%s>\n", argv[1] );
   
/* Look up important info from earthworm.h tables
 ************************************************/
   menlo_report_lookup();

/* Change working directory to that for writing tmp files
 ********************************************************/
   if ( chdir_ew( LocalDir ) == -1 )
   {
      fprintf( stderr, "%s: cannot change to directory <%s>\n",
               argv[0], LocalDir );
      fprintf( stderr, "%s: please correct <LocalDir> command in <%s>;",
               argv[0], argv[1] );
      fprintf( stderr, " exiting!\n" );
      exit( -1 );
   }

/* Get our process ID
 **********************/
   if ((MyPid = getpid ()) == -1)
   {
      logit ("e", "menlo_report: Call to getpid failed. Exiting.\n");
      return (EW_FAILURE);
   }

/* Attach to Input/Output shared memory ring
 *******************************************/
   tport_attach( &Region, RingKey );
   logit( "", "menlo_report: Attached to public memory region %s: %d\n",
          RingName, RingKey );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

/* Flush the incomming transport ring
 *************************************/
   while(tport_getmsg (&Region, GetLogo, nLogo,
		       &reclogo, &recsize, eqmsg, sizeof(eqmsg)-1) != GET_NONE);


/*----------------- setup done; start main loop ----------------------*/

   while( tport_getflag(&Region) != TERMINATE  && 
          tport_getflag(&Region) != MyPid         )
   {
     /* send menlo_report's heartbeat
      *******************************/
        if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval )
        {
            timeLastBeat = timeNow;
            menlo_report_status( TypeHeartBeat, 0, "" );
        }

     /* Process all new hypoinverse archive msgs and hypo71 summary msgs
      ******************************************************************/
        do
        {
        /* Get the next message from shared memory
         *****************************************/
           res = tport_getmsg( &Region, GetLogo, nLogo,
                               &reclogo, &recsize, eqmsg, sizeof(eqmsg)-1 );

        /* Check return code; report errors if necessary
         ***********************************************/
           if( res != GET_OK )
           {
              if( res == GET_NONE )
              {
                 break;
              }
              else if( res == GET_TOOBIG )
              {
                 sprintf( Text,
                         "Retrieved msg[%ld] (i%u m%u t%u) too big for eqmsg[%d]",
                          recsize, reclogo.instid, reclogo.mod, reclogo.type,
                          (int)sizeof(eqmsg)-1 );
                 menlo_report_status( TypeError, ERR_TOOBIG, Text );
                 continue;
              }
              else if( res == GET_MISS )
              {
                 sprintf( Text,
                         "Missed msg(s)  i%u m%u t%u  %s.",
                          reclogo.instid, reclogo.mod, reclogo.type, RingName );
                 menlo_report_status( TypeError, ERR_MISSMSG, Text );
              }
              else if( res == GET_NOTRACK )
              {
                 sprintf( Text,
                         "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                          reclogo.instid, reclogo.mod, reclogo.type );
                 menlo_report_status( TypeError, ERR_NOTRACK, Text );
              }
           }

        /* Process new message (res==GET_OK,GET_MISS,GET_NOTRACK)
         ********************************************************/
           eqmsg[recsize] = '\0';   /*null terminate the message*/
           /*printf( "%s", eqmsg );*/   /*debug*/

           if( reclogo.type == TypeHyp2000Arc )
           {
              if( !SendArc ) continue;
              menlo_report_hinvarc( eqmsg );
           }
           else if( reclogo.type == TypeH71Sum2K )
           {
              if( !SendSum ) continue;
              menlo_report_h71sum( eqmsg );
           }

        } while( res != GET_NONE );  /*end of message-processing-loop */

        sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */
   }
/*-----------------------------end of main loop-------------------------------*/

/* Termination has been requested
 ********************************/
/* detach from shared memory */
   tport_detach( &Region );
/* write a termination msg to log file */
   logit( "t", "menlo_report: Termination requested; exiting!\n" );
   exit( 0 );
}

/******************************************************************************
 *  menlo_report_hinvarc( )  process a Hypoinverse archive message            *
 ******************************************************************************/

#define B_DATE_HINVARC	0	/* yyyymmddhhmm string */
#define L_DATE_HINVARC	12
#define	B_EID_HINVARC	144	/* last 2 digits of the event id */
#define	L_EID_HINVARC	2
#define B_EID_HINVARC_L 136     /* all digits of the event id */
#define L_EID_HINVARC_L 10
#define B_VID_HINVARC_L 162     /* version of the event id */
#define L_VID_HINVARC_L 1


void menlo_report_hinvarc( char *arcmsg )
{
   FILE *fparc;
   char  fname[35];
   long  length;
   int   i, res;

/* Create name for local file using the event origin time and
     - last 2 digits of the event id and ArcSuffix/SumSuffix
       this is the default
       (i.e.  201011040930_57.arc)
     - all digits of the event id, plus id-version,  and ArcSuffix
       (i.e.  201011040932_0000097658_2.arc)
 *********************************************/
   i = 0;
   strncpy( fname, (arcmsg + B_DATE_HINVARC), L_DATE_HINVARC );
   i += L_DATE_HINVARC;
   fname[i] = '_';
   i += 1;

   if(LongFileName) {
       strncpy( (fname + i), (arcmsg + B_EID_HINVARC_L),  L_EID_HINVARC_L );
       i += L_EID_HINVARC_L;
       fname[i] = '_';
       i += 1;
       strncpy( (fname + i), (arcmsg + B_VID_HINVARC_L),  L_VID_HINVARC_L );
       i += L_VID_HINVARC_L;
   } else {
       strncpy( (fname + i), (arcmsg + B_EID_HINVARC),  L_EID_HINVARC);
       i += L_EID_HINVARC;
   }

   fname[i] = '.';
   i += 1;
   strcpy ( (fname + i), ArcSuffix );
   for ( i=0; i<(int) strlen(fname); i++ )
   {
       if( fname[i] == ' ' )  fname[i] = '0';
   }

/* Write local file
 ******************/
   if( (fparc = fopen( fname, "w" )) == (FILE *) NULL )
   {
       sprintf( Text, "menlo_report: error opening file <%s>\n",
                fname );
       menlo_report_status( TypeError, ERR_LOCALFILE, Text );
       return;
   }
   length = (long)strlen( arcmsg );
   if ( fwrite( arcmsg, (size_t)1, (size_t)length, fparc ) == 0 )
   {
       sprintf( Text, "menlo_report: error writing to file <%s>\n",
                fname );
       menlo_report_status( TypeError, ERR_LOCALFILE, Text );
       fclose( fparc );
       return;
   }
   fclose( fparc );

/* Copy file to remote machine
 *****************************/
   res = copyfile( fname, TmpFile, RemoteHost, RemoteDir,
                   RemoteUser, RemotePasswd, Text );
   if (res)
   {
       logit( "et", "menlo_report: error copying <%s> to <%s:%s>\n",
               fname, RemoteHost, RemoteDir );
       menlo_report_status( TypeError, ERR_COPYFILE, Text );
       return;
   }
   logit( "et", "Sent archive file: %s\n", fname );

/* Delete local file
 *******************/
   if( KeepLocal )  return;

   if( remove( fname ) == -1 )
   {
       sprintf( Text, "menlo_report: error removing tmpfile <%s>\n",
                fname );
       menlo_report_status( TypeError, ERR_RMFILE, Text );
   }

   return;
}

/*****************************************************************************
 *  menlo_report_h71sum( )  process a Hypo71 summary message                 *
 *****************************************************************************/
#define B_DATE_H71SUM  0           /* yyyymmdd string */
#define L_DATE_H71SUM  8
#define B_HOUR_H71SUM  9           /* hhmm string */
#define L_HOUR_H71SUM  4
#define B_EID_H71SUM   91          /* last 2 digits of the event id */
#define L_EID_H71SUM   2

#define B_EID_H71SUM_L 83          /* all digits of the event id */
#define L_EID_H71SUM_L 10
#define B_VID_H71SUM_L 94          /* version of the event id */
#define L_VID_H71SUM_L 1

void menlo_report_h71sum( char *summsg )
{
   FILE *fpsum;
   char  fname[35];
   long  length;
   int   i, res;

/* Create name for local file using the event origin time and
     - last 2 digits of the event id and ArcSuffix/SumSuffix
       this is the default
       (i.e.  201011040930_57.sum)
     - all digits of the event id, plus id-version,  and SumSuffix
       (i.e.  201011040932_0000097658_2.sum)
 *********************************************/
   i = 0;
   strncpy( fname, (summsg + B_DATE_H71SUM), L_DATE_H71SUM );
   i = i + L_DATE_H71SUM;

   strncpy( (fname + i), (summsg + B_HOUR_H71SUM), L_HOUR_H71SUM );
   i = i + L_HOUR_H71SUM;

   fname[i] = '_';
   i = i + 1;

   if(LongFileName) {
       strncpy( (fname + i), (summsg + B_EID_H71SUM_L), L_EID_H71SUM_L );
       i = i + L_EID_H71SUM_L;
       fname[i] = '_';
       i = i + 1;
       strncpy( (fname + i), (summsg + B_VID_H71SUM_L), L_VID_H71SUM_L );
       i = i + L_VID_H71SUM_L;
   } else {
       strncpy( (fname + i), (summsg + B_EID_H71SUM), L_EID_H71SUM );
       i = i + L_EID_H71SUM;
   }

   fname[i] = '.';
   i = i + 1;

   strcpy ( (fname + i), SumSuffix    );
   for ( i=0; i<(int) strlen(fname); i++ )
   {
       if( fname[i] == ' ' )  fname[i] = '0';
   }
   printf("Summary file: %s\n", fname ); /*DEBUG*/

/* Write local file
 ******************/
   if( (fpsum = fopen( fname, "w" )) == (FILE *) NULL )
   {
       sprintf( Text, "menlo_report: error opening file <%s>\n",
                fname );
       menlo_report_status( TypeError, ERR_LOCALFILE, Text );
       return;
   }
   length = (long)strlen( summsg );
   if ( fwrite( summsg, (size_t)1, (size_t)length, fpsum ) == 0 )
   {
       sprintf( Text, "menlo_report: error writing to file <%s>\n",
                fname );
       menlo_report_status( TypeError, ERR_LOCALFILE, Text );
       fclose( fpsum );
       return;
   }
   fclose( fpsum );

/* Copy file to remote machine
 *****************************/
   res = copyfile( fname, TmpFile, RemoteHost, RemoteDir,
                   RemoteUser, RemotePasswd, Text );
   if (res)
   {
       logit( "et", "menlo_report: error copying <%s> to <%s:%s>\n",
               fname, RemoteHost, RemoteDir );
       menlo_report_status( TypeError, ERR_COPYFILE, Text );
       return;
   }

/* Delete local file
 *******************/
   if( KeepLocal )  return;

   if( remove( fname ) == -1 )
   {
       sprintf( Text, "menlo_report: error removing tmpfile <%s>\n",
                fname );
       menlo_report_status( TypeError, ERR_RMFILE, Text );
   }

   return;
}

/******************************************************************************
 *  menlo_report_config() processes command file(s) using kom.c functions;    *
 *                       exits if any errors are encountered.                 *
 ******************************************************************************/
void menlo_report_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[15];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 13;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e" ,
                "menlo_report: Error opening command file <%s>; exiting!\n",
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
                  logit( "e" ,
                          "menlo_report: Error opening command file <%s>; exiting!\n",
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
                    logit( "e" ,
                            "menlo_report: Too many <GetEventsFrom> commands in <%s>",
                             configfile );
                    logit( "e" , "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e" ,
                               "menlo_report: Invalid installation name <%s>", str );
                       logit( "e" , " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e" ,
                               "menlo_report: Invalid module name <%s>", str );
                       logit( "e" , " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_HYP2000ARC", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e" ,
                               "menlo_report: Invalid message type <TYPE_HYP2000ARC>" );
                    logit( "e" , "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_H71SUM2K", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit( "e" ,
                               "menlo_report: Invalid message type <TYPE_H71SUM2K>" );
                    logit( "e" , "; exiting!\n" );
                    exit( -1 );
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

         /* Enter info about remote machine to copy files to
          **************************************************/
  /*5*/     else if( k_its("RemoteHost") ) {
                str = k_str();
                if(str) strcpy( RemoteHost, str );
                init[5] = 1;
            }
  /*6*/     else if( k_its("RemoteUser") ) {
                str = k_str();
                if(str) strcpy( RemoteUser, str );
                init[6] = 1;
            }
  /*7*/     else if( k_its("RemotePasswd") ) {
                str = k_str();
                if(str) strcpy( RemotePasswd  , str );
                init[7] = 1;
            }
  /*8*/     else if( k_its("RemoteDir") ) {
                str = k_str();
                if(str) strcpy( RemoteDir, str );
                init[8] = 1;
            }

         /* Enter name of local directory to write tmp files to
          *****************************************************/
  /*9*/     else if( k_its("LocalDir") ) {
                str = k_str();
                if(str) strcpy( LocalDir, str );
                init[9] = 1;
            }

         /* Suffix to use for output archive files
          ****************************************/
  /*10*/    else if( k_its("ArcSuffix") ) {
                str = k_str();
                if(str) {
                   if( strlen(str) >= sizeof(ArcSuffix) ) {
                      logit( "e" ,
                              "menlo_report: ArcSuffix <%s> too long; ", str );
                      logit( "e" , "max length:%d; exiting!\n",
                              (int)(sizeof(ArcSuffix)-1) );
                      exit(-1);
                   }
                /* ArcSuffix shouldn't include the period */
                   if( str[0] == '.' ) strcpy( ArcSuffix, &str[1] );
                   else                strcpy( ArcSuffix, str     );
                /* Set SendArc flag based on ArcSuffix value */
                   if     (strncmp(ArcSuffix, "none", 4) == 0) SendArc=0;
                   else if(strncmp(ArcSuffix, "NONE", 4) == 0) SendArc=0;
                   else if(strncmp(ArcSuffix, "None", 4) == 0) SendArc=0;
                   else                                        SendArc=1;
                }
                init[10] = 1;
            }

         /* Suffix to use for output summary files
          ****************************************/
  /*11*/    else if( k_its("SumSuffix") ) {
                str = k_str();
                if(str) {
                   if( strlen(str) >= sizeof(SumSuffix) ) {
                      logit( "e" ,
                              "menlo_report: SumSuffix <%s> too long; ", str );
                      logit( "e" , "max length:%d; exiting!\n",
                              (int)(sizeof(SumSuffix)-1) );
                      exit(-1);
                   }
                /* SumSuffix shouldn't include the period */
                   if( str[0] == '.' ) strcpy( SumSuffix, &str[1] );
                   else                strcpy( SumSuffix, str     );
                /* Set SendSum flag based on SumSuffix value */
                   if     (strncmp(SumSuffix, "none", 4) == 0) SendSum=0;
                   else if(strncmp(SumSuffix, "NONE", 4) == 0) SendSum=0;
                   else if(strncmp(SumSuffix, "None", 4) == 0) SendSum=0;
                   else                                        SendSum=1;
                }
                init[11] = 1;
            }

         /* Temporary remote name to use for copying file
          ***********************************************/
  /*12*/    else if( k_its("TmpRemoteFile") ) {
                str = k_str();
                if(str) {
                   if( strlen(str) >= sizeof(TmpFile) ) {
                      logit( "e" ,
                              "menlo_report: TmpRemoteFile <%s> too long; ",
                               str );
                      logit( "e" , "max length:%d; exiting!\n",
                              (int)(sizeof(TmpFile)-1) );
                      exit(-1);
                   }
                   strcpy( TmpFile, str );
                }
                init[12] = 1;
            }

        /* Set flag to keep a local copy of all files
           optional command; default is delete local copies
         **************************************************/
            else if( k_its("KeepLocalCopy") ) {
                KeepLocal = 1;
            }

        /* Set flag to create long file names 
           optional command; default is short names
         **************************************************/
            else if( k_its("EnableLongFileName") ) {
                LongFileName = 1;
            }

        /* Unknown command
         *****************/
            else {
                logit( "e" , "menlo_report: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e" ,
                       "menlo_report: Bad <%s> command in <%s>; exiting!\n",
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
       logit( "e" , "menlo_report: ERROR, no " );
       if ( !init[0] )  logit( "e" , "<LogFile> "           );
       if ( !init[1] )  logit( "e" , "<MyModuleId> "        );
       if ( !init[2] )  logit( "e" , "<RingName> "          );
       if ( !init[3] )  logit( "e" , "<HeartBeatInterval> " );
       if ( !init[4] )  logit( "e" , "<GetEventsFrom> "     );
       if ( !init[5] )  logit( "e" , "<RemoteHost> "        );
       if ( !init[6] )  logit( "e" , "<RemoteUser> "        );
       if ( !init[7] )  logit( "e" , "<RemotePasswd> "      );
       if ( !init[8] )  logit( "e" , "<RemoteDir> "         );
       if ( !init[9] )  logit( "e" , "<LocalDir> "          );
       if ( !init[10])  logit( "e" , "<ArcSuffix> "         );
       if ( !init[11])  logit( "e" , "<SumSuffix> "         );
       if ( !init[12])  logit( "e" , "<TmpRemoteFile> "     );
       logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/*******************************************************************************
 *  menlo_report_lookup( )   Look up important info from earthworm.h tables    *
 *******************************************************************************/
void menlo_report_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        fprintf( stderr,
                "menlo_report:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "menlo_report: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "menlo_report: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "menlo_report: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "menlo_report: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      fprintf( stderr,
              "menlo_report: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum2K ) != 0 ) {
      fprintf( stderr,
              "menlo_report: Invalid message type <TYPE_H71SUM2K>; exiting!\n" );
      exit( -1 );
   }
   return;
}

/*****************************************************************************
 * menlo_report_status() builds a heartbeat or error message & puts it into  *
 *                       shared memory.  Writes errors to log file & screen. *
 *****************************************************************************/
void menlo_report_status( unsigned char type, short ierr, char *note )
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
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "menlo_report: %s\n", note );
   }

   size = (long)strlen( msg );  /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","menlo_report:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","menlo_report:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

