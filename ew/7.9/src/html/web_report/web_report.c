
/*
 * web_report.c : Grabs final event messages and writes a html
 * file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <time_ew.h>

/* Functions in this source file
 *******************************/

void  web_report_config  ( char * );
void  web_report_lookup  ( void );

void  check_history( void );
void  update_history     ( char *summsg );
void  build_html_file    ( void );

void  web_report_status  ( unsigned char, short, char * );

static  SHM_INFO  Region;      /* shared memory region to use for i/o    */

#define   MAXLOGO   2
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;

#define BUF_SIZE MAX_BYTES_PER_EQ   /* define maximum size for event msg  */

/* Things to read from configuration file
 ****************************************/
static char    RingName[MAX_RING_STR];        /* name of transport ring for i/o     */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id       */
static int     LogSwitch;           /* 0 if no logfile should be written  */
static long    HeartBeatInterval;   /* seconds between heartbeats         */
static char    WebFileName[20];         /* temporary remote file name         */
static char    WebDir[50];        /* local directory to write tmp files */
static int     TotalLocations;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeH71Sum2K;

/* Error messages used by web_report
 *************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_LOCALFILE     3   /* error creating/writing local temp file */

static char  Text[150];        /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/

main( int argc, char **argv )
{
   static char  eqmsg[BUF_SIZE];  /* array to hold event message    */
   long         timeNow;          /* current time                   */
   long         timeLastBeat;     /* time last heartbeat was sent   */
   long         recsize;          /* size of retrieved message      */
   MSG_LOGO     reclogo;          /* logo of retrieved message      */
   int          res;


/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: web_report <configfile>\n" );
        exit( 0 );
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read the configuration file(s)
 ********************************/
   web_report_config( argv[1] );
      logit( "" , "web_report: Read command file <%s>\n", argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   web_report_lookup();

/* Change working directory to that for writing tmp files
 ********************************************************/
   if ( chdir_ew( WebDir ) == -1 )
   {
      fprintf( stderr, "%s: cannot change to directory <%s>\n",
               argv[0], WebDir );
      fprintf( stderr, "%s: please correct <WebDir> command in <%s>;",
               argv[0], argv[1] );
      fprintf( stderr, " exiting!\n" );
      exit( -1 );
   }

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, 256, LogSwitch );

/* Get our process ID
 **********************/
   if ((MyPid = getpid ()) == -1)
   {
      logit ("e", "web_report: Call to getpid failed. Exiting.\n");
      return (EW_FAILURE);
   }

/* Attach to Input/Output shared memory ring
 *******************************************/
   tport_attach( &Region, RingKey );
   logit( "", "web_report: Attached to public memory region %s: %d\n",
          RingName, RingKey );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

/* Flush the incomming transport ring
 *************************************/
   while(tport_getmsg (&Region, GetLogo, nLogo,
		       &reclogo, &recsize, eqmsg, sizeof(eqmsg)-1) != GET_NONE);

   check_history();
   build_html_file(); // build once to create webpages for first time


/*----------------- setup done; start main loop ----------------------*/

   while( tport_getflag(&Region) != TERMINATE  && 
          tport_getflag(&Region) != MyPid         )
   {
     /* send web_report's heartbeat
      *******************************/
        if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval )
        {
            timeLastBeat = timeNow;
            web_report_status( TypeHeartBeat, 0, "" );
        }

     /* Process all new hypoinverse archive msgs and hypo71 summary msgs
      ******************************************************************/
        do
        {
        /* Get the next message from shared memory
         *****************************************/
           res = tport_getmsg( &Region, GetLogo, nLogo,
                               &reclogo, &recsize, eqmsg, sizeof(eqmsg)-1 );

        /* Check return code; report errors if necessary
         ***********************************************/
           if( res != GET_OK )
           {
              if( res == GET_NONE )
              {
                 break;
              }
              else if( res == GET_TOOBIG )
              {
                 sprintf( Text,
                         "Retrieved msg[%ld] (i%u m%u t%u) too big for eqmsg[%d]",
                          recsize, reclogo.instid, reclogo.mod, reclogo.type,
                          sizeof(eqmsg)-1 );
                 web_report_status( TypeError, ERR_TOOBIG, Text );
                 continue;
              }
              else if( res == GET_MISS )
              {
                 sprintf( Text,
                         "Missed msg(s)  i%u m%u t%u  %s.",
                          reclogo.instid, reclogo.mod, reclogo.type, RingName );
                 web_report_status( TypeError, ERR_MISSMSG, Text );
              }
              else if( res == GET_NOTRACK )
              {
                 sprintf( Text,
                         "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                          reclogo.instid, reclogo.mod, reclogo.type );
                 web_report_status( TypeError, ERR_NOTRACK, Text );
              }
           }

        /* Process new message (res==GET_OK,GET_MISS,GET_NOTRACK)
         ********************************************************/
           eqmsg[recsize] = '\0';   /*null terminate the message*/
			if( reclogo.type == TypeH71Sum2K )
			{
				logit( "et", "web_report:Recieved Sum Message!\n" );

				check_history();
				logit( "et", "web_report:Checked history file!\n" );

				update_history(eqmsg);
				logit( "et", "web_report:Updated history file!\n" );

				build_html_file();
				logit( "et", "web_report:Updated web file!\n" );
			}

        } while( res != GET_NONE );  /*end of message-processing-loop */

        sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */
   }
/*-----------------------------end of main loop-------------------------------*/

/* Termination has been requested
 ********************************/
/* detach from shared memory */
   tport_detach( &Region );
/* write a termination msg to log file */
   logit( "t", "web_report: Termination requested; exiting!\n" );
   exit( 0 );
}

/*****************************************************************************
 *    process a Hypo71 summary message                 *
 *****************************************************************************/
#define B_DATE_H71SUM  0           /* yyyymmdd string */
#define L_DATE_H71SUM  8
#define B_HOUR_H71SUM  9           /* hhmm string */
#define L_HOUR_H71SUM  4
#define B_EID_H71SUM   91          /* last 2 digits of the event id */
#define L_EID_H71SUM   2


void check_history( void )
{
	FILE *hist_file;
	char HistFileName[200];
	int hist_file_present = 0;

	sprintf(HistFileName, "%sweb_report.hist", WebDir);

	if ((hist_file = fopen(HistFileName, "r")) != NULL)
	{
		fclose(hist_file);
		logit( "et", "\tFound History File: %s\n", HistFileName);
		return; /* file exists */
	}
	else if ((hist_file = fopen(HistFileName, "w")) == NULL)
	{
		logit( "et", "\tUnable to create History File: %s\n", HistFileName);
        return; /* can't create new file */
	}
	else
	{
		logit( "et", "\tCreated new empty History File: %s\n", HistFileName);
		fclose(hist_file);
		return; /* created new empty file */

	}
	
	return; /* shouldn't get here, if we do, big problems */
}


void update_history( char *summsg )
{
	FILE *hist_file;
	FILE *temp_file;
	char HistFileName[200];
	char TempFileName[200];
	char temp_char;
	
	int count = 0;
	int event_count = 0;

	sprintf(HistFileName, "%sweb_report.hist", WebDir);
	sprintf(TempFileName, "%sweb_report.tmp", WebDir);

	/* create new temp file from new location + old ones from */
	if ((temp_file = fopen(TempFileName, "wb")) == NULL)
	{
		logit( "et", "\tUnable to write temp History File: %s\n", TempFileName);
		return; /* can't write file */
	}
	else
	{
		logit( "et", "\tweb_report:About to write sum message to file\n");
		/* add new location (sum message) */

		temp_char = summsg[count];
		while (temp_char != '\0')
		{
			if (temp_char == 'W')
			{
				putc(' ', temp_file);
			}
			putc(temp_char, temp_file);
			count++;
			temp_char = summsg[count];
		}
		
		event_count++;
		
		logit( "et", "\tweb_report:About to get old events from history\n");
		/* get old locations from history */
		if ((hist_file = fopen(HistFileName, "r")) == NULL)
		{
			logit( "et", "\tUnable to read History File: %s\n", HistFileName);
		}
		else
		{
			temp_char = getc(hist_file);

			/* get locations while we are not at eof */
			while(temp_char != EOF)
			{			
				switch (temp_char)
				{
				case '\n':
					{
						putc(temp_char, temp_file);
						event_count++;
						break;
					}
				case EOF:
					{
						break;
					}
				default:
					{
						putc(temp_char, temp_file);
					}
				}
				/* halt if we have reached the limit of locations */
				if(event_count > (TotalLocations - 1))
				{
					break;
				}

				temp_char = getc(hist_file);
	        } 
			fclose(hist_file);
		}
		fclose(temp_file);
	}

	logit( "et", "\tweb_report:About to get copy tempfile into history file\n");
	/* copy temp file into history file */
	if ((hist_file = fopen(HistFileName, "wb")) == NULL)
	{
		logit( "et", "\tUnable to write History File: %s\n", TempFileName);
	}
	else
	{
		if ((temp_file = fopen(TempFileName, "r")) == NULL)
		{
			logit( "et", "\tUnable to read temp History File: %s\n", HistFileName);
			return; /* can't read new file */
		}
		else
		{
			/* copy temp file into history file */
			temp_char = getc(temp_file);

			/* get locations while we are not at eof */
			while(temp_char != EOF)
			{			
				putc(temp_char, hist_file);
				temp_char = getc(temp_file);			
			}
			fclose(temp_file);
		}
		fclose(hist_file);
	}

	logit( "et", "\tHistory File updated: %s\n", HistFileName);
	return; /* successfully created file */
}


void build_html_file( void )
{
	FILE *hist_file;
	FILE *web_file;
	FILE *temp_file;

	char HistFileName[50];
	char TempFileName[50];
	char TheWebFileName[50];
	char temp_char;

	static time_t now;
	static struct tm res;
	char date[10];
	char time_stamp[50];

	sprintf(HistFileName, "%sweb_report.hist", WebDir);
	sprintf(TheWebFileName, "%s%s", WebDir, WebFileName);

	/* create new temp file from new location + old ones from */
	if ((web_file = fopen(TheWebFileName, "wb")) == NULL)
	{
		logit( "et", "\tUnable to write html File: %s\n", WebFileName);
	}
	else
	{
		sprintf(TempFileName, "%sindexa.html", WebDir);

		if ((temp_file = fopen(TempFileName, "r")) == NULL)
		{
			/* Create html header */
			logit( "et", "\tNo indexa.html, createing placeholder code\n");
			fprintf(web_file, "<HTML><HEAD><TITLE>\nAutomatic Earthquake Locations\n</TITLE></HEAD>\n");
			fprintf(web_file, "<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>\n");
			fprintf(web_file, "<A NAME=\"top\"></A><CENTER><H2>\n");
			fprintf(web_file, "<FONT COLOR=red>Automatic Earthquake Locations</FONT></H2><br>\n\n");

		}
		else
		{
			logit( "et", "\tindexa.html found\n");
			/* copy temp file into history file */

			temp_char = getc(temp_file);
			/* get locations while we are not at eof */

			while(temp_char != EOF)
			{			
				putc(temp_char, web_file);
				temp_char = getc(temp_file);			
			}

			fclose(temp_file);
		}
		
		if ((hist_file = fopen(HistFileName, "r")) == NULL)
		{
			/* Unable to open a history file */
			logit( "et", "\tUnable to open history file\n");
			
			fprintf(web_file, "<TABLE cellSpacing=5 width=\"90%%\" border=0>\n");
			fprintf(web_file, "<TR><TD><HR></TD></TR></TABLE>\n");
			fprintf(web_file, "<TABLE cellSpacing=5 width=\"90%%\" border=0>\n");
			fprintf(web_file, "  <TR>\n");
			fprintf(web_file, "    <TD><TT><B>\n      <BR>");
			fprintf(web_file, "    History File not found<BR>");
			fprintf(web_file, "    </TT></TD>\n");
			fprintf(web_file, "  </TR>\n");
			fprintf(web_file, "</TABLE>\n");
			fprintf(web_file, "<TABLE cellSpacing=5 width=\"90%%\" border=0>\n");
			fprintf(web_file, "<TR><TD><HR></TD></TR></TABLE>\n");

		}
		else
		{
			/* generate html list */
			logit( "et", "\tUpdating earthquake list\n");
			fprintf(web_file, "<TABLE cellSpacing=5 width=\"90%%\" border=0>\n");
			fprintf(web_file, "<TR><TD><HR></TD></TR></TABLE>\n");
			fprintf(web_file, "<TABLE cellSpacing=5 width=\"90%%\" border=0>\n");
			fprintf(web_file, "  <TR>\n");
			fprintf(web_file, "    <TD><TT>");
			fprintf(web_file, "      <b>YearMoDy&nbsp;HrMn&nbsp;Sec&nbsp;&nbsp;&nbsp;Latitude&nbsp;Longitude&nbsp;&nbsp;Depth&nbsp;&nbsp;&nbsp;Mag&nbsp;&nbsp;No&nbsp;Gap&nbsp;Dmin&nbsp;RMS&nbsp;&nbsp;ERH&nbsp;&nbsp;ERZ&nbsp;&nbsp;Q&nbsp;DataSrc&nbsp;EventID</B><BR>\n");

			temp_char = getc(hist_file);

			if (temp_char == EOF)
			{
				logit( "e", "History File Empty\n");
				
				fprintf(web_file, "     No Seismic events yet.<BR>");
				
			}

			while (temp_char != EOF)
			{
				switch (temp_char)
				{
					case '\n':
					{	
						fprintf(web_file, "\n     <BR>");
						break;
					}
					case ' ':
					{
						fprintf(web_file, "&nbsp;");
						break;
					}
			
					default:
						putc(temp_char, web_file);

				}

				temp_char = getc(hist_file);
			}

			time( &now );
            gmtime_ew( &now, &res );

            sprintf (date, "%02d/%02d/%4d", (res.tm_mon + 1), res.tm_mday, (res.tm_year + TM_YEAR_CORR));

			sprintf( time_stamp, "%s at %02d:%02d:%02d UTC",
               date, res.tm_hour, res.tm_min, res.tm_sec );

			fprintf(web_file, "      </TT><B><font size=-1>List of the last %d seismic events, if available, automatically located by the EARTHWORM %s system, at installation %s.<BR><font size=-2>Generated by web_report on system %s.&nbsp;&nbsp;Last updated %s.</font></font> \n", TotalLocations, getenv( "EW_VERSION" ), getenv( "EW_INSTALLATION" ),/*(int)InstId,*/ getenv( "SYS_NAME" ), time_stamp);
			fprintf(web_file, "    </TD>\n");
			fprintf(web_file, "  </TR>\n");
			fprintf(web_file, "</TABLE>\n");

			fprintf(web_file, "<TABLE cellSpacing=5 width=\"90%%\" border=0>\n");
			fprintf(web_file, "<TR><TD><HR></TD></TR></TABLE>\n");

			fclose(hist_file);
		}		

		sprintf(TempFileName, "%sindexb.html", WebDir);

		if ((temp_file = fopen(TempFileName, "r")) == NULL)
		{
			/* Create html footer */
			logit( "et", "\tNo indexb.html, createing placeholder code\n");
			fprintf(web_file, "<P><HR><font color=red></font>\n");
			fprintf(web_file, "<P><A HREF=\"#top\">Top of this page\n</A>\n");
			fprintf(web_file, "<CENTER><font size=-2>\n\n Generated by web_report\n\n</font>");   
			fprintf(web_file, "</BODY></HTML>\n");
		}
		else
		{
			logit( "et", "\tindexb.html found\n");
			/* copy temp file into history file */
			
			temp_char = getc(temp_file);
			/* get locations while we are not at eof */
			while(temp_char != EOF)
			{			
				putc(temp_char, web_file);
				temp_char = getc(temp_file);			
			}

			fclose(temp_file);
		}

		fclose(web_file);
	}
	logit( "et", "\tWeb File updated: %s\n", TheWebFileName);
	return;
}



/******************************************************************************
 *  web_report_config() processes command file(s) using kom.c functions;    *
 *                       exits if any errors are encountered.                 *
 ******************************************************************************/
void web_report_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[15];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 8;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit ("e",
                "web_report: Error opening command file <%s>; exiting!\n",
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
                  logit ("e",
                          "web_report: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInterval") ) {
                HeartBeatInterval = k_long();
                init[3] = 1;
            }


         /* Enter installation & module to get event messages from
          ********************************************************/
  /*4*/     else if( k_its("GetEventsFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit ("e",
                            "web_report: Too many <GetEventsFrom> commands in <%s>",
                             configfile );
                    logit ("e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit ("e",
                               "web_report: Invalid installation name <%s>", str );
                       logit ("e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit ("e",
                               "web_report: Invalid module name <%s>", str );
                       logit ("e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_H71SUM2K", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit ("e",
                               "web_report: Invalid message type <TYPE_H71SUM2K>" );
                    logit ("e", "; exiting!\n" );
                    exit( -1 );
                }
                nLogo  += 2;
                init[4] = 1;
            }


         /* Enter name of local directory to write tmp files to
          *****************************************************/
  /*5*/     else if( k_its("WebDir") ) {
                str = k_str();
                if(str) strcpy( WebDir, str );
                init[5] = 1;
            }


         /* Temporary remote name to use for copying file
          ***********************************************/
  /*6*/    else if( k_its("WebFileName") ) {
                str = k_str();
                if(str) {
                   if( strlen(str) >= sizeof(WebFileName) ) {
                      logit ("e",
                              "web_report: WebFileName <%s> too long; ",
                               str );
                      logit ("e", "max length:%d; exiting!\n",
                              (int)(sizeof(WebFileName)-1) );
                      exit(-1);
                   }
                   strcpy( WebFileName, str );
                }
                init[6] = 1;
            }


  /*7*/     else if( k_its("TotalLocations") ) {
                TotalLocations = k_int();
                init[7] = 1;
			}

        /* Unknown command
         *****************/
            else 
			{
                logit ("e", "web_report: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               fprintf( stderr,
                       "web_report: Bad <%s> command in <%s>; exiting!\n",
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
   if ( nmiss ) {
       logit ("e", "web_report: ERROR, no " );
       if ( !init[0] )  logit ("e", "<LogFile> "           );
       if ( !init[1] )  logit ("e", "<MyModuleId> "        );
       if ( !init[2] )  logit ("e", "<RingName> "          );
       if ( !init[3] )  logit ("e", "<HeartBeatInterval> " );
       if ( !init[4] )  logit ("e", "<GetEventsFrom> "     );
       if ( !init[5] )  logit ("e", "<WebDir> "          );
       if ( !init[6] )  logit ("e", "<WebFileName> "     );
	   if ( !init[7] )  logit ("e", "<TotalLocations> "     );
       logit ("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/*******************************************************************************
 *  web_report_lookup( )   Look up important info from earthworm.h tables    *
 *******************************************************************************/
void web_report_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        fprintf( stderr,
                "web_report:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "web_report: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "web_report: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "web_report: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "web_report: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum2K ) != 0 ) {
      fprintf( stderr,
              "web_report: Invalid message type <TYPE_H71SUM2K>; exiting!\n" );
      exit( -1 );
   }
   return;
}

/*****************************************************************************
 * web_report_status() builds a heartbeat or error message & puts it into  *
 *                       shared memory.  Writes errors to log file & screen. *
 *****************************************************************************/
void web_report_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   long        t;

/* Build the message
 *******************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n\0", t, MyPid);
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n\0", t, ierr, note);
        logit( "et", "web_report: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","web_report:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","web_report:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

