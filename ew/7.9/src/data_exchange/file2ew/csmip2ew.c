
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: csmip2ew.c 1153 2002-12-06 01:15:33Z dietz $
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
 *     Revision 1.8  2001/04/11 22:56:04  dietz
 *     changed TYPE_STRONGMOTION2 to TYPE_STRONGMOTIONII to match earthworm_global.d
 *
 *     Revision 1.7  2001/03/27 01:09:57  dietz
 *     Added support for reading heartbeat file contents.
 *     Currently file2ewfilter_hbeat() is a dummy function.
 *
 *     Revision 1.6  2001/02/08 16:36:02  dietz
 *     Added file2ewfilter_com function to read config file commands
 *     and file2ewfilter_shutdown() to cleanup before exit.
 *
 *     Revision 1.5  2000/12/08 19:11:58  dietz
 *     Added conversion of pseudo-velocity spectral values to pseudo-acceleration
 *     spectral values.
 *
 *     Revision 1.4  2000/10/20 18:47:02  dietz
 *     *** empty log message ***
 *
 *     Revision 1.3  2000/09/26 22:59:05  dietz
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

/*  csmip2ew.c
 *  Read a CSMIP-format file, create Earthworm strongmotion
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

/* Globals from file2ew.c
 ************************/
extern int Debug;

/* Variables assigned thru config file
 *************************************/
#define MAXCHAN  100            /* default maximum # channels            */
int MaxChan = MAXCHAN;          /* configured max# chan (MaxChannel cmd) */
int NchanNames = 0;	        /* number of names in this config file   */
CHANNELNAME *ChannelName = NULL; /* table of box/channel to SCNL         */

#define BUFLEN  65000           /* define maximum size for an event msg  */
static char MsgBuf[BUFLEN];     /* char string to hold output message    */
static unsigned char TypeSM2;   /* message type we'll produce            */

/* CSMIP-specific definitions
 ****************************/
#define CSMIP_CHAN_COUNT  1	/* Number of channels per box            */
#define CSMIP_NRSA        3	/* Number of points in response spectrum */

static int Init = 0;            /* initialization flag */

#define TOKEN_LEN    256
#define CM_PER_INCH 2.54   /* conversion Spectral velocity in/s -> cm/s  */
#define TWO_PI 6.2831853   /* 2*PI, used in converting Spectral Velocity */
                           /*    to pseudo spectral acceleration         */

/* Functions in this source file
 *******************************/
int  csmip_cmpchan( const void *s1, const void *s2 );

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

/* Reset the maximum number of channels
 **************************************/
   if( k_its("MaxChannel") ) { /*optional*/
      MaxChan = k_int();
      if( ChannelName != NULL ) {
         logit( "e","csmip2ew_com: must specify <MaxChan> before any"
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
            logit( "e", "csmip2ew_com: Error allocating %ld bytes for"
                   " %d chans; exiting!\n",
                   MaxChan*sizeof(CHANNELNAME), MaxChan );
            exit( -1 );
         }
      }
   /* See if we have room for another channel */
      if( NchanNames >= MaxChan ) {
         logit( "e", "csmip2ew_com: Too many <ChannelName> commands"
                " in configfile; MaxChannel=%d; exiting!\n",
                (int) MaxChan );
         exit( -1 );
      }
   /* Get the box name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>=SM_BOX_LEN ) { 
            logit( "e", "csmip2ew_com: box name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].box, str);
      }
   /* Get the channel number */
      ChannelName[NchanNames].chan = k_int();
      if(ChannelName[NchanNames].chan > SM_MAX_CHAN){
         logit( "e", "csmip2ew_com: Channel number %d greater "
                "than %d in <ChannelName> cmd; exiting\n",
		 ChannelName[NchanNames].chan,SM_MAX_CHAN);
         exit(-1);
      }
   /* Get the SCNL name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_STA_LEN ) { /* from trace_buf.h */
            logit( "e", "csmip2ew_com: station name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].sta, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_CHAN_LEN ) { /* from trace_buf.h */
            logit( "e", "csmip2ew_com: component name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].comp, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_NET_LEN) { /* from trace_buf.h */
            logit( "e", "csmip2ew_com: network name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].net, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_LOC_LEN) { /* from trace_buf.h */
            logit( "e", "csmip2ew_com: location name <%s> too long"
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
 * file2ewfilter()   Do all the work to convert CSMIP files to Earthworm msgs *
 ******************************************************************************/
int file2ewfilter( FILE *fp, char *fname )
{
   char         token[TOKEN_LEN+1];
   CHANNELNAME  key;
   CHANNELNAME *match;
   SM_INFO      sm;
   struct Greg  boxtime;
   double       p[CSMIP_NRSA],sv[CSMIP_NRSA];
   int          nmsg = 0;
   int          i,rc;

/* Initialize variables
 **********************/
   if(!Init) file2ewfilter_init();
   memset( &sm,      0, sizeof(SM_INFO) );
   memset( &boxtime, 0, sizeof(struct Greg) );

   if( fp == NULL ) return( -1 );

/* We record the time of this parsing to cover the case where
   the box time is way off, and this is better than nothing.
 *************************************************************/
   sm.talt    = (double)time( (time_t*)NULL );
   sm.altcode = SM_ALTCODE_RECEIVING_MODULE;

/* Line 1:  PGA, PGV, time, box serial number
   This file contains the largest peak of any of the channels;
   you don't know which channel recorded this peak.
 *************************************************************/
   rc = fscanf( fp,
               "%lfg %lfcm/s %d:%d,%d/%d/%d; CSMIP Sta %s %*[^\n]\n",
                &(sm.pga), &(sm.pgv),
                &(boxtime.hour), &(boxtime.minute), &(boxtime.month),
                &(boxtime.day), &(boxtime.year),  token );
   if( rc != 8 ) {
      logit("e","csmip2ew: Error reading line 1: got %d items, expected 8\n",
             rc );
      return( -1 );
   }

/* convert time to sec since 1970; KLUDGE for 2-digit years!!!! */
   if( boxtime.year >= 70 ) boxtime.year += 1900;
   else                     boxtime.year += 2000;
   sm.t = 60.0*(double)julmin(&boxtime) - GSEC1970;
   if(Debug) {
     time_t tfield = (time_t) sm.t;
     logit("e","Time from file: (%.3lf)  %s",sm.t,ctime(&tfield));
   }

/* Find SCNL mapping for this token(box) */
   if( strlen(token)>=SM_BOX_LEN ) {
      logit("e","csmip2ew: Error: serial number %s too long\n",token);
      return( -1 );
   }
   strcpy( key.box, token );
   if(Debug) logit("e","Got serial number: %s\n",key.box);

   match = (CHANNELNAME *) bsearch( &key, ChannelName, NchanNames,
                                     sizeof(CHANNELNAME), csmip_cmpchan );
   if( match == NULL ) {
      logit("","csmip2ew: Error: No ChannelName mapping for"
               " box:%s\n", key.box );
      return(-1);
   }
   strncpy( sm.sta,  match->sta,  TRACE_STA_LEN  );
   strncpy( sm.comp, match->comp, TRACE_CHAN_LEN );
   strncpy( sm.net,  match->net,  TRACE_NET_LEN  );
   strncpy( sm.loc,  match->loc,  TRACE_LOC_LEN  );

/* convert data values to proper units */
   sm.pga *= GRAVITY_CGS;  /* convert from g to cm/s/s */

/* Line 2:  Spectral Velocity at 5% damping (which channel???),
   in inches/s??? We want Spectral Acceleration in cgs!
   From PSV, scale by (2 * PI * Freq) to convert to PSA.
   And make sure to convert from inches/s to cgs!
 *************************************************************/
   rc = fscanf( fp, "Sv@5%*c(%lf,%lf,%lfs): %lf %lf %lf in/s",
                &p[0], &p[1], &p[2], &sv[0], &sv[1], &sv[2] );
   if( rc != 6 ) {
      logit("e","csmip2ew: Error reading line 2: got %d items, expected 6\n",
             rc );
      return( -1 );
   }
   for( i=0; i<CSMIP_NRSA; i++ ) {
     double freq = 1.0/p[i];
     sm.pdrsa[i] = p[i];
     sm.rsa[i]   = sv[i] * TWO_PI * freq * CM_PER_INCH;
   }
   sm.nrsa = CSMIP_NRSA;
	
/* Build TYPE_SM2 msg from last SM_INFO structure and ship it
 ************************************************************/
   rc = wr_strongmotionII( &sm, MsgBuf, BUFLEN );
   if( rc != 0 ) {
      logit("e","csmip2ew: Error building TYPE_STRONGMOTIONII msg\n");
      return( -1 );
   }
   if( file2ew_ship( TypeSM2, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) return( -1 );
   nmsg++;

   return( nmsg );
}


/******************************************************************************
 * file2ewfilter_init()  check arguments for CSMIP data                       *
 ******************************************************************************/
int file2ewfilter_init( void )
{
   int chans;
   int i,j;
   int ret=0;


/* Look up earthworm message type(s)
 ***********************************/
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeSM2 ) != 0 ) {
      logit( "e",
             "csmip2ew: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n" );
      ret = -1;
   }

/* Make sure that some box->SCNL lines have been entered
 *******************************************************/
   if( NchanNames == 0 ) {
      logit("e","csmip2ew_init: ERROR: no ChannelName commands were provided\n" );
      ret = -1;
   }

/* Sort channel name list for future bsearches
 *********************************************/
   qsort( ChannelName, NchanNames, sizeof(CHANNELNAME), csmip_cmpchan );

/* Verify that we have correct # channel names for each box.
 ***********************************************************/
   for(i=0;i<NchanNames;i++) {
      chans=0;  /* the number of channels defined for box i */
      for(j=0;j<NchanNames;j++){
         if(strcmp(ChannelName[i].box,ChannelName[j].box)==0){
            chans++;
         }
      }
      if(chans != CSMIP_CHAN_COUNT) {
         logit("e","csmip2ew_init: ERROR: Box name %s has %d channels "
               "defined; %d required!\n",
               ChannelName[i].box, chans, CSMIP_CHAN_COUNT);
         ret = -1;
      }
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
   free( ChannelName );
   return;
}

/************************************************************************
 * csmip_cmpchan() a function passed to qsort; used to sort an array    *
 *   of CHANNELNAME struct by boxname                                   *
 ************************************************************************/
int csmip_cmpchan( const void *s1, const void *s2 )
{
   CHANNELNAME *ch1 = (CHANNELNAME *) s1;
   CHANNELNAME *ch2 = (CHANNELNAME *) s2;
   int  rc;

/* Test box name */
   rc = strcmp(ch1->box, ch2->box);
   if( rc < 0 ) return -1;
   if( rc > 0 ) return  1;

   return 0;
}


/*------------------------------------------------------------
   Sample CSMIP data file named:  apk_msg_021000_083329
   (the contents of file are between the lines)
  ------------------------------------------------------------
 .030g  .22cm/s 16:31,02/10/00; CSMIP Sta 13702 33.979N,117.373W Riverside
Sv@5%(.3,1,3s):   .075   .066   .064 in/s
  ------------------------------------------------------------*/
