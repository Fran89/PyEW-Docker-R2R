/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nsmp2ew.c 6126 2014-07-21 17:02:58Z luetgert $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.24  2008/09/26 22:25:00  kress
 *     Fix numerous compile warnings and some tab-related fortran errors for linux compile
 *
 *     Revision 1.23  2007/11/09 22:10:03  luetgert
 *     Added filter for text appearing in XML output files to replace
 *     XML special characers with their escaped equivalents. e.g. & -> &amp;
 *
 *     Revision 1.22  2007/10/29 17:41:15  luetgert
 *     .
 *
 *     Revision 1.20  2004/03/01 18:02:58  luetgert
 *     *** empty log message ***
 *
 *     Revision 1.19  2004/02/27 22:35:19  luetgert
 *     Fixed xml file write
 *
 *     Revision 1.18  2003/01/27 16:44:33  lombard
 *     The problems that prevented sm_nsmp2ew from running on sparc Solaris were
 *     fixed by changes Jim Luetgert made to the K2 header file. The original K2
 *     header file had some fields that were not byte-aligned for sparc hardware,
 *     which resulted in the program crashing.
 *
 *     Revision 1.17  2002/12/06 01:15:33  dietz
 *     added new instid argument to file2ew_ship call;
 *     changed fprintf(stderr...) calls to logit("e"...)
 *
 *     Revision 1.16  2002/02/25 21:29:01  lucky
 *     Fixed translation from box to SCN
 *
 *     Revision 1.15  2001/08/30 23:56:23  dietz
 *     changed a return code from -1 to 0 so non-EVT files are
 *     treated properly.
 *
 *     Revision 1.14  2001/05/03 23:07:35  dietz
 *     Changed to write TYPE_STRONGMOTIONII msgs.
 *     NOTE: sm_nsmp2ew DOES NOT run properly on Sparc Solaris!!!
 *
 *     Revision 1.13  2001/03/27 01:09:57  dietz
 *     Added support for reading heartbeat file contents.
 *     Currently file2ewfilter_hbeat() is a dummy function.
 *
 *     Revision 1.12  2001/03/13 21:30:10  dietz
 *     Jim tweaked the XML file writer a bit.
 *
 *     Revision 1.11  2001/03/09 17:14:03  dietz
 *     Jim fixed a bug in the response spectrum calculations
 *
 *     Revision 1.10  2001/02/08 16:36:02  dietz
 *      Added file2ewfilter_com function to read config file commands
 *     and file2ewfilter_shutdown() to cleanup before exit.
 *     NOTE: nsmp2ew.c has never worked properly on Sparc Solaris.
 *           It does work on NT and Intel Solaris.
 *
 *     Revision 1.9  2000/12/08 20:02:04  dietz
 *     Fixed a bug in which lat/lon was being written to the XML file
 *
 *     Revision 1.8  2000/12/08 18:39:15  dietz
 *     New feature (by Jim) to always write an XML file to the
 *     XML subdirectory of its input directory.
 *
 *     Revision 1.7  2000/11/27 21:34:17  dietz
 *     Added array bound checking in amaxper().
 *
 *     Revision 1.6  2000/11/21 22:04:01  dietz
 *     *** empty log message ***
 *
 *     Revision 1.5  2000/11/21 22:01:58  dietz
 *     New version from Jim Luetgert enables channel to SCNL mapping.
 *
 *     Revision 1.3  2000/11/03 21:33:03  dietz
 *     Replaces dummy functions with real functions which read K2-format .evt data
 *     files and calculate peak acceleration, velocity, etc.
 *     Written by Jim Luetgert, November,2000.
 *
 *     Revision 1.2  2000/10/20 18:47:14  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/09/26 22:59:05  dietz
 *     Initial revision
 *
 */

/*  nsmp2ew.c
 *  Read a K2-format .evt data file (from the NSMP system),
 *  create Earthworm strongmotion messages, and place them in a transport
 *  ring.  Returns number of TYPE_STRONGMOTION messages written to ring
 *  from file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <chron3.h>
#include <time_ew.h>
#include <kom.h>
#include <k2evt2ew.h>
#include <rw_strongmotionII.h>
#include "file2ew.h"

#define ABS(X) (((X) >= 0) ? (X) : -(X))

/* Functions in this source file
 *******************************/
int nsmp2ew( FILE *fp, char *fname );
int wr_strongmotionIII( SM_INFO *sm, char *buf, int buflen );
static int   strappend( char *s1, int s1max, char *s2 );
static char *datestr24( double t, char *pbuf, int len );
static int   addtimestr( char *buf, int buflen, double t );

void fixstring( char *istr, char *ostr );
void time2string( char *tstr, double secs );

/* Functions in other source files
 *********************************/
void logit( char *, char *, ... );         /* logit.c      sys-independent  */
int  GetType ( char *, unsigned char * );  /* getutil.c    sys-independent  */

/* Globals from file2ew.c
 ************************/
extern int Debug;
extern char NetworkName[];
extern char authority[];

/* Local globals
 ***************/
#define MAXCHAN  100             /* default maximum # channels            */
static int MaxChan = MAXCHAN;    /* configured max# chan (MaxChannel cmd) */
static int XMLType = 0;          /* configured XML format (XMLType cmd)   */
static int NchanNames = 0;	 /* number of names in this config file       */
CHANNELNAME *ChannelName = NULL; /* table of box/channel to SCNL          */

#define BUFLEN  65000           /* define maximum size for an event msg  */
static char MsgBuf[BUFLEN];     /* char string to hold output message    */
static unsigned char TypeSM2;   /* message type we'll produce            */
static unsigned char TypeSM3;   /* message type we'll produce            */

static int   Init = 0;     /* initialization flag */
static char *sNullDate = "0000/00/00 00:00:00.000";


/******************************************************************************
 * file2ewfilter_com() processes config-file commands to set up the           *
 *                     CSMIP-specific parameters                              *
 * Returns  1 if the command was recognized & processed                       *
 *          0 if the command was not recognized                               *
 * Note: this function may exit the process if it finds serious errors in     *
 *       any commands                                                         *
 ******************************************************************************/
int file2ewfilter_com( )
{
   char   *str;

/* Reset the maximum number of channels
 **************************************/
   if( k_its("MaxChannel") ) { /*optional*/
      MaxChan = k_int();
      if( ChannelName != NULL ) {
         logit( "e","nsmp2ew_com: must specify <MaxChan> before any"
                " <ChannelName> commands; exiting!\n" );
         exit( -1 );
      }
      return( 1 );
   }

/* Specify XML format
 **************************************/
   if( k_its("XMLType") ) { /*optional*/
      XMLType = k_int();
      return( 1 );
   }

/* Get the mappings from box id to SCNL name
 ********************************************/
   if( k_its("ChannelName") ) {  /*optional*/
   /* First one; allocate ChannelName struct */
      if( ChannelName == NULL ) {
         ChannelName = (CHANNELNAME *) malloc (MaxChan * sizeof(CHANNELNAME));
         if( ChannelName == NULL ) {
            logit( "e", "nsmp2ew_com: Error allocating %ld bytes for"
                   " %d chans; exiting!\n",
                   MaxChan*sizeof(CHANNELNAME), MaxChan );
            exit( -1 );
         }
      }
   /* See if we have room for another channel */
      if( NchanNames >= MaxChan ) {
         logit( "e", "nsmp2ew_com: Too many <ChannelName> commands"
                " in configfile; MaxChannel=%d; exiting!\n",
                (int) MaxChan );
         exit( -1 );
      }
   /* Get the box name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>SM_BOX_LEN ) { 
            logit( "e", "nsmp2ew_com: box name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].box, str);
      }
   /* Get the channel number */
      ChannelName[NchanNames].chan = k_int();
      if(ChannelName[NchanNames].chan > SM_MAX_CHAN){
         logit( "e", "nsmp2ew_com: Channel number %d greater "
                "than %d in <ChannelName> cmd; exiting\n",
		 ChannelName[NchanNames].chan,SM_MAX_CHAN);
         exit(-1);
      }
   /* Get the SCNL name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_STA_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: station name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].sta, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_CHAN_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: component name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].comp, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_NET_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: network name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].net, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_LOC_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: location name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].loc, str);
      }
      NchanNames++;
      return( 1 );
   } 		

   
   return( 0 );
}

/******************************************************************************
 * file2ewfilter()   Do all the work to convert NSMP files to Earthworm msgs  *
 ******************************************************************************/
int file2ewfilter( FILE *fp, char *fname )
{
	K2InfoStruct	*pk2Info;
    char     whoami[50], xmldir[50], fnew[90], xname[150], *cptr, tstr[50];
    char     comment[100];
    long     minute;
	struct Greg  g;
    FILE     *fx;
    int      i, nmsg;
    double   secs, sex, lat, lon;
    double   sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's        */
                                        /* time 0 and 1970-01-01 00:00:00.0 GMT    */


/* Initialize variables
 **********************/
    sprintf(whoami, " %s: %s: ", "sm_file2ew", "nsmp2ew");
    if(!Init) 
		file2ewfilter_init();
		
	/* Allocate the k2Info structure
	 *******************************/
	if ((pk2Info = (K2InfoStruct *) 

	malloc (sizeof (K2InfoStruct))) == NULL) 
	{
		logit ("e", "Could not malloc k2Info structure (%d bytes).\n",
                       sizeof(K2InfoStruct) );
		return -1;
	}

	/* Parse the file 
	 ******************/
	if (k2evt2ew (fp, fname, pk2Info, ChannelName, NchanNames, 
                                NetworkName, Debug) != EW_SUCCESS)
	{
		logit ("e", "Call to k2evt2ew failed -- file not processed.\n");
		free (pk2Info);
		return -1;
	}
	
	/*  THIS DOESN'T MAKE SENSE
	if(strcmp(pk2Info->sm[0].net, "--")==0) 
	{
		logit ("e", "Call to k2evt2ew failed -- Station %s unknown.\n", pk2Info->sm[0].sta);
		free (pk2Info);
		return -1;
	}
	*/

	nmsg = 0;
	for (i = 0; i < pk2Info->head.rwParms.misc.nchannels; i++)
	{

		/* Build TYPE_SM msg from last SM_DATA structure and ship it
		***********************************************************/
		if (wr_strongmotionII( &pk2Info->sm[i], MsgBuf, BUFLEN ) != 0 ) 
		{
		    logit("e","nsmp2ew: Error building TYPE_STRONGMOTIONII msg\n");
            free (pk2Info);
            return( -1 );
        }
    
        if (file2ew_ship( TypeSM2, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) 
        {
            logit("e","nsmp2ew: file2ew_ship error\n");
            free (pk2Info);
            return( -1 );
        }


		/* Build TYPE_SM3 msg from last SM_DATA structure and ship it
		***********************************************************/
		if (wr_strongmotionIII( &pk2Info->sm[i], MsgBuf, BUFLEN ) != 0 ) 
		{
		    logit("e","nsmp2ew: Error building TYPE_STRONGMOTIONIII msg\n");
            free (pk2Info);
            return( -1 );
        }
    logit("et","TYPE_STRONGMOTIONIII \n%s\n",MsgBuf);
		
        if (file2ew_ship( TypeSM3, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) 
        {
            logit("e","nsmp2ew: file2ew_ship error\n");
            free (pk2Info);
            return( -1 );
        }
		/**/
        nmsg = nmsg + 1;
    }




/* Make sure XML subdirectory exists
 ***********************************/
    sprintf (xmldir, "xml");
    if( CreateDir( xmldir ) != EW_SUCCESS ) 
    {
       logit( "e", "%s: trouble creating XML directory: /%s\n",
                  whoami, xmldir );
    } 
    else 
    {
        strcpy(xname, fname);
        cptr = strstr(xname, ".evt");
        *cptr = 0;
        strcat(xname, ".xml");
        sprintf(fnew,"%s/%s",xmldir, xname );
        fx = fopen( fnew, "wb" );
        if( fx == NULL ) 
        {
            logit( "e", "%s: trouble opening XML file: %s\n", whoami, fnew );
            free (pk2Info);
            return( nmsg );
        } 
        else     
        {
            fixstring( pk2Info->head.rwParms.misc.comment, comment);
            if(XMLType==0) {
				fprintf(fx, 
					"<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?> \n");
				fprintf(fx, "<amplitudes agency=\"NSMP\"> \n");
				fprintf(fx, "<record> \n");
				fprintf(fx, "<timing> \n");
				fprintf(fx, "<reference zone=\"GMT\" quality=\"0.5\"> \n");
	
				secs = pk2Info->sm[0].t;
				secs += sec1970;
				minute = (long) (secs / 60.0);
				sex = secs - 60.0 * minute;
				i = sex;
	/*
				j = (sex - i)*1000;
	*/
				grg(minute, &g);
				fprintf(fx, "<year value=\"%4d\"/>",   g.year);
				fprintf(fx, "<month value=\"%2d\"/>",  g.month);
				fprintf(fx, "<day value=\"%2d\"/>",    g.day);
				fprintf(fx, "<hour value=\" %2d\"/>",  g.hour);
				fprintf(fx, "<minute value=\"%2d\"/>", g.minute);
				fprintf(fx, "<second value=\"%2d\"/>", i);
				fprintf(fx, "<msec value=\"0\"/> \n");
				fprintf(fx, "</reference> \n");
				fprintf(fx, "<trigger value=\"0\"/> \n");
				fprintf(fx, "</timing> \n");
	
				lat = pk2Info->head.rwParms.misc.latitude;
				lon = pk2Info->head.rwParms.misc.longitude;
	
				for(i = 0; i < pk2Info->head.rwParms.misc.nchannels; i++) 
				{
					fprintf(fx, 
						"<station code=\"%s\" lat=\"%7.3f\" lon=\"%8.3f\" name=\"%s\"> \n",
							 pk2Info->sm[i].sta, lat, lon, comment);
					fprintf(fx, "<component name=\"%s\"> \n", pk2Info->sm[i].comp);
					fprintf(fx, 
						"<acc value=\"%f\" units=\"cm/s/s\"/> \n", pk2Info->sm[i].pga);
					fprintf(fx, "<vel value=\"%f\" units=\"cm/s\"/> \n", pk2Info->sm[i].pgv);
					fprintf(fx, "<sa period=\"0.3\" value=\"%f\" units=\"cm/s/s\"/> \n", 
														pk2Info->sm[i].rsa[0]);
					fprintf(fx, 
						"<sa period=\"1.0\" value=\"%f\" units=\"cm/s/s\"/> \n", 
														pk2Info->sm[i].rsa[1]);
					fprintf(fx, 
						"<sa period=\"3.0\" value=\"%f\" units=\"cm/s/s\"/> \n", 
														pk2Info->sm[i].rsa[2]);
					fprintf(fx, "</component> \n");
					fprintf(fx, "</station> \n\n");
				}
				fprintf(fx, "</record> \n");
				fprintf(fx, "</amplitudes> \n");
				fclose(fx);
            }
            if(XMLType==1) {
				fprintf(fx, 
					"<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?> \n");
				fprintf(fx, "<peakmotion agency=\"NSMP\"> \n\n");
				
				secs = pk2Info->sm[0].t;
				secs += sec1970;
				minute = (long) (secs / 60.0);
				sex = secs - 60.0 * minute;
				i = sex;
	/*
				j = (sex - i)*1000;
	*/
				grg(minute, &g);
				fprintf(fx, "<PGMTime time=\"%4d-%.2d-%.2dT%.2d:%.2d:%.2d.0Z\" code=\"0\"/> \n\n", 
							g.year, g.month, g.day, g.hour, g.minute, i);
	
				lat = pk2Info->head.rwParms.misc.latitude;
				lon = pk2Info->head.rwParms.misc.longitude;
	
				for(i=0; i<pk2Info->head.rwParms.misc.nchannels; i++) {
					fprintf(fx, 
						"<station site=\"%s\" net=\"%s\" lat=\"%7.3f\" lon=\"%8.3f\" name=\"%s\"> \n",
							 pk2Info->sm[i].sta, pk2Info->sm[i].net, lat, lon, comment);
					fprintf(fx, "   <channel comp=\"%s\" loc=\"%s\"> \n", pk2Info->sm[i].comp, pk2Info->sm[i].loc);
					
					time2string(tstr, pk2Info->sm[i].tpga);
					fprintf(fx, "      <pga value=\"%f\" units=\"cm/s/s\" time=\"%s\"/> \n", pk2Info->sm[i].pga, tstr);
					time2string(tstr, pk2Info->sm[i].tpgv);
					fprintf(fx, "      <pgv value=\"%f\" units=\"cm/s\" time=\"%s\"/> \n", pk2Info->sm[i].pgv, tstr);
					time2string(tstr, pk2Info->sm[i].tpgd);
					fprintf(fx, "      <pgd value=\"%f\" units=\"cm\" time=\"%s\"/> \n", pk2Info->sm[i].pgd, tstr);
					
					fprintf(fx, "      <sa period=\"0.3\" value=\"%f\" units=\"cm/s/s\"/> \n", 
														pk2Info->sm[i].rsa[0]);
					fprintf(fx, "      <sa period=\"1.0\" value=\"%f\" units=\"cm/s/s\"/> \n", 
														pk2Info->sm[i].rsa[1]);
					fprintf(fx, "      <sa period=\"3.0\" value=\"%f\" units=\"cm/s/s\"/> \n", 
														pk2Info->sm[i].rsa[2]);
					fprintf(fx, "   </channel> \n");
					fprintf(fx, "</station> \n\n");
				}
				fprintf(fx, "</peakmotion> \n");
				fclose(fx);
            }
        }
    }

    free (pk2Info);
    return( nmsg );
}


/********************************************************************
 * wr_strongmotionIII()                                             *
 * Reads a SM_INFO structure and writes an ascii TYPE_STRONGMOTION2 *
 * message (null terminated)                                        *
 * Returns 0 on success, -1 on failure (buffer overflow)            *
 ********************************************************************/
int wr_strongmotionIII( SM_INFO *sm, char *buf, int buflen )
{
	char     tmp[256]; /* working buffer */
	char    *qid;
	char    *qauthor;
	int      i;
	time_t     current_time;

   memset( buf, 0, (size_t)buflen );    /* zero output buffer */

/* channel codes */
   sprintf( buf, "SCNL: %s.%s.%s.%s", 
            sm->sta, sm->comp, sm->net, sm->loc );

/* start of record time */
   if( strappend( buf, buflen, "\nWINDOW: " ) ) return ( -1 );
   if( addtimestr( buf, buflen, sm->t ) ) return ( -1 );
   sprintf( tmp, " LENGTH: %.3lf ", (sm->length!=SM_NULL ? ABS(sm->length) : sm->length) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );

/* Print peak acceleration value & time */
   sprintf( tmp, "\nPGA: %.6lf TPGA: ", (sm->pga!=SM_NULL ? ABS(sm->pga) : sm->pga) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->tpga ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: MS" ) ) return( -1 );
      
/* Print peak velocity value & time */
   sprintf( tmp, "\nPGV: %.6lf TPGV: ", (sm->pgv!=SM_NULL ? ABS(sm->pgv) : sm->pgv) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->tpgv ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: IT 5.9" ) ) return( -1 );
      
/* Print peak displacement value & time */
   sprintf( tmp, "\nPGD: %.6lf TPGD: ", (sm->pgd!=SM_NULL ? ABS(sm->pgd) : sm->pgd) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->tpgd ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: IT 5.9" ) ) return( -1 );

/* Print spectral amplitude value & time for period = 0.3 */
   sprintf( tmp, "\nSA: 0.3 %.6lf TSA: ", (sm->rsa[0]!=SM_NULL ? ABS(sm->rsa[0]) : sm->rsa[0]) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->trsa[0] ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: NJ" ) ) return( -1 );

/* Print spectral amplitude value & time for period = 1.0 */
   sprintf( tmp, "\nSA: 1.0 %.6lf TSA: ", (sm->rsa[1]!=SM_NULL ? ABS(sm->rsa[1]) : sm->rsa[1]) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->trsa[1] ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: NJ" ) ) return( -1 );

/* Print spectral amplitude value & time for period = 3.0 */
   sprintf( tmp, "\nSA: 3.0 %.6lf TSA: ", (sm->rsa[2]!=SM_NULL ? ABS(sm->rsa[2]) : sm->rsa[2]) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->trsa[2] ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: NJ" ) ) return( -1 );

/* Print the eventid & event author */
   
   sprintf( tmp, "\nEVID: - -");
   if( strappend( buf, buflen, tmp ) ) return( -1 );

/*   sprintf( tmp, "\nAUTH: %s ", sm->net );  */
   sprintf( tmp, "\nAUTH: %s ", authority );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, time(&current_time) ) ) return ( -1 );
   if( strappend( buf, buflen, "\n" ) ) return( -1 );

   return( 0 );
}


/**********************************************************
 * Converts time (double, seconds since 1970:01:01) to    *
 * a 23-character, null-terminated string in the form of  *
 *            yyyy/mm/dd hh:mm:ss.sss                     *
 * Time is displayed in UTC                               *
 * Target buffer must be 24-chars long to have room for   *
 * null-character                                         *
 **********************************************************/
char *datestr24( double t, char *pbuf, int len )
{
   time_t    tt;       /* time as time_t                  */
   struct tm stm;      /* time as struct tm               */
   int       t_msec;   /* milli-seconds part of time      */

/* Make sure target is big enough
 ********************************/
   if( len < 24 ) return( (char *)NULL );

/* Convert double time to other formats
 **************************************/
   t += 0.0005;  /* prepare to round to the nearest 1000th */
   tt     = (time_t) t;
   t_msec = (int)( (t - tt) * 1000. );
   gmtime_ew( &tt, &stm );

/* Build character string
 ************************/
   sprintf( pbuf,
           "%04d/%02d/%02d %02d:%02d:%02d.%03d",
            stm.tm_year+1900,
            stm.tm_mon+1,
            stm.tm_mday,
            stm.tm_hour,
            stm.tm_min,
            stm.tm_sec,
            t_msec );

   return( pbuf );
}


/********************************************************************
 * addtimestr() append a date string to the end of existing string  *
 *   Return -1 if result would overflow the target,                 *
 *           0 if everything went OK                                *
 ********************************************************************/
int addtimestr( char *buf, int buflen, double t )
{
   char tmp[30];

   if( t == 0.0 )
   {
     if( strappend( buf, buflen, sNullDate ) ) return( -1 );
   } else {
     datestr24( t, tmp, 30 );  
     if( strappend( buf, buflen, tmp ) ) return( -1 );
   }
   return( 0 );
}

/********************************************************************
 * strappend() append second null-terminated character string to    *
 * the first as long as there's enough room in the target buffer    * 
 * for both strings and the null-byte                               *
 ********************************************************************/
int strappend( char *s1, int s1max, char *s2 )
{
   if( (int)strlen(s1)+(int)strlen(s2)+1 > s1max ) return( -1 );
   strcat( s1, s2 );
   return( 0 );
}


/******************************************************************************
 * fixstring()  Remove special characters from string                         *
 *              This is required for XML.                                     *
 ******************************************************************************/
void fixstring( char *istr, char *ostr )
{
	int  i, j;
	
	i = j = 0;
	while(istr[i]) {
		if(istr[i] == '&') {
			strcpy(&ostr[j], "&amp;");
			j += 5;
		} else if(istr[i] == '<') {
			strcpy(&ostr[j], "&lt;");
			j += 4;
		} else if(istr[i] == '>') {
			strcpy(&ostr[j], "&gt;");
			j += 4;
		} else if(istr[i] == '\"') {
			strcpy(&ostr[j], "&quot;");
			j += 6;
		} else if(istr[i] == '\'') {
			strcpy(&ostr[j], "&apos;");
			j += 6;
		} else {
			ostr[j++] = istr[i];
		}
		i++;
	}
	ostr[j] = 0;
}

/******************************************************************************
 * time2string()  convert seconds to xml time                                 *
 ******************************************************************************/
void time2string( char *tstr, double secs )
{
    long     minute;
	struct Greg  g;
    double   sex;
    double   sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's        */
                                        /* time 0 and 1970-01-01 00:00:00.0 GMT    */
	secs += sec1970;
	minute = (long) (secs / 60.0);
	sex = secs - 60.0 * minute;
	grg(minute, &g);
	sprintf(tstr, "%4d-%.2d-%.2dT%.2d:%.2d:%06.3fZ", 
				g.year, g.month, g.day, g.hour, g.minute, sex);

}

/******************************************************************************
 * file2ewfilter_init()  check arguments for NSMP data                        *
 ******************************************************************************/
int file2ewfilter_init( void )
{
   int ret=0;

/* Look up earthworm message type(s)
 ***********************************/
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeSM2 ) != 0 ) {
      logit( "e",
             "nsmp2ew: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n" );
      ret = -1;
   }
   if ( GetType( "TYPE_STRONGMOTIONIII", &TypeSM3 ) != 0 ) {
      logit( "e",
             "nsmp2ew: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n" );
      ret = -1;
   }
   Init = 1;
   return ret;
}


/******************************************************************************
 * file2ewfilter_hbeat()  read heartbeat file, check for trouble              *
 ******************************************************************************/
int file2ewfilter_hbeat( FILE *fp, char *fname, char *system )
{
   return( 0 );
}


/******************************************************************************
 * file2ewfilter_shutdown()  free memory, other cleanup tasks                 *
 ******************************************************************************/
void file2ewfilter_shutdown( void )
{
   free( ChannelName );
   return;
}
