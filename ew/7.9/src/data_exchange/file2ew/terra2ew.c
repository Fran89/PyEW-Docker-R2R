/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: terra2ew.c 4553 2011-08-11 06:17:35Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2002/12/06 01:15:33  dietz
 *     added new instid argument to file2ew_ship call;
 *     changed fprintf(stderr...) calls to logit("e"...)
 *
 *     Revision 1.8  2002/06/05 16:19:44  lucky
 *     I don't remember
 *
 *     Revision 1.7  2001/04/27 18:36:45  dietz
 *     *** empty log message ***
 *
 *     Revision 1.6  2001/04/11 22:56:04  dietz
 *     changed TYPE_STRONGMOTION2 to TYPE_STRONGMOTIONII to match earthworm_global.d
 *
 *     Revision 1.5  2001/03/27 01:09:57  dietz
 *     Added support for reading heartbeat file contents.
 *     file2ewfilter_hbeat() checks for number of COM ports
 *     in heartbeat and complains if it's not the correct number.
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

/*  terra2ew.c
 *  Read a terra-format file, create a Earthworm strongmotion
 *  messages, and place them in a transport ring.  Returns number
 *  of TYPE_STRONGMOTIONII messages written to ring from file.
 */

#include <errno.h>

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

/* Variables assigned thru config file
 *************************************/
#define MAXCHAN  100            /* default maximum # channels            */
int MaxChan = MAXCHAN;          /* configured max# chan (MaxChannel cmd) */
int NchanNames = 0;	        /* number of names in this config file   */
CHANNELNAME *ChannelName = NULL; /* table of box/channel to SCNL         */

static int  NumComPort = 0;     /* number of com ports on acq system     */
static int  PrevComAlive;       /* # active COM ports last in heartbeat  */

#define BUFLEN  65000           /* define maximum size for an event msg  */
static char MsgBuf[BUFLEN];     /* char string to hold output message    */
static unsigned char TypeSM2;   /* message type we'll produce            */

/* Terra-specific definitions
 ****************************/
#define TERRA_CHAN_COUNT  3	/* Number of channels per box            */
#define TERRA_NRSA       10	/* Number of points in response spectrum */

/* Time parsing stuff
*********************/
#define YEAR_LEN 4		/* four digits for year */
#define MONTH_LEN 2		/* two digits for month */
#define DAY_LEN 2		/* two digits for day   */
#define DATE_LEN YEAR_LEN + MONTH_LEN + DAY_LEN
static char YearString[YEAR_LEN+1];
static char MonthString[MONTH_LEN+1];
static char DayString[DAY_LEN+1];

#define HOUR_LEN 2
#define MIN_LEN 2
#define SEC_LEN 2
#define TIM_LEN HOUR_LEN + MIN_LEN + SEC_LEN
static char HourString[HOUR_LEN+1];
static char MinString[MIN_LEN+1];
static char SecString[SEC_LEN+1];

static int Init = 0;   /* initialization flag */

#define TOKEN_LEN 256

/* Functions in this source file
 *******************************/
int  terra_cmpchan( const void *s1, const void *s2 );

/* Functions in other source files
 *********************************/
void logit( char *, char *, ... );         /* logit.c      sys-independent  */
int  GetType ( char *, unsigned char * );  /* getutil.c    sys-independent  */

/******************************************************************************
 * file2ewfilter_com() processes config-file commands to set up the           *
 *                     Terra-specific parameters                              *
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
         logit( "e", "terra2ew_com: must specify <MaxChan> before any"
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
            logit( "e", "terra2ew_com: Error allocating %ld bytes for"
                   " %d chans; exiting!\n",
                   MaxChan*sizeof(CHANNELNAME), MaxChan );
            exit( -1 );
         }
      }
   /* See if we have room for another channel */
      if( NchanNames >= MaxChan ) {
         logit( "e","terra2ew_com: Too many <ChannelName> commands"
                " in configfile; MaxChannel=%d; exiting!\n",
                (int) MaxChan );
         exit( -1 );
      }
   /* Get the box name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>=SM_BOX_LEN ) { 
            logit( "e", "terra2ew_com: box name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].box, str);
      }
   /* Get the channel number */
      ChannelName[NchanNames].chan = k_int();
      if(ChannelName[NchanNames].chan > SM_MAX_CHAN){
         logit( "e", "terra2ew_com: Channel number %d greater "
                "than %d in <ChannelName> cmd; exiting\n",
		 ChannelName[NchanNames].chan,SM_MAX_CHAN);
         exit(-1);
      }
   /* Get the SCNL name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_STA_LEN ) { /* from trace_buf.h */
            logit( "e", "terra2ew_com: station name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].sta, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_CHAN_LEN ) { /* from trace_buf.h */
            logit( "e", "terra2ew_com: component name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].comp, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_NET_LEN ) { /* from trace_buf.h */
            logit( "e", "terra2ew_com: network name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].net, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>=TRACE_LOC_LEN ) { /* from trace_buf.h */
            logit( "e", "terra2ew_com: location name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].loc, str);
      }
      NchanNames++;
      return( 1 );
   } 		

   if( k_its("NumComPort") ) { 
      NumComPort = k_int();
      return( 1 );
   }

   return( 0 );
}

/******************************************************************************
 * file2ewfilter()   Do all the work to convert Terra files to Earthworm msgs *
 ******************************************************************************/
int file2ewfilter( FILE *fp, char *fname )
{
   char        token[TOKEN_LEN];
   char       *nxttok;
   struct Greg boxtime;
   SM_INFO     sm[TERRA_CHAN_COUNT];
   double      t, talt;
   double      second;
   int         altcode;
   CHANNELNAME key;
   int         nch  = 0;
   int         nmsg = 0;
   int         i,j,rc;

/* Initialize variables
 **********************/
   if(!Init) file2ewfilter_init();
   memset( &sm, 0, sizeof(SM_INFO)*TERRA_CHAN_COUNT );
   memset( token, 0, sizeof(token) );
   memset( &key, 0, sizeof(CHANNELNAME) );

   if( fp == NULL ) return( -1 );
   rewind( fp );
	
/* We record the time of this parsing to cover the case where
   the box time is way off, and this is better than nothing.
 *************************************************************/
   talt    = (double)time( (time_t*)NULL );
   altcode = SM_ALTCODE_RECEIVING_MODULE;

/* Line 1: Serial number (not used in TYPE_STRONGMOTIONII)
 ********************************************************/
   rc = fscanf( fp, "SN: %s", token );
   if( rc != 1 ) {
      logit("e","terra2ew: Error: Serial number (SN:) not found\n");
      return( -1 );
   }
   if(Debug) logit("e","Got serial number: %s\n", token );
	
/* Line 2: Box Name (used to look map to SCNL codes)
 ***************************************************/
   rc = fscanf( fp, "\nStation: %s", token );
   if( rc != 1 ) {
      logit("e","terra2ew: Error: Box name (Station:) not found\n");
      return( -1 );
   }
   if( strlen(token)>=SM_BOX_LEN ) {
      logit("e","terra2ew: Error: Box name <%s> too long; max=%d char\n",
             token, SM_BOX_LEN-1 );
      return( -1 );
   }
   strcpy( key.box, token );
   if(Debug) logit("e","Got box name: %s\n", key.box );

/* Line 3: Date
 **************/
   memset( token, 0, sizeof(token) );
   rc = fscanf( fp, "\nDate: %s", token );
   if( rc != 1 ) {
      logit("e","terra2ew: Error: Date: not found\n");
      return( -1 );
   }
   if( strlen(token) != DATE_LEN ) {
      logit("e","terra2ew: Error: Date in .%s. is not %d chars long\n",
             token, DATE_LEN);
      return( -1 );
   }
   if(Debug) logit("e","Got Date: %s\n", token);
	
   nxttok  = token;
   strncpy(YearString, nxttok, YEAR_LEN);
   nxttok += YEAR_LEN;
   strncpy(MonthString, nxttok, MONTH_LEN);
   nxttok += MONTH_LEN;
   strncpy(DayString, nxttok,DAY_LEN);
	
   boxtime.year  = atoi(YearString);
   boxtime.month = atoi(MonthString);
   boxtime.day   = atoi(DayString) ;
   if( boxtime.year<=0 || boxtime.month<=0 || boxtime.day<=0 ) {
      logit("e","terra2ew: Error: Date line contains invalid year,month,day\n");
      return( -1 );
   }

/* Line 4: Time
 **************/
   memset( token, 0, sizeof(token) );
   rc = fscanf( fp, "\nTime: %s", token );
   if( rc != 1 ) {
      logit("","terra2ew: Error: Time: not found\n");
      return( -1 );
   }
   if( strlen(token) != TIM_LEN ) {
      logit("et","terra2ew: Error: Time in .%s. is not %d chars long\n",
             token, TIM_LEN);
      return( -1 );
   }
   if(Debug) logit("e","Got Time: %s\n", token);

   nxttok  = token;
   strncpy(HourString, nxttok, HOUR_LEN);
   nxttok += HOUR_LEN;
   strncpy(MinString,  nxttok, MIN_LEN);
   nxttok += MIN_LEN;
   strncpy(SecString,  nxttok, SEC_LEN);
	
   boxtime.hour   = atoi(HourString);
   boxtime.minute = atoi(MinString);
   second         = atof(SecString);

/* Convert to seconds since 1970 */
   t = 60.0*(double)julmin(&boxtime) + second - GSEC1970;
   if(Debug) {
     time_t tfield = (time_t) t;
     logit("e","Time from message: (%.3lf)  %s",t,ctime(&tfield));
   }

/* OK, we've read the header values that pertain to all the channels.
 * Let's start filling in the SM_INFO structures, one for each channel.
 * Search for SCNL names (from the configuration file) that match the 
 * box name (from Line 2). Each box should have many channels. The user, 
 * we hope, supplied an SCNL name for every channel for this box. We now 
 * stuff all SCNL names for this box into the SM_INFO structs, each SCNL 
 * name in its corresponding structure, along with other header info.
 ***********************************************************************/
   nch=0;   /* channel count */
   for(i=0;i<TERRA_CHAN_COUNT;i++)
   {
      CHANNELNAME *match;
      key.chan = i;
      match = (CHANNELNAME *) bsearch( &key, ChannelName, NchanNames,
                                       sizeof(CHANNELNAME), terra_cmpchan );
      if( match == NULL ) {
         logit("","terra2ew: Error: No ChannelName mapping for"
                  " box:%s ch:%d\n", key.box, key.chan );
         return(-1);
      }
      strncpy( sm[i].sta,  match->sta,  TRACE_STA_LEN  );
      strncpy( sm[i].comp, match->comp, TRACE_CHAN_LEN );
      strncpy( sm[i].net,  match->net,  TRACE_NET_LEN  );
      strncpy( sm[i].loc,  match->loc,  TRACE_LOC_LEN  );
      sm[i].t       = t;
      sm[i].talt    = talt;
      sm[i].altcode = altcode;
      nch++;
   }

   if(Debug) {
      for(i=0;i<nch;i++) {
	 logit("e","Chan Name: %d %s %s %s %s\n",
                i,sm[i].sta,sm[i].comp,sm[i].net,sm[i].loc);
      }
   }

/* Line 5: no real info
 **********************/
   rc = fscanf( fp, "\n%*s Peak Values %s", token );
   if( rc != 1 ) {
      logit("","terra2ew: Error: Peak Values line not found\n");
      return( -1 );
   }

/* Line 6: Peak acceleration for all chans (g)
 *********************************************/
   rc = fscanf( fp, "\n Accel%[^=]= ", token );
   if( rc != 1 ) {
      logit("","terra2ew: Error: Accel line not found\n");
      return( -1 );
   }
   for( i=0;i<TERRA_CHAN_COUNT;i++) {
      fscanf( fp, "%*d(%lf)", &(sm[i].pga) );
      if( rc != 1 ) {
         logit("","terra2ew: Error: PGA for chan[%d] not found\n",i);
         return( -1 );
      }
      sm[i].pga *= GRAVITY_CGS;  /* convert it to cm/sec/sec */
      if(Debug)logit("e","ch[%d].pga= %.3lf cm/s/s\n",i,sm[i].pga);
   }

/* Line 7: Peak Velocity for all chans (g/s)
 *******************************************/
   rc = fscanf( fp, "\n Vel%[^=]= ", token );
   if( rc != 1 ) {
      logit("","terra2ew: Error: Vel line not found\n");
      return( -1 );
   }
   for( i=0;i<TERRA_CHAN_COUNT;i++) {
      fscanf( fp, "%*d(%lf)", &(sm[i].pgv) );
      if( rc != 1 ) {
         logit("","terra2ew: Error: PGV for chan[%d] not found\n",i);
         return( -1 );
      }
      sm[i].pgv *= GRAVITY_CGS;  /* convert it to cm/sec */
      if(Debug)logit("e","ch[%d].pgv= %.3lf cm/s\n",i,sm[i].pgv);
   }

/* Line 8: Peak Displacement for all chans
 *****************************************/
   rc = fscanf( fp, "\n Disp%[^=]= ", token );
   if( rc != 1 ) {
      logit("","terra2ew: Error: Disp line not found\n");
      return( -1 );
   }
   for( i=0;i<TERRA_CHAN_COUNT;i++) {
      fscanf( fp, "%*d(%lf)", &(sm[i].pgd) );
      if( rc != 1 ) {
         logit("","terra2ew: Error: PGD for chan[%d] not found\n",i);
         return( -1 );
      }
      sm[i].pgd *= GRAVITY_CGS;  /* convert it to cm */
      if(Debug)logit("e","ch[%d].pgd= %.3lf cm\n",i,sm[i].pgd);
   }

/* Line 9: no real info
 **********************/
   rc = fscanf( fp, "\n RSA%[^:]:", token );
   if( rc != 1 ) {
      logit("","terra2ew: Error: RSA line not found\n");
      return( -1 );
   }

/* Line 10 and remaining: spectral acceleration
 **********************************************/
   for( j=0; j<TERRA_NRSA; j++) {
      double freq, period;
      rc = fscanf( fp, "\n %lf Hz =", &freq );
      if( rc != 1 ) {
         logit("","terra2ew: Error reading freq from RSA line %d\n",j);
         return( -1 );
      }
      if( freq==0.33 ) freq = 0.333;  /* !FUDGE! so period=3.00 */
      period = 1.0/freq;
      if(Debug)logit("e","RSA at %.2lf sec  ", period );
      for( i=0;i<TERRA_CHAN_COUNT;i++) {
         fscanf( fp, "%*d(%lf)", &(sm[i].rsa[j]) );
         if( rc != 1 ) {
            logit("","terra2ew: Error: RSA[%d] for chan[%d] not found\n",j,i);
            return( -1 );
         }
         sm[i].nrsa     = j+1;
         sm[i].pdrsa[j] = period;
         sm[i].rsa[j]  *= GRAVITY_CGS;  /* convert it to cm/s/s */
         if(Debug)logit("e","ch[%d].RSA[%d]=%.3lf  ",i,j,sm[i].rsa[j]);
      }
      if(Debug)logit("e","\n");
   }

/* Build TYPE_SM msg from each SM_INFO structure and ship it
 ***********************************************************/
   for( i=0;i<TERRA_CHAN_COUNT;i++) {
      rc = wr_strongmotionII( &(sm[i]), MsgBuf, BUFLEN );
      if( rc != 0 ) {
         logit("e","terra2ew: Error building TYPE_STRONGMOTIONII msg\n");
         return( -1 );
      }
      if( file2ew_ship( TypeSM2, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) return( -1 );
      nmsg++;
   }

   return( nmsg );
}


/******************************************************************************
 * file2ewfilter_init()  check arguments for Terra data                       *
 ******************************************************************************/
int file2ewfilter_init( void )
{
   int i,j,chans,ret;

   ret = 0;

/* Look up earthworm message type(s)
 ***********************************/
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeSM2 ) != 0 ) {
      logit( "e",
             "terra2ew: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n" );
      ret = -1;
   }

/* Make sure that some box->SCNL lines have been entered
 *******************************************************/
   if( NchanNames == 0 ) {
      logit("e","terra2ew: ERROR: no <ChannelName> commands were provided\n" );
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
      if(chans != TERRA_CHAN_COUNT) {
         logit("e","terra2ew: ERROR: Box name %s has %d channels "
               "defined; %d required!\n",
                ChannelName[i].box, chans, TERRA_CHAN_COUNT);
         ret = -1;
      }
   }

/* Sort channel name list for future bsearches
 *********************************************/
   qsort( ChannelName, NchanNames, sizeof(CHANNELNAME), terra_cmpchan );

/* Make sure we know how many COM ports to expect in heartbeat file
 ******************************************************************/
   if( NumComPort == 0 ) {
      logit("e","terra2ew: ERROR: no <NumComPort> command was provided\n" );
      ret = -1;
   }
   PrevComAlive = NumComPort;  /* assume everything's up */

   Init = 1;
   return( ret );
}


/******************************************************************************
 * file2ewfilter_hbeat()  read heartbeat file, check for trouble              *
 ******************************************************************************/
int file2ewfilter_hbeat( FILE *fp, char *fname, char *sysname )
{
   char line[50];
   int  ncom = 0;
   int  i, rc;

/*--------------------------------------------------------------
A heartbeat file from the Terra/PG&E system should contain 1 line 
per active COM port (not necessarily in any order).  We'll complain
if the total # of lines doesn't match the expected number.  Here's
an example heartbeat file for a 10 port system:
2001/03/23 23:59:21 COM8
2001/03/23 23:59:21 COM6
2001/03/23 23:59:21 COM3
2001/03/23 23:59:21 COM5
2001/03/23 23:59:21 COM9
2001/03/23 23:59:21 COM7
2001/03/23 23:59:21 COM4
2001/03/23 23:59:21 COM10
2001/03/23 23:59:22 COM2
2001/03/23 23:59:22 COM1
 --------------------------------------------------------------*/

   if( fp == NULL ) return( -1 );
   rewind( fp );

   while( fgets( line, 50, fp ) != NULL ) {
      rc = sscanf( line, "%*s %*s COM%d", &i );
      ncom += rc;
   }

/* Too few lines in heartbeat file - complain if we haven't already 
 ******************************************************************/
   if( ncom != NumComPort ) {
      if( ncom != PrevComAlive )  {
         sprintf( Text, 
                 "please reboot %s - only %d of %d COM ports are active!",
                  sysname, ncom, NumComPort );
         file2ew_status( TypeError, ERR_PEER_HBEAT, Text );
      }
   }

/* Heartbeat file looks good - say so if we previously complained 
 ****************************************************************/
   else if( PrevComAlive != NumComPort ) {
      sprintf( Text, "%s is OK again - all COM ports are active!", sysname );
      file2ew_status( TypeError, ERR_PEER_HBEAT, Text );
   }

/* Store the number of active COM ports 
 **************************************/
   PrevComAlive = ncom;

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
 * terra_cmpchan() a function passed to qsort; used to sort an array    *
 *   of CHANNELNAME struct by boxname & channel#                        *
 ************************************************************************/
int terra_cmpchan( const void *s1, const void *s2 )
{
   CHANNELNAME *ch1 = (CHANNELNAME *) s1;
   CHANNELNAME *ch2 = (CHANNELNAME *) s2;
   int  rc;

/* Test box name */
   rc = strcmp(ch1->box, ch2->box);
   if( rc < 0 ) return -1;
   if( rc > 0 ) return  1;

/* box names's are the same, test channel # */
   if( ch1->chan > ch2->chan ) return -1;
   if( ch1->chan < ch2->chan ) return  1;

   return 0;
}

/*------------------------------------------------------------
   Sample Terra data file named:  WIG_20000108_000013.txt
   (contents of file are between the lines)
  ------------------------------------------------------------
SN: 117
Station: WIG
Date: 20000108
Time: 000013
  ***** Peak Values ******
  Accel(g    )   =   132688(  0.126541)   131448(  0.125359)   130164(  0.124134)
  Vel(g    -s)   =     9392(  0.008957)     8978(  0.008562)     8971(  0.008555)
  Disp(g    -s-s)=     1548(  0.001476)     1471(  0.001403)     1500(  0.001431)
  RSA(g    ):
  10.00 Hz       =   286480(  0.273209)   279136(  0.266205)   272219(  0.259608)
   6.67 Hz       =   285920(  0.272675)   278144(  0.265259)   271321(  0.258752)
   5.00 Hz       =   285616(  0.272385)   277920(  0.265045)   271008(  0.258453)
   3.33 Hz       =   285648(  0.272415)   278000(  0.265121)   270751(  0.258208)
   2.50 Hz       =   285248(  0.272034)   277312(  0.264465)   270554(  0.258020)
   2.00 Hz       =   282256(  0.269180)   274496(  0.261780)   267352(  0.254967)
   1.33 Hz       =   284016(  0.270859)   277168(  0.264328)   269600(  0.257111)
   1.00 Hz       =   282480(  0.269394)   275440(  0.262680)   267871(  0.255462)
   0.50 Hz       =   283568(  0.270432)   272112(  0.259506)   268176(  0.255753)
   0.33 Hz       =   276944(  0.264114)   273712(  0.261032)   265400(  0.253105)
  ------------------------------------------------------------*/
