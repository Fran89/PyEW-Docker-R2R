
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hyp71_mgr.c,v 1.4.2 2016/09/30 00:00:00  jmsaurel Exp $
 *
 *    Revision history:
 *     $Log: hyp71_mgr.c,v $
 *     Revision 1.4.2 2016/09/30 00:00:00  jmsaurel
 *      Added more debugs and verification for files operations
 *
 *     Revision 1.4.1 2011/08/19 00:00:00  jmsaurel
 *      To limit the number of station cards, just print the stations for which there are picks
 *
 *     Revision 1.4 2010/12/08 00:00:00  jmsaurel
 *      Hypo71 trial-depth search algorithm is now always selected but the list of trial_depth
 *	 is read from config file
 *	Changed some stuff in config file reading (GetConfig) to have more optionnal parameters
 *	Added a testing mode that allows to run the module independently, reading a file containing
 *	 a hyp2000_arc message. Nothing is read or send to any RingBuffer. Everything else behaves the same
 *
 *     Revision 1.3.3  2010/04/23 00:00:00  jmsaurel
 *      Minor changes regarding Magnitude output format
 *
 *     Revision 1.3.2  2010/01/26 00:00:00  jmsaurel
 *      DEBUG mode now is settable from config file, no more needs to recompile
 *
 *     Revision 1.3.1  2009/12/18 00:00:00  jmsaurel
 *      Big bug with TrialDepth corrected (at each new arc message, TrialDepth was multiplied by 2)
 *
 *     Revision 1.3  2009/12/17 00:00:00  jmsaurel
 *      Hypo71 trial-depth search algorithm is now possible uncommenting a define statement in this file
 *      DEBUG standalone mode is now possible by uncommenting a define statement in this file
 *      Made a separate routine to run Hypo71
 *
 *     Revision 1.2  2009/05/29 00:00:00  jmsaurel
 *     Suppressed Hypo71 trial-depth search algorithm
 *      New HEAD cards parameters (cf .d parameter file) allow much better convergence
 *
 *     Revision 1.1  2008/11/01 00:00:00  jmsaurel
 *     Initial revision
 *
 */


   /*******************************************************************
    *                          hyp71_mgr.c                            *
    *                                                                 *
    *     C program for managing hyp71.                               *
    *     Code based on hyp200_mgr.                                   *
    *******************************************************************/

#include "hyp71_mgr.h"

const int BufLen = MAX_BYTES_PER_EQ;      /* Message buffer length */

/* Function prototypes
 *********************/
void GetConfig( char * );
void GetLocalmagConfig( char * );
void LookUp( void );
void hyp2000sum_hypo71sum2k( char * );
void ReportError( int );
void PrintSumLine( char * );
int lay_model_com( void );
int hypo71_com( void );
int lay_model(double , double );

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
int Debug = 0;
char HYPO71PC_bin[MAX_STR];
char Hypo2000_Test_File[MAX_STR];

/* Main function
 ***************/

int main( int argc, char *argv[] )
{
	static char	msg[MAX_BYTES_PER_EQ];  /* Message buffer (for .arc messages) */
	MSG_LOGO	logo;                /* Type, module, instid of retrieved msg */
	FILE		*fptmp;           /* Pointer to temporary file */
	char		sumCard[200];    /* Summary card produced by hypoinverse      */
	int		rc;
	char		*configFile;
	int		type;
	char		control_card[80];
	char		h71_phs[80*MAX_PHASES];
	char		h71HEAD[1 + 26*MAXTESTS + 1 + 81*MAX_PHASES + 1 + 15*MAXLAY + 1];
	char		hypo_input[20];
	char		*out;            /* working pointer to archive out message    */
	char		line[MAX_STR];  /* to store lines from msg               */
	unsigned int	i,alti,lat_deg,lon_deg;
	double		lat_min,lon_min;
	double		trial_lat,trial_lon,trial_depth;
	char	        sta_list[6*MAX_PHASES + MAX_PHASES];	/* Hpck.site is 6 bytes, plus 1 blank for separation */
	char		*sta_found;
	int		Nhead_written;
	int		counter=0;
	int		errsv;
	char		lon_EW,lat_NS;
	char		cmdl[255],WAmag[4];
	

	strcpy(ArchiveDir,"./");
	strcpy(HYPO71PC_bin,"Hypo71PC");		/* Default HYPO71PC binary is located in module binary directory */

/* Check command line arguments
   ****************************/
	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: hyp71_mgr <config file>\n" );
		return -1;
	}
	configFile  = argv[1];

/* Initialize name of log-file & open it
 ***************************************/
	logit_init( argv[1], 0, MAX_BYTES_PER_EQ, 1 );

/* Read in the configuration file
   ******************************/
	TrialDepth[0] = 50;
	GetConfig( configFile );
	if(UseLocalmag)
		GetLocalmagConfig( LocalmagFile );
	else
           logit( "et", "hyp71_mgr: LocalMagFile not specified in <%s> - don't use localmag\n", configFile);
		
/* Make up the names of the temporary arc files
   ********************************************/
	sprintf( H71PRT,  "hyp71%u.PRT",  MyModId );
	if (Debug == 1)
		logit( "t", "H71PRT print output file:<%s>\n" ,H71PRT);
	sprintf( H71PUN,  "hyp71%u.PUN",  MyModId );
	if (Debug == 1)
		logit( "t", "H71PUN punched output file:<%s>\n" ,H71PUN);
	sprintf( arcIn,  "hyp71In%u",  MyModId );
	if (Debug == 1)
		logit( "t", "arcIn hyp2000_arc input message file:<%s>\n" ,arcIn);
	sprintf( arcOut, "hyp71Out%u", MyModId );
	if (Debug == 1)
		logit( "t", "arcOut hyp2000_arc output message file:<%s>\n" ,arcOut);
	strcpy( sumName, "none" );

/* Look up important info from earthworm.h tables
 ************************************************/
	LookUp();

/* Reinitialize logit to desired logging level
 *********************************************/
	logit_init( argv[1], 0, MAX_BYTES_PER_EQ, LogSwitch );

/* Attach to the transport ring (Must already exist)
   *************************************************/
	if(!TestMode)
		tport_attach( &Region, RingKey );

/* Wait for a message from the pipe
   Get it and proccess it
   ********************************/
	if(TestMode)
		goto test_mode;
	while ( 1 )
	{
		rc = pipe_get( msg, BufLen, &type );
		if ( rc < 0 )
		{
			if ( rc == -1 )
			{
				ReportError( ERR_TOOBIG );
				logit( "et", "hyp71_mgr: Message in pipe too big for buffer. Exiting.\n" );
			}
			else if ( rc == -2 )
				logit( "et", "hyp71_mgr: <null> on pipe_get. Exiting.\n" );
		else if ( rc == -3 )
			logit( "et", "hyp71_mgr: EOF on pipe_get. Exiting.\n" ); 
		break;
		}

/* Stop program if this is a "kill" message
   ****************************************/
		if ( type == (int) TypeKill )
		{
			logit( "t", "hyp71_mgr: Termination requested. Exiting.\n" );
			break;
		}

/* This is an archive file event
   *****************************/
		if ( type == (int) TypeHyp2000Arc )
		{
	test_mode:
			counter++;
			if(TestMode)
			{
			        fptmp = fopen( Hypo2000_Test_File, "r" );
	                        errsv=errno;
				if ( fptmp == (FILE *) NULL )
				{
					logit( "et", "hyp71_mgr: Error opening test file: %s\n\t%s\n", Hypo2000_Test_File, strerror(errsv) );
				}
				fread( msg, sizeof(char), (size_t)BufLen, fptmp );
				fclose( fptmp );
			}
			if (Debug == 1)
				logit( "t", "hyp71_mgr message received from pipe:\n%s\n" ,msg);

/* Create temporary archive file for depth iteration first pass */
			fptmp = fopen( arcIn, "w" );
                        errsv=errno;
			if ( fptmp == (FILE *) NULL )
			{
			ReportError( ERR_TMPFILE );
			logit( "et", "hyp71_mgr: Error creating file: %s\n\t%s\n", arcIn, strerror(errsv) );
			if(!TestMode)
				continue;
			}
	/* Process the input message and builds the Hypo71 phase message */
			if(hypoarc_2_hypo71inp(msg,h71_phs,&trial_lat,&trial_lon,sta_list) == -1)
			{
				logit( "et", "hyp71_mgr: Error converting Arc message\n");
				if(!TestMode)
					continue;
			}
			if (Debug == 1)
				logit( "t", "hyp71_mgr message converted:\n%s\n" ,h71_phs);
	/* Write Reset list, station list (only picked stations) and velocity model */

			Nhead_written = sprintf ( h71HEAD, "HEAD\n" );
			if (nTests != 0)
			{
				for ( i = 0; i < nTests; i++ )
					Nhead_written = Nhead_written +  sprintf ( h71HEAD + Nhead_written, "RESET TEST(%02.0f)=%#10f\n", Test_num[i], Test_val[i] );
			}
			Nhead_written = Nhead_written +  sprintf ( h71HEAD + Nhead_written, "\n");
			for ( i = 0; i < nSite; i++ )
			{
				sta_found = strstr(sta_list,Site[i].name);
				if ( sta_found == NULL )
					continue;
				strncpy( sta_found, "      ", strlen(Site[i].name) );
				if (Debug == 1)
					logit( "t", "Station found in station list :<%s>\n" ,Site[i].name);
				lat_deg = fabs(Site[i].lat);
				lat_min = 60 * ( fabs(Site[i].lat) - lat_deg );
				lat_NS = 'N';
				if ( Site[i].lat < 0 )
					lat_NS = 'S';
				lon_deg = fabs(Site[i].lon);
				lon_min = 60 * ( fabs(Site[i].lon) - lon_deg );
				lon_EW = 'E';
				if ( Site[i].lon < 0 )
					lon_EW = 'W';
				alti = 1000 * Site[i].elev;
				Nhead_written = Nhead_written +  sprintf ( h71HEAD + Nhead_written, "  %-4s%2u%5.2f%c%3u%5.2f%c%4d  0.00\n",
							Site[i].name, lat_deg, lat_min, lat_NS, lon_deg, lon_min, lon_EW, alti );
			}
			Nhead_written = Nhead_written +  sprintf ( h71HEAD + Nhead_written, "\n" );
			for ( i = 0; i < nLay; i++ )
				Nhead_written = Nhead_written +  sprintf ( h71HEAD + Nhead_written, "%7.3f%7.3f\n", vLay[i], zTop[i] );
			Nhead_written = Nhead_written +  sprintf ( h71HEAD + Nhead_written, "\n" );

			fprintf ( fptmp, "%s", h71HEAD );
	/* Locate the event at different depth */
			lat_deg = fabs(trial_lat);
			lat_min = 60 * (fabs(trial_lat) - lat_deg);
			lon_deg = fabs(trial_lon);
			lon_min = 60 * (fabs(trial_lon) - lon_deg);
			fprintf ( fptmp, "   0.%#5.0f%#5.0f%5.2f    4    0         1    1         0  1 1  %2u %5.2f %3u %5.2f\n",
				 Xnear, Xfar, PS_ratio, lat_deg, lat_min, lon_deg, lon_min );
			fprintf ( fptmp, "%s", h71_phs );
			for(i = 1; i < nTrialDepth; i++)
				fprintf ( fptmp, "     **          10%#5.0f\n", TrialDepth[i-1]);
			fprintf ( fptmp, "                 10%#5.0f\n", TrialDepth[i-1]);
			fclose(fptmp);
	/* Write the Hypo71 command parameters */
			sprintf(hypo_input,"hypo.input.%u",MyModId);
			fptmp = fopen( hypo_input, "w" );
                        errsv=errno;
 			if ( fptmp == (FILE *) NULL )
			{
				ReportError( ERR_TMPFILE );
				logit( "et", "hyp71_mgr: Error creating file: <%s>\n%s\n", hypo_input, strerror(errsv));
				if(!TestMode)
					continue;
			}
			fprintf(fptmp,"%s\n",arcIn);		/*Input Phase datas*/
			fprintf(fptmp,"%s\n",H71PRT);		/*Printed output*/
			fprintf(fptmp,"%s\n",H71PUN);		/*Output punched card*/
			fprintf(fptmp,"\n\n\n");
			fclose( fptmp );
	/* Run Hypo71 with the different depth */
			sprintf(cmdl,"%s < %s > %s",HYPO71PC_bin,hypo_input,arcOut);
			system(cmdl);
		        remove( arcIn );
	/* Process the Hypo71 result file and search for best trial depth */
			search_depth( H71PUN , &trial_depth , &trial_lat , &trial_lon );
			if (Debug == 1)
				logit( "", "Best trial depth is %#5.0f\n",trial_depth);
			lat_deg = fabs(trial_lat);
			lat_min = 60 * (fabs(trial_lat) - lat_deg);
			lon_deg = fabs(trial_lon);
			lon_min = 60 * (fabs(trial_lon) - lon_deg);
			sprintf ( control_card, "%#5.0f%#5.0f%#5.0f%5.2f    4    0         1    1         0  1 1  %2u %5.2f %3u %5.2f\n",
				trial_depth, Xnear, Xfar, PS_ratio, lat_deg, lat_min, lon_deg, lon_min );
	/* Final Hypo71 run */
			run_hypo71(arcIn,h71HEAD,h71_phs,hypo_input,arcOut,control_card);
	/*Process the output Hypo71 file and build a TypeHyp2000Arc message */
			out = hypo71pun_2_hypoarc( H71PUN , arcOut, msg );
	/* Archive Hypo71 Input/Output files */
			if(archive_files)
			{
				sprintf(cmdl,"cp %s %s%s%04d",arcIn,ArchiveDir,arcIn,counter);
				system(cmdl);
				sprintf(cmdl,"cp %s %s%s%04d",arcOut,ArchiveDir,arcOut,counter);
				system(cmdl);
			}
		        remove( arcIn );
			remove( arcOut );
	/* No hypocenter have been found, let's take the hypocenter from binder */
			if(out==NULL)
			{
				out=msg;
	         		logit("e", "No hypocenter found in Hypo71PC output, taking hypocenter from input\n" );
			}

	/* Calculate WoodAnderson local magnitude with localmag, if enabled
	   ****************************************************************/
			if(UseLocalmag)
			{
				fptmp = fopen ( arcOut,"w");
	                        errsv=errno;
	 			if ( fptmp != (FILE *) NULL )
				{
					fprintf(fptmp,"%s",out);
					fclose(fptmp);
					sprintf(cmdl,"localmag %s < %s",LocalmagFile,arcOut);
					if(Debug == 1)
			         		logit("t", "Performing localmag with command:<%s>\n", cmdl );
					system(cmdl);
					remove( arcOut );
					fptmp = fopen( MagOutputFile, "r" );
		                        errsv=errno;
					if ( fptmp == (FILE *) NULL )
					{
						logit( "et", "hyp71_mgr: Error opening Magnitude output file: %s\n\t%s\n", MagOutputFile, strerror(errsv));
						logit( "et", "\tNo magnitude found\n");
	                                        out[146]='D';                   /* Duration Magnitude */
					}
					else
					{
						if(fgets(line,sizeof line,fptmp) != NULL)
						{
							strncpy(WAmag,3+strstr(line,"ML"),1);
							WAmag[1]=0;
							strncat(WAmag,5+strstr(line,"ML"),2);
							fclose(fptmp);
		                                        strncpy(out+36,WAmag,3);        /* Amplitude Magnitude */
		                                        out[146]='L';                   /* Local Magnitude */
		                                        strncpy(out+147,WAmag,3);       /* Preferred Magnitude */
						}
						remove( MagOutputFile );
					}
				}
				else
				{
					ReportError( ERR_TMPFILE );
					logit( "et", "hyp71_mgr: Error creating file: <%s> for localmag\n\t%s\n", arcOut, strerror(errsv));
					logit( "et", "\tNo localmag calculation\n");
                                        out[146]='D';                   /* Duration Magnitude */
				}
			}

	/* Get the summary line from archive message (first line)
	   ******************************************************/
			sscanf( out, "%[^\n]", sumCard );
			if(Debug == 1)
	         		logit("e", "1st line of arcmsg:\n%s\n\n", sumCard );

	/* Convert the hypoinverse summary to hypo71
	   summary and add the source code to it
	   *****************************************/
			hyp2000sum_hypo71sum2k( sumCard );
			sumCard[81] = SourceCode;

	/* Send the Arc2000 message to the transport ring
	   **********************************************/
			if(!TestMode)
			{
				logo.instid = InstId;
				logo.mod    = MyModId;
				logo.type   = TypeHyp2000Arc;
				if ( tport_putmsg( &Region, &logo, strlen(out), out ) != PUT_OK )
				{
					ReportError( ERR_ARC2RING );
					logit( "et","hyp71_mgr: Error sending archive msg to transport ring.\n" );
				}
			}
			else
				logit( "et","hyp71_mgr: output archive msg\n%s\n",out );


	/* Print a status line to stderr
	   *****************************/
			PrintSumLine( sumCard );

	/* Send the summary file to the transport ring
	   *******************************************/
			if(!TestMode)
			{
				logo.instid = InstId;
				logo.mod    = MyModId;
				logo.type   = TypeH71Sum2K;
				if ( tport_putmsg( &Region, &logo, strlen(sumCard), sumCard ) != PUT_OK )
				{
					ReportError( ERR_SUM2RING );
					logit( "et","hyp71_mgr: Error sending summary line to transport ring.\n" );
				}
			}
			else
				goto end_test_mode;
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
				logit( "et","hyp71_mgr: Error passing msg (type %d) to transport ring.\n",
				(int) type );
			}
		}
	}

/* Detach from the transport ring and exit
   ***************************************/
	tport_detach( &Region );
end_test_mode:
	return 0;
}


  /*******************************************************************
   *                           GetConfig()                           *
   *                                                                 *
   *  Processes command file using kom.c functions.                  *
   *  Exits if any errors are encountered.                           *
   *******************************************************************/
#define ncommand 7        /* # of required commands you expect to process   */
void GetConfig(char *configfile)
{
   char   init[ncommand]; /* init flags, one byte for each required command */
   int    nmiss;          /* number of required commands that were missed   */
   char  *com;
   char  *str;
   int    nfiles;
   int    success;
   int    i;
   double t, val;
   double z, v;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "hyp71_mgr: Error opening command file <%s>. Exiting.\n",
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
                          "hyp71_mgr: Error opening command file <%s>. Exiting.\n",
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
                             "hyp71_mgr: Invalid module name <%s>. Exiting.\n",
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
   /*4*/    else if( k_its( "lay" ) ) {
                init[4] = 1;
                z = k_val();
                v = k_val();
                lay_model(z, v);
            }
   /*5*/    else if( site_com()      ) {
                init[5] = 1;
	    }
   /*6*/    else if( k_its( "HYPO71PC_bin" ) ) {
                str = k_str();
                if(str) strcpy(HYPO71PC_bin, str);
                init[6] = 1;
            }
            else if( k_its( "Debug" ) ) {		/* optional */
                Debug = k_int();
		if(Debug == 1)
			logit( "t","Debug = %d\n",Debug);
            }
            else if(k_its("psratio")) {			/* optional */
                PS_ratio = k_val();
            }
            else if(k_its("Test")) {			/* optional */
                t = k_val();
                val = k_val();
	        if(nTests < MAXTESTS) {
	                Test_num[nTests] = t;
	                Test_val[nTests] = val;
	                nTests++;
		}
	    }
	    else if(k_its("Xnear")) {			/* optional */
                Xnear = k_val();
	    }
            else if(k_its("Xfar")) {			/* optional */
                Xfar = k_val();
	    }
            else if(k_its("TrialDepth")) {		/* optional */
                val = k_val();
	        if(nTrialDepth < MAXTRIAL) {
	                TrialDepth[nTrialDepth] = val;
	                nTrialDepth++;
		}
	    }
            else if(k_its("ArchiveDirectory")) {	/* optional */
		str = k_str();
		if(str[strlen(str)]!='/')
			strcat(str,"/");
                strcpy(ArchiveDir,str);
		archive_files=1;
	    }
            else if(k_its("LocalmagFile")) {		/* optional */
		str = k_str();
                strcpy(LocalmagFile,str);
		UseLocalmag=1;
	    }
            else if(k_its("Hypo2000_Test_File")) {		/* optional */
		str = k_str();
                strcpy(Hypo2000_Test_File,str);
		TestMode=1;
	    }
            else {
                logit( "e", "hyp71_mgr: <%s> unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() )
            {
               logit( "e", "hyp71_mgr: Bad <%s> command in <%s>; \n",
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
       logit( "e", "hyp71_mgr: ERROR, no " );
       if ( !init[0] )  logit( "e", "<RingName> "   );
       if ( !init[1] )  logit( "e", "<LogFile> "    );
       if ( !init[2] )  logit( "e", "<MyModuleId> " );
       if ( !init[3] )  logit( "e", "<SourceCode> " );
       if ( !init[4] )  logit( "e", "<lay> "        );
       if ( !init[5] )  logit( "e", "<site_file> "  );
       if ( !init[6] )  logit( "e", "<HYPO71PC_bin> " );
       logit( "e", "command(s) in <%s>. Exiting.\n", configfile );
       exit( -1 );
   }

   return;
}

/**************************************************************************
 * lay_model()  Add layer to velocity model.                              *
 **************************************************************************/
int lay_model(double z, double v)
{
        int i;

        if(nLay < MAXLAY) {
                zTop[nLay] = z;
                vLay[nLay] = v;
                nLay++;
        }

        for(i=1; i<nLay; i++) {
                zTop[2*nLay-1-i] = 2*zTop[nLay-1] - zTop[i];
                vLay[2*nLay-1-i] = vLay[i-1];
        }
        zTop[nLay] = zTop[nLay-1] + 0.01;
	if(Debug == 1) {
		printf("nLay = %d\n", nLay);
	        for(i=0; i<2*nLay-1; i++)
	                printf("%d : %6.1f %6.1f\n", i, zTop[i], vLay[i]);
	}
        return nLay;
}


  /*******************************************************************
   *                           GetLocalMagConfig()                   *
   *                                                                 *
   *  Processes command file using kom.c functions.                  *
   *  Shows errors encountered and disable localmag.                 *
   *  Checks if some parameters are set correctly for use with hyp71 *
   *******************************************************************/
#define nLocalMagcommand 3        /* # of required commands you expect to process   */
void GetLocalmagConfig(char *configfile)
{
   char   init[nLocalMagcommand]; /* init flags, one byte for each required command */
   int    nmiss;          /* number of required commands that were missed   */
   char  *com;
   char  *str;
   int    nfiles;
   int    success;
   int    i;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<nLocalMagcommand; i++ )  init[i] = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "hyp71_mgr: Error opening command file <%s>. Exiting.\n",
                 configfile );
	UseLocalmag=0;
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
                          "hyp71_mgr: Error opening command file <%s>. Exiting.\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }
        /* Process anything else as a command
         ************************************/
/*0*/		if (k_its("outputFormat"))
		{
        		if ( (str = k_str()) )
		        {
        			if (k_its("File") )
				{
					str = k_str();
        	    			strcpy(MagOutputFile,str);
					init[0] = 1;
        	  		}
        	  		else
        	  		{
        	    			logit("e", "ReadConfig: bad output format <%s> (must be <File>)\n", str);
        	    			init[0] = 0;
        	  		}
        		}
        		else
        		{
        	  		logit("e", "ReadConfig: \"outputFormat\" missing argument\n");
        			init[0] = 0;
        		}
		}
/*1*/		else if (k_its("eventSource"))
		{
        		if ( (str = k_str()) )
		        {
        			if (k_its("ARCH") )
					init[1] = 1;
        	  		else
        	  		{
        	    			logit("e", "ReadConfig: bad eventSource <%s> (must be <ARCH>)\n", str);
        	    			init[1] = 0;
        	  		}
        		}
        		else
        		{
        	  		logit("e", "ReadConfig: \"eventSource\" missing argument\n");
        			init[1] = 0;
        		}
		}
/*2*/		else if (k_its("traceSource"))
		{
        		if ( (str = k_str()) )
		        {
        			if (k_its("waveServer") )
					init[2] = 1;
        	  		else
        	  		{
        	    			logit("e", "ReadConfig: bad traceSource <%s> (must be <waveServer>)\n", str);
        	    			init[2] = 0;
        	  		}
        		}
        		else
        		{
        	  		logit("e", "ReadConfig: \"traceSource\" missing argument\n");
        			init[2] = 0;
        		}
		}
          }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i = 0; i < nLocalMagcommand; i++ )
      if( !init[i] ) nmiss++;

   if ( nmiss )
   {
       logit( "e", "hyp71_mgr: ERROR, no " );
       if ( !init[0] )  logit( "e", "<outputFormat> "  );
       if ( !init[1] )  logit( "e", "<eventSource> "   );
       if ( !init[2] )  logit( "e", "<traceSource> "   );
       logit( "e", "command(s) in <%s>. LocalMag disabled.\n", configfile );
       UseLocalmag=0;
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
              "hyp71_mgr: Invalid ring name <%s>. Exiting.\n", RingName );
      exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      fprintf( stderr,
              "hyp71_mgr: error getting local installation id. Exiting.\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      fprintf( stderr,
              "hyp71_mgr: Invalid message type <TYPE_ERROR>. Exiting.\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 )
   {
      fprintf( stderr,
              "hyp71_mgr: Invalid message type <TYPE_HYP2000ARC>. Exiting.\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum2K ) != 0 )
   {
      fprintf( stderr,
              "hyp71_mgr: Invalid message type <TYPE_H71SUM2K>. Exiting.\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 )
   {
      fprintf( stderr,
              "hyp71_mgr: Invalid message type <TYPE_KILL>. Exiting.\n" );
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
   sprintf( errMsg, "%ld %d\n", (long) tstamp, errNum );

   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = TypeError;
   length      = strlen( errMsg );

   if ( tport_putmsg( &Region, &logo, length, errMsg ) != PUT_OK )
      logit( "et", "hyp71_mgr: Error sending error msg to transport region <%s>\n",
               RingName);
   return;
}


