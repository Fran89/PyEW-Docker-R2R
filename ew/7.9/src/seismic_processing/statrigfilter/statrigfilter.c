/*
 *   THIS FILE IS UNDER CVS - 
 *   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
 *
 *    $Id: statrigfilter.c 2710 2007-02-26 13:44:40Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.3  2007/02/23 17:10:40  paulf
 *     fixed long warning for time_t
 *
 *     Revision 1.2  2005/12/19 16:39:01  lombard
 *     Added missing newlines to logit calls.
 *
 *     Revision 1.1  2005/11/23 18:56:31  dietz
 *     New module for filtering individual station triggers. Heavily based
 *     on the code from pkfilter.
 *
 */

/*
 * statrigfilter.c
 *
 * Reads individual station triggers from one transport ring and writes
 * them to another ring. Filters out duplicate triggers from the same 
 * station that are close in time (where "close" is configurable).
 * Does not alter the contents of the message in any way.
 * Based heavily on the code of pkfilter!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <kom.h>

#define ABS(X) (((X) >= 0) ? (X) : -(X))

typedef union {
   char cmp[TRACE2_CHAN_LEN];
   int  i;
} COMPONENT;

/* Globals
 *********/
static SHM_INFO  InRegion;    /* shared memory region to use for input  */
static SHM_INFO  OutRegion;   /* shared memory region to use for output */
static pid_t	 MyPid;       /* Our process id is sent with heartbeat  */

/* Things to read or derive from configuration file
 **************************************************/
static int     LogSwitch;          /* 0 if no logfile should be written */
static long    HeartbeatInt;       /* seconds between heartbeats        */
static long    MaxMessageSize = 256; /* size (bytes) of largest msg     */
static int     Debug = 0;          /* 0=no debug msgs, non-zero=debug   */
static MSG_LOGO *GetLogo = NULL;   /* logo(s) to get from shared memory */
static short   nLogo   = 0;        /* # logos we're configured to get   */
static double  TimeTolerance;      /* # of seconds +- triggers match    */
static int     UseOriginalLogo=0;  /* 0=use statrigfilter's own logo on output msgs */
                                   /* non-zero=use original logos on output         */
                                   /*   NOTE: this requires that output go to       */
                                   /*   different transport ring than input         */
static int     nTriggerHistory;    /* Track this many of the most recent triggers   */
                                   /*   that have passed the filter for each sta    */
static int     OlderTrigAllowed;   /* 0=reject any trigger whose timestamp is more  */
                                   /*   than TimeTolerance sec earlier than the     */
                                   /*   youngest trigger which has been passed.     */
                                   /* 1=accept a trigger whose timestamp is         */
                                   /*   earlier than the youngest passed trigger,   */
                                   /*   but place a limit on how old it can be.     */
                                   /* 2=accept any trigger whose timestamp is       */
                                   /*   earlier than the youngest passed trigger.   */
static int     OlderTrigLimit=-1;  /* Accept an trigger whose timestamp is between  */
                                   /*   TimeTolerance and OlderTrigLimit sec        */
                                   /*   earlier than the youngest passed trigger.   */
                                   /*   Required only when OlderTrigAllowed=1.      */
static COMPONENT *AllowComp=NULL;  /* list of component codes allowed to pass       */
static short      nAllowComp = 0;  /* # components in allow-list. If nAllowComp     */
                                   /*   is zero, it means all components are        */ 
                                   /*   allowed (slightly counter-intuitive).       */

/* Handy statrigfilter definitions
 *********************************/
#define  OLDER_TRIG_NONE  0
#define  OLDER_TRIG_LIMIT 1
#define  OLDER_TRIG_ALL   2

/* Structures for holding station trigger info
 *********************************************/
typedef struct _STATRIG {
   char          site[TRACE2_STA_LEN];
   char          cmp[TRACE2_CHAN_LEN];
   char          net[TRACE2_NET_LEN];
   char          loc[TRACE2_LOC_LEN];
   double        ton;              /* trigger "on" time                  */
   double        toff;             /* trigger "off" time (0 while "on")  */
   double        onEta;            /* eta when the trigger went on       */ 
   long          seq;              /* trigger sequence number            */
} STATRIG;

typedef struct _TRGHISTORY {
   char          site[6];          /* site code we're tracking               */
   char          net[3];           /* net code we're tracking                */
   double        tyoung;           /* time of youngest trigger that's passed */
   unsigned long npass;            /* cumulative number of passed triggers   */
   int           ntrg;             /* max # of statrigs in the list          */
   STATRIG      *trg;              /* dynamically allocated list of last-    */
                                   /*   shipped triggers(s) for this sta     */
} TRGHISTORY;

static TRGHISTORY *TrgHist;        /* dynamically allocated list of last- */
                                   /*   shipped trigger(s) per site/net   */
static int     nHist = 0;          /* number of stations in history list  */
  

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input   */
static long          OutRingKey;    /* key of transport ring for output  */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeCarlStaTrig;
static unsigned char TypeCarlStaTrigSCNL;

/* Error messages used by statrigfilter
 *********************************/
#define  ERR_MISSGAP       0   /* sequence gap in transport ring         */
#define  ERR_MISSLAP       1   /* missed messages in transport ring      */
#define  ERR_TOOBIG        2   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       3   /* msg retreived; tracking limit exceeded */
static char  Text[150];        /* string for log/error messages          */

/* Functions in this source file
 *******************************/
void statrigfilter_config( char * );
void statrigfilter_logparm( void );
void statrigfilter_lookup( void );
void statrigfilter_status( unsigned char, short, char * );
int  statrigfilter_trg( STATRIG *trg );
int  statrigfilter_compare( const void *s1, const void *s2 );
int  comp_compare( const void *s1, const void *s2 );
int  statrigfilter_addsta( STATRIG *trg );

int main( int argc, char **argv )
{
   char         *msgbuf;           /* buffer for msgs from ring     */
   time_t          timeNow;          /* current time                  */
   time_t          timeLastBeat;     /* time last heartbeat was sent  */
   long          recsize;          /* size of retrieved message     */
   MSG_LOGO      reclogo;          /* logo of retrieved message     */
   MSG_LOGO      putlogo;          /* logo to use putting message into ring */
   STATRIG       trg;
   int           res, useit, i, nread;
   unsigned char seq;

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 1024, 1 );

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        logit( "e", "Usage: statrigfilter <configfile>\n" );
        exit( 0 );
   }

/* Read the configuration file(s)
 ********************************/
   statrigfilter_config( argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   statrigfilter_lookup();

/*  Set logit to LogSwitch read from configfile
 **********************************************/
   logit_init( argv[1], 0, 256, LogSwitch );
   logit( "" , "statrigfilter: Read command file <%s>\n", argv[1] );
   statrigfilter_logparm();

/* Check for different in/out rings if UseOriginalLogo is set
 ************************************************************/
   if( UseOriginalLogo  &&  (InRingKey==OutRingKey) ) 
   {
      logit ("e", "statrigfilter: InRing and OutRing must be different when"
                  "UseOriginalLogo is non-zero; exiting!\n");
      free( GetLogo );
      exit( -1 );
   }

/* Get our own process ID for restart purposes
 *********************************************/
   if( (MyPid = getpid()) == -1 )
   {
      logit ("e", "statrigfilter: Call to getpid failed. Exiting.\n");
      free( GetLogo );
      exit( -1 );
   }

/* Allocate the message input buffer 
 ***********************************/
  if ( !( msgbuf = (char *) malloc( (size_t)MaxMessageSize+1 ) ) )
  {
      logit( "et", 
             "statrigfilter: failed to allocate %d bytes"
             " for message buffer; exiting!\n", MaxMessageSize+1 );
      free( GetLogo );
      exit( -1 );
  }

/* Initialize outgoing logo
 **************************/
   putlogo.instid = InstId;
   putlogo.mod    = MyModId;

/* Attach to shared memory rings
 *******************************/
   tport_attach( &InRegion, InRingKey );
   logit( "", "statrigfilter: Attached to public memory region: %ld\n",
          InRingKey );
   tport_attach( &OutRegion, OutRingKey );
   logit( "", "statrigfilter: Attached to public memory region: %ld\n",
           OutRingKey );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartbeatInt - 1;

/* Flush the incoming transport ring on startup
 **********************************************/ 
   while( tport_copyfrom(&InRegion, GetLogo, nLogo,  &reclogo,
          &recsize, msgbuf, MaxMessageSize, &seq ) != GET_NONE );

/*----------------------- setup done; start main loop -------------------------*/

  while ( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid )
  {
     /* send statrigfilter's heartbeat
      ********************************/
        if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt )
        {
            timeLastBeat = timeNow;
            statrigfilter_status( TypeHeartBeat, 0, "" );
        }

     /* Get msg & check the return code from transport
      ************************************************/
        res = tport_copyfrom( &InRegion, GetLogo, nLogo, &reclogo, 
                              &recsize, msgbuf, MaxMessageSize, &seq );

        switch( res )
        {
        case GET_OK:      /* got a message, no errors or warnings         */
             break;

        case GET_NONE:    /* no messages of interest, check again later   */
             sleep_ew(100); /* milliseconds */
             continue;

        case GET_NOTRACK: /* got a msg, but can't tell if any were missed */
             sprintf( Text,
                     "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                      reclogo.instid, reclogo.mod, reclogo.type );
             statrigfilter_status( TypeError, ERR_NOTRACK, Text );
             break;

        case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
             sprintf( Text,
                     "Missed msg(s) from logo (i%u m%u t%u)",
                      reclogo.instid, reclogo.mod, reclogo.type );
             statrigfilter_status( TypeError, ERR_MISSLAP, Text );
             break;

        case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
             sprintf( Text,
                     "Saw sequence# gap for logo (i%u m%u t%u s%u)",
                      reclogo.instid, reclogo.mod, reclogo.type, seq );
             statrigfilter_status( TypeError, ERR_MISSGAP, Text );
             break;

       case GET_TOOBIG:  /* next message was too big, resize buffer      */
             sprintf( Text,
                     "Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%ld]",
                      recsize, reclogo.instid, reclogo.mod, reclogo.type,
                      MaxMessageSize );
             statrigfilter_status( TypeError, ERR_TOOBIG, Text );
             continue;

       default:         /* Unknown result                                */
             sprintf( Text, "Unknown tport_copyfrom result:%d", res );
             statrigfilter_status( TypeError, ERR_TOOBIG, Text );
             continue;
       }

    /* Decide if we should pass this message on or not
     *************************************************/
       msgbuf[recsize] = '\0'; /* Null terminate for ease of printing */
       useit           = 0;
       if( reclogo.type == TypeCarlStaTrigSCNL ) 
       {
          nread = sscanf( msgbuf, "%s %s %s %s %lf %lf %ld %lf", 
                          trg.site, trg.cmp, trg.net, trg.loc, 
                          &trg.ton, &trg.toff, &trg.seq, &trg.onEta );
          if( nread==8 ) useit = statrigfilter_trg( &trg );
       }
       else if( reclogo.type == TypeCarlStaTrig ) 
       {
          strcpy( trg.loc, LOC_NULL_STRING );
          nread = sscanf( msgbuf, "%s %s %s %lf %lf %ld %lf", 
                          trg.site, trg.cmp, trg.net, 
                          &trg.ton, &trg.toff, &trg.seq, &trg.onEta );
          if( nread==7 ) useit = statrigfilter_trg( &trg );
       }
      
       if( Debug ) {
          if( useit == TRUE ) logit("","Pass:%s\n", msgbuf);
          else                logit("","Stop:%s\n", msgbuf);
       }
       if( useit == FALSE ) continue;

       if( UseOriginalLogo ) {
          putlogo.instid = reclogo.instid;
          putlogo.mod    = reclogo.mod;
       }       
       putlogo.type = reclogo.type;

       if( tport_putmsg( &OutRegion, &putlogo, recsize, msgbuf ) != PUT_OK )
       {
          logit("et","statrigfilter: Error writing %d-byte msg to ring; "
                     "original logo (i%u m%u t%u)\n", recsize,
                      reclogo.instid, reclogo.mod, reclogo.type );
       }
   }

/*-----------------------------end of main loop-------------------------------*/

/* free allocated memory */
   free( GetLogo );
   free( msgbuf  );
   for ( i=0; i<nHist; i++ ) free( TrgHist[i].trg );
   free( TrgHist  );  

/* detach from shared memory */
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
           
/* write a termination msg to log file */
   logit( "t", "statrigfilter: Termination requested; exiting!\n" );
   fflush( stdout );
   return( 0 );
}

/******************************************************************************
 *  statrigfilter_config() processes command file(s) using kom.c functions;   *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 10        /* # of required commands you expect to process   */
void statrigfilter_config( char *configfile )
{
   char  init[ncommand];   /* init flags, one byte for each required command */
   int   nmiss;            /* number of required commands that were missed   */
   char *com;
   char *str;
   int   nfiles;
   int   success;
   int   i;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
               "statrigfilter: Error opening command file <%s>; exiting!\n",
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
                         "statrigfilter: Error opening command file <%s>; exiting!\n",
                          &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                if( LogSwitch<0 || LogSwitch>2 ) {
                   logit( "e",
                          "statrigfilter: Invalid <LogFile> value %d; "
                          "must = 0, 1 or 2; exiting!\n", LogSwitch );
                   exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("MyModuleId") ) {
                if( str=k_str() ) {
                   if( GetModId( str, &MyModId ) != 0 ) {
                      logit( "e",
                             "statrigfilter: Invalid module name <%s> "
                             "in <MyModuleId> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("InRing") ) {
                if( str=k_str() ) {
                   if( ( InRingKey = GetKey(str) ) == -1 ) {
                      logit( "e",
                             "statrigfilter: Invalid ring name <%s> "
                             "in <InRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("OutRing") ) {
                if( str=k_str() ) {
                   if( ( OutRingKey = GetKey(str) ) == -1 ) {
                      logit( "e",
                             "statrigfilter: Invalid ring name <%s> "
                             "in <OutRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[3] = 1;
            }

  /*4*/     else if( k_its("HeartbeatInt") ) {
                HeartbeatInt = k_long();
                init[4] = 1;
            }


         /* Enter installation & module to get messages from
          **************************************************/
  /*5*/     else if( k_its("GetLogo") ) {
                MSG_LOGO *tlogo = NULL;
                tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+2)*sizeof(MSG_LOGO) );
                if( tlogo == NULL )
                {
                   logit( "e", "statrigfilter: GetLogo: error reallocing"
                           " %d bytes; exiting!\n",
                           (nLogo+2)*sizeof(MSG_LOGO) );
                   exit( -1 );
                }
                GetLogo = tlogo;

                if( str=k_str() ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                              "statrigfilter: Invalid installation name <%s>"
                              " in <GetLogo> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                   if( str=k_str() ) {
                      if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                          logit( "e",
                                 "statrigfilter: Invalid module name <%s>"
                                 " in <GetLogo> cmd; exiting!\n", str );
                          exit( -1 );
                      }
                      GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                      if( GetType( "TYPE_CARLSTATRIG_SCNL", &GetLogo[nLogo].type ) != 0 ) {
                          logit( "e",
                                 "statrigfilter: Invalid msg type <TYPE_CARLSTATRIG_SCNL>" 
                                 "; exiting!\n" );
                          exit( -1 );
                      }
                      if( GetType( "TYPE_CARLSTATRIG", &GetLogo[nLogo+1].type ) != 0 ) {
                          logit( "e",
                                 "statrigfilter: Invalid msg type <TYPE_CARLSTATRIG>" 
                                 "; exiting!\n" );
                          exit( -1 );
                      }
                   }
                }
                nLogo  += 2;
                init[5] = 1;
            }

  /*6*/     else if( k_its("Debug") ) {
                Debug = k_int();
                init[6] = 1;
            }

  /*7*/     else if( k_its("TimeTolerance") ) {
                TimeTolerance = k_val();
                init[7] = 1;
            }

  /*8*/     else if( k_its("TriggerHistory") ) {
                nTriggerHistory = k_int();
                if( nTriggerHistory <= 0 ) 
                {
                   logit("e","statrigfilter: Invalid TriggerHistory value %d; "
                         "must be > 0; exiting!\n", nTriggerHistory );
                   exit( -1 );
                }
                init[8] = 1;
            }

  /*9*/    else if( k_its("OlderTrigAllowed") ) {
                OlderTrigAllowed = k_int();
                if( OlderTrigAllowed > 2  ||
                    OlderTrigAllowed < 0     ) 
                {
                   logit("e","statrigfilter: Invalid OlderTrigAllowed value %d "
                         "(valid=0,1,2); exiting!\n", OlderTrigAllowed );
                   exit( -1 );
                }
                init[9] = 1;
            }

  /*opt*/   else if( k_its("OlderTrigLimit") ) {
                int itmp = k_int();
                OlderTrigLimit = ABS(itmp);
            }

  /*opt*/   else if( k_its("MaxMessageSize") ) {
                MaxMessageSize = k_long();
            }

  /*opt*/   else if( k_its("UseOriginalLogo") )
            {
                UseOriginalLogo = k_int();
            }

  /*opt*/   else if( k_its("AllowComponent") )
            {
                COMPONENT *tmp = NULL;
                tmp = (COMPONENT *)realloc( AllowComp, 
                                           (nAllowComp+1)*sizeof(COMPONENT) );
                if( tmp == NULL ) {
                   logit( "e", "statrigfilter: AllowComponent: error reallocing"
                          " %d bytes; exiting!\n",
                          (nAllowComp+1)*sizeof(COMPONENT) );
                   exit( -1 );
                }
                AllowComp = tmp;

                if( str=k_str() ) {
                   int length = (int) strlen(str);
                   if( length<=0 || length>=TRACE2_CHAN_LEN ) {
                      logit( "e", "statrigfilter: AllowComponent: Invalid length"
                             " code <%s>; exiting!\n", str );
                      exit( -1 );
                   }
                   strcpy( AllowComp[nAllowComp].cmp, str );
                   nAllowComp++;
                }
            }

         /* Unknown command
          *****************/
            else {
                logit( "e", "statrigfilter: <%s> Unknown command in <%s>.\n",
                       com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                      "statrigfilter: Bad <%s> command in <%s>; exiting!\n",
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
       logit( "e", "statrigfilter: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "             );
       if ( !init[1] )  logit( "e", "<MyModuleId> "          );
       if ( !init[2] )  logit( "e", "<InRing> "              );
       if ( !init[3] )  logit( "e", "<OutRing> "             );
       if ( !init[4] )  logit( "e", "<HeartbeatInt> "        );
       if ( !init[5] )  logit( "e", "<GetLogo> "             );
       if ( !init[6] )  logit( "e", "<Debug> "               );
       if ( !init[7] )  logit( "e", "<TimeTolerance> "       );
       if ( !init[8] )  logit( "e", "<TriggerHistory> "      );
       if ( !init[9] )  logit( "e", "<OlderTrigAllowed> " );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

/* Check for one "required-optional" command
 *******************************************/
   if( OlderTrigAllowed == OLDER_TRIG_LIMIT  &&
       OlderTrigLimit   == -1 )
   {
      logit( "e","statrigfilter: no <OlderTrigLimit> command "
                 "(required when OlderTrigAllowed=1); exiting!\n" );
      exit( -1 );
   }

/* Sort the list of allowed components
 *************************************/
   qsort( AllowComp, nAllowComp, sizeof(COMPONENT), comp_compare );

   return;
}

/******************************************************************************
 *  statrigfilter_logparm( )   Log operating params                           *
 ******************************************************************************/
void statrigfilter_logparm( void )
{
   int i;
   logit("","MyModuleId:          %u\n",  MyModId );
   logit("","InRing key:          %ld\n", InRingKey );
   logit("","OutRing key:         %ld\n", OutRingKey );
   logit("","HeartbeatInt:        %ld sec\n", HeartbeatInt );
   logit("","LogFile:             %d\n",  LogSwitch );
   logit("","Debug:               %d\n",  Debug );
   logit("","MaxMessageSize:      %d bytes\n", MaxMessageSize );
   for(i=0;i<nLogo;i++)  logit("","GetLogo[%d]:         i%u m%u t%u\n", i,
                               GetLogo[i].instid, GetLogo[i].mod, GetLogo[i].type );
   logit("","TimeTolerance:       %.3lf sec\n", TimeTolerance );
   logit("","TriggerHistory:      %d triggers\n", nTriggerHistory );
   logit("","OlderTrigAllowed:    %d (allow: 0=none, 1=time-limited, 2=all)\n", 
                                     OlderTrigAllowed );
   if(OlderTrigLimit != -1) logit("","OlderTrigLimit:      %d sec\n", 
                                     OlderTrigLimit );
   logit("","UseOriginalLogo:     %d\n", UseOriginalLogo );

   if( nAllowComp == 0 ) {
         logit("","AllowComponent:     all\n" );
   } else {
      for( i=0; i<nAllowComp; i++ ) {
         logit("","AllowComponent %2d:  %s\n", i, AllowComp[i].cmp );
      }
   }
   return;
}


/******************************************************************************
 *  statrigfilter_lookup( )   Look up important info from earthworm tables    *
 ******************************************************************************/

void statrigfilter_lookup( void )
{

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit( "e",
             "statrigfilter: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e",
             "statrigfilter: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e",
             "statrigfilter: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CARLSTATRIG", &TypeCarlStaTrig ) != 0 ) {
      logit( "e",
             "statrigfilter: Invalid message type <TYPE_CARLSTATRIG>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CARLSTATRIG_SCNL", &TypeCarlStaTrigSCNL ) != 0 ) {
      logit( "e",
             "statrigfilter: Invalid message type <TYPE_CARLSTATRIG_SCNL>; exiting!\n" );
      exit( -1 );
   }

   return;
}


/******************************************************************************
 * statrigfilter_status() builds a heartbeat or error message & puts it into  *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void statrigfilter_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t      t;

/* Build the message
 *******************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "statrigfilter: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","statrigfilter:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","statrigfilter:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

/******************************************************************************
 *  statrigfilter_trg()  Look at the new trigger and decide if it should pass *
 ******************************************************************************/
int statrigfilter_trg( STATRIG *trg )
{
   COMPONENT   cmpkey;
   COMPONENT  *cmpmatch = (COMPONENT *)NULL;
   TRGHISTORY  key;
   TRGHISTORY *sta = (TRGHISTORY *)NULL;
   STATRIG    *last;
   STATRIG    *old;
   double      dt  = 0;
   int         dwt = 0;
   int         duplicate = 0; 

/* Check component code to see if it's allowed 
 *********************************************/
   if( nAllowComp ) {
      strcpy( cmpkey.cmp, trg->cmp );
      cmpmatch = (COMPONENT *)bsearch( &cmpkey, AllowComp, nAllowComp, 
                                       sizeof(COMPONENT), comp_compare );
      if( cmpmatch == (COMPONENT *)NULL ) {
         if( Debug ) logit("","CMP ");
         return FALSE;
      }
   }

/* Find station in our list
 **************************/
   if( nHist ) {
      strcpy( key.site, trg->site );
      strcpy( key.net,  trg->net  );
      sta = (TRGHISTORY *)bsearch( &key, TrgHist, nHist, sizeof(TRGHISTORY), 
                                   statrigfilter_compare );
   }

/* New station; add to list, pass the trigger
 ********************************************/
   if( sta == (TRGHISTORY *)NULL ) {
      statrigfilter_addsta( trg );
      return TRUE;
   }

/* Previously-seen station; trigger-off.
   Pass only if it exactly matches a previously-passed trigger-on.   
 *****************************************************************/
   if( sta->npass > (unsigned long)sta->ntrg ) last = sta->trg + sta->ntrg;
   else                         last = sta->trg + sta->npass;

   if( trg->toff != 0 )
   {
      if( Debug ) logit("","OFF ");           

      for( old=sta->trg; old<last; old++ )            /* for each trigger in history   */
      {
      /* We already know that site/net match; check everything else */
         if( strcmp( old->cmp, trg->cmp ) != 0 ) continue;  /* component doesn't match */
         if( strcmp( old->loc, trg->loc ) != 0 ) continue;  /* location  doesn't match */
         if( old->toff  != 0.0 )        continue;      /* old trigger was already off! */
         if( old->ton   != trg->ton )   continue;      /* trigger-on doesn't match     */
         if( old->seq   != trg->seq )   continue;      /* sequence# doesn't match      */
         if( old->onEta != trg->onEta ) continue;      /* onEta doesn't match          */
      /* Everything matches; store toff and pass it! */
         old->toff = trg->toff;
         return TRUE;             
      } /*end for each trigger in history*/

      return FALSE;   /* didn't find an exact match; reject it */
   }

/* Previously-seen station; trigger-on.
   Reject if trigger is a duplicate of any previously-passed triggers. 
 *********************************************************************/
   for( old=sta->trg; old<last; old++ )            /* for each trigger in history */
   {
      dt = ABS( trg->ton - old->ton );             /* Test trigger-on time;       */
      if( dt <= TimeTolerance )  {                 /* found a duplicate time!     */
         if( Debug ) logit("","DUP "); 
         return FALSE;                             /* not passing any duplicates! */
      }
   } /*end for each trigger in history*/

/* Previously-seen station; reject older triggers if necessary
 *************************************************************/
   dt = sta->tyoung - trg->ton;                 /* positive if current trigger is old */

   if( OlderTrigAllowed != OLDER_TRIG_ALL  &&   /* If older triggers are restricted   */
       dt > 0.0   )                             /* and it IS an older trigger         */
   {
      if( Debug ) logit("","OLD ");

   /* Reject all older triggers: */
      if( OlderTrigAllowed == OLDER_TRIG_NONE ) return FALSE;

   /* Or reject triggers which are older than the limit: */
      else if( dt > (double)OlderTrigLimit )    return FALSE;
   }

/* Passed all the tests; OK to ship this trigger! 
   Store time of youngest passed trigger.
   Replace first-passed trigger with current one;
 ************************************************/
   if( trg->ton > sta->tyoung ) sta->tyoung = trg->ton;
   old = &(sta->trg[sta->npass%sta->ntrg]);
   memcpy( old, trg, sizeof(STATRIG) ); 
   sta->npass++;

   return TRUE;
}

/******************************************************************************
 *  statrigfilter_compare()  This function is passed to qsort() & bsearch()   *
 *     so we can sort the trigger list by site & net codes, and then look up  *
 *     a station efficiently in the list.                                     *
 ******************************************************************************/
int statrigfilter_compare( const void *s1, const void *s2 )
{
   int rc;
   TRGHISTORY *t1 = (TRGHISTORY *) s1;
   TRGHISTORY *t2 = (TRGHISTORY *) s2;

   rc = strcmp( t1->site, t2->site );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->net,  t2->net );
   return rc;
}

/******************************************************************************
 *  comp_compare()  This function is passed to qsort() & bsearch() so we can  *
 *     sort the component list, and then look up a component efficiently.     *
 ******************************************************************************/
int comp_compare( const void *s1, const void *s2 )
{
   COMPONENT *t1 = (COMPONENT *) s1;
   COMPONENT *t2 = (COMPONENT *) s2;

   return( strcmp( t1->cmp, t2->cmp ) );
}


/******************************************************************************
 *  statrigfilter_addsta()  A new station has been identified by a site/net   *
 *     code pair.  Add it to the list of triggers we're tracking.             * 
 ******************************************************************************/
int statrigfilter_addsta( STATRIG *trg )
{
   TRGHISTORY *tlist = (TRGHISTORY *)NULL;
   STATRIG  *ttrg  = (STATRIG *)NULL;

/* Allocate space for tracking one more station 
 **********************************************/
   tlist = (TRGHISTORY *)realloc( TrgHist, (nHist+1)*sizeof(TRGHISTORY) );
   if( tlist == (TRGHISTORY *)NULL )
   {
      logit( "e", "statrigfilter_addsta: error reallocing TrgHist for"
             " %d stas\n", nHist+1  );
      return EW_FAILURE;
   }
   TrgHist = tlist;

/* Allocate space for the actual list of passed triggers
 *******************************************************/
   ttrg = (STATRIG *)calloc( (size_t)nTriggerHistory, sizeof(STATRIG) );
   if( ttrg == (STATRIG *)NULL )
   {
      logit( "e", "statrigfilter_addsta: error callocing %d trigger list for"
             " sta %d \n", nTriggerHistory, nHist+1  );
      return EW_FAILURE;
   }

/* Load new station in last slot of TrgHist
 ******************************************/
   tlist = &TrgHist[nHist];                 /* now point to last slot in TrgHist    */
   strcpy( tlist->site, trg->site );        /* copy info from this trigger          */
   strcpy( tlist->net,  trg->net  );
   tlist->tyoung = trg->ton;                /* youngest trigger passed so far       */
   tlist->npass  = 1;                       /* automatically pass this 1st trigger  */
   tlist->ntrg   = nTriggerHistory;         /* keep track of max size of list       */
   tlist->trg    = ttrg;                    /* store pointer to passed-trigger list */
   memcpy( &(tlist->trg[0]), trg, sizeof(STATRIG) );  /* copy current trg to list */
   nHist++;

/* Re-sort entire history list
 *****************************/
   qsort( TrgHist, nHist, sizeof(TRGHISTORY), statrigfilter_compare );

   logit("et","statrigfilter_addsta: %s.%s added; tracking %d stas\n",
          trg->site, trg->net, nHist );
  
   return EW_SUCCESS;
}
