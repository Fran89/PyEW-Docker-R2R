#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <earthworm.h>
#include <trace_buf.h>
#include <kom.h>
#include <read_arc.h>
#include <rw_mag.h>
#include <site.h>
#include <chron3.h>

#include "ewjson.h"


int arc2json( FILE *output, HypoArc *arcmsg, MAG_INFO *magmsg )
{
	int i;						/* Generic counters */
	char stastr[MAX_STA_BUFFER];	/* Buffer for the stations list */
	
	
	/* Create stations list
	 **********************/
	for( i = 0; i < MAX_STA_BUFFER; i++ ) stastr[i] = 0;
	for( i = 0; i < arcmsg->num_phases; i++ )
	{
		
		/* Search if this station has already been inserted
		 **************************************************/
		if( strcmp( stastr, arcmsg->phases[i]->site ) == 0 )
			continue;
			
			
		/* Insert new station in string
		 ******************************/
		if( i > 0 ) strncat( stastr, ",'", MAX_STA_BUFFER );
		else strncat( stastr, "'", MAX_STA_BUFFER );
		strncat( stastr, arcmsg->phases[i]->site, MAX_STA_BUFFER );
		strncat( stastr, "'", MAX_STA_BUFFER );
	}
	
	
	/* Create event JSON string
	 **************************/
	if( magmsg != NULL )
	{
		fprintf( output, 
			"{"
			"'qid':%ld,"
			"'ot':%f,"
			"'lat':%7.4f,"
			"'lon':%8.4f,"
			"'z':%9.3f,"
			"'md':%4.1f,"
			"'ml':%4.1f,"
			"'rms':%5.2f,"
			"'erh':%7.2f,"
			"'erz':%7.2f,"
			"'gap':%d,"
			"'npht':%d,"
			"'nph':%d,"
			"'nphs':%d,"
			"'qual':'%c',"
			"'stations':[%s]"
			"}",
			arcmsg->sum.qid, ( arcmsg->sum.ot - GSEC1970 ) * 1000, 
			arcmsg->sum.lat, arcmsg->sum.lon, arcmsg->sum.z, arcmsg->sum.Mpref,
			magmsg->mag, arcmsg->sum.rms, arcmsg->sum.erh, arcmsg->sum.erz,
			arcmsg->sum.gap, arcmsg->sum.nphtot, arcmsg->sum.nph,
			arcmsg->sum.nphS, 
			ComputeAverageQuality( arcmsg->sum.rms, 
			arcmsg->sum.erh, arcmsg->sum.erz, arcmsg->sum.z, 
			( float ) ( 1.0 * arcmsg->sum.dmin ), arcmsg->sum.nph, 
			arcmsg->sum.gap ), 
			stastr );
	}
	else
	{
		fprintf( output, 
			"{"
			"'qid':%ld,"
			"'ot':%f,"
			"'lat':%7.4f,"
			"'lon':%8.4f,"
			"'z':%9.3f,"
			"'md':%4.1f,"
			"'rms':%5.2f,"
			"'erh':%7.2f,"
			"'erz':%7.2f,"
			"'gap':%d,"
			"'npht':%d,"
			"'nph':%d,"
			"'nphs':%d,"
			"'qual':'%c',"
			"'stations':[%s]"
			"}",
			arcmsg->sum.qid, ( arcmsg->sum.ot - GSEC1970 ) * 1000, 
			arcmsg->sum.lat, arcmsg->sum.lon, arcmsg->sum.z, arcmsg->sum.Mpref,
			arcmsg->sum.rms, arcmsg->sum.erh, arcmsg->sum.erz,
			arcmsg->sum.gap, arcmsg->sum.nphtot, arcmsg->sum.nph,
			arcmsg->sum.nphS, 
			ComputeAverageQuality( arcmsg->sum.rms, 
			arcmsg->sum.erh, arcmsg->sum.erz, arcmsg->sum.z, 
			( float ) ( 1.0 * arcmsg->sum.dmin ), arcmsg->sum.nph, 
			arcmsg->sum.gap ), 
			stastr );
	}
	
	return 1;
}






int site2json( FILE *output )
{
	int i, f;
	char line[500];
	
	typedef struct{
		char name[TRACE2_STA_LEN];
		char channels[100][TRACE2_CHAN_LEN];
		int	nChan;
		double lat;
		double lon;
		double elev;
	} SSTATION;
	int		nSta = 0;
	SSTATION	*stations = ( SSTATION* ) malloc( sizeof( SSTATION ) * nSite );
	if( stations == NULL )
	{
		logit( "e", "Unable to allocate memory for site2json\n" );
		return -1;
	}
	

	for( i = 0; i < nSite; i++ )
	{
		/* Check if station is already on the list
		 *****************************************/
		int stapos = -1;
		for( f = 0; f < nSta; f++ )
		{
			if( strcmp( stations[f].name, Site[i].name ) == 0 )
			{
				stapos = f;
				break;
			}
		}
		
		if( stapos == -1 )
		{
			/* This is a new station
			 ***********************/
			strcpy( stations[nSta].name, Site[i].name );
			strcpy( stations[nSta].channels[0], Site[i].comp );
			stations[nSta].lat = Site[i].lat;
			stations[nSta].lon = Site[i].lon;
			stations[nSta].elev = Site[i].elev;
			stations[nSta].nChan = 1;
			nSta++;
		}
		else
		{
			/* This station was already introduced
			 *************************************/
			strcpy( stations[stapos].channels[stations[stapos].nChan], Site[i].comp );
			stations[stapos].nChan = stations[stapos].nChan + 1;
		}
	}
	
	
	/* Create json
	 *************/
	for( i = 0; i < nSta; i++ )
	{
		if( i == 0 )
			fprintf( output, "[" );
		else
			fprintf( output, ",\n" );
		
		sprintf( line, "{"
				"'name':'%s',"
				"'lat':%7.3f,"
				"'lon':%8.3f,"
				"'elev':%5.2f,'channels':[",
				stations[i].name, stations[i].lat, stations[i].lon, stations[i].elev );
		for( f = 0; f < stations[i].nChan; f++ )
		{
			strcat( line, "'" );
			strcat( line, stations[i].channels[f] );
			strcat( line, "'" );
			if( f < ( stations[i].nChan - 1 ) ) strcat( line, "," );
		}
		strcat( line, "]}" );
		fprintf( output, "%s", line );
	}
	fprintf( output, "\n]" );
	fflush( output );
	free( stations );
	return( 0 );
}













