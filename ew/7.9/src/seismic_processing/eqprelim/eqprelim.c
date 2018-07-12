
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqprelim.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2007/11/30 00:03:30  dietz
 *     Added config file option "ReportCoda" to control whether codas are
 *     written to the output or not.
 *
 *     Revision 1.8  2007/02/26 19:50:03  paulf
 *     fixed a ton of windows warnings
 *
 *     Revision 1.7  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.6  2005/03/11 18:08:12  dietz
 *     Modified to make pick/quake FIFO lengths configurable
 *
 *     Revision 1.5  2004/05/17 22:16:53  dietz
 *     Modified to work with TYPE_PICK_SCNL and TYPE_CODA_SCNL as input
 *     and to output TYPE_EVENT_SCNL.
 *
 *     Revision 1.4  2002/06/05 15:48:52  patton
 *     Made logit changes.
 *
 *     Revision 1.3  2001/05/09 18:38:18  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPID.
 *
 *     Revision 1.2  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and 
 *     message type strings.
 *
 *     Revision 1.1  2000/02/14 17:10:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * eqprelim.c : Send out a preliminary event notification
 *              after a user-set number of picks are associated
 *              with it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <chron3.h>
#include <earthworm.h>
#include <kom.h>
#include <rdpickcoda.h>
#include <site.h>
#include <tlay.h>
#include <transport.h>

#define ABS(x) (((x) >= 0.0) ? (x) : -(x))
#define X(lon) (facLon * ((lon) - orgLon))
#define Y(lat) (facLat * ((lat) - orgLat))
#define hypot(x,y) (sqrt((x)*(x) + (y)*(y)))

/* Functions in this source file
 *******************************/
void  eqprelim_config ( char * );
void  eqprelim_lookup ( void );
int   eqprelim_pickscnl( char * );
int   eqprelim_codascnl( char * );
int   eqprelim_link   ( char * );
int   eqprelim_quake2k( char * );
int   eqprelim_compare( const void *, const void * );
void  eqprelim_notify ( int );
void  eqprelim_status ( unsigned char, short, char * );
char *eqprelim_phscard( int, char * );
char *eqprelim_hypcard( int, char * );

static SHM_INFO  Region;         /* shared memory region to use for input  */

#define   MAXLOGO   8
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

/* Things to read from configuration file
 ****************************************/
static char RingName[MAX_RING_STR];       /* transport ring to read from    */
static char MyModName[MAX_MOD_STR];      /* speak as this module name/id    */
static int  LogSwitch;          /* 0 if no logging should be done to disk   */
static char NextProc[150];      /* actual command to start next program     */
static int  ReportS;            /* if 0, don't send S to next process       */
static int  ReportCoda = 1;     /* if 0, don't send codas to next process   */
static int  NumPickNotify;      /* send prelim eq if it has this many picks */
static char LocalCode;          /* single char to label local picks with    */
static long HeartbeatInt;       /* #seconds between heartbeats              */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of ring for input         */
static unsigned char InstId;        /* local installation id         */
static unsigned char MyModId;       /* Module Id for this program    */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeQuake2K;
static unsigned char TypeLink;
static unsigned char TypePickSCNL;
static unsigned char TypeCodaSCNL;
static unsigned char TypeEventSCNL;
static unsigned char TypeKill;

static char  EqMsg[MAX_BYTES_PER_EQ]; /* char array to hold eq message  */

/* Error messages used by eqprelim
 *********************************/
#define  ERR_MISSMSG            0
#define  ERR_TOOBIG             1
#define  ERR_NOTRACK            2
#define  ERR_PICKREAD           3
#define  ERR_CODAREAD           4
#define  ERR_QUAKEREAD          5
#define  ERR_LINKREAD           6
#define  ERR_UNKNOWNSTA         7
#define  ERR_TIMEDECODE         8

#define  MAXSTRLEN            256
static char  Text[MAXSTRLEN];  /* string for log/error messages */

#define MAXHYP 100     /* default quake fifo length */
typedef struct {
        double  trpt;  /* time the event was reported */
        double  t;
        double  lat;
        double  lon;
        double  z;
        long    id;
        float   rms;
        float   dmin;
        float   ravg;
        float   gap;
        int     nph;
        short   flag;    /* 0=virgin, 1=modified, 2=reported, -1=killed  */
} RPT_HYP;
RPT_HYP *Hyp; /* alloc to length=maxHyp */

#define MAXPCK 1000   /* default pick fifo length */
long nPck = 0;
typedef struct {
        double          t;
        long            quake;
        int             id;
        unsigned char   src;
        unsigned char   instid;
        int             site;
        char            phase;
        char            ie;
        char            fm;
        char            wt;
        long            pamp[3];
        long            paav;
        long            caav[6];
        short           clen;
        time_t          timeout;
} RPT_PCK;
RPT_PCK *Pck; /* alloc to length=maxPck */

int nSrt;
typedef struct {
        double  t;
        int     ip;
} SRT;
SRT *Srt; /* alloc to length=maxPck */

/* Parameters
 ************/
double orgLat;
double orgLon;
double facLat;
double facLon;
size_t maxHyp = MAXHYP;         /* Quake FIFO length                    */
size_t maxPck = MAXPCK;         /* Pick  FIFO length                    */
pid_t  MyPID=0;

int main( int argc, char **argv )
{
   char       rec[MAXSTRLEN];   /* actual retrieved message  */
   long       recsize;          /* size of retrieved message */
   MSG_LOGO   reclogo;          /* logo of retrieved message */
   int        res;
   int        iq;
   int        is;
   time_t     timeNow;          /* current time                */
   time_t     timeLastBeat;     /* time of previous heartbeat  */

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: eqprelim <configfile>\n" );
        exit( 0 );
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, MAXSTRLEN*2, 1 );

/* Read the configuration file(s)
 ********************************/
   eqprelim_config( argv[1] );
   logit( "" , "eqprelim: Read command file <%s>\n", argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   eqprelim_lookup();

/* Store my own processid
 ************************/
   MyPID = getpid();
 
/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, MAXSTRLEN*2, LogSwitch );

/* Allocate space for Quake FIFO, Pick FIFO, sorting array
 *********************************************************/
   Hyp = (RPT_HYP *) calloc( maxHyp, sizeof( RPT_HYP ) );
   if( Hyp == (RPT_HYP *) NULL ) {
      logit( "et","eqprelim: Error allocating quake FIFO at length=%ld; "
             "exiting!\n", maxHyp );
      exit( -1 );
   }
   logit( "", "eqprelim: Allocated quake fifo (length=%ld)\n", maxHyp );

   Pck = (RPT_PCK *) calloc( maxPck, sizeof( RPT_PCK ) );
   if( Pck == (RPT_PCK *) NULL ) {
      logit( "et","eqprelim: Error allocating pick FIFO at length=%ld; "
             "exiting!\n", maxPck );
      free( Hyp );
      exit( -1 );
   }
   logit( "", "eqprelim: Allocated pick fifo (length=%ld)\n", maxPck );

   Srt = (SRT *) calloc( maxPck, sizeof( SRT ) );
   if( Srt == (SRT *) NULL ) {
      logit( "et","eqprelim: Error allocating sort array at length=%ld; "
             "exiting!\n", maxPck );
      free( Hyp );
      free( Pck );
      exit( -1 );
   }

/* Initialize some variables
 ***************************/
   nPck   = 0;                  /* no picks have been processed    */
   for(iq=0; iq<(int)maxHyp; iq++)   /* set all hypocenter id's to zero */
   {
        Hyp[iq].id  = 0;
   }

/* Attach to PICK shared memory ring & flush out all old messages
 ****************************************************************/
   tport_attach( &Region, RingKey );
   logit( "", "eqprelim: Attached to public memory region %s: %d\n",
          RingName, RingKey );
   while( tport_getmsg( &Region, GetLogo, nLogo,
                        &reclogo, &recsize, rec, MAXSTRLEN ) != GET_NONE );

/* Start the next processing program & open pipe to it
 *****************************************************/
   if( pipe_init( NextProc, 0 ) < 0 ) {
        logit( "et",
               "eqprelim: Error opening pipe to %s; exiting!\n", NextProc);
        tport_detach( &Region );
        free( Hyp );
        free( Pck );
        free( Srt );
        exit( -1 );
   }
   logit( "e", "eqprelim: piping output to <%s>\n", NextProc );

/* Calculate network origin
 **************************/
   logit( "", "eqprelim: nSite = %d\n", nSite );
   orgLat = 0.0;
   orgLon = 0.0;
   for(is=0; is<nSite; is++) {
        orgLat += Site[is].lat;
        orgLon += Site[is].lon;
   }
   orgLat /= nSite;
   orgLon /= nSite;
   facLat = (double)(40000.0 / 360.0);
   facLon = facLat * cos(6.283185 * orgLat / 360.0);

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartbeatInt - 1;

/*----------------- setup done; start main loop -----------------------*/

   while(1)
   {
     /* Send eqprelim's heartbeat
      ***************************/
        if ( time(&timeNow) - timeLastBeat  >=  HeartbeatInt )
        {
            timeLastBeat = timeNow;
            eqprelim_status( TypeHeartBeat, 0, "" );
        }

     /* Process all new hypocenter, pick-time, pick-coda, & link messages
      *******************************************************************/
        do
        {
        /* see if a termination has been requested
         *****************************************/
           if ( pipe_error() || tport_getflag( &Region ) == TERMINATE  ||
                tport_getflag( &Region ) == MyPID )
           {
           /* send a kill message down the pipe */
                pipe_put( "", TypeKill );
                logit("e","eqprelim: sent TYPE_KILL msg to pipe.\n");
           /* detach from shared memory */
                tport_detach( &Region );
           /* free allocated memory */
                free( Hyp );
                free( Pck );
                free( Srt );
           /* write a few more things to log file and close it */
                if ( pipe_error() )
                     logit( "t", "eqprelim: Output pipe error; exiting\n" );
                else
                     logit( "t", "eqprelim: Termination requested; exiting\n" );
                sleep_ew( 500 );  /* give time for msg to get through pipe */
           /* close down communication to child */
                pipe_close();
                fflush( stdout );
                return( 0 );
           }

        /* Get next message from shared memory; check return code
         ********************************************************/
           res = tport_getmsg( &Region, GetLogo, nLogo,
                               &reclogo, &recsize, rec, MAXSTRLEN-1 );

           if( res == GET_NONE )           /* no new messages */
           {
                break;
           }
           else if( res == GET_TOOBIG )    /* next msg too big for target; */
           {                               /* complain & try for another   */
                sprintf(Text,
                        "Retrieved msg[%ld] (i%u m%u t%u) too big for rec[%d]",
                        recsize, reclogo.instid, reclogo.mod, reclogo.type,
                        MAXSTRLEN-1 );
                eqprelim_status( TypeError, ERR_TOOBIG, Text );
                continue;
           }
           else if( res == GET_MISS )      /* got a message, but missed some */
           {
                sprintf( Text,
                        "Missed msg(s)  i%u m%u t%u  region:%ld.",
                         reclogo.instid, reclogo.mod, reclogo.type, Region.key);
                eqprelim_status( TypeError, ERR_MISSMSG, Text );
           }
           else if( res == GET_NOTRACK )   /* got a message; couldn't tell */
           {                               /* if any were missed or not    */
                sprintf( Text,
                        "Msg received i%u m%u t%u; transport.h NTRACK_GET exceeded",
                         reclogo.instid, reclogo.mod, reclogo.type );
                eqprelim_status( TypeError, ERR_NOTRACK, Text );
           }

        /* Process the new message
         *************************/
           rec[recsize] = '\0';         /*null terminate the message*/
         /*logit( "", "type:%u\n%s", reclogo.type, rec );*/   /*DEBUG*/

           if( reclogo.type == TypePickSCNL )
           {
               eqprelim_pickscnl( rec );
           }
           else if( reclogo.type == TypeCodaSCNL )
           {
               eqprelim_codascnl( rec );
           }
           else if( reclogo.type == TypeQuake2K )
           {
               eqprelim_quake2k( rec );
           }
           else if( reclogo.type == TypeLink )
           {
               eqprelim_link( rec );
           }

        } while( res != GET_NONE );  /*end of message-processing-loop */

        sleep_ew( 500 );  /* wait a bit for more messages to show up  */

   }
/*-----------------------------end of main loop-------------------------------*/
}

/******************************************************************************
 *   eqprelim_config() processes command file(s) using kom.c functions        *
 *                   exits if any errors are encountered                      *
 ******************************************************************************/
#define ncommand 10         /* # of required commands you expect to process   */
void eqprelim_config( char *configfile )
{
   char     init[ncommand]; /* init flags, one byte for each required command */
   int      nmiss;          /* number of required commands that were missed   */
   char    *com; 
   char    *str;
   char     processor[64];
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "eqprelim: Error opening command file <%s>; exiting!\n",
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
                          "eqprelim: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
            strcpy( processor, "eqprelim_config" );

         /* Numbered commands are required
          ********************************/
  /*0*/     if( k_its("HeartbeatInt") ) {
                HeartbeatInt = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[1] = 1;
            }
  /*2*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[3] = 1;
            }

         /* Enter installation & module to get picks & codas from
          *******************************************************/
  /*4*/     else if( k_its("GetPicksFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit( "e",
                            "eqprelim: Too many <Get*> commands in <%s>",
                             configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                               "eqprelim: Invalid installation name <%s>", str );
                       logit( "e", " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e",
                               "eqprelim: Invalid module name <%s>", str );
                       logit( "e", " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_PICK_SCNL", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e",
                               "eqprelim: Invalid message type <TYPE_PICK_SCNL>" );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_CODA_SCNL", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit( "e",
                               "eqprelim: Invalid message type <TYPE_CODA_SCNL>" );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type ); */  /*DEBUG*/
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo+1, (int) GetLogo[nLogo+1].instid,
                               (int) GetLogo[nLogo+1].mod,
                               (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
                nLogo  += 2;
                init[4] = 1;
            }
         /* Enter installation & module to get associations from
          ******************************************************/
  /*5*/     else if( k_its("GetAssocFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit( "e",
                            "eqprelim: Too many <Get*From> commands in <%s>",
                             configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                               "eqprelim: Invalid installation name <%s>", str );
                       logit( "e", " in <GetAssocFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e",
                               "eqprelim: Invalid module name <%s>", str );
                       logit( "e", " in <GetAssocFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_QUAKE2K", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e",
                               "eqprelim: Invalid message type <TYPE_QUAKE2K>" );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_LINK", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit( "e",
                               "eqprelim: Invalid message type <TYPE_LINK>" );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type ); */  /*DEBUG*/
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo+1, (int) GetLogo[nLogo+1].instid,
                               (int) GetLogo[nLogo+1].mod,
                               (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
                nLogo  += 2;
                init[5] = 1;
            }

         /* Enter next process to hand the event to
          *****************************************/
   /*6*/    else if( k_its("PipeTo") ) {
                str = k_str();
                if(str) strcpy( NextProc, str );
                init[6] = 1;
            }

         /* Enter rule for making prelimary notification
          **********************************************/
   /*7*/    else if( k_its("NumPickNotify") ) {
                NumPickNotify = k_int();
                init[7] = 1;
            }

         /* Single char to label locally-made picks with
          **********************************************/
   /*8*/    else if( k_its("LocalCode") ) {
                if ( str=k_str() ) {
                   LocalCode = str[0];
                }
                init[8] = 1;
            }

        /* Set switch for sending S-phases to next process
          *************************************************/
  /*9*/    else if( k_its("ReportS") ) {
                ReportS = k_int();
                init[9] = 1;
            }

         /* Optional commands to change default settings
          **********************************************/
            else if( k_its("pick_fifo_length") ) {
                maxPck = (size_t) k_val();
            }

            else if( k_its("quake_fifo_length") ) {
                maxHyp = (size_t) k_val();
            }

            else if( k_its("ReportCoda") ) {
                ReportCoda = k_int();
            }


         /* Some commands may be processed by other functions
          ***************************************************/
            else if( t_com()    )  strcpy( processor, "t_com"    );
            else if( site_com() )  strcpy( processor, "site_com" );
            else {
                logit( "e", "eqprelim: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                       "eqprelim: Bad <%s> command for %s() in <%s>; exiting!\n",
                        com, processor, configfile );
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
       logit( "e", "eqprelim: ERROR, no " );
       if ( !init[0] )  logit( "e", "<HeartbeatInt> "  );
       if ( !init[1] )  logit( "e", "<LogFile> "       );
       if ( !init[2] )  logit( "e", "<MyModuleId> "    );
       if ( !init[3] )  logit( "e", "<RingName> "      );
       if ( !init[4] )  logit( "e", "<GetPicksFrom> "  );
       if ( !init[5] )  logit( "e", "<GetAssocFrom> "  );
       if ( !init[6] )  logit( "e", "<PipeTo> "        );
       if ( !init[7] )  logit( "e", "<NumPickNotify> " );
       if ( !init[8] )  logit( "e", "<LocalCode> "     );
       if ( !init[9] )  logit( "e", "<ReportS> "       );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/*****************************************************************************
 *  eqprelim_lookup( ) Look up important info from earthworm.h tables        *
 *****************************************************************************/
void eqprelim_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        fprintf( stderr,
                "eqprelim:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "eqprelim: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_QUAKE2K", &TypeQuake2K ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_QUAKE2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_LINK", &TypeLink ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_LINK>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PICK_SCNL", &TypePickSCNL ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_PICK_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CODA_SCNL", &TypeCodaSCNL ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_CODA_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_EVENT_SCNL", &TypeEventSCNL ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_EVENT_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
      fprintf( stderr,
              "eqprelim: Invalid message type <TYPE_KILL>; exiting!\n" );
      exit( -1 );
   }
   return;
}

/****************************************************************************
 *  eqprelim_quake2k() processes a TYPE_QUAKE2K message from binder_ew      *
 ****************************************************************************/
int eqprelim_quake2k( char *msg )
{
   char       cdate[25];
   char       corg[25];
   char       timestr[20];
   double     t;
   double     lat, lon, z;
   float      rms, dmin, ravg, gap;
   long       qkseq;
   int        nph;
   int        iq;
   int        narg;

/* Read info from ascii message
 ******************************/
   narg = sscanf( msg,
                 "%*d %*d %ld %s %lf %lf %lf %f %f %f %f %d",
                  &qkseq, timestr, &lat, &lon, &z,
                  &rms, &dmin, &ravg, &gap, &nph );

   if ( narg < 10 )
   {
       sprintf( Text, "eqprelim_quake2k: Error reading ascii quake msg: %s", msg );
       eqprelim_status( TypeError, ERR_QUAKEREAD, Text );
       return( 0 );
   }

   t = julsec17( timestr );
   if ( t == 0. )
   {
       sprintf( Text, "eqprelim_quake2k: Error decoding quake time: %s", timestr );
       eqprelim_status( TypeError, ERR_TIMEDECODE, Text );
       return( 0 );
   }

/* Store all hypo info in eqprelim's RPT_HYP structure
 *****************************************************/
   iq = qkseq % maxHyp;

   if( qkseq != Hyp[iq].id )        /* initialize flags the 1st  */
   {                                /* time a new quake id#      */
        Hyp[iq].flag  = 0;
        Hyp[iq].id    = qkseq;
   }
   Hyp[iq].t    = t;
   Hyp[iq].trpt = 0.0;
   Hyp[iq].lat  = lat;
   Hyp[iq].lon  = lon;
   Hyp[iq].z    = z;
   Hyp[iq].rms  = rms;
   Hyp[iq].dmin = dmin;
   Hyp[iq].ravg = ravg;
   Hyp[iq].gap  = gap;
   Hyp[iq].nph  = nph;

   if (Hyp[iq].flag != 2)             /* don't reflag if the event */
        Hyp[iq].flag =  1;            /* has already been reported */
   if (Hyp[iq].nph  == 0)
        Hyp[iq].flag = -1;            /* flag an event as killed   */

/* Write out the time-stamped hypocenter to the log file
 *******************************************************/
   t = tnow();
   date20( t, cdate );
   date20( Hyp[iq].t, corg );
   logit( "",
         "%s:%8ld %s%9.4f%10.4f%6.2f%5.2f%5.1f%5.1f%4.0f%3d\n",
          cdate+10, Hyp[iq].id, corg+10,
          Hyp[iq].lat, Hyp[iq].lon, Hyp[iq].z,
          Hyp[iq].rms, Hyp[iq].dmin, Hyp[iq].ravg,
          Hyp[iq].gap, Hyp[iq].nph);

/* Send event2k message if it passes
 ***********************************/
   eqprelim_notify( iq );

   return( 1 );
}

/*****************************************************************************
 *  eqprelim_link() processes a MSG_LINK                                     *
 *****************************************************************************/
int eqprelim_link( char *msg )
{
   long          lp1;
   long          lp2;
   long          lp;
   int           ip, narg;
   long          qkseq;
   int           pkseq;
   unsigned char pksrc;
   unsigned char pkinstid;
   int           isrc, iinstid, iphs;

   narg  = sscanf( msg, "%ld %d %d %d %d",
                  &qkseq, &iinstid, &isrc, &pkseq, &iphs );

   if ( narg < 5 )
   {
       sprintf( Text, "eqprelim_link: Error reading ascii link msg: %s", msg );
       eqprelim_status( TypeError, ERR_LINKREAD, Text );
       return( 0 );
   }
   pksrc    = (unsigned char) isrc;
   pkinstid = (unsigned char) iinstid;

   lp2 = nPck;
   lp1 = lp2 - maxPck;
   if(lp1 < 0)
        lp1 = 0;
   for( lp=lp1; lp<lp2; lp++ )   /* loop from oldest to most recent */
   {
        ip = lp % maxPck;
        if( pkseq    != Pck[ip].id )      continue;
        if( pksrc    != Pck[ip].src )     continue;
        if( pkinstid != Pck[ip].instid )  continue;
        Pck[ip].quake = qkseq;
        Pck[ip].phase = (char) iphs;
        return( 1 );
   }
   return( 0 );
}

/*****************************************************************************
 * eqprelim_pickscnl() decodes a TYPE_PICK_SCNL message from ascii to binary *
 *****************************************************************************/
int eqprelim_pickscnl( char *msg )
{
   EWPICK   pick;
   time_t   t_in;
   int      isite;
   int      lp, lp1, lp2, ip;
   int      i;

/* Make note of current system time
 **********************************/
   time( &t_in );

 /* Read the pick into an EWPICK struct
 *************************************/
   if( rd_pick_scnl( msg, strlen(msg), &pick ) != EW_SUCCESS )
   {
      sprintf( Text, "eqprelim_pickscnl: Error reading pick: %s", msg );
      eqprelim_status( TypeError, ERR_PICKREAD, Text );
      return( 0 );
   }
 
/* Get site index (isite)
 ************************/
   isite = site_index( pick.site, pick.net, pick.comp, pick.loc );
   if( isite < 0 )
   {
      sprintf( Text, "eqprelim_pickscnl: %s.%s.%s.%s - Not in station list.",
               pick.site, pick.comp, pick.net, pick.loc );
      eqprelim_status( TypeError, ERR_UNKNOWNSTA, Text );
      return( 0 );
   }
        
/* Try to find coda part in existing pick list
 *********************************************/
   lp2 = nPck;
   lp1 = lp2 - maxPck;
   if(lp1 < 0)
         lp1 = 0;
   for( lp=lp2-1; lp>=lp1; lp-- )  /* loop from most recent to oldest */
   {
      ip = lp % maxPck;
      if( pick.instid != Pck[ip].instid ) continue;
      if( pick.modid  != Pck[ip].src    ) continue;
      if( pick.seq    != Pck[ip].id     ) continue;
      Pck[ip].t       = pick.tpick + (double)GSEC1970; /* use gregorian time */
      Pck[ip].site    = isite;
      Pck[ip].phase   = 0;
      Pck[ip].fm      = pick.fm;
      Pck[ip].ie      = ' ';
      Pck[ip].wt      = pick.wt;
      Pck[ip].timeout = 0;
      for( i=0; i<3; i++ ) {
         Pck[ip].pamp[i] = pick.pamp[i];
      }
      return( 1 );
   }

/* Coda was not in list; load pick info into list
 ************************************************/
   ip  = nPck % maxPck;
   Pck[ip].instid  = pick.instid;
   Pck[ip].src     = pick.modid;
   Pck[ip].id      = pick.seq;
   Pck[ip].t       = pick.tpick + (double)GSEC1970; /* use gregorian time */
   Pck[ip].site    = isite;
   Pck[ip].phase   = 0;
   Pck[ip].fm      = pick.fm;
   Pck[ip].ie      = ' ';
   Pck[ip].wt      = pick.wt;
   for( i=0; i<3; i++ ) {
         Pck[ip].pamp[i] = pick.pamp[i];
   }
   Pck[ip].quake   = 0;
   Pck[ip].timeout = t_in + 150;
   nPck++;

/* Coda was not in list; zero-out all coda info; 
   will be filled by TYPE_CODA_SCNL later.
 ***********************************************/
   for( i=0; i<6; i++ ) {
      Pck[ip].caav[i] = 0;
   }
   Pck[ip].clen   = 0;

   return ( 1 );
}


/*****************************************************************************
 *  eqprelim_codascnl() processes a TYPE_CODA_SCNL message                   *
 *****************************************************************************/

int eqprelim_codascnl( char *msg )
{
   EWCODA  coda;
   long    lp, lp1, lp2;
   int     i, ip;

/* Read the coda into an EWCODA struct
 *************************************/
   if( rd_coda_scnl( msg, strlen(msg), &coda ) != EW_SUCCESS )
   {
      sprintf( Text, "eqprelim_codascnl: Error reading coda: %s", msg );
      eqprelim_status( TypeError, ERR_CODAREAD, Text );
      return( 0 );
   }
  
/* Try to find pick part in existing pick list
 *********************************************/
   lp2 = nPck;
   lp1 = lp2 - maxPck;
   if( lp1 < 0 ) lp1 = 0;
 
   for( lp=lp2-1; lp>=lp1; lp-- )  /* loop from most recent to oldest */
   {
      ip = lp % maxPck;
      if( coda.instid != Pck[ip].instid )  continue;
      if( coda.modid  != Pck[ip].src )     continue;
      if( coda.seq    != Pck[ip].id )      continue;
      for( i=0; i<6; i++ ) {
         Pck[ip].caav[i] = coda.caav[i];
      }
      Pck[ip].clen     = coda.dur;
      Pck[ip].timeout  = 0;
      return( 1 );
   }

/* Pick was not in list; load coda info into list
 ************************************************/
   ip  = nPck % maxPck;
   Pck[ip].instid = coda.instid;
   Pck[ip].src    = coda.modid;
   Pck[ip].id     = coda.seq;
   for( i=0; i<6; i++ ) {
      Pck[ip].caav[i] = coda.caav[i];
   }
   Pck[ip].clen   = coda.dur;
   Pck[ip].quake  = 0;
   nPck++;

/* Pick was not in list; zero-out all pick info; 
   will be filled by TYPE_PICK_SCNL later.
 ***********************************************/
   Pck[ip].t      = 0.0;
   Pck[ip].site   = 0;
   Pck[ip].phase  = 0;
   Pck[ip].fm     = ' ';
   Pck[ip].ie     = ' ';
   Pck[ip].wt     = '9';
   for( i=0; i<3; i++ ) {
      Pck[ip].pamp[i] = (long) 0.;
   }
   Pck[ip].timeout  = 0;
   return( 1 );
}

/*****************************************************************************
 * eqprelim_phscard() builds a char-string phase-card from RPT_PCK structure *
 *****************************************************************************/
char *eqprelim_phscard( int ip, char *phscard )
{
   char   timestr[19];
   int    is, iph;
   char   datasrc;
   char   imported = 'I';

/*--------------------------------------------------------------------------
Sample Earthworm format phase card (variable-length whitespace delimited):
CMN VHZ NC -- U1 P 19950831183134.902 953 1113 968 23 201 276 289 0 0 7 W\n
----------------------------------------------------------------------------*/
 
/* Get a character to denote the data source
 *******************************************/
   if   ( Pck[ip].instid == InstId )  datasrc = LocalCode;
   else                               datasrc = imported;

/* Convert julian seconds character string
 *****************************************/
   date18( Pck[ip].t, timestr );
   is  = Pck[ip].site;
   iph = Pck[ip].phase;

/* Write all info to an Earthworm phase card
 *******************************************/
   if( ReportCoda ) {
      sprintf( phscard,
           "%s %s %s %s %c%c %s %s %ld %ld %ld %ld %ld %ld %ld %ld %ld %hd %c\n",
            Site[is].name,
            Site[is].comp,
            Site[is].net,
            Site[is].loc,
            Pck[ip].fm,
            Pck[ip].wt,
            Phs[iph],
            timestr,
            Pck[ip].pamp[0],
            Pck[ip].pamp[1],
            Pck[ip].pamp[2],
            Pck[ip].caav[0],
            Pck[ip].caav[1],
            Pck[ip].caav[2],
            Pck[ip].caav[3],
            Pck[ip].caav[4],
            Pck[ip].caav[5],
            Pck[ip].clen,
            datasrc );
   } else {
       sprintf( phscard,
           "%s %s %s %s %c%c %s %s %ld %ld %ld 0 0 0 0 0 0 0 %c\n",
            Site[is].name,
            Site[is].comp,
            Site[is].net,
            Site[is].loc,
            Pck[ip].fm,
            Pck[ip].wt,
            Phs[iph],
            timestr,
            Pck[ip].pamp[0],
            Pck[ip].pamp[1],
            Pck[ip].pamp[2],
            datasrc );
   }

 /*logit( "", "%s", phscard );*/ /*DEBUG*/
   return( phscard );
}

/******************************************************************************
 * eqprelim_hypcard() builds char-string hypocenter card from RPT_HYP struct  *
 ******************************************************************************/
char *eqprelim_hypcard( int iq, char *hypcard )
{
   char   timestr[19];
   char   version = '0';  /* 0=preliminary (no mag); 1=final with Md */
 
/*------------------------------------------------------------------------------------
Sample binder-based hypocenter as built below (whitespace delimited, variable length);
Event id from binder is added at end of card.
19920429011704.653 36.346578 -120.546932 8.51 27 78 19.8 0.16 10103 1\n
--------------------------------------------------------------------------------------*/
 
 /* Convert julian seconds character string
 *****************************************/
   date18( Hyp[iq].t, timestr );

/* Write all info to hypocenter card
 ***********************************/
   sprintf( hypcard,
           "%s %.6lf %.6lf %.2lf %d %.0f %.1f %.2f %ld %c\n",
            timestr,
            Hyp[iq].lat,
            Hyp[iq].lon,
            Hyp[iq].z,
            Hyp[iq].nph,
            Hyp[iq].gap,
            Hyp[iq].dmin,
            Hyp[iq].rms,
            Hyp[iq].id,
            version );
 
/* logit( "", "%s", hypcard );*/ /*DEBUG*/
   return( hypcard );
}

/******************************************************************************
 *  eqprelim_compare() compare 2 times                                        *
 ******************************************************************************/
int eqprelim_compare( const void *p1, const void *p2 )
{
   SRT *srt1;
   SRT *srt2;

   srt1 = (SRT *) p1;
   srt2 = (SRT *) p2;
   if(srt1->t < srt2->t)   return -1;
   if(srt1->t > srt2->t)   return  1;
   return 0;
}


/******************************************************************************
 * eqprelim_notify() writes message for a hypocenter that passes the          *
 *                   preliminary notification rule                            *
 ******************************************************************************/
void eqprelim_notify( int iq )  /* iq is the index into Hyp    */
{
   struct Greg  g;
   long         minute;
   char         cdate[40];       /* string for current time      */
   double       t;               /* current time                 */
   double       xs, ys;          /* x,y coordinates of station   */
   double       xq, yq, zq;      /* x,y,x coords of hypocenter   */
   double       r;               /* epicentral distance (km)     */
   double       dtdr, dtdz;      /* travel-time stuff            */
   double       tres;            /* travel-time residual         */
   long         lp, lp1, lp2;    /* indices into pick structure  */
   int          i, is, ip, iph;  /* working indices              */
   int          nphs_P;          /* number of P-phases           */
   int          nsend;           /* number of phases to ship out */
   char        *eqmsg;           /* working pointer into EqMsg   */

/* See if this hypocenter passes the notification rule
 *****************************************************/
   if( Hyp[iq].flag == 2 )             return;   /* already reported it   */
   if( Hyp[iq].nph  < NumPickNotify )  return;   /* not enough phases yet */

/* Note current system time
 **************************/
   t = tnow();
   date20( t, cdate );

/* Loop thru all picks, collecting associated picks
 **************************************************/
   nSrt   = 0;
   nphs_P = 0;
   lp2    = nPck;
   lp1    = lp2 - maxPck;
   if(lp1 < 0) lp1 = 0;
   for(lp=lp1; lp<lp2; lp++)
   {
      ip  = lp % maxPck;
      if(Pck[ip].quake != Hyp[iq].id) continue;  /* not assoc w/this quake */
      if(Pck[ip].phase%2 == 0) {      /* It's a P-phase... */
         nphs_P++;                    /* ...count them     */
      }
      else {                          /* It's an S-phase... */
         if(ReportS == 0) continue;   /* ...see if it should be skipped */
      }
      Srt[nSrt].t  = Pck[ip].t;       /* load info for sorting  */
      Srt[nSrt].ip = ip;
      nSrt++;                         /* count total # phases   */
   }

/* Make sure we have the minimum number of P-phases before notifying
 *******************************************************************/
   if ( nphs_P < NumPickNotify )  {
      logit( "",
            "%s:%8ld #### Report delayed: %3d P-phs; %3d picks total\n",
            cdate+10, Hyp[iq].id, nphs_P, Hyp[iq].nph );
      return;
   }

/* Report this event
 *******************/
   minute = (long) (Hyp[iq].t / 60.0);
   grg(minute, &g);
   logit( "", "%s:%8ld #### Preliminary report: %04d%02d%02d%02d%02d%_%02d\n",
          cdate+10, Hyp[iq].id, g.year, g.month, g.day,
          g.hour, g.minute, (int) (Hyp[iq].id % 100) );
   date20( Hyp[iq].t, cdate );
   logit("", "%8ld %s%9.4f%10.4f%6.2f%5.2f%5.1f%5.1f%4.0f%3d\n",
         Hyp[iq].id, cdate,
         Hyp[iq].lat, Hyp[iq].lon, Hyp[iq].z,
         Hyp[iq].rms, Hyp[iq].dmin,
         Hyp[iq].ravg, Hyp[iq].gap, Hyp[iq].nph);
   xq = X(Hyp[iq].lon);
   yq = Y(Hyp[iq].lat);
   zq = Hyp[iq].z;

/* Loop thru all associated picks, writing logfile output
 ********************************************************/
   for(i=0; i<nSrt; i++ )
   {
      ip  = Srt[i].ip;
      is  = Pck[ip].site;
      iph = Pck[ip].phase;
      date20(Pck[ip].t, cdate);
      xs  = X(Site[is].lon);
      ys  = Y(Site[is].lat);
      r   = hypot(xs - xq, ys - yq);
      tres = Pck[ip].t - Hyp[iq].t
             - t_phase(iph, r, zq, &dtdr, &dtdz);
      logit("", "%-5s %-3s %-2s %-2s %s %-2s%c%c%6.1f%7.2f\n",
            Site[is].name, Site[is].comp, 
            Site[is].net, Site[is].loc, 
            cdate, Phs[iph], Pck[ip].fm,
            Pck[ip].wt, r, tres);
   }

/* Build an event message an write it to the pipe
 ************************************************/
/* write binder's hypocenter to message */
   eqprelim_hypcard(iq, EqMsg);
/* sort picks by time and add them to message */
   qsort(Srt, nSrt, sizeof(SRT), eqprelim_compare);
   nsend = nSrt;
   if( nsend > MAX_PHS_PER_EQ ) nsend = MAX_PHS_PER_EQ;
   for (i=0; i<nsend; i++) {
      eqmsg = EqMsg + strlen(EqMsg);
      eqprelim_phscard(Srt[i].ip, eqmsg);
   }
/* write event message to pipe */
   if ( pipe_put( EqMsg, TypeEventSCNL ) != 0 )
      logit("et","eqprelim_check: Error writing eq message to pipe.\n");

/* Flag event as reported & note the time
 ****************************************/
   Hyp[iq].flag = 2;
   Hyp[iq].trpt = t;

   return;
}


/******************************************************************************
 * eqprelim_status() builds a heartbeat or error msg & puts it into shared    *
 *                   memory                                                   *
 ******************************************************************************/
void eqprelim_status( unsigned char type, short ierr, char *note )
{
   char     msg[256];
   time_t   t;

/* Build the message
 *******************/
   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %d\n", (long) t, (int) MyPID );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %d %s\n", (long) t, ierr, note);
        logit( "et", "eqprelim:  %s\n", note );
   }

/* Write the message to the pipe
 *******************************/
   if( pipe_put( msg, type ) != 0 )
        logit( "et", "eqprelim:  Error sending msg to pipe.\n");

   return;
}
