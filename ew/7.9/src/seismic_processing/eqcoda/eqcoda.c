
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqcoda.c 6298 2015-04-10 02:49:19Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2006/09/20 18:34:29  dietz
 *     Modified to read per-channel parameters from more than one "StaFile"
 *
 *     Revision 1.7  2004/07/01 22:41:50  dietz
 *     increased phase line length to match newest hyp2000 format
 *
 *     Revision 1.6  2004/05/18 20:21:00  dietz
 *     Modified to use TYPE_EVENT_SCNL as input and to include location
 *     codes in the TYPE_HYP2000ARC output msg.
 *
 *     Revision 1.5  2002/05/16 15:27:51  patton
 *     Made logit changes.
 *
 *     Revision 1.4  2001/12/12 19:15:11  dietz
 *     Fixed bug that allowed extrapolated coda duration to overflow archive
 *     message format.
 *
 *     Revision 1.3  2000/08/17 20:09:28  dietz
 *     Changed configurable clip param from DigNbit to ClipCount
 *
 *     Revision 1.2  2000/07/21 23:06:47  dietz
 *     Added per-channel parameters (instead of globals).
 *
 *     Revision 1.1  2000/02/14 17:07:37  lucky
 *     Initial revision
 *
 *
 */

  /********************************************************************
   *                             eqcoda                               *
   *                                                                  *
   *     Program to take an event message from eqproc, calculate      *
   *     coda length & weight (along with other things) and send      *
   *     out a message that looks like a hypoinverse archive file     *
   *     with shadow cards.                                           *
   *                                                                  *
   ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <chron3.h>
#include <kom.h>
#include "eqcoda.h"

static const int InBufl = MAX_BYTES_PER_EQ;
# define LOGBUFSIZE 512

/* Functions in this source file:
 ********************************/
void  eqc_config( char * );
int   eqc_eventscnl( char *, char * );
int   eqc_readhyp( char * );
int   eqc_readphs( char * );
void  eqc_calc( void );
void  eqc_bldhyp ( char * );
void  eqc_bldphs ( char * );
void  eqc_bldterm( char * );

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
unsigned char TypeEventSCNL;
unsigned char TypeHyp2000Arc;
unsigned char TypeKill;
unsigned char TypeError;

/* Things read from the configuration file
 *****************************************/
#define MAXLEN 100
unsigned char MyModId;            /* eqcoda's module id (same as eqproc's)  */
static   int  LogSwitch = 1;      /* 0 if no logging should be done to disk */
static   char NextProcess[MAXLEN];/* command to send output to              */
static   int  LabelAsBinder = 0;  /* if 0, label phases as generic P and S  */
                                  /* if non-zero, use labels from binder    */
static   int  LabelVersion  = 0;  /* if non-zero, writes the version number */
                                  /* from eqproc,eqprelim on the archive    */
                                  /* summary line                           */
STAPARM *StaArray = NULL;         /* structure of per-channel parameters    */
int      Nsta = 0;                /* #channels read from StaFile(s)         */
int      ForceExtrapolation = 0;  /* =1 for debugging purposes              */
int      LogArcMsg = 0;           /* =nonzero if to log TYPE_HYP2000ARC msg */
                

/*----------------------------------------------------------------
 Notes about eqcoda constants:
   For historical perspective, most of this code was originally
   written to handle Rex Allen RTP digitizer data which had an output
   range of +/- 2500 counts for an input signal of +/- 2.5 Volts. 
   
   Many constants used here were originally defined for the RTP data
   to be "nice round numbers" corresponding to some number of mV.
   Original values for Rex Allen RTP data:
      KlipC  = 1000 count = 0.40 max zero-to-peak amplitude
      KlipP1 = 1200 count = 0.48 max zero-to-peak amplitude
      KlipP2 = 1400 count = 0.56 max zero-to-peak amplitude
   In Earthworm versions higher than v5.0, these constants have been 
   converted to Fraction of ClipCount (max # counts zero-to-peak), 
   allowing us to set constants for systems having different ranges 
   of digital counts. This version of eqcoda is able to set per-channel 
   params by reading a station list file, the same one that is read by 
   pick_ew. A new column, specifying the ClipCount (maximum counts
   zero-to-peak) for each channel, has been added for eqcoda to use.  
   If this value is omitted, eqcoda assumes the channel is a 
   standard Earthworm analog channel from a 12 bit digitizer and
   assigns a DefaultClipCount = 2048.
 ----------------------------------------------------------------*/

/* Default Clipping thresholds (fraction of max zero-to-peak value)
   to be applied after reading ClipCount from station list.
 ******************************************************************/
double FracKlipC  = 0.40; /* Clip: coda 2-sec avg abs amplitudes */
double FracKlipP1 = 0.48; /* Clip: first P-amplitude             */
double FracKlipP2 = 0.56; /* Clip: 2nd & 3rd P-amplitude         */

/* Below, we'll define a set of default parameters to use with
   SCNs that are not listed in the station file, StaFile.
   These are the same params that were used in v5.0 and earlier, 
   when eqcoda used global params instead per-channel params.
 ***************************************************************/
STAPARM DefaultSta;           /* default set of params to use w/unknown */
                              /*  SCNLs (assumes an EW analog channel   */
                              /*  from a 12-bit digitizer)              */
long DefaultClipCount = 2048; /* default ClipCount value; EW analog chan*/

/* Default clipping thresholds (#counts) for an EW 12-bit system: 
 * Command "coda_clip" over-rides default of KlipC
 *         "p_clip1"   over-rides default of KlipP1
 *         "p_clip2"   over-rides default of KlipP2
 * NOTE:  Used only if SCN is not in station list, StaFile.
 */
long  KlipC  =  820;      /* Clip: coda 2-sec avg abs amplitudes */
long  KlipP1 =  984;      /* Clip: first P-amplitude             */
long  KlipP2 = 1148;      /* Clip: 2nd & 3rd P-amplitude         */

/* Default coda termination level (counts) for an EW 12-bit system.  
 * Traditionally, codas have been "terminated" when the average absolute 
 * amplitude in a 2 second window of discriminator output reaches 60 mV.  
 * The value below is for 60mV input to a 12-bit Earthworm digitizer 
 * (+/-2048 counts for +/-2.5 V). Over-ride default with "pi.c7" command 
 * of picker.  
 * NOTE:  Used only if SCN is not in station list, StaFile!! 
 */
float    XtrapTo = (float) 49.15;


/* Define structures used to store pick, coda, hypocenter info
 *************************************************************/
static struct {
        char    cdate[19];
        double  t;
        int     dlat;
        float   mlat;
        char    ns;
        int     dlon;
        float   mlon;
        char    ew;
        float   z;
        float   rms;
        float   dmin;
        int     gap;
        int     nph;
        long    id;
        char    version;
} Hyp;

static struct {
        char    site[TRACE2_STA_LEN];
        char    comp[TRACE2_CHAN_LEN];
        char    net[TRACE2_NET_LEN];
        char    loc[TRACE2_LOC_LEN];
        char    cdate[19];
        double  t;
        char    phase[3];
        char    fm;
        char    wt;
        long    pamp[3];
        long    paav;
        long    caav[6];
        float   ccntr[6];       /* added by this program */
        int     clen;
        char    datasrc;
} Pck;

static struct {
        long    avgpamp;        /* average of 3 pamp values            */
        int     pampwt;         /* quality of pamp average             */
        float   afix;           /* intercept of fixed-fit to coda amps */
        float   qfix;           /* slope used in fixed-fit to coda amps*/
        float   afree;          /* intercept of free-fit to coda amps  */
        float   qfree;          /* slope from free-fit to coda amps    */
        float   rms;            /* measure of error on fit to caavs    */
        int     clenx;          /* extrapolated coda length            */
        short   cwtx;           /* weight given to extrapolated length */
        short   naav;           /* number of caav'sd used in fit       */
        char    cdesc[4];       /* alpha-numeric coda descriptor       */
} Calc;

int main( int argc, char *argv[] )
{
   static char inmsg[MAX_BYTES_PER_EQ];
   static char outmsg[MAX_BYTES_PER_EQ];

   int type;
   int rc;
   int exit_status = 0;

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: eqcoda <configfile>\n" );
      exit( 0 );
   }

/* Initialize some variables
 ***************************/
   memset(  NextProcess, 0, (size_t)MAXLEN  );
   memset( &DefaultSta,  0, sizeof(STAPARM) );

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, MAX_BYTES_PER_EQ, 1 );

/* Read the configuration file(s)
 ********************************/
   eqc_config( argv[1] );
   logit( "" , "eqcoda: Read command file <%s>\n", argv[1] );

/* Look up message types in earthworm.h tables
 *********************************************************/
   if ( GetType( "TYPE_EVENT_SCNL", &TypeEventSCNL ) != 0 ) {
      fprintf( stderr,
              "eqcoda: Invalid message type <TYPE_EVENT_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      fprintf( stderr,
              "eqcoda: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
      fprintf( stderr,
              "eqcoda: Invalid message type <TYPE_KILL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "eqcoda: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, MAX_BYTES_PER_EQ, LogSwitch );
   
/* Sort and log the station list as read from "StaFile" commands
   *************************************************************/
   if ( Nsta )  /* if at least one StaFile command was given */
   {
      qsort( StaArray, Nsta, sizeof(STAPARM), CompareSCNLs );
      LogStaList( StaArray, Nsta );
   } 
   else 
   {
      logit( "e", 
             "eqcoda: WARNING: No StaFile of per-channel parameters given; "
             "global defaults will be used!\n" );
   }

/* Set up default station based on v5.0 and earlier global values
 ****************************************************************/
   DefaultSta.CodaTerm    = XtrapTo;
   DefaultSta.ClipCount   = DefaultClipCount;
   DefaultSta.KlipC       = KlipC;
   DefaultSta.KlipP1      = KlipP1;
   DefaultSta.KlipP2      = KlipP2;
   DefaultSta.UsedDefault = 1;
   logit("","Default Parameters used with channels not in StaFile:\n" );
   LogStaList( &DefaultSta, 1 );

/* Spawn the next process
 ************************/
   if ( pipe_init( NextProcess, (unsigned long)0 ) == -1 )
   {
      logit( "e", "eqcoda: Error starting next process <%s>; exiting!\n",
              NextProcess );
      exit( -1 );
   }
   logit( "e", "eqcoda: piping output to <%s>\n", NextProcess );

/*-------------------------Main Program Loop----------------------------*/

   do
   {
   /* Get a message from the pipe
    *****************************/
      rc = pipe_get( inmsg, InBufl, &type );
      if ( rc < 0 )
      {
         if ( rc == -1 )
            logit( "et", "eqcoda: Message in pipe too big for buffer\n" );
         if ( rc == -2 )
            logit( "et", "eqcoda: <null> on pipe_get.\n" );
         if ( rc == -3 )
            logit( "et", "eqcoda: EOF on pipe_get.\n" );
         exit_status = rc;
         break;
      }

   /* Process the event messages
    ****************************/
      if ( type == (int) TypeEventSCNL )
      {
         /*printf("eqcoda: incoming message:\n%s\n", inmsg );*/ /*DEBUG*/
           if ( eqc_eventscnl( inmsg, outmsg ) == -1 ) {
                logit( "et", "eqcoda: Error processing TYPE_EVENT_SCNL message.\n" );
                continue;
           }
           if ( LogArcMsg ) {
                logit( "", "%s", outmsg ); 
           }
           if ( pipe_put( outmsg, (int) TypeHyp2000Arc ) != 0 ) {
                logit( "et", "eqcoda: Error writing msg to pipe.\n"); 
           }
         /*printf("eqcoda: outgoing message:\n%s\n", outmsg );*/ /*DEBUG*/
      }

   /* Pass all other types of messages along
    ****************************************/
      else if ( type != (int) TypeKill )
      {
         if ( pipe_put( inmsg, type ) != 0 )
            logit( "et", "eqcoda: Error writing msg to pipe.\n");
      }
      else /* type == (int) TypeKill */
      {
           logit( "et", "eqcoda: Termination requested; exiting\n" );
      }

      if ( pipe_error() )
      {
           logit( "et", "eqcoda: Output pipe error; exiting\n" );
           break;
      }

/* Terminate when a kill message is received
 *******************************************/
   } while ( type != (int) TypeKill );

/*-----------------------------End Main Loop----------------------------*/

/* Send a kill message to the child.
   Wait for the child to die.
 ***********************************/
   pipe_put( "", (int) TypeKill );
   sleep_ew( 500 );  /* give time for msg to get through pipe */
   pipe_close();
   free( StaArray );
   return( exit_status );
}

/****************************************************************************/
/* eqc_config() processes the configuration file using kom.c functions      */
/*              exits if any errors are encountered                         */
/****************************************************************************/
#define ncommand  4       /* # of required commands you expect to process   */
void eqc_config( char *configfile )
{
   char     init[ncommand]; /* init flags, one byte for each required command */
   int      nmiss;          /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "eqcoda: Error opening command file <%s>; exiting!\n",
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
                          "eqcoda: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
         /* Numbered commands are required
          ********************************/
   /*0*/    if( k_its( "LogFile" ) )
            {
                LogSwitch = k_int();
                init[0] = 1;
            }
   /*1*/    else if( k_its( "MyModuleId" ) )
            {
                if ( ( str=k_str() ) ) {
                   if ( GetModId( str, &MyModId ) != 0 ) {
                      logit( "e",
                             "eqcoda: Invalid module name <%s>; exiting!\n",
                              str );
                      exit( -1 );
                   }
                }
                init[1] = 1;
            }
    /*2*/   else if( k_its("PipeTo") )
            {
                str = k_str();
                if( str && strlen(str)<MAXLEN ) {
                   strcpy( NextProcess, str );
                }
                else {
                   logit( "e", "eqcoda: Invalid PipeTo command argument; "
                           "must be 1-%d chars long; exiting!\n",
                            MAXLEN-1 );
                   exit( -1 );
                }
                init[2] = 1;
            }
    /*3*/   else if( k_its("LabelAsBinder") )
            {
                LabelAsBinder = k_int();
                init[3] = 1;
            }
            else if( k_its("LabelVersion") )  /*Optional*/
            {
                LabelVersion = k_int();
            }
            else if( k_its("StaFile") )  /*Optional: added 000710:LDD*/
            {
                int nstabefore = Nsta;
                str = k_str();
                if( !str ) {
                   logit( "e", "eqcoda: Empty StaFile command argument; exiting!\n" );
                   exit( -1 );
                }
                if ( GetStaList( &StaArray, &Nsta, str ) == -1 ) {
                   logit( "e", "eqcoda: Error reading StaFile \"%s\"; exiting!\n",
                          str );
                   exit( -1 );
                } 
                if ( Nsta == nstabefore ) {
                   logit( "e", "eqcoda: Empty StaFile \"%s\"; exiting!\n",
                          str );
                   exit( -1 );
                }
                logit("","\nRead per-channel parameters from StaFile: %s\n", str);
            }
            else if( k_its("ForceExtrapolation") )  /*Optional; no arg*/
            {
                ForceExtrapolation = 1;
            }
            else if( k_its("LogArcMsg") )  /*Optional*/
            {
                LogArcMsg = k_int();
            }

         /* The following commands change default settings;
            they are not required 
           000710:LDD These commands have become outdated
             with per-channel parameters...
          ************************************************/
            else if( k_its("pi.c7") || k_its("coda_cutoff") )
            {
                XtrapTo = (float) k_val();
            }
            else if( k_its("coda_clip") )
            {
                KlipC = k_long();
            }
            else if (k_its("p_clip1") )
            {
                KlipP1 = k_long();
            }
            else if( k_its("p_clip2") )
            {
                KlipP2 = k_long();
            }
            else
            {
                logit( "e", "eqcoda: <%s> Unknown command in <%s>.\n",
                        com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                       "eqcoda: Bad <%s> command in <%s>; exiting!\n",
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
       logit( "e", "eqcoda: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "       );
       if ( !init[1] )  logit( "e", "<MyModuleId> "    );
       if ( !init[2] )  logit( "e", "<PipeTo> "        );
       if ( !init[3] )  logit( "e", "<LabelAsBinder> " );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
    }

    return;
}

/**************************************************************************/
/* eqc_eventscnl() supervises does all the work for one event message     */
/**************************************************************************/
int eqc_eventscnl( char *inmsg,   /* event message from eqproc/eqprelim */
                   char *outmsg ) /* event message to build here        */
{
   char  *in, *out;             /* working pointers to messages */
   char   line[126];            /* to store lines from inmsg    */
   int    inmsglen;             /* length of input message      */
   int    nline;                /* number of lines processed    */


/* Initialize some stuff
 ***********************/
   inmsglen = strlen( inmsg );
   in       = inmsg;
   out      = outmsg;
   nline    = 0;

/* Read & process one line at a time from inmsg
 **********************************************/
   while( in < inmsg+inmsglen )
   {
      if ( sscanf( in, "%[^\n]", line ) != 1 )  return( -1 );
      /*logit( "", "nline%3d: %s\n", nline, line );*/  /*DEBUG*/
      in += strlen( line ) + 1;

/* Process the hypocenter card (1st line of msg)
 ***********************************************/
      if ( nline == 0 ) {
         if( eqc_readhyp( line ) == -1 ) {
             logit("et", "eqcoda: error reading hypocenter; skip this event!\n");
             return( -1 );
         }
         logit("t", "Processing eventid: %ld\n", Hyp.id );
         nline++;
         eqc_bldhyp( out );
      }

/* Process all the phase cards (remaining lines of msg)
 ******************************************************/
      else {
         if( eqc_readphs( line ) == -1 )  continue;
         nline++;
         eqc_calc();
         eqc_bldphs( out );
      }

/* Reset out to point to the end of the outmsg
 *********************************************/
      out = outmsg + strlen( outmsg );

/* Process only so many phases per event; skip any extras
 ********************************************************/
      if ( nline >= (MAX_PHS_PER_EQ + 1) ) {
         logit( "et",
                "eqcoda: Warning: %d phases for current event; skipping extras.\n",
                 MAX_PHS_PER_EQ );
         break;
      }

   } /*end while of message loop*/

/* Make sure we processed at least one phase per event
 *****************************************************/
   if( nline <= 1 ) return( -1 );

/* Add the terminator card to end of outmsg
 ******************************************/
   eqc_bldterm( out );
   logit("t", "Finished processing eventid: %ld\n", Hyp.id );

   return( 0 );
}


/***************************************************************************/
/* eqc_readhyp() decodes a TYPE_EVENT_SCNL hypocenter card into Hyp struct */
/***************************************************************************/
int eqc_readhyp( char *card )
{
   float  lat,lon;
   int    rc;

/*------------------------------------------------------------------------------------
Sample binder-based hypocenter as built below (whitespace delimited, variable length);
Event id from binder is added at end of card.
19920429011704.653 36.346578 -120.546932 8.51 27 78 19.8 0.16 10103 1\n
--------------------------------------------------------------------------------------*/

/* Scan line for required values
 *******************************/
   rc = sscanf( card, "%s %f %f %f %d %d %f %f %ld %c",
                Hyp.cdate,
                &lat,
                &lon,
                &Hyp.z,
                &Hyp.nph,
                &Hyp.gap,
                &Hyp.dmin,
                &Hyp.rms,
                &Hyp.id,
                &Hyp.version );

   if( rc < 10 )
   {
      logit( "t", "eqc_readhyp: Error: read only %d of 10 fields "
                  "from hypocenter line: %s\n", rc, card );
      return( -1 );
   }

/* Read origin time
 ******************/
   Hyp.t = julsec18( Hyp.cdate );
   if ( Hyp.t == 0. ) 
   {
      logit( "t", "eqc_readhyp: Error decoding origin time: %s\n",
                   Hyp.cdate );
      return( -1 );
   }

/* Convert decimal Lat & Lon to degree/minutes
 **********************************************/
   if( lat >= 0.0 ) {
      Hyp.ns = 'N';
   } else {
      Hyp.ns = 'S';
      lat    = -lat;
   }
   Hyp.dlat = (int)lat;
   Hyp.mlat = (lat - (float)Hyp.dlat) * 60.0;

   if( lon >= 0.0 ) {
      Hyp.ew = 'E';
   } else {
      Hyp.ew = 'W';
      lon    = -lon;
   }
   Hyp.dlon = (int)lon;
   Hyp.mlon = (lon - (float)Hyp.dlon) * 60.0;

   return( 0 );
}

/***************************************************************************/
/* eqc_readphs() decodes a phase card into the Pck structure               */
/***************************************************************************/
int eqc_readphs( char *card )
{
   int rc;

/*--------------------------------------------------------------------------
Sample Earthworm format phase card (variable-length whitespace delimited):
CMN VHZ NC -- U1 P 19950831183134.902 953 1113 968 23 201 276 289 0 0 7 W\n
----------------------------------------------------------------------------*/
   rc = sscanf( card, 
               "%s %s %s %s %c%c %s %s %ld %ld %ld %ld %ld %ld %ld %ld %ld %d %c",
                 Pck.site,
                 Pck.comp,
                 Pck.net,
                 Pck.loc,
                &Pck.fm,
                &Pck.wt,
                 Pck.phase,
                 Pck.cdate,
                &Pck.pamp[0],
                &Pck.pamp[1],
                &Pck.pamp[2],
                &Pck.caav[0],
                &Pck.caav[1],
                &Pck.caav[2],
                &Pck.caav[3],
                &Pck.caav[4],
                &Pck.caav[5],
                &Pck.clen,
                &Pck.datasrc );

   if( rc < 19 )
   {
      logit( "t", "eqc_readphs: Error: read only %d of 19 fields "
                  "from hypocenter line: %s\n", rc, card );
      return( -1 );
   }

/* Read pick time
 ****************/
   Pck.t = julsec18( Pck.cdate );
   if ( Pck.t == 0. ) 
   {
      logit( "t", "eqc_readphs: Error decoding arrival time: %s\n",
                   Pck.cdate );
      return( -1 );
   }

   return( 0 );
}

/*************************************************************************/
/* eqc_calc() calculates the average P-amplitude and the extrapolated    */
/*  coda length based on an l1fit to the 2-second avg-amplitude values   */
/*  (some stuff that eqmeas and/or eqm2 used to do)                      */
/*  The coda fitting and extrapolating functions used here are taken     */
/*  from eqm2.f (converted to C by Lynn Dietz)  Barry Hirshorn said      */
/*  these functions do a better job than corresponding ones in eqmeas.c  */
/*************************************************************************/
void eqc_calc( void )
{
   float         sminp;      /* approximate S-P time              */
   float         dist;       /* approximate epicentral distance   */
   float         cmag;       /* trial duration magnitude          */
   short         ires;       /* error return                      */
   STAPARM      *sta = &DefaultSta;

/* Find parameters to use with this channel 
 ******************************************/
   if( Nsta ) /* if a StaFile was read in */
   {
      STAPARM  key;
      strcpy( key.sta,  Pck.site );
      strcpy( key.chan, Pck.comp );
      strcpy( key.net,  Pck.net  );
      strcpy( key.loc,  Pck.loc );
      sta = (STAPARM *) bsearch( &key, StaArray, Nsta, sizeof(STAPARM),
                                 CompareSCNLs );
      if( sta == (STAPARM *) NULL ) {
         logit("e", "eqcoda: %s %s %s %s unknown channel; using default params\n",
                key.sta, key.chan, key.net, key.loc );
         sta = &DefaultSta;
      }
   }

/* Find avg amp of 3 first half cycle amplitudes
 ***********************************************/
   Calc.avgpamp = (long) ( (Pck.pamp[0]+Pck.pamp[1]+Pck.pamp[2])/3. );

/* Assign a wt to avgpamp based on how many of the 3    */
/* half-cycle amplitudes exceed clipping levels.        */
   Calc.pampwt = 0;
   if (Pck.pamp[0] > sta->KlipP1) Calc.pampwt++;
   if (Pck.pamp[1] > sta->KlipP2) Calc.pampwt++;
   if (Pck.pamp[2] > sta->KlipP2) Calc.pampwt++;

/* Try to extrapolate coda length based on a fit to caav values
 **************************************************************/

/* Calculate approximate S-P time */
   sminp = (float) (( Pck.t - Hyp.t )*0.75);

/* Calculate approximate epicentral distance                   */
/* Assumes 2 layer crustal model as our magnitude correction   */
/* is lee's .0035*dist. [ v1=6 km/sec, v2=8 km/sec, h1=25km ]  */
   if ( sminp <= 22.27 ) {
       dist =  (float) (6.0 * ( Pck.t - Hyp.t ));
   } else  {
       dist = (float) (( 8.0 * ( Pck.t - Hyp.t ) ) - 44.1);
   }

/* Calculate a least squares fit on the naav values of    */
/* log(ia) vs log(t), where t is the time of the ia(naav) */
/* amplitude values after the ptime                       */
/* Calc.rms is the rms fit                                */

   codasub( (float) Pck.clen, sminp, Pck.caav, sta->KlipC, Pck.ccntr,
           &Calc.naav, &Calc.afree, &Calc.qfree, &Calc.afix, &Calc.qfix,
           &Calc.rms );

/* Calculate single-station magnitude for use in weighting coda */
   codmag ( (float) Pck.clen, dist, &cmag );

/* Build phase-descriptor string */
   cphs2( (float) Pck.clen, sminp, Calc.cdesc );

/* Assign a coda weight for this seismogram */
   asnwgt( Calc.naav, cmag, Calc.qfree, Calc.rms, &Calc.cwtx, &ires );

/* Come up with final coda length and coda weight:
 *************************************************/
/* If picker's coda length is less than S-P time, zero it out! */
   if ( Calc.cdesc[1] != 'S' ) {
       /*Calc.clenx    = 0;*/           /*original as in eqm2, changed to: */
         Calc.clenx    = ABS(Pck.clen); /*as per by DaveO.      960626:ldd */
         Calc.cwtx     = 4;
         Calc.cdesc[2] = ' ';
   }

/* Extrapolate the coda for                             */
/*    "noisy" (negative lengths from the picker) or     */
/*    "short" (length=144 from picker) codas     or     */
/*     for DEBUG purposes if ForceExtrapolation is set  */
   else if ( Calc.cdesc[0] != 'P' || ForceExtrapolation ) {
       cdxtrp ( sta->CodaTerm, Pck.clen, Calc.cdesc, &Calc.cwtx,
                Calc.naav, Calc.afix, Calc.qfix, Calc.afree, Calc.qfree,
                Calc.rms, &Calc.clenx, &ires );
       if( ForceExtrapolation ) {  /* Log some stuff for DEBUG */
         int i;
         logit("","%s %s %s %s  obs: %4d   xtp: %4d   %s%d   "
                  "KlipC: %ld CodaTerm: %.2f\n",
                 sta->sta, sta->chan, sta->net, sta->loc, Pck.clen, Calc.clenx, 
                 Calc.cdesc, Calc.cwtx, sta->KlipC, sta->CodaTerm );
         logit("","      ctime:caav" );
         for(i=5;i>=0;i--) logit("","  %.0f:%ld",  Pck.ccntr[i], Pck.caav[i] );
         logit("","\n");
         logit("","      naav:%hd  intrcpt-slope fixed: %.2f %.2f"
                  "  free: %.4f %.4f  rms: %.2f\n\n",
               Calc.naav,Calc.afix,Calc.qfix,Calc.afree,Calc.qfree,Calc.rms);
       }
   }

/* Use picker's coda length if it terminated normally; */
   else {
       Calc.clenx = Pck.clen;
   }

   if ( Calc.cwtx >= 4 ) Calc.cwtx = 4;

   return;
}

/***************************************************************************/
/* eqc_bldhyp() builds a hyp2000 (hypoinverse) summary card & its shadow   */
/***************************************************************************/
void eqc_bldhyp( char *hypcard )
{
   char line[170];   /* temporary working place */
   char shdw[170];   /* temporary shadow card   */
   int  tmp;
   int  i;

/* Put all blanks into line and shadow card
 ******************************************/
   for ( i=0; i<170; i++ )  line[i] = shdw[i] = ' ';

/*----------------------------------------------------------------------------------
Sample HYP2000 (HYPOINVERSE) hypocenter and shadow card.  Binder's eventid is stored 
in cols 136-145.  Many fields will be blank due to lack of information from binder.
(summary is 165 chars, including newline; shadow is 81 chars, including newline):
199806262007418537 3557118 4836  624 00 26 94  2   633776  5119810  33400MOR  15    0  32  50 99
   0 999  0 11MAM WW D189XL426 80         51057145L426  80Z464  102 \n
$1                                                                              \n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 12345
6789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
-----------------------------------------------------------------------------------*/

/* Write out hypoinverse summary card
 ************************************/
   strncpy( line,     Hyp.cdate,     14 );
   strncpy( line+14,  Hyp.cdate+15,  2  );
   sprintf( line+16,  "%2d%c", Hyp.dlat, Hyp.ns );
   tmp = (int) (Hyp.mlat*100.0);
   sprintf( line+19,  "%4d", (int) MIN( tmp, 9999 ) );
   sprintf( line+23,  "%3d%c", Hyp.dlon, Hyp.ew );
   tmp = (int) (Hyp.mlon*100.0);
   sprintf( line+27,  "%4d", (int) MIN( tmp, 9999 ) );
   tmp = (int) (Hyp.z*100.0);
   sprintf( line+31,  "%5d", (int) MIN( tmp, 99999 ) ); 
   sprintf( line+39,  "%3d", (int) MIN( Hyp.nph, 999 ) );
   sprintf( line+42,  "%3d", (int) MIN( Hyp.gap, 999 ) );
   sprintf( line+45,  "%3d", (int) MIN( Hyp.dmin, 999 ) );
   tmp = (int) (Hyp.rms*100.);
   sprintf( line+48,  "%4d", (int) MIN( tmp, 9999 ) );
   sprintf( line+136, "%10ld", Hyp.id );
   if(LabelVersion) line[162] = Hyp.version;
   else             line[162] = ' ';

   for( i=0; i<164; i++ ) if( line[i]=='\0' ) line[i] = ' ';
   sprintf( line+164, "\n" );

/* Write out summary shadow card
 *******************************/
   sprintf( shdw,     "$1"   );
   for( i=0; i<80; i++ ) if( shdw[i]=='\0' ) shdw[i] = ' ';
   sprintf( shdw+80,  "\n" );

/* Copy both to the target address
 *********************************/
   strcpy( hypcard, line );
   strcat( hypcard, shdw );
   return;
}

/*******************************************************************************/
/* eqc_bldphs() builds a hyp2000 (hypoinverse) archive phase card & its shadow */
/*******************************************************************************/
void eqc_bldphs( char *phscard )
{
   char line[125];     /* temporary phase card    */
   char shdw[125];     /* temporary shadow card   */
   float qfix, qfree;
   float afix, afree;
   int   nblank;
   int   offset;
   int   i;

/* Put all blanks into line and shadow card
 ******************************************/
   for ( i=0; i<125; i++ )  line[i] = shdw[i] = ' ';

/*-----------------------------------------------------------------------------
Sample Hyp2000 (hypoinverse) station archive cards (for a P-arrival and 
an S-arrival) and a sample shadow card. Many fields are blank due to lack 
of information from binder. Station phase card is 121 chars, including newline; 
shadow is 96 chars, including newline:
MTC  NC  VLZ  PD0199806262007 4507                                                0      79                 W  --       \n
MTC  NC  VLZ    4199806262007 9999        5523 S 1                                4      79                 W  --       \n
$   6 5.27 1.80 4.56 1.42 0.06 PSN0   79 PHP2 1197 39 198 47 150 55 137 63 100 71  89 79  48   \n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 12
-------------------------------------------------------------------------------*/
/* Build station archive card for a P-phase
 ******************************************/
   if( Pck.phase[0] == 'P' || Pck.phase[1] == 'P' )
   {
        strncpy( line,    Pck.site, 5 );
        strncpy( line+5,  Pck.net,  2 );
        strncpy( line+9,  Pck.comp, 3 );
        if( LabelAsBinder ) strncpy( line+13, Pck.phase, 2 );  
        else                strncpy( line+13, " P",      2 );
        if( Pck.fm == '?' ) line[15] = ' ';
        else                line[15] = Pck.fm;
        line[16] = Pck.wt;
        strncpy( line+17, Pck.cdate,   12 );        /* yyyymmddhhmm    */
        strncpy( line+30, Pck.cdate+12, 2 );        /* whole secs      */
        strncpy( line+32, Pck.cdate+15, 2 );        /* fractional secs */
        if( Calc.clenx > 9999 ) Calc.cwtx = 4;      /* weight out duration which */
                                                    /* overflows format          */
        sprintf( line+82, "%1d", (int) MIN( Calc.cwtx,  9 )    );   line[83] = ' ';
        sprintf( line+87, "%4d", (int) MIN( Calc.clenx, 9999 ) );   line[91] = ' ';
        line[108] = Pck.datasrc;
        strncpy( line+111, Pck.loc, 2 );

        for( i=0; i<120; i++ ) if( line[i]=='\0' ) line[i] = ' ';
        sprintf( line+120, "\n" );
   }

/* Build station archive card for a S-phase
 ******************************************/
   else
   {
   /* Force the coda-length to be weighted-out since it was measured */
   /* from the S-arrival time instead of the P-arrival time          */
        Calc.cwtx = 4;
   /* Build the phase card */
        strncpy( line,    Pck.site, 5 );
        strncpy( line+5,  Pck.net,  2 );
        strncpy( line+9,  Pck.comp, 3 );
        line[16] = '4';  /* weight out P-arrival; we're loading a dummy time */
        strncpy( line+17, Pck.cdate, 12 );    /* real year,mon,day,hr,min    */
        sprintf( line+29,  " 9999"      );    /* dummy seconds for P-arrival */
        strncpy( line+42, Pck.cdate+12, 2 );  /* actual whole secs S-arrival */
        strncpy( line+44, Pck.cdate+15, 2 );  /* actual fractional secs S    */
        if( LabelAsBinder ) strncpy( line+46, Pck.phase, 2 );
        else                strncpy( line+46, " S",      2 );
        line[49] = Pck.wt;
        if( Calc.clenx > 9999 ) Calc.cwtx = 4; /* weight out duration which  */
                                               /* overflows format           */
        sprintf( line+82, "%1d", (int) MIN( Calc.cwtx,  9 )    );
        sprintf( line+87, "%4d", (int) MIN( Calc.clenx, 9999 ) );
        line[108] = Pck.datasrc;
        strncpy( line+111, Pck.loc, 2 );

        for( i=0; i<120; i++ ) if( line[i]=='\0' ) line[i] = ' ';
        sprintf( line+120, "\n" );
   }

/* Build the shadow card
 ***********************/
   /* To follow CUSP convention, the slope of a "normal" coda decay    */
   /* should be positive; so print the inverse of the calculated slope */
   qfix  = (float) (-1.0 * Calc.qfix);
   qfree = (float) (-1.0 * Calc.qfree);

   /* Protect against print-format over-flow from negative numbers */
   qfix  = (float) MAX( qfix,       -9.99 );
   qfree = (float) MAX( qfree,      -9.99 );
   afix  = (float) MAX( Calc.afix,  -9.99 );
   afree = (float) MAX( Calc.afree, -9.99 );
   if( Calc.rms < 0. ) Calc.rms *= -1.;

   /* Now really build the shadow card */
   sprintf( shdw,    "$ %3d",  (int) Calc.naav            );
   sprintf( shdw+5,  "%5.2f",  MIN( afix,  99.99 )        );
   sprintf( shdw+10, "%5.2f",  MIN( qfix,  99.99 )        );
   sprintf( shdw+15, "%5.2f",  MIN( afree, 99.99 )        );
   sprintf( shdw+20, "%5.2f",  MIN( qfree, 99.99 )        );
   sprintf( shdw+25, "%5.2f ", MIN( Calc.rms, 99.99 )     );
   strncpy( shdw+31,  Calc.cdesc, 3 );
   sprintf( shdw+34, "%1d",    (int) MIN( Calc.cwtx, 9 )  );
   sprintf( shdw+35, "%5d ",   Pck.clen                   );
   sprintf( shdw+41, "PHP%1d", MIN( Calc.pampwt, 9 )      );
   sprintf( shdw+45, "%5ld",   MIN( Calc.avgpamp, 99999 ) );

   offset = 50;
   nblank = 0;
   for ( i=5; i>=0; i-- ) {  /* add coda aav's in increasing time order */
      if( Pck.ccntr[i] == 0. ) {
          nblank++;
          continue;
      }
      sprintf( shdw+offset, "%3d%4d", 
              (int) Pck.ccntr[i], (int) MIN( Pck.caav[i], 9999 ) );
      offset += 7;
   }
   if( nblank ) {      /* put unused (blank) coda aav's at end of line */
      for( i=0; i<nblank; i++ ) {
         sprintf( shdw+offset, "%3d%4d", 
                 (int) Pck.ccntr[5-i], (int) MIN( Pck.caav[5-i], 9999 ) );
         offset += 7;
      }
   }
   for( i=0; i<95; i++ ) if( shdw[i]=='\0' ) shdw[i] = ' ';
   sprintf( shdw+95, "\n" );

/* Copy both to the target address
 *********************************/
   strcpy( phscard, line );
   strcat( phscard, shdw );
   return;
}

/***************************************************************************/
/* eqc_bldterm() builds a hypoinverse event terminator card & its shadow   */
/***************************************************************************/
void eqc_bldterm( char *termcard )
{
        char line[100];   /* temporary working place */
        char shdw[100];   /* temporary shadow card   */
        int  i;

/* Put all blanks into line and shadow card
 ******************************************/
        for ( i=0; i<100; i++ )  line[i] = shdw[i] = ' ';

/* Build terminator
 ******************/
        sprintf( line+62, "%10ld\n",  Hyp.id );

/* Build shadow card
 *******************/
        sprintf( shdw, "$" );
        shdw[1] = ' ';
        sprintf( shdw+62, "%10ld\n",  Hyp.id );

/* Copy both to the target address
 *********************************/
        strcpy( termcard, line );
        strcat( termcard, shdw );
        return;
}
