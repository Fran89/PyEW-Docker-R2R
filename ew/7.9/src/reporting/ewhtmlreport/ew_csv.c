#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <earthworm.h>
#include <read_arc.h>
#include <rw_mag.h>
#include <chron3.h>


void csv_header(FILE *output)
{
	fprintf(output, "QuakeID,OriginTime(UTC),Latitude,Longitude,DepthBSL(km),Md,Ml,Ml_err,RMS,ErrorH(km),ErrorZ(km),Gap,TotalPhases,UsedPhases,SPhasesUsed,Quality\n");
}

int arc2csv( FILE *output, HypoArc *arcmsg, MAG_INFO *magmsg )
{
	char timestr[30];	
	char frac_sec_str[30];	
	time_t ot;
	struct tm       *timeinfo;
        double frac_sec_real;


	
	
	ot = ( time_t )( arcmsg->sum.ot - GSEC1970 );
	timeinfo = gmtime ( &ot );
       	strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", timeinfo ); /* Prepare origin time */
	frac_sec_real = arcmsg->sum.ot - floor(arcmsg->sum.ot);
        sprintf(frac_sec_str, "%5.3f", frac_sec_real); /* build a string for the fractional seconds */
        strcat(timestr, &frac_sec_str[1]); 	/* only print out the .NNN of the frac second */
	
	/* Create event CSV string
	 **************************/
	fprintf( output, "%ld,%s,%7.4f,%8.4f,%9.3f,%4.1f,",
		arcmsg->sum.qid, timestr, arcmsg->sum.lat, arcmsg->sum.lon, arcmsg->sum.z, arcmsg->sum.Mpref);
	if( magmsg != NULL )
	{

		fprintf( output, "%4.1f,%4.2f,",  magmsg->mag, magmsg->error);
	}
	else
	{
		fprintf( output, "NA,NA,");
	}
	fprintf( output, "%5.2f,%7.2f,%7.2f,%d,%d,%d,%d,%c\n",
			arcmsg->sum.rms, arcmsg->sum.erh, arcmsg->sum.erz,
			arcmsg->sum.gap, arcmsg->sum.nphtot, arcmsg->sum.nph,
			arcmsg->sum.nphS, ComputeAverageQuality( arcmsg->sum.rms, 
			arcmsg->sum.erh, arcmsg->sum.erz, arcmsg->sum.z, 
			( float ) ( 1.0 * arcmsg->sum.dmin ), arcmsg->sum.nph, 
			arcmsg->sum.gap ) );
	return 1;
}
