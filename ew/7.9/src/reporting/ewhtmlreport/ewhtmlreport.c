
/* 0.0.26 2014-09-30 - made report interval a float */
/* 0.0.27 2014-11-21 - added frac sec to origin time in CSV output*/
/* 0.0.28 2015-01-07 - saved memory for large catalogs of 50K quakes by freeing mag aux struct */
/* 0.0.29 2015-11-06 - discovered and fixed bug in site listing in ew2json code" */

#define VERSION_STR "0.0.29.1 - 2015-11-06"



/* Includes
 **********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <earthworm.h>
#include <transport.h>
#include <kom.h>
#include <read_arc.h>
#include <rw_mag.h>
#include <site.h>
#include <chron3.h>
#ifndef _WINNT
#include <unistd.h>
#endif

#include "ewjson.h"
#include "arc2csv.h"
//#include "webdata.h"


/* Defines
 *********/
#define MAX_STATIONS 200
#define MAX_ADDRESS 80
#define MAX_EMAIL_RECIPIENTS 20
#define MAX_EMAIL_CHAR 160
#define DEFAULT_SUBJECT_PREFIX "EWreport"
#define MAX_FILENAME	2024					/* Maximum size of filename */
#define MAX_MAG_CHANS			1000
#define MAX_STRING_SIZE 1024  /* used for many static string place holders */

#define MIN_QUALITY_FLAG 'D'

#define DEFAULT_HTMLBASE "./ewhtmlreport.html"






/* Functions in this file
 ************************/
void PrintErrorMsg( void );
void config(char*);
void lookup(void);
void status( unsigned char, short, char* );
int PrintHTMLReport( FILE*, FILE*, double );
int ScanArcFolder( FILE*, FILE*, time_t, time_t );
void sendEmailToAll(char *, char *, time_t , time_t , int );
void MyCopyFile(char *from, char *to);



/* Externals - Removed to allow compilation without objcopy
 ***********/
/*
extern char _binary_ewhtmlreport_html_start;
extern char _binary_ewhtmlreport_html_end;
extern char _binary_ewhtmlreport_js_start;
extern char _binary_ewhtmlreport_js_end;
extern char _binary_dygraph_combined_js_start;
extern char _binary_dygraph_combined_js_end;
*/
		

/* Globals
 *********/
static SHM_INFO		InRegion;		/* shared memory region to use for input */
static pid_t		MyPid;			/* Our process id is sent with heartbeat */
int					Standalone = 0;	/* Control standalone mode */
int					jsonOnly = 0;	/* Output only JSON string */
int					UseML = 0;		/* Use ML */

typedef struct {
   char address[MAX_EMAIL_CHAR];
   double min_magnitude;
} EMAILREC;


/* Things to read or derive from configuration file
 **************************************************/
static int		LogSwitch;				/* 0 if no logfile should be written */
static long		HeartbeatInt;           /* seconds between heartbeats */       
static long		MaxMessageSize = 100000;  /* size (bytes) of largest msg */
static int		Debug = 0;              /* 0=no debug msgs, non-zero=debug */
static char		CSV_Filename[MAX_FILENAME];	/* output csv file to this basename*/
static char		HTMLFile[MAX_FILENAME];	/* Base name of the out files */
static char		ArcFolder[MAX_FILENAME];/* Folder for arc files */
static char		MagFolder[MAX_FILENAME];/* Folder for mag files */
static double		ReportInt = 1.0;			/* Interval between reports in days */
static time_t	ReportTime;				/* Time when reports are sent */
static long		ReportPeriod = 60;		/* Time interval considered for the report in days */
static int		MaxDist = 10;			/* Max. distance considered for station average */
static char		HTMLBaseFile[MAX_FILENAME];
static int        UseBlat = 0;        // use blat syntax as the mail program instead of a UNIX like style
static int        WriteIndex = 0;        // write an index.html copy of the report too
static char  	  IndexFile[MAX_FILENAME];	/* name of the index file, full path*/
static char       BlatOptions[MAX_STRING_SIZE];      // apply any options needed after the file....-server -p profile etc...
static int        nemailrecipients = 0;   // Number of email recipients
// Array of email recipients
static EMAILREC   emailrecipients[MAX_EMAIL_RECIPIENTS];
static char       EmailProgram[MAX_STRING_SIZE];      // Path to the email program
static char       SubjectPrefix[MAX_STRING_SIZE];     // defaults to EWalert, settable now
static char          MinQuality = MIN_QUALITY_FLAG;	// minimum quality flag to allow into report



/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long               InRingKey;       // key of transport ring for input
static unsigned char      InstId;          // local installation id   
static unsigned char      MyModId;         // Module Id for this program 
static unsigned char      TypeHeartBeat;
static unsigned char      TypeError;

/* for Heartbeat work */
static time_t		timeNow;			/* current time */           
static time_t		timeLastBeat;		/* time last heartbeat was sent */

#define MAX_QUAKES 	100	// should make this configurable at some point

/* Main program starts here
 **************************/
int main(int argc, char **argv) 
{
	
	int	argcounter = 0;
	char		outfilename[MAX_FILENAME];
	char		csv_outfilename[MAX_FILENAME];
	FILE		*output;			/* Output file  for HTML */
	FILE		*csv_output = NULL;			/* Output file for CSV */
	int		i;					/* Generic counter */
   	int 		num_arcs = 0;	/* number of events processed for time period */
	
	CSV_Filename[0]=0;		/* no CSV by default */

        strcpy(HTMLBaseFile, DEFAULT_HTMLBASE);	/* set the default */

	/* Check command line arguments
	 ******************************/
	if( argc < 2 ) 
	{
		PrintErrorMsg();
	}
	else if( argc > 2 )
	{
		// Standalone mode
		Standalone = 1;
		output = stdout;
		
		/* Process input arguments
		 *************************/
		#define MINARG 3 // Number of mandatory arguments 
		for( i = 1; i < argc; i++ )
		{
			if( strstr( argv[i], "-j" ) != NULL )
			{
				jsonOnly = 1;
			}
			else if( strstr( argv[i], "-a" ) != NULL )
			{
				if( argc < i + 1 ) PrintErrorMsg();
				strcpy( ArcFolder, argv[++i] );
				argcounter++;
			}
			else if( strstr( argv[i], "-c" ) != NULL )
			{
				if( argc < i + 1 ) PrintErrorMsg();
				strcpy( CSV_Filename, argv[++i] );
				argcounter++;
			}
			else if( strstr( argv[i], "-m" ) != NULL )
			{
				if( argc < i + 1 ) PrintErrorMsg();
				strcpy( MagFolder, argv[++i] );
				UseML = 1;
			}
			else if( strstr( argv[i], "-q" ) != NULL )
			{
				if( argc < i + 1 ) PrintErrorMsg();
				MinQuality=argv[++i][0];
 			}
			else if( strstr( argv[i], "-h" ) != NULL )
			{
				if( argc < i + 1 ) PrintErrorMsg();
				strcpy( HTMLBaseFile, argv[++i] );
				//UseML = 1; //RSL This was wrong!
			}
			else if( strstr( argv[i], "-s" ) != NULL )
			{
				if( argc < i + 1 ) PrintErrorMsg();
				site_read( argv[++i] );
				argcounter++;
			}
			else if( strstr( argv[i], "-d" ) != NULL )
			{
				Debug = 1;
				argcounter++;
			}
			else if( strstr( argv[i], "-i" ) != NULL )
			{
				if( argc < i + 1 ) PrintErrorMsg();
				ReportPeriod = ( long ) atoi( argv[++i] );
				argcounter++;
			}
		}
		if( argcounter < MINARG )
		{
			fprintf( stderr, "Not enough input arguments\n" );
			PrintErrorMsg();
		}
	}
	else
	{
		/* Initialize name of log-file & open it
		***************************************/
		logit_init( argv[1], 0, MaxMessageSize, 1 );
   

		/* Read the configuration file(s)
		 ********************************/
		config( argv[1] );
   

		/* Lookup important information from earthworm.d
		 ***********************************************/
		lookup();
   

		/* Set logit to LogSwitch read from configfile
		 *********************************************/
		logit_init( argv[1], 0, MaxMessageSize, LogSwitch );
		logit( "", "ewhtmlreport: Read command file <%s>\n", argv[1] );
		logit( "", "ewhtmlreport: Version %s\n", VERSION_STR );


		/* Get our own process ID for restart purposes
		 *********************************************/
		if( ( MyPid = getpid()) == -1 )
		{
			logit ( "et", "ewhtmlreport: Call to getpid failed. Exiting.\n" );
			exit( -1 );
		}


		/* Attach to shared memory rings
		 *******************************/
		tport_attach( &InRegion, InRingKey );
		logit( "", "ewhtmlreport: Attached to public memory region: %ld\n",
				InRingKey );
      

		/* Force a heartbeat to be issued in first pass thru main loop
		 *************************************************************/
		timeLastBeat = time( &timeNow ) - HeartbeatInt - 1;
	}


	/*-------------------- setup done; start main loop -----------------------*/

	if( !Standalone )
	{
		struct tm 	*timeinfo;
		char		TimeStr[80];
		time_t		rt;
		time_t		start;
				
		while( tport_getflag( &InRegion ) != TERMINATE  &&
				tport_getflag( &InRegion ) != MyPid ) 
		{
			/* send heartbeat
			 ***************************/
			if( HeartbeatInt  &&  time( &timeNow ) - timeLastBeat >= HeartbeatInt ) 
			{
				timeLastBeat = timeNow;
				status( TypeHeartBeat, 0, "" );
			}
			
			/* check report time
			 *******************/
			if( timeNow >= ReportTime )
			{
				
				
				/* Produce output filename
				 *************************/
				rt = ( time_t ) ReportTime;
				timeinfo = gmtime( &rt );
				strftime( TimeStr, 80, "%Y%m%d%H%M%S", timeinfo );
				sprintf( outfilename, "%s%s.html", HTMLFile, TimeStr );
				
				/* Open output file
				 ******************/
				output = fopen( outfilename, "w" );
				if( output == NULL )
				{
					logit( "et", "ewhtmlreport: Unable to open output file - %s\n",
							outfilename );
					ReportTime += ( time_t )( ReportInt * 24 * 3600 );
					sleep_ew(1000);
					continue;
				}
				output = fopen( outfilename, "w" );

				/* Open optional CSV file for data if specified
				 ******************/
				if (CSV_Filename[0] != 0) 
				{
					sprintf( csv_outfilename, "%s%s.csv", CSV_Filename, TimeStr );
					csv_output = fopen( csv_outfilename, "w" );
					if( csv_output == NULL )
					{
						logit( "et", "ewhtmlreport: Unable to open csv output file - %s\n",
							csv_outfilename );
					}
					else
					{
						logit( "t", "ewhtmlreport: writing csv output file - %s\n",
							csv_outfilename );
						csv_header(csv_output);
					}
				}
		
		
				if (Debug) {
				   logit( "t", "ewhtmlreport: Debug: starting PrintHTMLReport(ReportTime=%ld)", (long) ReportTime);
				}
				/* Create html report
				 ********************/
				num_arcs = PrintHTMLReport( output, csv_output, ReportTime );
				if (num_arcs == -1) {
					exit(1);
				}
				
				
				/* Close output file
				 *******************/
				fclose( output );
				if (csv_output != NULL) fclose(csv_output);
				if (WriteIndex) MyCopyFile(outfilename, IndexFile);
				
			
				/* Send html report to email recipients
				 **************************************/
				start = ReportTime-( time_t )( ReportPeriod * 24 * 3600 );
				sendEmailToAll(outfilename, csv_outfilename, start , ReportTime, num_arcs);	
			
				/* Update report time
				 ********************/
				ReportTime += ( time_t )( ReportInt * 24 * 3600 );
			}
		
			/* Wait for next time
			 ********************/
			sleep_ew(1000); /* milliseconds */

		}

		/* free allocated memory */

		/* detach from shared memory */
		tport_detach( &InRegion );

		/* write a termination msg to log file */
		logit( "t", "ewhtmlreport: Termination requested; exiting!\n" );
		fflush( stdout );
	}
	else
	{
		/* Standalone Mode
		 *****************/
		time(&ReportTime); logit_init( "ewhtmlreport.d", 0, MaxMessageSize, 0 );
		if (CSV_Filename[0] != 0) csv_output = fopen( CSV_Filename, "w" );
		if (csv_output != NULL) csv_header(csv_output);
		num_arcs = PrintHTMLReport( output, csv_output, ReportTime );
		if (num_arcs == -1) {
			exit(1);
		}
		fflush( stdout );
		if (csv_output != NULL) fclose( csv_output );
	}
	exit( 0 );
}

void MyCopyFile(char *from, char *to)
{
	FILE *Ffrom, *Fto;
 	char Cbuf[1024];
 	int nread;

	if ((Ffrom = fopen(from, "r")) == NULL) {
		logit( "t", "ewhtmlreport: CopyFile could not open %s for reading!\n", from );
		return;
	}
	if ((Fto = fopen(to, "w")) == NULL) {
		logit( "t", "ewhtmlreport: CopyFile could not open %s for writing!\n", to );
		return;
	}
	while ( (nread = fread(Cbuf, 1, 1024, Ffrom)) != 0 ) {
		fwrite(Cbuf, 1, nread, Fto);
	}
	fclose(Fto);
	fclose(Ffrom);
}

void PrintErrorMsg( void )
{
	fprintf( stderr, "Version: %s\n", VERSION_STR );
	fprintf( stderr, "Usage as EW module:\n"
			"   ewhtmlreport <configfile>\n\n" );
	fprintf( stderr, 
			"Usage as standalone mode:\n"
			"   ewhtmlreport [options] > htmlfile\n"
			"Options:\n" 
			"   -c <file>   : Specify a CSV filename for output of event data \n"
			"   -d          : turn on Debugging messages (same as Debug in config) \n"
			"   -a <folder> : Specify folder with arc messages - Mandatory\n"
			"   -m <folder> : Specify folder with mag messages - Optional\n"
			"   -s <file>   : Specify sites file - Mandatory\n"
			"   -h <file>   : HTML template - default is ewhtmlreport.html \n"
			"   -q <Q>      : Minimum Quality to include in report (D is default, causing ALL events to be included) \n"
			"   -i <number> : Number of days for the report interval - Mandatory\n\n");
	exit( EW_FAILURE );
}








/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;                *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
#define ncommand 7        /* # of required commands you expect to process */
void config( char *configfile ) 
{
	char init[ncommand]; /* init flags, one byte for each required command */
	int nmiss; /* number of required commands that were missed   */
	char *com;
	char *str;
	int nfiles;
	int success;
	char processor[15];
	int i;
	strcpy(SubjectPrefix, DEFAULT_SUBJECT_PREFIX);

	/* Set to zero one init flag for each required command
	 *****************************************************/
	for( i = 0; i < ncommand; i++ ) init[i] = 0;

	/* Open the main configuration file
	 **********************************/
	nfiles = k_open( configfile );
	if( nfiles == 0 ) 
	{
		if( !Standalone ) 
			logit("e",
				"ewhtmlreport: Error opening command file <%s>; exiting!\n",
				configfile);
		else
			fprintf( stderr, "ewhtmlreport: Error opening command file <%s>; exiting!\n",
				configfile );
		exit(-1);
	}


	/* Process all command files
	 ***************************/
	while( nfiles > 0 ) /* While there are command files open */ 
	{
		while( k_rd())  /* Read next line from active file  */ 
		{
			com = k_str(); /* Get the first token from line */


			/* Ignore blank lines & comments
			 *******************************/
			if( !com ) continue;
			if( com[0] == '#' ) continue;


			/* Open a nested configuration file
			 **********************************/
			if( com[0] == '@' )
			{
				success = nfiles + 1;
				nfiles = k_open( &com[1] );
				if( nfiles != success ) 
				{
					if( !Standalone ) 
						logit("e",
							"ewhtmlreport: Error opening command file <%s>; exiting!\n",
							&com[1]);
					else
						fprintf( stderr, "ewhtmlreport: Error opening command file <%s>; exiting!\n",
							&com[1] );
					exit( -1 );
				}
				continue;
			}
			
			
			/* Process anything else as a command
			 ************************************/
			 
			/*0*/ 
			if( k_its( "LogFile" ) ) 
			{
				LogSwitch = k_int();
				if( LogSwitch < 0 || LogSwitch > 2 ) 
				{
					logit( "e",
							"ewhtmlreport: Invalid <LogFile> value %d; "
							"must = 0, 1 or 2; exiting!\n", LogSwitch );
					exit( -1 );
				}
				init[0] = 1;
			}
			
			/*1*/
			else if ( k_its( "MyModuleId" ) )
			{
				if( (str = k_str()) != NULL ) 
				{
					if( GetModId(str, &MyModId) != 0 ) 
					{
						logit( "e",
								"ewhtmlreport: Invalid module name <%s> "
								"in <MyModuleId> command; exiting!\n", str);
						exit( -1 );
					}
				}
				init[1] = 1;
			}
			/*2*/
			else if( k_its( "InRing" ) ) 
			{
				if( (str = k_str()) != NULL ) 
				{
					if( ( InRingKey = GetKey( str ) ) == -1 ) 
					{
						logit( "e",
								"ewhtmlreport: Invalid ring name <%s> "
								"in <InRing> command; exiting!\n", str );
						exit(-1);
					}
				}
				init[2] = 1;
			}
			
			/*3*/
			else if( k_its( "HeartbeatInt" ) )
			{
				HeartbeatInt = k_long();
				init[3] = 1;
			}
			
			else if( k_its( "Debug" ) ) 
			{
				Debug = k_int();
			}
			
			else if( k_its( "MaxMessageSize" ) ) 
			{
				MaxMessageSize = k_long();
			}
			
			/*4*/
			else if( k_its( "HTMLFile" ) ) 
			{
				strcpy( HTMLFile, k_str() );
				init[4] = 1;
			}
			
			/*5*/
			else if( k_its( "ArcFolder" ) ) 
			{
				strcpy( ArcFolder, k_str() );
				init[5] = 1;
			}
			
			/*6*/
			else if( k_its( "HTMLBaseFile" ) ) 
			{
				strcpy( HTMLBaseFile, k_str() );
				init[6] = 1;
			}
			
			else if( k_its( "CSVBasename" ) ) 
			{
				strcpy( CSV_Filename, k_str() );
			}
			else if( k_its( "MagFolder" ) ) 
			{
				UseML = 1;
				strcpy( MagFolder, k_str() );
			}
			
			else if( k_its( "ReportInt" ) ) 
			{
				ReportInt = k_val();
			}
			
			else if( k_its( "ReportPeriod" ) ) 
			{
				ReportPeriod = k_long();
			}
			
			else if( k_its( "MaxDist" ) ) 
			{
				MaxDist = k_int();
			}
			
			else if( k_its( "ReportTime" ) ) 
			{
				long reptime = -1;
				struct tm 	*timeinfo;
				
				reptime = k_long();
				time( &ReportTime );
				timeinfo = gmtime( &ReportTime );
				
				/* Set current time as report time */
				if( reptime != -1 )
				{
					timeinfo->tm_hour = reptime;
					timeinfo->tm_min = 0;
					timeinfo->tm_sec = 0;
					ReportTime = timegm( timeinfo );
				}			
				
				if( Debug )
				{
					char timestr[80];
					timeinfo = gmtime( &ReportTime );
					strftime( timestr, 80, "%H:%M:%S", timeinfo );
					logit( "o", "Report time set to: %s\n", timestr);
				}
			}
			
			else if ( k_its("UseBlat") )
			{
				UseBlat = 1;
			}
			else if ( k_its("WriteIndex") )
			{
				WriteIndex = 1;
				strcpy( IndexFile, k_str() );
			}
			else if ( k_its("MinQuality") )
			{
            			str = k_str();
            			MinQuality = str[0];
			}
			else if ( k_its("BlatOptions") )
			{
				strcpy(BlatOptions, k_str());
			}
			else if ( k_its("EmailRecipient") )
			{
				if (nemailrecipients<MAX_EMAIL_RECIPIENTS)
				{
					strcpy(emailrecipients[nemailrecipients].address, k_str());
					emailrecipients[nemailrecipients].min_magnitude = -9.0;
					nemailrecipients++;
				}
				else
      	{
					logit("e", "ewhtmlemail: Excessive number of email recipients. Exiting.\n");
					exit(-1);
				}
			}
			else if ( k_its("EmailProgram") )
			{
				strcpy(EmailProgram, k_str());
			}
			else if ( k_its("SubjectPrefix") )
			{
				strcpy(SubjectPrefix, k_str());
			}
			/* Site command to get sites file */
			else if( site_com() ) 
				strcpy( processor, "site_com" );
			/* Unknown command
			 *****************/
			else 
			{
				logit( "e", "ewhtmlreport: Fatal Error: <%s> Unknown command in <%s>.\n",
						com, configfile);
				exit(-1);
			}

			/* See if there were any errors processing the command
			 *****************************************************/
			if (k_err())
			{
				logit( "e",
						"ewhtmlreport: Bad <%s> command in <%s>; exiting!\n",
						com, configfile );
				exit( -1 );
			}
		}
		nfiles = k_close();
	}

	/* After all files are closed, check init flags for missed commands
	 ******************************************************************/
	nmiss = 0;
	for(i = 0; i < ncommand; i++ ) 
		if( !init[i] ) nmiss++;
	if( nmiss ) 
	{
		logit( "e", "ewhtmlreport: ERROR, no " );
		if( !init[0] ) logit( "e", "<LogFile> " );
		if( !init[1] ) logit( "e", "<MyModuleId> " );
		if( !init[2] ) logit( "e", "<InRing> " );
		if( !init[3] ) logit( "e", "<HeartbeatInt> " );
		if( !init[4] ) logit( "e", "<HTMLFile> " );
		if( !init[5] ) logit( "e", "<ArcFolder> " );
		if( !init[6] ) logit( "e", "<HTMLBaseFile> " );
		logit( "e", "command(s) in <%s>; exiting!\n", configfile );
		exit( -1 );
	}
	return;
}


/*********************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 *********************************************************************/
void lookup(void) {
    /* Look up installations of interest
     *********************************/
    if (GetLocalInst(&InstId) != 0) {
        logit("e",
                "ewhtmlreport: error getting local installation id; exiting!\n");
        exit(-1);
    }

    /* Look up message types of interest
     *********************************/
    if (GetType("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) {
        logit("e",
                "ewhtmlreport: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_ERROR", &TypeError) != 0) {
        logit("e",
                "ewhtmlreport: Invalid message type <TYPE_ERROR>; exiting!\n");
        exit(-1);
    }
    return;
}

/******************************************************************************
 * status() builds a heartbeat or error message & puts it into                *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void status(unsigned char type, short ierr, char *note) {
    MSG_LOGO logo;
    char msg[256];
    long size;
    time_t t;

    /* Build the message
     *******************/
    logo.instid = InstId;
    logo.mod = MyModId;
    logo.type = type;

    time(&t);

    if (type == TypeHeartBeat) {
        sprintf(msg, "%ld %ld\n", (long) t, (long) MyPid);
    } else if (type == TypeError) {
        sprintf(msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit("et", "ewhtmlreport: %s\n", note);
    }

    size = strlen(msg); /* don't include the null byte in the message */

    /* Write the message to shared memory
     ************************************/
    if (tport_putmsg(&InRegion, &logo, size, msg) != PUT_OK) {
        if (type == TypeHeartBeat) {
            logit("et", "ewhtmlreport:  Error sending heartbeat.\n");
        } else if (type == TypeError) {
            logit("et", "ewhtmlreport:  Error sending error:%d.\n", ierr);
        }
    }

    return;
}




/******************************************************************************
 * Prints an HTML report to output                    (and optional CSV)      *
 ******************************************************************************/
/* now returns number of ARC messages processed */
int PrintHTMLReport( FILE *output, FILE *csv_output, double ReportTime )
{
	//char *p;
	FILE	*baseHTML;
	int	num_arcs,c, i = 0;
	int     return_value;
	
	/* Copy base html file to output
	 *******************************/
        if (Debug) logit( "et", "ewhtmlreport: Debug: PrintHTMLReport() opening html base file - %s\n", HTMLBaseFile );
	baseHTML = fopen( HTMLBaseFile, "r" );
	if( baseHTML == NULL )
	{
		logit( "et", "ewhtmlreport: Fatal Error: Unable to load html base file - %s\n", HTMLBaseFile );
		return(-1);
	}
        if (Debug) logit( "et", "ewhtmlreport: Debug: PrintHTMLReport() html base file - opened successfully\n");
	while( ( c = fgetc( baseHTML ) ) != EOF )
	{
		return_value = fputc( ( char ) c, output );
 		if (return_value == EOF) {
			logit("et", "ewhtmlreport: Fatal Error writing to output html file\n");
			return(-1);
		}
		i++;
		if( i % 1000 == 0 ) { 
			fflush( output );
			i = 0;
		}
	}
	fclose( baseHTML );
	fflush( output );
        if (Debug) logit( "et", "ewhtmlreport: Debug: PrintHTMLReport() finished parsing html base file - %s\n", HTMLBaseFile );
	
	/* Send Version Number */
	fprintf( output, "<div class=\"eventsTable\">This report is generated by "
			"<a style=\"cursor:pointer;text-decoration:underline\" href=\"http://www.earthwormcentral.org\">Earthworm</a> "
			"and the program ewhtmlreport (version %s)</div>", VERSION_STR );
	fflush( output );
	
	/* Print HTML
	 ************/
	/* Alternative using objcopy
	p = &_binary_ewhtmlreport_html_start;
	while( p != &_binary_ewhtmlreport_html_end ) fputc( *p++, output );
	*/
	
	/* Print Javascript
	 ******************/
	/* Alternative using objcopy
	fprintf( output, "<script type=\"text/javascript\">\n" );
	p = &_binary_ewhtmlreport_js_start;
	while( p != &_binary_ewhtmlreport_js_end ) fputc( *p++, output );
	fprintf( output, "\n</script>\n" );
	
	fprintf( output, "<script type=\"text/javascript\">\n" );
	p = &_binary_dygraph_combined_js_start;
	while( p != &_binary_dygraph_combined_js_end ) fputc( *p++, output );
	fprintf( output, "\n</script>\n" );
	*/
	
	/* Print event data
	 ******************/
	fprintf( output, "<script type=\"text/javascript\">\n" );
	fprintf( output, "refdate = %f;\n", ReportTime * 1000.0 );
	fprintf( output, "repint = %ld;\n", ReportPeriod );
	fprintf( output, "maxDist = %d;\n", MaxDist );
	fprintf( output, "events = \n" );
	fflush( output );
	
        if (Debug) logit( "et", "ewhtmlreport: Debug: header writing complete; entering ScanArcFolder()\n");
	num_arcs = ScanArcFolder( output, csv_output, ReportTime - ( time_t )( ReportPeriod * 24 * 3600 ),
			ReportTime );
	fflush( output );
	fprintf( output, "stations = \n" );
	if (site2json( output ) == -1) {
		logit( "et", "ewhtmlreport: Fatal out of memory error from site2json, exiting\n");
		exit(1);
	}
	fprintf( output, "\n</script>\n" );
	fflush( output );
        if (Debug) {
		logit( "et", "ewhtmlreport: Debug base file written - %s\n", HTMLBaseFile );
	}
        return(num_arcs);
}






/******************************************************************************
 * ScanArcFolder : Scans a folder of arc files and converts messages to arc   *
 *		structures to be then converted to json.                              *
 *		returns 0 on success, or -1 on any failure, it exits on any fatal failure
 ******************************************************************************/
int ScanArcFolder( FILE *output, FILE *csv_output, time_t MinTime, time_t MaxTime )
{
	int			nfiles;		/* return from OpenDir */
	FILE			*fptr;		/* Pointer to file */
	int			f;		/* Generic counters */
	char			filename[MAX_FILENAME];
	char			base_filename[MAX_FILENAME];
	char			*fbuffer;		/* File read buffer */
	char			quality_char;
	size_t			nfbuffer;		/* Read bytes */
	HypoArc			arcmsg;			/* ARC message */
	MAG_CHAN_INFO *pMagchan;
	MAG_INFO **temp;
	MAG_INFO		*mag = NULL;	/* MAG message */
	MAG_INFO		**mags = NULL;	/* Array of MAG messages */
	int				nmags = 0;		/* Number of stored MAG messages */
	int				num_arcs = 0;		/* Number of processed ARC messages */
	time_t			origintime;		/* Origin time for current message */
	int				isFirst = 1;	/* To separate ',' in output string */

	
	
	/* Allocate memory for file reading buffer
	 *****************************************/
	fbuffer = ( char* ) calloc( 1, sizeof( char ) * MaxMessageSize );
	if( fbuffer == NULL )
	{
		if( !Standalone ) logit( "et", "ewhtmlreport: Unable to allocate memory for arc file "
				"reading buffer.\n" );
		return -1;
	}

	/* Create array of magnitude messages
	 ************************************/
	if( UseML )
	{
	
		/* Read magnitude files
		 **********************/
		nfiles = OpenDir( MagFolder );
		if( !Standalone && Debug ) logit( "et", "ewhtmlreport: Debug: scanning folder (%s), reviewing %d files\n", MagFolder, nfiles );
		if( nfiles == 1 )
		{
			if( !Standalone ) logit( "et", "ewhtmlreport: Invalid mag folder (%s)\n", MagFolder );
			free( fbuffer );
			return -1;
		}
		else {
			while ( !GetNextFileName(base_filename) )
			{
				/* beat heart in here if necessary since parsing logs can take a while */
				if( !Standalone && HeartbeatInt  &&  time( &timeNow ) - timeLastBeat >= HeartbeatInt ) 
				{
					timeLastBeat = timeNow;
					status( TypeHeartBeat, 0, "" );
				}
				// Skip dirs
				if( strcmp( base_filename, "." ) == 0 ||
						strcmp( base_filename, ".." ) == 0 )
				continue;
		
				// Combine file name
				sprintf( filename, "%s/%s", 
						MagFolder, base_filename);
		
				if( Debug ) logit( "et", "ewhtmlreport: Debug: Mag Processing %s\n", filename );
			
				
				/* Open file
		 		 ***********/
				if( ( fptr = fopen( filename, "r" ) ) == NULL )
				{
					//if( Debug ) logit( "o", "ewhtmlreport: Unable to open %s\n", 
					//		filename );
					continue;
				}
		
		
				/* Read file to buffer
				 *********************/
				nfbuffer = fread( fbuffer, sizeof( char ), 
						( size_t ) MaxMessageSize, fptr );
				fclose( fptr ); // Done reading
				if( nfbuffer == 0 )
				{
					//if( Debug ) logit( "o", "ewhtmlreport: No data read from %s\n",
					//		filename );
					continue;
				}

				/* Allocate memory for magnitude message
		 		 ***************************************/
				if( !( mag = ( MAG_INFO* ) calloc( 1, sizeof( MAG_INFO ) ) ) )
				{
					if( !Standalone ) logit( "et", "ewhtmlreport: Unable to allocate memory for "
							"mag message\n" );
					free( fbuffer );
					exit( -1 );
				}
				if( !( mag->pMagAux = ( char* ) calloc( 1, sizeof( MAG_CHAN_INFO )
						* MAX_MAG_CHANS ) ) )
				{
					if( !Standalone ) logit( "et",
							"ewhtmlreport: failed to allocate %d bytes"
							" for ML buffer; exiting!\n", 
							sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS  );
					free( fbuffer );
					exit( -1 );
				}
				mag->size_aux = sizeof( MAG_CHAN_INFO ) * MAX_MAG_CHANS;
		

				/* Parse message
		 		 ***************/
				if( rd_mag( fbuffer, nfbuffer, mag ) == -1 )
				{
					if( Debug ) logit( "e", "ewhtmlreport: Invalid magnitude file - %s\n", filename );
					free( mag->pMagAux );
					free( mag );
					continue;
				}

				
				/* Check if there are any picks available
				 ****************************************/
				if( mag->nstations == 0 ) // No stations, discard
				{			
					free( mag->pMagAux );
					free( mag );	
					continue;
				}


				/* Check if message is within required time interval
				 ***************************************************/
				pMagchan = ( MAG_CHAN_INFO* ) mag->pMagAux; // 1st channel
				if( pMagchan->Time1 < MinTime || pMagchan->Time1 >= MaxTime )
				{			
					free( mag->pMagAux );
					free( mag );	
					continue;
				}

			
				/* Store message in array
				 ************************/
				if( !( temp = ( MAG_INFO** ) realloc( mags,
						( nmags + 1 ) * sizeof( MAG_INFO* ) ) ) )
				{
					if( !Standalone ) logit( "et",
							"ewhtmlreport: failed to allocate "
							"for ML buffer; exiting!\n" );
					free( fbuffer );
					exit( -1 );
				}
				mags = temp;
				mags[nmags++] = mag;
				free( mag->pMagAux ); /* save space, we don't need the Aux info actually yet */
			}
			if( Debug ) logit( "et", "Debug: ewhtmlreport: Collected %d MAG messages\n", nmags );
		}
	}





	/* List all files in arc folder
	 ******************************/
	nfiles = OpenDir( ArcFolder );
	if( nfiles == 1 )
	{
		if( !Standalone ) logit( "et", "ewhtmlreport: Invalid arc folder (%s)\n", ArcFolder );
		free( fbuffer );
		if( UseML ) free( mags );
		exit( -1 );
	}

	
	if( Debug ) logit( "et", "ewhtmlreport: Debug: scanning folder (%s), reviewing %d files\n", ArcFolder, nfiles );
	/* Check files for arc messages
	 ******************************/
	fprintf( output, "[" );
	while(!GetNextFileName(base_filename))
	{
		
		if( !Standalone && HeartbeatInt  &&  time( &timeNow ) - timeLastBeat >= HeartbeatInt ) 
		{
			timeLastBeat = timeNow;
			status( TypeHeartBeat, 0, "" );
		}
		// Skip dirs
		if( strcmp( base_filename, "." ) == 0 ||
				strcmp( base_filename, ".." ) == 0 )
			continue;
			
			
		// Combine file name
		sprintf( filename, "%s/%s", 
				ArcFolder, base_filename);
				
						
		/* Open file
		 ***********/
		if( ( fptr = fopen( filename, "r" ) ) == NULL )
		{
			if( Debug ) logit( "et", "ewhtmlreport: Debug: Unable to open %s, skipping file\n", 
					filename );
			continue;
		}
		
			
		/* Read file to buffer
		 *********************/
		nfbuffer = fread( fbuffer, sizeof( char ), 
				( size_t ) MaxMessageSize, fptr );
		fclose( fptr ); // Done reading
		if( nfbuffer == 0 )
		{
			//if( Debug ) logit( "o", "ewhtmlreport: No data read from %s\n",
			//		filename );
			continue;
		}
		
			
		/* Parse message
		 ***************/
		arcmsg.sum.qid = -1;
		arcmsg.num_phases = 0;
		if( Debug ) logit( "et", "ewhtmlreport: Debug: Arc parsing %s\n", filename );
		if( parse_arc( fbuffer, &arcmsg ) != 0 || arcmsg.sum.qid == -1 )
		{
			if( Debug && !Standalone ) logit( "o", "ewhtmlreport: Error parsing %s\n", 
					filename );
			else if( Debug ) logit( "et", "ewhtmlreport: Error parsing %s\n", 
					filename );
			if (arcmsg.num_phases > 0) free_phases(&arcmsg);
			continue;
		}

	
		/* Check time of the message
		 ***************************/
		origintime = arcmsg.sum.ot - GSEC1970;
		if( origintime < MinTime || origintime >= MaxTime )
		{
			if ( Debug ) {
				fprintf( stderr, "Event %ld from %s discarded as out of time-range\n", 
					arcmsg.sum.qid, filename);
			}
 			free_phases(&arcmsg);
			continue;
		}
 		num_arcs ++;
		
	
		/* Find magnitude message associated with event
		 **********************************************/
		mag = NULL;
		if( UseML )
		{
			for( f = 0; f < nmags; f++ )
			{
				if( arcmsg.sum.qid == atoi( mags[f]->qid ) )
				{
					mag = mags[f];
					break;
				}
			}
		}


	
		/* Produce JSON associated with this event
		 *****************************************/
		quality_char = ComputeAverageQuality( arcmsg.sum.rms, 
                        arcmsg.sum.erh, arcmsg.sum.erz, arcmsg.sum.z, 
                        ( float ) ( 1.0 * arcmsg.sum.dmin ), arcmsg.sum.nph, 
                        arcmsg.sum.gap );
		if (quality_char <= MinQuality) 
		{
			if( isFirst == 1 )
			{
				arc2json( output, &arcmsg, mag );
				isFirst = 0;
			}
			else
			{
				fprintf( output, ",\n" );
				arc2json( output, &arcmsg, mag );
			}
			if (csv_output != NULL) arc2csv( csv_output, &arcmsg, mag );
			fflush( output );
		}
		else
		{
			if( Debug && !Standalone ) 
				logit( "o", "ewhtmlreport: Quality %c below desired level for qid %ld, skipping\n", 
					quality_char, arcmsg.sum.qid); 
			num_arcs--;
		}
                free_phases(&arcmsg);
	}
	
	fprintf( output, "\n]\n" );
	fflush( output );
	if( Debug ) logit( "et", "Debug: ewhtmlreport: Used %d ARC messages\n", num_arcs );

	
	// Free memory
	free( fbuffer );
	if( UseML )  
	{
		for( f = 0; f < nmags; f++ ) {
			/* free(mags[f]->pMagAux); */
			free(mags[f]);
		}
		free( mags );
	}
	return num_arcs;
}

void sendEmailToAll(char outfilename[MAX_FILENAME], char csv_outfilename[MAX_FILENAME], time_t StartTime, time_t EndTime, int num)
{
	char    system_command[MAX_STRING_SIZE];/* System command to call email prog */
	FILE    *header_file;         /* Email header file */
	char    hdrFilename[250];       /* Full header file name */
	int i, return_value;
	char StartStr[80];
	char EndStr[80];
	strftime( StartStr, 80, "%Y%m%d%H%M%S", gmtime(&StartTime) );
	strftime( EndStr, 80, "%Y%m%d%H%M%S", gmtime(&EndTime) );

	if( strlen( EmailProgram ) > 0 && nemailrecipients > 0 )
	{
		logit( "ot", "ewhtmlreport: Sending email alert to %d recipients.\n", nemailrecipients );
		for( i=0; i<nemailrecipients; i++ )// One email for each recipient
		{
			if( UseBlat )           // Use blat for sending email
			{
				/* the latest version of blat supports -attach option for text file attachment, send the HTML and CSV file */
				sprintf(system_command,"%s - -to %s -subject \"%s CSV data from %s to %s - %d events\" -body \"See attached files\" -attach %s -attach %s %s",
					EmailProgram, emailrecipients[i].address,
					SubjectPrefix, StartStr, EndStr, num, outfilename, csv_outfilename, BlatOptions);
			}
			else        // Use sendmail
			{
          /* this syntax below is for UNIX mail, which on most systems does 
 *            * not handle HTML content type, may use that in future...
 *                       ****************************************************************/
          /*sprintf(system_command,"cat %s | %s -s '%s - EW Event ID: %ld' %s",
 *               fullFilename, EmailProgram, SubjectPrefix,
 *                             arc->sum.qid, emailrecipients[i].address);
 *                                       */

          /* sendmail -t handles in line To: and other header info */

				/* Create email header file
				  **************************/
				sprintf( hdrFilename, "%s_header.tmp", HTMLFile );
				header_file = fopen( hdrFilename, "w" );
				fprintf( header_file, "To: %s\n", emailrecipients[i].address );
				fprintf( header_file, "Subject: %s CSV data from %s to %s - %d events\n",
					SubjectPrefix, StartStr, EndStr, num );
				fprintf( header_file, "Content-Type: text/plain\n\n" );
				fclose( header_file );

				/* System command for sendmail */
				sprintf(system_command,"cat %s %s | %s -t ",
					hdrFilename, csv_outfilename, EmailProgram);
			}
			if (Debug) {
				logit("et", "Debug: issuing this email command: %s\n", system_command);
			}

			/* Execute system command to send email
			  **************************************/
			return_value = system(system_command);
			if (Debug) {
				logit("et", "Debug: return value from command: %d\n", return_value);
			}
		}
	} else {
		if (Debug) {
			logit("t", "Debug: No email recipients or EmailProgram argument set, not sending email\n");
		}
  	}


	//For each recipient
	//Send email
	//If Blat
	//Send with blat
	//else
	//Send with UNIX mail
}
