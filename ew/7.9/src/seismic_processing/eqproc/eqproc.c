
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqproc.c 6298 2015-04-10 02:49:19Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.7  2005/03/11 00:22:43  dietz
 *     Modified to make these items configurable: pick/quake FIFO lengths,
 *     heartbeat interval to statmgr, time interval at which quake FIFO is
 *     checked for events that are ready to send downstream.  All four items
 *     default to previous hardcoded values if their commands are ommitted.
 *
 *     Revision 1.6  2004/05/17 20:30:34  dietz
 *     unused variable cleanup
 *
 *     Revision 1.5  2004/05/17 20:25:49  dietz
 *     Modified to use TYPE_PICK_SCNL and TYPE_CODA_SCNL as input and
 *     to produce TYPE_EVENT_SCNL as output.
 *
 *     Revision 1.4  2002/05/16 15:34:50  patton
 *     Made logit changes.
 *
 *     Revision 1.3  2001/05/09 18:39:43  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPID.
 *
 *     Revision 1.2  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 17:12:03  lucky
 *     Initial revision
 *
 *
 */

/*
 * eqproc.c : Determine event termination and report results.
 *              This is a notifier module prototype
 */
#define ABS(x) (((x) >= 0.0) ? (x) : -(x) )
#define X(lon) (facLon * ((lon) - orgLon))
#define Y(lat) (facLat * ((lat) - orgLat))

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

#define hypot(x,y) (sqrt((x)*(x) + (y)*(y)))

/* Functions in this source file
 *******************************/
void  eqp_config ( char * );
void  eqp_lookup ( void   );
int   eqp_pickscnl ( char * );
int   eqp_codascnl ( char * );
int   eqp_link   ( char * );
int   eqp_quake2k( char * );
int   eqp_compare( const void *, const void * );
void  eqp_check  ( double );
void  eqp_status ( unsigned char, short, char * );
char *eqp_phscard( int, char * );
char *eqp_hypcard( int, char * );

static SHM_INFO  Region;          /* shared memory region to use for input  */

#define   MAXLOGO   8
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;

/* Things to read from configuration file
 ****************************************/
static char RingName[MAX_RING_STR];  /* transport ring to read from         */
static char MyModName[MAX_MOD_STR];  /* speak as this module name/id        */
static int  LogSwitch;            /* 0 if no logging should be done to disk */
static int  ReportS;              /* 0 means don't send S to next process   */
static char NextProc[150];        /* actual command to start next program   */

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

static char  EqMsg[MAX_BYTES_PER_EQ];    /* char string to hold eventscnl message */

/* Error messages used by eqproc
 *******************************/
#define  ERR_MISSMSG            0
#define  ERR_TOOBIG             1
#define  ERR_NOTRACK            2
#define  ERR_PICKREAD           3
#define  ERR_CODAREAD           4
#define  ERR_QUAKEREAD          5
#define  ERR_LINKREAD           6
#define  ERR_UNKNOWNSTA         7
#define  ERR_TIMEDECODE         8

#define  DT_CHECK               5     /* interval (sec) to loop thru quakes  */

#define MAXSTRLEN             256
static char  Text[MAXSTRLEN];         /* string for log/error messages       */

#define MAXHYP 100     /* default quake fifo length */
typedef struct {
        double  trpt;
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
double  tReport = 30.0;         /* Latency before quake report          */
double  tGrab   =  4.0;         /* Tolerance for reporting waifs        */
double orgLat;
double orgLon;
double facLat;
double facLon;
size_t maxHyp = MAXHYP;         /* Quake FIFO length                    */
size_t maxPck = MAXPCK;         /* Pick  FIFO length                    */
int HypCheckInterval = 0;       /* Time Interval (s) to check quakes    */
int HeartbeatInt = 0;           /* Heartbeat interval (s)               */
int iPrint = 0;                 /* Set to print each eq (PR...)         */
int iGraph = 0;                 /* Set to plot residuals (GR...)        */
int nT;                         /* Graph : Time axis dimension          */
int nR;                         /* Graph : Distance axis dimension      */
double tMin = -3.0;             /* Graph : Minimum time residual        */
double tMax = 3.0;              /* Graph : Maximum time residual        */
double tInc = 0.1;              /* Graph : Time grid increment          */
double tLab = 1.0;              /* Graph : Time label interval          */
double rMin = 0.0;              /* Graph : Minimum distance             */
double rMax = 600.0;            /* Graph : Maximum distance             */
double rInc = 10.0;             /* Graph : Distance grid increment      */
double rLab = 100.0;            /* Graph : Distance label interval      */
char *cUp = "PSQTRU";           /* Graph : Phase symbols (associated)   */
char *cLo = "psqtru";           /* Graph : Phase symbols (waifs)        */
char *Gr;                       /* Graph                                */
pid_t  MyPID=0;

int main( int argc, char **argv )
{
   double     t;
   double     tcheck        = 0.0;
   double     NextHeartbeat = 0.0;
   char       rec[MAXSTRLEN];   /* actual retrieved message  */
   long       recsize;          /* size of retrieved message */
   MSG_LOGO   reclogo;          /* logo of retrieved message */
   int        res;
   int        iq;
   int        is;

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: eqproc <configfile>\n" );
        exit( 0 );
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, MAXSTRLEN*2, 1 );

/* Read the configuration file(s)
 ********************************/
   eqp_config( argv[1] );
   logit( "" , "eqproc: Read command file <%s>\n", argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   eqp_lookup();

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
      logit( "et","eqproc: Error allocating quake FIFO at length=%ld; "
             "exiting!\n", maxHyp );
      exit( -1 );
   }
   logit( "", "eqproc: Allocated quake fifo (length=%ld)\n", maxHyp );

   Pck = (RPT_PCK *) calloc( maxPck, sizeof( RPT_PCK ) );
   if( Pck == (RPT_PCK *) NULL ) {
      logit( "et","eqproc: Error allocating pick FIFO at length=%ld; "
             "exiting!\n", maxPck );
      free( Hyp );
      exit( -1 );
   }
   logit( "", "eqproc: Allocated pick fifo (length=%ld)\n", maxPck );

   Srt = (SRT *) calloc( maxPck, sizeof( SRT ) );
   if( Srt == (SRT *) NULL ) {
      logit( "et","eqproc: Error allocating sort array at length=%ld; "
             "exiting!\n", maxPck );
      free( Hyp );
      free( Pck );
      exit( -1 );
   }

/* Initializate some variables
 *****************************/
   nPck = 0;                    /* no picks have been processed    */
   for(iq=0; iq<maxHyp; iq++)   /* set all hypocenter id's to zero */
   {
        Hyp[iq].id  = 0;
   }
   if( HeartbeatInt     <= 0 ) HeartbeatInt     = (int)(0.3 * tReport);
   if( HypCheckInterval <= 0 ) HypCheckInterval = (int)(0.3 * tReport);

/* Attach to PICK shared memory ring & flush out all old messages
 ****************************************************************/
   tport_attach( &Region, RingKey );
   logit( "", "eqproc: Attached to public memory region %s: %d\n",
          RingName, RingKey );
   while( tport_getmsg( &Region, GetLogo, nLogo,
                        &reclogo, &recsize, rec, MAXSTRLEN ) != GET_NONE );

/* Start the next processing program & open pipe to it
 *****************************************************/
   if( pipe_init( NextProc, 0 ) < 0 ) {
        logit( "et",
               "eqproc: Error opening pipe to %s; exiting!\n", NextProc);
        tport_detach( &Region );
        free( Hyp );
        free( Pck );
        free( Srt );
        exit( -1 );
   }
   logit( "e", "eqproc: piping output to <%s>\n", NextProc );

/* Calculate network origin
 **************************/
   logit( "", "eqproc: nSite = %d\n", nSite );

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

/*------------------- setup done; start main loop -------------------------*/

   while(1)
   {
     /* Check the hypocenters for reporting if it's time
      **************************************************/
        t = tnow();
        if(t < tcheck) {
           sleep_ew(1000);
        }
        else {
           eqp_check(t);
           tcheck = t + (double) HypCheckInterval;
        }

     /* Send heartbeat if it's time
      *****************************/
        if( t >= NextHeartbeat ) 
        {
           eqp_status( TypeHeartBeat, 0, "" );
           NextHeartbeat = t + (double) HeartbeatInt;
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
                logit( "t","eqproc: sent TYPE_KILL msg to pipe.\n");
           /* detach from shared memory */
                tport_detach( &Region );
           /* Free allocated memory */
                free( Hyp );
                free( Pck );
                free( Srt );
           /* write a few more things to log file and close it */
                if ( pipe_error() )
                     logit( "t", "eqproc: Output pipe error; exiting\n" );
                else
                     logit( "t", "eqproc: Termination requested; exiting\n" );
                sleep_ew( 500 );  /* give time for msg to get through pipe */
           /* close down communication to child */
                pipe_close();
                fflush( stdout );
                return 0;
           }

        /* Get & process the next message from shared memory
         ***************************************************/
           res = tport_getmsg( &Region, GetLogo, nLogo,
                               &reclogo, &recsize, rec, MAXSTRLEN-1 );
           switch(res)
           {
           case GET_MISS:   /* got a message; missed some */
                sprintf( Text,
                        "Missed msg(s)  i%u m%u t%u  region:%ld.",
                         reclogo.instid, reclogo.mod, reclogo.type, Region.key);
                eqp_status( TypeError, ERR_MISSMSG, Text );

           case GET_NOTRACK:   /* got a message; can't tell is any were missed */
                if(res == GET_NOTRACK)
                {
                    sprintf( Text,
                            "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                             reclogo.instid, reclogo.mod, reclogo.type );
                    eqp_status( TypeError, ERR_NOTRACK, Text );
                }
           case GET_OK:   /* got a message just fine */
                /*logit( "", "%s", rec );*/  /*debug*/
                rec[recsize] = '\0';              /*null terminate the message*/

                if( reclogo.type == TypePickSCNL )
                {
                    eqp_pickscnl( rec );
                }
                else if( reclogo.type == TypeCodaSCNL )
                {
                    eqp_codascnl( rec );
                }
                else if( reclogo.type == TypeQuake2K )
                {
                    eqp_quake2k( rec );
                }
                else if( reclogo.type == TypeLink )
                {
                    eqp_link( rec );
                }
                break;

           case GET_TOOBIG:  /* no message returned; it was too big */
                sprintf(Text,
                        "Retrieved msg[%ld] (i%u m%u t%u) too big for rec[%d]",
                        recsize, reclogo.instid, reclogo.mod, reclogo.type,
                        MAXSTRLEN-1 );
                eqp_status( TypeError, ERR_TOOBIG, Text );

           case GET_NONE:  /* no messages to process */
                break;

           }
           fflush( stdout );

        } while( res != GET_NONE );  /*end of message-processing-loop */

   }
/*-----------------------------end of main loop-------------------------------*/
}

/******************************************************************************/
/*      eqp_config() processes command file(s) using kom.c functions          */
/*                   exits if any errors are encountered                      */
/******************************************************************************/
void eqp_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   char     processor[15];
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 7;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        fprintf( stderr,
                "eqproc: Error opening command file <%s>; exiting!\n",
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
                  fprintf( stderr,
                          "eqproc: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
            strcpy( processor, "eqp_config" );

         /* Numbered commands are required
          ********************************/
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

         /* Enter installation & module to get picks & codas from
          *******************************************************/
  /*3*/     else if( k_its("GetPicksFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    fprintf( stderr,
                            "eqproc: Too many <Get*> commands in <%s>",
                             configfile );
                    fprintf( stderr, "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       fprintf( stderr,
                               "eqproc: Invalid installation name <%s>", str );
                       fprintf( stderr, " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       fprintf( stderr,
                               "eqproc: Invalid module name <%s>", str );
                       fprintf( stderr, " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_PICK_SCNL", &GetLogo[nLogo].type ) != 0 ) {
                    fprintf( stderr,
                               "eqproc: Invalid message type <TYPE_PICK_SCNL>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_CODA_SCNL", &GetLogo[nLogo+1].type ) != 0 ) {
                    fprintf( stderr,
                               "eqproc: Invalid message type <TYPE_CODA_SCNL>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type ); */  /*DEBUG*/
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo+1, (int) GetLogo[nLogo+1].instid,
                               (int) GetLogo[nLogo+1].mod,
                               (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
                nLogo  += 2;
                init[3] = 1;
            }
         /* Enter installation & module to get associations from
          ******************************************************/
  /*4*/     else if( k_its("GetAssocFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    fprintf( stderr,
                            "eqproc: Too many <Get*From> commands in <%s>",
                             configfile );
                    fprintf( stderr, "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       fprintf( stderr,
                               "eqproc: Invalid installation name <%s>", str );
                       fprintf( stderr, " in <GetAssocFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       fprintf( stderr,
                               "eqproc: Invalid module name <%s>", str );
                       fprintf( stderr, " in <GetAssocFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_QUAKE2K", &GetLogo[nLogo].type ) != 0 ) {
                    fprintf( stderr,
                            "eqproc: Invalid message type <TYPE_QUAKE2K>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_LINK", &GetLogo[nLogo+1].type ) != 0 ) {
                    fprintf( stderr,
                            "eqproc: Invalid message type <TYPE_LINK>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type ); */  /*DEBUG*/
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo+1, (int) GetLogo[nLogo+1].instid,
                               (int) GetLogo[nLogo+1].mod,
                               (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
                nLogo  += 2;
                init[4] = 1;
            }
   /*5*/    else if( k_its("PipeTo") ) {
                str = k_str();
                if(str) strcpy( NextProc, str );
                init[5] = 1;
            }
   /*6*/    else if( k_its("ReportS") ) {
                ReportS = k_int();
                init[6] = 1;
            }

         /* These commands change default values; so are not required
          ***********************************************************/
            else if( k_its("rpt_dwell") )
                tReport = k_val();

            else if( k_its("rpt_grab") )
                tGrab = k_val();

            else if( k_its("rpt_check") ) 
                HypCheckInterval = (int) k_val();

            else if( k_its("print") )
                iPrint = 1;

            else if( k_its("graph") ) {
                iGraph = 1;
                nT = (int)((tMax - tMin) / tInc) + 1;
                nR = (int)((rMax - rMin) / rInc) + 1;
                Gr = (char *)malloc(nT*nR);
            }

            else if( k_its("pick_fifo_length") )
                maxPck = (size_t) k_val();

            else if( k_its("quake_fifo_length") )
                maxHyp = (size_t) k_val();

            else if( k_its("HeartbeatInt") ) 
                HeartbeatInt = k_int();


         /* Some commands may be processed by other functions
          ***************************************************/
            else if( t_com()    )  strcpy( processor, "t_com"    );
            else if( site_com() )  strcpy( processor, "site_com" );
            else {
                fprintf( stderr, "eqproc: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               fprintf( stderr,
                       "eqproc: Bad <%s> command for %s() in <%s>; exiting!\n",
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
       fprintf( stderr, "eqproc: ERROR, no " );
       if ( !init[0] )  fprintf( stderr, "<LogFile> "      );
       if ( !init[1] )  fprintf( stderr, "<MyModuleId> "   );
       if ( !init[2] )  fprintf( stderr, "<RingName> "     );
       if ( !init[3] )  fprintf( stderr, "<GetPicksFrom> " );
       if ( !init[4] )  fprintf( stderr, "<GetAssocFrom> " );
       if ( !init[5] )  fprintf( stderr, "<PipeTo> "       );
       if ( !init[6] )  fprintf( stderr, "<ReportS> "     );
       fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/****************************************************************************/
/*  eqp_lookup( ) Look up important info from earthworm.h tables            */
/****************************************************************************/
void eqp_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        fprintf( stderr,
                "eqproc:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "eqproc: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_QUAKE2K", &TypeQuake2K ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_QUAKE2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_LINK", &TypeLink ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_LINK>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PICK_SCNL", &TypePickSCNL ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_PICK_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CODA_SCNL", &TypeCodaSCNL ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_CODA_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_EVENT_SCNL", &TypeEventSCNL ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_EVENT_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
      fprintf( stderr,
              "eqproc: Invalid message type <TYPE_KILL>; exiting!\n" );
      exit( -1 );
   }
   return;
}

/****************************************************************************/
/*  eqp_quake2k() processes a TYPE_QUAKE2K message from binder              */
/****************************************************************************/
int eqp_quake2k( char *msg )
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
   narg =  sscanf( msg,
                  "%*d %*d %ld %s %lf %lf %lf %f %f %f %f %d",
                   &qkseq, timestr, &lat, &lon, &z,
                   &rms, &dmin, &ravg, &gap, &nph );

   if ( narg < 10 )
   {
      sprintf( Text, "eqp_quake2k: Error reading ascii quake msg: %s", msg );
                     eqp_status( TypeError, ERR_QUAKEREAD, Text );
      return( 0 );
   }

   t = julsec17( timestr );
   if ( t == 0. )
   {
      sprintf( Text, "eqp_quake2k: Error decoding quake time: %s", timestr );
                eqp_status( TypeError, ERR_TIMEDECODE, Text );
      return( 0 );
   }

/* Store all hypo info in eqproc's RPT_HYP structure
 ***************************************************/
   iq = qkseq % maxHyp;

   if( qkseq != Hyp[iq].id )                /* initialize flags the 1st  */
   {                                        /* time a new quake id#      */
      Hyp[iq].flag  = 0;
      Hyp[iq].id    = qkseq;
   }
   Hyp[iq].t    = t;
   Hyp[iq].trpt = tnow() + tReport;
   Hyp[iq].lat  = lat;
   Hyp[iq].lon  = lon;
   Hyp[iq].z    = z;
   Hyp[iq].rms  = rms;
   Hyp[iq].dmin = dmin;
   Hyp[iq].ravg = ravg;
   Hyp[iq].gap  = gap;
   Hyp[iq].nph  = nph;

   if (Hyp[iq].flag != 2)                   /* don't reflag if the event */
      Hyp[iq].flag =  1;                    /* has already been reported */
   if (Hyp[iq].nph  == 0)
      Hyp[iq].flag = -1;                    /* flag an event as killed   */


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
          Hyp[iq].gap, Hyp[iq].nph );

   return( 1 );
}

/****************************************************************************/
/*  eqp_link() processes a TYPE_LINK message                                */
/****************************************************************************/
int eqp_link( char *msg )
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
      sprintf( Text, "eqp_link: Error reading ascii link msg: %s", msg );
      eqp_status( TypeError, ERR_LINKREAD, Text );
      return( 0 );
   }
   pksrc    = (unsigned char) isrc;
   pkinstid = (unsigned char) iinstid;

   lp2 = nPck;
   lp1 = lp2 - maxPck;
   if(lp1 < 0) lp1 = 0;

   for( lp=lp1; lp<lp2; lp++ )   /* loop from oldest to most recent */
   {
      ip = lp % maxPck;
      if( pkseq    != Pck[ip].id )        continue;
      if( pksrc    != Pck[ip].src )       continue;
      if( pkinstid != Pck[ip].instid )    continue;
      Pck[ip].quake = qkseq;
      Pck[ip].phase = (char) iphs;
      return( 1 );
   }

   return( 0 );
}

/****************************************************************************/
/*    eqp_pickscnl() decodes a TYPE_PICK_SCNL message from ascii to binary  */
/****************************************************************************/
int eqp_pickscnl( char *msg )
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
      sprintf( Text, "eqp_pickscnl: Error reading pick: %s", msg );
      eqp_status( TypeError, ERR_PICKREAD, Text );
      return( 0 );
   } 

/* Get site index (isite)
 ************************/
   isite = site_index( pick.site, pick.net, pick.comp, pick.loc );
   if( isite < 0 )
   {
      sprintf( Text, "eqp_pickscnl: %s.%s.%s.%s - Not in station list.",
               pick.site, pick.comp, pick.net, pick.loc );
      eqp_status( TypeError, ERR_UNKNOWNSTA, Text );
      return( 0 );
   }
 
/* Try to find coda part in existing pick list
 *********************************************/
   lp2 = nPck;
   lp1 = lp2 - maxPck;
   if( lp1 < 0 ) lp1 = 0;

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
   it will be filled by TYPE_CODA_SCNL later.
 ***********************************************/
   for( i=0; i<6; i++ ) {
      Pck[ip].caav[i] = 0;
   }
   Pck[ip].clen   = 0;

   return ( 1 );
}

/****************************************************************************/
/*  eqp_codascnl() processes a TYPE_CODA_SCNL message                       */
/****************************************************************************/

int eqp_codascnl( char *msg )
{
   EWCODA  coda;
   long    lp, lp1, lp2;
   int     i, ip;

/* Read the coda into an EWCODA struct
 *************************************/
   if( rd_coda_scnl( msg, strlen(msg), &coda ) != EW_SUCCESS )
   {
      sprintf( Text, "eqp_codascnl: Error reading coda: %s", msg );
      eqp_status( TypeError, ERR_CODAREAD, Text );
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

/* Pick was not in list; zero-out all pick info; will be filled by TYPE_PICK_SCNL
 ********************************************************************************/
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

/******************************************************************************/
/* eqp_phscard() builds a character-string phase-card from RPT_PCK structure  */
/******************************************************************************/
char *eqp_phscard( int ip, char *phscard )
{
   char   timestr[19];
   int    is, iph;
   char   datasrc;

/*--------------------------------------------------------------------------
Sample Earthworm format phase card (variable-length whitespace delimited):
CMN VHZ NC -- U1 P 19950831183134.902 953 1113 968 23 201 276 289 0 0 7 W\n
----------------------------------------------------------------------------*/

/* Get a character to denote the data source 
 *******************************************/
   if   ( Pck[ip].instid == InstId ) datasrc = 'W'; /* local worm */
   else                              datasrc = 'I'; /* imported   */

/* Convert julian seconds character string 
 *****************************************/
   date18( Pck[ip].t, timestr );
   is  = Pck[ip].site;
   iph = Pck[ip].phase;

/* Write all info to an Earthworm phase card
 *******************************************/
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

 /*logit( "", "%s", phscard );*/ /*DEBUG*/
   return( phscard );
}

/*********************************************************************************/
/* eqp_hypcard() builds a character-string hypocenter card from RPT_HYP struct   */
/*********************************************************************************/
char *eqp_hypcard( int iq, char *hypcard )

{
   char   timestr[19];
   char   version = '1';  /* 0=preliminary (no mag); 1=final with Md */

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

/*********************************************************************************/
/*  eqp_compare() compare 2 times                                                */
/*********************************************************************************/
int eqp_compare( const void *p1, const void *p2 )
{
   SRT *srt1;
   SRT *srt2;

   srt1 = (SRT *) p1;
   srt2 = (SRT *) p2;
   if(srt1->t < srt2->t)   return -1;
   if(srt1->t > srt2->t)   return  1;
   return 0;
}


/*********************************************************************************/
/* eqp_check() writes messages & files for hypocenters whose latency has expired */
/*********************************************************************************/
void eqp_check( double t )
{
        TPHASE treg[10];
        struct Greg  g;
        long minute;
        FILE *fpr;
        FILE *fgr;
        double xs, ys;
        double xq, yq, zq;
        double r;
        double dtdr, dtdz;
        double tres;
        long lp, lp1, lp2;
        int iq;
        int ip;
        int is;
        int nph;
        int nphs_P;
        int nsend;
        int iph;
        int i;
        int n;
        int ir;
        int maxr;
        int it;
        char *eqmsg;
        char chr;
        char cdate[40];
        char cwaif[40];
        char file[20];
        char txthyp[120];
        char txtpck[120];
        time_t     tsys;
        time_t     twait;

/*** Make note of current system time ***/
        time( &tsys );

/*** Loop thru all hypocenters, see if it's time to report any ***/

        for(iq=0; iq<maxHyp; iq++) {
                if(Hyp[iq].flag != 1)   continue;
                if(Hyp[iq].trpt > t)    continue;

           /* Before reporting, loop thru picks to see if all codas */
           /* have arrived or if their coda-wait time has elapsed   */
                twait = 0;
                lp2   = nPck;
                lp1   = lp2 - maxPck;
                if (lp1 < 0)
                        lp1 = 0;
                for(lp=lp1; lp<lp2; lp++) {
                        ip = lp % maxPck;
                        if ( Pck[ip].quake  != Hyp[iq].id ) continue;
                        if ( Pck[ip].instid != InstId    )  continue;
                        if ( Pck[ip].timeout > twait ) twait = Pck[ip].timeout;
                }
           /* Need to wait for more codas to arrive */
                if ( twait > tsys ) {
                        Hyp[iq].trpt += 0.2 * tReport;
                        date20( t, cdate );
                        logit( "",
                               "%s:%8ld waiting for codas.\n",
                                cdate+10, Hyp[iq].id);
                        continue;
                }

           /* All codas are available or timed-out; report this event */
                Hyp[iq].flag = 2;
                minute = (long) (Hyp[iq].t / 60.0);
                grg(minute, &g);
                sprintf(file, "PR%4d%2d%2d%2d%2d%2d",
                        g.year, g.month, g.day,
                        g.hour, g.minute, (int) (Hyp[iq].id % 100) );
                for(i=0; i<(int) strlen(file); i++)
                        if(file[i] == ' ')
                                file[i] = '0';
                date20( t, cdate );
                logit( "", "%s:%8ld #### %s\n", cdate+10, Hyp[iq].id, file);
                if(iPrint)
                        fpr = fopen(file, "w");
                if(iGraph) {
                        memset(Gr, ' ', nR*nT);
                        maxr = 0;
                }
                date20(Hyp[iq].t, cdate);
                sprintf(txthyp,
                        "%8ld %s%9.4f%10.4f%6.2f%5.2f%5.1f%5.1f%4.0f%3d",
                        Hyp[iq].id, cdate,
                        Hyp[iq].lat, Hyp[iq].lon, Hyp[iq].z,
                        Hyp[iq].rms, Hyp[iq].dmin,
                        Hyp[iq].ravg, Hyp[iq].gap, Hyp[iq].nph);
                logit( "", "%s\n", txthyp);
                if(iPrint)
                        fprintf(fpr, "%s\n", txthyp);
                xq = X(Hyp[iq].lon);
                yq = Y(Hyp[iq].lat);
                zq = Hyp[iq].z;

           /* Loop thru all picks, collecting associated picks & writing files */
                lp2 = nPck;
                lp1 = lp2 - maxPck;
                if(lp1 < 0)
                        lp1 = 0;
                nSrt  = 0;
                nphs_P = 0;
                for(lp=lp1; lp<lp2; lp++) {
                        ip = lp % maxPck;
                        if(Pck[ip].t < Hyp[iq].t - tGrab)       continue;
                        if(Pck[ip].t > Hyp[iq].t + 120.0)       continue;
                        is = Pck[ip].site;
                        iph = Pck[ip].phase;
                        date20(Pck[ip].t, cdate);
                        xs = X(Site[is].lon);
                        ys = Y(Site[is].lat);
                        r = hypot(xs - xq, ys - yq);
                        tres = Pck[ip].t - Hyp[iq].t
                                - t_phase(iph, r, zq, &dtdr, &dtdz);
                        if(Pck[ip].quake == Hyp[iq].id) {
                                sprintf(txtpck,
                                        "%-5s %-3s %-2s %-2s %s %-2s%c%c%6.1f%7.2f",
                                        Site[is].name, Site[is].comp, 
                                        Site[is].net, Site[is].loc, 
                                        cdate, Phs[iph], Pck[ip].fm,
                                        Pck[ip].wt, r, tres);
                                logit( "", "%s\n", txtpck);
                                if( iPrint ) {
                                        fprintf(fpr, "%s\n", txtpck);
                                }
                                if( iGraph ) {
                                        it = (int) ((tres - tMin) / tInc + 0.5);
                                        if(it < 0) it = 0;
                                        if(it >= nT) it = nT - 1;
                                        ir = (int) ((r - rMin) / rInc+ 0.5);
                                        if(ir < 0) ir = 0;
                                        if(ir >= nR) ir = nR - 1;
                                        if(ir > maxr) maxr = ir;
                                        if(iph >= 0 && iph <= 5)
                                                Gr[nT*ir + it] = cUp[iph];
                                }
                                if( Pck[ip].phase%2 == 0 ) {    /* It's a P-phase... */
                                   nphs_P++;                    /* ...count them     */
                                }
                                else {                          /* It's an S-phase... */
                                   if( ReportS == 0 ) continue; /* ...see if it should be skipped */
                                }
                                Srt[nSrt].t  = Pck[ip].t;       /* Add the pick to sorting list */
                                Srt[nSrt].ip = ip;
                                nSrt++;
                                continue;
                        }
                        nph = t_region(r, zq, treg);
                        strcpy(cwaif, "WAIF");
                        if(Pck[ip].quake)
                                sprintf(cwaif, "#%ld",   Pck[ip].quake);
                        for(i=0; i<nph; i++) {
                            tres = Pck[ip].t - Hyp[iq].t - treg[i].t;
                            if(tres < -tGrab)   continue;
                            if(tres >  tGrab)   continue;
                            iph = treg[i].phase;
                            sprintf(txtpck,
                                "%-5s %-3s %-2s %-2s %s %-2s%c%c%6.1f%7.2f %s",
                                Site[is].name, Site[is].comp,
                                Site[is].net, Site[is].loc, 
                                cdate, Phs[iph], Pck[ip].fm, Pck[ip].wt,
                                r, tres, cwaif);
                            logit( "", "%s\n", txtpck);
                            if(iPrint)
                                fprintf(fpr, "%s\n", txtpck);
                            if(iGraph) {
                                it = (int) ((tres - tMin) / tInc + 0.5);
                                if(it < 0) it = 0;
                                if(it >= nT) it = nT - 1;
                                ir = (int) ((r - rMin) / rInc + 0.5);
                                if(ir < 0) ir = 0;
                                if(ir >= nR) ir = nR - 1;
                                if(ir > maxr) maxr = ir;
                                if(iph >= 0 && iph <= 5)
                                        Gr[nT*ir+it] = cLo[iph];
                                if(Pck[ip].quake)
                                        Gr[nT*ir+it] = '0' + Pck[ip].quake%10;
                            }
                        }
                }
            /* close the PR* file
             ********************/
                if(iPrint)
                        fclose(fpr);

            /* build an event message an write it to the pipe
             ************************************************/
                if(nphs_P >= 4) {
                /* write binder's hypocenter to message */
                        eqp_hypcard(iq, EqMsg);
                /* sort picks by time and add them to message */
                        qsort(Srt, nSrt, sizeof(SRT), eqp_compare);
                        nsend = nSrt;
                        if( nsend > MAX_PHS_PER_EQ ) nsend = MAX_PHS_PER_EQ;
                        for (i=0; i<nsend; i++) {
                                eqmsg = EqMsg + strlen(EqMsg);
                                eqp_phscard(Srt[i].ip, eqmsg);
                        }
                /* write event message to pipe */
                        if ( pipe_put( EqMsg, TypeEventSCNL ) != 0 )
                                logit("et","eqp_check: Error writing eq message to pipe.\n");
                }

            /* write the GR* file if it was requested
             ****************************************/
                if(iGraph) {
                        file[0] = 'G';
                        file[1] = 'R';
                        fgr = fopen(file, "w");
                        fprintf(fgr, "%s\n", txthyp);
                        fprintf(fgr,
                                "KEY : P=P, S=S, Q=Pn, T=Sn, R=Pg, U=Sg\n");
                        fprintf(fgr,
                                "      Integers last dec of other quake\n");
                        fprintf(fgr,
                                "      Lower case waifs.\n");
                        n = (int) (rMin / rLab);
                        for(r=(n-2)*rLab; r<rMax+rLab; r+=rLab) {
                                ir = (int) ((r - rMin) / rInc + 0.5);
                                if(ir < 0 || ir >= nR)
                                        continue;
                                for(it=0; it<nT; it++) {
                                        if(Gr[ir*nT + it] == ' ' || it == 0)
                                                Gr[ir*nT+it] = '-';
                                }
                        }
                        n = (int) (tMin / tLab);
                        for(tres=(n-2)*tLab; tres<tMax+tLab; tres+=tLab) {
                                it = (int) ((tres - tMin) / tInc + 0.5);
                                if(it < 0 || it >= nT)
                                        continue;
                                for(ir=0; ir<nR; ir++) {
                                        if(Gr[ir*nT+it] == ' ')
                                                Gr[ir*nT+it] = '|';
                                        if(Gr[ir*nT+it] == '-')
                                                Gr[ir*nT+it] = '+';
                                }
                        }
                        for(ir=0; ir<=maxr; ir++) {
                                for(it=0; it<nT; it++) {
                                        chr = Gr[nT*ir + it];
                                        fprintf(fgr, "%c", chr);
                                }
                                fprintf(fgr, "\n");
                        }
                        fclose(fgr);
                }
        }
        fflush(stdout);
}


/****************************************************************************/
/* eqp_status() builds a heartbeat or error msg & puts it into the output   *
 * pipe.  The program at the end of mega-module chain (hyp2000_mgr) will    *
 * respond to a heartbeat message by putting it into the transport ring.    *
/****************************************************************************/
void eqp_status( unsigned char type, short ierr, char *note )
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
        logit( "et", "eqproc:  %s\n", note );
   }
   else
        msg[0] = '\0';

/* Write the message to the pipe
 *******************************/
   if( pipe_put( msg, type ) != 0 )
        logit( "et", "eqproc:  Error sending msg to pipe.\n");

   return;
}
