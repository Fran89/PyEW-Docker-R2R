
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *     Revision 1.4.3  2016/09/30 00:00:00  jmsaurel
 *     Corrected various bug in hypo71pun_2_hypoarc
 *	Arc output messages was well formatted only in nice cases
 *
 *     Revision 1.4.2  2011/10/25 00:00:00  jmsaurel
 *     Corrected major str formatting bug in hypoarc_2_hypo71inp
 *	This prevented the module from locating any event
 *
 *     Revision 1.4.1  2011/08/19 00:00:00  jmsaurel
 *     Corrected exception when picks where both before and after an hour
 *     At anytime, all picks must be refered to the same hour,
 *	the extra time is then into the minute
 *
 *     Revision 1.4  2010/12/08 00:00:00  jmsaurel
 *     Added some debug output
 *
 *     Revision 1.3  2010/01/26 00:00:00  jmsaurel
 *     Corrected an important bug related to termination line and shadow
 *     The processing was wrong in hypo71pun_2_hypoarc
 *     Corrected a bug with 4 letter site code inside hypoarc_2_hypo71inp
 *
 *     Revision 1.2  2009/12/17 00:00:00  jmsaurel
 *     Added a function to run Hypo71
 *     Added a function to check if the location is stucked to the trial depth
 *
 *     Revision 1.1  2008/11/01 00:00:00  jmsaurel
 *     Initial revision
 *
 *
 */

  /********************************************************************
   *                             hypo71_hypo2000_conv                 *
   *                                                                  *
   *                                                                  *
   ********************************************************************/

#include "hyp71_mgr.h"

extern int Debug;
extern char HYPO71PC_bin[];

/****************************************************************************/
/* hypoarc_2_hypo71inp() processes an TypeHyp2000Arc message and            */
/*                       converts it into an Hypo71 phase list at the end   */
/*                       of the hypo_input file in argument, then close it  */
/****************************************************************************/

int hypoarc_2_hypo71inp(char *in_msg, char *hyp71_phases, double *latitude, double *longitude, char *Psta_list)
{
	char     *in;             /* working pointer to archive message    */
	char      line[MAX_STR];  /* to store lines from msg               */
	char      shdw[MAX_STR];  /* to store shadow cards from msg        */
	int       msglen;         /* length of input archive message       */
	int       nline;          /* number of lines (not shadows) so far  */
//	unsigned int	lat_deg,lon_deg;
//	double	lat_min,lon_min;
	double	REFat;		  /* Hour of first arrival time for this quake	*/
//	double	Minuteat;	  /* Seconds since first 	*/
//	char	lon_EW,lat_NS;
	char	str[18];
	char	hypo_msg[80];
	char	h71_sta_name[5];
	/* Initialize some stuff
	***********************/
	nline  = 0;
	msglen = strlen( in_msg );
	in     = in_msg;
	strcpy ( hyp71_phases, "" );
	strcpy ( Psta_list, "" );

	/* Read one data line and its shadow at a time from arcmsg; process them
	***********************************************************************/
	while( in < in_msg+msglen )
	{
		if ( sscanf( in, "%[^\n]", line ) != 1 )
		{
			logit( "et", "HypoArc Translation : zerolength line message\n");
			return( -1 );
		}
		in += strlen( line ) + 1;
		if ( sscanf( in, "%[^\n]", shdw ) != 1 )
		{
			logit( "et", "HypoArc Translation : zerolength line shadow\n");
			return( -1 );
		}
		in += strlen( shdw ) + 1;
		nline++;

	/* Process the hypocenter card (1st line of msg) & its shadow
	************************************************************/
		if( nline == 1 )                /* hypocenter is 1st line in msg  */
		{
			read_hyp( line, shdw, &Sum );
			*latitude = Sum.lat;
			*longitude = Sum.lon;
			continue;
		}

	/* Process all the phase cards & their shadows
	*********************************************/
		if( strlen(line) < (size_t) 75 )	/* found the terminator line      */
			break;
		read_phs( line, shdw, &Pick );		/* load info into Pick structure   */
		strcpy(h71_sta_name,Pick.site);
		if(strlen(Psta_list)<2)				/* First read phase is assumed to be first arrival */
			REFat = floor(Pick.Pat/3600) * 3600;	/* REFat is the reference hour arrival */
		if(Pick.Plabel!=' ')			/* Record this station has a P-phase */
		{
			strcat(Psta_list,h71_sta_name);
			strcat(Psta_list," ");
		}
		strncpy(hypo_msg,Pick.site,4);



		strcat(hypo_msg,"       ");
		hypo_msg[4] = '\0';
		strncat(hypo_msg,&Pick.Ponset,1);
		strncat(hypo_msg,&Pick.Plabel,1);
		strncat(hypo_msg,&Pick.Pfm,1);
		sprintf(str,"%1d",Pick.Pqual);
		strcat(hypo_msg,str);
		strcat(hypo_msg," ");
		date17(REFat,str);
		strncat(hypo_msg,str+2,8);		/* Reference hour */
		sprintf(str,"%02.0f",floor((Pick.Pat - REFat)/60));
		if(Pick.Plabel==' ')
			sprintf(str,"%02.0f",floor((Pick.Sat - REFat)/60));
		strncat(hypo_msg,str,2);		/* minute */
		date17(Pick.Pat,str);
		if(Pick.Plabel==' ')
			date17(Pick.Sat,str);
		strncat(hypo_msg,str+12,5);		/* seconds */
		strcat(hypo_msg,"       ");
		date17(Pick.Sat,str);
		if(Pick.Slabel==' ')
			strcat(hypo_msg,"     ");
		else
			strncat(hypo_msg,str+12,5);
		strncat(hypo_msg,&Pick.Sonset,1);
		strncat(hypo_msg,&Pick.Slabel,1);
		strncat(hypo_msg,&Pick.Sfm,1);
	/* If this station doens't already have a P-phase, don't take into account the S-phase, so weight it with 4 */
		if(strstr(Psta_list,h71_sta_name)==NULL)
		{
			sprintf(str,"4");
			strcat(Psta_list,h71_sta_name);
			strcat(Psta_list," ");
		}
		else
			sprintf(str,"%1d",Pick.Squal);
		if(Pick.Slabel==' ')
			strcat(hypo_msg," ");
		else
			strcat(hypo_msg,str);
		strcat(hypo_msg,"                              ");
		if(Pick.codalen != 0)
		{
			sprintf(str,"%4i",Pick.codalen);
			str[4]='.';
			str[5]='\0';
			strcat(hypo_msg,str);
		}	
		strcat ( hyp71_phases, hypo_msg );
		strcat ( hyp71_phases, "\n" );
	} /*end while over reading message*/
	return(1);
}


/****************************************************************************/
/* hypo71pun_2_hypoarc() processes the Hypo71 punched output file and       */
/*                       converts it into a TypeHyp2000Arc message and      */
/*                       a TypeHyp71Sum2k message                           */
/****************************************************************************/

char  *hypo71pun_2_hypoarc( char *hypo71_pun , char *hypo71_out, char *arc_msg )
{
	static char inmsg[MAX_BYTES_PER_PUN];  /* Message buffer (for .pun messages) */
	static char outmsg[MAX_BYTES_PER_PUN];  /* Message buffer (for arc messages) */
	char  *in, *hypo2000_in;             /* working pointers to messages */
	char  *hypo2000_out;             /* working pointers to messages */
	char   line_in[MAX_STR];            /* to store lines from inmsg    */
	char   line_out[MAX_STR];        /* to store lines from arcmsg    */
	char	shdw[MAX_STR];  /* to store shadow cards from msg        */
	char	str[10];  /* working string for values conversion        */
	double val;
	int    inmsglen,outmsglen;             /* length of messages */
	int    nline_in,nline_out;                /* number of lines processed    */
	int    begin=0;
	int	BufLen = MAX_BYTES_PER_PUN;      /* Message buffer length */
	FILE	*fp_h71;
/* Initialize some stuff
 ***********************/
	hypo2000_out = outmsg;
	hypo2000_in = arc_msg;
	nline_in  = 0;
	nline_out = 0;
	outmsglen = strlen( arc_msg );

/* Read Hypo71 punched output
 ****************************/
	fp_h71 = fopen ( hypo71_pun , "r" );
        if ( fp_h71 == (FILE *) NULL )
		return(NULL);
	inmsglen = fread( inmsg, sizeof(char), (size_t)BufLen, fp_h71 );
	fclose( fp_h71 );
	if ( inmsglen == 0 )
		return(NULL);
	in = inmsg;
	/* Read one data line and its shadow at a time from arcmsg; process them
	***********************************************************************/
	while(( hypo2000_in < arc_msg+outmsglen ) || ( in < inmsg+inmsglen ))
	{
		if ( sscanf( hypo2000_in, "%[^\n]", line_out ) != 1 )  return( NULL );
			hypo2000_in += strlen( line_out ) + 1;
		if ( sscanf( hypo2000_in, "%[^\n]", shdw ) != 1 )  return( NULL );
			hypo2000_in += strlen( shdw ) + 1;
		nline_out++;
		if ( sscanf( in, "%[^\n]", line_in ) != 1 )  return( NULL );
	/* Process the hypocenter card
	*****************************/
		if (nline_out==1)
		{
			while(nline_in<2)
			{
				if ( sscanf( in, "%[^\n]", line_in ) != 1 )  return( NULL );
				in += strlen( line_in ) + 1;
				nline_in++;
			}
			if(Debug == 1)
	         		logit("e", "Punched card found :\n%s\n\n", line_in );
			strncpy(hypo2000_out,line_out,2);	/* Year 2k yy */
			hypo2000_out[2]=0;
			strncat(hypo2000_out,line_in,6);	/* Hypocenter date yymmdd */
			strncat(hypo2000_out,line_in+7,4);	/* Hypocenter date HHMM */
			strncat(hypo2000_out,line_in+12,2);	/* Seconds */
			strncat(hypo2000_out,line_in+15,2);	/* Seconds decimal part */
			strncat(hypo2000_out,line_in+18,2);	/* Latitude degree */
			strncat(hypo2000_out,line_out+18,1);
			strncat(hypo2000_out,line_in+21,2);	/* Latitude minute */
			strncat(hypo2000_out,line_in+24,2);	/* Latitude minute decimal part */
			strncat(hypo2000_out,line_in+27,3);	/* Longitude degree */
			strncat(hypo2000_out,line_out+26,1);
			strncat(hypo2000_out,line_in+31,2);	/* Longitude minute */
			strncat(hypo2000_out,line_in+34,2);	/* Longitude minute decimal part */
			strncat(hypo2000_out,line_in+37,3);	/* Depth */
			strncat(hypo2000_out,line_in+41,2);	/* Depth decimal part */
			strcat(hypo2000_out,"   ");		/* Amplitude Magnitude not calculated */
			strncat(hypo2000_out,line_in+50,3);	/* Number of phases processed */
			strncat(hypo2000_out,line_in+54,3);	/* Maximum GAP */
			strncat(hypo2000_out,line_in+57,3);	/* Dmin in km */
			strncat(hypo2000_out,line_in+62,2);	/* RMS */
			strncat(hypo2000_out,line_in+65,2);	/* RMS decimal part */
			strncat(hypo2000_out,line_out+52,18);
			strncat(hypo2000_out,line_in+46,1);	/* Coda duration Magnitude */
			strncat(hypo2000_out,line_in+48,2);	/* Coda duration Magnitude decimal part */
			strncat(hypo2000_out,line_out+73,12);
			strncat(hypo2000_out,line_in+67,3);	/* ERH */
			strncat(hypo2000_out,line_in+71,1);	/* ERH decimal part */
			strncat(hypo2000_out,line_in+72,3);	/* ERZ */
			strncat(hypo2000_out,line_in+76,1);	/* ERZ decimal part */
			strncat(hypo2000_out,line_out+73,25);
			strncat(hypo2000_out,line_in+50,3);	/* Number of used phases */
			strncat(hypo2000_out,line_out+121,27);
			strncat(hypo2000_out,line_in+46,1);	/* Coda duration Magnitude is preferred Magnitude for the moment */
			strncat(hypo2000_out,line_in+48,2);	/* Coda duration Magnitude decimal part */
			strncat(hypo2000_out,line_out+150,14);
			strcat(hypo2000_out,"\n");
			hypo2000_out += 166;
			strncpy(hypo2000_out,shdw,strlen(shdw));
			strcat(hypo2000_out,"\n");
			hypo2000_out += strlen(shdw)+1;
		/* Now that the hypocenter card is processed, continue with the output of Hypo71 */
			fp_h71 = fopen ( hypo71_out , "r" );
		        if ( fp_h71 == (FILE *) NULL )
				return(NULL);
			inmsglen = fread( inmsg, sizeof(char), (size_t)BufLen, fp_h71 );
			fclose( fp_h71 );
			if ( inmsglen == 0 )
				return(NULL);
			in = inmsg;
			nline_in=0;
		/* If Hypo71 failed to locate the quake, return */
			if(strstr(inmsg,"*** INSUFFICIENT DATA FOR LOCATING THIS QUAKE ***") != NULL)
				{
				logit( "t", "Hypo71 : *** INSUFFICIENT DATA FOR LOCATING THIS QUAKE ***\n");
				return(NULL);
				}
			else
				logit( "t", "Hypo71 : QUAKE LOCATED\n");
		/* Find the beginning of the cards that interest us */
			while( in < inmsg+inmsglen )
			{
				if(sscanf( in, "%[^\n]", line_in ) == 0)
					{
					begin++;
					strcpy(line_in,"");
					}
				in += strlen( line_in ) + 1;
				if(begin==1 && strlen(line_in)==78)
				{
		/* We found the beginning of the cards of interest */
					nline_in++;
					break;
				}
			}
			continue;
		}
	/* Process all the phase cards & their shadows
	*********************************************/
		if( strlen(line_out) < (size_t) 75 )  /* found the terminator line      */
			{
			if(Debug == 1)
	         		logit("e", "End of arcmsg or input message\nline_out = <%s>\n", line_out );
			strcpy(hypo2000_out,line_out);
			strcat(hypo2000_out,"\n");
			hypo2000_out+=strlen(line_out)+1;
			strcpy(hypo2000_out,shdw);
			break;
			}
		else
			{
			if(Debug == 1)
	         		logit("e", "Phase card found :%s\n", line_in );
			strncpy(hypo2000_out,line_out,34);
			hypo2000_out[34]=0;
			strncpy(str,line_out,4);			/* Station code */
			str[4]='\0';
			if(strstr(line_in,str)!=NULL)			/* Station from Hypo71 is the station we are working on from hypo2000_arc */
			{
				strncpy(str,line_in+39,6);		/* PRes */
				str[6]='\0';
				val=atof(str);
				if(val> 9.99) val= 9.99;
				if(val<-9.99) val=-9.99;		/* Clip residual to 9.99 */
				sprintf(str,"%4.0f",val*100);
				strncat(hypo2000_out,str,4);		/* PRes */

				strncat(hypo2000_out,line_in+46,1);	/* PW-T */
				strncat(hypo2000_out,line_in+48,2);	/* PW-T decimal part */
				strncat(hypo2000_out,line_out+41,9);	/* S arrival time */

				strncpy(str,line_in+66,6);		/* SRes */
				str[6]='\0';
				val=atof(str);
				if(val> 9.99) val= 9.99;
				if(val<-9.99) val=-9.99;		/* Clip residual to 9.99 */
				sprintf(str,"%4.0f",val*100);
				strncat(hypo2000_out,str,4);		/* SRes */

				strncat(hypo2000_out,line_out+54,9);
				strncat(hypo2000_out,line_in+74,1);	/* SW-T */
				strncat(hypo2000_out,line_in+76,1);	/* SW-T decimal part */
				strcat(hypo2000_out,"0");		/* SW-T decimal part */
				strncat(hypo2000_out,line_out+66,8);
				strncat(hypo2000_out,line_in+5,3);	/* Dist */
				strncat(hypo2000_out,line_in+9,1);	/* Dist decimal part */
				strncat(hypo2000_out,line_out+78,13);
				strncat(hypo2000_out,line_in+11,3);	/* Azimuth */
				strncat(hypo2000_out,line_out+94,19);
				in += strlen( line_in ) + 1;		/* Finished processing this Hypo71 station line */
				nline_in++;
			}
			else			/* Station from Hypo71 is not the station we are working on from hypo2000_arc, probably not processed by hypo71, skip it */
			{
				if(Debug == 1)
		         		logit("e", "Station %s from arc_message not found in Hypo71 output, skip it\n", str );
				strncat(hypo2000_out,line_out+34,79);
				hypo2000_out[49]='4';
			}
			strcat(hypo2000_out,"\n");
			hypo2000_out+=114;
			strncpy(hypo2000_out,shdw,strlen(shdw));
			strcat(hypo2000_out,"\n");
			hypo2000_out+=strlen(shdw)+1;
			}
	}		
/* Make sure we processed at least one phase per event
 *****************************************************/
	if( nline_out <= 1 )
		{
		logit( "et", "Hypo71 : no phases read\n\tnline_out = <%d>",nline_out);
		return( NULL );
		}
	return(outmsg);
}

/****************************************************************************/
/* search_depth() processes the Hypo71 trial punched output file and        */
/*                       search for the best trial depth looking at         */
/*                       RMS, ERH and ERZ - outputs trial location          */
/****************************************************************************/

int search_depth( char *hypo71_pun , double *trial_depth , double *trial_lat , double *trial_lon )
{
	static char inmsg[MAX_BYTES_PER_PUN];	/* Message buffer (for .pun messages) */
	char	*in;				/* working pointers to messages */
	char	str[10];				/* working pointers to messages */
	char	line_in[MAX_STR];		/* to store lines from inmsg    */
	int	inmsglen;			/* length of messages */
	int	nline_in;	                /* number of lines processed    */
	int	BufLen = MAX_BYTES_PER_PUN;	/* Message buffer length */
	FILE	*fp_h71;
//	double	max_err=99999999;
	double	min_err_RMS=99999999;
	double	min_err_HZ=99999999;
	double	RMS,ERH,ERZ,error_HZ;
	/* Read Hypo71 punched output
	 ****************************/
	fp_h71 = fopen ( hypo71_pun , "r" );
        if ( fp_h71 == (FILE *) NULL )
		return(-1);
	inmsglen = fread( inmsg, sizeof(char), (size_t)BufLen, fp_h71 );
	fclose( fp_h71 );
	if ( inmsglen == 0 )
		return(-2);
	in = inmsg;
	/* Read one line and process
	***************************/
	while( in < inmsg+inmsglen )
	{
		if ( sscanf( in, "%[^\n]", line_in ) != 1 )  return( -1 );
		in += strlen( line_in ) + 1;
		nline_in++;
		if ( strstr(line_in,"DATE") != NULL)
			continue;
		else
		{
	/* Process the hypocenter card
	*****************************/
			strncpy(str,line_in+62,5);	/* RMS */
			str[5]='\0';
			if( strstr(str,"  nan")==NULL )
				RMS=atof(str);
			else
				RMS=99.0;
			strncpy(str,line_in+67,5);	/* ERH */
			str[5]='\0';
			if(strstr(str,"     ")==NULL || strstr(str,"*****")==NULL || strstr(str,"  nan")==NULL )
				ERH=atof(str);
			else
				ERH=999.0;
			strncpy(str,line_in+72,5);	/* ERZ */
			str[5]='\0';
			if(strstr(str,"     ")==NULL || strstr(str,"*****")==NULL || strstr(str,"  nan")==NULL )
				ERZ=atof(str);
			else
				ERZ=999.0;
			error_HZ=sqrt(ERH*ERH+ERZ*ERZ);
			if (Debug == 1)
				logit( "", "Trial result read : %s\n" ,line_in);
			if(RMS<=min_err_RMS)
			{
				if(RMS!=min_err_RMS || error_HZ<min_err_HZ)
				{
					min_err_RMS=RMS;
					min_err_HZ=error_HZ;
					strncpy(str,line_in+18,2);	/* Latitude degree */
					str[2]='\0';
					*trial_lat=atof(str);
					strncpy(str,line_in+21,5);	/* Latitude minute */
					str[5]='\0';
					*trial_lat=*trial_lat+atof(str)/60;
					strncpy(str,line_in+27,3);	/* Longitude degree */
					str[3]='\0';
					*trial_lon=atof(str);
					strncpy(str,line_in+31,5);	/* Longitude minute */
					str[5]='\0';
					*trial_lon=*trial_lon+atof(str)/60;
					strncpy(str,line_in+37,6);	/* Depth */
					*trial_depth=atof(str);
				}
			}
		}
	}
	return(1);		
}

/****************************************************************************/
/* check_depth() processes the Hypo71 trial punched output file and         */
/*                       checks if depth is not equal at trial depth        */
/*                       if equal, return 1                                 */
/****************************************************************************/

int check_depth( char *hypo71_pun , double trial_depth)
{
	static char inmsg[MAX_BYTES_PER_PUN];	/* Message buffer (for .pun messages) */
	char	*in;				/* working pointers to messages */
	char	str[10];				/* working pointers to messages */
	char	line_in[MAX_STR];		/* to store lines from inmsg    */
	int	inmsglen;			/* length of messages */
	int	nline_in;	                /* number of lines processed    */
	int	BufLen = MAX_BYTES_PER_PUN;	/* Message buffer length */
	FILE	*fp_h71;
	double	computed_depth=99999999;
	/* Read Hypo71 punched output
	 ****************************/
	fp_h71 = fopen ( hypo71_pun , "r" );
        if ( fp_h71 == (FILE *) NULL )
		return(-1);
	inmsglen = fread( inmsg, sizeof(char), (size_t)BufLen, fp_h71 );
	fclose( fp_h71 );
	if ( inmsglen == 0 )
		return(-2);
	in = inmsg;
	/* Read one line and process
	***************************/
	while( in < inmsg+inmsglen )
	{
		if ( sscanf( in, "%[^\n]", line_in ) != 1 )  return( -1 );
		in += strlen( line_in ) + 1;
		nline_in++;
		if ( strstr(line_in,"DATE") != NULL)
			continue;
		else
		{
	/* Process the hypocenter card
	*****************************/
		strncpy(str,line_in+37,6);	/* Depth */
		computed_depth=atof(str);
		}
	}
	if(computed_depth==trial_depth)
		return(1);
	else
		return(0);
}

/****************************************************************************/
/* run_hypo71() builds Hypo71 Input file and runs Hypo71                    */
/****************************************************************************/

void run_hypo71( char *arcInput , char *hypo71_HEAD, char *hyp71_phases, char *hypo_cmd, char *arcOutput, char *ctrl_card)
{
	FILE		*fptmp;           /* Pointer to temporary file */
//	FILE		*fpconfig;        /* Pointer to Hypo71 config file */
//	char		hypo_msg[80];
	char		cmdl[255];


	fptmp = fopen( arcInput, "w" );
	if ( fptmp == (FILE *) NULL )
	{
		ReportError( ERR_TMPFILE );
		logit( "et", "hyp71_mgr: Error creating Hypo71 Input file: %s\n", arcInput );
		return;
	}
/* Hypo71 Input file : copying HEAD values, station table and velocity model */
	fprintf ( fptmp, "%s", hypo71_HEAD );
/* Hypo71 Input file : control card */
	fprintf ( fptmp, "%s", ctrl_card );
/* Hypo71 Input file : copying phase list */
	fprintf ( fptmp, "%s", hyp71_phases );
/* Hypo71 Input file : instruction card */
	fprintf ( fptmp, "                 10\n");
	fclose(fptmp);
	sprintf(cmdl,"%s < %s > %s",HYPO71PC_bin,hypo_cmd,arcOutput);
	system(cmdl);
}

