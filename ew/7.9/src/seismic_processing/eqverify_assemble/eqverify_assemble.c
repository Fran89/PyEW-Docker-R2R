/* Modified for use with eqassemble - PNL 2009/03/03 */
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqverify_assemble.c 6298 2015-04-10 02:49:19Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2009/06/22 18:10:13  lombard
 *     renamed to eqverify_assemble
 *
 *     Revision 1.1  2009/06/19 18:11:47  lombard
 *     Added new program eqverify_assemble.
 *     This is a small change from eqverify that is needed to be used with
 *     eqassemble. It adds configurability to the four test_* parameters and the
 *     Threshold to work with the three different versions that eqassemble could be
 *     configured to emit.
 *     eqverify_assemble CAN be used under eqprelim or eqproc if it is properly
 *     configured.
 *     The old eqverify CANNOT be used under eqassemble if you want to use any of the
 *     coda tests.
 *
 *     Revision 1.4  2004/07/01 22:45:01  dietz
 *     changed comments regarding length of phase line
 *
 *     Revision 1.3  2004/05/19 23:12:18  dietz
 *     minor cleanup
 *
 *     Revision 1.2  2002/06/05 15:13:59  patton
 *     Made logit changes.
 *
 *     Revision 1.1  2000/02/14 17:13:57  lucky
 *     Initial revision
 *
 *
 */


  /********************************************************************
   *                             eqverify                             *
   *                                                                  *
   *   Program to take an event message from eqcoda, study the:       *
   *     + distribution of arrival times and                          *
   *     + coda amplitude characteristics and coda duration           *
   *   and make a decision about whether the event is a noise-event   *
   *   or a real earthquake                                           *
   *                                                                  *
   * NOTE: This code was written to emulate the glitch-detecting part *
   *   of the program "eqmeas" by Al Lindh and Barry Hirshorn.  Some  *
   *   variables used here store the same information about the pick  *
   *   or coda that eqmeas does.  In that case, the equivalent eqmeas *
   *   variable name is included in the comments of the variable      *
   *   declaration as ("name").                                       *
   ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <earthworm.h>
#include <chron3.h>
#include <kom.h>

#define ABS(x) (( (x) >= 0.0 ) ? (x) : -(x) )

static const int InBufl = MAX_BYTES_PER_EQ;

/* Functions in this source file:
 ********************************/
void   eqverify_config ( char * );
int    eqverify_test   ( char * );
int    eqverify_rdphs  ( char *, char * );
int    eqverify_compare( const void *, const void * );
int    test_slopevsmag( void ); /* test from eqmeas */
int    test_freefitrms( void ); /* test from eqmeas */
int    test_codawt( void );     /* test from eqmeas */
int    test_ptimes( void );     /* Dave Oppenheimer's version; use only 1 ptime test    */
int    test_ptimeslh( void );   /* eqmeas (Lindh-Hirshorn)version; use only 1 ptime test*/
int    test_pgroup( void );     /* experimental test looking for groupings of picks; LDD*/
                                /*    this one may be better than test_ptimes or        */
                                /*    test_ptimeslh; use only 1 ptime test              */
int    test_pcodas( void );     /* experimental test on #p-codas vs #s-codas; LDD       */
int    eqmeas_cwt( int, long *, int, float, float );
float  bmag( float );
int    medcmp( const void *, const void * );
double median( int, double * );

/* Return values from eqverify_test()
 ************************************/
#define READERROR   -1  /* had trouble reading the message.      */
#define GLITCH       0  /* qualified as a noise; kill it.        */
#define FORCE        1  /* qualified as a noise, but passes the  */
                        /*    "force_report" rules; report it!   */
#define EARTHQUAKE   2  /* passes as an earthquake; report it!   */


/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
unsigned char TypeHyp2000Arc;
unsigned char TypeKill;
unsigned char TypeError;

/* Things read from the configuration file
 *****************************************/
unsigned char   MyModId;             /* eqverify's module id (same as eqproc's)*/
static   char   NextProcess[100];    /* command to send output to              */
static   int    LogSwitch   = 1;     /* 0 if no logging should be done to disk */
static   int    BadThresh[3];        /* if eqwt>=BadThresh, it's a noise event;*/
                                     /*   override with "EventThreshold"       */
static   int    version;             /* version number from eqassemble	       */
static   int    Force_npck  = 18;    /* Pass an event along regardless of its  */
static   float  Force_mag   = 2.0;   /*   eqwt if it has at least Force_npck   */
                                     /*   arrivals and its magnitude from the  */
                                     /*   median clenx is at least Force_mag.  */
                                     /*   Defaults are from eqmeas; override   */
                                     /*   with "force_report" command.         */
static   double MaxDeltaT   = 30.;   /* Consider pick-times within this many   */
                                     /*    seconds of the 1st-arriving pick    */
                                     /*    over-ride default with "MaxDeltaT"  */
static   long   KlipC       = 820;   /* Clip level for coda 2-sec avg absolute */
                                     /* amplitudes; over-ride with "coda_clip" */
static   double GlitchNsec  = 0.035; /* A glitch is defined as a group of at   */
static   int    GlitchMinPk = 4;     /*    least GlitchMinPk picks within      */
                                     /*    GlitchNsec seconds.  Defaults are   */
                                     /*    over-ridden with "define_glitch"    */
static   int    Do_slopevsmag;       /* switch for doing test_slopevsmag       */
static   int    Do_freefitrms;       /* switch for doing test_freefitrms       */
static   int    Do_codawt;           /* switch for doing test_codawt           */
static   int    Do_pgroup;           /* switch for doing test_pgroup (dietz)   */
static   int    Do_ptimes = 1;       /* switch for doing test_ptimes (oppen)   */

/* Structure to store all pertinent info for one event
 *****************************************************/
typedef struct {
       double  t;         /* pick arrival time                       */
       float   qfree;     /* free-fit slope to coda aav's            */
       float   qrms;      /* rms of free-fit to coda's aav's         */
       int     naav;      /* number of coda windows used in free-fit */
       char    cdesc1;    /* 2nd letter of 3-letter coda descriptor  */
                          /* from eqcoda:                            */
                          /*      P = P-wave coda (length < S-P time)*/
                          /*      S = S-wave coda                    */
                          /*  blank = no coda info for this pick     */
       int     clen;      /* raw coda length from picker             */
       int     clenx;     /* best coda length from eqcoda            */
       int     cwtx;      /* coda weight for "best" coda from eqcoda */
       int     cwt;       /* coda weight as calc by eqmeas           */
} PCK;
static PCK   Pck[MAX_PHS_PER_EQ];  /* all pick info for this event   */
static int   nP;                   /* number of entries in Pck       */
static float MedMag;               /* magnitude from median clenx    */

static double Srt[MAX_PHS_PER_EQ]; /* array for sorting various data */
static int    nSrt;                /* number of values in Srt        */


int main( int argc, char *argv[] )
{
   static char msg[MAX_BYTES_PER_EQ];  /* message from pipe          */
   char        line[200];              /* to store lines from msg    */
   char       *in;                     /* working pointer to message */
   int         msglen;                 /* length of input message    */
   int         type = 0;
   int         rc;
   int         exit_status = 0;

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: eqverify <configfile>\n" );
      exit( 0 );
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read the configuration file(s)
 ********************************/
   eqverify_config( argv[1] );
   logit( "" , "eqverify: Read command file <%s>\n", argv[1] );

/* Look up message types in earthworm.h tables
   *******************************************/
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      fprintf( stderr,
              "eqverify: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
      fprintf( stderr,
              "eqverify: Invalid message type <TYPE_KILL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "eqverify: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, 512, LogSwitch );


/* Spawn the next process
 ************************/
   sleep_ew(100);
   if ( pipe_init( NextProcess, (unsigned long)0 ) == -1 )
   {
      logit( "e", "eqverify: Error starting next process <%s>; exiting!\n",
              NextProcess );
      exit( -1 );
   }
   logit( "e", "eqverify: piping output to <%s>\n", NextProcess );

/*-------------------------Main Program Loop----------------------------*/

   do
   {
/* Get a message from the pipe
 *****************************/
      rc = pipe_get( msg, InBufl, &type );

      if ( rc < 0 )
      {
         if ( rc == -1 )
            logit( "et", "eqverify: Message in pipe too big for buffer\n" );
         else if ( rc < 0 )
            logit( "et", "eqverify: Trouble reading msg type from pipe\n" );
         exit_status = rc;
         break;
      }

/* Process the event messages
 ****************************/
      if ( type == (int) TypeHyp2000Arc )
      {
           rc = eqverify_test( msg );

        /* send along real earthquakes
         *****************************/
           if( rc == EARTHQUAKE )
           {
              if ( pipe_put( msg, (int) TypeHyp2000Arc ) != 0 )
              {
                 logit( "et", "eqverify: Error writing msg to pipe.\n");
              }
           }

        /* send along event (qualified as noise) that passed force_report rules
         **********************************************************************/
           else if( rc == FORCE )
           {
              sscanf( msg, "%[^\n]", line );
              logit( "e",
                     "eqverify: forced report of event:\n" );
              logit( "e", "F:%s\n", line );

              if ( pipe_put( msg, (int) TypeHyp2000Arc ) != 0 )
              {
                 logit( "et", "eqverify: Error writing msg to pipe.\n");
              }
           }

        /* don't send noise events; log them one line at a time
         ******************************************************/
           else if( rc == GLITCH )
           {
              logit( "e",
                     "eqverify: following event qualified as noise:\n" );
              msglen = strlen( msg );
              in     = msg;
              while( in < msg+msglen )
              {
                 if ( sscanf( in, "%[^\n]", line ) != 1 )  break;
                 logit( "e", "%s\n", line );
                 in += strlen( line ) + 1;
              }
              logit( "", "\n" );
           }

        /* complain if there was an error
         ********************************/
           else
              logit( "et", "eqverify: Error reading TYPE_HYP2000ARC message.\n" );
      }

/* Pass all other types of messages along
 ****************************************/
      else if ( type != (int) TypeKill )
      {
         if ( pipe_put( msg, type ) != 0 )
            logit( "et", "eqverify: Error writing msg to pipe.\n");
      }

      if ( pipe_error() )
      {
           logit( "et", "eqverify: Output pipe error; exiting\n" );
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

   if ( type == (int) TypeKill )
        logit( "t", "eqverify: Termination requested. Exiting.\n" );
   exit( exit_status );

   return (0);
}

/**************************************************************************
 * eqverify_test() reads a TYPE_HYP2000ARC message (or an archive message)*
 *                 and performs the noise tests                           *
 **************************************************************************/
int eqverify_test( char *msg )  /* TYPE_HYP2000ARC message from eqcoda */
{
   char  *in;             /* working pointer to message            */
   char   line[200];      /* to store lines from msg               */
   char   shdw[200];      /* to store shadow cards from msg        */
   long   id;             /* event id from binder; hypocenter line */
   int    msglen;         /* length of input message               */
   int    nline;          /* number of lines (not shadows) so far  */
   int    eqwt;           /* weight of event (0=good) ("iwt")      */
   int    rc;

/* Initialize some stuff
 ***********************/
   nline  = 0;
   nP     = 0;
   msglen = strlen( msg );
   in     = msg;

/* Read one data line and its shadow at a time from msg; process them
 ********************************************************************/
   while( in < msg+msglen )
   {
      if ( sscanf( in, "%[^\n]", line ) != 1 )  return( READERROR );
      in += strlen( line ) + 1;
      if ( sscanf( in, "%[^\n]", shdw ) != 1 )  return( READERROR );
      in += strlen( shdw ) + 1;
      nline++;
      /*logit( "", "%s\n", line );*/  /*DEBUG*/
      /*logit( "", "%s\n", shdw );*/  /*DEBUG*/

/* Process the hypocenter card (1st line of msg) & its shadow
 ************************************************************/
      if( nline == 1 ) {         /* hyp2000 hypocenter */
	  line[163] = '\0';
	  version = atoi(line+162);
	  if (version > 2) version = 2;
	  line[146] = '\0';
	  id = atol( line+136 );
	  line[76] = '\0';
	  logit( "", "\n  %s    ID:%ld\n", line, id );
	  continue;
      }

/* Process all the phase cards & their shadows
 *********************************************/
      if( strlen(line) < (size_t) 100 )   /* expected a pick; found the terminator */
          break;
      if( eqverify_rdphs( line, shdw ) != 0 )
          return( READERROR );

   } /*end while over reading message*/

   if( nP == 0 ) return( READERROR );     /* complain if no picks were read */

/* Sort picks by arrival time
 ****************************/
   qsort( Pck, nP, sizeof(PCK), eqverify_compare );

/* Arrive at an event weight based on glitch tests
 *************************************************/
   eqwt  = 0;
   eqwt += (version >= Do_slopevsmag) ? test_slopevsmag() : 0;   /* eqmeas test */
   eqwt += (version >= Do_freefitrms) ? test_freefitrms() : 0;   /* eqmeas test */
   eqwt += (version >= Do_codawt) ? test_codawt() : 0;       /* eqmeas test */
   eqwt += (version >= Do_pgroup) ? test_pgroup() : 0;       /* experimental arrival-time test on glitch-groups; LDD*/
    rc   = (version >= Do_ptimes) ? test_ptimes() : 0;       /* Oppenheimer arrival-time test; done for comparison  */
/* eqwt += test_ptimeslh();*/   /* eqmeas (Lindh-Hirshorn) arrival-time test--BAD!!*/
/* eqwt += test_pcodas();  */   /* experimental test on #p-codas vs #s-codas; LDD */

   logit("","  final event wt:       %2d                  BadThresh:  %2d\n",
          eqwt, BadThresh[version] );

/* See if event qualified as noise
 *********************************/
   if( eqwt >= BadThresh[version] ) {
       if( nP > Force_npck  &&  MedMag >= Force_mag )  return( FORCE  );
       else             			       return( GLITCH );
   }

/* If you got here, the event was a real earthquake
 **************************************************/
   return( EARTHQUAKE );
}

/**************************************************************************
 * eqverify_rdphs() reads an archive phase card an its shadow; calculates *
 *                  a few odds 'n ends; stores info in Pck structure      *
 **************************************************************************/
int eqverify_rdphs( char *pick, char *shdw )
{
/* numbers that came from the picker
 ***********************************/
   double  t;              /* arrival time (Gregorian seconds) ("ptime") */
   int     wt;             /* quality of arrival time                    */
   long    caav[6];        /* coda window avg absolute values            */
   int     clen;           /* raw coda length from the picker ("mcda")   */

/* numbers that came from eqcoda
 *******************************/
   float   qfree;          /* slope from free-fit to coda amps ("slope") */
   float   qrms;           /* measure of error on fit to caavs ("del")   */
   int     clenx;          /* extrapolated coda length ("fmp")           */
   int     cwtx;           /* weight given to extrapolated length        */
   int     naav;           /* number of caav'sd used in fit ("na")       */
   char    cdesc[4];       /* 3-letter coda descriptor                   */

/* stuff to use here
 *******************/
   char    timestr[18];    /* char-string holding arrival time           */
   float   secs;           /* whole-seconds field of arrival time        */
   int     hsec;           /* hundredths of a second of arrival time     */
   int     cwt;            /* coda-wghting criteria from eqmeas ("cwt")  */

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

/* Make sure there's room for another pick
 *****************************************/
   if( nP >= MAX_PHS_PER_EQ )  return( 0 );

/* Ignore weighted-out P-arrivals and their shadows
 **************************************************/
   wt = pick[16] - '0';
   if( wt >= 4 ) return( 0 );

/* Read parts of pick card, back to front
 ****************************************/
   pick[91] = '\0';   clenx = atoi( pick+87 );
   pick[83] = '\0';   cwtx  = atoi( pick+82 );

   pick[34] = '\0';   hsec  = atoi( pick+32 );
   pick[32] = '\0';   secs  = (float) atof( pick+29 );
   strncpy( timestr,    pick+17, 12 );
   strncpy( timestr+12, "00.00",  5 );
   timestr[17] = '\0';

/* Decode the arrival time
 *************************/
   if( ( t = julsec17(timestr) ) == 0.0 ) {
      logit( "", "eqverify: error decoding arrival time <%s>\n", timestr );
      return( -1 );
   }
   t += (double) secs + (double) hsec/100.0;
   /*logit("", "Pick timestr: %s secs:%.0f hsec:%02d   Gregorian t: %.2lf \n",
             timestr, secs, hsec, t );*/  /*DEBUG*/
      
/* Read other interesting info off shadow card
 *********************************************/
   shdw[92] = '\0';    caav[5] = atol( shdw+88 );  
   shdw[85] = '\0';    caav[4] = atol( shdw+81 );
   shdw[78] = '\0';    caav[3] = atol( shdw+74 );
   shdw[71] = '\0';    caav[2] = atol( shdw+67 );
   shdw[64] = '\0';    caav[1] = atol( shdw+60 );
   shdw[57] = '\0';    caav[0] = atol( shdw+53 );
   shdw[40] = '\0';    clen    = atoi( shdw+35 );
   shdw[34] = '\0';    strcpy( cdesc, shdw+31 );
   shdw[30] = '\0';    qrms    = (float) atof( shdw+25 );
   shdw[25] = '\0';    qfree   = (float) atof( shdw+20 );
   shdw[5]  = '\0';    naav    = atoi( shdw+2  );


   /*logit("","naav: %d  qfree: %.2f  qrms: %.2f  clen: %4d  caav: %d %d %d %d %d %d\n",
          naav, qfree, qrms, clen,
          caav[0], caav[1], caav[2], caav[3], caav[4], caav[5] );*/ /*DEBUG*/

/* Adjust values so they match what eqmeas used
 **********************************************/
   qfree = -qfree;  /* slopes stored in archive msg are in CUSP convention */
                    /* which is the inverse of the eqmeas convention       */
   cwt = eqmeas_cwt( clen, caav, naav, qfree, qrms );

   /*logit("","cwtx: %1d   cwt: %1d  cwtx-cwt: %2d\n", cwtx, cwt, cwtx-cwt );*/ /*DEBUG*/

/* cdesc, the 3-letter coda descriptor uses these codes for Earthworm picks:
 * 1st letter:  P  normal coda termination; picker coda >0 and <144 sec.
 *              S  short/timed-out coda termination; picker coda = 144 sec.
 *              N  noisy coda with early termination; picker coda < 0.
 *
 * 2nd letter:  S  S-wave coda; duration is longer than S-minus-P time.
 *              P  P-wave coda; duration is less than S-minus-P time.
 *
 * 3rd letter:  _  (blank space)  Did not do a coda fit to the caav's.
 *              X  duration (clenx) is from fixed-slope L1-norm fit to caav's.
 *              R  duration (clenx) is from a free L1-norm fit to the caav's.
 *              N  normal coda termination; no fit necessary.  clenx = clen.
 */

/* Store useful info in Pck structure
 ************************************/
   Pck[nP].t      = t;
   Pck[nP].qfree  = qfree;
   Pck[nP].qrms   = qrms;
   Pck[nP].naav   = naav;
   Pck[nP].clen   = clen;
   Pck[nP].clenx  = clenx;
   Pck[nP].cwtx   = cwtx;
   Pck[nP].cwt    = cwt;
   Pck[nP].cdesc1 = cdesc[1];
   nP++;

   return( 0 );
}

/****************************************************************************
 * test_slopevsmag() performs the free-fit slope vs coda magnitude test     *
 *            from eqmeas.  Calculates the event magnitude from the median  *
 *            coda length.  Returns some number to add to the event weight  *
 *            based on the test (0=good agreement); (eqmeas test)           *
 ****************************************************************************/
int test_slopevsmag( void )
{
   float  medqfree;   /* median free-fit slope ("medslope") */
   float  medclenx;   /* median coda length ("medfmp")      */
   int    keep;
   int    i;

   MedMag = 0.0;  /* initialize event magnitude */
   if( nP == 0 )  return( 0 );

/* Find the median "best" coda length; calculate event magnitude.
   Always use the first 10 coda lengths; then consider only
   non-zero lengths of S-wave codas
 ****************************************************************/
   nSrt = 0;
   for( i=0; i<nP; i++ ) {
      if( i<10 )
      {
         if ( Pck[i].clenx != 0 ) Srt[nSrt++] = (double) Pck[i].clenx;
         else                     Srt[nSrt++] = (double) ABS((float)Pck[i].clen);
      }
      else if( Pck[i].clenx > 0  &&  Pck[i].cdesc1 == 'S' )
      {
         Srt[nSrt++] = (double) Pck[i].clenx;
      }
   }
   medclenx = (float) median( nSrt, Srt );
   MedMag   = bmag( medclenx );

/* Find the median free-fit slope.
   Always use the first 10 slopes, even if they are from P-wave codas;
   then use only slopes of S-wave codas fit to at least 2 coda windows
 *********************************************************************/
   nSrt = 0;
   keep = 1;
   for( i=0; i<nP; i++ ) {
      if( i>=10 ) {
         if( Pck[i].cdesc1 == 'S' ) keep = 1;
         else                       keep = 0;
      }
      if( Pck[i].naav >= 2  &&  keep )
      {
         Srt[nSrt++] = (double) Pck[i].qfree;
      }
   }
   medqfree = (float) median( nSrt, Srt );

/* Perform test as in eqmeas
 ***************************/
   i      = (int) ( medqfree + 0.7*MedMag + 0.5 );

   logit("","  test_slopevsmag:  res:%2d  medqfree: %5.2f  medclenx: %3d   MedMag: %4.1f\n",
         (int) ABS((float)i), medqfree, (int) medclenx, MedMag );

   if( i < 0 )  return( -i );
   else         return(  i );
}

/****************************************************************************
 * test_freefitrms() looks at the rms fit of the L1 free-fit slope to the   *
 *            coda window avg absolute values.  Returns a value to add to   *
 *            the event weight based on the test (0=good rms fit) (eqmeas)  *
 ****************************************************************************/
int test_freefitrms( void )
{
   float medqrms;  /* median rms free-fit slopes ("medfit") */
   int   keep;
   int   i;

   if( nP == 0 )            return( 0 );

/* Find the median rms to free-fit slopes
   Always use the first 10 values; then use
   only rms's from free-fits to S-wave codas
 *******************************************/
   nSrt = 0;
   keep = 1;
   for( i=0; i<nP; i++ ) {
      if( i>=10 ) {
         if( Pck[i].cdesc1 == 'S' ) keep = 1;
         else                       keep = 0;
      }
      if( Pck[i].naav >= 3  &&  keep )
      {
         Srt[nSrt++] = (double) Pck[i].qrms;
      }
   }
   medqrms = (float) median( nSrt, Srt );

   if( nSrt < 3 )  return( 0 );   /* not enough data; don't do test */

   if( medqrms > 0.25 )  i = 1;   /* bad fit                        */
   else                  i = 0;   /* good fit; or not enough info   */

   logit("","  test_freefitrms:  res:%2d   medqrms: %5.2f      nrms: %3d\n",
         i, medqrms, nSrt );

   return( i );
}

/****************************************************************************
 * test_codawt() returns the average coda wght to be added to the event wgt *
 *               eqmeas test                                                *
 ****************************************************************************/
int test_codawt( void )
{
   int  sumcwtx = 0;  /* sum of coda weights from eqcoda (cwtx < 4) */
   int  sumcwt  = 0;  /* sum of coda weights from eqmeas ("cwt")    */
   int  ncwtx   = 0;  /* number of eqcoda-coda weights in sumcwtx   */
   int  ncwt    = 0;  /* number of eqmeas-coda weights in sumcwt    */
   int  keep;
   int  i;

   if( nP == 0 )        return( 0 );

/* Get a sum of the coda weights
   Always use the first 10 weights; then
   only use coda weights from S-wave codas
 *****************************************/
   keep = 1;
   for( i=0; i<nP; i++ ) {
      if( i>=10 ) {
         if( Pck[i].cdesc1 == 'S' ) keep = 1;
         else                       keep = 0;
      }
      if( keep )
      {
         if( Pck[i].cwtx < 4 ) {
            sumcwtx += Pck[i].cwtx;
            ncwtx++;
         }
         sumcwt += Pck[i].cwt;
         ncwt++;
      }
   }

   if ( ncwtx > 0 ) {
     logit("","  test_codawt:      res:%2d    avgcwt:    %2d   avgcwtx: %3d (eqcoda)\n",
         (int) sumcwt/ncwt, (int) sumcwt/ncwt, (int) sumcwtx/ncwtx );
   } else {
     logit("","  test_codawt:      res:%2d    avgcwt:    %2d   avgcwtx: %3d (eqcoda)\n",
         (int) sumcwt/ncwt, (int) sumcwt/ncwt, sumcwtx );
   }

   return( (int) (sumcwt/ncwt) );
}

/****************************************************************************
 * test_ptimes() looks at the distribution of P-arrival times to determine  *
 *      if they may be related to a glitch.  Uses in its analysis only      *
 *      picks arriving within MaxDeltaT seconds of the first-arriving pick. *
 *      Returns a value to increment the event weight by.                   *
 *      This version is from Dave Oppenheimer                               *
 ****************************************************************************/
int test_ptimes( void )
{
   double t0;           /* time of first-arriving pick                  */
   double medptime;     /* median arrival time (of all picks within     */
                        /*     MaxDeltaT sec of 1st-arriving pick)      */
   double meddt;        /* median of all dt's with respect to the       */
                        /*     median arrival time                      */
   int  i;

   if( nP == 0 )        return( 0 );

/* Store all arrival times within MaxDeltaT sec of first
   arriving pick.  Find the median of these arrival times.
 *********************************************************/
   t0   = Pck[0].t;
   nSrt = 0;
   for( i=0; i<nP; i++ ) {
       if( (Pck[i].t - t0) > MaxDeltaT ) break;
       Srt[nSrt++] = Pck[i].t;
   }
   medptime = median( nSrt, Srt );

/* Subtract the median arrival time from all arrivals.
   (keep the absolute value); find the median dt.
 *****************************************************/
   for( i=0; i<nSrt; i++ ) {
       Srt[i] -= medptime;
       if( Srt[i] < 0 ) Srt[i] = -Srt[i];
   }
   meddt = median( nSrt, Srt );

/* Do the test on median dt
 **************************/
/*   if     ( meddt < 0.02 ) i = 4; */  /* Original test is a bit harsh */
/*   else if( meddt < 0.05 ) i = 3; */
/*   else if( meddt < 0.10 ) i = 2; */
/*   else if( meddt < 0.15 ) i = 1; */
/*   else                    i = 0; */

   if     ( meddt < 0.05 ) i = BadThresh[version]; /* new test is somewhat gentler          */
   else if( meddt < 0.10 ) i = 1;         /* hopefully won't throw away little eqs */
   else                    i = 0;

   logit("","  test_ptimes:     #res:%2d     meddt: %5.2f       ndt: %3d       nP:  %3d\n",
          i, (float) meddt, nSrt, nP );

   return( i );
}

/******************************************************************************
 * test_pgroup() looks for groups of picks that belong to glitches.  A glitch *
 *      is defined as a group of at least GlitchMinPk picks within GlitchNsec *
 *      seconds.  Call the event noise if the total of all picks belonging    *
 *      to glitches is at least 50% of all picks within MaxDeltaT seconds of  *
 *      the first-arriving pick.                      experimental test...    *
 * This may be better than either test_ptime or test_ptimelh cause it doesn't *
 * seem to throw away small real earthquakes           LDD 4/5/96             *
 ******************************************************************************/
int test_pgroup( void )
{
   double t0;                       /* time of 1st-arriving pick            */
   float  percentglitch;            /* % of picks that belong to glitches   */
   int    mintolocate = 4;          /* min # picks required to locate event */
   char   flag[MAX_PHS_PER_EQ];     /* flag for each pick (>0 if in glitch) */
   int    nglitch;                  /* total number of picks in glitches    */
   int    i, ig, j;

   if( nP < GlitchMinPk ) return( 0 );

/* Gather all picks within MaxDeltaT sec of 1st one
 **************************************************/
   t0   = Pck[0].t;
   nSrt = 0;
   for( i=0; i<nP; i++ )
   {
       if( (Pck[i].t - t0) > MaxDeltaT ) break;
       Srt[nSrt]  = Pck[i].t;
       flag[nSrt] = 0;
       nSrt++;
   }

/* Look for glitch-groups; keep track of #picks in glitches
 **********************************************************/
   nglitch = 0;
   for( i=0; i<=nSrt-GlitchMinPk; i++ )
   {
      ig = i + GlitchMinPk - 1;
      if( (Srt[ig] - Srt[i]) <= GlitchNsec )  /* found a glitch! */
      {
         for( j=i; j<=ig; j++ )               /* loop over all picks in this glitch */
         {
             if( flag[j] == 0 ) nglitch++;    /* new glitch pick; increment counter */
             flag[j]++;                       /* flag this pick as a glitch pick    */
         }
      }
   }
   percentglitch = (float) nglitch / (float) nSrt;

/* Do the test!  If at least 50% of the picks are glitch-picks,
   or if there aren't enough non-glitch picks left to locate the
   event, it will be called noise.  I chose 50% to be consistent
   with other tests in this program since they all use medians or
   averages in their decisions.  If at least 30% are glitch-picks,
   I'll be skeptical about the event and add one to its weight.
 *****************************************************************/
   if( percentglitch >= 0.50    ||
       nP - nglitch  <  mintolocate )  i = BadThresh[version];
   else if( percentglitch >= 0.30   )  i = 1;
   else                                i = 0;

   logit("","  test_pgroup:      res:%2d   nglitch:   %3d  npcktest: %3d       nP:  %3d\n",
          i, nglitch, nSrt, nP );

   return( i );
}

/****************************************************************************
 * test_ptimeslh() looks at the distribution of P-arrival times to determine*
 *      if they may be related to a glitch.  Uses in its analysis only      *
 *      picks arriving within MaxDeltaT seconds of the first-arriving pick. *
 *      Returns a value to increment the event weight by.                   *
 *      This version is the Lindh-Hirshorn method from eqmeas               *
 * This test is bad because it gives results that make real events look     *
 * like noise events.  LDD 4/5/96                                           *
 ****************************************************************************/
int test_ptimeslh( void )
{
   double meddt;        /* median of inter-pick dt's                    */
   int    maxdt = 15;   /* number of delta-t's to analyze (from eqmeas) */
   int  i;

   if( nP == 0 ) return( 0 );

/* Store the time difference between consecutive arrivals.
   Find the median of these dt's.
 *********************************************************/
   nSrt = 0;
   for( i=0; i<maxdt; i++ ) {
       Srt[nSrt++] = Pck[i+1].t - Pck[i].t;
   }
   meddt = median( nSrt, Srt );

/* Do the test on median dt
 **************************/
   if     ( meddt < 0.1 ) i = 2;
   else if( meddt < 0.2 ) i = 1;
   else                   i = 0;

   logit("","  test_ptimeslh:    res:%2d     meddt: %5.2f       ndt: %3d       nP:  %3d\n",
          i, (float) meddt, nSrt, nP );

   return( i );
}

/****************************************************************************
 * test_pcodas() counts the number of picks that have codas labeled as      *
 *      P-wave codas; gets upset if there are more P-wave codas for an      *
 *      event than there S-wave codas plus no-codas                         *
 *   experimental test I'm not sure if this one should be used or not...    *
 *                                                          LDD 4/4/96      *
 ****************************************************************************/
int test_pcodas( void )
{
   int  n_pcoda  = 0;
   int  n_scoda  = 0;
   int  n_nocoda = 0;
   int  i;

/* Count the number of different coda types
 ******************************************/
   for( i=0; i<nP; i++ ) {
       if     ( Pck[i].cdesc1 == 'P' ) n_pcoda++;
       else if( Pck[i].cdesc1 == 'S' ) n_scoda++;
       else if( Pck[i].cdesc1 == ' ' ) n_nocoda++;
   }

/* Do the test coda types
 ************************/
   if( n_pcoda > (n_scoda+n_nocoda) )  i = 1;
   else                                i = 0;

   logit("","  test_pcodas:      res:%2d    P-coda:   %3d    S-coda: %3d  no-coda:  %3d\n",
          i, n_pcoda, n_scoda, n_nocoda );

   return( i );
}

/****************************************************************************
 * eqverify_config() processes the configuration file using kom.c           *
 *                   functions exits if any errors are encountered          *
 ****************************************************************************/
void eqverify_config( char *configfile )
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
   ncommand = 10;
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "eqverify: Error opening command file <%s>; exiting!\n",
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
                          "eqverify: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command.
           Numbered commands are required;
           Un-numbered commands change default values.
         *********************************************/
 /*0*/      if( k_its( "LogFile" ) )
            {
                LogSwitch = k_int();
                init[0] = 1;
            }
 /*1*/      else if( k_its( "MyModuleId" ) )
            {
                if ( ( str=k_str() ) ) {
                   if ( GetModId( str, &MyModId ) != 0 ) {
                      logit( "e",
                             "eqverify: Invalid module name <%s>; exiting!\n",
                              str );
                      exit( -1 );
                   }
                }
                init[1] = 1;
            }

         /* Read in the next command to pipe messages to
          **********************************************/
  /*2*/     else if( k_its("PipeTo") )
            {
                str = k_str();
                if(str) strcpy( NextProcess, str );
                init[2] = 1;
            }

         /* Set switch for test_slopevsmag
          ********************************/
  /*3*/     else if( k_its("test_slopevsmag") )
            {
                Do_slopevsmag = k_int();
                init[3] = 1;
            }

         /* Set switch for test_freefitrms
          ********************************/
  /*4*/     else if( k_its("test_freefitrms") )
            {
                Do_freefitrms = k_int();
                init[4] = 1;
            }

         /* Set switch for test_codawt
          ****************************/
  /*5*/     else if( k_its("test_codawt") )
            {
                Do_codawt = k_int();
                init[5] = 1;
            }

         /* Set switch for test_pgroup
          ****************************/
  /*6*/     else if( k_its("test_pgroup") )
            {
                Do_pgroup = k_int();
                init[6] = 1;
            }

         /* Optional command: reset the minimum number of picks
            and the minimum magnitude to force the reporting on
            an event regardless of its event weight
          *****************************************************/
            else if( k_its("force_report") )
            {
                Force_npck = k_int();
                Force_mag  = (float) k_val();
            }

         /* Optional command: reset glitch definition
          *******************************************/
            else if( k_its("define_glitch") )
            {
                GlitchMinPk = k_int();
                GlitchNsec  = k_val();
            }

         /* Required command: reset event weight threshold for
            declaring an Prelim (version 0) event a noise event
          ****************************************************/
            else if( k_its("PrelimThreshold") )
            {
                BadThresh[0] = k_int();
		init[7] = 1;
            }

         /* Required command: reset event weight threshold for
            declaring an Rapid (version 1) event a noise event
          ****************************************************/
            else if( k_its("RapidThreshold") )
            {
                BadThresh[1] = k_int();
		init[8] = 1;
            }

         /* Required command: reset event weight threshold for
            declaring an Final (version 2) event a noise event
          ****************************************************/
            else if( k_its("FinalThreshold") )
            {
                BadThresh[2] = k_int();
		init[9] = 1;
            }

         /* Optional command: reset time-interval after 1st-arriving
            pick to use for studying pick-time distribution
          **********************************************************/
            else if( k_its("MaxDeltaT") )
            {
                MaxDeltaT = k_val();
            }

         /* Optional command: reset coda clipping level
          *********************************************/
            else if( k_its("coda_clip") )
            {
                KlipC = k_long();
            }

         /* Unknown command
          *****************/
            else
            {
                fprintf(stderr, "eqverify: <%s> Unknown command in <%s>.\n",
                        com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                       "eqverify: Bad <%s> command in <%s>; exiting!\n",
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
       logit( "e", "eqverify: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "         );
       if ( !init[1] )  logit( "e", "<MyModuleId> "      );
       if ( !init[2] )  logit( "e", "<PipeTo> "          );
       if ( !init[3] )  logit( "e", "<test_slopevsmag> " );
       if ( !init[4] )  logit( "e", "<test_freefitrms> " );
       if ( !init[5] )  logit( "e", "<test_codawt> "     );
       if ( !init[6] )  logit( "e", "<test_pgroup> "     );
       if ( !init[7] )  logit( "e", "<PrelimThreshold> " );
       if ( !init[8] )  logit( "e", "<RapidThreshold> "  );
       if ( !init[9] )  logit( "e", "<FinalThreshold> "  );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
    }

    return;
}


/****************************************************************************
 * eqmeas_cwt() returns a coda weight calculated in the eqmeas fashion;     *
 *     this may or may not be the same as that calculated by eqcoda (cwtx)  *
 ****************************************************************************/
int eqmeas_cwt( int clen, long *caav, int naav, float qfree, float qrms )
{
   int   kgood;      /* counter of unclipped coda avg absolute amplitudes  */
   int   kbad;       /* counter of clipped coda avg absolute amplitudes    */
   float cmag;       /* coda mag from raw picker coda duration             */
   int   cwt;        /* eqmeas's coda weight                               */
   int   i;
   float test;

/* CALCULATE CWT FOR THIS SEISMOGRAM
   cwt is an estimate of overall arrival quality
   based on the amplitude characteristics
 ************************************************/
   cwt  = 0;
   cmag = bmag( ABS((float)clen) );       /* Calculate a preliminary magnitude */
   /*logit( "","clen:%4d  bmag(clen): %.2f\n", clen, cmag );*/ /*DEBUG*/

   if (naav >= 2) {
   /* Now test cmag vs. free-fit slope */
      test = (float) (qfree+0.7*cmag+0.5);
      if (ABS(test) > .75)  cwt++;
      if (ABS(test) > 1.5)  cwt++;
   /* Now test fit to coda amplitude */
      if (qrms > 0.1)       cwt++;
      if (qrms > 0.2)       cwt++;
   }
   else {	
      cwt += 1;       /* Add 2 to cwt if only one amplitude left */
   }

/* Add 1 if number of clipped coda amplitudes
   exceeds the number of onscale samples	
 ********************************************/
   kgood = 0;
   kbad  = 0;
   for( i=0; i<6; i++ ) {
       if( caav[i] == 0 )     continue;
       if( caav[i] < KlipC )  kgood++;
       else                   kbad++;
   }
   if (kbad > kgood)   cwt++;

   return( cwt );
}

/****************************************************************
 * bmag() computes an ML based on Bakun's new coda calibration	*
 *        Does NOT include the distance term			*
 *        Pulled from funcs.c in eqmeas's source code directory *
 ****************************************************************/
float bmag (float tau)
{
	if (tau <= 0.0)
		return 0.0;
	return (float)(0.69 + 0.655*pow(log10((double)tau), 2.));
}

/****************************************************************
 *  eqverify_compare() compare 2 arrival times                  *
 ****************************************************************/
int eqverify_compare( const void *p1, const void *p2 )
{
        PCK *pck1;
        PCK *pck2;

        pck1 = (PCK *) p1;
        pck2 = (PCK *) p2;
        if(pck1->t < pck2->t)   return -1;
        if(pck1->t > pck2->t)   return  1;
        return 0;
}

/****************************************************************
 *  median()  Calculate median of elements in array of doubles. *
 *    Note: the array is sorted in ascending order upon return. *
 ****************************************************************/
int medcmp( const void *x1, const void *x2 )
{
        if(*(double *)x1 < *(double *)x2)   return -1;
        if(*(double *)x1 > *(double *)x2)   return  1;
        return 0;
}

double median( int n, double *x )
{
        if(n <  1) return(  0.0 );
        if(n == 1) return( x[0] );
        qsort(x, n, sizeof(double), medcmp);
        return( n%2 ? x[n/2] : 0.5*(x[n/2]+x[n/2-1]) );
}

