

/*
*
*     Revision 1.0  2006/06/15 14:00:00  Anthony Lomax
*     Created from hyp2000_mgr.c (hyp2000_mgr.c,v 1.1.1.1 2005/07/14 20:10:34 paulf)
*
*     Revision ?.?  2010/05/12 15:00:00  Ruben S. Luis
*     Modified statements to support Linux.
*
*
*/


/*
*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
*   CHECKED IT OUT USING THE COMMAND CHECKOUT.
*
*    $Id: hyp2000_mgr.c,v 1.1.1.1 2005/07/14 20:10:34 paulf Exp $
*
*    Revision history:
*     $Log: hyp2000_mgr.c,v $
*     Revision 1.1.1.1  2005/07/14 20:10:34  paulf
*     Local ISTI CVS copy of EW v6.3
*
*     Revision 1.3  2002/06/05 15:30:48  patton
*     Made _Xlogit changes.
*
*     Revision 1.2  2001/04/18 21:08:57  dietz
*     Added the following commands to the hypoinverse startup phase
*     of hyp2000_mgr.  These commands set the input/output formats
*     to those used by Earthworm modules.  Added commands:
*       200, COP, CAR
*
*     Revision 1.1  2000/02/14 18:40:56  lucky
*     Initial revision
*
*
*/


/*******************************************************************
*                          nll_mgr.c                              *
*                                                                 *
*     C program for managing NLLoc                                *
*     nll_mgr calls NLLoc                                         *
*******************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <read_arc.h>
#include <chron3.h>

/* debug mode */
#define DEBUG_NLL 1

/* runs outsid active earthworm (no transport, etc.) */
//#define NO_EARTHWORM 1
//#undef IN_ACTIVE_EARTHWORM
/* runs inside active earthworm */
#define NO_EARTHWORM 0
#define IN_ACTIVE_EARTHWORM

/* Compatibility Flag */
/*   0 = old nll_mgr behaviour (no shadow lines, no loccode) */
/*   1 = full hyp2000_arc compatibility (shadow lines and loccode) */
#define LEGACY_NLL_COMPATIBILITY     0
#define FULL_HYP2000_COMPATIBILLITY  1

const int BufLen = MAX_BYTES_PER_EQ;      /* Message buffer length */
#define SUMCARDLEN 200
const int SumCardLen = SUMCARDLEN;

/* Function prototypes
*********************/
void GetConfig( char * );
void LookUp( void );
void hyp2000sum_hypo71sum2k( char * );
void ReportError( int );
void LogHypoError( int );
void PrintSumLine( char * );
int  callHypo( char * );
void Hypo( char *, int * );
int hypo2000arc_addLoccode( char *, char *, char * );

/* Read from (or derived from info in) configuration file
********************************************************/
char          RingName[20];  /* Name of transport ring to write to   */
SHM_INFO      Region;        /* working info for transport ring      */
unsigned char MyModId;       /* label outgoing messages with this id */
char          SourceCode;    /* label to tack on summary cards       */
int           LogSwitch;     /* Set level of output to log file:     */
			/*   0 = don't write a log file at all  */
			/*   1 = write only errors to log file  */
			/*   2 = write errors & summary lines   */
int           HypCompatFlag; /* Hyp2000_arc message compatibility flag */

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;       /* key to pick transport ring            */
static unsigned char InstId;        /* local installation id                 */
static unsigned char TypeError;
static unsigned char TypeKill;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeH71Sum2K;

/* These strings contain file names
********************************/
static char arcIn[1024];          /* Archive file sent to NLLoc            */
static char arcOut[1024];         /* Archive file output by NLLoc          */
static char arcOutLC[1024];         /* Archive file output by NLLoc with input Loccodes        */
static char sumName[1024];        /* Summary file output by NLLoc          */

/* Error to send to statmgr.  Errors 100-149 are reserved for nll_mgr.
(must not overlap with error numbers of other links in the sausage.)
************************************************************************/
#define ERR_TOOBIG        101   /* Got a msg from pipe too big for target  */
#define ERR_TMPFILE       102   /* Error writing hypoinverse input file    */
#define ERR_ARCFILE       103   /* Error reading hypoinverse archive file  */
#define ERR_ARC2RING      104   /* Trouble putting arcmsg in memory ring   */
#define ERR_SUM2RING      105   /* Trouble putting summsg in memory ring   */
#define ERR_MSG2RING      106   /* Trouble passing a msg from pipe to ring */
#define ERR_PIPEPUT       107   /* Trouble writing to pipe                 */


/* 2006/06/16 AJL - Additional defines and vairables added
********************************/
static char nllCtrlFile[1024];    /* NLL control file with full path       */
static char nllStaFile[1024];     /* NLL file containing station coordinates in NLL GTSRCE format */
static char nllTimePath[1024];    /* NLL travel-time grid files path/root  */
static char nllTimeSwap[16];      /* NLL LOCFILES iSwapBytesOnInput value  */
static char nllOutPath[1024];     /* NLL output file path/root             */
#ifdef _WINNT
#define PATH_SEPARATOR '\\' /* Modified by RSL */
#endif
#if defined(_SOLARIS) || defined(_LINUX) || defined(_MACOSX)
#define PATH_SEPARATOR '/'
#endif










       /******************************************************
       * _Xlogit
       *
       *   logit alias for debugging
       *
       *   AJL 20051010
       *
        ****************************************************/

#ifdef IN_ACTIVE_EARTHWORM
#define _Xlogit logit
#else
#undef _Xlogit
int _Xlogit( char *flag, char *format, ... )
{
            printf(format);

}
#endif

void _Xlogit_init( char *prog, short mid, int bufSize, int logflag )
{
#ifdef IN_ACTIVE_EARTHWORM
       logit_init(prog, mid, bufSize, logflag);
#endif
}





int main( int argc, char *argv[] )
{
	static char msg[MAX_BYTES_PER_EQ];  /* Message buffer (for .arc messages) */
	MSG_LOGO    logo;                /* Type, module, instid of retrieved msg */
	FILE       *fptmp;           /* Pointer to temporary file (hypoinv input) */
	FILE       *fparc;           /* Pointer to archive file (hypoinv output)  */
	char        sumCard[200];    /* Summary card produced by hypoinverse      */
	int         rc;
	size_t      nread;           /* Number of bytes read from file            */
	int         length;
	char        configFile[1024];
	char        commandFile[1024];
	int         type;
	int         iresr;
	char        hypo_msg[80];

/* 2006/06/16 AJL - Additional defines and vairables added
********************************/
	int         istat;
	FILE       *fptmpin;           /* Pointer to temporary file */
	FILE       *fptmpout;          /* Pointer to temporary file */
	int         chrtmp;
	char        eventIdentNumStr[16]; /* Hypoinverse Y2K Event identification number string */
	int         eventIdentNum;        /* Hypoinverse Y2K Event identification number */


/* Check command line arguments
****************************/
        //printf("DEBUG argc %d\n", argc);
	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: nll_mgr <config file>\n" );
		return -1;
	}
	strcpy(configFile, argv[1]);
//printf("DEBUG configfile %s\n", configFile);

/* Initialize name of log-file & open it
***************************************/
	_Xlogit_init( argv[1], 0, 512, 1 );
//printf("DEBUG TP 2\n");
        //printf("DEBUG argc %d\n", argc);


/* Read in the configuration file
******************************/
	GetConfig( configFile );
//printf("DEBUG TP 3\n");
        //printf("DEBUG argc %d\n", argc);

        _Xlogit( "e" , "nll_mgr: Read configuration file <>\n", configFile );
	//_Xlogit( "e" , "nll_mgr: Read configuration file <%s>\n", configFile );
//printf("DEBUG TP 4\n");
        //printf("DEBUG argc %d\n", argc);


/* Make up the names of the temporary arc files
********************************************/
	/* 2006/06/16 AJL - following 2 lines commented, now set in GetConfig */
/*sprintf( arcIn,  "hypoMgrArcIn%u",  MyModId );
/*sprintf( arcOut, "hypoMgrArcOut%u", MyModId );
*/
	strcpy( sumName, "none" );
                //printf("DEBUG argc %d\n", argc);


	/* 2006/06/16 AJL - Added */
/* Make up the names of nll i/o files
********************************/
	sprintf( arcIn,  "%s%cnllMgrArcIn", nllOutPath, PATH_SEPARATOR );
	if (DEBUG_NLL) printf( "nll_mgr: arcIn <%s>\n", arcIn); /*DEBUG*/
                //printf("DEBUG argc %d\n", argc);

	sprintf( arcOut, "%s%clast.arc", nllOutPath, PATH_SEPARATOR );
	if (DEBUG_NLL) printf( "nll_mgr: arcOut <%s>\n", arcOut); /*DEBUG*/
                //printf("DEBUG argc %d\n", argc);

	sprintf( arcOutLC, "%s%clast.arc.lc", nllOutPath, PATH_SEPARATOR );
	if (DEBUG_NLL) printf( "nll_mgr: arcOutLC <%s>\n", arcOutLC); /*DEBUG*/
                //printf("DEBUG argc %d\n", argc);



/* Look up important info from earthworm.h tables
************************************************/
	LookUp();

/* Reinitialize _Xlogit to desired logging level
*********************************************/
        //printf("DEBUG argc %d\n", argc);
        //printf("DEBUG argv[1] %s\n", argv[1]);
	_Xlogit_init( argv[1], 0, 512, LogSwitch );

/* Attach to the transport ring (Must already exist)
*************************************************/
	if (DEBUG_NLL) printf( "nll_mgr: tport_attach called\n"); /*DEBUG*/
	if (!NO_EARTHWORM) tport_attach( &Region, RingKey );

/* Read in station & model info via command file
*********************************************/
	/* 2006/06/16 AJL - following 4 lines commented, not needed for NLL */
/*sprintf( hypo_msg, "@%s", commandFile );
	/*if( callHypo( hypo_msg ) != 0 ) return -1;     /* load startup file */
/*_Xlogit( "t", "nll_mgr: Initialized hypoinverse with file <%s>\n",
	commandFile );
*/

/* Hard-wire input phase file name & format
****************************************/
	/* 2006/06/16 AJL - following 4 lines commented, not needed for NLL */
	/*if( callHypo( "200 T 1900 0" ) != 0 ) return -1; /* force Y2K formats  */
	/*if( callHypo( "COP 5" )        != 0 ) return -1; /* input arc w/shadow */
/*sprintf( hypo_msg, "PHS '%s'", arcIn );
	/*if( callHypo( hypo_msg )       != 0 ) return -1; /* input phase file   */

/* Hard-wire output file names & formats
*************************************/
	/* 2006/06/16 AJL - following 5 lines commented, not needed for NLL */
	/*if( callHypo( "CAR 3" )        != 0 ) return -1; /* output arc w/shadow*/
/*sprintf( hypo_msg, "ARC '%s'", arcOut );
	/*if( callHypo( hypo_msg )       != 0 ) return -1; /* archive output file*/
/*sprintf( hypo_msg, "SUM '%s'", sumName );
	/*if( callHypo( hypo_msg )       != 0 ) return -1; /* summary output file*/

/* Get a message from the pipe
***************************/
	while ( 1 )
	{
		if (DEBUG_NLL) printf( "nll_mgr: pipe_get called\n"); /*DEBUG*/
		rc = pipe_get( msg, BufLen, &type );
		if (DEBUG_NLL) printf( "nll_mgr: pipe_get return <%d>\n", rc); /*DEBUG*/
		if ( rc < 0 )
		{
			if ( rc == -1 )
			{
				ReportError( ERR_TOOBIG );
				_Xlogit( "et", "nll_mgr: Message in pipe too big for buffer. Exiting.\n" );
			}
			else if ( rc == -2 )
				_Xlogit( "et", "nll_mgr: <null> on pipe_get. Exiting.\n" );
			else if ( rc == -3 )
				_Xlogit( "et", "nll_mgr: EOF on pipe_get. Exiting.\n" );
			break;
		}

/* Stop program if this is a "kill" message
****************************************/
		if ( type == (int) TypeKill )
		{
			_Xlogit( "t", "nll_mgr: Termination requested. Exiting.\n" );
			break;
		}

/* This is an archive file event
*****************************/
		if ( type == (int) TypeHyp2000Arc )
		{
			if (DEBUG_NLL) printf( "nll_mgr: type == (int) TypeHyp2000Arc\n"); /*DEBUG*/


/* Create temporary archive file.  This file will be
			overwritten the next time an event is processed.
*************************************************/
			if (DEBUG_NLL) printf( "nll_mgr: creating file <%s>\n", arcIn); /*DEBUG*/
			fptmp = fopen( arcIn, "w" );
			if ( fptmp == (FILE *) NULL )
			{
				ReportError( ERR_TMPFILE );
				_Xlogit( "et", "nll_mgr: Error creating file: %s\n", arcIn );
				continue;
			}
			length = strlen( msg );
			if ( fwrite( msg, (size_t)1, (size_t)length, fptmp ) == 0 )
			{
				ReportError( ERR_TMPFILE );
				_Xlogit( "et", "nll_mgr: Error writing to file: %s\n", arcIn );
				fclose( fptmp );
				continue;
			}
			fclose( fptmp );

/* Get the Event identification number
****************/
			strncpy( eventIdentNumStr, msg+136, 10 );  /* event id */
			istat = sscanf(eventIdentNumStr, "%d", &eventIdentNum);
			if (DEBUG_NLL) printf( "nll_mgr: event identification number <%d>\n", eventIdentNum); /*DEBUG*/
			if ( istat != 1 )
			{
				ReportError( ERR_TMPFILE );
				_Xlogit( "et", "nll_mgr: Error parsing event identification number: <%s>\n", eventIdentNumStr );
				return -1;
			}



/*** 2006/06/16 AJL - Added */

/* Construct the NLL control file
********************************/
			if (DEBUG_NLL) printf( "nll_mgr: opening file <%s>\n", nllCtrlFile); /*DEBUG*/
			fptmpin = fopen( nllCtrlFile, "r" );
			if ( fptmpin == (FILE *) NULL )
			{
				ReportError( ERR_TMPFILE );
				_Xlogit( "et", "nll_mgr: Error opening file: %s\n", nllCtrlFile );
				return -1;
			}
			sprintf( commandFile, "%s%cnll_mgr.in", nllOutPath, PATH_SEPARATOR );
			if (DEBUG_NLL) printf( "nll_mgr: creating file <%s>\n", commandFile); /*DEBUG*/
			fptmpout = fopen( commandFile, "w" );
			if ( fptmpout == (FILE *) NULL )
			{
				ReportError( ERR_TMPFILE );
				_Xlogit( "et", "nll_mgr: Error creating file: %s\n", commandFile );
				return -1;
			}
/* Copy the skeleton NLL control file
********************************/
			if (DEBUG_NLL) printf( "nll_mgr: copying file <%s> to file <%s>\n", nllCtrlFile, commandFile); /*DEBUG*/
			while ((chrtmp = fgetc(fptmpin)) != EOF) {
				if ( fputc(chrtmp, fptmpout) == EOF )
				{
					ReportError( ERR_TMPFILE );
					_Xlogit( "et", "nll_mgr: Error writing to file: %s\n", commandFile );
					fclose( fptmp );
					return -1;
				}
			}
			fclose( fptmpin );
/* Append custom LOCFILES statement, e.g.
			LOCFILES /temp/nlloc_tmp/earthworm/nll_mgr0/nllMgrArcIn HYPOINVERSE_Y2000_ARC /temp/nlloc_tmp/taup/ak135/ak135 /temp/nlloc_tmp/earthworm/nll_mgr0/nll_mgr 1
********************************/
			fprintf(fptmpout, "\n# Following line added autmoatically by nll_mgr:\n");
			fprintf(fptmpout, "LOCFILES %s HYPOINVERSE_Y2000_ARC %s %s%c%010d %s\n\n",
				arcIn, nllTimePath, nllOutPath, PATH_SEPARATOR, eventIdentNum, nllTimeSwap);
/* Append file containing station coordinates in NLL GTSRCE format
********************************/
			fprintf(fptmpout, "\n# Following line added autmoatically by nll_mgr:\n");
			fprintf(fptmpout, "INCLUDE %s\n\n", nllStaFile);

/* Make sure LOCHYPOUT SAVE_HYPOINVERSE_Y2000_ARC is present
********************************/
			fprintf(fptmpout, "\n# Following line added autmoatically by nll_mgr:\n");
			fprintf(fptmpout, "LOCHYPOUT SAVE_HYPOINVERSE_Y2000_ARC\n\n");

			fclose( fptmpout );

/*** END - 2006/06/16 AJL - Added */



/* Locate the event
****************/
			if( callHypo( commandFile ) != 0 ) {
				_Xlogit( "e", "nll_mgr: Nonfatal error(s) locating event.\n" );
			}

			if ( HypCompatFlag == LEGACY_NLL_COMPATIBILITY ) {
/* Get the archive file from disk
******************************/
				if (DEBUG_NLL) printf( "nll_mgr: reading arcfile <%s>\n", arcOut); /*DEBUG*/
				fparc = fopen( arcOut, "r" );
				if ( fparc == (FILE *) NULL )
				{
					if (DEBUG_NLL) printf( "nll_mgr: Error opening arcfile <%s>\n", arcOut); /*DEBUG*/
					ReportError( ERR_ARCFILE );
					_Xlogit( "et", "nll_mgr: Error opening arcfile <%s>\n", arcOut );
					continue;
				}
				nread = fread( msg, sizeof(char), (size_t)BufLen, fparc );
				fclose( fparc );
				if ( nread == 0 )
				{
					ReportError( ERR_ARCFILE );
					_Xlogit( "et", "nll_mgr: Error reading arcfile <%s>\n", arcOut );
					continue;
				}
				remove( arcOut );
			} else if ( HypCompatFlag == FULL_HYP2000_COMPATIBILLITY ) {
/* Replace the default output "  " loccode by the input loccode
************************************************************/
				if( hypo2000arc_addLoccode( arcIn, arcOut, arcOutLC ) != 0 ) {
					_Xlogit( "e", "nll_mgr: Nonfatal error(s) updating loccodes.\n" );
				}
/* Get the loccode enabled archive file from disk
**********************************************/
				if (DEBUG_NLL) printf( "nll_mgr: reading arcfile <%s>\n", arcOutLC); /*DEBUG*/
				fparc = fopen( arcOutLC, "r" );
				if ( fparc == (FILE *) NULL )
				{
					if (DEBUG_NLL) printf( "nll_mgr: Error opening arcfile <%s>\n", arcOutLC); /*DEBUG*/
					ReportError( ERR_ARCFILE );
					_Xlogit( "et", "nll_mgr: Error opening arcfile <%s>\n", arcOutLC );
					continue;
				}
				nread = fread( msg, sizeof(char), (size_t)BufLen, fparc );
				fclose( fparc );
				if ( nread == 0 )
				{
					ReportError( ERR_ARCFILE );
					_Xlogit( "et", "nll_mgr: Error reading arcfile <%s>\n", arcOutLC );
					continue;
				}
				remove( arcOut );
				remove( arcOutLC );
			}
	//DEBUG 	remove( arcIn );
/* Send the archive file to the transport ring
*******************************************/
			if (DEBUG_NLL) printf( "nll_mgr: sending the archive file to the transport ring\n"); /*DEBUG*/
			logo.instid = InstId;
			logo.mod    = MyModId;
			logo.type   = TypeHyp2000Arc;
			if (NO_EARTHWORM) {
				printf( "nll_mgr: No active EW: Bypassing sending of archive msg to transport ring  <%s>\n", RingName);
			} else {
				if ( tport_putmsg( &Region, &logo, (long) nread, msg ) != PUT_OK )
				{
					ReportError( ERR_ARC2RING );
					_Xlogit( "et",
					       "nll_mgr: Error sending archive msg to transport ring.\n" );
				}
			}

/* Get the summary line from archive message (first line)
******************************************************/
			if (DEBUG_NLL) printf( "nll_mgr: getting the summary line from archive message (first line)\n"); /*DEBUG*/
			sscanf( msg, "%[^\n]", sumCard );
			/*_Xlogit("e", "1st line of arcmsg:\n%s\n\n", sumCard );*/ /*DEBUG*/

/* Convert the hypoinverse summary to hypo71
			summary and add the source code to it
*****************************************/
			if (DEBUG_NLL) printf( "nll_mgr: converting the hypoinverse summary to hypo71 summary and add the source code to it\n"); /*DEBUG*/
			hyp2000sum_hypo71sum2k( sumCard );
			sumCard[81] = SourceCode;
			
/* Print a status line to stderr
*****************************/
			PrintSumLine( sumCard );

/* Send the summary file to the transport ring
*******************************************/
			if (DEBUG_NLL) printf( "nll_mgr: sending the hypo71 summary file to the transport ring\n"); /*DEBUG*/
			logo.instid = InstId;
			logo.mod    = MyModId;
			logo.type   = TypeH71Sum2K;
			if (NO_EARTHWORM) {
				printf( "nll_mgr: No active EW: Bypassing sending of summary line to transport ring  <%s>\n%s\n", RingName, sumCard);
			} else {
				if ( tport_putmsg( &Region, &logo, strlen(sumCard), sumCard ) != PUT_OK )
				{
					ReportError( ERR_SUM2RING );
					_Xlogit( "et",
					       "nll_mgr: Error sending summary line to transport ring.\n" );
				}
			}
		}

/* The message is not of TYPE_KILL or TYPE_HYP2000ARC.
		Send it to the HYPO_RING.
***************************************************/
		else
		{
			logo.instid = InstId;
			logo.mod    = MyModId;
			logo.type   = type;
			if (NO_EARTHWORM) {
				printf( "nll_mgr: No active EW: Bypassing sending of msg (type %d) to transport ring  <%s>\n", (int) type, RingName);
			} else {
				if ( tport_putmsg( &Region, &logo, strlen(msg), msg ) != PUT_OK )
			{
				ReportError( ERR_MSG2RING );
				_Xlogit( "et",
				       "nll_mgr: Error passing msg (type %d) to transport ring.\n",
				       (int) type );
			}
			}
		}
	}

/* Detach from the transport ring and exit
***************************************/
	if (DEBUG_NLL) printf( "nll_mgr: tport_detach called\n"); /*DEBUG*/
	if (!NO_EARTHWORM) tport_detach( &Region );

	return 0;
}


/*******************************************************************
*                           GetConfig()                           *
*                                                                 *
*  Processes command file using kom.c functions.                  *
*  Exits if any errors are encountered.                           *
*******************************************************************/
#define ncommand 9        /* # of required commands you expect to process   */
void GetConfig(char *configfile)
{
	char   init[ncommand]; /* init flags, one byte for each required command */
	int    nmiss;          /* number of required commands that were missed   */
	char  *com;
	char  *str;
	int    nfiles;
	int    success;
	int    i;

/* Set to zero one init flag for each required command
*****************************************************/
	for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file
**********************************/
	nfiles = k_open( configfile );
	if ( nfiles == 0 ) {
		_Xlogit( "e",
		       "nll_mgr: Error opening configuration file <%s>. Exiting.\n",
		       configfile );
		exit( -1 );
	}
/* By default, make nll_mgr behave as it has always behaved
**********************************************************/
        HypCompatFlag = LEGACY_NLL_COMPATIBILITY;

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
					_Xlogit( "e",
					       "nll_mgr: Error opening command file <%s>. Exiting.\n",
					       &com[1] );
					exit( -1 );
				}
				continue;
			}

	/* Process anything else as a command
	************************************/
/*0*/    if ( k_its( "RingName" ) ) {
		 str = k_str();
		 if(str) strcpy(RingName, str);
		 init[0] = 1;
}
/*1*/    else if( k_its( "LogFile" ) ) {
		 LogSwitch = k_int();
		 init[1] = 1;
}
/*2*/    else if( k_its( "MyModuleId" ) ) {
		 if ( ( str=k_str() ) ) {
			 if ( GetModId( str, &MyModId ) != 0 ) {
				 _Xlogit( "e",
					"nll_mgr: Invalid module name <%s>. Exiting.\n",
					str );
				 exit( -1 );
			 }
		 }
		 init[2] = 1;
}
/*3*/    else if( k_its( "SourceCode" ) ) {
		 if ( str=k_str() ) {
			 SourceCode = str[0];
		 }
		 init[3] = 1;
}
/*4*/    else if( k_its( "NllCtrlFile" ) ) {
		 if ( str=k_str() ) {
			 strcpy(nllCtrlFile, str);
		 }
		 init[4] = 1;
}
/*5*/    else if( k_its( "NllTimePath" ) ) {
		 if ( str=k_str() ) {
			 strcpy(nllTimePath, str);
		 }
		 init[5] = 1;
}
/*6*/    else if( k_its( "NllTimeSwap" ) ) {
		 if ( str=k_str() ) {
			 strcpy(nllTimeSwap, str);
		 }
		 init[6] = 1;
}
/*7*/    else if( k_its( "NllOutPath" ) ) {
		 if ( str=k_str() ) {
			 strcpy(nllOutPath, str);
		 }
		 init[7] = 1;
}
/*8*/    else if( k_its( "NllStaFile" ) ) {
		 if ( str=k_str() ) {
			 strcpy(nllStaFile, str);
		 }
		 init[8] = 1;
}
         else if( k_its( "Hyp2000arcCompatibilityMode" ) ) {
                 HypCompatFlag = FULL_HYP2000_COMPATIBILLITY;
}
	else {
		_Xlogit( "e", "nll_mgr: <%s> unknown command in <%s>.\n",
		       com, configfile );
		continue;
	}

	/* See if there were any errors processing the command
	*****************************************************/
	if( k_err() )
	{
		_Xlogit( "e", "nll_mgr: Bad <%s> command in <%s>; \n",
		       com, configfile );
		exit( -1 );
	}
		}
		nfiles = k_close();
	}

/* After all files are closed, check init flags for missed commands
******************************************************************/
	nmiss = 0;
	for ( i = 0; i < ncommand; i++ )
		if( !init[i] ) nmiss++;

	if ( nmiss )
	{
		_Xlogit( "e", "nll_mgr: ERROR, no " );
		if ( !init[0] )  _Xlogit( "e", "<RingName> "   );
		if ( !init[1] )  _Xlogit( "e", "<LogFile> "    );
		if ( !init[2] )  _Xlogit( "e", "<MyModuleId> " );
		if ( !init[3] )  _Xlogit( "e", "<SourceCode> " );
		if ( !init[4] )  _Xlogit( "e", "<NllCtrlFile> " );
		if ( !init[5] )  _Xlogit( "e", "<NllTimePath> " );
		if ( !init[6] )  _Xlogit( "e", "<NllTimeSwap> " );
		if ( !init[7] )  _Xlogit( "e", "<NllOutPath> " );
		if ( !init[8] )  _Xlogit( "e", "<NllStaFile> " );
		_Xlogit( "e", "command(s) in <%s>. Exiting.\n", configfile );
		exit( -1 );
	}

	return;
}


/************************************************************************
*                               LookUp()                               *
*            Look up important info from earthworm.h tables            *
************************************************************************/

void LookUp( void )
{
/* Look up keys to shared memory regions of interest
*************************************************/
	if( (RingKey = GetKey(RingName)) == -1 )
	{
		fprintf( stderr,
			 "nll_mgr: Invalid ring name <%s>. Exiting.\n", RingName );
		exit( -1 );
	}

/* Look up installations of interest
*********************************/
	if ( GetLocalInst( &InstId ) != 0 )
	{
		fprintf( stderr,
			 "nll_mgr: error getting local installation id. Exiting.\n" );
		exit( -1 );
	}

/* Look up message types of interest
*********************************/
	if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
	{
		fprintf( stderr,
			 "nll_mgr: Invalid message type <TYPE_ERROR>. Exiting.\n" );
		exit( -1 );
	}

	if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 )
	{
		fprintf( stderr,
			 "nll_mgr: Invalid message type <TYPE_HYP2000ARC>. Exiting.\n" );
		exit( -1 );
	}

	if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum2K ) != 0 )
	{
		fprintf( stderr,
			 "nll_mgr: Invalid message type <TYPE_H71SUM2K>. Exiting.\n" );
		exit( -1 );
	}

	if ( GetType( "TYPE_KILL", &TypeKill ) != 0 )
	{
		fprintf( stderr,
			 "nll_mgr: Invalid message type <TYPE_KILL>. Exiting.\n" );
		exit( -1 );
	}

	return;
}


	/****************************************************
	*                   PrintSumLine()                 *
	*  Print a summary line to stderr (always) and     *
	*  to the log file if LogSwitch = 2                *
	****************************************************/

void PrintSumLine( char *sumCard )
{

/* Send the summary line to the screen
***********************************/
	fprintf( stderr, "%s", sumCard );

/* Write summary line to log file
******************************/
	if ( LogSwitch == 2 )
		_Xlogit( "", "%s", sumCard );
	return;
}


/*******************************************************************
*                  hyp2000sum_hypo71sum2k()                       *
*                                                                 *
*  Converts from hypoinverse to hypo71 summary format.            *
*  Original hinv_hypo71 function written by Andy Michael.         *
*  Modified by Lynn Dietz to make it work with Y2K-compliant      *
*  hyp2000(hypoinverse) and hypo71 summary formats. 11/1998       *
*******************************************************************/

void hyp2000sum_hypo71sum2k( char *sumcard )
{
	char hinvsum[200];
	char h71sum[200];
	float rms,erh,erz,dmin,depth;
	int no,gap;
	int qs,qd,qa;
	int i;

/*-------------------------------------------------------------------------------------------
Sample Hyp2000 (Hypoinverse) summary card, fixed format, 165 chars, including newline:
	199204290116449937 3790122 2919  505  0 31109  8   810577  4625911  31276SFP  13    0  31  45 58
	0 570  0 39PEN WW D 67X   0  0L  0  0     10133D276 570Z  0   01 \n
	0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456
	789 123456789 123456789 123456789 123456789 123456789 123456789 123456789

Sample TYPE_H71SUM2K summary card, fixed-format, 96 characters, including newline:
	19920429 0116 44.99 37 37.90 122 29.19   5.05 D 2.76 31 109  8.  0.08  0.3  0.4 BW      10133 1\n
	0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456
	---------------------------------------------------------------------------------------------*/

/* Make a working copy of the original hypoinverse summary card.
	Don't use spot 0 in the arrays; it is easier to read &
	matches column numbers in documentation.
*************************************************************/
	hinvsum[0] = ' ';
	strcpy( hinvsum+1, sumcard );
	for( i = 0; i < 200; ++i )  h71sum[i] = ' ';

/* Transfer event origin time
**************************/
	strncpy( h71sum+1, hinvsum+1, 8 );                /* date (yyyymmdd) */
	for(i=1;i<=8;++i)    if(h71sum[i]==' ')  h71sum[i]='0';
	h71sum[9] = ' ';

	strncpy( h71sum+10, hinvsum+9, 4 );               /* hour minute */
	for(i=11;i<=13;++i)  if(h71sum[i]==' ')  h71sum[i]='0';
	h71sum[14] = ' ';

	strncpy( h71sum+15, hinvsum+13, 2 );              /* whole seconds */
	h71sum[17] = '.';
	strncpy( h71sum+18, hinvsum+15, 2 );              /* fractional seconds */
	if(h71sum[18]==' ') h71sum[18]='0';
	h71sum[20] = ' ';

/* Transfer event location
***********************/
	strncpy( h71sum+21 , hinvsum+17, 5 );            /* lat degs and whole mins */
	h71sum[26] = '.';
	strncpy( h71sum+27 , hinvsum+22, 2  );           /* lat fractional mins */
	if(h71sum[27]==' ') h71sum[27]= '0';
	h71sum[29] = ' ';

	strncpy( h71sum+30, hinvsum+24, 6 );              /* lon degs and whole mins */
	h71sum[36] = '.';
	strncpy( h71sum+37, hinvsum+30, 2 );              /* lon fractional mins */
	if(h71sum[37]==' ') h71sum[37]= '0';
	h71sum[39] = ' ';

	strncpy( h71sum+40, hinvsum+32, 3 );              /* whole depth */
	h71sum[43] = '.';
	strncpy( h71sum+44, hinvsum+35, 2 );              /* fractional depth */
	for(i=42;i<=45;++i) if(h71sum[i]==' ') h71sum[i]='0';
	h71sum[46]=' ';

/* Transfer magnitude.  Get the "Preferred magnitude" if it exists;
	otherwise, use the "Primary coda duration magnitude" field
***************************************************************/
	if( strlen(sumcard) >= (size_t) 151 ) {
		h71sum[47] = hinvsum[147];    /* Preferred magnitude label */
		h71sum[48] = ' ';
		h71sum[49] = hinvsum[148];    /* Preferred magnitude       */
		h71sum[50] = '.';
		h71sum[51] = hinvsum[149];
		h71sum[52] = hinvsum[150];
		for(i=49;i<=51;++i) if(h71sum[i]==' ') h71sum[i]='0';
	} else {
		h71sum[47] = hinvsum[118];    /* primary coda magnitude label */
		h71sum[48] = ' ';
		h71sum[49] = hinvsum[71];     /* primary coda magnitude       */
		h71sum[50] = '.';
		h71sum[51] = hinvsum[72];
		h71sum[52] = hinvsum[73];
		for(i=49;i<=51;++i) if(h71sum[i]==' ') h71sum[i]='0';
	}

/* Transfer other location statistics
**********************************/
	strncpy( h71sum+53, hinvsum+40, 3 );    /* number of stations */
	h71sum[56] = ' ';

	strncpy( h71sum+57, hinvsum+43, 3 );    /* azimuthal gap */

	strncpy( h71sum+60, hinvsum+46, 3 );    /* distance to closest station */
	h71sum[63] = '.';
	h71sum[64] = ' ';

	strncpy( h71sum+65, hinvsum+49, 2 );    /* whole rms */
	h71sum[67] = '.';
	strncpy( h71sum+68, hinvsum+51, 2 );    /* frac rms */
	for(i=66;i<=68;++i) if(h71sum[i]==' ') h71sum[i]='0';
	h71sum[70] = ' ';

	strncpy( h71sum+71, hinvsum+86, 2 );    /* whole horiz err */
	h71sum[73] = '.';
	h71sum[74] = hinvsum[88];               /*frac horiz err   */
	for(i=72;i<=74;++i) if(h71sum[i]==' ') h71sum[i]='0';
	h71sum[75]=' ';

	strncpy( h71sum+76, hinvsum+90, 2 );    /* whole vert err */
	h71sum[78] = '.';
	h71sum[79] = hinvsum[92];               /*frac vert err   */
	for(i=77;i<=79;++i) if(h71sum[i]==' ') h71sum[i]='0';
	h71sum[80] = hinvsum[81];               /* auxiliary remark 1 */

	h71sum[82] = hinvsum[115];              /* most common data source used in location */
	h71sum[83] = ' ';

/* Transfer event id
*****************/
	strncpy( h71sum+84, hinvsum+137, 10 );  /* event id */
	h71sum[94] = ' ';
	h71sum[95] = hinvsum[163];              /* version number */
	h71sum[96] = '\n';
	h71sum[97] = '\0';

/* Extract rms, erh, erz, no, gap, dmin, depth to decide on location quality
*************************************************************************/
	sscanf( h71sum+41, "%5f", &depth );
	sscanf( h71sum+53, "%3d", &no    );
	sscanf( h71sum+57, "%3d", &gap   );
	sscanf( h71sum+60, "%5f", &dmin  );
	sscanf( h71sum+65, "%5f", &rms   );
	sscanf( h71sum+71, "%4f", &erh   );
	sscanf( h71sum+76, "%4f", &erz   );

/* Compute qs, qd, and average quality
***********************************/
	if     (rms <0.15 && erh<=1.0 && erz <= 2.0) qs=4;  /* qs is A */
	else if(rms <0.30 && erh<=2.5 && erz <= 5.0) qs=3;  /* qs is B */
	else if(rms <0.50 && erh<=5.0)               qs=2;  /* qs is C */
	else                                         qs=1;  /* qs is D */

	if     (no >= 6 && gap <=  90 && (dmin<=depth    || dmin<= 5)) qd=4; /* qd is A */
	else if(no >= 6 && gap <= 135 && (dmin<=2.*depth || dmin<=10)) qd=3; /* qd is B */
	else if(no >= 6 && gap <= 180 &&  dmin<=50)                    qd=2; /* qd is C */
	else                                                           qd=1; /* qd is D */

	qa = (qs+qd)/2; /* use integer truncation to round down */
	if(qa>=4) h71sum[81] = 'A';
	if(qa==3) h71sum[81] = 'B';
	if(qa==2) h71sum[81] = 'C';
	if(qa<=1) h71sum[81] = 'D';

/* Copy converted summary card back to caller's address
****************************************************/
	strcpy( sumcard, h71sum+1 );
	return;
}


/****************************************************************************/
/* hypo2000arc_addLoccode() reads input arc_message and adds the stations   */
/*                       loccodes to the output arc_message produced by     */
/*                       NLL                                                */
/****************************************************************************/

int hypo2000arc_addLoccode( char *arcIn, char *arcOut, char *arcOutLC )
{
	HypoArc	   In;        /* Hyp2000 arcIn data                    */
	HypoArc    Out;       /* Hyp2000 arcOut data                   */
	int	   i,j;
	char      *msgIn;  /* Message buffer (for .arc messages) */
	char      *msgOut;  /* Message buffer (for .arc messages) */
	FILE      *fparc;           /* Pointer to archive file (hypoinv output)  */
	FILE      *fptmp;           /* Pointer to temporary file (hypoinv input) */
	size_t     nread;           /* Number of bytes read from file            */
	int        length;

	/* Allocate the input message buffer
	***********************************/
	if ( !( msgIn = (char *) calloc( 1, MAX_BYTES_PER_EQ ) ) )
	{
		_Xlogit( "et", "hypo2000arc_addLoccode: Error allocating %s bytes for input message buffer\n", MAX_BYTES_PER_EQ );
		return(1);
	}
	/* Allocate the output message buffer
	***********************************/
	if ( !( msgOut = (char *) calloc( 1, MAX_BYTES_PER_EQ ) ) )
	{
		_Xlogit( "et", "hypo2000arc_addLoccode: Error allocating %s bytes for output message buffer", MAX_BYTES_PER_EQ );
		return(1);
	}

/* Get the input archive file from disk
******************************/
	if (DEBUG_NLL) printf( "nll_mgr: reading input arcfile <%s>\n", arcIn); /*DEBUG*/
	fparc = fopen( arcIn, "r" );
	if ( fparc == (FILE *) NULL )
	{
	if (DEBUG_NLL) printf( "nll_mgr: Error opening input arcfile <%s>\n", arcIn); /*DEBUG*/
		_Xlogit( "et", "nll_mgr: Error opening input arcfile <%s>\n", arcIn );
		return(1);
	}
	nread = fread( msgIn, sizeof(char), (size_t)BufLen, fparc );
	fclose( fparc );
	if ( nread == 0 )
	{
		_Xlogit( "et", "nll_mgr: Error reading input arcfile <%s>\n", arcIn );
		return(1);
	}
	/* Parse message
	 ***************/
	In.num_phases = 0;
	_Xlogit( "et", "nll_arcOut_addLoccode: Input arc parsing %s\n", arcIn );
	if( parse_arc( msgIn, &In ) != 0 || In.sum.qid == -1 )
	{
		_Xlogit( "et", "nll_arcOut_addLoccode: Error parsing %s\n", arcIn );
		return(1);
		if (In.num_phases > 0)
			free_phases(&In);
	}

/* Get the output archive file from disk
******************************/
	if (DEBUG_NLL) printf( "nll_mgr: reading output arcfile <%s>\n", arcOut); /*DEBUG*/
	fparc = fopen( arcOut, "r" );
	if ( fparc == (FILE *) NULL )
	{
	if (DEBUG_NLL) printf( "nll_mgr: Error opening output arcfile <%s>\n", arcOut); /*DEBUG*/
		_Xlogit( "et", "nll_mgr: Error opening output arcfile <%s>\n", arcOut );
		return(1);
	}
	nread = fread( msgOut, sizeof(char), (size_t)BufLen, fparc );
	fclose( fparc );
	if ( nread == 0 )
	{
		_Xlogit( "et", "nll_mgr: Error reading output arcfile <%s>\n", arcOut );
		return(1);
	}
	/* Parse message
	 ***************/
	Out.num_phases = 0;
	_Xlogit( "et", "nll_arcOut_addLoccode: Output arc parsing %s\n", arcOut );
	if( parse_arc_no_shdw( msgOut, &Out ) != 0 || Out.sum.qid == -1 )
	{
		_Xlogit( "et", "nll_arcOut_addLoccode: Error parsing %s\n", arcOut );
		return(1);
		if (Out.num_phases > 0)
			free_phases(&Out);
	}

/* Check the input arc as at least the same number of phases than the output arc
******************************************************************************/
	if (DEBUG_NLL) printf( "OutArcMessage as %d phases  and InArcMessage as %d phases\n", Out.num_phases, In.num_phases ); /*DEBUG*/
	if( Out.num_phases > In.num_phases )
		_Xlogit( "et", "nll_arcOut_addLoccode: more phases in output message (%d) than in input message (%d)\n", Out.num_phases, In.num_phases );

	for(i=0;i<Out.num_phases;i++)
	{
		for(j=0;j<In.num_phases;j++)
		{
			if( ! ( strcmp(Out.phases[i]->net,In.phases[j]->net) || strcmp(Out.phases[i]->site,In.phases[j]->site) || strcmp(Out.phases[i]->comp,In.phases[j]->comp) ) )
			{
				strcpy(Out.phases[i]->loc,In.phases[j]->loc);
				break;
			}
		}
		if( ( strcmp(Out.phases[i]->net,In.phases[j]->net) || strcmp(Out.phases[i]->site,In.phases[j]->site) || strcmp(Out.phases[i]->comp,In.phases[j]->comp) ) && j == In.num_phases )
			_Xlogit( "et", "Station not found : %s.%s.%s.%s\n", Out.phases[i]->net, Out.phases[i]->site, Out.phases[i]->comp, Out.phases[i]->loc );
	}

/* Rewrite the output arc message
*******************************/
	if( write_arc( msgOut, &Out ) != 0 )
	{
		_Xlogit( "et", "nll_arcOut_addLoccode: Error parsing %s\n", arcOut );
		if (Out.num_phases > 0)
			free_phases(&Out);
	}
/* Create temporary archive file.  This file will be
			overwritten the next time an event is processed.
*************************************************/
	if (DEBUG_NLL) printf( "nll_mgr: creating file <%s>\n", arcOutLC); /*DEBUG*/
	fptmp = fopen( arcOutLC, "w" );
	if ( fptmp == (FILE *) NULL )
	{
		ReportError( ERR_TMPFILE );
		_Xlogit( "et", "nll_mgr: Error creating file: %s\n", arcOutLC );
		return(1);
	}
	length = strlen( msgOut );
	if ( fwrite( msgOut, (size_t)1, (size_t)length, fptmp ) == 0 )
	{
		ReportError( ERR_TMPFILE );
		_Xlogit( "et", "nll_mgr: Error writing to file: %s\n", arcOutLC );
		fclose( fptmp );
		return(1);
	}
	fclose( fptmp );

	/* free allocated memory */
	free( msgIn );
	free( msgOut );

	return(0);
}


/******************************************************************
*                          ReportError()                         *
*         Send error message to the transport ring buffer.       *
*     This version doesn't allow an error string to be sent.     *
******************************************************************/

void ReportError( int errNum )
{
	MSG_LOGO       logo;
	unsigned short length;
	time_t         tstamp;
	char           errMsg[100];

	time( &tstamp );
	sprintf( errMsg, "%ld %d\n", (long)tstamp, errNum );

	logo.instid = InstId;
	logo.mod    = MyModId;
	logo.type   = TypeError;
	length      = strlen( errMsg );

	if (NO_EARTHWORM) {
		printf( "nll_mgr: No active EW: Bypassing sending of error msg <%s> to transport ring <%s>\n", errMsg, RingName);
	} else {
		if ( tport_putmsg( &Region, &logo, length, errMsg ) != PUT_OK )
			_Xlogit( "et", "nll_mgr: Error sending error msg to transport ring <%s>\n",
			       RingName);
	}
	return;
}


	/************************************************************
	*                      LogHypoError()                      *
	*                                                          *
	*  Convert a hypo return code to a text string and log it. *
	************************************************************/

void LogHypoError( int err )
{
	if ( err == -13 )
		_Xlogit( "", "nll_mgr: Skip phase card with wrong time.\n" );
	if ( err == -14 )
		_Xlogit( "", "nll_mgr: Bad format phase card, skip it.\n" );
	if ( err == -15 )
		_Xlogit( "", "nll_mgr: Skip phase card with unknown station.\n" );
	if ( err == -16 )
		_Xlogit( "", "nll_mgr: Too many unknown stations on ASCII phase cards.\n" );
	if ( err == -17 )
		_Xlogit( "", "nll_mgr: Too many phase cards in event.\n" );
	if ( err == -18 )
		_Xlogit( "", "nll_mgr: Bad ASCII terminator line.\n" );
	if ( err == -21 )
		_Xlogit( "", "nll_mgr: Pick from unknown station.\n" );
	if ( err == -22 )
		_Xlogit( "", "nll_mgr: Picks from too many stations.\n" );
	if ( err == -23 )
		_Xlogit( "", "nll_mgr: Coda from unknown station.\n" );
	if ( err == -24 )
		_Xlogit( "", "nll_mgr: Amp from unknown station.\n" );
	if ( err == -25 )
		_Xlogit( "", "nll_mgr: Wood-Anderson amp from unknown station.\n" );
	if ( err == -34 )
		_Xlogit( "", "nll_mgr: Too many stations in station file.\n" );
	if ( err == -35 )
		_Xlogit( "", "nll_mgr: Attempted to read attenuations before reading station file.\n" );
	if ( err == -36 )
		_Xlogit( "", "nll_mgr: Station attenuation file does not exist.\n" );
	if ( err == -37 )
		_Xlogit( "", "nll_mgr: Attempted to read FMAG corrections before reading station file.\n" );
	if ( err == -38 )
		_Xlogit( "", "nll_mgr: Station FMAG correction file does not exist.\n" );
	if ( err == -39 )
		_Xlogit( "", "hypoinv: Attempted to read XMAG corrections before reading station file.\n" );
	if ( err == -40 )
		_Xlogit( "", "nll_mgr: Station XMAX correction file does not exist.\n" );
	if ( err == -41 )
		_Xlogit( "", "nll_mgr: Maximum number of unknown stations exceeded.\n" );
	if ( err == -42 )
		_Xlogit( "", "nll_mgr: Attempted to read delays before reading station file.\n" );
	if ( err == -51 )
		_Xlogit( "", "nll_mgr: Not enough weighted readings for a solution.\n" );
	if ( err == -52 )
		_Xlogit( "", "nll_mgr: Event has fewer than 3 picks.\n" );
	if ( err == -53 )
		_Xlogit( "", "nll_mgr: Stop locating eq because it has been pushed far outside network.\n" );
	if ( err == -61 )
		_Xlogit( "", "nll_mgr: No phase file.\n" );
	if ( err == -62 )
		_Xlogit( "", "nll_mgr: No CUSP id file.\n" );
	if ( err == -63 )
		_Xlogit( "", "nll_mgr: Error in command arguments (detected by hycmd).\n" );
	if ( err == -64 )
		_Xlogit( "", "nll_mgr: Maximum depth of command files exceeded.\n" );
	if ( err == -65 )
		_Xlogit( "", "nll_mgr: Unknown command.\n" );
	if ( err == -66 )
		_Xlogit( "", "nll_mgr: Command file does not exist.\n" );
	if ( err == -67 )
		_Xlogit( "", "nll_mgr: Node model number too high.\n" );
	if ( err == -68 )
		_Xlogit( "", "nll_mgr: Crust file does not exist.\n" );
	if ( err == -69 )
		_Xlogit( "", "nll_mgr: Crust model number is out of range.\n" );
	if ( err == -70 )
		_Xlogit( "", "nll_mgr: Station file does not exist.\n" );
	if ( err == -71 )
		_Xlogit( "", "nll_mgr: Delay file does not exist.\n" );
	if ( err == -72 )
		_Xlogit( "", "nll_mgr: Crust binary snapshot file not found.\n" );
	if ( err == -73 )
		_Xlogit( "", "nll_mgr: Station binary snapshot file not found.\n" );
	if ( err == -74 )
		_Xlogit( "", "nll_mgr: Inconsistent use of station component letters.\n" );
	if ( err == -75 )
		_Xlogit( "", "nll_mgr: Error reading delay file.\n" );
	return;
}





	/*******************************************************
	*                   callHypo()                        *
	*                                                     *
	*  Calls the function Hypo and logs any errors        *
	*******************************************************/
int callHypo( char *cmd )
{
	int hypret;
	if (DEBUG_NLL) printf( "nll_mgr: entered callHypo()\n"); /*DEBUG*/
	Hypo( cmd, &hypret );
	if ( hypret != 0 )
	{
		_Xlogit( "e", "\nnll_mgr: Error occurred while executing <%s>.\n",
		       cmd );
		LogHypoError( hypret );
		return( -1 );
	}
	return( 0 );
}


	/*******************************************************
	*                       Hypo()                        *
	*                                                     *
	*  System dependent function to call NLLoc            *
	*******************************************************/

/* 2006/06/16 AJL - this function modified extensively */

void Hypo( char *cmd_file, int *iresr )
{
	char cmd_string[1024];

	if (DEBUG_NLL) printf( "nll_mgr: entered Hypo()\n"); /*DEBUG*/

/* The Windows NT version
**********************/
#ifdef _WINNT
/*
extern void __stdcall HYPO_EW( STRING *, int * );
strcpy( cmd_string.a, inMsg );
HYPO_EW( &cmd_string, iresr );
*/
#endif

/* The Solaris version
*******************/
#ifdef _SOLARIS
sprintf( cmd_string, "NLLoc %s", cmd_file );
if (DEBUG_NLL) printf( "nll_mgr: calling system command <%s>\n", cmd_string); /*DEBUG*/
*iresr = system(cmd_string);
if (DEBUG_NLL) printf( "nll_mgr: system command return <%d>\n", *iresr); /*DEBUG*/
#endif

/* The Linux version --- Modified by RSL 13/05/2010 - Just made it identical to the SOLARIS verion
*******************/
#ifdef _LINUX
sprintf( cmd_string, "NLLoc %s", cmd_file );
if (DEBUG_NLL) printf( "nll_mgr: calling system command <%s>\n", cmd_string); /*DEBUG*/
*iresr = system(cmd_string);
if (DEBUG_NLL) printf( "nll_mgr: system command return <%d>\n", *iresr); /*DEBUG*/
#endif

/* Print a newline to avoid overwriting the
last line written by hypoinverse, if any
****************************************/
putchar( '\n' );
return;
}

