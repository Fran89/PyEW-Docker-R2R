/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tremor2ew.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.11  2002/12/06 01:15:33  dietz
 *     added new instid argument to file2ew_ship call;
 *     changed fprintf(stderr...) calls to logit("e"...)
 *
 *     Revision 1.10  2002/06/05 16:19:44  lucky
 *     I don't remember
 *
 *     Revision 1.9  2001/08/30 08:24:17  dietz
 *     Fixed to read 2nd line of Tremor version2 fully.
 *
 *     Revision 1.8  2001/06/19 23:46:58  dietz
 *     Changed to read Tremor version 1 and 2.
 *     Heartbeat function changed to read number of live stations
 *     from heartbeat file, complaining if it's not the # expected.
 *
 *     Revision 1.7  2001/04/27 20:20:43  dietz
 *     *** empty log message ***
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

/*  tremor2ew.c
 *  Read a tremor-format file (John Evans' system), create Earthworm
 *  strongmotion messages, and place them in a transport ring.  Returns number
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

/* Globals from file2ew.c
 ************************/
extern int Debug;
extern unsigned char TypeError;
extern char Text[];

/* Local Globals
 ***************/
#define MAXCHAN  100            /* default maximum # channels            */
int MaxChan = MAXCHAN;          /* configured max# chan (MaxChannel cmd) */
int NchanNames = 0;	        /* number of names in this config file   */
CHANNELNAME *ChannelName = NULL; /* table of box/channel to SCNL         */

static int  NumTremorSites = 0; /* number of Tremor sites we should see  */
static int  PrevSitesAlive;     /* # active sites in last in heartbeat   */

#define BUFLEN  65000           /* define maximum size for an event msg  */
static char MsgBuf[BUFLEN];     /* char string to hold output message    */
static unsigned char TypeSM2;   /* message type we'll produce            */


/* Tremor-specific definitions
 *****************************/
#define TREMOR_CHAN_COUNT  3	/* Number of channels per box            */
#define TREMOR_NRSA        3	/* Number of points in response spectrum */

static int Init = 0;   /* initialization flag */

#define TOKEN_LEN 256

/* Functions in this source file
 *******************************/
int  tremor_cmpchan( const void *s1, const void *s2 );

/* Functions in other source files
 *********************************/
void logit( char *, char *, ... );         /* logit.c      sys-independent  */
int  GetType ( char *, unsigned char * );  /* getutil.c    sys-independent  */


/******************************************************************************
 * file2ewfilter_com() processes config-file commands to set up the           *
 *                     CSMIP-specific parameters                              *
 * Returns  1 if the command was recognized & processed                       *
 *          0 if the command was not recognized                               *
 * Note: this function may exit the process if it finds serious errors in     *
 *       any commands                                                         *
 ******************************************************************************/
int file2ewfilter_com( )
{
   char   *str;

/* Set number of Tremor sites to expect
 **************************************/
   if( k_its("NumTremorSites") ) { /*optional*/
      NumTremorSites = k_int();
      return( 1 );
   }

/* Reset the maximum number of channels
 **************************************/
   if( k_its("MaxChannel") ) { /*optional*/
      MaxChan = k_int();
      if( ChannelName != NULL ) {
         logit( "e", "tremor2ew_com: must specify <MaxChan> before any"
                " <ChannelName> commands; exiting!\n" );
         exit( -1 );
      }
      return( 1 );
   }

/* Get the mappings from box id to SCNL name
 ********************************************/
   if( k_its("ChannelName") ) {  /*optional*/
   /* First one; allocate ChannelName struct */
      if( ChannelName == NULL ) {
         ChannelName = (CHANNELNAME *) calloc( (size_t)MaxChan,
                                               sizeof(CHANNELNAME) );
         if( ChannelName == NULL ) {
            logit( "e", "tremor2ew_com: Error allocating %ld bytes for"
                   " %d chans; exiting!\n",
                   MaxChan*sizeof(CHANNELNAME), MaxChan );
            exit( -1 );
         }
      }
   /* See if we have room for another channel */
      if( NchanNames >= MaxChan ) {
         logit( "e", "tremor2ew_com: Too many <ChannelName> commands"
                " in configfile; MaxChannel=%d; exiting!\n",
                (int) MaxChan );
         exit( -1 );
      }
   /* Get the box name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>=SM_BOX_LEN ) { /* from rw_strongmotion.h */
            logit( "e", "tremor2ew_com: box name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].box, str);
      }
   /* Get the channel number */
      ChannelName[NchanNames].chan = k_int();
      if(ChannelName[NchanNames].chan > SM_MAX_CHAN){
         logit( "e", "tremor2ew_com: Channel number %d greater "
                "than %d in <ChannelName> cmd; exiting\n",
		 ChannelName[NchanNames].chan,SM_MAX_CHAN);
         exit(-1);
      }
   /* Get the SCNL name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_STA_LEN ) { /* from trace_buf.h */
            logit( "e", "tremor2ew_com: station name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].sta, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_CHAN_LEN ) { /* from trace_buf.h */
            logit( "e", "tremor2ew_com: component name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].comp, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_NET_LEN ) { /* from trace_buf.h */
            logit( "e", "tremor2ew_com: network name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].net, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_LOC_LEN ) { /* from trace_buf.h */
            logit( "e", "tremor2ew_com: location name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].loc, str);
      }
      NchanNames++;
      return( 1 );
   } 		

   return( 0 );
}

/******************************************************************************
 * file2ewfilter()  Do all the work to convert Tremor files to Earthworm msgs *
 ******************************************************************************/
int file2ewfilter( FILE *fp, char *fname )
{
   char        token[TOKEN_LEN+1];
   char        comp[TOKEN_LEN+1];
   struct Greg boxtime;
   double      t,talt;
   int         altcode;
   int         tremorversion = 1;
   float       fsec;
   SM_INFO     sm;
   CHANNELNAME key;
   double      period[TREMOR_NRSA];
   int         nmsg = 0;
   int         i,j,rc;

/* Initialize variables
 **********************/
   if(!Init) file2ewfilter_init();
   memset( token,    0, TOKEN_LEN+1 );
   memset( &boxtime, 0, sizeof(struct Greg) );
   memset( &key,     0, sizeof(CHANNELNAME) );

   if( fp == NULL ) return( -1 );

/* We record the time of this parsing to cover the case where
   the box time is way off, and this is better than nothing.
 *************************************************************/
   talt    = (double)time( (time_t*)NULL );
   altcode = SM_ALTCODE_RECEIVING_MODULE;

/* Line 1: SendersFileName; nothing useful
 *****************************************/
   rc = fscanf( fp, "%*[^S]SendersFileName=%[^\n]", token );
   if( rc != 1 ) {
      logit("e","tremor2ew: Error: SendersFileName line not found\n");
      return( -1 );
   }

/* Line 2: Trigger time from file
 ********************************/
   rc = fscanf( fp, "\nOrigin Time : %d/%d/%d %d:%d:%f UTC%*[^\n]",
                &(boxtime.year), &(boxtime.month), &(boxtime.day),
                &(boxtime.hour), &(boxtime.minute), &fsec );

   if( rc != 6 ) {
      logit("e","tremor2ew: Error: not enough fields Origin Time line\n");
      return( -1 );
   }

   t = 60.0*(double)julmin(&boxtime) + (double)fsec - GSEC1970;
   if(Debug) {
     time_t tfield = (time_t) t;
     logit("e","Time from message: (%.3lf)  %s",t,ctime(&tfield));
   }

/* Line 3: Column headings, including Spectral Periods
 *****************************************************/
/* Skip first part of line */
   rc = fscanf( fp, "\nSta Str Accel Vel D%s", token );
   if( rc != 1 ) {
      logit("e","tremor2ew: Error: Sta line not found\n");
      return( -1 );
   }
/* Read spectral periods; convert to frequency */
   for( j=0; j<TREMOR_NRSA; j++ )
   {
      rc = fscanf( fp, " SP_%lf", &period[j] );
      if( rc != 1 ) {
         logit("e","tremor2ew: Error: SP_ value %d not found\n", j);
         return( -1 );
      }
   }
/* Discover tremor version from last part of the line */
   rc = fscanf( fp, "%[^\n]", token );
   if( rc != 1 ) {
      logit("e","tremor2ew: Error: Unable to read end of line3\n");
      return( -1 );
   }
   if( strstr(token,"tVel") != NULL ) tremorversion = 2;
   if(Debug) logit("e","tremor2ew: version: %d  (end-of-line3: %s)\n", 
                    tremorversion, token );
 
/* Line 4 and remaining: 
   Read all values for a given channel & create a msg for that channel
   TREMOR VERSION 2: valid values in PGA, PGV, SP*, tVel fields 6/19/2001 
   TREMOR VERSION 1: only PGA is valid (as per John Evans 3/15/2001)
*********************************************************************/
   for( i=0; i<TREMOR_CHAN_COUNT; i++ )
   {
      CHANNELNAME *match;

   /* Initialize structure & fill in known header values */
      memset( &sm, 0, sizeof(SM_INFO) ); 
      sm.t       = t;
      sm.talt    = talt;
      sm.altcode = altcode;

   /* Read box name, component, PGA, PGV, PGD (all in cgs) */
      rc = fscanf( fp, "\n%s %s : %lf %lf %lf",
                   token, comp, &(sm.pga), &(sm.pgv), &(sm.pgd) );
      if( rc != 5 ) {
         logit("e","tremor2ew: Error reading data on line %d\n", i+4);
         return( -1 );
      }

      if( strlen(token)>=SM_BOX_LEN ) {
         logit("e","tremor2ew: Error: Box name <%s> too long; max=%d char\n",
                token, SM_BOX_LEN-1 );
         return( -1 );
      }
      if( strlen(comp)>=TRACE_CHAN_LEN ) {
         logit("e","tremor2ew: Error: component name <%s> too long; "
                "max=%d char\n", comp, TRACE_CHAN_LEN-1 );
         return( -1 );
      }

   /* Find SCNL mapping for this token(box)/comp pair */
      strcpy( key.box,  token );
      strcpy( key.comp, comp  );
      match = (CHANNELNAME *) bsearch( &key, ChannelName, NchanNames,
                                       sizeof(CHANNELNAME), tremor_cmpchan );
      if( match == NULL ) {
         logit("","tremor2ew: Error: No ChannelName mapping for"
                  " box:%s comp:%s\n", key.box, key.comp );
         return(-1);
      }
      strncpy( sm.sta,  match->sta,  TRACE_STA_LEN  );
      strncpy( sm.comp, match->comp, TRACE_CHAN_LEN );
      strncpy( sm.net,  match->net,  TRACE_NET_LEN  );
      strncpy( sm.loc,  match->loc,  TRACE_LOC_LEN  );

   /* Tremor Version 2: Read SP values (cm/s/s), tVel (time of PGV)         
    * PGA, PGV, SP*, tVel are the only valid values (set PGD to NULL) 
    *****************************************************************/
      if( tremorversion == 2 ) {
         double tVel;

         for(j=0;j<TREMOR_NRSA;j++)
         {
            rc = fscanf( fp, "%lf", &(sm.rsa[j]) );
            if( rc != 1 ) {
               logit("e","tremor2ew: Error: RSA[%d] not found for chan[%d]\n",j,i);
               return( -1 );
            }
            sm.pdrsa[j] = period[j];
         }
         sm.nrsa = TREMOR_NRSA;
         rc = fscanf( fp, "%*f %*f %lf", &tVel );
         if( rc != 1 ) {
            logit("e","tremor2ew: Error: tVel not found for chan[%d]\n",i);
            return( -1 );
         }
         sm.tpgv = sm.t + tVel;
         sm.pgd  = SM_NULL;
      }

   /* Tremor Version 1: Skip last part of line (WA_amp and ML);   
    * PGA is only valid value for version1, set others to NULL  
    ***********************************************************/
      else {  /* assume it's version 1 */
         fscanf( fp, "%*[^\n]" );
         sm.pgv = SM_NULL;
         sm.pgd = SM_NULL;
      }

   /* Build TYPE_SM2 msg from this SM_INFO structure and ship it */
      rc = wr_strongmotionII( &sm, MsgBuf, BUFLEN );
      if( rc != 0 ) {
         logit("e","tremor2ew: Error building TYPE_STRONGMOTIONII msg\n");
         return( -1 );
      }
      if( file2ew_ship( TypeSM2, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) return( -1 );
      nmsg++;

   } /* end for(TREMOR_CHAN_COUNT) */

   return( nmsg );
}


/******************************************************************************
 * file2ewfilter_init()  check arguments for Tremor data                      *
 ******************************************************************************/
int file2ewfilter_init( void )
{
   int i,j,chans,ret;

   ret = 0;

/* Look up earthworm message type(s)
 ***********************************/
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeSM2 ) != 0 ) {
      logit( "e",
             "tremor2ew: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n" );
      ret = -1;
   }

/* Make sure that some box->SCNL lines have been entered
 *******************************************************/
   if( NchanNames == 0 ) {
      logit("e","tremor2ew: ERROR: no ChannelName commands were provided\n" );
      ret = -1;
   }

/* Verify that we have correct # channel names for each box.
 ***********************************************************/
   for(i=0;i<NchanNames;i++) {
      chans=0;  /* the number of channels defined for box i */
      for(j=0;j<NchanNames;j++){
         if(strcmp(ChannelName[i].box,ChannelName[j].box)==0){
            chans++;
         }
      }
      if(chans != TREMOR_CHAN_COUNT) {
         logit("e","tremor2ew: ERROR: Box name %s has %d channels "
               "defined; %d required!\n",
                ChannelName[i].box, chans, TREMOR_CHAN_COUNT);
         ret = -1;
      }
   }

/* Sort channel name list for future bsearches
 *********************************************/
   qsort( ChannelName, NchanNames, sizeof(CHANNELNAME), tremor_cmpchan );

/* Set values used in reading heartbeat file
 *******************************************/
   PrevSitesAlive = NumTremorSites;  /* assume everything's up */

   Init = 1;
   return( ret );
}


/******************************************************************************
 * file2ewfilter_hbeat()  read heartbeat file, check for trouble              *
 ******************************************************************************/
int file2ewfilter_hbeat( FILE *fp, char *fname, char *system )
{
   int  nlive = 0;
   int  rc;

/*--------------------------------------------------------------
Here's an example of a Tremor heartbeat file:
992458231 TREMOR_Receiver() heartbeat (1 live sites). 
%#!HeartbeatFileName=AllsWell.hrt 
 --------------------------------------------------------------*/
   if( fp == NULL ) return( -1 ); 
   rewind( fp );

   rc = fscanf( fp, "%*d TREMOR_Receiver() heartbeat (%d", &nlive );
/*  if( rc != 1 ) {
      logit("e","tremor2ew: Error reading #live sites from heartbeat file\n" );
      return( -1 );
   }
 */ /* !!! UNCOMMENT above section when heartbeats are changed !!! */

/* Too few live sites in heartbeat file - complain if we haven't already
 ***********************************************************************/
   if( nlive < NumTremorSites ) {
      if( nlive != PrevSitesAlive )  {
         sprintf( Text,
                 "%d of %d Tremor sites are NOT alive!",
                  NumTremorSites-nlive, NumTremorSites );
         file2ew_status( TypeError, ERR_PEER_HBEAT, Text );
      }
   }
 
/* Heartbeat file looks good - say so if we previously complained
 ****************************************************************/
   else if( PrevSitesAlive != NumTremorSites ) {
      sprintf( Text, "all %d Tremor sites are alive!", NumTremorSites );
      file2ew_status( TypeError, ERR_PEER_HBEAT, Text );
   }

/* More live sites than we expected (change expectations)
 ********************************************************/
   if( nlive > NumTremorSites ) {
      logit("e","tremor2ew: Resetting NumTremorSites=%d (heartbeat shows %d,"
             " expected only %d)\n", nlive, nlive, NumTremorSites );
      NumTremorSites = nlive;
   }
       
/* Store the current number of live Tremor sites
 ***********************************************/
   PrevSitesAlive = nlive;
 
   return( 0 );
}


/******************************************************************************
 * file2ewfilter_shutdown()  free memory, other cleanup tasks                 *
 ******************************************************************************/
void file2ewfilter_shutdown( void )
{
   free( ChannelName );
   return;
}


/************************************************************************
 * tremor_cmpchan() a function passed to qsort; used to sort an array   *
 *   of CHANNELNAME struct by boxname & component code                  *
 ************************************************************************/
int tremor_cmpchan( const void *s1, const void *s2 )
{
   CHANNELNAME *ch1 = (CHANNELNAME *) s1;
   CHANNELNAME *ch2 = (CHANNELNAME *) s2;
   int  rc;

/* Test box name */
   rc = strcmp(ch1->box, ch2->box);
   if( rc < 0 ) return -1;
   if( rc > 0 ) return  1;

/* box names's are the same, test component code */
   rc = strcmp(ch1->comp, ch2->comp);
   if( rc < 0 ) return -1;
   if( rc > 0 ) return  1;

   return 0;
}


/*------------------------------------------------------------
   Sample Tremor Version 1 data file named:  P10_20000118_192929.udp
   NOTE: only Accel (PGA) is valid in this version.
   (contents of file are between the lines)
  ------------------------------------------------------------
%#!SendersFileName=P10_20000118_192929.udp
Origin Time     : 2000/01/18 19:29:29.015  UTC
Sta   Str     Accel      Vel     Displ   SP_0.3   SP_1.0   SP_3.0  WA_Amp  Ml100
P10   ADZ :    4.27     0.000   0.0000    0.000    0.000    0.000    0.00   0.00
P10   ADN :   14.89     0.000   0.0000    0.110    0.560    0.000    0.00   0.00
P10   ADE :    7.17     0.000   0.0000    0.120    0.020    0.000    0.00   0.00

  ------------------------------------------------------------
   NOTE: there is a blank line after the 3rd channel's data.
  ------------------------------------------------------------*/

/*------------------------------------------------------------
   Sample Tremor Version 2 data file named:  P13_20010613_174129.udp
   NOTE: Accel, Vel, SP*, tVel are valid in this version.
   (contents of file are between the lines)
  ------------------------------------------------------------
%#!SendersFileName=P13_20010613_174129.udp 
Origin Time     : 2001/06/13 17:41:28.910  UTC 
Sta   Str     Accel       Vel    Displ   SP_0.3   SP_1.0   SP_3.0  WA_Amp  Ml100  tVel 
P13   HNZ :   16.01     0.380   0.0000    0.000    0.000    0.000    0.00   0.00  5.38 
P13   HN2 :  239.15    16.040   0.0000  480.570   94.800    7.640    0.00   0.00  5.39 
P13   HN3 :  182.25    12.460   0.0000  413.360   76.400    5.850    0.00   0.00  5.38 

  ------------------------------------------------------------
   NOTE: there is a blank line after the 3rd channel's data.
  ------------------------------------------------------------*/
