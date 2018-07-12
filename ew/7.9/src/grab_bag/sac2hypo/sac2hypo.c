  
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sac2hypo.c,v 1.1.1.1 2005/07/14 20:06:46 paulf Exp $
 *
 *    Revision history:
 *     $Log: sac2hypo.c,v $
 *     Revision 1.1.1.1  2005/07/14 20:06:46  paulf
 *     Local ISTI CVS copy of EW v6.3
 *
 *     Revision 1.12  2001/05/16 17:52:13  lucky
 *     Fixed to work with the new EWEvent structure naming convention
 *
 *     Revision 1.11  2001/05/16 17:11:29  lucky
 *     Fixed to work with the new naming scheme; also moved under the oracle
 *     source tree because this module uses converted routines which rely on the
 *     mondo database event structure (EW_Event).
 *
 *     Revision 1.10  2001/04/17 17:19:57  davidk
 *     added #include of Sac2DBEvent.h header file, and removed an unused local
 *     variable (i), to eliminate compiler warnings on NT.
 *
 *     Revision 1.9  2001/02/28 17:19:03  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.8  2000/10/10 20:13:16  lucky
 *     modified the call to InitDBEvent to reflect the new pChan allocation scheme
 *
 *     Revision 1.7  2000/08/30 18:23:16  lucky
 *     Cleanup debug messages
 *
 *     Revision 1.6  2000/08/30 15:20:16  lucky
 *     Added a call to InitDBEvent (which used to be in Sac2DBEvent where it
 *     does not belong).
 *
 *     Revision 1.5  2000/08/28 15:52:09  lucky
 *     get_db_event_info.h -> db_event_info.h
 *
 *     Revision 1.4  2000/08/15 19:00:53  lucky
 *     Removed InitDBEvent -- it is now included in the conversion routines themselves.
 *
 *     Revision 1.3  2000/05/26 21:40:39  lucky
 *     *** empty log message ***
 *
 *     Revision 1.2  2000/05/02 15:41:19  lucky
 *     Major rewrite - using DBEventSacArc for the routines to translate
 *     between Sac and DBstructs, and then from DBstructs to ARC messages.
 *     readsac stuff is no longer used.
 *
 *     Revision 1.1  2000/02/14 19:13:13  lucky
 *     Initial revision
 *
 *
 */

/*  sac2hypo.c
 *  This interactive program reads the headers of all the SAC files
 *  in the current directory and creates a Hypoinverse archive file.
 *  This archive file can be used to relocate the event.
 *  	Modified for hyp2000 format	BB 1/25/99
 *
 *   Transfered into Earthworm source tree - made to compile as 
 *   an earthworm utility (standalone) program.   LV  8/12/1999 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <earthworm.h>
#include <ws_clientII.h>
#include <sachead.h>
#include <time_ew.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <sac_2_ewevent.h>

#define MAXNAME 64

extern int errno;   /* system error variable */

/* Function prototypes
 *********************/

extern	int     SacHeaderInit (struct SAChead *);
extern	int		Sac2Origin (EWDB_OriginStruct *, struct SAChead *);
extern	int		Sac2Channel (EWChannelDataStruct *, int, struct SAChead *, char *, int);



#define		MAX_SAC_DATA		5000000     /* SAC data cannot be bigger than this */

/* Define max lengths of various lines in the Hypoinverse archive format
   (includes space for a newline & null-terminator on each line )
 ************************************************************************/
#define ARC_HYP_LEN   190
#define ARC_SHYP_LEN   96
#define ARC_PHS_LEN   102
#define ARC_SPHS_LEN  105
#define ARC_TRM_LEN    74
#define ARC_STRM_LEN   74


main (int argc, char **argv)
{

   char           *sacfilelist = "saclist";
   char           *outfilebase = "Hypo.arc.in";
   char            outfilename[MAXNAME+1];
   char            infile[MAXNAME+1];
   char            line[MAXNAME+1];
   FILE           *flist;
   FILE           *fin;         
   FILE           *fout;
   char           *outbuf=(char *)NULL;   /* to be malloc'd for converted data */
   size_t          outbuflen=0;           /* length of this outbuf             */
   int             eventid;
   int             len;
   int             rc;
	EWEventInfoStruct	DBEvent;
	struct SAChead 		*psachd;				/* pointer to SAC header structure   */
	char 				sachd[SACHEADERSIZE]; /* buffer to hold a SAC header       */
	char				sacdata[MAX_SAC_DATA];
	char				*psacdata;


	/* Initialize pointers
	 *********************/
	psachd = (struct SAChead *) sachd;
	psacdata = (char *) sacdata;


	/* Initialize SAC header
	 *************************/
	if (SacHeaderInit (psachd) != EW_SUCCESS)
	{
		fprintf (stderr, "Call to SacHeaderInit failed -- exiting.\n");
		return EW_FAILURE;
	}

    /* initialize the Event structure  */
	if (InitEWEvent (&DBEvent) != EW_SUCCESS)
	{
		fprintf (stderr, "Call to InitEWEvent failed.\n");
		return EW_FAILURE;
	}

	/* Open filename list 
	 ********************/
	if ((flist = fopen (sacfilelist, "r")) == NULL)
	{
		fprintf (stderr, "Cannot open file: %s, exiting!\n", sacfilelist );
		return EW_FAILURE;
	}

	/* Read the eventid from the 1st line of the file list
	 *****************************************************/
	fgets (line, MAXNAME, flist);
	if (sscanf (line, "EVENTID:%d", &eventid ) != 1) 
	{
		fprintf (stderr, "Cannot read eventid from: %s in file %s; exiting!\n", 
											line, sacfilelist );
		fclose (flist);
		return EW_FAILURE;
	}

	/* Loop over all SAC files in list
	 *********************************/
	while (fgets (line, MAXNAME, flist) != NULL)
	{
		if (sscanf (line, "%s", infile) != 1) 
		{
			fprintf (stderr, 
				"Cannot read sacfilename from file: %s, exiting!\n", sacfilelist);
			fclose (flist);
			return EW_FAILURE;
		}

		/* open this SAC file; read its header and data
		 ***********************************************/
		if ((fin = fopen (infile, "rb")) == NULL)
		{
			fprintf (stderr, "Cannot open file: %s, exiting!\n", infile);
			fclose (flist);
			return EW_FAILURE;
		}
	
		if (fread (sachd, sizeof(char), (size_t)SACHEADERSIZE, fin)
												!= (size_t) SACHEADERSIZE)
		{
			fprintf (stderr, "Error reading header from file: %s, exiting!\n", infile);
			fclose (fin);
			fclose (flist);
			return EW_FAILURE;
		}
	
		if (fread (sacdata, sizeof(float), (size_t) psachd->npts, fin)
												!= (size_t) psachd->npts)
		{
			fprintf (stderr, "Error reading %d bytes of data from file: %s, exiting!\n", 
											psachd->npts, infile);
			fclose (fin);
			fclose (flist);
			return EW_FAILURE;
		}

		fclose (fin);


		DBEvent.PrefOrigin.idEvent = eventid;
		DBEvent.Event.idEvent = eventid;

		/* Get gory details for this event from SAC header 
	    *************************************************/
		if (Sac2EWEvent (&DBEvent, psachd, psacdata, 0) != EW_SUCCESS)
		{
			fprintf (stderr, "Call to Sac2EWEvent failed -- continuing.\n");
		}
		else
		{
			DBEvent.iNumChans = DBEvent.iNumChans + 1;
		}

	} /* while loop over files in the sacfilelist */

	fclose (flist);

	/* Create a Hypoinverse Archive message
	 ****************************************/

	/* malloc space, convert it all to an arc message
	 ************************************************/
	outbuflen = ARC_HYP_LEN + ARC_SHYP_LEN +            			/* hypocenter + shadow */
				DBEvent.iNumChans * (ARC_PHS_LEN + ARC_SPHS_LEN) +  /* phases + shadows    */
				ARC_TRM_LEN + ARC_STRM_LEN;             			/* terminator + shadow */

	outbuf = (char *) malloc ((size_t) outbuflen);
	if (outbuf == (char *)NULL)
	{
		fprintf (stderr, "Trouble allocating %d byte buffer for output\n", outbuflen);
		return EW_FAILURE;
	}
	outbuf[0]='\0';


	if (EWEvent2ArcMsg (&DBEvent, outbuf, outbuflen) != EW_SUCCESS)
	{
		fprintf (stderr, "Call to EWEvent2ArcMsg failed.\n");
		return EW_FAILURE;
	}


/* Write the arc message to a file
 *********************************/
   sprintf (outfilename, "%s.%d", outfilebase, eventid);
   fout = fopen( outfilename, "w" );
   if( fout == (FILE *)NULL )
   {
      printf("cannot open output file: %s, exiting!\n", outfilename );
      return( -1 );
   }
   len = strlen( outbuf );
   rc = fwrite( outbuf, sizeof(char), len, fout );
   if( rc != len )
   {
      printf("error writing archive output file: %s, exiting!\n", outfilename );
   }
   fclose( fout );
   free( outbuf );
   printf("Hypoinverse archive format written to: %s\n", outfilename );

   
   return( 0 );
}


