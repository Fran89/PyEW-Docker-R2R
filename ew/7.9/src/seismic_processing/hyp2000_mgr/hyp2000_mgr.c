
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hyp2000_mgr.c 6379 2015-06-12 01:20:24Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2010/04/27 20:03:18  paulf
 *     added _GFORTRAN compile time flag for linking for that compiler versus g77
 *
 *     Revision 1.8  2008/12/22 20:58:07  paulf
 *     added MACOSX INTEL defines for compiling with gfortran
 *
 *     Revision 1.7  2007/12/14 21:47:58  paulf
 *     changes for Linux and g77 compiling
 *
 *     Revision 1.6  2007/01/19 16:08:37  paulf
 *     patched WINNT for older compiler
 *
 *     Revision 1.5  2006/10/16 17:13:51  paulf
 *     intel 9.1 fixes
 *
 *     Revision 1.4  2006/06/06 21:19:33  paulf
 *     upgraded for intel9
 *
 *     Revision 1.1.1.1  2005/07/14 20:10:34  paulf
 *     Local ISTI CVS copy of EW v6.3
 *
 *     Revision 1.3  2002/06/05 15:30:48  patton
 *     Made logit changes.
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
    *                          hyp2000_mgr.c                          *
    *                                                                 *
    *     C program for managing hyp2000.                             *
    *     Hypo_mgr calls the FORTRAN subroutine "hypo_ew".            *
    *******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <read_arc.h>

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

int  DoNotRemoveArc = 0;	/* do not remove the arc in and out files from run to run */
char * PRTdir = NULL;		/* the directory where PRT files are written, if not NULL */
char * PRTfile = NULL;		/* the dir+file holder */

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
static char arcIn[30];          /* Archive file sent to hypo_ew            */
static char arcOut[30];         /* Archive file output by hypo_ew          */
static char sumName[30];        /* Summary file output by hypo_ew          */

/* Error to send to statmgr.  Errors 100-149 are reserved for hyp2000_mgr.
  (must not overlap with error numbers of other links in the sausage.)
 ************************************************************************/
#define ERR_TOOBIG        101   /* Got a msg from pipe too big for target  */
#define ERR_TMPFILE       102   /* Error writing hypoinverse input file    */
#define ERR_ARCFILE       103   /* Error reading hypoinverse archive file  */
#define ERR_ARC2RING      104   /* Trouble putting arcmsg in memory ring   */
#define ERR_SUM2RING      105   /* Trouble putting summsg in memory ring   */
#define ERR_MSG2RING      106   /* Trouble passing a msg from pipe to ring */
#define ERR_PIPEPUT       107   /* Trouble writing to pipe                 */

#define VERSION_NUMBER "1.46 2015/06/11 hypoinverse-1.41"


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
   char       *configFile;
   char       *commandFile;
   char       *phaseFile;	/* for debugging purposes */
   int         type;
   char        hypo_msg[80];
   char        qid_str[11];	/* more than big enough, qid's can be 10 chars */
   long	       qid;

/* Check command line arguments
   ****************************/
   if ( argc < 3 )
   {
      fprintf( stderr, "Usage: hyp2000_mgr <config file> <command file> [phs_file]\n" );
      fprintf( stderr, "Version: %s\n", VERSION_NUMBER);
      fprintf( stderr, "Note: phs_file input is a debug command line mode\n");
      return -1;
   }
   configFile  = argv[1];
   commandFile = argv[2];
   phaseFile = NULL;
   if (argc == 4) 
   {
     phaseFile=argv[3];
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read in the configuration file
   ******************************/
   GetConfig( configFile );
   logit( "" , "hyp2000_mgr: Version %s Read command file <%s>\n", VERSION_NUMBER, configFile );

/* Make up the names of the temporary arc files
   ********************************************/
   sprintf( arcIn,  "hypoMgrArcIn%u",  MyModId );
   sprintf( arcOut, "hypoMgrArcOut%u", MyModId );
   strcpy( sumName, "none" );

/* Look up important info from earthworm.h tables
 ************************************************/
   LookUp();

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, 512, LogSwitch );

/* Attach to the transport ring (Must already exist)
   *************************************************/
   tport_attach( &Region, RingKey );

/* Read in station & model info via command file
   *********************************************/
   sprintf( hypo_msg, "@%s", commandFile );
   if( callHypo( hypo_msg ) != 0 ) return -1;       /* load startup file */
   logit( "t", "hyp2000_mgr: Version %s: Initialized hypoinverse with file <%s>\n",
           VERSION_NUMBER, commandFile );

/* Hard-wire input phase file name & format
   ****************************************/
   if( callHypo( "200 T 1900 0" ) != 0 ) return -1; /* force Y2K formats  */
   if( callHypo( "COP 5" )        != 0 ) return -1; /* input arc w/shadow */
   if (phaseFile)
   {
     sprintf( hypo_msg, "PHS '%s'", phaseFile );
   }
   else
   {
     sprintf( hypo_msg, "PHS '%s'", arcIn );
   }
   if( callHypo( hypo_msg )       != 0 ) return -1; /* input phase file   */

/* Hard-wire output file names & formats
   *************************************/
   if( callHypo( "CAR 3" )        != 0 ) return -1; /* output arc w/shadow*/
   sprintf( hypo_msg, "ARC '%s'", arcOut );
   if( callHypo( hypo_msg )       != 0 ) return -1; /* archive output file*/
   sprintf( hypo_msg, "SUM '%s'", sumName );
   if( callHypo( hypo_msg )       != 0 ) return -1; /* summary output file*/

/* Get a message from the pipe
   ***************************/
   while ( 1 )
   {
      if (phaseFile)
      {
         /* no need to read in a message, we have the phase file already  */
         type=TypeHyp2000Arc;
      }
      else
      {
         rc = pipe_get( msg, BufLen, &type );
         if ( rc < 0 )
         {
            if ( rc == -1 )
            {
               ReportError( ERR_TOOBIG );
               logit( "et", "hyp2000_mgr: Message in pipe too big for buffer. Exiting.\n" );
            }
            else if ( rc == -2 )
               logit( "et", "hyp2000_mgr: <null> on pipe_get. Exiting.\n" );
            else if ( rc == -3 )
               logit( "et", "hyp2000_mgr: EOF on pipe_get. Exiting.\n" );
            break;
         }
      }

/* Stop program if this is a "kill" message
   ****************************************/
      if ( type == (int) TypeKill )
      {
         logit( "t", "hyp2000_mgr: Termination requested. Exiting.\n" );
         break;
      }

/* This is an archive file event
   *****************************/
      if ( type == (int) TypeHyp2000Arc )
      {

         if(!phaseFile) 
         {
            /* Create temporary archive file.  This file will be
               overwritten the next time an event is processed.
               *************************************************/
            fptmp = fopen( arcIn, "w" );
            if ( fptmp == (FILE *) NULL )
            {
               ReportError( ERR_TMPFILE );
               logit( "et", "hyp2000_mgr: Error creating file: %s\n", arcIn );
               continue;
            }
            length = strlen( msg );
            if ( fwrite( msg, (size_t)1, (size_t)length, fptmp ) == 0 )
            {
               ReportError( ERR_TMPFILE );
               logit( "et", "hyp2000_mgr: Error writing to file: %s\n", arcIn );
               fclose( fptmp );
               continue;
            }
            fclose( fptmp );
         }
         if(PRTdir) 
         {
		strcpy(PRTfile, PRTdir);
		strcat(PRTfile, "/");
                strncat(PRTfile, msg, 14); /* first 14 chars have binder OT */
		strcat(PRTfile, ".");
  		qid_str[0] = 0;
                strncat(qid_str, msg+136, 10);
		qid= atoi(qid_str);
		sprintf(qid_str, "%010ld", qid);
		strcat(PRTfile, qid_str);
		strcat(PRTfile, ".prt");
                sprintf( hypo_msg, "PRT '%s'", PRTfile );
                if( callHypo( hypo_msg )       != 0 ) return -1; /* named PRT file   */
         }

/* Locate the event
   ****************/
         if( callHypo( "LOC" ) != 0 ) {
            logit( "e", "hyp2000_mgr: Nonfatal error(s) locating event.\n" );
         }
         if(!phaseFile && !DoNotRemoveArc) 
         {
            remove( arcIn );
         }

/* Get the archive file from disk
   ******************************/
         fparc = fopen( arcOut, "r" );
         if ( fparc == (FILE *) NULL )
         {
            ReportError( ERR_ARCFILE );
            logit( "et", "hyp2000_mgr: Error opening arcfile <%s>\n", arcOut );
            continue;
         }
         nread = fread( msg, sizeof(char), (size_t)BufLen, fparc );
         fclose( fparc );
         if ( nread == 0 )
         {
            ReportError( ERR_ARCFILE );
            logit( "et", "hyp2000_mgr: Error reading arcfile <%s>\n", arcOut );
            continue;
         }
         remove( arcOut );

/* Send the archive file to the transport ring
   *******************************************/
         logo.instid = InstId;
         logo.mod    = MyModId;
         logo.type   = TypeHyp2000Arc;
         if ( tport_putmsg( &Region, &logo, (long) nread, msg ) != PUT_OK )
         {
            ReportError( ERR_ARC2RING );
            logit( "et",
                   "hyp2000_mgr: Error sending archive msg to transport ring.\n" );
         }

/* Get the summary line from archive message (first line)
   ******************************************************/
         sscanf( msg, "%[^\n]", sumCard );
         /*logit("e", "1st line of arcmsg:\n%s\n\n", sumCard );*/ /*DEBUG*/

/* Convert the hypoinverse summary to hypo71
   summary and add the source code to it
   *****************************************/
         hyp2000sum_hypo71sum2k( sumCard );
         sumCard[81] = SourceCode;

/* Print a status line to stderr
   *****************************/
         PrintSumLine( sumCard );

/* Send the summary file to the transport ring
   *******************************************/
         logo.instid = InstId;
         logo.mod    = MyModId;
         logo.type   = TypeH71Sum2K;
         if ( tport_putmsg( &Region, &logo, strlen(sumCard), sumCard ) != PUT_OK )
         {
            ReportError( ERR_SUM2RING );
            logit( "et",
               "hyp2000_mgr: Error sending summary line to transport ring.\n" );
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
         if ( tport_putmsg( &Region, &logo, strlen(msg), msg ) != PUT_OK )
         {
            ReportError( ERR_MSG2RING );
            logit( "et",
                   "hyp2000_mgr: Error passing msg (type %d) to transport ring.\n",
                    (int) type );
         }
      }
      if  (phaseFile)
      {
         fprintf(stderr, "hyp2000_mgr in single phase file debug mode, exiting cleanly\n");
         break;
      }
   }

/* Detach from the transport ring and exit
   ***************************************/
   tport_detach( &Region );

   return 0;
}


  /*******************************************************************
   *                           GetConfig()                           *
   *                                                                 *
   *  Processes command file using kom.c functions.                  *
   *  Exits if any errors are encountered.                           *
   *******************************************************************/
#define ncommand 4        /* # of required commands you expect to process   */
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
        logit( "e",
                "hyp2000_mgr: Error opening command file <%s>. Exiting.\n",
                 configfile );
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
                          "hyp2000_mgr: Error opening command file <%s>. Exiting.\n",
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
                      logit( "e",
                             "hyp2000_mgr: Invalid module name <%s>. Exiting.\n",
                              str );
                      exit( -1 );
                   }
                }
                init[2] = 1;
            }
   /*3*/    else if( k_its( "SourceCode" ) ) {
                if ( (str=k_str()) != NULL ) {
                   SourceCode = str[0];
                }
                init[3] = 1;
            }
   /*opt*/  else if( k_its( "DoNotRemoveArc" ) ) {
                DoNotRemoveArc = 1;
            }
   /*opt*/  else if( k_its( "SeparatePRTdir" ) ) {
                PRTdir = strdup(k_str());
		if (strlen(PRTdir) > 40) {
                      logit( "e", "hyp2000_mgr: PRTdir string len too large. Must be less than 40 chars. Hint: Use a relative path. Exiting.\n");
                      exit( -1 );
		}
		if ((PRTfile = calloc(1,strlen(PRTdir)+51)) == NULL) {
                      logit( "e", "hyp2000_mgr: Could not calloc() space for PRT filename array. Exiting.\n");
                      exit( -1 );
                }
	    }
            else {
                logit( "e", "hyp2000_mgr: <%s> unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() )
            {
               logit( "e", "hyp2000_mgr: Bad <%s> command in <%s>; \n",
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
       logit( "e", "hyp2000_mgr: ERROR, no " );
       if ( !init[0] )  logit( "e", "<RingName> "   );
       if ( !init[1] )  logit( "e", "<LogFile> "    );
       if ( !init[2] )  logit( "e", "<MyModuleId> " );
       if ( !init[3] )  logit( "e", "<SourceCode> " );
       logit( "e", "command(s) in <%s>. Exiting.\n", configfile );
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
              "hyp2000_mgr: Invalid ring name <%s>. Exiting.\n", RingName );
      exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      fprintf( stderr,
              "hyp2000_mgr: error getting local installation id. Exiting.\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      fprintf( stderr,
              "hyp2000_mgr: Invalid message type <TYPE_ERROR>. Exiting.\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 )
   {
      fprintf( stderr,
              "hyp2000_mgr: Invalid message type <TYPE_HYP2000ARC>. Exiting.\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum2K ) != 0 )
   {
      fprintf( stderr,
              "hyp2000_mgr: Invalid message type <TYPE_H71SUM2K>. Exiting.\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 )
   {
      fprintf( stderr,
              "hyp2000_mgr: Invalid message type <TYPE_KILL>. Exiting.\n" );
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
      logit( "", "%s", sumCard );
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
   char c_depth[8];
   float rms,erh,erz,dmin,depth;
   int no,gap;
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

   strncpy(c_depth, hinvsum+32, 5);
   c_depth[5] = 0;
   depth = atoi(c_depth);
   depth = depth/100.;
   sprintf(c_depth, " %5.2f", depth);
   strncpy( h71sum+40, c_depth, 6); 
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

   h71sum[81] = ComputeAverageQuality(rms, erh, erz, depth, dmin, no, gap);

/* Copy converted summary card back to caller's address
   ****************************************************/
   strcpy( sumcard, h71sum+1 );
   return;
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
   sprintf( errMsg, "%d %d\n", (int) tstamp, errNum );

   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = TypeError;
   length      = strlen( errMsg );

   if ( tport_putmsg( &Region, &logo, length, errMsg ) != PUT_OK )
      logit( "et", "hyp2000_mgr: Error sending error msg to transport region <%s>\n",
               RingName);
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
      logit( "", "hyp2000_mgr: Skip phase card with wrong time.\n" );
   if ( err == -14 )
      logit( "", "hyp2000_mgr: Bad format phase card, skip it.\n" );
   if ( err == -15 )
      logit( "", "hyp2000_mgr: Skip phase card with unknown station.\n" );
   if ( err == -16 )
      logit( "", "hyp2000_mgr: Too many unknown stations on ASCII phase cards.\n" );
   if ( err == -17 )
      logit( "", "hyp2000_mgr: Too many phase cards in event.\n" );
   if ( err == -18 )
      logit( "", "hyp2000_mgr: Bad ASCII terminator line.\n" );
   if ( err == -21 )
      logit( "", "hyp2000_mgr: Pick from unknown station.\n" );
   if ( err == -22 )
      logit( "", "hyp2000_mgr: Picks from too many stations.\n" );
   if ( err == -23 )
      logit( "", "hyp2000_mgr: Coda from unknown station.\n" );
   if ( err == -24 )
      logit( "", "hyp2000_mgr: Amp from unknown station.\n" );
   if ( err == -25 )
      logit( "", "hyp2000_mgr: Wood-Anderson amp from unknown station.\n" );
   if ( err == -34 )
      logit( "", "hyp2000_mgr: Too many stations in station file.\n" );
   if ( err == -35 )
      logit( "", "hyp2000_mgr: Attempted to read attenuations before reading station file.\n" );
   if ( err == -36 )
      logit( "", "hyp2000_mgr: Station attenuation file does not exist.\n" );
   if ( err == -37 )
      logit( "", "hyp2000_mgr: Attempted to read FMAG corrections before reading station file.\n" );
   if ( err == -38 )
      logit( "", "hyp2000_mgr: Station FMAG correction file does not exist.\n" );
   if ( err == -39 )
      logit( "", "hypoinv: Attempted to read XMAG corrections before reading station file.\n" );
   if ( err == -40 )
      logit( "", "hyp2000_mgr: Station XMAX correction file does not exist.\n" );
   if ( err == -41 )
      logit( "", "hyp2000_mgr: Maximum number of unknown stations exceeded.\n" );
   if ( err == -42 )
      logit( "", "hyp2000_mgr: Attempted to read delays before reading station file.\n" );
   if ( err == -51 )
      logit( "", "hyp2000_mgr: Not enough weighted readings for a solution.\n" );
   if ( err == -52 )
      logit( "", "hyp2000_mgr: Event has fewer than 3 picks.\n" );
   if ( err == -53 )
      logit( "", "hyp2000_mgr: Stop locating eq because it has been pushed far outside network.\n" );
   if ( err == -61 )
      logit( "", "hyp2000_mgr: No phase file.\n" );
   if ( err == -62 )
      logit( "", "hyp2000_mgr: No CUSP id file.\n" );
   if ( err == -63 )
      logit( "", "hyp2000_mgr: Error in command arguments (detected by hycmd).\n" );
   if ( err == -64 )
      logit( "", "hyp2000_mgr: Maximum depth of command files exceeded.\n" );
   if ( err == -65 )
      logit( "", "hyp2000_mgr: Unknown command.\n" );
   if ( err == -66 )
      logit( "", "hyp2000_mgr: Command file does not exist.\n" );
   if ( err == -67 )
      logit( "", "hyp2000_mgr: Node model number too high.\n" );
   if ( err == -68 )
      logit( "", "hyp2000_mgr: Crust file does not exist.\n" );
   if ( err == -69 )
      logit( "", "hyp2000_mgr: Crust model number is out of range.\n" );
   if ( err == -70 )
      logit( "", "hyp2000_mgr: Station file does not exist.\n" );
   if ( err == -71 )
      logit( "", "hyp2000_mgr: Delay file does not exist.\n" );
   if ( err == -72 )
      logit( "", "hyp2000_mgr: Crust binary snapshot file not found.\n" );
   if ( err == -73 )
      logit( "", "hyp2000_mgr: Station binary snapshot file not found.\n" );
   if ( err == -74 )
      logit( "", "hyp2000_mgr: Inconsistent use of station component letters.\n" );
   if ( err == -75 )
      logit( "", "hyp2000_mgr: Error reading delay file.\n" );
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
   Hypo( cmd, &hypret );
   if ( hypret != 1 )
   {
      logit( "e", "\nhyp2000_mgr: Error occurred while executing <%s>. Return i s %d\n",
             cmd , hypret);
      LogHypoError( hypret );
      return( -1 );
   }    
   return( 0 );
}


        /*******************************************************
         *                       Hypo()                        *
         *                                                     *
         *  System dependent function to call the hypo_ew      *
         *  FORTRAN subroutine.                                *
         *******************************************************/

typedef struct {
   char a[80];
} STRING;

void Hypo( char *inMsg, int *iresr )
{

#if defined(_MACOSX) && defined(_INTEL) && defined(_GFORTRAN) || defined(_LINUX) || defined(_SOLARIS)
   /* added a func prototype to Fortran call */
   int hypo_ew_( char *, int*, int); 
#endif

   STRING outMsg;
   int len;

/* The Windows NT version
   **********************/
#if defined(_WINNT_INTEL9)
   extern void __stdcall hypo_ew( STRING, int * );
   strcpy( outMsg.a, inMsg );
   *iresr=0;
   hypo_ew( outMsg, iresr );
/*    fprintf(stderr, "DEBUG: after fortran call iresr=%d\n", *iresr); */
#elif defined(_WINNT)
   extern void __stdcall hypo_ew( STRING *, int * );
   strcpy( outMsg.a, inMsg );
   HYPO_EW( &outMsg, iresr );
#endif


#if defined(_MACOSX) && defined(_SPARC)
   /* extern hypo_ew_( char *, int * , int);  */
   strcpy( outMsg.a, inMsg );
   len=strlen(outMsg.a);
   hypo_ew__( (char *) outMsg.a, iresr, len); 
#endif

/* the gfortran version, tested on Mac OS X and Linux with gfortran and Forte F77 on Solaris */
#if defined(_MACOSX) && defined(_INTEL) && defined(_GFORTRAN) || defined(_LINUX) || defined(_SOLARIS)
   strcpy( outMsg.a, inMsg );
   len=strlen(outMsg.a);
   hypo_ew_( (char *) outMsg.a, iresr, len); 
#endif

/* Print a newline to avoid overwriting the
   last line written by hypoinverse, if any
   ****************************************/
   putchar( '\n' );
   return;
}

