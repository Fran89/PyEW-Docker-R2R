/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: redi2ew.c 5697 2013-08-02 11:48:06Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2002/12/06 01:15:33  dietz
 *     added new instid argument to file2ew_ship call;
 *     changed fprintf(stderr...) calls to logit("e"...)
 *
 *     Revision 1.9  2002/06/05 16:19:44  lucky
 *     I don't remember
 *
 *     Revision 1.8  2001/08/30 06:36:30  dietz
 *     Changed to read the eventid properly from the new-format REDI groundmotion
 *     file names (eventid_ground.wave).
 *
 *     Revision 1.7  2001/04/27 18:33:19  dietz
 *     Added support for reading REDI version 1.0 and version 2.0 files.
 *
 *     Revision 1.6  2001/04/11 22:56:04  dietz
 *     changed TYPE_STRONGMOTION2 to TYPE_STRONGMOTIONII to match earthworm_global.d
 *
 *     Revision 1.5  2001/03/27 01:09:57  dietz
 *     Added support for reading heartbeat file contents.
 *     Currently file2ewfilter_hbeat() is a dummy function.
 *
 *     Revision 1.4  2001/02/08 16:36:02  dietz
 *     Added file2ewfilter_com function to read config file commands
 *     and file2ewfilter_shutdown() to cleanup before exit.
 *
 *     Revision 1.3  2000/10/20 18:47:14  dietz
 *     *** empty log message ***
 *
 *     Revision 1.2  2000/09/07 21:40:21  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 19:19:05  lucky
 *     Initial revision
 *
 *
 */

/*  redi2ew.c
 *  Read a REDI-format file (from UCB), create Earthworm strongmotion
 *  messages, and place them in a transport ring.  Returns number
 *  of TYPE_STRONGMOTIONII messages written to ring from file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <chron3.h>
#include <kom.h>
#include <k2evt2ew.h>
#include "file2ew.h"

/* Globals from sm_file2ew.c
 ***************************/
extern int  Debug;

/* Local globals
 ****************/
#define BUFLEN  65000           /* define maximum size for an event msg  */
static char MsgBuf[BUFLEN];     /* char string to hold output message    */
static unsigned char TypeSM2;   /* message type we'll produce            */
static int Init = 0;            /* initialization flag                   */
   
static char QuakeAuthor[AUTHOR_FIELD_SIZE]; /* string from config file   */
static int  QuakeAuthorInit = 0; /* initialization flag */

static char *RediNullLoc = "--"; /* identifies loc (L in SCNL) as unused  */

#define LINE_LEN 256

/* Functions in other source files
 *********************************/
void logit( char *, char *, ... );         /* logit.c      sys-independent  */
int  GetType ( char *, unsigned char * );  /* getutil.c    sys-independent  */

/******************************************************************************
 * file2ewfilter_com() processes config-file commands to set up the           *
 *                     REDI-specific parameters                               *
 * Returns  1 if the command was recognized & processed                       *
 *          0 if the command was not recognized                               *
 * Note: this function may exit the process if it finds serious errors in     *
 *       any commands                                                         *
 ******************************************************************************/
int file2ewfilter_com( )
{
   char *str;

/* Author of event files 
 ***********************/
   if( k_its( "QuakeAuthor" ) ) {
      if( str=k_str() ) {
         if(strlen(str)>=AUTHOR_FIELD_SIZE ) { 
            logit( "e", "redi2ew_com: author <%s> too long"
                   " in <QuakeAuthor> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(QuakeAuthor, str);
         QuakeAuthorInit=1;
      }
      return( 1 );
   }

   return( 0 );
}

/******************************************************************************
 * file2ewfilter()   Do all the work to convert REDI files to Earthworm msgs  *
 ******************************************************************************/
int file2ewfilter( FILE *fp, char *fname )
{
   char        line[LINE_LEN];
   char        sta[TRACE_STA_LEN];
   char        comp[TRACE_CHAN_LEN];
   char        net[TRACE_NET_LEN];
   char        loc[TRACE_LOC_LEN];
   char        qid[EVENTID_SIZE];
   char        datatype[10];
   char       *c;
   double      t;
   double      data;
   SM_INFO     sm;
   struct Greg boxtime;
   int         yday;
   int         sec,microsec;
   int         newmsg  = 1;
   int         nline   = 0;
   int         nmsg    = 0;
   int         nval    = 0;
   float       version = 0.0;
   int         rc;

/* Initialize variables
 **********************/
   if(!Init) file2ewfilter_init();
   memset( &sm,  0, sizeof(SM_INFO) );
   sm.pga = sm.pgv = sm.pgd = SM_NULL;
   memset( line, 0, LINE_LEN );

   if( fp == NULL ) return( -1 );

/* Get eventid from the file name (example: 51111793_ground.wave)
 ****************************************************************/
   strncpy( qid, fname, EVENTID_SIZE-1 );
   qid[EVENTID_SIZE-1] = '\0';
   c = strchr( qid, '_' );
   if( c == NULL ) {
      logit("e","redi2ew: Error reading eventid from filename: %s\n",
             fname );
      return( -1 );
   }
   *c = '\0';

/* Read file one line at a time
 ******************************/
   while( fgets(line, LINE_LEN, fp) != NULL )
   {
    /* Read version, skip other header lines which begin with '#'
    *************************************************************/
      nline++;
      if( strncmp( line, "#PGA", 4 ) == 0 ) {
         rc = sscanf( line, "#PGA Ver %f", &version );
         if( rc != 1 ) {
            logit("e","redi2ew: Error on line %d; did not read version properly\n",
                   nline );
            return( -1 );
         }
      }
      if( line[0] == '#' ) continue;

   /* Read all other lines;
      each contains either PGA,PGV,or PGD for one component.
    ********************************************************/
      memset( &boxtime, 0, sizeof(struct Greg) );
      if( version==2.0 ) {
         rc = sscanf( line, "%s %s %s %s %s %*f %*f %d %d %d %d %d %d %lf",
                      sta, net, comp, loc, datatype,
                     &(boxtime.year), &yday, &(boxtime.hour),
                     &(boxtime.minute), &sec, &microsec, &data );
         if( rc != 12 ) {
            logit("e","redi2ew: Error on line %d; read only %d of 12 fields\n",
                  nline, rc );
            return( -1 );
         }
         if( strcmp( loc, RediNullLoc ) == 0 ) strcpy( loc, "" );
      }
      else if( version==1.0 ) {
         rc = sscanf( line, "%s %s %s %s %*f %*f %d %d %d %d %d %d %lf",
                      sta, comp, net, datatype,
                     &(boxtime.year), &yday, &(boxtime.hour),
                     &(boxtime.minute), &sec, &microsec, &data );
         if( rc != 11 ) {
            logit("e","redi2ew: Error on line %d; read only %d of 11 fields\n",
                  nline, rc );
            return( -1 );
         }
         strcpy( loc, "" );
      } else {
         logit("e","redi2ew: Unknown version %.1f (check for #PGA header)\n",
                version );
         return( -1 );
      }

   /* Is data is from same channel as previous line?
    ************************************************/
      newmsg = 0;
      if( strcmp( sm.comp, comp ) != 0  || 
          strcmp( sm.sta,  sta  ) != 0  || 
          strcmp( sm.net,  net  ) != 0  || 
          strcmp( sm.loc,  loc  ) != 0     ) newmsg = 1;

   /* Start new message
    *******************/
      if( newmsg ) {

      /* Ship current SM_INFO struct if there's something in it
       ********************************************************/
         if( nval ) { 
            rc = wr_strongmotionII( &sm, MsgBuf, BUFLEN );
            if( rc != 0 ) {
               logit("e","redi2ew: Error building TYPE_STRONGMOTIONII msg\n");
               return( -1 );
            }
            if( file2ew_ship(TypeSM2, 0, MsgBuf, strlen(MsgBuf)) != 0 ) return( -1 );
            nval = 0; /* reset values counter      */
            nmsg++;   /* increment message counter */
         }

      /* Start filling out the clean SM_INFO struct:
       * We record the time of this parsing to cover the case where
       * the box time is way off, and this is better than nothing.
       *************************************************************/
         memset( &sm, 0, sizeof(SM_INFO) );   /* initialize SM_INFO struct */
         sm.pga = sm.pgv = sm.pgd = SM_NULL;  /* initialize SM_INFO struct */
         strncpy( sm.sta,  sta,  TRACE_STA_LEN  );
         strncpy( sm.comp, comp, TRACE_CHAN_LEN );
         strncpy( sm.net,  net,  TRACE_NET_LEN  );
         strncpy( sm.loc,  loc,  TRACE_LOC_LEN  );
         strcpy( sm.qid, qid );
         strcpy( sm.qauthor, QuakeAuthor );
         sm.talt    = (double)time( (time_t*)NULL );
         sm.altcode = SM_ALTCODE_RECEIVING_MODULE;

      } /* end if( newmsg ) */

   /* REDI data has a timestamp for each PGA,PGV,PGD. We will use the
    * earliest of these timestamps as the main timestamp for this channel.
    **********************************************************************/
      boxtime.month  = 1; /* fudge month,day; we'll add day-of-year later */
      boxtime.day    = 1;
      boxtime.second = 0.;
      t  =    60.0 * (double)julmin(&boxtime) - GSEC1970;  /* yr,hr,min */
      t += 86400.0 * (double)(yday-1);        /* adjust for day-of-year */
      t += (double)sec + (double)microsec/1000000.;   /* adjust seconds */
      if( sm.t==0.0 || t<sm.t ) sm.t = t;

   /* Now stuff the data in the proper field for this channel
    *********************************************************/
      data *= 0.1; /* convert from mm to cm */
      if     ( strcmp( datatype, "PGA" ) == 0 ) { sm.pga = data; sm.tpga = t; }
      else if( strcmp( datatype, "PGV" ) == 0 ) { sm.pgv = data; sm.tpgv = t; }
      else if( strcmp( datatype, "PGD" ) == 0 ) { sm.pgd = data; sm.tpgd = t; }
      else { /* unknown datatype! */
         logit("e","redi2ew: Error on line %d; unknown data type <%s>\n",
                nline, datatype );
         return( -1 );
      }
      nval++;

   } /* end of while - go get next line */

/* Build TYPE_SM msg from last SM_DATA structure and ship it
 ***********************************************************/
   if( nval ) {
      rc = wr_strongmotionII( &sm, MsgBuf, BUFLEN );
      if( rc != 0 ) {
         logit("e","redi2ew: Error building TYPE_STRONGMOTIONII msg\n");
         return( -1 );
      }
      if( file2ew_ship( TypeSM2, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) return( -1 );
      nmsg++;
   }

   return( nmsg );
}


/******************************************************************************
 * file2ewfilter_init()  check arguments for REDI data                        *
 ******************************************************************************/
int file2ewfilter_init( void )
{
   int ret=0;

/* Look up earthworm message type(s)
 ***********************************/
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeSM2 ) != 0 ) {
      logit( "e",
             "redi2ew: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n" );
      ret = -1;
   }

   if( !QuakeAuthorInit ) {
      logit( "e",
             "redi2ew: No <QuakeAuthor> command in config file; exiting!\n" );
      ret = -1;
   }

   Init = 1;
   return( ret );
}


/******************************************************************************
 * file2ewfilter_hbeat()  read heartbeat file, check for trouble              *
 ******************************************************************************/
int file2ewfilter_hbeat( FILE *fp, char *fname, char *system )
{
   return( 0 );
}


/******************************************************************************
 * file2ewfilter_shutdown()  free memory, other cleanup tasks                 *
 ******************************************************************************/
void file2ewfilter_shutdown( void )
{
   return;
}



/*------------------------------------------------------------
   Sample version 1.0 REDI data file named:  ground.wave.EQ.0040109548
   (a portion of the contents of file are between the lines)
  ------------------------------------------------------------
#PGA Ver 1.0
#Stat Com  Nt  Val  Azi Dip  Year Doy Hr Mn Sc    Ms         Amax    Freq(hz)    Lat      Lon     Rdist(km)
 BDM  HLE  BK  PGA  90.   0. 2000  21  4 41  9  4778      0.14136     3.52502 37.941 -121.813      0.00000
 BDM  HLE  BK  PGV  90.   0. 2000  21  4 42  8  5178      0.01328     0.17449 37.941 -121.813      0.00000
 BDM  HLE  BK  PGD  90.   0. 2000  21  4 42  6  4578      0.01321     0.14068 37.941 -121.813      0.00000
 BDM  HLN  BK  PGA   0.   0. 2000  21  4 41 19  2178      0.12524     2.22707 37.941 -121.813      0.00000
 BDM  HLN  BK  PGV   0.   0. 2000  21  4 41 18  2178      0.01136     1.17429 37.941 -121.813      0.00000
 BDM  HLN  BK  PGD   0.   0. 2000  21  4 40 30  3878      0.01299     0.08642 37.941 -121.813      0.00000
 BDM  HLZ  BK  PGA   0. -90. 2000  21  4 41 13  4678      0.13583     3.82536 37.941 -121.813      0.00000
 BDM  HLZ  BK  PGV   0. -90. 2000  21  4 41 12  3878      0.01055     0.27838 37.941 -121.813      0.00000
 BDM  HLZ  BK  PGD   0. -90. 2000  21  4 42 14  5778      0.00941     0.10016 37.941 -121.813      0.00000
 BRIB HLE  BK  PGA  90.   0. 2000  21  4 40 35  2418      4.02777     2.37646 37.919 -122.151      0.00000
 BRIB HLE  BK  PGV  90.   0. 2000  21  4 40 35  2668      0.33412     0.17453 37.919 -122.151      0.00000
 BRIB HLE  BK  PGD  90.   0. 2000  21  4 40 36  2168      0.30361     0.12937 37.919 -122.151      0.00000
 BRIB HLN  BK  PGA   0.   0. 2000  21  4 40 35  2418      2.99367     3.13824 37.919 -122.151      0.00000
 BRIB HLN  BK  PGV   0.   0. 2000  21  4 40 35    43      0.19554     0.54223 37.919 -122.151      0.00000
 BRIB HLN  BK  PGD   0.   0. 2000  21  4 41 44   793      0.16389     0.14047 37.919 -122.151      0.00000
 BRIB HLZ  BK  PGA   0. -90. 2000  21  4 40 35  2418      4.94826     2.36809 37.919 -122.151      0.00000
 BRIB HLZ  BK  PGV   0. -90. 2000  21  4 40 35   668      0.53321     0.13015 37.919 -122.151      0.00000
 BRIB HLZ  BK  PGD   0. -90. 2000  21  4 40 35  8918      0.35968     0.12399 37.919 -122.151      0.00000
  ------------------------------------------------------------
   NOTE: this file contains multiple stations;
         multiple lines per station.
         units: PGA=mm/s/s  PGV=mm/s  PGD=mm
  ------------------------------------------------------------*/


/*------------------------------------------------------------
   Sample version 2.0 REDI data file named:  ground.wave.EQ.0040109548
   (a portion of the contents of file are between the lines)
  ------------------------------------------------------------
#PGA Ver 2.0 
#Stat Net Chan Loc Val  Azi Dip  Year Doy Hr Mn Sc     Us         Amax    Freq(hz)       Lat         Lon   Elev    Rdist(km)
 ARC  BK  HLE  --  PGA  90.   0. 2001 110  5 11 29 491896     47.87501     2.45684 40.877720 -124.077377   30.1      0.00000
  ------------------------------------------------------------
   NOTE: this file contains multiple stations;
         multiple lines per station.
         units: PGA=mm/s/s  PGV=mm/s  PGD=mm
  ------------------------------------------------------------*/
 
