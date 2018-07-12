/*
 *   THIS FILE IS UNDER CVS - 
 *   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
 *
 *    $Id: $
 *
 *    Revision history:
 *    $Log$
 *
 */

/*
 * wftimeshifter.c
 *
 * Reads waveform data (compressed or uncompressed) from one 
 * transport ring and writes it to another ring, adding an
 * offset to starttime and endtime. Intended to accomodate
 * low-jitter, high-latency analog telemetry.
 *
 * Based on wftimefilter and ringdup
 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>
#include <trheadconv.h>

char *Wild          = "*";   /* wildcard string for SCNL        */
#define INCREMENT_SCNL 10    /* increment the limit of # scnl's */
#define NotWild(x)     strcmp((x),Wild)
#define NotMatch(x, y)     strcmp((x),(y))

/* Store offset for each SCNL
 **********************************************/
typedef struct {
	char     sta[TRACE2_STA_LEN];   /* sta code we're tracking */
	char     cmp[TRACE2_CHAN_LEN];  /* cmp code we're tracking */
	char     net[TRACE2_NET_LEN];   /* net code we're tracking */
	char     loc[TRACE2_LOC_LEN];   /* loc code we're tracking */
	double   offset;                /* time offset             */
} SCNLstruct;

/* Globals
 **********/
static SHM_INFO  InRegion;    /* shared memory region to use for input  */
static SHM_INFO  OutRegion;   /* shared memory region to use for output */
static pid_t	 MyPid;       /* Our process id is sent with heartbeat  */
static SCNLstruct *Offset;    /* list of SCNL's and offsets             */
int    Max_SCNL      = 0;     /* working limit on # scnl's to offset    */
int    nSCNL         = 0;     /* # of scnl's we're configured to offset */
static SCNLstruct  WfKey;     /* key for looking up SCNLs in Offset     */
int    nWild         = 0;     /* # of Wildcards used in config file     */
char   *Argv0;                /* pointer to executable name             */

/* Things to read or derive from configuration file
 ***************************************************/
static int     LogSwitch;              /* 0 if no logfile should be written      */
static long    HeartbeatInt;           /* seconds between heartbeats             */
static long    MaxMessageSize = MAX_TRACEBUF_SIZ; /* size (bytes) of largest msg */
static MSG_LOGO *GetLogo = NULL;       /* logo(s) to get from shared memory      */
static short   nLogo     = 0;          /* # logos we're configured to get        */

/* Things to look up in the earthworm.h tables with getutil.c functions
 ***********************************************************************/
static long          InRingKey;    /* key of transport ring for input     */
static long          OutRingKey;   /* key of transport ring for output    */
static unsigned char InstId;       /* local installation id               */
static unsigned char MyModId;      /* Module Id for this program          */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeTrace;
static unsigned char TypeTrace2;
static unsigned char TypeCompress;
static unsigned char TypeCompress2;

/* Error messages used by wftshifter
 ************************************/
#define  ERR_MISSGAP       0   /* sequence gap in transport ring         */
#define  ERR_MISSLAP       1   /* missed messages in transport ring      */
#define  ERR_TOOBIG        2   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       3   /* msg retreived; tracking limit exceeded */
#define  ERR_CHANHIST      4   /* couldn't add SCNL to channel history   */
static char Text[256];         /* string for log/error messages          */

/* Functions in this source file
 ********************************/
void       wftshifter_lookup( void );
void       wftshifter_config( char * );
void       wftshifter_logparm( void );
void       wftshifter_status( unsigned char, short, char * );
int        wftshifter_compare( const void *s1, const void *s2 );
SCNLstruct *wftshifter_findchan( TRACE2_HEADER *thd );



int main( int argc, char **argv )
{
	char         *msgbuf;           /* buffer for msgs from ring     */
	TracePacket   wf;               /* trace packet                  */
	time_t        timeNow;          /* current time                  */
	time_t        timeLastBeat;     /* time last heartbeat was sent  */
	long          recsize;          /* size of retrieved message     */
	MSG_LOGO      reclogo;          /* logo of retrieved message     */
	int           res;
	unsigned char seq;
	SCNLstruct    *tlist;

	Argv0 = argv[0];
	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
		return( 0 );
	}

	/* Initialize name of log-file & open it
	 ****************************************/
	logit_init( argv[1], 0, 1024, 1 );

	/* Look up important info from earthworm*d tables
	 ************************************************/
	wftshifter_lookup();

	/* Read the configuration file(s)
	 ********************************/
	wftshifter_config( argv[1] );

	/*  Set logit to LogSwitch read from configfile
	 **********************************************/
	logit_init( argv[1], 0, 1024, LogSwitch );
	logit( "" , "%s: Read command file <%s>\n", Argv0, argv[1] );
	wftshifter_logparm();

	/* Check for different in/out rings
	 **********************************/
	if( InRingKey==OutRingKey )
	{
		logit ("e", "%s: InRing and OutRing must be different;"
				" exiting!\n", Argv0);
		free( GetLogo );
		exit( -1 );
	}

	/* Get our own process ID for restart purposes
	 *********************************************/
	if( (MyPid = getpid()) == -1 )
	{
		logit ("e", "%s: Call to getpid failed. Exiting.\n", Argv0);
		free( GetLogo );
		exit( -1 );
	}

	/* Allocate the message input buffer
	 ***********************************/
	if ( !( msgbuf = (char *) malloc( (size_t)MaxMessageSize ) ) )
	{
		logit( "e", "%: failed to allocate %d bytes"
				" for message buffer; exiting!\n", Argv0, MaxMessageSize );
		free( GetLogo );
		exit( -1 );
	}

	/* Attach to shared memory rings
	 *******************************/
	tport_attach( &InRegion, InRingKey );
	logit( "", "%s: Attached to public memory region: %ld\n",
			Argv0, InRingKey );
	tport_attach( &OutRegion, OutRingKey );
	logit( "", "%s: Attached to public memory region: %ld\n",
			Argv0, OutRingKey );

	/* Force a heartbeat to be issued in first pass thru main loop
	 *************************************************************/
	timeLastBeat = time(&timeNow) - HeartbeatInt - 1;

	/* Flush the incoming transport ring on startup
	 **********************************************/
	while( tport_copyfrom( &InRegion, GetLogo, nLogo,  &reclogo,
			&recsize, msgbuf, MaxMessageSize, &seq ) != GET_NONE );

	/*----------------------- setup done; start main loop -------------------------*/
	while( tport_getflag( &InRegion ) != TERMINATE  &&
			tport_getflag( &InRegion ) != MyPid )
	{
		/* send q's heartbeat
		 *******************************/
		if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt )
		{
			timeLastBeat = timeNow;
			wftshifter_status( TypeHeartBeat, 0, "" );
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
			sleep_ew(50); /* milliseconds */
			continue;

		case GET_NOTRACK: /* got a msg, but can't tell if any were missed */
			sprintf( Text,
					"Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
					reclogo.instid, reclogo.mod, reclogo.type );
			wftshifter_status( TypeError, ERR_NOTRACK, Text );
			break;

		case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
			sprintf( Text,
					"Missed msg(s) from logo (i%u m%u t%u)",
					reclogo.instid, reclogo.mod, reclogo.type );
			wftshifter_status( TypeError, ERR_MISSLAP, Text );
			break;

		case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
			sprintf( Text,
					"Saw sequence# gap for logo (i%u m%u t%u s%u)",
					reclogo.instid, reclogo.mod, reclogo.type, seq );
			wftshifter_status( TypeError, ERR_MISSGAP, Text );
			break;

		case GET_TOOBIG:  /* next message was too big, resize buffer      */
			sprintf( Text,
					"Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%ld]",
					recsize, reclogo.instid, reclogo.mod, reclogo.type,
					MaxMessageSize );
			wftshifter_status( TypeError, ERR_TOOBIG, Text );
			continue;

		default:         /* Unknown result                                */
			sprintf( Text, "Unknown tport_copyfrom result:%d", res );
			wftshifter_status( TypeError, ERR_TOOBIG, Text );
			continue;
		}

		/* Prepare info to pass to the filter.
		 * Make working copy of header, SCNLize it, put in local byte order
		 ******************************************************************/
		memcpy( wf.msg, msgbuf, sizeof(TRACE2_HEADER) );
		if     ( reclogo.type == TypeTrace    ) TrHeadConv( &(wf.trh) );
		else if( reclogo.type == TypeCompress ) TrHeadConv( &(wf.trh) );

		/* See if this channel is in the list
		 ********************************************/
		tlist = wftshifter_findchan( &wf.trh2 );
		if(tlist)
		{
			if (tlist->offset != 0)
			{
				TRACE2_HEADER *wfh = &wf.trh2;
				wfh->starttime += tlist->offset;
				wfh->endtime += tlist->offset;
				memcpy( msgbuf, wf.msg, sizeof(TRACE2_HEADER) );
			}

			if( tport_putmsg( &OutRegion, &reclogo, recsize, msgbuf ) != PUT_OK )
			{
				logit("et","%s: Error writing %d-byte msg to ring; "
						"original logo (i%u m%u t%u)\n", Argv0, recsize,
						reclogo.instid, reclogo.mod, reclogo.type );
			}
		}
	}
	/*-----------------------------end of main loop-------------------------------*/

	/* free allocated memory */
	free( GetLogo );
	free( msgbuf  );
	free( Offset );

	/* detach from shared memory */
	tport_detach( &InRegion );
	tport_detach( &OutRegion );

	/* write a termination msg to log file */
	logit( "t", "%s: Termination requested; exiting!\n", Argv0 );
	fflush( stdout );
	return( 0 );
}


/******************************************************************************
 *  wftshifter_config() processes command file(s) using kom.c functions;       *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 7        /* # of required commands you expect to process   */
void wftshifter_config( char *configfile )
{
	char  init[ncommand];   /* init flags, one byte for each required command */
	int   nmiss;            /* number of required commands that were missed   */
	char *com;
	char *str;
	int   nfiles;
	int   success;
	int   i;
	size_t  size;

	/* Set to zero one init flag for each required command
	 *****************************************************/
	for( i=0; i<ncommand; i++ )  init[i] = 0;
	nLogo = 0;

	/* Open the main configuration file
	 **********************************/
	nfiles = k_open( configfile );
	if ( nfiles == 0 ) {
		logit( "e",
				"%s: Error opening command file <%s>; exiting!\n",
				Argv0, configfile );
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
						"%s: Error opening command file <%s>; exiting!\n",
						Argv0, &com[1] );
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
						"%s: Invalid <LogFile> value %d; "
						"must = 0, 1 or 2; exiting!\n", Argv0, LogSwitch );
					exit( -1 );
				}
				init[0] = 1;
			}

			/*1*/     else if( k_its("MyModuleId") ) {
				if( str=k_str() ) {
					if( GetModId( str, &MyModId ) != 0 ) {
						logit( "e",
							"%s: Invalid module name <%s> "
							"in <MyModuleId> command; exiting!\n", Argv0, str);
						exit( -1 );
					}
				}
				init[1] = 1;
			}

			/*2*/     else if( k_its("InRing") ) {
				if( str=k_str() ) {
					if( ( InRingKey = GetKey(str) ) == -1 ) {
						logit( "e",
							"%s: Invalid ring name <%s> "
							"in <InRing> command; exiting!\n", Argv0, str);
						exit( -1 );
					}
				}
				init[2] = 1;
			}

			/*3*/     else if( k_its("OutRing") ) {
				if( str=k_str() ) {
					if( ( OutRingKey = GetKey(str) ) == -1 ) {
						logit( "e",
							"%s: Invalid ring name <%s> "
							"in <OutRing> command; exiting!\n", Argv0, str);
						exit( -1 );
					}
				}
				init[3] = 1;
			}

			/*4*/     else if( k_its("HeartbeatInt") ) {
				HeartbeatInt = k_long();
				init[4] = 1;
			}

			/* Enter installation/module/msgtype to process
			 **********************************************/
			/*5*/     else if( k_its("GetLogo") ) {
				int       oktype = 0;
				MSG_LOGO *tlogo  = NULL;
				tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+1)*sizeof(MSG_LOGO) );
				if( tlogo == NULL )
				{
					logit( "e", "%s: GetLogo: error reallocing"
						" %d bytes; exiting!\n", Argv0,
						(nLogo+1)*sizeof(MSG_LOGO) );
					exit( -1 );
				}
				GetLogo = tlogo;

				if( str=k_str() ) {
					if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
						logit( "e",
							"%s: Invalid installation name <%s>"
							" in <GetLogo> cmd; exiting!\n", Argv0, str );
						exit( -1 );
					}
					if( str=k_str() ) {
						if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
							logit( "e",
								"wftimefilter: Invalid module name <%s>"
								" in <GetLogo> cmd; exiting!\n", str );
							exit( -1 );
						}

						if( str=k_str() ) {
							if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
								logit( "e",
									"wftimefilter: Invalid message type <%s>"
									" in <GetLogo> cmd; exiting!\n", str );
								exit( -1 );
							}
							if     ( GetLogo[nLogo].type == TypeTrace     ) oktype = 1;
							else if( GetLogo[nLogo].type == TypeTrace2    ) oktype = 1;
							else if( GetLogo[nLogo].type == TypeCompress  ) oktype = 1;
							else if( GetLogo[nLogo].type == TypeCompress2 ) oktype = 1;
							else                                            oktype = 0;
							if( !oktype ) {
								logit( "e",
									"wftimefilter: Cannot process message type <%s>;"
									" exiting!\n", str );
								exit( -1 );
							}
							nLogo++;
							init[5] = 1;
						} /* end type */
					} /* end modid */
				} /* end instid */
			}

			/*6*/     else if( k_its("MaxMessageSize") ) {
				MaxMessageSize = k_long();
				init[6] = 1;
			}
			else if( k_its("Offset_scnl") )
			{
				int iarg;

				if( nSCNL >= Max_SCNL )
				{
					Max_SCNL += INCREMENT_SCNL;
					size     = Max_SCNL * sizeof( SCNLstruct );
					Offset   = (SCNLstruct *) realloc( Offset, size );
					if( Offset == NULL )
					{
						logit( "e",
							"%s: Error allocating %d bytes"
							" for SCNL list; exiting!\n", Argv0, size );
						exit( -1 );
					}
				}

				for( iarg=1; iarg<=4; iarg++ )
				{
					str=k_str();
					if( !str ) break; /* no string; error!   */
					if( iarg==1 )  /* station code */
					{
						if( strlen(str)>(size_t)TRACE2_STA_LEN ) break;
						if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
						strcpy(Offset[nSCNL].sta,str);
						continue;
					}
					if( iarg==2 ) /* component code */
					{
						if( strlen(str)>(size_t)TRACE2_CHAN_LEN ) break;
						if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
						strcpy(Offset[nSCNL].cmp,str);
						continue;
					}
					if( iarg==3 ) /* network code */
					{
						if( strlen(str)>(size_t)TRACE2_NET_LEN ) break;
						if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
						strcpy(Offset[nSCNL].net,str);
						continue;
					}
					if( iarg==4 ) /* location code */
					{
						if( strlen(str)>(size_t)TRACE2_LOC_LEN ) break;
						if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
						strcpy(Offset[nSCNL].loc,str);
						continue;
					}
				}
				Offset[nSCNL].offset=k_val(); /* time offset */
				//Offset[nSCNL].offset=atof(k_str()); /* time offset */
				nSCNL++;
			}
			/* Unknown command
			 *****************/
			else {
				logit( "e", "%s: <%s> Unknown command in <%s>.\n",
					Argv0, com, configfile );
				continue;
			}


			/* See if there were any errors processing the command
			 *****************************************************/
			if( k_err() ) {
				logit( "e",
					"%s: Bad <%s> command in <%s>; exiting!\n",
					Argv0, com, configfile );
				exit( -1 );
			}
		}
		nfiles = k_close();
	}


	/* Don't bother to sort SCNLs if wild cards are present.
	   We'll have to do a linear search anyways. */
	if (nWild == 0)
		qsort( Offset, nSCNL, sizeof(SCNLstruct), wftshifter_compare );


	/* After all files are closed, check init flags for missed commands
	 ******************************************************************/
	nmiss = 0;
	for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;

	if ( nmiss ) {

		logit( "e", "%s: ERROR, no ", Argv0 );
		if ( !init[0] )  logit( "e", "<LogFile> "            );
		if ( !init[1] )  logit( "e", "<MyModuleId> "         );
		if ( !init[2] )  logit( "e", "<InRing> "             );
		if ( !init[3] )  logit( "e", "<OutRing> "            );
		if ( !init[4] )  logit( "e", "<HeartbeatInt> "       );
		if ( !init[5] )  logit( "e", "<GetLogo> "            );
		if ( !init[6] )  logit( "e", "<MaxMessageSize> "     );
		logit( "e", "command(s) in <%s>; exiting!\n", configfile );
		exit( -1 );
	}
	return;

}

/******************************************************************************
 *  wftshifter_logparm( )   Log operating params                                *
 ******************************************************************************/
void wftshifter_logparm( void )
{
	int i;
	logit("","MyModuleId:         %u\n",        MyModId );
	logit("","InRing key:         %ld\n",       InRingKey );
	logit("","OutRing key:        %ld\n",       OutRingKey );
	logit("","HeartbeatInt:       %ld sec\n",   HeartbeatInt );
	logit("","LogFile:            %d\n",        LogSwitch );
	logit("","MaxMessageSize:     %d bytes\n",  MaxMessageSize );
	for(i=0;i<nLogo;i++)logit("","GetLogo[%d]:         i%u m%u t%u\n", i,
			GetLogo[i].instid, GetLogo[i].mod, GetLogo[i].type );

	for(i=0;i<nSCNL;i++)  logit("","Offset[%d]:          %s %s %s %s %g\n", i,
			Offset[i].sta, Offset[i].cmp, Offset[i].net, Offset[i].loc, Offset[i].offset );

	return;
}


/******************************************************************************
 * wftshifter_status() builds a heartbeat or error message & puts it into      *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void wftshifter_status( unsigned char type, short ierr, char *note )
{
	MSG_LOGO    logo;
	char        msg[1024];
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
		logit( "et", "%s: %s\n", Argv0, note );
	}

	size = strlen( msg );   /* don't include the null byte in the message */

	/* Write the message to shared memory
	 ************************************/
	if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
	{
		if( type == TypeHeartBeat ) {
			logit("et","wftimefilter:  Error sending heartbeat.\n" );
		}
		else if( type == TypeError ) {
			logit("et","wftimefilter:  Error sending error:%d.\n", ierr );
		}
	}

	return;
}


/******************************************************************************
 *  wftshifter_lookup( )   Look up important info from earthworm tables        *
 ******************************************************************************/
void wftshifter_lookup( void )
{

	/* Look up installations of interest
	 *********************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
		logit( "e",
			"%s: error getting local installation id; exiting!\n", Argv0 );
		exit( -1 );
	}

	/* Look up message types of interest
	 *********************************/
	if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
		logit( "e",
			"%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
		exit( -1 );
	}
	if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
		logit( "e",
			"%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
		exit( -1 );
	}
	if ( GetType( "TYPE_TRACEBUF", &TypeTrace ) != 0 ) {
		logit( "e",
			"%s: Invalid message type <TYPE_TRACEBUF>; exiting!\n", Argv0 );
		exit( -1 );
	}
	if ( GetType( "TYPE_TRACEBUF2", &TypeTrace2 ) != 0 ) {
		logit( "e",
			"%s: Invalid message type <TYPE_TRACEBUF2>; exiting!\n", Argv0 );
		exit( -1 );
	}
	if ( GetType( "TYPE_TRACE_COMP_UA", &TypeCompress ) != 0 ) {
		logit( "e",
			"%s: Invalid message type <TYPE_TRACE_COMP_UA>; exiting!\n", Argv0 );
		exit( -1 );
	}
	if ( GetType( "TYPE_TRACE2_COMP_UA", &TypeCompress2 ) != 0 ) {
		logit( "e",
			"%s: Invalid message type <TYPE_TRACE2_COMP_UA>; exiting!\n", Argv0 );
		exit( -1 );
	}
	return;
}

/******************************************************************************
 *  wftshifter_findchan()  find channel in list of SCNLs we're tracking        *
 ******************************************************************************/
SCNLstruct *wftshifter_findchan( TRACE2_HEADER *thd )
{
	SCNLstruct *wf = NULL;

	if( nSCNL ) {
		strcpy( WfKey.sta,  thd->sta  );
		strcpy( WfKey.cmp,  thd->chan );
		strcpy( WfKey.net,  thd->net  );
		strcpy( WfKey.loc,  thd->loc  );

		/* Use binary search if no wildcards in SCNL list. */
		if( nWild==0 )
			wf = (SCNLstruct *)bsearch( &WfKey, Offset, nSCNL, sizeof(SCNLstruct),
					wftshifter_compare );

		/* Use linear search if wildcards were used */
		else
		{
			int i;
			for( i=0; i<nSCNL; i++ )
			{
				SCNLstruct *next = &(Offset[i]);
				if( NotWild(next->sta)  && NotMatch(next->sta, WfKey.sta)  ) continue;
				if( NotWild(next->cmp) && NotMatch(next->cmp,WfKey.cmp) ) continue;
				if( NotWild(next->net)  && NotMatch(next->net, WfKey.net)  ) continue;
				if( NotWild(next->loc)  && NotMatch(next->loc, WfKey.loc)  ) continue;
				wf = next;  /* found a match! */
				break;
			}
		}
	}

	return( wf );
}

/******************************************************************************
 *  wftshifter_compare()  This function is passed to qsort() * bsearch() so    *
 *     we can sort the channel list by sta, net, component & location codes,  *
 *     and then look up a channel efficiently in the list.                    *
 ******************************************************************************/
int wftshifter_compare( const void *s1, const void *s2 )
{
	int rc;
	SCNLstruct *t1 = (SCNLstruct *) s1;
	SCNLstruct *t2 = (SCNLstruct *) s2;

	rc = NotMatch( t1->sta, t2->sta );
	if( rc != 0 ) return rc;

	rc = NotMatch( t1->net, t2->net );
	if( rc != 0 ) return rc;

	rc = NotMatch( t1->cmp, t2->cmp );
	if( rc != 0 ) return rc;

	rc = NotMatch( t1->loc, t2->loc );
	if( rc != 0 ) return rc;

	return 0;
}
