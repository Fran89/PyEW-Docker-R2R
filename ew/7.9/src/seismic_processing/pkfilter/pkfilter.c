/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pkfilter.c 2710 2007-02-26 13:44:40Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.5  2007/02/23 17:06:14  paulf
 *     fixed long warning for time_t
 *
 *     Revision 1.4  2005/07/29 23:22:16  dietz
 *     Added optional "AllowComponent" command to enable the user to filter
 *     picks/codas based on component code.  If no "AllowComponent" commands
 *     are used, ALL components will be eligible to pass thru the filter.
 *
 *     Revision 1.3  2004/04/30 22:52:34  dietz
 *     fixed bugs in parsing new _SCNL msg types
 *
 *     Revision 1.2  2004/04/29 22:01:07  dietz
 *     added capability to process TYPE_PICK_SCNL and TYPE_CODA_SCNL msgs
 *
 *     Revision 1.1  2004/04/22 18:01:56  dietz
 *     Moved pkfilter source from Contrib/Menlo to the earthworm orthodoxy
 *
 *
 *
 */

/*
 * pkfilter.c
 *
 * Reads picks/codas one transport ring and writes them to another 
 * ring.  Filters out duplicate picks from the same station that
 * are close in time (where "close" is configurable).
 * Does not alter the contents of the message in any way.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <kom.h>
#include "rdpickcoda.h"

#define ABS(X) (((X) >= 0) ? (X) : -(X))

typedef union {
   char cmp[TRACE2_CHAN_LEN];
   int  i;
} COMPONENT;

/* Functions in this source file
 *******************************/
void pkfilter_config( char * );
void pkfilter_logparm( void );
void pkfilter_lookup( void );
void pkfilter_status( unsigned char, short, char * );
int  pkfilter_pick( EWPICK *pk );
int  pkfilter_coda( EWCODA *cd );
int  pkfilter_compare( const void *s1, const void *s2 );
int  comp_compare( const void *s1, const void *s2 );
int  pkfilter_addsta( EWPICK *pk );

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
static double  PickTolerance;      /* # of seconds +- picks match       */
static int     DuplicateOnQuality; /* This means pass on higher quality */ 
                                   /*  picks if you receive one after   */
                                   /*  you've transfered a lower quality*/
                                   /*  pick already                     */
static int     QualDiffAllowed;    /* This is the diff between quality  */
                                   /*   of old pick and new pick.       */
                                   /* 0 means any pick greater than current pick */
                                   /* 1 means a pick of 1 replaces a pick of 3   */
                                   /* 2 means a pick of 0 replaces a pick of 3   */
                                   /* 3 is invalid because it is equivalent to   */
                                   /* turning off the duplicate on quality test  */
static int     UseOriginalLogo=0;  /* 0=use pkfilter's own logo on output msgs   */
                                   /* non-zero=use original logos on output      */
                                   /*   NOTE: this requires that output go to    */
                                   /*   different transport ring than input      */
static int     nPickHistory;       /* Track this many of the most recent picks   */
                                   /*   that have passed the filter for each sta */
static int     OlderPickAllowed;   /* 0=reject any pick whose timestamp is more  */
                                   /*   than PickTolerance sec earlier than the  */
                                   /*   youngest pick which has been passed.     */
                                   /* 1=accept a pick whose timestamp is         */
                                   /*   earlier than the youngest passed pick,   */
                                   /*   but place a limit on how old it can be.  */
                                   /* 2=accept any pick whose timestamp is       */
                                   /*   earlier than the youngest passed pick.   */
static int     OlderPickLimit=-1;  /* Accept an pick whose timestamp is between  */
                                   /*   PickTolerance and OlderPickLimit sec     */
                                   /*   earlier than the youngest passed pick.   */
                                   /*   Required only when OlderPickAllowed=1.   */
static int     CodaFilter;         /* 0=allow no codas to pass                   */
                                   /* 1=allow only matching codas thru (those    */
                                   /*   with site/net,instid,modid,seq# that     */
                                   /*   match a passed pick will pass)           */
                                   /* 2=allow all codas to pass                  */
static COMPONENT *AllowComp=NULL;  /* list of component codes allowed to pass    */
static short      nAllowComp = 0;  /* # components in allow-list. If nAllowComp  */
                                   /*   is zero, it means all components are     */ 
                                   /*   allowed (slightly counter-intuitive).    */

/* Handy pkfilter definitions
 ****************************/
#define  OLDER_PICK_NONE  0
#define  OLDER_PICK_LIMIT 1
#define  OLDER_PICK_ALL   2
#define  CODA_ALLOW_NONE  0
#define  CODA_ALLOW_MATCH 1
#define  CODA_ALLOW_ALL   2

/* List of most-recently shipped pick for each site/net code
 ***********************************************************/
typedef struct _PKHISTORY {
   char          site[6];          /* site code we're tracking            */
   char          net[3];           /* net code we're tracking             */
   double        tyoung;           /* time of youngest pick that's passed */
   unsigned long npass;            /* cumulative number of passed picks   */
   int           npk;              /* max # of picks in the list          */
   EWPICK       *pk;               /* dynamically allocated list of last- */
                                   /*   shipped pick(s) for this station  */
} PKHISTORY;

static PKHISTORY *PkHist;          /* dynamically allocated list of last- */
                                   /*   shipped pick(s) per site/net code */
static int     nHist = 0;          /* number of stations in history list  */
  

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input   */
static long          OutRingKey;    /* key of transport ring for output  */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypePick2K;
static unsigned char TypeCoda2K;
static unsigned char TypePickSCNL;
static unsigned char TypeCodaSCNL;

/* Error messages used by pkfilter
 *********************************/
#define  ERR_MISSGAP       0   /* sequence gap in transport ring         */
#define  ERR_MISSLAP       1   /* missed messages in transport ring      */
#define  ERR_TOOBIG        2   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       3   /* msg retreived; tracking limit exceeded */
static char  Text[150];        /* string for log/error messages          */


int main( int argc, char **argv )
{
   char         *msgbuf;           /* buffer for msgs from ring     */
   time_t          timeNow;          /* current time                  */
   time_t          timeLastBeat;     /* time last heartbeat was sent  */
   long          recsize;          /* size of retrieved message     */
   MSG_LOGO      reclogo;          /* logo of retrieved message     */
   MSG_LOGO      putlogo;          /* logo to use putting message into ring */
   EWPICK        pk;
   EWCODA        cd;
   int           res, useit, i, rc;
   unsigned char seq;

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 1024, 1 );

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        logit( "e", "Usage: pkfilter <configfile>\n" );
        exit( 0 );
   }

/* Read the configuration file(s)
 ********************************/
   pkfilter_config( argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   pkfilter_lookup();

/*  Set logit to LogSwitch read from configfile
 **********************************************/
   logit_init( argv[1], 0, 256, LogSwitch );
   logit( "" , "pkfilter: Read command file <%s>\n", argv[1] );
   pkfilter_logparm();

/* Check for different in/out rings if UseOriginalLogo is set
 ************************************************************/
   if( UseOriginalLogo  &&  (InRingKey==OutRingKey) ) 
   {
      logit ("e", "pkfilter: InRing and OutRing must be different when"
                  "UseOriginalLogo is non-zero; exiting!\n");
      free( GetLogo );
      exit( -1 );
   }

/* Get our own process ID for restart purposes
 *********************************************/
   if( (MyPid = getpid()) == -1 )
   {
      logit ("e", "pkfilter: Call to getpid failed. Exiting.\n");
      free( GetLogo );
      exit( -1 );
   }

/* Allocate the message input buffer 
 ***********************************/
  if ( !( msgbuf = (char *) malloc( (size_t)MaxMessageSize+1 ) ) )
  {
      logit( "et", 
             "pkfilter: failed to allocate %d bytes"
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
   logit( "", "pkfilter: Attached to public memory region: %ld\n",
          InRingKey );
   tport_attach( &OutRegion, OutRingKey );
   logit( "", "pkfilter: Attached to public memory region: %ld\n",
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
     /* send pkfilter's heartbeat
      ***************************/
        if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt )
        {
            timeLastBeat = timeNow;
            pkfilter_status( TypeHeartBeat, 0, "" );
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
             pkfilter_status( TypeError, ERR_NOTRACK, Text );
             break;

        case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
             sprintf( Text,
                     "Missed msg(s) from logo (i%u m%u t%u)",
                      reclogo.instid, reclogo.mod, reclogo.type );
             pkfilter_status( TypeError, ERR_MISSLAP, Text );
             break;

        case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
             sprintf( Text,
                     "Saw sequence# gap for logo (i%u m%u t%u s%u)",
                      reclogo.instid, reclogo.mod, reclogo.type, seq );
             pkfilter_status( TypeError, ERR_MISSGAP, Text );
             break;

       case GET_TOOBIG:  /* next message was too big, resize buffer      */
             sprintf( Text,
                     "Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%ld]",
                      recsize, reclogo.instid, reclogo.mod, reclogo.type,
                      MaxMessageSize );
             pkfilter_status( TypeError, ERR_TOOBIG, Text );
             continue;

       default:         /* Unknown result                                */
             sprintf( Text, "Unknown tport_copyfrom result:%d", res );
             pkfilter_status( TypeError, ERR_TOOBIG, Text );
             continue;
       }

    /* Decide if we should pass this message on or not
     *************************************************/
       msgbuf[recsize] = '\0'; /* Null terminate for ease of printing */
       useit           = 0;
       if( reclogo.type == TypePickSCNL ) 
       {
          rc = rd_pick_scnl( msgbuf, recsize, &pk );
          if( rc==EW_SUCCESS ) useit = pkfilter_pick( &pk );
       }
       else if( reclogo.type == TypeCodaSCNL ) 
       {
          rc = rd_coda_scnl( msgbuf, recsize, &cd );
          if( rc==EW_SUCCESS ) useit = pkfilter_coda( &cd );
       }
       else if( reclogo.type == TypePick2K ) 
       {
          rc = rd_pick2k( msgbuf, recsize, &pk );
          if( rc==EW_SUCCESS ) useit = pkfilter_pick( &pk );
       }
       else if( reclogo.type == TypeCoda2K ) 
       {
          rc = rd_coda2k( msgbuf, recsize, &cd );
          if( rc==EW_SUCCESS ) useit = pkfilter_coda( &cd );
       }
      
       if( Debug ) {
          if( useit == TRUE ) logit("","Pass:%s", msgbuf);
          else                logit("","Stop:%s", msgbuf);
       }
       if( useit == FALSE ) continue;

       if( UseOriginalLogo ) {
          putlogo.instid = reclogo.instid;
          putlogo.mod    = reclogo.mod;
       }       
       putlogo.type = reclogo.type;

       if( tport_putmsg( &OutRegion, &putlogo, recsize, msgbuf ) != PUT_OK )
       {
          logit("et","pkfilter: Error writing %d-byte msg to ring; "
                     "original logo (i%u m%u t%u)\n", recsize,
                      reclogo.instid, reclogo.mod, reclogo.type );
       }
   }

/*-----------------------------end of main loop-------------------------------*/

/* free allocated memory */
   free( GetLogo );
   free( msgbuf  );
   for ( i=0; i<nHist; i++ ) free( PkHist[i].pk );
   free( PkHist  );  

/* detach from shared memory */
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
           
/* write a termination msg to log file */
   logit( "t", "pkfilter: Termination requested; exiting!\n" );
   fflush( stdout );
   return( 0 );
}

/******************************************************************************
 *  pkfilter_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 13        /* # of required commands you expect to process   */
void pkfilter_config( char *configfile )
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
               "pkfilter: Error opening command file <%s>; exiting!\n",
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
                         "pkfilter: Error opening command file <%s>; exiting!\n",
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
                          "pkfilter: Invalid <LogFile> value %d; "
                          "must = 0, 1 or 2; exiting!\n", LogSwitch );
                   exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("MyModuleId") ) {
                if( str=k_str() ) {
                   if( GetModId( str, &MyModId ) != 0 ) {
                      logit( "e",
                             "pkfilter: Invalid module name <%s> "
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
                             "pkfilter: Invalid ring name <%s> "
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
                             "pkfilter: Invalid ring name <%s> "
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
                tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+4)*sizeof(MSG_LOGO) );
                if( tlogo == NULL )
                {
                   logit( "e", "pkfilter: GetLogo: error reallocing"
                           " %d bytes; exiting!\n",
                           (nLogo+2)*sizeof(MSG_LOGO) );
                   exit( -1 );
                }
                GetLogo = tlogo;

                if( str=k_str() ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                              "pkfilter: Invalid installation name <%s>"
                              " in <GetLogo> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                   GetLogo[nLogo+2].instid = GetLogo[nLogo].instid;
                   GetLogo[nLogo+3].instid = GetLogo[nLogo].instid;
                   if( str=k_str() ) {
                      if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                          logit( "e",
                                 "pkfilter: Invalid module name <%s>"
                                 " in <GetLogo> cmd; exiting!\n", str );
                          exit( -1 );
                      }
                      GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                      GetLogo[nLogo+2].mod = GetLogo[nLogo].mod;
                      GetLogo[nLogo+3].mod = GetLogo[nLogo].mod;
                      if( GetType( "TYPE_PICK_SCNL", &GetLogo[nLogo].type ) != 0 ) {
                          logit( "e",
                                 "pkfilter: Invalid message type <TYPE_PICK_SCNL>" 
                                 "; exiting!\n" );
                          exit( -1 );
                      }
                      if( GetType( "TYPE_CODA_SCNL", &GetLogo[nLogo+1].type ) != 0 ) {
                          logit( "e",
                                 "pkfilter: Invalid message type <TYPE_CODA_SCNL>" 
                                 "; exiting!\n" );
                          exit( -1 );
                      }
                      if( GetType( "TYPE_PICK2K", &GetLogo[nLogo+2].type ) != 0 ) {
                          logit( "e",
                                 "pkfilter: Invalid message type <TYPE_PICK2K>" 
                                 "; exiting!\n" );
                          exit( -1 );
                      }
                      if( GetType( "TYPE_CODA2K", &GetLogo[nLogo+3].type ) != 0 ) {
                          logit( "e",
                                 "pkfilter: Invalid message type <TYPE_CODA2K>" 
                                 "; exiting!\n" );
                          exit( -1 );
                      }
                   }
                }
                nLogo+=4;
                init[5] = 1;
            }

  /*6*/     else if( k_its("Debug") ) {
                Debug = k_int();
                init[6] = 1;
            }

  /*7*/     else if( k_its("PickTolerance") ) {
                PickTolerance = k_val();
                init[7] = 1;
            }

  /*8*/     else if( k_its("DuplicateOnQuality") ) {
                DuplicateOnQuality = k_int();
                init[8] = 1;
            }

  /*9*/     else if( k_its("QualDiffAllowed") ) {
                QualDiffAllowed = k_int();
                if( QualDiffAllowed > 2  ||
                    QualDiffAllowed < 0     ) 
                {
                   logit("e","pkfilter: Invalid QualDiffAllowed value %d "
                         "(valid 0,1,2); set to 0\n", QualDiffAllowed );
                   QualDiffAllowed = 0;
                }
                init[9] = 1;
            }

  /*10*/    else if( k_its("PickHistory") ) {
                nPickHistory = k_int();
                if( nPickHistory <= 0 ) 
                {
                   logit("e","pkfilter: Invalid PickHistory value %d; "
                         "must be > 0; exiting!\n", nPickHistory );
                   exit( -1 );
                }
                init[10] = 1;
            }

  /*11*/    else if( k_its("OlderPickAllowed") ) {
                OlderPickAllowed = k_int();
                if( OlderPickAllowed > 2  ||
                    OlderPickAllowed < 0     ) 
                {
                   logit("e","pkfilter: Invalid OlderPickAllowed value %d "
                         "(valid=0,1,2); exiting!\n", OlderPickAllowed );
                   exit( -1 );
                }
                init[11] = 1;
            }

  /*12*/    else if( k_its("CodaFilter") ) {
                CodaFilter = k_int();
                if( CodaFilter > 2  ||
                    CodaFilter < 0     ) 
                {
                   logit("e","pkfilter: Invalid CodaFilter value %d "
                         "(valid=0,1,2); exiting!\n", CodaFilter );
                   exit( -1 );
                }
                init[12] = 1;
            }

  /*opt*/   else if( k_its("OlderPickLimit") ) {
                int itmp = k_int();
                OlderPickLimit = ABS(itmp);
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
                   logit( "e", "pkfilter: AllowComponent: error reallocing"
                          " %d bytes; exiting!\n",
                          (nAllowComp+1)*sizeof(COMPONENT) );
                   exit( -1 );
                }
                AllowComp = tmp;

                if( str=k_str() ) {
                   int length = (int) strlen(str);
                   if( length<=0 || length>=TRACE2_CHAN_LEN ) {
                      logit( "e", "pkfilter: AllowComponent: Invalid length"
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
                logit( "e", "pkfilter: <%s> Unknown command in <%s>.\n",
                       com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                      "pkfilter: Bad <%s> command in <%s>; exiting!\n",
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
       logit( "e", "pkfilter: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "            );
       if ( !init[1] )  logit( "e", "<MyModuleId> "         );
       if ( !init[2] )  logit( "e", "<InRing> "             );
       if ( !init[3] )  logit( "e", "<OutRing> "            );
       if ( !init[4] )  logit( "e", "<HeartbeatInt> "       );
       if ( !init[5] )  logit( "e", "<GetLogo> "            );
       if ( !init[6] )  logit( "e", "<Debug> "              );
       if ( !init[7] )  logit( "e", "<PickTolerance> "      );
       if ( !init[8] )  logit( "e", "<DuplicateOnQuality> " );
       if ( !init[9] )  logit( "e", "<QualDiffAllowed> "    );
       if ( !init[10] ) logit( "e", "<PickHistory> "        );
       if ( !init[11] ) logit( "e", "<OlderPickAllowed> "   );
       if ( !init[12] ) logit( "e", "<CodaFilter> "         );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

/* Check for one "required-optional" command
 *******************************************/
   if( OlderPickAllowed == OLDER_PICK_LIMIT  &&
       OlderPickLimit   == -1 )
   {
      logit( "e","pkfilter: no <OlderPickLimit> command "
                 "(required when OlderPickAllowed=1); exiting!\n" );
      exit( -1 );
   }

/* Sort the list of allowed components
 *************************************/
   qsort( AllowComp, nAllowComp, sizeof(COMPONENT), comp_compare );

   return;
}

/******************************************************************************
 *  pkfilter_logparm( )   Log operating params                                *
 ******************************************************************************/
void pkfilter_logparm( void )
{
   int i;
   logit("","MyModuleId:         %u\n",  MyModId );
   logit("","InRing key:         %ld\n", InRingKey );
   logit("","OutRing key:        %ld\n", OutRingKey );
   logit("","HeartbeatInt:       %ld sec\n", HeartbeatInt );
   logit("","LogFile:            %d\n",  LogSwitch );
   logit("","Debug:              %d\n",  Debug );
   logit("","MaxMessageSize:     %d bytes\n", MaxMessageSize );
   for(i=0;i<nLogo;i++)  logit("","GetLogo[%d]:         i%u m%u t%u\n", i,
                               GetLogo[i].instid, GetLogo[i].mod, GetLogo[i].type );
   logit("","PickTolerance:      %.3lf sec\n", PickTolerance );
   logit("","PickHistory:        %d picks\n", nPickHistory );
   logit("","DuplicateOnQuality: %d\n", DuplicateOnQuality );
   logit("","QualDiffAllowed:    %d\n", QualDiffAllowed );
   logit("","OlderPickAllowed:   %d (allow: 0=none, 1=time-limited, 2=all)\n", 
                                     OlderPickAllowed );
   if(OlderPickLimit != -1) logit("","OlderPickLimit:     %d sec\n", 
                                     OlderPickLimit );
   logit("","CodaFilter:         %d (allow: 0=none, 1=matching, 2=all)\n", 
                                     CodaFilter );
   logit("","UseOriginalLogo:    %d\n", UseOriginalLogo );

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
 *  pkfilter_lookup( )   Look up important info from earthworm tables         *
 ******************************************************************************/

void pkfilter_lookup( void )
{

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit( "e",
             "pkfilter: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e",
             "pkfilter: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e",
             "pkfilter: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PICK2K", &TypePick2K ) != 0 ) {
      logit( "e",
             "pkfilter: Invalid message type <TYPE_PICK2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CODA2K", &TypeCoda2K ) != 0 ) {
      logit( "e",
             "pkfilter: Invalid message type <TYPE_CODA2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PICK_SCNL", &TypePickSCNL ) != 0 ) {
      logit( "e",
             "pkfilter: Invalid message type <TYPE_PICK_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CODA_SCNL", &TypeCodaSCNL ) != 0 ) {
      logit( "e",
             "pkfilter: Invalid message type <TYPE_CODA_SCNL>; exiting!\n" );
      exit( -1 );
   }

   return;
}


/******************************************************************************
 * pkfilter_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void pkfilter_status( unsigned char type, short ierr, char *note )
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
        logit( "et", "pkfilter: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","pkfilter:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","pkfilter:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

/******************************************************************************
 *  pkfilter_pick()  Look at the new pick and decide if we should pass it     *
 ******************************************************************************/
int pkfilter_pick( EWPICK *pk )
{
   COMPONENT  cmpkey;
   COMPONENT *cmpmatch = (COMPONENT *)NULL;
   PKHISTORY  key;
   PKHISTORY *sta = (PKHISTORY *)NULL;
   EWPICK    *last;
   EWPICK    *old;
   double     dt  = 0;
   int        dwt = 0;
   int        duplicate = 0; 

/* Check component code to see if it's allowed 
 *********************************************/
   if( nAllowComp ) {
      strcpy( cmpkey.cmp, pk->comp );
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
      strcpy( key.site, pk->site );
      strcpy( key.net,  pk->net  );
      sta = (PKHISTORY *)bsearch( &key, PkHist, nHist, sizeof(PKHISTORY), 
                                  pkfilter_compare );
   }

/* New station; add to list, pass the pick
 *****************************************/
   if( sta == (PKHISTORY *)NULL ) {
      pkfilter_addsta( pk );
      return TRUE;
   }

/* Previously-seen station; see if pick is a duplicate of any 
   previously-passed picks.  Don't stop after first time-match
   is found if "DuplicateOnQuality" is set, because a second
   higher-quality pick could've also made it thru and might
   disqualify the current pick.
 *************************************************************/
   if( sta->npass > sta->npk ) last = sta->pk + sta->npk;
   else                        last = sta->pk + sta->npass;
   for( old=sta->pk; old<last; old++ )             /* for each pick in history   */
   {
   /* Test pick time */
      dt = ABS( pk->tpick - old->tpick );          
      if( dt <= PickTolerance )  {                 /* found a duplicate time!    */
         duplicate = 1;
         if( Debug ) logit("","DUP ");
         if( !DuplicateOnQuality )   return FALSE; /* not passing ANY duplicates */   
                                       
      /* Found DUP; test pick quality (0-3, with 0 being best): */
         dwt = old->wt - pk->wt - 1;               /* >= 0 if new pick is better */
         if( dwt < QualDiffAllowed ) return FALSE; /* quality not high enough to */
                                                   /* override previous pick     */
      }
   } /*end for each pick in history*/

/* Previously-seen station; reject older picks if necessary
 **********************************************************/
   dt = sta->tyoung - pk->tpick;             /* positive if current pick is old */

   if( OlderPickAllowed != OLDER_PICK_ALL  &&  /* If older picks are restricted */
      !duplicate                           &&  /* and it isn't a duplicate      */
       dt > 0.0   )                            /* and it IS an older pick       */
   {
      if( Debug ) logit("","OLD ");

   /* Reject all older picks: */
      if( OlderPickAllowed == OLDER_PICK_NONE ) return FALSE;

   /* Or reject picks which are older than the limit: */
      else if( dt > (double)OlderPickLimit )    return FALSE;
   }

/* Passed all the tests; OK to ship this pick! 
   Store time of youngest passed pick.
   Replace first-passed pick with current one;
 *********************************************/
   if( pk->tpick > sta->tyoung ) sta->tyoung = pk->tpick;
   old = &(sta->pk[sta->npass%sta->npk]);
   memcpy( old, pk, sizeof(EWPICK) ); 
   sta->npass++;

   return TRUE;
}


/******************************************************************************
 *  pkfilter_coda() Test new coda msg to see if it should be passed.          *
 ******************************************************************************/
int pkfilter_coda( EWCODA *cd )
{
   COMPONENT  cmpkey;
   COMPONENT *cmpmatch = (COMPONENT *)NULL;
   PKHISTORY  key;
   PKHISTORY *sta = (PKHISTORY *)NULL;
   EWPICK    *old;
   EWPICK    *last;

/* Check the easy cases first
 ****************************/
   if( CodaFilter == CODA_ALLOW_NONE ) return FALSE;
   if( CodaFilter == CODA_ALLOW_ALL  ) return TRUE;

/* Must be CODA_ALLOW_MATCH; check component code to see if it's allowed 
 ***********************************************************************/
   if( nAllowComp ) {
      strcpy( cmpkey.cmp, cd->comp );
      cmpmatch = (COMPONENT *)bsearch( &cmpkey, AllowComp, nAllowComp, 
                                       sizeof(COMPONENT), comp_compare );
      if( cmpmatch == (COMPONENT *)NULL ) {
         if( Debug ) logit("","CMP ");
         return FALSE;
      }
   }

/* Must be allowed component; find sta in list
 *********************************************/
   strcpy( key.site, cd->site );
   strcpy( key.net,  cd->net  );
   if( nHist ) {
      sta = (PKHISTORY *)bsearch( &key, PkHist, nHist, sizeof(PKHISTORY), 
                                  pkfilter_compare );
   }

/* Station not in pick history list; reject this coda 
 ****************************************************/
   if( sta == (PKHISTORY *)NULL ) return FALSE; 

/* Station in list; see if this coda matches a passed pick 
 *********************************************************/
   if( sta->npass > sta->npk ) last = sta->pk + sta->npk;
   else                        last = sta->pk + sta->npass;
   for( old=sta->pk; old<last; old++ )
   {
      if( old->modid  == cd->modid  &&
          old->instid == cd->instid &&
          old->seq    == cd->seq       ) return TRUE; 
   }

   return FALSE;
}


/******************************************************************************
 *  pkfilter_compare()  This function is passed to qsort() & bsearch() so     *
 *     we can sort the pick list by site & net codes, and then look up a      *
 *     station efficiently in the list.                                       *
 ******************************************************************************/
int pkfilter_compare( const void *s1, const void *s2 )
{
   int rc;
   PKHISTORY *t1 = (PKHISTORY *) s1;
   PKHISTORY *t2 = (PKHISTORY *) s2;

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
 *  pkfilter_addsta()  A new station has been identified by a site/network    *
 *     code pair.  Add it to the list of picks we're tracking.                * 
 ******************************************************************************/
int pkfilter_addsta( EWPICK *pk )
{
   PKHISTORY *tlist = (PKHISTORY *)NULL;
   EWPICK    *tpk   = (EWPICK *)NULL;

/* Allocate space for tracking one more station 
 **********************************************/
   tlist = (PKHISTORY *)realloc( PkHist, (nHist+1)*sizeof(PKHISTORY) );
   if( tlist == (PKHISTORY *)NULL )
   {
      logit( "e", "pkfilter_addsta: error reallocing PkHist for"
             " %d stas\n", nHist+1  );
      return EW_FAILURE;
   }
   PkHist = tlist;

/* Allocate space for the actual list of passed picks
 ****************************************************/
   tpk = (EWPICK *)calloc( (size_t)nPickHistory, sizeof(EWPICK) );
   if( tpk == (EWPICK *)NULL )
   {
      logit( "e", "pkfilter_addsta: error callocing %d pick list for"
             " sta %d \n", nPickHistory, nHist+1  );
      return EW_FAILURE;
   }

/* Load new station in last slot of PkHist
 *****************************************/
   tlist = &PkHist[nHist];               /* now point to last slot in PkHist  */
   strcpy( tlist->site, pk->site );      /* copy info from this pick          */
   strcpy( tlist->net,  pk->net  );
   tlist->tyoung= pk->tpick;             /* youngest pick passed so far       */
   tlist->npass = 1;                     /* automatically pass this 1st pick  */
   tlist->npk   = nPickHistory;          /* keep track of max size of list    */
   tlist->pk    = tpk;                   /* store pointer to passed-pick list */
   memcpy( &(tlist->pk[0]), pk, sizeof(EWPICK) );  /* copy current pk to list */
   nHist++;

/* Resort entire history list
 ****************************/
   qsort( PkHist, nHist, sizeof(PKHISTORY), pkfilter_compare );

   logit("et","pkfilter_addsta: %s.%s added; tracking %d stas\n",
          pk->site, pk->net, nHist );
  
   return EW_SUCCESS;
}
