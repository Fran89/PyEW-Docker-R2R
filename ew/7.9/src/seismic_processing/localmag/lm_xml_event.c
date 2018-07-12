/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_xml_event.c 3857 2010-03-15 16:22:50Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2010/03/15 16:22:50  paulf
 *     merged from Matteos branch for adding version to directory naming
 *
 *     Revision 1.3.2.1  2010/03/15 15:15:56  quintiliani
 *     Changed output file and directory name
 *     Detailed description in ticket 22 from
 *     http://bigboy.isti.com/trac/earthworm/ticket/22
 *
 *     Revision 1.3  2008/08/12 07:03:18  quintiliani
 *     Writing correct value of magnitude.
 *     When the parameter "useMedian" is declared  in the configuration file,
 *     the function xml_event_write() uses the value "pEvt->magMed"  instead of "pEvt->mag".
 *
 *     Revision 1.2  2007/03/30 14:14:05  paulf
 *     added saveXMLdir option
 *
 *     Revision 1.1  2007/03/29 20:09:50  paulf
 *     added eventXML option from INGV. This option allows writing the Shakemap style event information out as XML in the SAC out dir
 *
 *     Revision 1.1  2005/11/16 09:26:49  mtheo
 *     Added xml_event_write()
 *     	write shakemap event XML file
 *
 *
 */
/*
 * lm_xml_event.c: write shakemap event XML file.
 * Author: Matteo Quintiliani - quintiliani@ingv.it
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lm_xml_event.h"
#include "lm_version.h"

/* xml_event_write()
 * Write shakemap event XML file
 * if it fails returns 0
 */
int xml_event_write( EVENT *pEvt, LMPARAMS *plmParams ) {
#define N_STRING  25
	FILE *f;
	int ret = 1;
	const char xml_string[N_STRING][80] = {
		"<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>",
		"<!DOCTYPE earthquake [",
		"<!ELEMENT  earthquake EMPTY>",
		"<!ATTLIST earthquake",
		"  id            ID      #REQUIRED",
		"  lat           CDATA   #REQUIRED",
		"  lon           CDATA   #REQUIRED",
		"  mag           CDATA   #REQUIRED",
		"  year          CDATA   #REQUIRED",
		"  month         CDATA   #REQUIRED",
		"  day           CDATA   #REQUIRED",
		"  hour          CDATA   #REQUIRED",
		"  minute        CDATA   #REQUIRED",
		"  second        CDATA   #REQUIRED",
		"  timezone      CDATA   #REQUIRED",
		"  depth         CDATA   #REQUIRED",
		"  locstring     CDATA   #REQUIRED",
		"  pga           CDATA   #REQUIRED",
		"  pgv           CDATA   #REQUIRED",
		"  sp03          CDATA   #REQUIRED",
		"  sp10          CDATA   #REQUIRED",
		"  sp30          CDATA   #REQUIRED",
		"  created       CDATA   #REQUIRED",
		">",
		"]>"
	};

	char locstring[500];
	int i;
	char  outfile[PATH_MAX];
	char timezone[10] = "GMT";
	time_t orig = (time_t) pEvt->origin_time;
	struct tm *tm_orig = gmtime(&orig);

        sprintf(locstring, "Earthworm localmag module - %s", LOCALMAG_VERSION);

	if (plmParams->saveXMLdir != NULL) {
		snprintf(outfile, PATH_MAX, "%s/%s_%d_event.xml", plmParams->saveXMLdir, pEvt->eventId, pEvt->origin_version );
        } else if (plmParams->sacOutDir != NULL) {
		snprintf(outfile, PATH_MAX, "%s/%s_%d_event.xml", plmParams->sacOutDir, pEvt->eventId, pEvt->origin_version );
	} else {
		logit("et", "xml_event_write: Unable to write ShakeMap XML event file: SAC out dir and saveXMLdir not set!\n" );
		return( -1 );
	}

	if ( (f = fopen( outfile, "wt")) == (FILE *)NULL)
	{
		logit("et", "xml_event_write: Unable to open ShakeMap XML event file for writing: %s\n", outfile );
		return( -1 );
	}   

	for(i=0; i<N_STRING; i++) {
		fprintf(f, "%s\n", xml_string[i]);
	}

       /* this if added by MO to prevent printing of Ml when the minimum number of stations has not been reached */
   if (pEvt->nMags < plmParams->minStationsMl)
     {

	fprintf(f,"<earthquake id=\"%s\" lat=\"%.4f\" lon=\"%.4f\" mag=\"\" year=\"%04d\" month=\"%02d\" day=\"%02d\" hour=\"%02d\" minute=\"%02d\" second=\"%02d\" timezone=\"%s\" depth=\"%2.2f\" locstring=\"%s \" created=\"%s\" />\n",
			pEvt->eventId, pEvt->lat, pEvt->lon,
			tm_orig->tm_year+1900, tm_orig->tm_mon+1, tm_orig->tm_mday,
			tm_orig->tm_hour, tm_orig->tm_min, tm_orig->tm_sec, timezone,
			pEvt->depth, locstring, pEvt->author);
         }
	else
         {
	fprintf(f,"<earthquake id=\"%s\" lat=\"%.4f\" lon=\"%.4f\" mag=\"%1.1f\" year=\"%04d\" month=\"%02d\" day=\"%02d\" hour=\"%02d\" minute=\"%02d\" second=\"%02d\" timezone=\"%s\" depth=\"%2.2f\" locstring=\"%s \" created=\"%s\" />\n",
			pEvt->eventId, pEvt->lat, pEvt->lon,
			(plmParams->useMedian == 1)? pEvt->magMed : pEvt->mag,
			tm_orig->tm_year+1900, tm_orig->tm_mon+1, tm_orig->tm_mday,
			tm_orig->tm_hour, tm_orig->tm_min, tm_orig->tm_sec, timezone,
			pEvt->depth, locstring, pEvt->author);

         }

	fclose(f);

	return ret;
}
