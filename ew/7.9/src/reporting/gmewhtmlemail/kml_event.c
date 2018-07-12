/* write this format out along with the kml_preamble to a specified filename

        <Folder id="layer 0">
                <name>EW Event ID 45</name>
                <Placemark>
                        <styleUrl>#pointStyleMap</styleUrl>
                        <ExtendedData>
                                <SchemaData schemaUrl="#HypoSummary">
                                        <SimpleData name="Origin">1/1/2012</SimpleData>
                                        <SimpleData name="Lat">36.9087</SimpleData>
                                        <SimpleData name="Long">-104.7755</SimpleData>
                                        <SimpleData name="Depth">13.2</SimpleData>
                                        <SimpleData name="MAG">2.7</SimpleData>
                                        <SimpleData name="Number_Picks">13</SimpleData>
                                        <SimpleData name="Azimuthal_Gap">289</SimpleData>
                                        <SimpleData name="Nearest_Station">15.6</SimpleData>
                                        <SimpleData name="RMS">0.24</SimpleData>
                                        <SimpleData name="Error_H">4.2</SimpleData>
                                        <SimpleData name="Error_Z">0.7</SimpleData>
                                        <SimpleData name="Quality">C</SimpleData>
                                </SchemaData>
                        </ExtendedData>
                        <Point>
                                <coordinates>-104.7755,36.9087,0</coordinates>
                        </Point>
                </Placemark>
        </Folder>
*/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "read_arc.h"
#include "site.h"
#include "earthworm.h"
#include "chron3.h"

#define MAX_STR 1024
void kml_station(FILE *fp, SITE *site);	/* defined at end of file, but used by kml_writer */

/* 
kml_writer() - requires a specific preamble file containing all the XML BS

args: 
	dir = directory name where to write the kml file
	sum = Hypo2000 SUM struct pointer
	preamble = filename  of preamble KML file, specific to this function
	output_filename = an array where to write the name of the file upon output
	**sites = an array of SITE structs that were used in the solution
        nsites = the number of sites to be found.

build a single event KML file for a summary struct.

returns 0 upon success, -1 upon any problems 

Caveat Emptor, this is NOT PRETTY, I needed a quick one off.

****************************************************************/
int kml_writer(char * dir, struct Hsum *sum, char * preamble, char *output_filename, SITE **sites, int nsites) {
char outkml[MAX_STR];
char timestr[MAX_STR];
char Quality,ch;
time_t ot;
struct tm *timeinfo;
FILE *kmlfp, *tmpfp;

         output_filename[0] = 0;
         sprintf(outkml, "%s/qid%ld.kml", dir, sum->qid);
         if (!(kmlfp = fopen(outkml, "w"))) {
             logit("e", "kml_writer(): could not open output file %s\n", outkml);
             return(-1);
         }
         
 
         if (tmpfp=fopen(preamble,"r"))
         {
         while(!feof(tmpfp))
         {
            ch = getc(tmpfp);
            if(ferror(tmpfp))
            {
               logit("e","kml_writer(): Read kml preamble error\n");
               clearerr(tmpfp);
               fclose(kmlfp);
               fclose(tmpfp);
               return(-1);
            }
            else
            {
               if(!feof(tmpfp)) putc(ch, kmlfp);
               if(ferror(kmlfp))
               {
                  logit("e","kml_writer(): Write kml preamble error\n");
                  clearerr(kmlfp);
                  fclose(kmlfp);
                  fclose(tmpfp);
                  return(-1);
               }
            }
         }
         fclose(tmpfp);
         }
         else
         {
            logit("et","kml_writer(): Unable to open kml preamble file\n");
            fclose(kmlfp);
            return(-1);
         }

/* now start writing all the KML stuff */
         ot = (time_t)(sum->ot - GSEC1970);
         timeinfo = gmtime ( &ot );
         strftime (timestr,80,"%Y.%m.%d %H:%M:%S",timeinfo);

         fprintf(kmlfp, "<Folder id=\"layer 0\">\n");
         fprintf(kmlfp, "\t<name>EW Event ID %ld</name>\n", sum->qid);
         fprintf(kmlfp, "\t<Placemark>\n");
         fprintf(kmlfp, "\t<name>OT%s - Md%5.2f</name>\n", timestr, sum->Mpref);
         fprintf(kmlfp, "\t<styleUrl>#pointStyleMap</styleUrl>\n");
         fprintf(kmlfp, "\t<ExtendedData>\n");
         fprintf(kmlfp, "\t<SchemaData schemaUrl=\"#HypoSummary\">\n");

         fprintf(kmlfp, "\t\t<SimpleData name=\"Origin\">%s</SimpleData>\n", timestr);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Lat\">%f</SimpleData>\n", sum->lat);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Long\">%f</SimpleData>\n", sum->lon);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Depth\">%f</SimpleData>\n", sum->z);
         fprintf(kmlfp, "\t\t<SimpleData name=\"MAG\">%5.2f</SimpleData>\n", sum->Mpref);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Number_Picks\">%d</SimpleData>\n", sum->nph);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Azimuthal_Gap\">%d</SimpleData>\n", sum->gap);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Nearest_Station\">%d</SimpleData>\n", sum->dmin);
         fprintf(kmlfp, "\t\t<SimpleData name=\"RMS\">%5.2f</SimpleData>\n", sum->rms);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Error_H\">%6.2f</SimpleData>\n", sum->erh);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Error_Z\">%6.2f</SimpleData>\n", sum->erz);
         Quality = ComputeAverageQuality(sum->rms, sum->erh, sum->erz, sum->z, 
			(float) (1.0*sum->dmin), sum->nph, sum->gap);
         fprintf(kmlfp, "\t\t<SimpleData name=\"Quality\">%c</SimpleData>\n", Quality);


/* close it out */
         fprintf(kmlfp, "\t</SchemaData>\n");
         fprintf(kmlfp, "\t</ExtendedData>\n");
/* put in the location tag */
         fprintf(kmlfp, "\t<Point>\n");
         fprintf(kmlfp, "\t\t<coordinates>%f,%f,0</coordinates>\n", sum->lon, sum->lat);
         fprintf(kmlfp, "\t</Point>\n");
/* close out the event doc */
         fprintf(kmlfp, "\t</Placemark>\n");

/* stuff the station in a new sub folder if desired */
         if (nsites > 0) {
             int i;
             fprintf(kmlfp, "\t<Folder id=\"stations\">\n");
             fprintf(kmlfp, "\t\t<name id=\"SWP\"> Stations with Picks </name>\n");
             for(i=0; i < nsites; i++) {
                  kml_station(kmlfp, sites[i]);
             }
             fprintf(kmlfp, "</Folder>\n");
         }
 

/* close out the containing folder and doc  and return created filename */
         fprintf(kmlfp, "</Folder>\n");
         fprintf(kmlfp, "</Document>\n");
         fprintf(kmlfp, "</kml>\n");
         fclose(kmlfp);
         strcpy(output_filename, outkml);
	 return(0);
}

/* write out placemarks like this one below for a single station:

                <styleUrl>#msn_triangle23</styleUrl>
                <Point>
                        <altitudeMode>clampToGround</altitudeMode>
                        <gx:altitudeMode>clampToSeaFloor</gx:altitudeMode>
                        <coordinates>-104.9216328752934,37.2921999498278,9.999999999999998</coordinates>
                </Point>
*/

void kml_station(FILE *kmlfp, SITE *site) {
         fprintf(kmlfp, "\t\t\t<Placemark>\n");
         fprintf(kmlfp, "\t\t\t\t<name>%s</name>\n", site->name);
         fprintf(kmlfp, "\t\t\t\t<styleUrl>#msn_triangle23</styleUrl>\n");
         fprintf(kmlfp, "\t\t\t\t<Point>\n");
         fprintf(kmlfp, "\t\t\t\t\t<altitudeMode>clampToGround</altitudeMode>\n");
         fprintf(kmlfp, "\t\t\t\t\t<gx:altitudeMode>clampToSeaFloor</gx:altitudeMode>\n");
         fprintf(kmlfp, "\t\t\t\t\t<coordinates>%f,%f,0</coordinates>\n", site->lon, site->lat);
         fprintf(kmlfp, "\t\t\t\t</Point>\n");
         fprintf(kmlfp, "\t\t\t</Placemark>\n");
}
