/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: globalproc.c 2710 2007-02-26 13:44:40Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.1  2006/01/18 17:19:51  friberg
 *     added in globalproc source
 *
 *     Revision 1.11  2003/08/04 22:07:56  lucky
 *     Removed inclusion of process.h - does not exist on solaris
 *
 *     Revision 1.10  2003/07/01 17:39:26  lucky
 *     Bug fixes for v1.0
 *
 *     Revision 1.9  2003/06/11 15:03:33  lucky
 *     The pick_sequence number was not being properly carried along, so that
 *     the amplitudes, when written into the LOC_GLOBAL message, would have
 *     sequence number 0.
 *     Fixed this problem by religiously copying, and copying, and copying
 *     the sequence number from one structure to another
 *
 *     Revision 1.8  2003/05/29 14:00:16  dhanych
 *     updated GLOBAL_AMPLINE_STRUCT member name to match global_loc_rw.h
 *
 *     Revision 1.1  2002/06/29 18:53:52  alex
 *     Initial revision
 *
 *
 *                       
 */


/*
** globalproc.c : Derived from Lynn Dietz' (or is it Carl's) eqproc. Alex 3/14/02
**
** Converted to read global messages, and general clean up; Dale, 7/31/02
**
** ORIGINAL CONVERSION COMMENTS

   Determine event termination and report results for Carl's 
   Global Associator, glass.

The big idea: keep glass as compatible to binder_ew as possible. The 
directive is that 'one size fits all'. That is, what works for global
processing at Golden is to work for regionals. (good luck). So make
glass as much as a drop-in replacement for binder_ew as possible. 
Minimize destabilization  caused by replacing binder_ew with glass in 
complex running regional system. Configuration is king. 

The solution is to:
	* create a new global pick message, and have glass eat those as well 
	  as the old TYPE_PICK2K. 
	* Have glass produce a modified link message, and tweak eqproc to use 
	  those. 
	* Have globalproc produce a new style of eartqhuake message.

The result (hopefully) is that (1) binder_ew can be replaced with glass in an 
existing binder_ew setup, and all will continue as it was, and (2) globalproc 
and eqproc can both eat glass output at the same time.


Link messages:  
One problem is phase naming. binder_ew produces link messages which 
carry integer codes for phases, as defined in char Phs[] in tlay.h. glass
will produce a modified link message, the difference being that it will 
append a char string at the end, containing the ascii string name of the 
phase. This means that eqproc will have to be tweaked to read both types of
link messages, ignoring the phase name string. We (globalproc) will ignore 
the integer, and use the character string. eqproc shall learn to ignore the 
string.
 
Pick messages:
There are now two kinds of picks: The traditional TYPE_PICK2K as produced by
pick_ew, and a new global type, say TYPE_AMP_NSN. For now, this is just 
the old TYPE_PICK2K without the three amplitude values at the end, and a string
stating the phase name. This phase name shall be "?" for unknown phases. It's 
expected that this format will be modified greatly.

For now, the coda-message processing stuff is left in. We'll ignore them. 
The code is left there because future pickers may produce similar messages 
(amplitude, etc), and the coda-code will make nice examples.

The WAIF stuff has been removed.

The residual computations have been removed. They were used for producing 
graphic plots, and for rejecting picks if the residual was too large. We'll 
leave that to glass.

**
** GLOBAL MESSAGE CONVERSION COMMENTS
**

Generally, it is expected that for any event, several TYPE_PICK_GLOBAL
messages will arrive first, possibly interleaved with several
TYPE_AMP_GLOBAL messages.  There is no reliable order for arriving pick
and amps for an event, picks will tend to arrive first, but transmission
conditions may cause an amp to arrive before its related pick.
Some time after the picks and amps, glass will emit a TYPE_QUAKE2K
followed by several TYPE_LINK messages.  Additional picks and amps
may arrive later, spurring glass to send an updated quake and more link
messages.

The program now has three storage arrays: Picks[], Amps[] and Quakes[].
Pick[] holds all recent pick arrivals, Amps[] holds all unassociated amps,
and Quakes[] holds the most recent quakes.  Note that the elements of the
Picks[] array are structures that can contain amps that match the pick.
And, elements of the Quakes[] array are structures that can contain
associated picks (called "phases") which can contain their matching amps.

As TYPE_PICK_GLOBAL messages arrive, they are put into the Picks[] array.
Then, Amps[] is searched for any matching amps, if a match is found the
amp data is copied into the pick structure and the amp is marked to
prevent a subsequent match [AMP BEFORE PICK].

As TYPE_AMP_GLOBAL messages arrive, first the Picks[] array is checked for
a matching pick.  If no match is found, the amp is put into Amps[]
[AMP BEFORE PICK].  If a match is found [PICK BEFORE AMP] then the
pick element is checked to see if it has already been associated with a quake.
If an pick-quake association is found [AMP AFTER QUAKE], then the amp
is put directly into the quake's phase's list of amps.  If no pick-quake
association is found the amp is put into the matching pick [AMP AFTER PICK,
BEFORE QUAKE].

As TYPE_QUAKE2K messages arrive, they are put into the Quakes[] array, and
updated on subsequent arrivals.

As TYPE_LINK messages arrive, the Quakes[] array is searched for a matching quake.
If no match is found, or if the quake is found but is dead, then no further action
is taken.  If the quake is found, then the Picks[] array is searched for the
referenced pick.  If the pick is found, any existing association is checked
(throwing an error on a different association), if already associated, the phase
name is are updated.  If not previously associated, the pick is associated
with the quake, and the pick (along with any existing matching amps) is copied
into the quake as phases.

AS A RESULT: Amplitude data is only stored in an Amps[] element until the amp
can be matched to a pick.  Thereafter amp information is stored in the Quakes[]
element (if the matching pick has been associated with a quake) or in the Picks[]
array (if the matching pick has not been associated with a quake).  Similarly,
pick information is only stored in the Picks[] array until the pick can be
associated with a quake.  Thereafter an associated pick is stored (as a Phase)
in the Quakes[] array.

Removed the coda-related code for this version.

***************************************************************
LIMITATIONS:  THERE MAY BE ONLY ONE PICK MESSAGE WITH THE SAME SEQUENCE NUMBER,
              MODULE ID, AND INSTALLATION ID.
              THERE MAY BE MORE THAN ONE AMP MESSAGE OF THE SAME [PICKER] SEQUENCE NUMBER
              MODULE ID, AND INSTALLATION ID -- DATA WILL BE UPDATED, NOT DUPLICATED.

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <chron3.h>

#ifdef __BORLANDC__
#include <platform.h>
#ifdef getpid
#undef getpid  /* prevent getpid conflict when compiling on Borland */
#endif
#ifdef thr_ret    /* prevent thr_ret conflict when using a C++ compiler */
#undef thr_ret
#define thr_ret void fun
#endif
#endif


#include <earthworm.h>
#include <kom.h>
#include <site.h>
#include <tlay.h>

#include <global_loc_rw.h>
#include <global_amp_rw.h>
#include <global_pick_rw.h>


/* these should go someplace sacred */
#define MAX_REPORT		20		/* max number of report intervals */


#define PHASE_NAME_LEN  8

/* ******************************
** Functions in this source file
*/
int   glproc_lookup( void   );                       /* lookup global/site variables */
int   glproc_config( char * );                       /* parse config file(s) */
void  glproc_quake2k( MSG_LOGO p_logo , char * );    /* handle quake2k messages */
void  glproc_link( char * );                         /* handle link messages */
void  glproc_pick( char * );                         /* handle pick messages */
void  glproc_amp( char * );                          /* handle amp messages  */
void  glproc_check( double );                        /* is it time to send reports? */
void  glproc_status( unsigned char, short, char * ); /* send status/heartbeats */

/*
** RETURNS
**    GLOBAL_MSG_SUCCESS
**    GLOBAL_MSG_NULL = location structure pointer is NULL
*/
GLOBAL_MSG_STATUS CopyGlobalAmpLine(       GLOBAL_AMPLINE_STRUCT * p_destin
                                   , const GLOBAL_AMPLINE_STRUCT * p_source
                                   );
GLOBAL_MSG_STATUS CopyGlobalAmp2AmpLine(       GLOBAL_AMPLINE_STRUCT * p_destin
                                       , const GLOBAL_AMP_STRUCT     * p_source
                                       );
GLOBAL_MSG_STATUS CopyGlobalPHS2PhaseLine(       GLOBAL_PHSLINE_STRUCT * p_destin
                                         , const GLOBAL_PHSLINE_STRUCT * p_source
                                         );
/*
** RETURNS
**    number 0 - n = index of successfully added item
**    GLOBAL_MSG_NULL = location structure pointer is NULL
**    GLOBAL_MSG_VERSINVALID = invalid message version (location)
**    GLOBAL_MSG_BADPARAM = child pointer is NULL, or version invalid
**    GLOBAL_MSG_MAXCHILDREN = too many children (of this type)
*/
int AddAmpToPhase( GLOBAL_PHSLINE_STRUCT * p_phase
                 , GLOBAL_AMP_STRUCT     * p_amp
                 );

static SHM_INFO  InRegion;		  /* shared memory region to use for input	*/
static SHM_INFO  OutRegion;		  /* shared memory region to use for output */

/* *******************************************
** array for desired logo types for processing
*/
#define   MAXLOGO	10
MSG_LOGO  GetLogo[MAXLOGO];
short	  LogoCount;

/* Logo of outgoing messages */
MSG_LOGO	OutLogo;


/* Things to read from configuration file
 ****************************************/
static char InRingName[MAX_RING_STR];  /* transport ring to get picks from	  */
static char OutRingName[MAX_RING_STR]; /* transport ring to put locations into	  */
static char MyModName[MAX_MOD_STR]; 	 /* speak as this module name/id			 */
static int	 LogSwitch;			          /* 0 if no logging should be done to disk */
static int  Debug = 0;				       /* 0 means don't, 1 means produce debug output */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long 	       InRingKey;	        /* key of ring for input      */
static long 	       OutRingKey;	     /* key of ring for output     */
static int 	       HeartBeatInterval;
static int           MinPhases = 0;
static unsigned char MyInstId;          /* local installation id      */
static unsigned char MyModId;		     /* Module Id for this program */

/* Message Types */
static unsigned char TYPE_HEARTBEAT;
static unsigned char TYPE_ERROR;
static unsigned char TYPE_QUAKE2K;  	     /* comes from the associator  */
static unsigned char TYPE_LINK;            /* comes from the associator  */
static unsigned char TYPE_PICK_GLOBAL;     /* from pick washer           */
static unsigned char TYPE_AMP_GLOBAL;      /* from picker/amper/codaer   */
static unsigned char TYPE_LOC_GLOBAL;      /* we produce these */

/* static char  EqMsg[MAX_BYTES_PER_EQ]; */ /* char string to hold event2k message */

/* *********************************
** Error messages used by globalproc
*/
#define  ERR_MISSMSG			0
#define  ERR_TOOBIG 			1
#define  ERR_NOTRACK			2
#define  ERR_PICKREAD			3
#define  ERR_AMPREAD			4
#define  ERR_QUAKEREAD			5
#define  ERR_LINKREAD			6
#define  ERR_UNKNOWNSTA 		7
#define  ERR_TIMEDECODE 		8
#define  ERR_DEADQUAKE       9  /* link message arrived for a dead quake */
#define  ERR_PHSQUAKEID     10  /* phase message quake id different from previous */
#define  ERR_MAGQUAKEID     11  /* amp unexpectedly associated with quake */
#define  ERR_PERSISTENCE    12  /* persistence file error */
#define  ERR_OVERWRITEAMP   13  /* an amp in Amps[] was overwritten before it could be associated */


#define MAXSTRLEN		      256
static char  Text[MAXSTRLEN]; /* string for log/error messages */


static const char QUAKE_DEAD     = -1; /* not a live quake */
static const char QUAKE_MODIFIED =  0; /* ready to be reported */
static const char QUAKE_REPORTED =  1; /* not changed since last report */


#define MAX_QUAKES 50
typedef struct GLPROC_QUAKE_STRUCT
{
  char               state;            /* use statics: "QUAKE_xxxx," from just above */
  double             reporting_time;   /* time for reporting */
  int                reporting_count;  /* number of times we've been reported */
  GLOBAL_LOC_STRUCT  quake;
  char origin_time[19];

} GLPROC_QUAKE;

GLPROC_QUAKE Quakes[MAX_QUAKES];       /* Master quakes array */

/*
** To prevent duplicate origin version numbers from being issued
** for the same event(s), events that were started prior to the
** start are ignored.
**
** This is accomplished by updating a file whenever a new event_id
** is received.  This is tracked in variable 'HighestEventId.'
** On restart the persistence file is read into variable 'LowestEventId'
** which is then incremented +1.  Thereafter, any event received
** which is less than LowestEventId is rejected.
*/
#define PERSISTENCE_FILE "globalproc_versions.dat" /* sacred name */
static HighestEventId = 0;
static LowestEventId = 0;

/*
** The Picks[] and Amps[] arrays should be considered as pseudo-ordered
** queue of all of the picks and amps that might be referenced at any
** particular time by either TYPE_LINK, TYPE_PICK_GLOBAL or TYPE_AMP_GLOBAL
** messages.
**
** MAX_PICKS should be set to a size that will prevent
** the oldest pick or amp from being overwritten until after the last
** expected TYPE_LINK message would be expected to reference the
** data element.
**
** It should be noted, that once a pick or amp is associated with
** a quake, the relevant information is copied into the appropriate
** array of the quake (in the Quakes[] array), thus representing
** 'conversion' to a phase or 'amp-pick'.
** Therefore aging-off of a pick or amp does not prevent correct
** reporting, if the association has already been made.
*/
#define MAX_PICKS 1000
typedef struct
{
   short              quake_index; /* Quakes[quake_index] = quake with which this pick associated,
                                   ** -1 if not associated
                                   */
   short              phase_index; /* the phase made from this pick,
                                   ** obtained by:  Quakes[quake_index].quake.phases[phase_index]
                                   */
   GLOBAL_PHSLINE_STRUCT pick;
} ASSOC_PICK_STRUCT;

static ASSOC_PICK_STRUCT Picks[MAX_PICKS];  /* "circular" list of all picks */
static int NextPickIndex = 0;                 /* index into Picks][ where next pick will go */


/*
**  The following describes a container for unassociated amps
**  generally, just want to provide a buffer in case some amps happen
**  to arrive before their related picks.
**
**  MAX_UNASSOC_AMPS should be set to a size that will allow arriving picks
**  to locate any earlier arriving (and matching) amps before the
**  unassociated amps are overwriten.
*/
#define MAX_UNASSOC_AMPS 200
typedef struct
{
   MSG_LOGO           logo;        /* logo of the picker in the PICK_GLOBAL message */
   long               pick_seq;    /* When amp arrives and a matching pick cannot be found in
                                   ** Picks[] = sequence number assigned by the picker
                                   ** If the matching pick later arrives, and the amp can
                                   ** be associated, this is set to 0 [zero]
                                   */
/*
   short              quake_index; / * Quakes[quake_index] = quake with which this amp associated,
                                   * * -1 if not associated
                                   * /
   short              amp_index;   / * the magnitude made from this amplitude,
                                   * * obtained by:  Quakes[quake_index].quake.amps[amp_index]
                                   * /
*/
   GLOBAL_AMPLINE_STRUCT  amp;
} UNASSOC_AMP_STRUCT;

static UNASSOC_AMP_STRUCT Amps[MAX_UNASSOC_AMPS];  /* "circular" list of all amps */
static int NextAmpIndex = 0;               /* index into Amps[] where next amplitude will go */



static int  Nreport = 0;
typedef struct _report                         
{
	int		interval;
	int		force;
} ReportStruct;


static ReportStruct	Report[MAX_REPORT];		/* Array of reporting intervals	*/


/* Process id used for heartbeats and shutdown checking */
pid_t  MyPid = 0;



/* **************************************************************************
**                               MAIN
*/
int main (int argc, char **argv)
{
   int        r_returncode = 0;

   char       msg[MAXSTRLEN];  /* actual retrieved message  */
   long       msgsize;	        /* size of retrieved message */
   MSG_LOGO   msglogo;         /* logo of retrieved message */
   int        loop_index;
   time_t     timeNow
      ,       timeLastBeat = 0
      ;
   double     work_time;       /* used to check heartbeat interval and message processing   was: t;  */

   int running = TRUE;  /* set to FALSE to stop main loop */

   int getting_msg   /* used to control repeat/exit of inner loop   */
     , have_message  /* inner-loop flag indicating message obtained */
     ;

   /* *****************************
   ** Check command line arguments
   */
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: globalproc <configfile>\n" );
      sleep_ew(3000);
      exit( 0 );
   }

   /* *****************************
   ** Initialization
   */

   NextPickIndex   = 0;
   NextAmpIndex   = 0;

   /*
   ** set all hypocenter id's to dead (no live ones)
   */
   for( loop_index = 0 ; loop_index < MAX_QUAKES ; loop_index++ )
   {
      Quakes[loop_index].state  = QUAKE_DEAD;
      InitGlobalLoc( &Quakes[loop_index].quake );
   }

   for ( loop_index = 0 ; loop_index < MAX_PICKS ; loop_index++ )
   {
      Picks[loop_index].quake_index = -1;  /* no quake association  */
      Picks[loop_index].phase_index = -1;  /* not made into a phase */
      InitGlobalPhaseLine( &(Picks[loop_index].pick) );
   }

   for ( loop_index = 0 ; loop_index < MAX_UNASSOC_AMPS ; loop_index++ )
   {
      Amps[loop_index].pick_seq  = -1;   /* no pick yet matched */
      Amps[loop_index].logo.type = 0;
      Amps[loop_index].logo.mod = 0;
      Amps[loop_index].logo.instid = 0;
      InitGlobalAmpLine( &(Amps[loop_index].amp) );
   }



   /* *************************************
   ** Initialize name of log-file & open it
   */
   logit_init( argv[1], (short)MyModId, 4096*2, 1 );



   /* **********************************************
   ** Look up important info from earthworm.h tables
   */
   if ( glproc_lookup() == FALSE )
   {
      logit( "e"
           , "globalproc: Error looking up values, must exit\n"
           );
      exit( -1 );
   }


   /* ******************************
   ** Read the configuration file(s)
   */
   logit( "" , "globalproc: Read command file <%s>\n", argv[1] );
   if ( glproc_config( argv[1] ) == FALSE )
   {
      logit( "e"
           , "globalproc: Error reading/parsing file <%s>, must exit\n"
           , argv[1]
           );
      running = FALSE;
   }



   /* *************************************************
   ** Reinitialize logger, based on LogSwitch parameter
   */
   logit_init( argv[1], (short)MyModId, 4096*2, LogSwitch );

   /* *************************************************
   ** Attempt to read the persistence file
   */
   if ( running == TRUE )
   {
      char lastid[20];
      FILE * persistence = fopen( PERSISTENCE_FILE, "r" );
      if ( persistence == NULL )
      {
         logit( "e"
              , "globalproc:  Failed to open persistence file (this is okay for 1st run at an installation)\n"
              );
         HighestEventId = LowestEventId = 0;
      }
      else
      {
         if ( fscanf(persistence, "%s", lastid ) != 1 )
         {
            logit( "e"
                 , "%s%s%s%s"
                 , "globalproc: ****************************************************************\n"
                 , "            *** WARNING: Failed to read persistence file line            ***\n"
                 , "            ***          This could result in duplicate origin versions. ***\n"
                 , "            ****************************************************************\n"
                 );
            HighestEventId = LowestEventId = 0;
            glproc_status( TYPE_ERROR, ERR_PERSISTENCE, "Failed to read persistence file" );
         }
         else
         {
            /* null-terminate the string */
            lastid[strlen(lastid)-1] = '\0';
            HighestEventId = LowestEventId = atoi( lastid );
            /* must reject all events, including the last */
            LowestEventId++;
         }
         fclose( persistence );
      }
   }



   if ( running == TRUE )
   {
      /* *****************************
      ** Look up keys to shared memory regions
      */
      if( ( InRingKey = GetKey(InRingName) ) == -1 )
      {
         logit( "e"
              , "globalproc:  Invalid InRingKey ring name <%s>; exiting!\n"
              , InRingName
              );
         running = FALSE;
      }
      if( ( OutRingKey = GetKey(OutRingName) ) == -1 )
      {
         logit( "e"
              , "globalproc:  Invalid OutRingKey ring name <%s>; exiting!\n"
              , OutRingName
              );
         running = FALSE;
      }

      /* *****************************
      ** Look up installations of interest
      */
      if ( GetLocalInst( &MyInstId ) != 0 )
      {
         logit( "e"
              , "globalproc: error getting local installation id; exiting!\n"
              );
         running = FALSE;
      }

      /* *****************************
      ** Look up modules of interest
      */
      if ( GetModId( MyModName, &MyModId ) != 0 )
      {
         logit( "e"
              , "globalproc: Invalid module name <%s>; exiting!\n"
              , MyModName
              );
         running = FALSE;
      }
   }

   if ( running == FALSE )
   {
      exit( -1 );
   }

   /* *****************************
   ** Set outgoing logo
   */
   OutLogo.instid = MyInstId;
   OutLogo.mod = MyModId;
   OutLogo.type = TYPE_LOC_GLOBAL;


   /* *****************************
   ** Store my own processid
   */
   MyPid = getpid();

   /* Output for Debug */
   if( 2 <= Debug )
   {
      /* Dump the report intervals */
      for( loop_index = 0 ; loop_index < Nreport ; loop_index++ )
      {
         logit( ""
              , "Report[%d]=%d (force=%d)\n"
              , loop_index
              , Report[loop_index].interval
              , Report[loop_index].force
              );
      }

      /* Dump the logos */
      for( loop_index = 0 ; loop_index < LogoCount ; loop_index++ )
      {
         logit( ""
              , "GetLogo[%d] inst:%d module:%d type:%d\n"
              , loop_index
              , (int)GetLogo[loop_index].instid
              , (int)GetLogo[loop_index].mod
			      , (int)GetLogo[loop_index].type
              );
      }
	}

  
   /* *****************************
   ** Attach to shared memory rings
   */
   tport_attach( &OutRegion, OutRingKey );
   logit( "", "globalproc: Attached to public memory region %s: %d\n", OutRingName, OutRingKey );

   tport_attach( &InRegion, InRingKey );
   logit( "", "globalproc: Attached to public memory region %s: %d\n", InRingName, InRingKey );


   /* *****************************
   ** Flush out all old messages
   */
   while( tport_getmsg( &InRegion, GetLogo, LogoCount, &msglogo, &msgsize, msg, MAXSTRLEN ) != GET_NONE );


   /* **********************************************************
   **                       MAIN LOOP
   **
   **  This loop has three general activities:
   **  1. Call the publishing function
   **  2. Send heartbeat message if time
   **  3. Enter a loop to retrieve all [currently] pending messages
   */

   while ( running == TRUE )
   {


      /* ****************
      ** Get current time
      */
      work_time = tnow();

      /* ***********************************************************
      ** See if it's time to check all the hypocenters for reporting
      */
      glproc_check( work_time );



      /* **************
      ** Send heartbeat
      */
      if ( time(&timeNow) - timeLastBeat >= HeartBeatInterval )
      {
         timeLastBeat = timeNow;
         glproc_status( TYPE_HEARTBEAT, 0, "" );
      }

      /* ***************************************
      ** Inner-loop to retrieve all available messages
      */
      getting_msg = TRUE;



      do
      {
         /* *****************************
         ** see if a termination has been requested
         */
         if (   tport_getflag( &InRegion ) == TERMINATE
             || tport_getflag( &InRegion ) == MyPid
            )
         {
            /* write a few more things to log file and close it */
            logit( "t", "globalproc: Termination requested; exiting!\n" );
            fflush( stdout );
            getting_msg = FALSE; /* exit inner loop */
            running = FALSE;     /* exit main loop */
            continue;            /* go to end of inner loop */
         }


         /*  *****************************
         **  Get & process the next message from shared memory
         */

         have_message = FALSE;  /* will be set to 1 if message obtained */

         switch( tport_getmsg( &InRegion
                             , GetLogo
                             , LogoCount
                             , &msglogo
                             , &msgsize
                             , msg
                             , MAXSTRLEN-1
               )             )
         {
           case GET_MISS:     /* good message, but missed expected sequence number */
                sprintf( Text
                       , "Missed msg(s)  i%u m%u t%u  region:%ld."
                       , msglogo.instid
                       , msglogo.mod
                       , msglogo.type
                       , InRegion.key
                       );
                glproc_status( TYPE_ERROR, ERR_MISSMSG, Text );
                have_message = TRUE;
                break;

           case GET_NOTRACK:  /* good message, but can't track this logo */
                sprintf( Text
                       , "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded"
                       , msglogo.instid
                       , msglogo.mod
                       , msglogo.type
                       );
                glproc_status( TYPE_ERROR, ERR_NOTRACK, Text );
                have_message = TRUE;
                break;

           case GET_OK:      /* good message */
                have_message = TRUE;
                break;

           case GET_TOOBIG:   /* message too big to for arrival buffer */
                sprintf( Text
                       , "Retrieved msg[%ld] (i%u m%u t%u) too big for arriving message buffer[%d]"
                       , msgsize
                       , msglogo.instid
                       , msglogo.mod
                       , msglogo.type
                       , MAXSTRLEN-1
                       );
                glproc_status( TYPE_ERROR, ERR_TOOBIG, Text );
                break;

           case GET_NONE:
                getting_msg = 0;
                break;
         }


         if ( have_message == TRUE )
         {
            /* ***************************
            ** have a good message, handle by type
            */
            msg[msgsize] = '\0';  /* null terminate the message*/

            if( msglogo.type == TYPE_PICK_GLOBAL )
            {
               logit ("", "Parsing pick msg <%s>\n", msg);
               glproc_pick( msg );
            }
            else if( msglogo.type == TYPE_AMP_GLOBAL ) /* Post-P amplitude from nsn */
            {
               logit ("", "Parsing amp msg <%s>\n", msg);
               glproc_amp ( msg );
            }
            else if( msglogo.type == TYPE_LINK )
            {
               logit ("", "Parsing link msg <%s>\n", msg);
               glproc_link( msg );
            }
            else if( msglogo.type == TYPE_QUAKE2K )
            {
               logit ("", "Parsing quake msg <%s>\n", msg);
               glproc_quake2k( msglogo , msg );
            }
         }

         fflush( stdout );

      } while( getting_msg == TRUE );  /* res != GET_NONE );  / * end of message-processing-loop */

      /* ************************************************
      ** if still running,
      ** avoid cpu hogging by sleeping for a second
      ** after all available messages have been processed
      */
      if ( running == TRUE )
      {
         sleep_ew(1000);
      }

   } /* main loop: check publishing */


   /* detach from shared memory */
   tport_detach( &InRegion );
   tport_detach( &OutRegion );

   return r_returncode;


}  /*-----------------------------end of main loop-------------------------------*/

/****************************************************************************
**	glproc_quake2k() processes a TYPE_QUAKE2K message from glass
****************************************************************************/
void glproc_quake2k( MSG_LOGO p_logo , char *msg )
{

	 double	 time_now;
	    int 	 quake_index;  /* index into Quakes[] */

   /* variables obtained from message */
	   long	 quake_sequence;
     char   origin_time[40];
   double   lat
        ,   lon
        ;
	  float	 depth
        ,   rms
        ,   dmin
        ,   ravg
        ,   gap
        ;
      int   pick_count;

   /*
   ** Read info from ascii message
   */
	 if ( sscanf( msg
              , "%*d %*d %ld %s %lf %lf %f %f %f %f %f %d"
              , &quake_sequence
              ,  origin_time
              , &lat
              , &lon
              , &depth
              , &rms
              , &dmin
              , &ravg
              , &gap
              , &pick_count
              ) != 10 )
   {
      sprintf( Text, "glproc_quake2k: Error reading ascii quake msg: %s", msg );
      glproc_status( TYPE_ERROR, ERR_QUAKEREAD, Text );
      return;
   }
   if ( quake_sequence < LowestEventId )
   {
      if( 2 <= Debug )
      {
         logit( "e"
              , "globalproc:  rejecting event %d: existed before program start (lowest = %d)\n"
              , quake_sequence, LowestEventId
              );
      }
      return;
   }

   if ( HighestEventId < quake_sequence )
   {
      /*
      ** This is a never-before seen event id,
      ** must write it to the persistence file.
      */
      FILE * persistence;

      HighestEventId = quake_sequence;


      if ( (persistence = fopen( PERSISTENCE_FILE, "w" )) == NULL )
      {
         logit( "e"
              , "%s%s%s%s"
              , "globalproc: ****************************************************************\n"
              , "            *** WARNING: Failed to open persistence file for writing     ***\n"
              , "            ***          This could result in duplicate origin versions. ***\n"
              , "            ****************************************************************\n"
              );
         glproc_status( TYPE_ERROR, ERR_PERSISTENCE, "Failed to open persistence file" );
      }
      else
      {
         if ( fprintf(persistence, "%d" , HighestEventId ) == EOF )
         {
            logit( "e"
                 , "%s%s%s%s"
                 , "globalproc: ****************************************************************\n"
                 , "            *** WARNING: Failed to write persistence file line           ***\n"
                 , "            ***          This could result in duplicate origin versions. ***\n"
                 , "            ****************************************************************\n"
                 );
            glproc_status( TYPE_ERROR, ERR_PERSISTENCE, "Failed to write persistence file" );
         }
         fclose( persistence );
      }
   }

	 if( 2 <= Debug )
   {
      logit( "e"
           , "Parsed quake message: seq:%ld %s lat:%lf lon:%lf %lf %f %f %f %f nph:%d \n"
           , quake_sequence
           , origin_time
           , lat
           , lon
           , depth
           , rms
           , dmin
           , ravg
           , gap
           , pick_count
           );
   }


   /*
   **  Normalize the quake sequence number assigned by the associator,
   **  (and arriving in the message) into the domain of an index into
   **  the local quake storage array.  That is, this modulus will convert
   **  quake_sequence into the range 0....(MAX_QUAKES - 1), wrapping forever.
   */
   quake_index = quake_sequence % MAX_QUAKES;



   if(   quake_sequence == Quakes[quake_index].quake.event_id
      && Quakes[quake_index].state == QUAKE_DEAD
     )
   {
      /* Quake existed previously, but associator reported it dead.
      ** Dead quakes can not be revived.
      ** Thus, arriving data is useless, just return.
      */
      return;
   }


   /* ****************************
   ** Set quake's state
   */
   if ( pick_count == 0 )
   {
      /* ******************************************************
      ** If number of phases on arrival is zero, the associator
      ** is saying to kill the quake
      */
      Quakes[quake_index].state = QUAKE_DEAD;
   }
   else
   {
      /* current time used to initialize reporting interval */
      time_now = tnow();

      if( quake_sequence != Quakes[quake_index].quake.event_id )
      {
         /*
         ** The arriving quake id [qkseq] is not the same
         ** as that stored at the appropriate location in the array,
         ** thus this is the first time seen.
         */
         Quakes[quake_index].reporting_time = time_now + Report[0].interval;
         Quakes[quake_index].reporting_count = 0;		/* we've never been reported */

         InitGlobalLoc( &Quakes[quake_index].quake );

         Quakes[quake_index].quake.event_id    = quake_sequence;

         Quakes[quake_index].quake.origin_id   = 1;

         /*
         ** Setting logo values here (rather than just after
         ** this block) saves a little time on subsequent
         ** QUAKE2K message arrivals, but is based on the
         ** premise that globalproc will only listen to a
         ** single glass, that as a result, there is only a
         ** single sequence stream for quake/event ids.
         */
         Quakes[quake_index].quake.logo.type   = p_logo.type;
         Quakes[quake_index].quake.logo.mod    = p_logo.mod;
         Quakes[quake_index].quake.logo.instid = p_logo.instid;
      }

      /* QUAKE_MODIFIED means "ready to be reported" */
      Quakes[quake_index].state =	QUAKE_MODIFIED;

      strcpy( Quakes[quake_index].origin_time , origin_time );
      if((epochsec17(&Quakes[quake_index].quake.tOrigin,origin_time))!=0){
        logit("pt", ": Failed to convert origin_time epoch\n");
      }


      Quakes[quake_index].quake.lat                 = lat;
      Quakes[quake_index].quake.lon                 = lon;
      Quakes[quake_index].quake.depth               = depth;
      Quakes[quake_index].quake.rms                 = rms;
      Quakes[quake_index].quake.dmin                = dmin;
      Quakes[quake_index].quake.gap                 = gap;
      Quakes[quake_index].quake.pick_count          = pick_count;

      if( 1 <= Debug )
      {
         /* ******************************************************
         **  Write out the time-stamped hypocenter to the log file
         */
         char	   cdate[25];
         char	   corg[25];
         date20( time_now, cdate );
         date20( DTStringToTime( Quakes[quake_index].origin_time )
               , corg
               );

         logit( ""
              , "Quake: %s seq:%8ld t received:%s lat:%9.4f lon:%10.4f z:%6.2f rms:%5.2f dmin:%5.1f gap:%4.0f nph:%3d\n"
              , cdate + 10
              , Quakes[quake_index].quake.event_id
              , corg + 10
              , Quakes[quake_index].quake.lat
              , Quakes[quake_index].quake.lon
              , Quakes[quake_index].quake.depth
              , Quakes[quake_index].quake.rms
              , Quakes[quake_index].quake.dmin
              , Quakes[quake_index].quake.gap
              , Quakes[quake_index].quake.pick_count
              );
      }

   } /* processing, not deleting, quake */
}


/***************************************************************************
**	glproc_link() processes a processes a TYPE_LINK message from glass
****************************************************************************/
void glproc_link( char *msg )
{
   long          quake_sequence; /* quake id assigned by associator */
   int 		      iinstid         /* installation id of incoming message as int, temporary */
     ,           isrc            /* module id of incoming message as int, temporary */
     ;
   long          pick_sequence;  /* sequence number from picker */
   int			   iphs = 0;       /* Integer count of phases, as reported by the associator
                                 ** Not used herein; retained for message compatiblity with eqproc
                                 */
   char		      szPhase[PHASE_NAME_LEN+1]
      ,          lat[64]         /* not used herein. For compatiblity with TYPE_LINK */
      ,          lon[64]         /* not used herein. For compatiblity with TYPE_LINK */
      ,          elev[64]        /* not used herein. For compatiblity with TYPE_LINK */
      ;

   unsigned char pick_source;    /* picker module id; from iinstid */
   unsigned char pick_instid;    /* picker installation id; from isrc */

   int           quake_index; /* index into Quakes[] */
   int           phase_index; /* index into a Quake[n].phases[] */

   /* indices used to loop through phases and amps */
   int           start_index
     ,           check_index
     ;



   /*
   **  Parse the TYPE_LINK message:

                   2804 13 74 123153 0 Pup 44.1204 -104.0361 2060.00
                      |  |  |    |   | |    |         |         |
         quake_sequence  |  |    |   | |    |         |         |
                   instid?  |    |   | |    |         |         |
                        isrc?    |   | |    |         |         |
                    pick_sequence?   | |    |         |         |
                                  iphs |    |         |         |
                                 szPhase    |         |         |
                                     Latitude         |         |
                                              Longitude         |
                                                        Elevation?

   */
   if ( sscanf( msg
              , "%ld %d %d %ld %d %s %s %s %s"
              , &quake_sequence
              , &iinstid
              , &isrc
              , &pick_sequence
              , &iphs
              ,  szPhase
              ,  lat
              ,  lon
              ,  elev
              ) < 6 )
   {
      sprintf( Text, "glproc_link: Error reading ascii link msg: %s", msg );
      glproc_status( TYPE_ERROR, ERR_LINKREAD, Text );
      return;
   }



   if ( 2 <= Debug )
   {
      logit( "e"
           , "Parsed a link message: %ld %d %d %d %d %s (%s,%s,%s)\n"
           , quake_sequence
           , iinstid
           , isrc
           , pick_sequence
           , iphs
           , szPhase
           , lat
           , lon
           , elev
           );
   }

   if ( PHASE_NAME_LEN < strlen(szPhase) )
   {
      sprintf( Text, "glproc_link: Phase name too long in link msg: %s", msg );
      glproc_status( TYPE_ERROR, ERR_LINKREAD, Text );
      return;
   }


   pick_source = (unsigned char) isrc;
   pick_instid = (unsigned char) iinstid;

   /*
   ** Check if the quake referenced by this TYPE_LINK message
   ** is currently alive.
   **
   ** --------------------------------------------------------
   ** NOTE: This code expects that the earthworm structure
   **       into which this module is being placed will cause
   **       all relevant picks to arrive at this module prior
   **       to quake and link messages (from glass).
   ** --------------------------------------------------------
   */

   /*
   ** Normalize the quake sequence number assigned by the associator,
   ** into the domain of an index into the Quakes array.
   ** yields:  0 <= quake_index < MAX_QUAKES
   */
   quake_index = quake_sequence % MAX_QUAKES;


   /*
   ** Note that the quake is expected to already be in the Quakes
   ** array (due to an earlier TYPE_QUAKE2K arrival).  If it is not
   ** [id mismatch] or if it has already been set dead [unlikely]
   ** then the link is irrelevant.  Just return.
   */
   if (   Quakes[quake_index].state == QUAKE_DEAD
       || Quakes[quake_index].quake.event_id != quake_sequence
      )
   {
      sprintf( Text
             , "glproc_link: link message arrived for a dead quake (%d)\n"
             , quake_sequence
             );
      logit( "e", Text );
      glproc_status( TYPE_ERROR, ERR_DEADQUAKE, Text );
      return;
   }

   /*
   ** Look through the list of Picks to attempt to find the pick
   ** referenced by this TYPE_LINK message.
   **
   ** Since arriving picks are inserted into Picks at an incrementing
   ** index, tracked with the variable NextPickIndex, this check should
   ** start at NextPickIndex - 1 and work backwards.
   ** (Based on the premise that links will more likely apply to picks that have
   ** recently arrived, rather than older ones.)
   */
   if ( (start_index = (NextPickIndex - 1)) < 0 )
   {
      /* insert location at lowest index start at highest */
      start_index = MAX_PICKS - 1;
   }

   /*
   **  start_index just holds the starting location,
   **  check_index is the working location.
   */
   check_index = start_index;


   do
   {
      /*
      ** A match is found when the pick sequence number,
      ** module id and institution id of the pick in the arriving
      ** link message are the same as those for an item in the
      ** unassociated picks array.
      */
      if (   pick_sequence == Picks[check_index].pick.sequence
          && pick_source   == Picks[check_index].pick.logo.mod
          && pick_instid   == Picks[check_index].pick.logo.instid
         )
      {
         /*
         ** The pick described in the arriving link message
         ** matches the pick at check_index.
         ** (There can be only one match in the array.)
         */

         if ( Picks[check_index].quake_index == -1 )
         {
            /*
            ** This matched pick has not previously been associated with a quake
            */

            /*
            ** Assign phase name to pick
            */
            strcpy( Picks[check_index].pick.phase_name , szPhase );

            /*
            ** Tell the quake about the association
            */
            switch ( (phase_index = AddPhaseLineToLoc( &Quakes[quake_index].quake
                                                     , &Picks[check_index].pick
                   ) )                               )
            {
              case GLOBAL_MSG_MAXCHILDREN:
                   logit( "e"
                        , "glproc_link: Can't add pick to quake: too many children\n"
                        );
                   glproc_status( TYPE_ERROR, ERR_LINKREAD, "glproc_link: Too many picks to add to quake" );
                   return;

              case GLOBAL_MSG_VERSINVALID:
                   logit( "e"
                        , "glproc_link: invalid phase version\n"
                        );
                   glproc_status( TYPE_ERROR, ERR_LINKREAD, "glproc_link: invalid phase version" );
                   return;

              case GLOBAL_MSG_NULL:
                   logit( "e"
                        , "Error adding pick to quake\n"
                        );
                   glproc_status( TYPE_ERROR, ERR_LINKREAD, "glproc_link: Error adding pick to quake" );
                   return;

              case GLOBAL_MSG_BADPARAM:
                   logit( "e"
                        , "Error adding pick to quake: bad phase version\n"
                        );
                   glproc_status( TYPE_ERROR, ERR_LINKREAD, "glproc_link: Error adding pick to quake: bad phase version" );
                   return;

              default:
                   /*
                   ** Set the pick's quake id to associate it with the quake.
                   ** If a TYPE_PICK_GLOBAL message arrives, this will be helpful
                   ** to quickly locate the quake and phase structures.
                   */
                   Picks[check_index].quake_index = quake_index;
                   Picks[check_index].phase_index = phase_index;

                   /*
                   ** set the phase name
                   */
                   strcpy( Quakes[quake_index].quake.phases[phase_index].phase_name , szPhase );


                   /* Mark the quake as modified so it can be reported */
                   Quakes[quake_index].state = QUAKE_MODIFIED;

                   if( 1 <= Debug )
                   {
                      logit( ""
                           , "Linking .%s.%s.%s. with event %d as .%s.\ntaging event %d as modified\n"
                           , Picks[check_index].pick.station
                           , Picks[check_index].pick.channel
                           , Picks[check_index].pick.network
                           , quake_sequence
                           , szPhase
                           , quake_index
                           );
                   }
                   break;
            }
         } /* pick not previously associated with quake */
         else
         {
            /*
            ** Matched pick has been previously associated with a quake
            */

            /*
            ** At the time of writing, it is categorically stated
            ** that a link message will never assign a
            ** different quake id to the same pick on subsequent
            ** issuances.
            ** However, it is acknowledged as conceivable that such
            ** might occur in the future.
            **
            ** Therefore, send a panic message if encountered.
            */
            if ( Quakes[Picks[check_index].quake_index].quake.event_id == quake_sequence )
            {
               /* update the phase name in case it changed */
               phase_index = Picks[check_index].phase_index;
               strcpy( Quakes[quake_index].quake.phases[phase_index].phase_name , szPhase );
            }
            else
            {
               sprintf( Text
                      , "glproc_link: phase (%d %d %d) arrived with quake id %d, was %d\n"
                      , pick_instid
                      , pick_source
                      , pick_sequence
                      , quake_sequence
                      , Quakes[Picks[check_index].quake_index].quake.event_id
                      );
               logit( "e", Text );
               glproc_status( TYPE_ERROR, ERR_PHSQUAKEID, Text );
            }
         } /* pick was previously associated with quake */

         /* only one matching pick is possible, ready to return */
         
         return;

      } /* found pick referenced in link message */

      /* decriment the check index
      ** in preparation to check next pick
      */
      if ( --check_index < 0 )
      {
         check_index = MAX_PICKS - 1;
      }

   } while ( check_index != start_index );
}


/****************************************************************************
**	  glproc_pick() parses a TYPE_PICK_GLOBAL message and puts the values
**                   into the appropriate data structure.
****************************************************************************/
void glproc_pick( char *msg )
{
   GLOBAL_PICK_STRUCT _pick_data;

   int           _ampindex; /* used to search for matching amps */

   InitGlobalPick(&_pick_data);

   switch ( StringToPick( &_pick_data, msg ) )
   {
     case GLOBAL_MSG_FORMATERROR:
		  sprintf( Text, "glproc_pick: pick message format invalid:\n%s\n", msg );
		  glproc_status( TYPE_ERROR, ERR_PICKREAD, Text );
          return;

     case GLOBAL_MSG_VERSINVALID:
		  sprintf( Text, "glproc_pick: pick message: version invalid <%d>\n%s\n"
			     , _pick_data.version
				 , msg
				 );
		  glproc_status( TYPE_ERROR, ERR_PICKREAD, Text );
          return;

     case GLOBAL_MSG_BADPARAM:
     case GLOBAL_MSG_NULL:
		  sprintf( Text, "glproc_pick: Error decoding pick message:\n%s\n", msg );
		  glproc_status( TYPE_ERROR, ERR_PICKREAD, Text );
          return;
   }


   if( 2 <= Debug )
   {
      logit( "e"
           , "Parsed pick msg: %d %d %d %d .%s.%s.%s.%s. %c %f %s %s\n"
           , _pick_data.logo.type
           , _pick_data.logo.mod
           , _pick_data.logo.instid
           , _pick_data.sequence
           , _pick_data.station
           , _pick_data.channel
           , _pick_data.network
           , _pick_data.location
           , _pick_data.polarity
           , _pick_data.quality
           , _pick_data.pick_time
           , _pick_data.phase_name
           );
   }

   /*
   ** It is currently expected that only a single pick message for a
   ** given sequence number will ever be sent (no updates).
   ** Therefore, a simple incrementing variable (NextPickIndex) is
   ** used for the write location.  This is slighly more efficient,
   ** a little clearer, and it enables requisite searchs in glproc_link()
   ** and glproc_amp() to be as short as possible.
   */

   /*
   **
   **
   ** Because TYPE_PICK_GLOBAL messages will always arrive before
   ** TYPE_LINK, there is no way to make an association at present.
   ** Therefore set the indices to -1.
   */
   Picks[NextPickIndex].quake_index = -1;
   Picks[NextPickIndex].phase_index = -1;

   InitGlobalPhaseLine( &(Picks[NextPickIndex].pick) );

           Picks[NextPickIndex].pick.logo.type   = _pick_data.logo.type;
           Picks[NextPickIndex].pick.logo.mod    = _pick_data.logo.mod;
           Picks[NextPickIndex].pick.logo.instid = _pick_data.logo.instid;

           Picks[NextPickIndex].pick.sequence    = _pick_data.sequence;

   strcpy( Picks[NextPickIndex].pick.station     , _pick_data.station );
   strcpy( Picks[NextPickIndex].pick.channel     , _pick_data.channel );
   strcpy( Picks[NextPickIndex].pick.network     , _pick_data.network );
   strcpy( Picks[NextPickIndex].pick.location    , _pick_data.location );

   /* pick_time is now epoch
   strcpy( Picks[NextPickIndex].pick.pick_time   , _pick_data.pick_time );
   */
   if((epochsec17(&Picks[NextPickIndex].pick.tPhase, _pick_data.pick_time))!=0){
      logit("pt", ": Failed to convert origin_time epoch\n");
   }

   strcpy( Picks[NextPickIndex].pick.phase_name  , "?" );

           Picks[NextPickIndex].pick.polarity    = _pick_data.polarity;
           Picks[NextPickIndex].pick.quality     = _pick_data.quality;

   /*
   ** There may have been amps that arrived before this pick message.
   ** Look through the Amps[] array, searching for matches, if any are
   ** found, copy the amp data
   **
   ** Because there may be more than one match, search the entire array
   */
   for ( _ampindex = 0 ; _ampindex < MAX_UNASSOC_AMPS ; _ampindex++ )
   {
      if (           Amps[_ampindex].pick_seq    == _pick_data.sequence
          &&         Amps[_ampindex].logo.mod    == _pick_data.logo.mod
          &&         Amps[_ampindex].logo.instid == _pick_data.logo.instid
/*
          && strcmp( Amps[_ampindex].amp.station ,  _pick_data.station ) == 0
*/
         )
      {
         /* matching amp located */


         /*
         ** Prevent a future match with the unassociated amp.
         ** If an amp is updated later (allowed for some pickers),
         ** then must look in the quakes array.
         */
          Amps[_ampindex].pick_seq = -1;

         /* copy the data into the new pick element */
         switch( AddAmpLineToPhase( &Picks[NextPickIndex].pick, &Amps[_ampindex].amp ) )
         {
           case GLOBAL_MSG_NULL:
           case GLOBAL_MSG_BADPARAM:
                logit( "e"
                     , "error adding amp to new pick\n"
                     );
                glproc_status( TYPE_ERROR, ERR_PICKREAD, "AddAmpLineToPhase() returned error" );
                break;

           case GLOBAL_MSG_VERSINVALID:
                logit( "e"
                     , "invalid version when adding amp from <%d %d %d> to phase\n"
                     , _pick_data.logo.type
                     , _pick_data.logo.mod
                     , _pick_data.logo.instid
                     );
                glproc_status( TYPE_ERROR, ERR_PICKREAD, "invalid version from AddAmpLineToPhase()" );
                break;

           case GLOBAL_MSG_MAXCHILDREN:
                logit( "e"
                     , "Can't add amp to phase: too many children\n"
                     );
                glproc_status( TYPE_ERROR, ERR_PICKREAD, "Too many amps to add to phase" );
                _ampindex = MAX_UNASSOC_AMPS; /* leave the for() loop */
                break;
         }
      } /* amp matched */
   }
   

   if ( ++NextPickIndex == MAX_PICKS )
   {
      NextPickIndex = 0;
   }

   if( 1 <= Debug )
   {
      logit ("", "Pick seq %d inserted at new index %d\n", _pick_data.sequence, NextPickIndex);
   }
}


/****************************************************************************
**	  glproc_amp() decodes a TYPE_AMP_GLOBAL message and puts the values
**                   into the appropriate data structure.
/****************************************************************************/
void glproc_amp( char * msg )
{
   GLOBAL_AMP_STRUCT _amp_data;

   int           start_index
     ,           check_index
     ;

   GLOBAL_PHSLINE_STRUCT * matchedPhase;

   long          quake_index
      ,          phase_index
      ;


   InitGlobalAmp(&_amp_data);

   switch ( StringToAmp( &_amp_data, msg ) )
   {
     case GLOBAL_MSG_FORMATERROR:
		  sprintf( Text, "glproc_pick: amp message format invalid:\n%s\n", msg );
		  glproc_status( TYPE_ERROR, ERR_PICKREAD, Text );
          return;

     case GLOBAL_MSG_VERSINVALID:
		  sprintf( Text, "glproc_pick: amp message: version invalid <%d>\n%s\n"
			     , _amp_data.version
				 , msg
				 );
		  glproc_status( TYPE_ERROR, ERR_PICKREAD, Text );
          return;

     case GLOBAL_MSG_BADPARAM:
     case GLOBAL_MSG_NULL:
		  sprintf( Text, "glproc_pick: Error decoding amp message:\n%s\n", msg );
		  glproc_status( TYPE_ERROR, ERR_PICKREAD, Text );
          return;
   }


   if(Debug>=2)
   {
      logit( "e"
           , "Parsed amp msg:  %d %d %d %d .%s.%s.%s.%s. %s %d %f %0.2f\n"
           , _amp_data.logo.type
           , _amp_data.logo.mod
           , _amp_data.logo.instid
           , _amp_data.sequence
           , _amp_data.station
           , _amp_data.channel
           , _amp_data.network
           , _amp_data.location
           , _amp_data.amp_time
           , _amp_data.amptype
           , _amp_data.adcounts
           , _amp_data.period
           );
   }

   /* Attempt to find a matching Pick to check if
   ** it has already been associated with a quake.
   */

   /*
   ** Look through the entire list of Picks to attempt to find the pick
   ** referenced by this message.
   **
   ** Since arriving picks are inserted into Picks[] at an incrementing
   ** point tracked with the variable NextPickIndex, this check should
   ** start at NextPickIndex - 1 and work backwards.
   ** (Based on the premise that amps will more likely apply to picks that have
   ** recently arrived, rather than older ones.)
   */
   if ( (start_index = (NextPickIndex - 1)) < 0 )
   {
      /* insert location at lowest index, start checking at highest */
      start_index = MAX_PICKS - 1;
   }

   /*
   **  start_index just holds the starting location,
   **  check_index is the working location.
   */
   check_index = start_index;


   do
   {
      /*
      ** A match is found when the pick sequence number,
      ** module id and installation id of the amp in the arriving
      ** amp message are the same as those for an item in the
      ** picks array.
      */
      if (           _amp_data.sequence    == Picks[check_index].pick.sequence
          &&         _amp_data.logo.mod    == Picks[check_index].pick.logo.mod
          &&         _amp_data.logo.instid == Picks[check_index].pick.logo.instid
/*
          && strcmp( _amp_data.station     ,  Picks[check_index].pick.station ) == 0
*/
         )
      {
         /*
         ** The amp described in the arriving amp message
         ** matches the pick at check_index.
         ** (There can be only one matching pick for an amp.)
         */

         /*
         ** NOTE: this following block implies that an arriving
         **       amp will only be stored in a matching pick in
         **       the Picks[] array until the pick has been linked
         **       (associated) with a quake.
         **       When the association is made, all data from the
         **       pick (including all amp data) are moved into a
         **       phase element in the quake.
         **       Subsequently, all amps matched to the pick
         **       are stored directly in the quake.
         **
         ** THEREFORE: Once a pick has been associated with
         **       a quake, the Picks[] element is never again
         **       updated, it is only used to maintain the
         **       indexing into Quakes[].
         */

         if ( (quake_index = Picks[check_index].quake_index) == -1 )
         {
            /*
            ** This pick has not yet been associated with a quake
            **
            ** Thus, it is appropriate to store the amp information
            ** in the pick structure, awaiting a later association.
            */
            matchedPhase = &Picks[check_index].pick;
         }
         else
         {
            /*
            ** The pick that matches this amp has already
            ** been associated with a quake, associate this
            ** amp with the same quake.
            */

            /*
            ** Picks[check_index].quake_index != -1
            ** implies that Picks[check_index].phase_index
            ** will also have a good value refering to the
            ** index [in the quake] of the phase that was
            ** created from the pick that is the current match.
            */
            phase_index = Picks[check_index].phase_index;

            matchedPhase = &Quakes[quake_index].quake.phases[phase_index];
         }

         /*
         ** copy the amp information into either the unassociated pick
         ** or the associated phase (made from the unassociated pick).
         */


         switch ( AddAmpToPhase( matchedPhase, &_amp_data ) )
         {
           case GLOBAL_MSG_NULL:
                logit( "e"
                     , "Error adding amp to phase\n"
                     );
                glproc_status( TYPE_ERROR, ERR_AMPREAD, "Error adding amp to phase" );
                return;

           case GLOBAL_MSG_MAXCHILDREN:
                logit( "e"
                     , "Can't add amp to phase: too many children\n"
                     );
                glproc_status( TYPE_ERROR, ERR_AMPREAD, "Too many amps to add to phase" );
                return;
         }

         /* only one match possible, time to return */
         return;

      }  /* matching pick found for amp */

      /* decriment the check index
      ** in preparation to check next pick
      */
      if ( --check_index < 0 )
      {
         check_index = MAX_PICKS - 1;
      }

   } while ( check_index != start_index );


   /* ONLY ARRIVE HERE IF NO MATCHING PICK FUOND */

   /*
   ** If a matching pick is not found, put this amp into the
   ** unassociated amps array.
   */

   if ( Amps[NextAmpIndex].pick_seq != -1 )
   {
       logit( ""
            , "Overwriting an amp (%d) that was never associated.%s%s%d%s%s%s"
            , Amps[NextAmpIndex].pick_seq
            , "  There are two likely causes:\n"
            , "  1. The Amps array is too small for the data flow (currently MAX_UNASSOC_AMPS = "
            , MAX_UNASSOC_AMPS
            , ")\n"
            , "  2. The matching pick, in fact, never did arrive at globalproc\n"
            , "     (probably okay after a restart).\n"
            );
       glproc_status( TYPE_ERROR, ERR_OVERWRITEAMP, "Overwriting unassociated amp" );
   }

   /* array header */
/*
   Amps[NextAmpIndex].logo.type   = _amp_data.logo.type;
   Amps[NextAmpIndex].logo.mod    = _amp_data.logo.mod;
   Amps[NextAmpIndex].logo.instid = _amp_data.logo.instid;
   Amps[NextAmpIndex].pick_seq    = _amp_data.sequence;
*/
logit ("", "Calling Init and CopyGlobalAmp2AmpLine\n");
   InitGlobalAmpLine( &Amps[NextAmpIndex].amp );
   CopyGlobalAmp2AmpLine( &Amps[NextAmpIndex].amp, &_amp_data );

   /* increment inset point for next unassociated amp */
   if ( ++NextAmpIndex == MAX_UNASSOC_AMPS )
   {
      NextAmpIndex = 0;
   }

}


/********************************************************************************
**	glproc_check() writes messages & files for hypocenters whose latency has expired
*********************************************************************************/
void glproc_check( double p_currenttime )
{
   GLOBAL_LOC_BUFFER _buffer;

   int quake_index;

   /*
   ** Loop thru all hypocenters, see if it's time to report any
   */
   for( quake_index = 0 ; quake_index < MAX_QUAKES ; quake_index++ )
   {
      /* skip quakes that aren't alive */
      if( Quakes[quake_index].state == QUAKE_DEAD ) continue;


      /* skip quakes that are not yet ripe for reporting */
      if( p_currenttime < Quakes[quake_index].reporting_time ) continue;


      if( Quakes[quake_index].state == QUAKE_REPORTED )
      {
         /* This quake hasn't been modified (since last reported) */
         if( Report[Quakes[quake_index].reporting_count].force == FALSE )
         {
            /* This is not a forced report, so skip it */
            continue;
         }
      }

      if( Quakes[quake_index].reporting_count == Nreport )		/* no more reports for this event */
      {
         Quakes[quake_index].state = QUAKE_DEAD;		/* kill it */
         if( 1 <= Debug )
         {
            logit("","Killed event %d for over-reporting\n",quake_index);
         }
         continue;
      }


      if ( Quakes[quake_index].quake.nphs < MinPhases )
      {
         continue;  /* too few phases */
      }


      /* *****************
      ** Report this event
      */
      logit( "et"
           , "Reporting event %d at interval %d (force=%d); state=%d\n"
           , Quakes[quake_index].quake.event_id
           , Report[Quakes[quake_index].reporting_count].interval
           , Report[Quakes[quake_index].reporting_count].force
           , Quakes[quake_index].state
           );

      /* number of times we've been reported */
      Quakes[quake_index].reporting_count++;

      /* calculate next report time */
      Quakes[quake_index].reporting_time = p_currenttime
                                         + Report[Quakes[quake_index].reporting_count].interval
                                         ;

      /* must be modified before reported again (unless a forced report) */
      Quakes[quake_index].state = QUAKE_REPORTED;


      if( 1 <= Debug )
      {
         char cdate[40];
         date20( p_currenttime, cdate );
         logit( ""
              , "%s id:%8ld report:%3d #### \n"
              , cdate + 10
              , Quakes[quake_index].quake.event_id
              , Quakes[quake_index].quake.origin_id /* was: Quakes[quake_index].reporting_count */
              );
      }


      if( 1 <= Debug )
      {
         logit( ""
              , "%8ld %d %s%9.4f%10.4f%6.2f%5.2f%5.1f%4.0f%3d\n"
              , Quakes[quake_index].quake.event_id
              , Quakes[quake_index].quake.origin_id
              , Quakes[quake_index].origin_time
              , Quakes[quake_index].quake.lat
              , Quakes[quake_index].quake.lon
              , Quakes[quake_index].quake.depth
              , Quakes[quake_index].quake.rms
              , Quakes[quake_index].quake.dmin
              , Quakes[quake_index].quake.gap
              , Quakes[quake_index].quake.pick_count
              );
      }


      /* **********************
      ** build the event message
      */

      WriteLocToBuffer( &Quakes[quake_index].quake, _buffer, sizeof(_buffer) );

      /*
      ** Update the origin version number for next time
      */
      Quakes[quake_index].quake.origin_id++;

      /* ****************************************
      ** write event message to the output ring
      */
      if ( 1 <= Debug )
      {
         logit( "e", "Output Event message (%d bytes):\n%s\n", strlen(_buffer), _buffer );
      }

      if( tport_putmsg( &OutRegion, &OutLogo, strlen(_buffer), _buffer ) != PUT_OK )
      {
         logit( "et", "%s:  Error sending location message.\n", MyModName );
      }

   } /* all quakes */

   fflush(stdout);
}


/******************************************************************************
** glproc_status() builds a heartbeat or error msg & puts it into shared memory
*******************************************************************************/
void glproc_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char	       msg[256];
   long	       size;
   time_t        current_time = time(NULL);

   /* ******************
   ** Build the message
   */
   logo.instid = MyInstId;
   logo.mod    = MyModId;
   logo.type   = type;


   if( type == TYPE_HEARTBEAT )
   {
      sprintf( msg, "%ld %ld\n\0", (long) current_time, (long) MyPid );
   }
   else if( type == TYPE_ERROR )
   {
      sprintf( msg, "%ld %hd %s\n\0", (long) current_time, ierr, note );

	    logit( "et", "%s: %s\n", MyModName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */ 	

   /* ***********************************
   ** Write the message to shared memory
   */
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
      if( type == TYPE_HEARTBEAT )
      {
         logit( "et" ,"%s:  Error sending heartbeat.\n", MyModName );
      }
      else if( type == TYPE_ERROR )
      {
         logit("et", "%s:  Error sending error:%d.\n", MyModName, ierr );
      }
   }
}


/******************************************************************************
**		glproc_config() processes command file(s) using kom.c functions
**					 exits if any errors are encountered
**
** RETURNS:  TRUE = good
**          FALSE = error
**
*******************************************************************************/
int glproc_config( char *configfile )
{
   int     return_code = TRUE;

   char  * com;           /* command (i.e., the first token on a command line */
   char  * _token;        /* token parsed from command line                   */
   char 	processor[15]; /* which component is using the config line */
   int		nfiles;        /* number of opened files */
   int		success;
   int		loop_counter;

   int reading = TRUE; /* still reading config files? */

   const char flag_log       =  0
            , flag_moduleid  =  1
            , flag_inring    =  2
            , flag_outring   =  3
            , flag_picksfrom =  4
            , flag_assocfrom =  5
            , flag_report    =  6
            , flag_heartbeat =  7
            , flag_minphase  =  8
            , flag_count     =  9 /* array loop boundary, must be <previous item> + 1 */
            ;
   char check_vars[9]; /* array size must be flag_count */

   /* these are used for error reporting (at end of method) */
   const char *ConfigTags[] =
   {
       "LogFile"
     , "MyModuleId"
     , "InRingName"
     , "OutRingName"
     , "GetPicksFrom"
     , "GetAssocFrom"
     , "Report"
     , "HeartBeatInterval"
     , "MinPhases"
   };


   /* ***************************************************
   ** Set to FALSE init flags for each required command
   */
   for( loop_counter = 0 ; loop_counter < flag_count ; loop_counter++ )
   {
      check_vars[loop_counter] = FALSE;
   }

   LogoCount = 0;


   /* ********************************
   ** Open the main configuration file
   */
   nfiles = k_open( configfile );

   if ( nfiles == 0 )
   {
      logit( "e"
           , "globalproc: Error opening command file <%s>\n"
           , configfile
           );
		 return_code = FALSE;
      reading = FALSE;
   }

   /* *************************
   ** Process all command files
   */
   while( reading == TRUE && 0 < nfiles )    /* While there are command files open */
   {
      while( reading == TRUE && k_rd() )     /* Read next line from active file  */
      {
         com = k_str();   /* Get the first token from line */

         /* ******************************
         ** Ignore blank lines & comments
		    */
         if( com == NULL )
         {
            continue;
         }

         if( com[0] == '#' )
         {
            continue;
         }

         /*
         **  This loop is just a convenience to simplify the
         **  enclosed code -- once the command token is
         **  matched to identify the command, the processing is
         **  done and a 'continue' command is called to jump to
         **  the end of this "loop."
         */
         do   /* while ( 0 ) */
         {

            /* ********************************
            ** Open a nested configuration file
            */
            if( com[0] == '@' )
            {
               success = nfiles + 1;
               nfiles  = k_open( &com[1] );
               if ( nfiles != success )
               {
                  logit( "e"
                       , "globalproc: Error opening command file <%s> can't continue parsing\n"
                       , &com[1]
                       );
                  /* sleep 5 seconds so user can see the error message */
                  sleep_ew(5000);
                  return_code = FALSE;
                  reading = FALSE;
               }
               continue;
            }

            /* **********************************
            ** Process anything else as a command
            */
            strcpy( processor, "glproc_config" );

            /* ******************************
            ** Numbered commands are required
            */

            if( k_its("LogFile") )
            {
               LogSwitch = k_int();
               check_vars[flag_log] = TRUE;
               continue;
            }

            if( k_its("MyModuleId") )
            {
               _token = k_str();
               if ( _token != NULL )
               {
                  strcpy( MyModName, _token );
               }
               check_vars[flag_moduleid] = TRUE;
               continue;
            }

            if( k_its("InRingName") )
            {
               _token = k_str();
               if ( _token != NULL )
               {
                  strcpy( InRingName, _token );
               }
               check_vars[flag_inring] = TRUE;
               continue;
            }

            if( k_its("OutRingName") )
            {
               _token = k_str();
               if ( _token != NULL )
               {
                  strcpy( OutRingName, _token );
               }
               check_vars[flag_outring] = TRUE;
               continue;
            }

            /*  *********************************************
            **  Parse Logo parts identifying message types to
            **  associate.
            **
            **  Get installation and module, make three logos
            **  with types: PICK2K, CODA2K and AMP_NSN
            */
            if( k_its("GetPicksFrom") )
            {
               /*
               ** Ensure that there is space to add three logos
               */
               if ( MAXLOGO < ( LogoCount + 2 ) )
               {
                  logit( "e"
                       , "globalproc: Too many <Get*> commands in <%s>; max = %d\n"
                       , configfile
                       , (int) MAXLOGO/2
                       );
                  return_code = FALSE;
                  continue;
               }

               if( ( _token = k_str() ) == NULL )
               {
                  if( GetInst( _token, &GetLogo[LogoCount].instid ) != 0 )
                  {
                     logit( "e"
                          , "globalproc: Invalid installation name <%s> in <GetPicksFrom> cmd\n"
                          , _token
                          );
                     return_code = FALSE;
                  }
               }

               if( ( _token = k_str() ) == NULL )
               {
                  if( GetModId( _token, &GetLogo[LogoCount].mod ) != 0 )
                  {
                     logit( "e"
                          , "globalproc: Invalid module name <%s> in <GetPicksFrom> cmd\n"
                          , _token
                          );
                     return_code = FALSE;
                  }
               }

               GetLogo[LogoCount].type =  TYPE_PICK_GLOBAL;
               LogoCount++;

               GetLogo[LogoCount].type = TYPE_AMP_GLOBAL;
               GetLogo[LogoCount].mod = GetLogo[LogoCount-1].mod;
               GetLogo[LogoCount].instid = GetLogo[LogoCount-1].instid;
               LogoCount++;

               check_vars[flag_picksfrom] = TRUE;
               continue;
            }


            /*
            ** Parse Logo parts identifying the associator messages
            **
            **  Get installation and module, make two logos
            **  with types: QUAKE2K and LINK
            */
            if( k_its("GetAssocFrom") )
            {
               /*
               ** Ensure that there is space to add two logos
               */
               if ( MAXLOGO < (LogoCount + 2)  )
               {
                  logit( "e"
                       , "globalproc: Too many <Get*From> commands in <%s>; max = %d\n"
                       , configfile
                       , (int) MAXLOGO / 2
                       );
                  return_code = FALSE;
                  continue;
               }

               if( ( _token = k_str() ) == NULL )
               {
                  if( GetInst( _token, &GetLogo[LogoCount].instid ) != 0 )
                  {
                     logit( "e"
                          , "globalproc: Invalid installation name <%s> in <GetAssocFrom> line\n"
                          , _token
                          );
                     return_code = FALSE;
                  }
               }

               if( ( _token = k_str() ) == NULL )
               {
                  if( GetModId( _token, &GetLogo[LogoCount].mod ) != 0 )
                  {
                     logit( "e"
                          , "globalproc: Invalid module name <%s> in <GetAssocFrom> line\n"
                          , _token
                          );
                     return_code = FALSE;
                  }
               }

               GetLogo[LogoCount].type = TYPE_QUAKE2K;
               LogoCount++;

               GetLogo[LogoCount].type = TYPE_LINK;
               GetLogo[LogoCount].instid = GetLogo[LogoCount-1].instid;
               GetLogo[LogoCount].mod = GetLogo[LogoCount-1].mod;
               LogoCount++;

               check_vars[flag_assocfrom] = TRUE;
               continue;
            }

            if( k_its("Report") )
            {
               if( Nreport == MAX_REPORT )
               {
                  logit( "e", "Too many Report lines. Max = %d\n", MAX_REPORT );
                  return_code = FALSE;
               }
               else
               {
                  Report[Nreport].interval = k_int();
                  Report[Nreport].force = k_int();
                  Nreport++;

                  check_vars[flag_report] = TRUE;
               }
               continue;
            }

            if( k_its("HeartBeatInterval") )
            {
               HeartBeatInterval = k_int();
               check_vars[flag_heartbeat] = TRUE;
               continue;
            }

            if( k_its("MinPhases") )
            {
               MinPhases = k_int();
               if ( MinPhases < 4 )
               {
                  logit( "e"
                       , "WARNING: <MinPhases> %d is < Minimum number of phases needed to find a location (4)\n"
                       , MinPhases
                       );
               }
               check_vars[flag_minphase] = TRUE;
               continue;
            }

            /* ******************
            ** Optional commands
            */
            else if( k_its("Debug") )
            {
               Debug = k_int();
               continue;
            }

            /* **************************************************
            ** Some commands may be processed by other functions
            */
            if ( t_com() )
            {
               strcpy( processor, "t_com" );
               continue;
            }

            if ( site_com() )
            {
               strcpy( processor, "site_com" );
               continue;
            }

            /*
            ** If a command line was identified, there should be a
            ** continue line that jumps to the while(), skipping this
            ** next bit.
            */
            logit( "e"
                 , "globalproc: Unknown command <%s> in file <%s>.\n"
                 , com
                 , configfile
                 );

            return_code = FALSE;

        } while( 0 );


        /* ****************************************************
        ** See if there were any errors processing the command
        */
        if( k_err() )
        {
           logit( "e"
                , "globalproc: Bad <%s> command for %s() in <%s>\n"
                , com
                , processor
                , configfile
                );
           return_code = FALSE;
        }

      } /* each command file line */

      /* ******************************
      ** Close the current command file
      */
      nfiles = k_close();

   } /* reading command files */


   if ( return_code == TRUE )
   {
      /* no error previously identified */
      /* ****************************************************************
      ** After all files are closed, check init flags for missed commands
      */
      int found_miss = FALSE;
      for ( loop_counter = 0 ; loop_counter < flag_count ; loop_counter++ )
      {
          if ( check_vars[loop_counter] == FALSE )
          {
             if ( found_miss == FALSE )
             {
                logit( "e", "globalproc: ERROR, no" );
                found_miss  = TRUE;
             }
                logit( "e", " <%s>", ConfigTags[loop_counter] );
          }
      }

      if ( found_miss == TRUE )
      {
                logit( "e", " command(s) in <%s>; exiting!\n", configfile );
         return_code = FALSE;
      }
   }

   return return_code;
}


/****************************************************************************
**  glproc_lookup( )
**
**  Look up memory keys, installations, modules, message types
**  from earthworm.h tables
**
** RETURNS:  TRUE = good
**          FALSE = error
**
****************************************************************************/
int glproc_lookup( void )
{
   int return_code = TRUE; /* set to FALSE to indicate fatal error */

   /* *********************************
   ** Look up message types of interest
   */
   if ( GetType( "TYPE_HEARTBEAT", &TYPE_HEARTBEAT ) != 0 )
   {
	    logit( "e"
		      , "globalproc: Invalid message type <TYPE_HEARTBEAT>; exiting!\n"
           );
      return_code = FALSE;
   }
   if ( GetType( "TYPE_ERROR", &TYPE_ERROR ) != 0 )
   {
	    logit( "e"
			   , "globalproc: Invalid message type <TYPE_ERROR>; exiting!\n"
           );
      return_code = FALSE;
   }
   if ( GetType( "TYPE_QUAKE2K", &TYPE_QUAKE2K ) != 0 )
   {
	    logit( "e"
		      , "globalproc: Invalid message type <TYPE_QUAKE2K>; exiting!\n"
           );
      return_code = FALSE;
   }
   if ( GetType( "TYPE_LINK", &TYPE_LINK ) != 0 )
   {
	    logit( "e"
		      , "globalproc: Invalid message type <TYPE_LINK>; exiting!\n"
           );
      return_code = FALSE;
   }
   if ( GetType( "TYPE_PICK_GLOBAL", &TYPE_PICK_GLOBAL ) != 0 )
   {
	    logit( "e"
		      , "globalproc: Invalid message type <TYPE_PICK2K>; exiting!\n"
           );
      return_code = FALSE;
   }
   if ( GetType( "TYPE_AMP_GLOBAL", &TYPE_AMP_GLOBAL ) != 0 )
   {
	    logit( "e"
		      , "globalproc: Invalid message type <TYPE_PICK2K>; exiting!\n"
           );
      return_code = FALSE;
   }
   if ( GetType( "TYPE_LOC_GLOBAL", &TYPE_LOC_GLOBAL ) != 0 )
   {
	    logit( "e"
		      , "globalproc: Invalid message type <TYPE_GLOBALLOC>; exiting!\n"
           );
      return_code = FALSE;
   }

   return return_code;
}

/* ------------------------------------------------------------------------- */

GLOBAL_MSG_STATUS CopyGlobalAmpLine(       GLOBAL_AMPLINE_STRUCT * p_destin
                                   , const GLOBAL_AMPLINE_STRUCT * p_source
                                   )
{
   if ( p_source == NULL || p_destin == NULL )
   {
     return GLOBAL_MSG_NULL;
   }
           p_destin->version     = p_source->version;
           p_destin->logo.instid = p_source->logo.instid;
           p_destin->logo.mod    = p_source->logo.mod;
           p_destin->logo.type   = p_source->logo.type;
   strcpy( p_destin->station     , p_source->station  );
   strcpy( p_destin->channel     , p_source->channel  );
   strcpy( p_destin->network     , p_source->network  );
   strcpy( p_destin->location    , p_source->location );
   strcpy( p_destin->amp_time    , p_source->amp_time );
           p_destin->amptype     = p_source->amptype;
           p_destin->adcounts    = p_source->adcounts;
           p_destin->period      = p_source->period;

           p_destin->pick_sequence      = p_source->pick_sequence;

   return GLOBAL_MSG_SUCCESS;
}

/* ------------------------------------------------------------------------- */

GLOBAL_MSG_STATUS CopyGlobalAmp2AmpLine(       GLOBAL_AMPLINE_STRUCT * p_destin
                                       , const GLOBAL_AMP_STRUCT     * p_source
                                       )
{
   if ( p_source == NULL || p_destin == NULL )
   {
     return GLOBAL_MSG_NULL;
   }
           p_destin->logo.instid   = p_source->logo.instid;
           p_destin->logo.mod      = p_source->logo.mod;
           p_destin->logo.type     = p_source->logo.type;
           p_destin->pick_sequence = p_source->sequence;
   strcpy( p_destin->station       , p_source->station  );
   strcpy( p_destin->channel       , p_source->channel  );
   strcpy( p_destin->network       , p_source->network  );
   strcpy( p_destin->location      , p_source->location );
   strcpy( p_destin->amp_time      , p_source->amp_time );
           p_destin->amptype       = p_source->amptype;
           p_destin->adcounts      = p_source->adcounts;
           p_destin->period        = p_source->period;

logit ("", "Copied ampline entry with seq %d\n", p_destin->pick_sequence);

   return GLOBAL_MSG_SUCCESS;
}

/* ------------------------------------------------------------------------- */

GLOBAL_MSG_STATUS CopyGlobalPHS2PhaseLine(       GLOBAL_PHSLINE_STRUCT * p_destin
                                         , const GLOBAL_PHSLINE_STRUCT * p_source
                                         )
{
   short _a;

   if ( p_source == NULL || p_destin == NULL )
   {
     return GLOBAL_MSG_NULL;
   }
           p_destin->logo.instid = p_source->logo.instid;
           p_destin->logo.mod    = p_source->logo.mod;
           p_destin->logo.type   = p_source->logo.type;
           p_destin->sequence    = p_source->sequence;
   strcpy( p_destin->station     , p_source->station );
   strcpy( p_destin->channel     , p_source->channel );
   strcpy( p_destin->network     , p_source->network );
   strcpy( p_destin->location    , p_source->location );
   /* global phase time is now an epoch 
   strcpy( p_destin->pick_time   , p_source->pick_time ); */
           p_destin->tPhase      = p_source->tPhase;

   strcpy( p_destin->phase_name  , p_source->phase_name );
           p_destin->quality     = p_source->quality;
           p_destin->polarity    = p_source->polarity;
   
   for ( _a = 0 ; _a < MAX_AMPS_PER_GLOBALPHASE ; _a++ )
   {
      CopyGlobalAmpLine( &p_destin->amps[_a], &p_source->amps[_a] );
   }                              

   return GLOBAL_MSG_SUCCESS;
}

/* ------------------------------------------------------------------------- */

int AddAmpToPhase( GLOBAL_PHSLINE_STRUCT * p_phase
                 , GLOBAL_AMP_STRUCT     * p_amp
                 )
{
   GLOBAL_AMPLINE_STRUCT * _destin;

   if ( p_phase == NULL || p_amp == NULL )
   {
      return GLOBAL_MSG_NULL;
   }

   _destin = &p_phase->amps[p_amp->amptype - 1];

           _destin->logo.type   = p_amp->logo.type;
           _destin->logo.mod    = p_amp->logo.mod;
           _destin->logo.instid = p_amp->logo.instid;
   strcpy( _destin->station     , p_amp->station );
   strcpy( _destin->channel     , p_amp->channel );
   strcpy( _destin->network     , p_amp->network );
   strcpy( _destin->location    , p_amp->location );
   strcpy( _destin->amp_time    , p_amp->amp_time );
           _destin->amptype     = p_amp->amptype;
           _destin->adcounts    = p_amp->adcounts;
           _destin->period      = p_amp->period;

		   _destin->pick_sequence      = p_amp->sequence;

logit ("", "Added amp with seq %d to phase with seq %d\n", p_amp->sequence, p_phase->sequence);

   return p_amp->amptype - 1;
}


