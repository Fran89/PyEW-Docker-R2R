
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: statmgr.c 6188 2014-11-14 20:59:32Z paulf $
 *
 *    Revision history:
 *     $Log: statmgr.c,v $
 *     Revision 1.17  2010/03/09 21:37:34  paulf
 *     started logging reception of TYPE_STOP messages
 *
 *     Revision 1.16  2009/12/14 19:27:32  scott
 *     Use negative limits for pager and/or email to mean no limit.
 *
 *     Revision 1.15  2007/02/27 04:59:44  stefan
 *     statmgr watches for STOP messages and will listen to more than one ring
 *
 *     Revision 1.14  2006/05/19 23:44:54  dietz
 *     Added optional command "From" to load the From field of email headers.
 *     Used by Windows only.
 *
 *     Revision 1.13  2006/04/26 00:25:34  dietz
 *     + Modified to allow up to 10 pagegroup commands in statmgr config file.
 *     + Modified descriptor files to allow optional module-specific settings for
 *     pagegroup (up to 10) and mail (up to 10) recipients. Any pagegroup or
 *     mail setting in a descriptor file override the statmgr config settings.
 *     + Modified logfile name to use the name of the statmgr config file (had
 *     been hard-coded to 'statmgr*'.
 *     + Modified logging of configuration and descriptor files.
 *
 *     Revision 1.12  2004/07/16 18:12:42  dietz
 *     increased logit buffer to MAXMSG*2; cleaned up code formatting
 *
 *     Revision 1.11  2004/06/29 16:26:57  patton
 *     Added New "Robust" token parsing to heartbeat and error message handling
 *     in statmgr.c.  This was to correct statmgr crashes where sprintf could not
 *     handle malformed messages coming out of the global associator (glass).
 *
 *     Revision 1.10  2002/07/09 23:08:49  dietz
 *     added optional command pagegroup to descriptor file.
 *     If it exists, it overrides the pagegroup command in statmgr config
 *
 *     Revision 1.9  2002/05/15 22:06:29  patton
 *     Made logit changes
 *
 *     Revision 1.8  2001/10/02 20:55:54  dietz
 *     *** empty log message ***
 *
 *     Revision 1.7  2001/10/02 20:44:19  dietz
 *     Gave the variable Subject a default value of "Earthworm Status Message".
 *     Previously had no default.  "Subject" config command overrides.
 *
 *     Revision 1.6  2001/05/10 23:52:52  kohler
 *     Bug fix.  statmgr now correctly reads nested descriptor files.
 *
 *     Revision 1.5  2001/05/09 18:02:09  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.4  2001/03/20 23:31:16  alex
 *     added 'PID unkown' to 'human intervention required' error message
 *
 *     Revision 1.3  2001/03/16 00:02:21  alex
 *     Merely a note about a fix to SendMail(). See below. Alex
 *
 *     Revision 1.2  2000/06/30 17:54:57  lombard
 *     Added check for return status of SendMail(); add transport flush
 *     on startup; added module ID name to module health messages.
 *
 *     Revision 1.1  2000/02/14 19:39:55  lucky
 *     Initial revision
 *
 *
 */

 /***************************************************************************
  *                        Status Manager (statmgr.c)                       *
  *                                                                         *
  *   Earthworm status manager program.                                     *
  *   Determines whether to report an error.                                *
  *   Generates email and paging.                                           *
  *   Monitors heartbeats of client programs.                               *
  *                                                                         *
  *  Argument 1 name of the statmgr configuration file.                     *
  ***************************************************************************/
/*
Modified by alex 7/3/97: Export would hang occasionally when trying to
recover from a broken socket link. Rather than analyzing the failure, we
decided to implement a general restart feature:
        A module, in its ".desc" file, could request to be restarted if it's
heartbeat stopped. This is requested via a command "restartMe" appearing anyhwere
in the .desc file. If the command is not present, no restart attempt will be made.
The restart attempt consists of Status Manager sending a TYPE_RESTART message,
giving the pid of the module. Startstop will pick the message up, and perform
the restart. This scheme was resorted to as attempts to have a module restart
itsself resulted in the module inheriting too much from it's sick predecessor,
and thus having related problems - e.g. weird socket effects.
        Modules wishing this service must include their Pid in ascii as part
of their heartbeat. The heartbeat is expected to be 'time-space-pid'.
*/
/*
 * Change 1/23/99, PNL: don't send RESTART message until we know the module's
 * PID.  This means we must get at least one heartbeat message from the module
 * with the module's pid included.  If the module needs to be restarted (no
 * heartbeats within specified interval) and module.desc says restartMe, then
 * statmgr sends a warning.
 */

 /* Alex, 3/15/02: No fixes to this code, but made a fix to sendmail(). It would
 free() an array which wasn't mallocd, but was declared. This seemingly fixed a
 bizzare bug wherein statmgr would exit if the email address didn't have enough
 "."s in the address. This was due to not returning from the piped command to the
 mail program. Most bizzare.
 */
/* Alex 6/24/4: A long error message would cause an exit because of varying message buffer sizes.
 * Introduced the MAXMSG symbol. */

/* Philip 6/1/2014 fix 5 year old issue with .d and .desc both having heartbeatint but
 statmgr having no clue about existence of .d value. Now it checks and makes reasonable
 actions based on the tsec and HeartbeatInt values in both .d and .desc files. 
 See trac issues 2, 3, and 484. */

/* Paul Friberg introduced versioning to statmgr in May 2012 after a bug fix */

/* 2014-11-14 -> remove need for MailServer argument, which is windows centric only */
#define STATMGR_VERSION "2.0.13 - 2014-11-14"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <time_ew.h>
#include <ctype.h>
#include <sys/types.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include "statmgr.h"

int PageOut( SHM_INFO *, char [][MAXRECIPLEN], int, char * );
int SendPageHeart( SHM_INFO *, char * );
void SendRestartRequest( char* );
void SendStatusRequest( void );
int AccumulateRings( void );

#define MAXLOGO       4
#define TIMESTR_LEN  26
#define MAXMSG      512

/* Define error message numbers */
#define STARTUP         (short)0
#define SHUTDOWN        (short)1
#define ERR_UNKNOWNMOD  (short)2
#define ERR_DECODE      (short)3
#define ERR_MISSMSG     (short)4
#define ERR_TOOBIG      (short)5
#define ERR_NOTRACK     (short)6
#define MAX_BYTES_STATUS MAX_BYTES_PER_EQ

SHM_INFO   region;                /* The shared memory region  to poll 
					(place holder for current ring, if CheckAllRings is set) */
SHM_INFO   page_region;           /* The shared memory region where to send pages (same as RingName ring)*/
MSG_LOGO   GetLogo[MAXLOGO];      /* Array of requested module,type,instId  */
MSG_LOGO   logo;                  /* One item of the above to manipulate    */
short      nLogo;                 /* Number of logos being requested        */
char      *sysName;               /* From environmental variable SYS_NAME   */
static CNF cnf;                   /* Contents of configuration file         */
static int ndesc;                 /* Number of descriptor files             */
static DESCRIPTOR desc[MAXDESC];  /* Contents of descriptor files           */
static char mailServer[MAXMS][80];       /* Name of the mail server computers       */
static int nMailServer;           /* nbr of mailservers */
static char *mailProg  = NULL;    /* Path to the mail program               */
static char *Subject   = NULL;    /* String to hold the subject line        */
static char *From      = NULL;    /* String to hold the email sender info   */
static char *msgPrefix = NULL;    /* String to hold the message prefix      */
static char *msgSuffix = NULL;    /* String to hold the message suffix      */
static pid_t MyPid;               /* Used to check for termination requests */
static unsigned char TypeReqStatus; /* For returning status requests        */
RINGER   Ringset;                 /* If we monitor mult rings,find 'em here */
int currentring;                  /* The ring we're working on right now    */
int Debug = 0;			  /* optional flag for debugging messages */

int main( int argc, char *argv[] )
{
  int       j, k, n;
  int       jem;
  int       status;
  char      rec[MAXMSG];             /* actual retrieved message    */
  long      recsize;                 /* size of retrieved message   */
  MSG_LOGO  reclogo;                 /* logo of retrieved message   */
  time_t    emtime;                  /* time from statmgr           */
  time_t    t1;                      /* For pageit heartbeat        */
  time_t    tnow;                    /* For pageit heartbeat        */
  short     tporterr;                /* statmgr's code for errors   */
  char      errtxt[MAXMSG];          /* text describing error       */

  char ErrorMessage[MAXMSG];
  char HBMessage[MAXMSG];            /* Heartbeat Message           */

  char _str[MAXMSG];                 /* as big as the whole message */
  char * NextToken;
  int tokencount;
  int temp;
  int count;
  int procId, messageProcId;         /* Process IDs to compare      */
  int sleep_time=1024;		/* Sleep time, if there are missed messages, halve this */
  int all_some=0;
  int indiv_some=0;

  struct {                     /* information from a heartbeat or error msg */
    time_t   time;             /* time of heartbeat or error (from source)  */
    short    ierr;             /* error number (error msg only)             */
    char     note[MAXMSG];     /* string describing error (error msg only)  */
  } msg;


  nLogo = 0;
  nMailServer = 0;


/* Check the number of arguments.
 *******************************/
  if ( argc != 2 )
  {
    fprintf( stderr, "Usage: statmgr <configfile>\n" );
    fprintf( stderr, "Version: %s\n", STATMGR_VERSION );
    return -1;
  }

/* Initialize name of log-file & open it
 ***************************************/
  logit_init( argv[1], 0, MAXMSG*2, 1 );


/* Get the system name from the environment variable SYS_NAME
 ************************************************************/
  sysName = getenv( "SYS_NAME" );

  if ( sysName == NULL )
  {
    logit( "e",
           "statmgr: Environment variable SYS_NAME not defined; exiting.\n" );
    return -1;
  }

  if ( *sysName == '\0' )
  {
    logit( "e", "statmgr: Environment variable SYS_NAME " );
    logit( "e", "defined, but has no value; exiting.\n" );
    return -1;
  }

/* Look up local installation id
 *******************************/
  if ( GetLocalInst( &InstId ) != 0 ) {
    logit( "e",
           "statmgr: error getting local installation id; exiting.\n" );
    return -1;
  }

/* Look up message types of interest in earthworm.h tables
 *********************************************************/
  if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
    logit( "e",
           "statmgr: Can't resolve the needed <TYPE_HEARTBEAT> from earthworm.d or earthworm_global.d; Make sure this type is there. Exiting.\n" );
    return -1;
  }
  if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
    logit( "e",
           "statmgr: Can't resolve the needed <TYPE_ERROR> from earthworm.d or earthworm_global.d; Make sure this type is there. Exiting.\n" );
    return -1;
  }
  if ( GetType( "TYPE_PAGE", &TypePage ) != 0 ) {
    logit( "e",
           "statmgr: Can't resolve the needed <TYPE_PAGE> from earthworm.d or earthworm_global.d; Make sure this type is there. Exiting.\n" );
    return -1;
  }
  if ( GetType( "TYPE_RESTART", &TypeRestart ) != 0 ) {
    logit( "e",
           "statmgr: Can't resolve the needed <TYPE_RESTART> from earthworm.d or earthworm_global.d; Make sure this type is there. Exiting.\n" );
    return -1;
  }
  if ( GetType( "TYPE_STATUS", &TypeStatus ) != 0 ) {
    logit( "e",
           "statmgr: Can't resolve the needed <TYPE_STATUS> from earthworm.d or earthworm_global.d; Make sure this type is there. Exiting.\n" );
    return -1;
  }
  if ( GetType( "TYPE_STOP", &TypeStop ) != 0 ) {
    logit( "e",
           "statmgr: Can't resolve the needed <TYPE_STOP> from earthworm.d or earthworm_global.d; Make sure this type is there. Exiting.\n" );
    return -1;
  }
  if ( GetType( "TYPE_REQSTATUS", &TypeReqStatus ) != 0 ) {
    logit( "e",
           "statmgr: Can't resolve the needed <TYPE_REQSTATUS> from earthworm.d or earthworm_global.d; Make sure this type is there. Exiting.\n" );
    return -1;
  }

/* Get our own pid for restart purposes
 **************************************/
  MyPid = getpid();
  if ( MyPid == -1 )
  {
    logit( "e", "statmgr: Can't get my pid; exiting.\n" );
    return -1;
  }

/* Set restart flags to don't restart
 *************************************/
  {                    /* alex 7/3/97 */
    int i;
    for(i=0; i< MAXDESC; i++)
    {
      desc[i].restart=0;
      strcpy(desc[i].modPid, "\0") ;
    }
  }

/* Read configuration & descriptor files
 ***************************************/
  statmgr_config( argv[1] );
  logit( "" , "statmgr: Read command file <%s>\n", argv[1] );
  logit( "t", "statmgr: startup of statmgr version: %s", STATMGR_VERSION );

/* Reinitialize logit to the desired logging level
 *************************************************/
  logit_init( argv[1], 0, MAXMSG*2, cnf.logswitch );

/* Print descriptor & configuration files to log file
 ****************************************************/
  PrintCnf( cnf );
  PrintDf( desc, ndesc );
  logit( "", "\n       *****  Start Message Log  *****\n" );

/* Look up module id and installation id of the status manager.
 * Index is returned, or -1 if not found in table.
 **************************************************************/
  jem = LookUpModId( MyModId, InstId );
  if ( jem == -1 )
  {
    logit( "et",
           "statmgr: Descriptor file of statmgr not loaded; or\n" );
    logit("e", "\tEW_INSTALLATION and instId in statmgr.desc don't match.\n");
    return -1;
  }

/* Attach to shared memory ring
 ******************************/
  tport_attach( &region, cnf.ringKey );
  page_region = region;

  if (cnf.CheckAllRings == 1) {
      /* using status to get ring names */
      if (AccumulateRings()) {

        /* Flush out any old junk from all rings we're monitoring */
        for ( currentring = 0; currentring < Ringset.ringcount; currentring++ ) {
            region = Ringset.region[currentring];
            /* logit("e", "Flushing old messages from: %s \n", Ringset.ringName[currentring] ); */
            while (tport_getmsg(&region, GetLogo, nLogo, &reclogo, &recsize,
                                 rec, sizeof(rec) ) != GET_NONE)
              {}
        }
      } else {
          logit("e", "Can't get any ring information back from startstop. Reverting to just check the one ring specified in statmgr.d: %s", cnf.ringName );
          cnf.CheckAllRings = 0;
      }
  }
  if (cnf.CheckAllRings != 1) {
      Ringset.region[0] = region;
      /* Flush out any old junk from the only ring we're monitoring */
      while (tport_getmsg(&region, GetLogo, nLogo, &reclogo, &recsize,
                         rec, sizeof(rec) ) != GET_NONE)
      {}
  }
  currentring = 0;

/* Send startup message
 **********************/
  k = LookUpErrorNumber( STARTUP, jem );    /* statmgr error 0 */
  if ( k == -1 )
    logit("e", "statmgr message 0 not found\n" );
  else
  {
    time( &emtime );
    ReportErr( jem, k, &emtime, "" );
  }

/* Start timers for heartbeats and error counting
 ************************************************/
  time( &emtime );

  t1 = emtime;                           /* For pageit heartbeat */

  for ( j = 0; j < ndesc; j++ )
  {
    desc[j].hbeat.pagecnt = 0;
    desc[j].hbeat.mailcnt = 0;
    desc[j].hbeat.alive   = 1;          /* Assume alive at beginning */
    desc[j].hbeat.timer   = emtime;

    for ( k = 0; k < desc[j].nerr; k++ )
    {
      desc[j].err[k].pagecnt = 0;
      desc[j].err[k].mailcnt = 0;
      desc[j].err[k].errcnt  = 0;
      desc[j].err[k].tref    = emtime;
    }
  }

  /* initialize region here, if Checking multiple rings, then this will allways be ring 0 */
  region = Ringset.region[currentring];
/****************************************************
 *          Main program loop starts here.          *
 *     Check shared memory ring for new messages.   *
 ****************************************************/
carousel:
  status = tport_getmsg( &region, GetLogo, nLogo, &reclogo, &recsize,
                         rec, sizeof(rec)-1 );

/* No msgs to process; take care of some chores
 **********************************************/
  if ( status == GET_NONE ) 
  {
  /* Terminate if the kill flag has been set
    *****************************************/
    if ( tport_getflag( &region ) == TERMINATE ||
	  tport_getflag( &region ) == MyPid )
    {

    /* Report that the status manager has been killed
      ************************************************/
      k = LookUpErrorNumber( SHUTDOWN, jem );    /* statmgr error 1 */
      if ( k == -1 )
      {
	logit("et", "statmgr message 1 not found\n" );
	return -1;
      }
      time( &emtime );
      ReportErr( jem, k, &emtime, "" );
      tport_detach( &region );
      return 0;
    }

  /* Send heartbeat to pageit system via transport ring
    ****************************************************/
    time( &tnow );
    if ( (tnow - t1) >= cnf.heartbeatPageit )
    {
      t1 = tnow;
      SendPageHeart( &page_region, sysName );
    }

  /* See if any heartbeats have expired.
    * If the module was previously alive, report error.
    **************************************************/
    time( &emtime );
    for ( j = 0; j < ndesc; j++ )
    {
      if ( desc[j].hbeat.tsec != 0 )
      {
	if ( desc[j].hbeat.alive == 1 )
	{
	  if ( (emtime - desc[j].hbeat.timer) > (desc[j].hbeat.tsec) )
	  {
	    desc[j].hbeat.alive = 0;
	    if ((desc[j].restart==UNKNOWNPID) || ( strcmp( desc[j].modPid, "0" ) == 0 ))
	    {  /* module needs restart but I don't know its pid */
	      ReportHealth( j, &emtime,
			    "module dead, PID unknown; needs human intervention." );
	      /* sleep_ew( 1000 ); */
	      goto carousel;
	    }
	    if( desc[j].restart==STOPPED) {
		  logit("et","Statmgr: Module pid %s has been marked as intentionally stopped, so we won't try to restart it.\n", desc[j].modPid  );

	    }
	    ReportHealth( j, &emtime, "module dead" );
	    if( desc[j].restart==RESTARTME) /* then tell startstop to restart this module */
	    {
		SendRestartRequest((char*)&(desc[j].modPid)); /* alex 7/3/97 */
		logit("et","Statmgr: sent restart request for %s pid %s\n", desc[j].modName, desc[j].modPid);
	    }
	  }
	}
      }
    }
    if (currentring == 0 ) 
    {
      /* for now we sleep only on the first ring 
	(this works for both only 1 ring being checked and should for CheckAllRings) 
      *******************************************************************************/
      
      if (cnf.CheckAllRings == 1) 
      {
	if(all_some == 0)
	{
	  if (Debug) logit("t", "Debug: statmgr sleeping for %d msecs\n", sleep_time);
	  sleep_ew( sleep_time );
	} 
	else
	{
	  all_some=0;
	}
	
      }
      else
      {
	if (Debug) logit("t", "Debug: statmgr sleeping for %d msecs\n", sleep_time);
	sleep_ew( sleep_time );
      }
    }
    if (cnf.CheckAllRings == 1) 
    {
      currentring ++;
      if (currentring >= Ringset.ringcount) 
      {
          currentring = 0;
      }
      region = Ringset.region[currentring];
      if (Debug > 1)
      {
          logit("t", "Debug: Checking ring %s \n", Ringset.ringName[currentring] );
      }
      if (indiv_some!=0) all_some=1;
      indiv_some=0;
    }
    goto carousel;
  }
  else
  {
    indiv_some=1;
  }

/* Complain if there was a transport ring error
 **********************************************/
  if ( status != GET_OK )
  {
    switch ( status ) {
    case GET_TOOBIG:
      tporterr = ERR_TOOBIG;
      sprintf( errtxt, "msg[%ld]  i%d m%d t%d  too large to retrieve",
               recsize, (int) reclogo.instid, (int) reclogo.mod, (int) reclogo.type );
      break;
    case GET_NOTRACK:
      tporterr = ERR_NOTRACK;
      sprintf( errtxt, "transport.h NTRACK_GET exceeded" );
      break;
    case GET_MISS:
      tporterr = ERR_MISSMSG;
      sprintf( errtxt, "Missed msg(s)  inst:%d mod:%d typ:%d  region:%ld",
               (int) reclogo.instid, (int) reclogo.mod, (int) reclogo.type,
               region.key );
      //Only reduce sleep_time if we are checking all rings
      if (cnf.CheckAllRings == 1)
      {
	sleep_time = sleep_time/2 + sleep_time/4;
	if (sleep_time<10) sleep_time=10;
        if (sleep_time != 10) 
        { 
	   logit("t", "statmgr tport_getmsg missed messages in %s: Lowering sleep_time, new value is %d\n", 
		Ringset.ringName[currentring], sleep_time);
        } 
        else
        { 
	   logit("t", "statmgr tport_getmsg missed messages in %s", Ringset.ringName[currentring]);
        } 
      }
      break;
    }

    k = LookUpErrorNumber( tporterr, jem );
    if ( k == -1 )
        logit( "e", "statmgr message tporterr: %hd status: %d not found\n", tporterr, status );
    else {
      time( &emtime );
      ReportErr( jem, k, &emtime, errtxt );
    }

    if ( status == GET_TOOBIG ) goto carousel;
  }

/* Do the following as long as new messages are available.
 *********************************************************/
  rec[recsize]='\0';
  if(recsize>MAXMSG){
    logit("et","panic: oversized message got through transport");
    logit("et","recsize: %d inst:%d mod:%d typ:%d \n",
          recsize, (int) reclogo.instid, (int) reclogo.mod, (int) reclogo.type);
    logit("et","msg: .%s. \n",rec);
    return 0;
  }

/* Look up module id and installation id in descriptor table.
   Index is returned, or -1 if not found in table.
  **********************************************************/
  j = LookUpModId( reclogo.mod, reclogo.instid );
  if (( j == -1) && (reclogo.mod != 0))
  {
    k = LookUpErrorNumber( ERR_UNKNOWNMOD, jem );    /* statmgr error 2 */
    if ( k == -1 ) goto carousel;
    if (cnf.DontReportUnknownModule) goto carousel; 
    time( &emtime );
    sprintf( errtxt, "msg from unknown module  (statmgr doesn't have a .desc file for this one) inst:%d mod:%d typ:%d",
             (int) reclogo.instid, (int) reclogo.mod, (int) reclogo.type );
    ReportErr( jem, k, &emtime, errtxt );
    goto carousel;
  }

/* The message is a heartbeat message.
 * Reset heartbeat timer.
 * Take action if the module was dead.
 *************************************/
  if ( reclogo.type == TypeHeartBeat )
  {

  /* BEGIN new "robust" message parsing
   * JMP 06-25-2004
   ************************************/
    strcpy(HBMessage, rec);
    if (Debug) 
    {
        logit("t", "Debug: HB Observed in ring %s: inst:%d mod:%s typ:%d msg: %s",
	 Ringset.ringName[currentring],
         (int) reclogo.instid, GetModIdName((int) reclogo.mod), (int) reclogo.type, HBMessage);
    }

  /* First, count the tokens, there should be 1 or 2
   *************************************************/
    tokencount = 0;
    NextToken = strtok(HBMessage, " \n");
    while (NextToken != NULL)
    {
      tokencount++;
    /*printf("#%d: <%s> ", tokencount, NextToken);*/
      NextToken = strtok(NULL, " \n");
    }

  /*printf("There are #%d total tokens\n", tokencount);*/

    if ((tokencount > 2) || (tokencount < 1))
    {
    /* Wrong number of tokens, something's
     * messed up, return with error
      *************************************/
      k = LookUpErrorNumber( ERR_DECODE, jem );
      if ( k == -1 ) goto carousel;
      time( &emtime );
      sprintf( errtxt, "error decoding heartbeat:%s, wrong number of tokens %d, "
               "from mod:%d", rec, tokencount, (int) reclogo.mod );
      ReportErr( jem, k, &emtime, errtxt );
      goto carousel;
    }

  /* This message has at the right number
   * of tokens at least, reinitilize HBMessage
   * and move on
   ********************************************/
    strcpy(HBMessage, rec);

  /* Now decode time
   *****************/
    NextToken = strtok(HBMessage, " \n");
    strcpy( _str , NextToken );
    temp = strlen(_str); /* get length of time string */

  /* check to see that only digits are present */
    for (count = 0; count < temp; count++)
    {
      if (isdigit(_str[count]) == 0)
      {
      /* This character is not a digit, time string
       * must have only digits in it, return with error
       ************************************************/
        k = LookUpErrorNumber( ERR_DECODE, jem );
        if ( k == -1 ) goto carousel;
        time( &emtime );
        sprintf( errtxt, "error decoding heartbeat time, non-numeric "
                "char <%c> found at position %d, from mod:%d",
                 _str[count], count, (int) reclogo.mod );
        ReportErr( jem, k, &emtime, errtxt );
        goto carousel;
      }
    }

   /* seems ok, parse it out */
    msg.time = atoi(_str);
   /*printf("Copying <%s> into msg.time\n", _str);*/

   /* MOST modules have two token heartbeats, but
    * startstop doesn't, thus this two step dance
    *********************************************/
    if (tokencount == 2)
    {
    /* Now decode PID
     *****************/
      NextToken = strtok(NULL, " \n");
      strcpy( _str , NextToken );

    /* get length of PID */
      temp = strlen(_str);

    /* check to see that only digits are present */
      for (count = 0; count < temp; count++)
      {
        if (isdigit(_str[count]) == 0)
        {
        /* This character is not a digit, PID
         * must have only digits in it, return with error
         ************************************************/
          k = LookUpErrorNumber( ERR_DECODE, jem );
          if ( k == -1 ) goto carousel;
          time( &emtime );
          sprintf( errtxt, "error decoding heartbeat PID, non-numeric "
                  "char <%c> found at position %d, from mod:%d",
                   _str[count], count, (int) reclogo.mod );
          ReportErr( jem, k, &emtime, errtxt );
          goto carousel;
        }
      }

   /* seems ok, parse it out */
      strcpy(desc[j].modPid, _str);
    /*printf("Copying <%s> into desc[j].modPid\n", _str);*/
      n = 2;
    }

 /* If we learned the module pid and its desc said restartMe,
    now we record that we can restart it. */
    if (n == 2 && desc[j].restart == UNKNOWNPID ) desc[j].restart = RESTARTME;

    if ( desc[j].hbeat.tsec != 0 )
    {
      if ( desc[j].hbeat.alive == 0 )
      {
        ReportHealth( j, &msg.time, "module alive" );
        desc[j].hbeat.alive = 1;
      }

      desc[j].hbeat.timer = msg.time;
    }
  }
/* The message is a stop or restart message

 *********************************/

  if (( reclogo.type == TypeStop ) || ( reclogo.type == TypeRestart ))

  {

    messageProcId = atoi(rec);

    for(j=0; j< ndesc; j++) {

        procId = atoi(desc[j].modPid);

        if( messageProcId == procId ) {

          if (( reclogo.type == TypeStop ) && (desc[j].restart == RESTARTME)){

            /* If the desc said restartMe, now we record that we will not restart it. */

            desc[j].restart = STOPPED;
            logit( "t", "TYPE_STOP message received for %s with pid %s; module marked as stopped", desc[j].modName, desc[j].modPid );

          } else if (( reclogo.type == TypeRestart ) && (desc[j].restart == STOPPED)){

            /* If we are stopped, now we record that we     *

             * will monitor again and restart it if needed. */

            desc[j].restart = RESTARTME;

          }

          break;

        }

    }

  }


/* The message is an error message
 *********************************/
  if ( reclogo.type == TypeError )
  {
    rec[recsize] = '\0';
    strcpy(ErrorMessage, rec);

  /* First, count the tokens, there should be 2 or 3
   *************************************************/
    tokencount = 0;
    NextToken = strtok(ErrorMessage, " \n");
    while (NextToken != NULL)
    {
      tokencount++;
    /*printf("#%d: <%s> ", tokencount, NextToken);*/

      if (tokencount == 2)
      {
      /* don't want to break the error string into tokens */
        NextToken = strtok(NULL, "\0\n");
      }
      else
      {
        NextToken = strtok(NULL, " \n");
      }
    }

    /*printf("There are #%d total tokens\n", tokencount);*/

    if ((tokencount > 3) || (tokencount < 2))
    {
    /* Wrong number of tokens, something's
     * messed up, return with error
     *************************************/
      k = LookUpErrorNumber( ERR_DECODE, jem );
      if ( k == -1 ) goto carousel;
      time( &emtime );
      sprintf( errtxt, "error decoding error msg:%s, wrong number of "
              "tokens %d, from mod:%d", rec, tokencount, (int) reclogo.mod);
      ReportErr( jem, k, &emtime, errtxt );
      goto carousel;
    }

    /* This message has at the right number of tokens
     * at least, reinitilize HBMessage and move on
     ********************************************/
    strcpy(ErrorMessage, rec);

    /* Now decode time
     *****************/
    NextToken = strtok(ErrorMessage, " \n");
    strcpy( _str , NextToken );
    temp = strlen(_str); /* get length of time string */

  /* check to see that only digits are present */
    for (count = 0; count < temp; count++)
    {
      if (isdigit(_str[count]) == 0)
      {
      /* This character is not a digit, time string
       * must have only digits in it, return with error
       ************************************************/
        k = LookUpErrorNumber( ERR_DECODE, jem );
        if ( k == -1 ) goto carousel;
        time( &emtime );
        sprintf( errtxt, "error decoding error msg time, non-numeric "
                "char <%c> found at position %d, from mod:%d",
                _str[count], count, (int) reclogo.mod );
        ReportErr( jem, k, &emtime, errtxt );
        goto carousel;
      }
    }

  /* seems ok, parse it out */
    msg.time = atoi(_str);
  /*printf("Copying <%s> into msg.time\n", _str);*/

  /* Now decode Error Number
   *************************/
    NextToken = strtok(NULL, " \n");
    strcpy( _str , NextToken );
    temp = strlen(_str);  /* get length of error number */

  /* check to see that only digits are present */
    for (count = 0; count < temp; count++)
    {
      if (isdigit(_str[count]) == 0)
      {
      /* This character is not a digit, error number
       * must have only digits in it, return with error
       ************************************************/
        k = LookUpErrorNumber( ERR_DECODE, jem );
        if ( k == -1 ) goto carousel;
        time( &emtime );
        sprintf( errtxt, "error decoding error msg number, non-numeric "
                "char <%c> found at position %d, from mod:%d",
                _str[count], count, (int) reclogo.mod );
        ReportErr( jem, k, &emtime, errtxt );
        goto carousel;
      }
    }

  /* seems ok, parse it out */
    msg.ierr = atoi(_str);
  /*printf("Copying <%s> into msg.ierr\n", _str);*/

    if (tokencount == 2)
    {
    /* No error note with this one, init msg.note to null */
      strcpy(msg.note, "");
    }
    if (tokencount == 3)
    {
    /* Now decode error note
     ***********************/

    /* don't want to break the error string into tokens */
      NextToken = strtok(NULL, "\0\n");
      strcpy( _str , NextToken );
      temp = strlen(_str); /* get length of error note */

    /* check to see that only printable characters are present */
      for (count = 0; count < temp; count++)
      {
        if ((isprint(_str[count]) == 0) && (_str[count] != '\n'))
        {
      /* This character is not a printable character, error message
       * must have only digits in it, return with error
       ************************************************/
          k = LookUpErrorNumber( ERR_DECODE, jem );
          if ( k == -1 ) goto carousel;
          time( &emtime );
          sprintf( errtxt, "error decoding error note, non-printable "
                  "char <%c>[%d] found at position %d, from mod:%d",
                   _str[count], _str[count], count, (int) reclogo.mod );
          ReportErr( jem, k, &emtime, errtxt );
          goto carousel;
        }
      }

    /* seems ok, parse it out */
      strcpy(msg.note, _str);
    /*printf("Copying <%s> into msg.note\n", _str);*/
    }

/* END new "robust" message parsing
 * JMP 06-25-2004
 **********************************/

 /* Look up error number in descriptor table.
    Index is returned, or -1 if not found in table.
   ***********************************************/
    k = LookUpErrorNumber( msg.ierr, j );
    if ( k == -1 ) goto carousel;

    /* Report error if the allowable error rate has been exceeded
    **********************************************************/
    else
    {
      if ( desc[j].err[k].errcnt == 0 )  desc[j].err[k].tref = msg.time;

      if ( ++desc[j].err[k].errcnt >= desc[j].err[k].nerr )
      {
        if ( desc[j].err[k].tsec == 0 )
          ReportErr( j, k, &msg.time, msg.note );

        else if ( (msg.time - desc[j].err[k].tref) <= desc[j].err[k].tsec )
          ReportErr( j, k, &msg.time, msg.note );

        desc[j].err[k].errcnt = 0;
      }
    }
  }

  /* Get the next message
  ********************/
  goto carousel;

} /* end main */


/*********************************************************
 *                     ReportHealth                      *
 *********************************************************/
void ReportHealth( int j,
                   time_t *ktime,          /* time of current error */
                   char *text )
{
  struct tm  tstruct;
  char       timestr[TIMESTR_LEN];

  /* If statmgr is crashing, it may be necessary to increase this value: */
  char       message[255];

  int        pageStatus;

  int sendPage = 0, sendMail = 0; /* = "send pager msg" and "send email" respectively */

 /* Build message
  **************/
  gmtime_ew( ktime, &tstruct );
  asctime_ew( &tstruct, timestr, TIMESTR_LEN );
  timestr[strlen( timestr ) - 1] = 0;
  sprintf( message, "UTC_%s  %s/%s (%s) %s%c",
           timestr, desc[j].sysName, desc[j].modName, desc[j].modIdName,
           text, 0 );

  if ( desc[j].hbeat.page < 0 || desc[j].hbeat.pagecnt < desc[j].hbeat.page ) {
    sendPage = 1;
    strcat( message, " Page sent." );
  }

  if ( desc[j].nmail > 0 && (desc[j].hbeat.mail < 0 || desc[j].hbeat.mailcnt < desc[j].hbeat.mail )) {
    sendMail = 1;
    strcat( message, " Mail sent." );
  }

  strcat( message, "\n\0" );

  /* Send message to computer terminal & logfile
   *********************************************/
  logit( "e", "%s", message );
  fflush( stderr );

 /* Send pager message to transport ring
  **************************************/
  if ( sendPage )
  {
    if( desc[j].npagegroup != 0 ) {
      pageStatus = PageOut( &page_region, desc[j].pagegroup, desc[j].npagegroup, message );
    } else {
      pageStatus = PageOut( &page_region, cnf.pagegroup, cnf.npagegroup, message );
    }
    if( pageStatus != 0 ) logit( "et", "statmgr: pageStatus: %d\n", pageStatus );
    desc[j].hbeat.pagecnt++;
  }

 /* Send email
  ************/
  if ( sendMail && cnf.nmail > 0)
  {
      int i;
      if (nMailServer == 0) {
              if ( SendMail( cnf.mail, cnf.nmail, mailProg, Subject,
                      message, msgPrefix, msgSuffix, NULL, From ) < 0)
              	logit("et", "statmgr: error sending mail without MailServer specified\n");
      } else {
          for ( i=0; i<nMailServer; i++ ) {
              if ( SendMail( cnf.mail, cnf.nmail, mailProg, Subject,
                      message, msgPrefix, msgSuffix, mailServer[i], From ) >= 0)
                  break;
          }
          if ( i == nMailServer)
              logit("et", "statmgr: error sending mail\n");
          }

    desc[j].hbeat.mailcnt++;
  }
}


/************************************************************
 *                        ReportErr                       *
 ************************************************************/
void ReportErr( int j,
                  int k,
                  time_t *ktime,       /* time of current error */
                  char   *note )       /* optional error message to print out */
{
  struct tm  tstruct;
  char       timestr[TIMESTR_LEN];
  char       message[512];
  int        pageStatus;
  int        mailStatus;

 /* Build message
  ***************/
  gmtime_ew( ktime, &tstruct );
  asctime_ew( &tstruct, timestr, TIMESTR_LEN );
  timestr[strlen( timestr ) - 1] = 0;

  if ( strlen( note ) < (size_t) 4 )
    sprintf( message, "UTC_%s  %s/%s %s%c",
             timestr, desc[j].sysName, desc[j].modName, desc[j].err[k].text, 0 );
  else
    sprintf( message, "UTC_%s  %s/%s %s%c",
             timestr, desc[j].sysName, desc[j].modName, note, 0 );

  if ( desc[j].err[k].pagecnt < desc[j].err[k].page )
    strcat( message, " Page sent." );

  if ( desc[j].err[k].mailcnt < desc[j].err[k].mail && desc[j].nmail > 0)
    strcat( message, " Mail sent." );

  strcat( message, "\n\0" );

 /* Send message to computer terminal & log file
  **********************************************/
  logit( "e", "%s", message );

 /* Send pager message to transport ring
  **************************************/
  if ( desc[j].err[k].pagecnt < desc[j].err[k].page )
  {
    if( desc[j].npagegroup != 0 ) {
      pageStatus = PageOut( &page_region, desc[j].pagegroup, desc[j].npagegroup, message );
    } else {
      pageStatus = PageOut( &page_region, cnf.pagegroup, cnf.npagegroup, message );
    }
    if( pageStatus != 0 ) logit( "et", "statmgr: pageStatus: %d\n", pageStatus );

    desc[j].err[k].pagecnt++;
  }

 /* Send email
  ************/
  mailStatus = 1;	/* pre-set this for the case of no email sending */
  if ( desc[j].err[k].mailcnt < desc[j].err[k].mail )
  {
  	int ms;
        if (nMailServer == 0) {
		  if ( SendMail( cnf.mail, cnf.nmail, mailProg, Subject,
		       message, msgPrefix, msgSuffix, NULL, From ) < 0 )
	    	     logit("et", "statmgr: error sending email, with no MailServer set\n");
	} else {
  	    for ( ms = 0; ms < nMailServer; ms++ ) {
		if( desc[j].nmail != 0 ) {
		  mailStatus = SendMail( desc[j].mail, desc[j].nmail, mailProg, Subject,
								 message, msgPrefix, msgSuffix, mailServer[ms], From );
		} else if (cnf.nmail > 0) {
		  mailStatus = SendMail( cnf.mail, cnf.nmail, mailProg, Subject,
								 message, msgPrefix, msgSuffix, mailServer[ms], From );
		}
		if( mailStatus >= 0 ) 
			break;
	    }
	    if( ms == nMailServer) logit("et", "statmgr: error sending email\n");
	}
        desc[j].err[k].mailcnt++;
  }
}

/************************************************************
 *                   SendRestartRequest                     *
 *  To send a message requesting the restarting of a module *
 ************************************************************/
void SendRestartRequest( char*  modPid )       /* pid of module as ascii string*/
{
  MSG_LOGO logo;
  char message[512];

 /* Build message
  *************/
  strcpy (message,modPid);
  strcat( message, "\n\0" );

 /* Set logo values of pager message
  ********************************/
  logo.type   = TypeRestart;
  logo.mod    = MyModId;
  logo.instid = InstId;

 /* Send restart message to transport ring
  **************************************/
  if ( tport_putmsg( &region, &logo, strlen(message), message ) != PUT_OK )
    logit("et", "Error sending restart message to transport region.\n" );

  return;

}

/*************************************************************
                        LookUpModId

   Look up module id and installation id in descriptor table.
   Index is returned, or -1 if not found in table.
**************************************************************/
int LookUpModId( unsigned char modId, unsigned char instid )
{
  int i;

  for ( i = 0; i < ndesc; i++ )
    if ( (unsigned) modId == desc[i].modId )
      if ( (unsigned) instid == desc[i].instId )
        return( i );
  return( -1 );
}


/**********************************************************
                     LookUpErrorNumber

      Look up action code in descriptor table.
      Index is returned, or -1 if not found in table.
***********************************************************/
int LookUpErrorNumber( short msg, int j )
{
  int k;

  for ( k = 0; k < desc[j].nerr; k++ )
    if ( msg == desc[j].err[k].err )
      return( k );
  return( -1 );
}


/***********************************************************************
 * statmgr_config()  processes command file using kom.c functions      *
 *                   exits if any errors are encountered               *
 ***********************************************************************/
#define DFILENAME_LEN 64
void statmgr_config( char *configfile )
{
  int      ncommand;     /* # of required commands you expect to process   */
  char     init[10];     /* init flags, one byte for each required command */
  int      nmiss;        /* number of required commands that were missed   */
  char    *com;
  char    *str;
  char     dfname[DFILENAME_LEN];   /* name of descriptor file */
  char    *defaultsubj = {"Earthworm Status Message"};
  int      nfiles;
  int      arglen;
  int      i, ip, im, j;

  /* Set to zero one init flag for each required command
  *****************************************************/
  ncommand  = 7;
  for( i=0; i < ncommand; i++ )  init[i] = 0;
  ip        = 0;   /*number of page groups loaded */
  im        = 0;   /*number of mail recipients loaded */
  ndesc     = 0;   /*number of descriptor files read  */
  cnf.npagegroup = ip;
  cnf.CheckAllRings = 0;
  cnf.DontReportUnknownModule = 0;
  cnf.nmail = im;
  msgPrefix = NULL;
  msgSuffix = NULL;
  Subject   = (char *) calloc ((strlen(defaultsubj) + 1), sizeof (char));
  strcpy( Subject, defaultsubj );

  /* Open the main configuration file
  **********************************/
  nfiles = k_open( configfile );
  if ( nfiles == 0 )
  {
    logit( "e", "statmgr: Error opening command file <%s>; exiting.\n",
           configfile );
    exit( -1 );
  }

  /* Process all command files
  ***************************/
  while ( nfiles > 0 )            /* While there are command files open */
  {
    while ( k_rd() )           /* Read next line from active file  */
    {
      com = k_str();         /* Get the first token from line */

      /* Ignore blank lines & comments
      *******************************/
      if( !com )           continue;
      if( com[0] == '#' )  continue;

      /* Open a nested configuration file
      **********************************/
      if( com[0] == '@' )
      {
        int success = nfiles+1;
        nfiles  = k_open(&com[1]);
        if ( nfiles != success )
        {
          logit("e",
                   "statmgr: Error opening command file <%s>; exiting.\n",
                   &com[1] );
          exit( -1 );
        }
        continue;
      }

      /* Process anything else as a command
      ************************************/
      /* Name of transport ring to live on
      ***********************************/
/*0*/ if( k_its( "RingName" ) )
      {
        str = k_str();
        if(str)
        {
          strcpy( cnf.ringName, str );
          if( (cnf.ringKey = GetKey(cnf.ringName)) == -1 )
          {
            logit("e",
                    "statmgr: Invalid ring name <%s>; exiting.\n",
                    cnf.ringName );
            exit( -1 );
          }
        }
        init[0] = 1;
      }

      /* Interval between heartbeats sent to pageit
      ********************************************/
/*1*/ else if( k_its( "heartbeatPageit" ) )
      {
        cnf.heartbeatPageit = k_int();
        init[1] = 1;
      }

      /* Read pager group name
      ***********************/
/*2*/ else if( k_its( "pagegroup" ) )
      {
        if( ip >= MAXRECIP )
        {
          logit("e", "statmgr: Too many <pagegroup> commands in <%s>;"
                " max=%d; exiting.\n", configfile, MAXRECIP );
          exit( -1 );
        }
        str = k_str();
        if( !str )
        {
          logit("e","statmgr: No argument for <pagegroup> command in <%s>; "
                "exiting.\n", configfile );
          exit( -1 );
        }
        arglen = strlen(str);
        if( arglen==0 || arglen>=MAXRECIPLEN )
        {
          logit("e","statmgr: Invalid length=%d for <pagegroup> argument in <%s>; "
                "valid=1-%d; exiting.\n",
                 arglen, configfile, MAXRECIPLEN-1 );
          exit( -1 );
        }
        strcpy(cnf.pagegroup[ip], str);
        ip++;
        cnf.npagegroup = ip;
        init[2] = 1;
      }

      /* Read names of email recipients (Not Required)
      ***********************************************/
      else if( k_its( "mail" ) )  /* optional */
      {
        if( im >= MAXRECIP )
        {
          logit("e", "statmgr: Too many <mail> commands in <%s>;"
                " max=%d; exiting.\n", configfile, MAXRECIP );
          exit( -1 );
        }
        str = k_str();
        if( !str )
        {
          logit("e","statmgr: No argument for <mail> command in <%s>; "
                "exiting.\n", configfile );
          exit( -1 );
        }
        arglen = strlen(str);
        if( arglen==0 || arglen>=MAXRECIPLEN )
        {
          logit("e","statmgr: Invalid length=%d for <mail> argument in <%s>; "
                "valid=1-%d; exiting.\n",
                 arglen, configfile, MAXRECIPLEN-1 );
          exit( -1 );
        }
        strcpy(cnf.mail[im], str);
        im++;
        cnf.nmail = im;
      }

      /* Set log switch
      ****************/
/*3*/ else if( k_its( "LogFile" ) )
      {
        cnf.logswitch = k_int();
        init[3] = 1;
      }

      /* Read descriptor file name & load it
       *************************************/
/*4*/ else if( k_its( "Descriptor" ) )
      {
        if ( ndesc >= MAXDESC )
        {
          logit("e", "statmgr: Too many <Descriptor> commands in <%s>;",
                  configfile );
          logit("e", " max=%d; exiting.\n", MAXDESC );
          exit( -1 );
        }
        str = k_str();
        if ( str )
        {
          if (strlen(str) > DFILENAME_LEN-1)
          {
              logit("e", "statmgr: <Descriptor> %s length in <%s> too long;",
                  str, configfile );
              logit("e", " max=%d; exiting.\n", DFILENAME_LEN-1);
              exit( -1 );
          }
          strcpy( dfname, str );
          statmgr_getdf( dfname, desc, ndesc, nfiles );
        }
        ndesc++;
        init[4] = 1;
      }

      /* Read module id for this program
      *********************************/
/*5*/ else if( k_its( "MyModuleId" ) )
      {
        if( ( str=k_str() ) )
        {
          if ( GetModId( str, &MyModId ) < 0 )
          {
            logit("e",
                     "statmgr: Invalid MyModuleId <%s> in <%s>",
                     str, configfile );
            logit("e", "; exiting.\n" );
            exit( -1 );
          }
        }
        init[5] = 1;
      }

      /* Read installation & module to get
         status messages (errors & heartbeats) from
         ********************************************/
/*6*/ else if( k_its("GetStatusFrom") )
      {
        if ( nLogo+1 >= MAXLOGO )
        {
          logit("e",
                   "statmgr: Too many <GetStatusFrom> commands in <%s>",
                   configfile );
          logit("e", "; max=%d; exiting.\n", (int) (MAXLOGO/2) );
          exit( -1 );
        }
        if( ( str=k_str() ) )
        {
          if( GetInst( str, &GetLogo[nLogo].instid ) != 0 )
          {
            logit("e",
                     "statmgr: Invalid installation name <%s>", str );
            logit("e", " in <GetStatusFrom> cmd; exiting.\n" );
            exit( -1 );
          }
        }
        if( ( str=k_str() ) )
        {
          if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
            logit("e",
                     "statmgr: Invalid module name <%s>", str );
            logit("e", " in <GetStatusFrom> cmd; exiting.\n" );
            exit( -1 );
          }
        }

        for (j=0; j<MAXLOGO; j++) {

            GetLogo[j].instid = GetLogo[nLogo].instid;

            GetLogo[j].mod = GetLogo[nLogo].mod;

        }

        GetLogo[nLogo].type   = TypeHeartBeat;

        GetLogo[nLogo+1].type = TypeError;

        GetLogo[nLogo+2].type = TypeStop;

        GetLogo[nLogo+3].type = TypeRestart;



        nLogo  += 4;
        init[6] = 1;
      }

      /* Read the name of the computer which serves mail
      ***********************************************/
/* made optional*/ 
      else if( k_its( "MailServer" ) )
      {
        str = k_str();
        if ( str ) {
            if ( nMailServer+1 == MAXMS )
                logit( "w", "statmgr: maximum of %d mail servers allowed; '%s' ignored\n",
                    MAXMS, str );
            else {
                strcpy( mailServer[nMailServer], str );
                nMailServer++;
            }
        }
      }

      /* Read the full path to the program used to send mail
      ****************************************************/
      else if( k_its( "MailProgram" ) )  /* optional */
      {
        if( (str = k_str()) != NULL )
        {
          free( mailProg );
          mailProg = (char *) calloc ((strlen (str) + 1), sizeof (char));
          strcpy( mailProg, str );
        }
      }

      /* Set the sender/from field of the email messages
      **************************************************/
      else if( k_its( "From" ) )  /* optional */
      {
        if( (str = k_str()) != NULL )
        {
          free( From );
          From = (char *) calloc ((strlen (str) + 1), sizeof (char));
          strcpy( From, str );
        }
      }

      /* Read the text of the subject line for the messages
      ****************************************************/
      else if( k_its( "Subject" ) )  /* optional */
      {
        if( (str = k_str()) != NULL )
        {
          free( Subject );
          Subject = (char *) calloc ((strlen (str) + 1), sizeof (char));
          strcpy( Subject, str );
        }
      }



      /* Whether to check all rings or just one
      ********************************************/
      else if( k_its( "CheckAllRings" ) )
      {
        cnf.CheckAllRings = k_int();
      }
      else if( k_its( "DontReportUnknownModule" ) )
      {
        cnf.DontReportUnknownModule = k_int();
      }


      /* Read the optional message prefix
      ********************************/
      else if( k_its( "MsgPrefix" ) )  /* optional */
      {
        if( (str = k_str()) != NULL )
        {
          free( msgPrefix );
          msgPrefix = (char *) calloc ((strlen (str) + 1), sizeof (char));
          strcpy( msgPrefix, str );
        }
      }
      /* Read the optional Debug flag
      ********************************/
      else if( k_its( "Debug" ) )  /* optional */
      {
        Debug=k_int();
      }

      /* Read the optional message suffix
      ********************************/
      else if( k_its( "MsgSuffix" ) )  /* optional */
      {
        if( (str = k_str()) != NULL )
        {
          free( msgSuffix );
          msgSuffix = (char *) calloc ((strlen (str) + 1), sizeof (char));
          strcpy( msgSuffix, str );
        }
      }

      else
      {
        logit("e", "statmgr: <%s> unknown command in <%s>.\n", com, configfile );
        continue;
      }

      /* See if there were any errors processing the command
      *****************************************************/
      if ( k_err() )
      {
        logit("e", "statmgr: Bad <%s> command in <%s>; exiting.\n",
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
  if ( nmiss )
  {
    logit("e", "statmgr: ERROR, no " );
    if ( !init[0] ) logit("e", "<RingName> "        );
    if ( !init[1] ) logit("e", "<heartbeatPageit> " );
    if ( !init[2] ) logit("e", "<pagegroup> "       );
    if ( !init[3] ) logit("e", "<LogFile> "         );
    if ( !init[4] ) logit("e", "<Descriptor> "      );
    if ( !init[5] ) logit("e", "<MyModuleId> "      );
    if ( !init[6] ) logit("e", "<GetStatusFrom> "   );
    logit("e", "command(s) in <%s>; exiting.\n", configfile );
    exit( -1 );
  }

  return;
}

/***********************************************************************
 * statmgr_getdf()  processes descriptor file using kom.c functions    *
 *                  exits if any errors are encountered                *
 *                  no file-nesting is allowed in descriptor files     *
 ***********************************************************************/
void statmgr_getdf(char *descfile, DESCRIPTOR *desc, int id, int nfopen )
{
  int      ncommand;     /* # of required commands you expect to process   */
  char     init[10];     /* init flags, one byte for each required command */
  char    *com;
  char    *str;
  int      nfiles;
  int      nmiss;
  int      narg;
  int      arglen;
  int      i;
  int      jerr;         /* number of error types in this descriptor file */
  int      geterrtxt;    /* flag; expecting text description of error     */
  int      dfileHeartbeat;
  char     dotdfname[DFILENAME_LEN];

  /* Set to zero one init flag for each required command
  *****************************************************/
  ncommand  = 5;
  for( i=0; i<ncommand; i++ )  init[i] = 0;
  jerr          = 0;
  geterrtxt     = 0;
  desc[id].nerr = jerr;
  desc[id].npagegroup = 0;
  desc[id].nmail      = 0;
    
    
  /* Open the main configuration file
  **********************************/
  nfiles = k_open( descfile );
  if ( nfiles <= nfopen )
  {
    logit( "e",
           "statmgr: Error opening descriptor file <%s>; exiting.\n",
           descfile );
    exit( -1 );
  }

  /* Process one descriptor file
  *****************************/
  while( nfiles > nfopen )  /* While there's a descriptor file open */
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
      if( com[0] == '@' )
      {
        int success = nfiles+1;
        nfiles  = k_open(&com[1]);
        if ( nfiles != success )
        {
          logit( "e",
                 "statmgr: Error opening command file <%s>; exiting.\n",
                 &com[1] );
          exit( -1 );
        }
        continue;
      }

      /* Process anything else as a command
      ************************************/
      /* Read name of module
      *********************/
/*0*/ if( k_its( "modName" ) )
      {
        str = k_str();
        if(str) strcpy( desc[id].modName, str );
        init[0] = 1;
      }

      /* Read name of system on which module is running
      ************************************************/
/*1*/ else if( k_its( "system" ) )
      {
        str = k_str();
        if(str) strcpy( desc[id].sysName, str );
        init[1] = 1;
      }

      /* Read module id as a string; convert it to a number
      ****************************************************/
/*2*/ else if( k_its( "modId" ) )
      {
        str = k_str();
        if(str)
        {
          if (strlen(str) < 30)
          {
            strcpy(desc[id].modIdName, str);
            if( GetModId( str, &desc[id].modId ) < 0 )
            {
              logit( "e",
                     "statmgr: Invalid modId name <%s> ", str );
              logit( "e", "in descfile <%s>; exiting.\n",
                      descfile );
              exit( -1 );
            }
          }
          init[2] = 1;
        }
        else
        {
          logit( "e", "statmgr: modId <%s> in file <%s> too long; max is 29 chars\n",
                  str, descfile);
          exit( -1 );
        }
      }

      /* Read installation id as a string; convert it to a number
      **********************************************************/
/*3*/ else if( k_its( "instId" ) )
      {
        str = k_str();
        if(str)
        {
          if( GetInst( str, &desc[id].instId ) < 0 )
          {
            logit( "e",
                   "statmgr: Invalid installation name <%s> ",
                    str );
            logit( "e", "in descfile <%s>; exiting.\n",
                    descfile );
            exit( -1 );
          }
        }
        init[3] = 1;
      }

      /* Heartbeat interval to expect
      ******************************/
/*4*/ else if( k_its( "tsec:" ) )
      {
        str  = k_com();
        narg = sscanf( str, "tsec: %d page: %d mail: %d",
                       &desc[id].hbeat.tsec,
                       &desc[id].hbeat.page,
                       &desc[id].hbeat.mail );
        if ( narg < 3 )
        {
          logit( "e",
                 "statmgr: error decoding heartbeat from descfile <%s>; exiting.\n",
                   descfile );
          exit( -1 );
        }
        init[4] = 1;
      }

      /* Read pagegroup - overrides pagegroup commands in statmgr config
      ******************************************************************/
      else if( k_its( "pagegroup" ) )  /* optional */
      {
        if( desc[id].npagegroup >= MAXRECIP )
        {
          logit("e", "statmgr: Too many <pagegroup> commands in Descriptor <%s>;"
                " max=%d; exiting.\n", descfile, MAXRECIP );
          exit( -1 );
        }
        str = k_str();
        if( !str )
        {
          logit("e","statmgr: No argument for <pagegroup> command in "
                "Descriptor <%s>; exiting.\n", descfile );
          exit( -1 );
        }
        arglen = strlen(str);
        if( arglen==0 || arglen>=MAXRECIPLEN )
        {
          logit("e","statmgr: Invalid length=%d for <pagegroup> argument in "
                "Descriptor <%s>; valid=1-%d; exiting.\n",
                 arglen, descfile, MAXRECIPLEN-1 );
          exit( -1 );
        }
        strcpy( desc[id].pagegroup[desc[id].npagegroup], str );
        desc[id].npagegroup++;
      }

      /* Read mail recipients - overrides mail commands in statmgr config
      *******************************************************************/
      else if( k_its( "mail" ) )  /* optional */
      {
        if( desc[id].nmail >= MAXRECIP )
        {
          logit("e", "statmgr: Too many <mail> commands in Descriptor <%s>;"
                " max=%d; exiting.\n", descfile, MAXRECIP );
          exit( -1 );
        }
        str = k_str();
        if( !str )
        {
          logit("e","statmgr: No argument for <mail> command in Descriptor <%s>; "
                "exiting.\n", descfile );
          exit( -1 );
        }
        arglen = strlen(str);
        if( arglen==0 || arglen>=MAXRECIPLEN )
        {
          logit("e","statmgr: Invalid length=%d for <mail> argument in "
                "Descriptor <%s>; valid=1-%d; exiting.\n",
                 arglen, descfile, MAXRECIPLEN-1 );
          exit( -1 );
        }
        strcpy( desc[id].mail[desc[id].nmail], str );
        desc[id].nmail++;
      }

      /* Does this module wish to be resurrected? alex 7/3/97
      ******************************************************/
      /* Show that module wants to be restarted but we can't until
       * we learn it's pid PNL, 1/23/99 */
      else if( k_its( "restartMe" ) ) desc[id].restart=UNKNOWNPID;  /* optional */

      /* Read error types from descriptor file (Not Required)
      ******************************************************/
      else if( !geterrtxt && k_its( "err:" ) )  /* optional */
      {
        if ( jerr >= MAXERR )
        {
          logit( "e",
                 "statmgr: too many error types in descfile <%s>",
                  descfile );
          logit( "e", "; MAXERR=%d; exiting.\n", MAXERR );
          exit( -1 );
        }
        str  = k_com();
        narg = sscanf( str,
                       "err: %hd nerr: %d tsec: %d page: %d mail: %d",
                       &desc[id].err[jerr].err,
                       &desc[id].err[jerr].nerr,
                       &desc[id].err[jerr].tsec,
                       &desc[id].err[jerr].page,
                       &desc[id].err[jerr].mail );
        if ( narg < 5 )
        {
          logit( "e",
                   "statmgr: error decoding error-handling line " );
          logit( "e", "from descfile <%s>; exiting.\n",
                   descfile );
          exit( -1 );
        }
        geterrtxt = 1;
      }

      /* Read text line that goes with previous error
      **********************************************/
      else if( geterrtxt )  /* optional */
      {
        if ( !k_its( "text:" ) )
        {
          logit( "e",
                   "statmgr: no text line for err: %hd in <%s>; exiting.\n",
                   desc[id].err[jerr].err, descfile );
          exit( -1 );
        }
        str = k_str();
        if(str) strcpy(desc[id].err[jerr].text, str);
        jerr++;
        desc[id].nerr = jerr;
        geterrtxt = 0;
      }

      else
      {
        logit( "e",
                 "statmgr: <%s> unknown command in descfile <%s>.\n",
                 com, descfile );
        continue;
      }

      /* See if there were any errors processing the command
      *****************************************************/
      if( k_err() )
      {
        logit( "e",
                 "statmgr: Bad <%s> command in descfile <%s>; exiting.\n",
                 com, descfile );
        exit( -1 );
      }
    }
    nfiles = k_close();
  }
    
  /* If no <system> command was given, use environment SYS_NAME
  *************************************************************/
  if ( !init[1] )
  {
    strcpy( desc[id].sysName, sysName );
    init[1] = 1;
  }
    
  dfileHeartbeat = -1;
  if (strcmp(descfile, "statmgr.desc") == 0 || strcmp(descfile, "startstop.desc") == 0) {
    dfileHeartbeat = -2;
  } else {
      dfileHeartbeat = statmgr_checkheartbeat(descfile, desc, nfopen);
  }
    
    
    /* create .d filename from .desc, mostly for logging here. */
    if (strlen(descfile)-3 > DFILENAME_LEN) {
        logit("e", "FATAL, buffer overflow, dfile name len > DFILENAME_LEN");
        exit(-1);
    }
    strncpy(dotdfname, descfile, strlen(descfile)-3); /* trim esc from end of .desc */
    dotdfname[strlen(descfile)-3] = 0;
    
  /* If no <tsec> command was given, use value from .d file * 1.5 or default
  *************************************************************/
  if ( !init[4])
  {
      
    /* Read the corresponding .d file to try to pull heartbeat interval. If
     * found and read, mul by 3/2 and use as the default. Any tsec value
     * in the .desc file will override this default.
     */
      if (dfileHeartbeat > 0) {
          desc[id].hbeat.tsec = dfileHeartbeat *3/2;
          logit("t", "Heartbeat tsec not in <%s> but found in <%s>, using %d *3/2 = %d\n", descfile, dotdfname, dfileHeartbeat, desc[id].hbeat.tsec );
      } else {
          desc[id].hbeat.tsec = DEFAULT_HEARTBEAT *3/2;
          logit("t", "Heartbeat tsec not in <%s> and can't find in <%s>, using default %d *3/2 = %d\n", descfile, dotdfname, DEFAULT_HEARTBEAT, desc[id].hbeat.tsec );
      }
    init[4] = 1;
  }
  if (dfileHeartbeat > 0 && dfileHeartbeat >= desc[id].hbeat.tsec) {
    logit( "e", "WARNING: Heartbeat in .d file <%s> (%d) is >= heartbeat (%d) in .desc file <%s>, this will cause statmgr to restart unnecessarily. Setting receive tsec to %d+5\n", dotdfname, dfileHeartbeat, desc[id].hbeat.tsec, descfile, desc[id].hbeat.tsec);
      desc[id].hbeat.tsec = desc[id].hbeat.tsec+5;
  }
  logit("t", "Heartbeat setting for %s: send: %d receive: %d\n", descfile, dfileHeartbeat, desc[id].hbeat.tsec);
  /* After the file is closed, check init flags for missed commands
  ****************************************************************/
  nmiss = 0;
  for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
  if ( nmiss )
  {
    logit( "e", "statmgr: ERROR, no " );
    if ( !init[0] )  logit( "e", "<modName> "   );
    if ( !init[1] )  logit( "e", "<system> "    );
    if ( !init[2] )  logit( "e", "<modId> "     );
    if ( !init[3] )  logit( "e", "<instId> "    );
    if ( !init[4] )  logit( "e", "<tsec:> "     );
    logit( "e", "command(s) in descfile <%s>; exiting.\n", descfile );
    exit( -1 );
  }
  return;
}


/***********************************************************************
 * statmgr_checkheartbeat()  processes .d file using kom.c functions    *
 *                  only looks for heartbeat interval                *
 *                  no file-nesting is allowed in descriptor files     *
 ***********************************************************************/
int statmgr_checkheartbeat(char *dfile, DESCRIPTOR *desc, int nfopen)
{
    char    *com;
    char     dfname[DFILENAME_LEN];   /* name of .d file */
    int      nfiles;
    int      heartbeat;
    FILE    *file;
    
    heartbeat = -1;
    /* Open the .d configuration file
     **********************************/
    strncpy(dfname, dfile, strlen(dfile)-3); /* trim esc from end of .desc */
    dfname[strlen(dfile)-3] = 0;
    if ((file = fopen(dfname, "r")) == NULL) {
        // File doesn't exist
        logit("e", ".d file <%s> for desc file <%s> doesn't exist", dfname, dfile);
        return -2;
    } else {
        fclose(file);
    }
    
    nfiles = k_open( dfname );
    if ( nfiles <= nfopen )
    {
        logit( "e",
              "statmgr: Error opening .d file <%s>; skipping.\n",
              dfname );
    }
    
    /* Process one descriptor file
     *****************************/
    while( nfiles > nfopen )  /* While there's a descriptor file open */
    {
    /* Process one descriptor file
     *****************************/
        while(k_rd())        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */
            
            /* Ignore blank lines & comments
             *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;
            
            /* Open a nested configuration file
             **********************************/
            
            if( com[0] == '@' )
            {
                int success = nfiles+1;
                nfiles  = k_open(&com[1]);
                if ( nfiles != success )
                {
                    logit( "e",
                          "statmgr: Error opening command file <%s>; exiting.\n",
                          &com[1] );
                    exit( -1 );
                }
                continue;
            }
            
            /* Process anything else looking for one the the 5 heartbeat strings
             * somebody really should have pick a default name! :(
             ************************************/
            if( k_its( "HeartBeatInt" )
               || k_its( "HeartbeatInt" )
               || k_its( "HeartbeatSecs" )
               || k_its( "HeartbeatInterval" )
               || k_its( "HeartBeatInterval" ) )
            {
                heartbeat = k_long();
            }
        }
        nfiles = k_close();
    }
    return heartbeat;
}

  /************************************************************
   *                    SendStatusRequest                     *
   * To send a message requesting the Earthworm system status *
   ************************************************************/
void SendStatusRequest( void )
{
   char message[16];
   MSG_LOGO rlogo;
   long     rlen;

/* Flush all old messages in the ring
   **********************************/
   logo.type   = TypeStatus;
   logo.mod    = 0;
   logo.instid = InstId;
   while( tport_getmsg( &region, &logo, 1,
                        &rlogo, &rlen, message, (long)16 ) != GET_NONE );

/* Build status request message
   ****************************/
   sprintf(message,"%d\n", MyModId );

/* Set logo values of message
   **************************/
   logo.type   = TypeReqStatus;
   logo.mod    = MyModId;
   logo.instid = InstId;

/* Send status request message to transport ring
   **************************************/
   if ( tport_putmsg( &region, &logo, strlen(message), message ) != PUT_OK )
      fprintf(stderr, "status: Error sending message to transport region.\n" );

   return;

}

  /************************************************************
   *                    AccumulateRings                       *
   *   Get the Earthworm status message from shared memory.   *
   *   Use it to get a list of rings.                         *
   *   We'd rather get ring list from here than from          *
   *   startstop*.d, since users can change that filename, or *
   *   change the contents of the file without restarting.    *
   ************************************************************/
int AccumulateRings( void )
{
   char     msg[MAX_BYTES_STATUS];
   MSG_LOGO rlogo;
   long     rlen;
   int      timeout=30;
   int      rc;
   int      i;
   char    *NextToken;
   int      ringcount = 0;

   SendStatusRequest();

   /* Set logo values of message
   **************************/
   logo.type   = TypeStatus;
   logo.mod    = 0;           /* zero is the wildcard */
   logo.instid = InstId;

   for(i=1; i<timeout; i++)
   {
      rc = tport_getmsg( &region, &logo, 1,
                         &rlogo, &rlen, msg, (long)MAX_BYTES_STATUS-1 );
      if( rc == GET_NONE   )  {
         sleep_ew( 1000 );
         continue;
      }
      if( rc == GET_TOOBIG ) {
         fprintf( stdout,
            "Earthworm is running, but the status msg is too big for me!\n" );
         return 0;
      }
      else {
         msg[rlen]='\0';  /* null terminate the message */
         NextToken = strtok(msg, " \n");
         /* we're parsing a string that looks like this:
             Ring  1 name/key/size:      WAVE_RING / 1000 / 1024 kb    */
         while (NextToken != NULL) {
            if (strcmp ("name/key/size:", NextToken) == 0){
                NextToken = strtok(NULL, " \n");
                sprintf (Ringset.ringName[ringcount], "%s", NextToken);
                NextToken = strtok(NULL, " \n");
                Ringset.ringKey[ringcount] = atol(strtok(NULL, " \n"));
                NextToken = strtok(NULL, " \n");
                Ringset.ringSize[ringcount] =  atoi(strtok(NULL, " \n"));
                fprintf( stdout, "%s %d %ld\n", Ringset.ringName[ringcount], Ringset.ringKey[ringcount], Ringset.ringSize[ringcount]  );
		logit("t", "Attaching to RING: %s %d %ld\n", Ringset.ringName[ringcount], Ringset.ringKey[ringcount], Ringset.ringSize[ringcount]  );
                /* attach now to all the shared memory regions */
                tport_attach ( &(Ringset.region[ringcount]), Ringset.ringKey[ringcount] );
                ringcount ++;
           }
            NextToken = strtok(NULL, " \n");
            Ringset.ringcount = ringcount;
         }
         return 1;
      }
   }

/* If you got here, you timed out
 ********************************/
   fprintf( stdout,
       "Earthworm may be hung; no response from startstop in %d seconds while trying to get status message back.\n",
        timeout );
   return 0;
}

