
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tankplayer.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.18  2010/03/19 16:59:35  paulf
 *     made tankplayer check size against MAX_TRACEBUF_SIZ for tracebuf2s
 *
 *     Revision 1.17  2009/03/13 20:01:44  paulf
 *     added new field InterMessageDelayMillisecs for rapid fire loading of tanks
 *
 *     Revision 1.16  2008/09/09 20:02:47  paulf
 *     upgraded tankplayer to use GetFromDir as an option to load tank files as they are created
 *
 *     Revision 1.15  2008/08/22 20:21:19  paulf
 *     increased MAX_WF to 1000 tanks
 *
 *     Revision 1.14  2007/12/16 17:52:33  paulf
 *     fixed SwapLong for SwapInt in tankplayer for some OS
 *
 *     Revision 1.13  2007/03/28 18:04:54  paulf
 *     removed incl of malloc.h since it was in platform.h
 *
 *     Revision 1.12  2007/02/26 14:47:38  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.11  2007/02/23 15:16:42  paulf
 *     fixed long to time_t declaration
 *
 *     Revision 1.10  2006/07/01 03:29:06  stefan
 *     SendLate command in tankplayer.d did not appear to be working.
 *     The problem is that while tankplayer will play back 3 waveform types
 *     (TypeADBuf, TypeTraceBuf, TypeTraceBuf2) it would only set the time
 *     if the type was one of the first two.
 *     set_time had code else if( msgtype == TypeTraceBuf )
 *     - John Patton
 *     changed to
 *     else if( msgtype == TypeTraceBuf ||  msgtype == TypeTraceBuf2)
 *     - Stefan
 *
 *     Revision 1.9  2006/03/09 17:18:55  davek
 *     removed a c++ style comment.
 *
 *     Revision 1.8  2006/03/09 17:07:28  davek
 *     Added support for TypeTraceBuf2, which was missing from the get_time function
 *     that is used to determine the time of the packet from the trace header.
 *     Changed the program's behavior when Tracebuf2 packets are encountered with
 *     VERSION fields that don't match those compiled into the program.  Previously
 *     the program always quit at the first sign of a tracebuf with a mismatched version
 *     number.  Now depending on the value of the IgnoreTBVersionNumbers config command,
 *     the program attempts to parse and either play or skip the message based on
 *     the value of the command.  (TRUE = play, FALSE= ignore).
 *     Added a log message when tankplayer comes across a large delay between packets.
 *     Modified the sleep code that executes between packets, so that it sleeps
 *     in small intervals and continues to issue heartbeats even during a large gap.
 *     Added additional debugging info for dealing with problems encountered in tracebuf files.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:36  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.7  2004/05/21 23:56:07  dietz
 *     modified to play either TYPE_TRACEBUF or TYPE_TRACEBUF2 files
 *
 *     Revision 1.6  2004/05/18 22:31:08  lombard
 *     Modified for location code
 *
 *     Revision 1.5  2001/10/01 20:13:48  patton
 *     Made changes due to slightly reworked logit.
 *     JMP 10/1/2001
 *
 *     Revision 1.4  2001/08/28 05:20:34  patton
 *     Made logit changes due to new logit code.
 *     JMP 8/27/2001
 *
 *     Revision 1.3  2001/04/17 16:35:26  davidk
 *     Added fixes to eliminate compiler warnings on NT.
 *     (Added param to exit call, added an explicit typecast.)
 *
 *     Revision 1.2  2000/08/08 17:04:13  lucky
 *     Lint cleanup.
 *
 *     Revision 1.1  2000/02/14 19:41:59  lucky
 *     Initial revision
 *
 *
 */

   /*********************************************************************
    *                           tankplayer.c                            *
    *                                                                   *
    *  Program to read a waveform file & play it into a transport ring. *
    *  Reads files containing TYPE_ADBUF, TYPE_TRACEBUF, or             *
    *  TYPE_TRACEBUF2 msgs.                                             *
    *  Places messages into ring in pseudo-real-time, using the         *
    *  timestamps in the message headers to determine the temporal      *
    *  spacing of messages.                                             *
    *    For TYPE_ADBUF msgs, tankplayer spaces msgs by the delta-t     *
    *                    between the starttimes in the headers,         *
    *    For TYPE_TRACEBUF & TYPE_TRACEBUF2, tankplayer spaces msgs by  *
    *                    the delta-t between the endtimes in headers.   *
    *                                                                   *
    *  Command line arguments:                                          *
    *     arvg[1] = tankplayer's configuration file                     *
    *********************************************************************/
/* Modified for Y2K testing: PNL, 2/17/99 */

/* added VERSION number 1.0.2  on Nov 5, 2014 */
#define VERSION "1.0.3 2016.07.01"

#define NAM_LEN 256         /* length of full directory name          */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <data_buf.h>
#include <trace_buf.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <time_ew.h>
#include <transport.h>

#define MAX_BUFSIZ   51740

/* Function prototypes
 *********************/
void tankplayer_config( char * );
void tankplayer_lookup( void );
void tankplayer_status( unsigned char, short, char * );
double get_time( char *, unsigned char );
void set_time( char *, unsigned char, double );

static SHM_INFO      Region;         /* Info structure for memory region  */
pid_t                MyPID=0;        /* to use in heartbeat               */

/* Things read from configuration file
 *************************************/
static long          RingKey;        /* transport ring to attach to       */
static int           LogSwitch;      /* Log-to-disk switch                */
static unsigned char MyModId;        /* play as this module id            */
static long          HeartBeatInt;   /* seconds between heartbeats        */
static unsigned char PlayType;       /* read this msgtype from wavefile   */
static int           Pause;          /* # sec to sleep between wave files */
static long          StartUpDelay;   /* seconds to wait before playing    */
static int           ScreenMsg = 0;  /* =1 write info to screen, =0 don't */
static int           Debug = 0;
static int           AdjTime = 0;    /* =1 to adjust packet time, =0 don't*/
static double        LateTime = 0.0; /* seconds earlier than present to   */
                                     /*  put in packet timestamp.         */
static int           bBeSuperLenient = 0; /* be lenient about putting out
                                            tracebufs with incorrect
                                            version numbers.              */

static int       InterMessageDelayMillisecs = 0; /* the flag and number of ms to sleep between 
							stuffing in tbufs, regardless of header time stamp!  
							If this is set to 0, then all other delay settings apply
							*/
static char      GetFromDir[NAM_LEN]; /* directory to monitor for data     */
static unsigned  CheckPeriod = 1;         /* secs between looking for new files*/
static int       OpenTries = 5;
static int       OpenWait= 200;		/* milliseconds between open tries */
static int       SaveDataFiles=1;       /* if non-zero, move to SaveSubdir,  */
                                      /*           0, remove files         */
static char *TroubleSubdir = "trouble";   /* subdir for problem files    */
static char *SaveSubdir = "save";         /* subdir for processed files  */

#define MAX_WF  1000                      /* max # waveform files to play   */
#define MAX_LEN 1024
static char WaveFile[MAX_WF][MAX_LEN];  /* list of waveform files to play */
static int  nWaveFile = 0;              /* # waveform files to play       */

/* Look up from earthworm.h
 **************************/
static unsigned char InstId;         /* local installation id             */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeADBuf;
static unsigned char TypeTraceBuf;
static unsigned char TypeTraceBuf2;


int main( int argc, char *argv[] )
{
  static char   msg[MAX_BUFSIZ];  /* waveform data buffer read from file   */
  WF_HEADER    *adhead;           /* pntr to header of TYPE_ADBUF msg      */
  TRACE2_HEADER *wfhead;          /* pntr to header of TYPE_TRACEBUF2 msg  */
  MSG_LOGO      logo;             /* logo to attach to waveform msgs       */
  FILE         *fp;               /* file of waveform data to read from    */
  double        CurrentTime;      /* current system time                   */
  double        Ptime = 0.0;      /* original time stamp on packet         */
  double        offsetTime = 0.0; /* Difference between Ptime and CurrentTime */
  double        lastdot=0.;       /* time last dot was written to screen   */
  double        wait;             /* Seconds to wait before sending pkt    */
  time_t        itime;            /* integer version of double times       */
  time_t        timeNow;          /* current system time                   */
  time_t        timeLastBeat;     /* system time of last heartbeat sent    */
  short         nchan;            /* #channels in this message             */
  short         nscan;            /* #scans in this message                */
  int           nsamp;            /* #samples in this message              */
  unsigned char module = 0;       /* source module of this waveform msg    */
  char          byte_order;       /* byte order of this trace msg          */
  int           byte_per_sample;  /* for trace msg                         */
  size_t        size = 0;
  int           first;            /* flag 1st waveform message of file     */
  int           iw, i;
  char          lo[2];  /* logit arg1: ""  if ScreenMsg=0; "o"  otherwise  */
  char          lot[3]; /* logit arg1: "t" if ScreenMsg=0; "ot" otherwise  */
  int           iFileOffset, iLastRead;/* tracking where we are in the     */
                                       /* current file - Debug info only   */
  int           bIsCorrectVersion=1;   /* recording match of version info  */
  int 		continue_processing=1; /* switch to continue looking for more files or processing listed ones */

  char          fname[NAM_LEN];		/* filename grabbed from polling dir */
  char          fnew[NAM_LEN+25];	/* filename for moving to save dir */

  char          *current_file;		/* a pointer to the currently named tank file */
  char          *ProgName;
  int 		result; 		/* processing result  -1 == failure */

  adhead  = ( WF_HEADER *)&msg[0];
  wfhead =  ( TRACE2_HEADER *)&msg[0];


  /* Check arguments
  *****************/
  if ( argc != 2 )
  {
    fprintf( stdout, "Usage: tankplayer configfile\n" );
    fprintf( stdout, "Version: %s\n", VERSION );
    return( 0 );
  }
 /* Initialize name of log-file & open it
  ***************************************/
  logit_init( argv[1], 0, 512, 1 );

  GetFromDir[0]=0;

  ProgName=argv[0];


  /* Read configuration file
  *************************/
  tankplayer_config( argv[1] );

  logit_init( argv[1], 0, 512, LogSwitch );
  logit( "" , "tankplayer: Read command file <%s>\n", argv[1] );
  logit( "" , "tankplayer: Version %s\n", VERSION );

  if (nWaveFile> 0 && GetFromDir[0] != 0) {
     logit( "e", "%s: You can only use GetFromDir OR WaveFile modes individually, not together; "
                 "exiting!\n", ProgName);
      return( -1 );
  }

  /* Look up important info from earthworm.h tables
  ************************************************/
  tankplayer_lookup();

  /* Store my own processid
  ************************/
  MyPID = getpid();

  /* Set up out-going logo
  ***********************/
  if( PlayType!=TypeADBuf     &&
      PlayType!=TypeTraceBuf  &&
      PlayType!=TypeTraceBuf2    )
  {
    logit( "e", "tankplayer: Not programmed to process msg type <%d>",
	   (int) PlayType );
    logit( "e", "; exiting!\n" );
    return( 1 );
  }
  logo.instid = InstId;
  logo.mod    = MyModId;
  logo.type   = PlayType;

  /* Attach to transport ring
  **************************/
  tport_attach( &Region, RingKey );
  logit( "", "tankplayer: Attached to public memory region: %d\n",
	 RingKey );

  /* Force a heartbeat to be issued in first pass thru main loop
  *************************************************************/
  timeLastBeat = time(&timeNow) - HeartBeatInt - 1;

  /* Set up logit's first argument based on value of ScreenMsg
  ***********************************************************/
  if( ScreenMsg==0 ) { strcpy( lo, "" );  strcpy( lot, "t" );  }
  else               { strcpy( lo, "o" ); strcpy( lot, "ot" ); }

  /* Hold off sending data for a specified bit while system staggers up
  ********************************************************************/
  logit( lo, "tankplayer: startup. Waiting %d seconds\n",StartUpDelay);
  for( i=0; i<StartUpDelay; i++ )
  {
    sleep_ew( 1000 );
    if( time(&timeNow)-timeLastBeat >= HeartBeatInt )
    {
      timeLastBeat = timeNow;
      tankplayer_status( TypeHeartBeat, 0, "" );
    }
  }
  if( nWaveFile == 0 && chdir_ew( GetFromDir ) == -1 )
  {
     logit( "e", "%s: GetFromDir directory <%s> not found; "
                 "exiting!\n", ProgName, GetFromDir );
      return( -1 );
  }
  if (GetFromDir[0]!=0 && Debug) logit("et","%s: changed to directory <%s>\n", ProgName,GetFromDir);

  if (nWaveFile==0) {
   /* Make sure trouble subdirectory exists
    ***************************************/
   if( CreateDir( TroubleSubdir ) != EW_SUCCESS ) {
      logit( "e", "%s: trouble creating trouble directory: %s/%s\n",
              ProgName, GetFromDir, TroubleSubdir );
      return( -1 );
   }

   /* Make sure save subdirectory exists (if it will be used)
    *********************************************************/
   if( SaveDataFiles ) {
      if( CreateDir( SaveSubdir ) != EW_SUCCESS ) {
         logit( "e", "%s: trouble creating save directory: %s/%s\n",
                ProgName, GetFromDir, SaveSubdir );
      return( -1 );
      }
   }
  }

  /*--------- Main Loop: over list of waveform files to play----------------*/
  iw=-1;
  while (continue_processing)
  {
    if (nWaveFile > 0) {
       /* we are processing via listed files */
       iw++;
       if (iw==nWaveFile) {
	  /* we have reached the end! */
          break;
       }
       /* Open a listed waveform file
    	********************/
       fp = fopen( WaveFile[iw], "rb" );
       if ( fp == NULL ) {
          logit( "e", "tankplayer: Cannot open tank file <%s>\n", WaveFile[iw] );
          continue;
        }
        if (Debug > 0 ) {
          logit("e", "tankplayer: starting tank <%s>\n", WaveFile[iw] );
        }
        current_file = WaveFile[iw];
    } else {
        /* open the file in the directory specified */
	while (1) {
            if( tport_getflag( &Region ) == TERMINATE || tport_getflag( &Region ) == MyPID ) {
              logit( lot, "tankplayer: Termination requested; exiting!\n" );
              tport_detach( &Region );
              return( 0 );
            }
            if( time(&timeNow)-timeLastBeat >= HeartBeatInt )
            {
	      timeLastBeat = timeNow;
	      tankplayer_status( TypeHeartBeat, 0, "" );
            }
            if( GetFileName(fname) == 1 ) {  /* No files found; wait for one to appear */
              sleep_ew( CheckPeriod*1000 );
              continue;
            } else {
              if(Debug)logit("et","%s: got file name <%s>\n",ProgName,fname);
              break;
            }
        }
        /* we should have a file at this point */
        current_file = fname;
        /* Open the file.
         * We open for updating (even though we only want to read it),
         * as that will hopefully get us an exclusive open.
         * We don't ever want to look at a file that's being written to.
         ***************************************************************/
         fp = NULL;
         for( i=0; i<OpenTries; i++ ) {
            fp = fopen( fname, "rb+" );
            if( fp != NULL ) break;
            sleep_ew( OpenWait );
         }
         if( fp == NULL ) { /* failed to open file! */
            logit( "et","%s: Error: Could not open %s after %d*%d msec.",
                   ProgName, fname, OpenTries, OpenWait);
            result = -1;
            goto ProcessedFile;
         }
         if( i>0 ) {
            logit("t","Warning: %d attempts required to open file %s\n",
                   i+1, fname);
         }
    }
    iFileOffset = 0;
    iLastRead   = 0;
    /* See if it's time to stop
    ************************/
    if( tport_getflag( &Region ) == TERMINATE || tport_getflag( &Region ) == MyPID ) {
      logit( lot, "tankplayer: Termination requested; exiting!\n" );
      tport_detach( &Region );
      return( 0 );
    }

    first = 1;

    /* Loop over one file: get a msg from file, write it to ring
    *********************************************************/
    while( tport_getflag( &Region ) != TERMINATE && tport_getflag( &Region ) != MyPID )
    {
      /* Send tankplayer's heartbeat
      *****************************/
      if( time(&timeNow)-timeLastBeat >= HeartBeatInt )
      {
	timeLastBeat = timeNow;
	tankplayer_status( TypeHeartBeat, 0, "" );
      }

      /* Read ADBuf waveform message from tank
      ***************************************/
      if( PlayType == TypeADBuf )
      {
	/* Read the header
	*****************/
	size = sizeof( WF_HEADER );

  iFileOffset += iLastRead;
	if( fread( msg, sizeof(char), size, fp ) < size )  break;
  iLastRead =  (int)(sizeof(char) * size);

	nchan  = adhead->nchan;
	nscan  = adhead->nscan;
	module = adhead->mod_id;
#ifdef _SPARC
	SwapShort( &nscan );  /* Note: By definition, TYPE_ADBUF msgs */
	SwapShort( &nchan );  /*       are always in INTEL byte order */
#endif
	adhead->mod_id = MyModId; /*relabel mod_id in msg header*/

	/* Read pin #'s and data samples from file
	***************************************/
	size = sizeof(short) * nchan * (nscan+1);
	if( size + sizeof(WF_HEADER) > MAX_BUFSIZ ) {
	  logit( "e",
		 "tankplayer: msg[%d] adtype overflows internal buffer[%d]\n",
		 size+sizeof(WF_HEADER), MAX_BUFSIZ );
	  exit(-1);  /* DK 04/12/01 added param to exit() call */
	}
  iFileOffset += iLastRead;
	if( fread( &msg[sizeof(WF_HEADER)], sizeof(char), size, fp ) < size )
	  break;
  iLastRead =  (int)(sizeof(char) * size);
  size += sizeof( WF_HEADER );
      }

      /* Read TraceBuf waveform message from file
      ******************************************/
      else if( PlayType == TypeTraceBuf2 ||
               PlayType == TypeTraceBuf     )
      {
	/* Read the header
	***************/
	size = sizeof( TRACE2_HEADER );

  iFileOffset += iLastRead;
	if( fread( msg, sizeof(char), size, fp ) < size )
    break;
  iLastRead =  (int)(sizeof(char) * size);

	nsamp           = wfhead->nsamp;
	byte_order      = wfhead->datatype[0];
	byte_per_sample = atoi(&wfhead->datatype[1]);
	module          = 0;
#ifdef _SPARC
	if( byte_order == 'i' || byte_order == 'f' ) SwapInt( &nsamp );
#endif
#ifdef _INTEL
	if( byte_order == 's' || byte_order == 't' ) SwapInt( &nsamp );
#endif

  /* test for version; skip file if wrong type
  *******************************************/
  if( PlayType == TypeTraceBuf2 )
  {
    if( !TRACE2_HEADER_VERSION_IS_VALID(wfhead) )
    {
      if(!bBeSuperLenient)
        logit( "e", "tankplayer: Error: packet (%s,%s,%s) at file %s offset %d is not the correct TYPE_TRACEBUF2 version (%c%c).  \n"
          "Parsing it as one and attempting to continue.  Will not output this packet!\n",
          wfhead->sta, wfhead->chan, wfhead->net,
          current_file, iFileOffset, wfhead->version[0], wfhead->version[1] );
      bIsCorrectVersion=0;
    }
    else
    {
      bIsCorrectVersion=1;
    }
  }

	/* Read data samples from file
	***************************/
	size = byte_per_sample * nsamp;
	if( size + sizeof(TRACE2_HEADER) > MAX_TRACEBUF_SIZ ) {
	  logit( "e",
		 "tankplayer: msg[%d] tbuf2 overflows internal buffer[%d]\n",
		 size+sizeof(TRACE2_HEADER), MAX_TRACEBUF_SIZ );
	  break;
	}

  iFileOffset += iLastRead;
  if( fread( &msg[sizeof(TRACE2_HEADER)], sizeof(char), size, fp ) < size )
    break;
  iLastRead =  (int)(sizeof(char) * size);


	size += sizeof( TRACE2_HEADER );
      }

      /* Sleep until it's time to send this message
      ********************************************/
      Ptime = get_time( msg, PlayType );
      hrtime_ew( &CurrentTime );

      if( first ) {
	offsetTime = CurrentTime - Ptime;
        first = 0;
	logit( lo, "\n" );
	logit( lo,
	       "tankplayer:  Reading    type:%3d  mod:%3d  from <%s>\n",
	       (int)PlayType, (int)module, current_file );
	logit( lo,
	       "             Playing as type:%3d  mod:%3d  inst:%3d\n",
	       (int)PlayType, (int)MyModId, (int)InstId );
        if ( AdjTime )
        {
          logit( lot, "time shifted by %lf seconds from original\n", offsetTime - LateTime );
          itime = (time_t) (Ptime + offsetTime - LateTime);
        }
        else
          itime = (time_t) Ptime;

        logit( lot," 1st header time-stamp: UTC %s", asctime(gmtime(&itime)) );
      }

      if (InterMessageDelayMillisecs > 0) 
      {
            sleep_ew( (unsigned)( InterMessageDelayMillisecs ) );
      }
      else
      {
        wait = Ptime + offsetTime - CurrentTime;
        if ( wait > 0 )
        {
          if( wait > 120)
           logit("e", "WARNING:  waiting %d seconds for packet: <%s.%s.%s> %15.2lf\n",
                 (int)wait, wfhead->sta, wfhead->chan, wfhead->net, Ptime );
          while(wait > 1.0)
          {
            wait -= 1;
            sleep_ew( (unsigned)( 1 * 1000.0 ) );
            if( time(&timeNow)-timeLastBeat >= HeartBeatInt )
            {
              timeLastBeat = timeNow;
              tankplayer_status( TypeHeartBeat, 0, "" );
            }
          }
        }
      }

      if ( AdjTime ) set_time( msg, PlayType, Ptime + offsetTime - LateTime );

      /* Write waveform message to transport region
      ********************************************/
      if( ScreenMsg &&  (CurrentTime - lastdot) > 1.0 )
      {
	lastdot = CurrentTime;
	fprintf(stdout,".");
	fflush( stdout );
      }
      /* fprintf(stdout,
	 "tankplayer: current time-stamp:%15.2lf  dt:%5.0lf ms  dtsys:%5.0lf ms\r",
	 tcurr, dt, dtsys );*/ /*DEBUG*/
   if(bIsCorrectVersion || bBeSuperLenient)
   {
     if ( tport_putmsg( &Region, &logo, (long)size, msg ) != PUT_OK )
       logit("e", "tankplayer: tport_putmsg error.\n" );
     if (Debug > 0 )
       logit("t", "packet: <%s.%s.%s> %15.2lf\n", wfhead->sta, wfhead->chan,
	      wfhead->net, Ptime );
   }

    } /*end-while over one file*/

    /* Clean up; get ready for next file
    ***********************************/
    if(ScreenMsg) fprintf(stdout,"\n");

    result = 0; /* success up to this point */
    if ( AdjTime )
      itime = (time_t) (Ptime + offsetTime - LateTime);
    else
      itime = (time_t) Ptime;
    logit( lot, "last header time-stamp: UTC %s", asctime(gmtime(&itime)) );
    if( feof(fp) )
      logit(lo, "tankplayer:  Reached end of <%s>\n", current_file);
    else if( ferror(fp) ) {
      logit(lo, "tankplayer:  Error reading from <%s>\n", current_file );
      result= -1;
    } else
      logit(lo, "tankplayer:  Closing <%s>\n", current_file );
    fclose( fp );

    /* Pause between playing files
    *****************************/
    for( i=0; i<Pause; i++ )
    {
      sleep_ew( 1000 );
      if( time(&timeNow)-timeLastBeat >= HeartBeatInt )
      {
	timeLastBeat = timeNow;
	tankplayer_status( TypeHeartBeat, 0, "" );
      }
    }
    if (iw != -1) continue;	/* jump to next file in list */

ProcessedFile:
    /* now handle the GetFromDir case, and clean up the tank file lying around as directed */
        if( result >= 0 ) {
        /* Keep file around */
           if( SaveDataFiles ) {
              sprintf(fnew,"%s/%s",SaveSubdir,fname );
              if( rename_ew( fname, fnew ) != 0 ) {
                 logit( "e", "error moving file to ./%s\n; exiting!",
                        fnew );
                 break;
              } else {
                 logit("e","moved to ./%s\n", SaveSubdir );
              }
           }
        /* Delete the file */
           else {
              if( remove( fname ) != 0 ) {
                 logit("e","error deleting file; exiting!\n");
                 break;
              } else  {
                 logit("e","deleted file.\n");
              }
           }
        } else {
           /* ...or there was trouble (result<0)! */
           logit("e","\n");
           sprintf(fnew,"%s/%s",TroubleSubdir,fname );
           if( rename_ew( fname, fnew ) != 0 ) {
             logit( "e", " error moving file to ./%s ; exiting!\n",
                     fnew );
             break;
           } else {
             logit( "e", " moved to ./%s\n", fnew );
           }
        }


  } /*end-while */

  tport_detach( &Region );
  logit( lot, "tankplayer: No more wavefiles to play; exiting!\n" );
  fflush ( stdout );
  return( 0 );
}

/***********************************************************************
 * tankplayer_config()  processes command file using kom.c functions   *
 *                      exits if any errors are encountered            *
 ***********************************************************************/
void tankplayer_config(char *configfile)
{
  int      ncommand;   /* # of required commands you expect to process   */
  char     init[10];   /* init flags, one byte for each required command */
  int      nmiss;      /* number of required commands that were missed   */
  char    *comm;
  char    *str;
  int      nfiles;
  int      success;
  int      i;

  /* Set to zero one init flag for each required command
  *****************************************************/
  ncommand = 8;
  for( i=0; i<ncommand; i++ )  init[i] = 0;

  /* Open the main configuration file
  **********************************/
  nfiles = k_open( configfile );
  if ( nfiles == 0 ) {
    logit( "e",
	     "tankplayer: Error opening command file <%s>; exiting!\n",
	     configfile );
    exit( -1 );
  }

  /* Process all command files
  ***************************/
  while(nfiles > 0)   /* While there are command files open */
  {
    while(k_rd())        /* Read next line from active file  */
    {
      comm = k_str();         /* Get the first token from line */

      /* Ignore blank lines & comments
      *******************************/
      if( !comm )           continue;
      if( comm[0] == '#' )  continue;

      /* Open a nested configuration file
      **********************************/
      if( comm[0] == '@' ) {
	success = nfiles+1;
	nfiles  = k_open(&comm[1]);
	if ( nfiles != success ) {
	  logit( "e",
		   "tankplayer: Error opening command file <%s>; exiting!\n",
		   &comm[1] );
	  exit( -1 );
	}
	continue;
      }

      /* Process anything else as a command
      ************************************/
      /* Read the transport ring name
      ******************************/
      /*0*/    if( k_its( "RingName" ) )
      {
	if ( (str=k_str()) != NULL ) {
	  if ( (RingKey = GetKey(str)) == -1 ) {
	    logit( "e",
		     "tankplayer: Invalid ring name <%s>; exiting!\n",
		     str );
	    exit( -1 );
	  }
	}
	init[0] = 1;
      }

      /* Read the log file switch
      **************************/
      /*1*/    else if( k_its( "LogFile" ) )
      {
	LogSwitch = k_int();
	init[1] = 1;
      }

      /* Read tankplayer's module id
      ******************************/
      /*2*/    else if( k_its( "MyModuleId" ) )
      {
	if ( (str=k_str()) != NULL ) {
	  if ( GetModId( str, &MyModId ) != 0 ) {
	    logit( "e",
		     "tankplayer: Invalid module name <%s>; exiting!\n",
		     str );
	    exit( -1 );
	  }
	}
	init[2] = 1;
      }

      /* Read type of waveform msg to read from tankfile
      *************************************************/
      /*3*/    else if( k_its("PlayMsgType") )
      {
	if( (str=k_str()) != NULL ) {
	  if( GetType( str, &PlayType ) != 0 ) {
	    logit( "e",
		     "tankplayer: Invalid message type <%s>", str );
	    logit( "e", " in <PlayMsgType> cmd; exiting!\n" );
	    exit( -1 );
	  }
	}
	init[3] = 1;
      }

      /* Read heartbeat interval (seconds)
      ***********************************/
      /*4*/    else if( k_its("HeartBeatInt") )
      {
	HeartBeatInt = k_long();
	init[4] = 1;
      }

      /* Read a list of wave files to play
      ***********************************/
      /*5*/    else if( k_its("WaveFile") )
      {
	if( nWaveFile+1 >= MAX_WF ) {
	  logit( "e",
		   "tankplayer: Too many <WaveFile> commands in <%s>",
		   configfile );
	  logit( "e", "; max=%d; exiting!\n", MAX_WF );
	  exit( -1 );
	}
	if( ( str=k_str() ) != NULL ) {
	  if( strlen(str) > (size_t)MAX_LEN-1 ) {
	    logit( "e",
		     "tankplayer: Filename <%s> too long in <WaveFile>",
		     str );
	    logit( "e", " cmd; max=%d; exiting!\n", MAX_LEN-1 );
	    exit( -1 );
	  }
	  strcpy( WaveFile[nWaveFile], str );
	}
	nWaveFile++;
	init[5] = 1;
      } else if( k_its("GetFromDir") ) {
                str = k_str();
                if(str) strncpy( GetFromDir, str, NAM_LEN );
                init[5] = 1; 	
      } else if( k_its("CheckPeriod") ) {
                CheckPeriod = k_int();
      } else if( k_its("OpenTries") ) {
                OpenTries = k_int();
      } else if( k_its("OpenWait") ) {
                OpenWait = k_int();
      } else if( k_its("SaveDataFiles") ) {
                SaveDataFiles = k_int();
      }


      /* Read #seconds to pause between playing wave files
      ***************************************************/
      /*6*/    else if( k_its("Pause") )
      {
	Pause = k_int();
	init[6] = 1;
      }

      /* Read #seconds to wait for system to come up before playing data
      ****************************************************************/
      /*7*/    else if( k_its("StartUpDelay") )
      {
	StartUpDelay = k_int();
	init[7] = 1;
      }

      /* Flag for writing info to screen
      *********************************/
      else if( k_its("ScreenMsg") )     /*Optional command*/
      {
	ScreenMsg = k_int();
      }

      /* Optional packet time adjustment
      **********************************/
      else if ( k_its("SendLate") )
      {
        AdjTime = 1;
        LateTime = k_val();
      }
      else if ( k_its("InterMessageDelayMillisecs") )
      {
        InterMessageDelayMillisecs = (int)k_val();
      }

      /* Optional debug command */
      else if ( k_its("Debug") )
      {
	Debug = k_int();
      }
      else if ( k_its("IgnoreTBVersionNumbers") )
      {
        bBeSuperLenient = k_int();
      }
      /* Command is not recognized
      ***************************/
      else
      {
	logit( "e", "tankplayer: <%s> unknown command in <%s>.\n",
		comm, configfile );
	continue;
      }

      /* See if there were any errors processing the command
      *****************************************************/
      if( k_err() ) {
	logit( "e",
		 "tankplayer: Bad <%s> command in <%s>; exiting!\n",
		 comm, configfile );
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
    logit( "e", "tankplayer: ERROR, no " );
    if ( !init[0] )  logit( "e", "<RingName> "     );
    if ( !init[1] )  logit( "e", "<LogFile> "      );
    if ( !init[2] )  logit( "e", "<MyModuleId> "   );
    if ( !init[3] )  logit( "e", "<PlayMsgType> "  );
    if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
    if ( !init[5] )  logit( "e", "<WaveFile> "     );
    if ( !init[6] )  logit( "e", "<Pause> "        );
    if ( !init[7] )  logit( "e", "<StartUpDelay> " );
    logit( "e", "command(s) in <%s>; exiting!\n", configfile );
    exit( -1 );
  }

  return;
}

/***************************************************************************
 *  tankplayer_lookup( )   Look up important info from earthworm.h tables  *
 ***************************************************************************/
void tankplayer_lookup( void )
{
  if ( GetLocalInst( &InstId ) != 0 ) {
    fprintf( stderr,
	     "tankplayer: error getting local installation id; exiting!\n" );
    exit( -1 );
  }
  if( GetType( "TYPE_ADBUF", &TypeADBuf ) != 0 ) {
    fprintf( stderr,
	     "tankplayer: Invalid message type <TYPE_ADBUF>; exiting!\n" );
    exit( -1 );
  }
  if( GetType( "TYPE_TRACEBUF", &TypeTraceBuf ) != 0 ) {
    fprintf( stderr,
	     "tankplayer: Invalid message type <TYPE_TRACEBUF>; exiting!\n" );
    exit( -1 );
  }
  if( GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 ) != 0 ) {
    fprintf( stderr,
	     "tankplayer: Invalid message type <TYPE_TRACEBUF2>; exiting!\n" );
    exit( -1 );
  }
  if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
    fprintf( stderr,
	     "tankplayer: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
    exit( -1 );
  }
  if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
    fprintf( stderr,
	     "tankplayer: Invalid message type <TYPE_ERROR>; exiting!\n" );
    exit( -1 );
  }
  return;
}

/******************************************************************************
 * tankplayer_status() builds a heartbeat or error message & puts it into     *
 *                     shared memory.  Writes errors to log file & screen.    *
 ******************************************************************************/
void tankplayer_status( unsigned char type, short ierr, char *note )
{
  MSG_LOGO    logo;
  char        msg[256];
  long        size;
  time_t        t;

  /* Build the message
  *******************/
  logo.instid = InstId;
  logo.mod    = MyModId;
  logo.type   = type;

  time( &t );

  if( type == TypeHeartBeat )
  {
    sprintf( msg, "%ld %ld\n", (long) t, (long) MyPID );
  }
  else if( type == TypeError )
  {
    sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
    logit( "et", "tankplayer: %s\n", note );
  }

  size = (long)strlen( msg );   /* don't include the null byte in the message */

  /* Write the message to shared memory
  ************************************/
  if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
  {
    if( type == TypeHeartBeat ) {
      logit("et","tankplayer:  Error sending heartbeat.\n" );
    }
    else if( type == TypeError ) {
      logit("et","tankplayer:  Error sending error:%d.\n", ierr );
    }
  }

  return;
}


/*********************************************************************
 * get_time() reads the header of a TYPE_ADBUF or TYPE_TRACEBUF2 msg.*
 *   For TYPE_ADBUF msgs, returns the starttime as a double          *
 *   For TYPE_TRACEBUF2 msgs, returns the endtime as a double        *
 *********************************************************************/
double get_time( char *msg, unsigned char msgtype )
{
  WF_HEADER    *adhd;    /* header for TYPE_ADBUF message     */
  TRACE2_HEADER *wvhd;    /* header for TYPE_TRACEBUF2 message  */
  int32_t       tssec;
  int32_t       tsmic;
  double        hdtime;
  char          byte_order;

  if( msgtype == TypeADBuf )
  {
    adhd = (WF_HEADER *) msg;
    tssec = adhd->tssec;
    tsmic = adhd->tsmic;
#ifdef _SPARC
    SwapInt32( &tssec );  /* Note: By definition, TYPE_ADBUF msgs */
    SwapInt32( &tsmic );  /*       are always in INTEL byte order */
#endif
    return( (double)tssec + (0.000001*tsmic) );
  }

  else if( msgtype == TypeTraceBuf ||  msgtype == TypeTraceBuf2)
  {
    wvhd       = (TRACE2_HEADER *) msg;
    hdtime     = wvhd->endtime;
    byte_order = wvhd->datatype[0];
#ifdef _SPARC
    if(byte_order=='i') SwapDouble( &hdtime );
#endif
#ifdef _INTEL
    if(byte_order=='s') SwapDouble( &hdtime );
#endif
    return( hdtime );
  }

  return( 0.0 );
}

/*********************************************************************
 * set_time() reads the header of a TYPE_ADBUF or TYPE_TRACEBUF2 msg.*
 *  For TYPE_ADBUF msgs, it sets the starttime as a double           *
 *  For TYPE_TRACEBUF2 msgs, it sets the endtime and then calculates *
 *  and sets the starttime based on sample rate and packet size.     *
 *  Byte swapping is done as necessary.                              *
 *********************************************************************/
void set_time( char *msg, unsigned char msgtype, double time )
{
  WF_HEADER    *adhd;    /* header for TYPE_ADBUF message     */
  TRACE2_HEADER *wvhd;    /* header for TYPE_TRACEBUF2 message  */
  int32_t       tssec;
  int32_t       tsmic;
  double        starttime, endtime;
  char          byte_order;
  int           nsamp;
  double        samprate;

  if( msgtype == TypeADBuf )
  {
    adhd = (WF_HEADER *) msg;
    tssec = (int32_t) time;
    tsmic = (int32_t)(1000000 * ( time - (double) tssec ));
#ifdef _SPARC
    SwapInt32( &tssec );  /* Note: By definition, TYPE_ADBUF msgs */
    SwapInt32( &tsmic );  /*       are always in INTEL byte order */
#endif
    adhd->tssec = tssec;
    adhd->tsmic = tsmic;
    return;
  }

  else if( msgtype == TypeTraceBuf ||  msgtype == TypeTraceBuf2)
  {
    wvhd       = (TRACE2_HEADER *) msg;
    endtime     = time;
    byte_order = wvhd->datatype[0];
    nsamp      = wvhd->nsamp;
    samprate   = wvhd->samprate;

#ifdef _SPARC
    if(byte_order=='i' || byte_order == 'f')
    {
      SwapDouble( &endtime );
      SwapDouble( &samprate );
      SwapInt( &nsamp );
    }
#endif
#ifdef _INTEL
    if(byte_order=='s' || byte_order == 't')
    {
      SwapDouble( &endtime );
      SwapDouble( &samprate );
      SwapInt( &nsamp );
    }
#endif

    starttime = time - (nsamp - 1) / samprate;
#ifdef _SPARC
    if(byte_order=='i' || byte_order == 'f')  SwapDouble( &starttime );
#endif
#ifdef _INTEL
    if(byte_order=='s' || byte_order == 't') SwapDouble( &starttime );
#endif
    wvhd->starttime = starttime;
    wvhd->endtime   = endtime;
  }
  return;
}
