/******************************************************************************
 *
 *	File:			compute_gm.c
 *
 *	Function:		Post a message to request gmew processing
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			compute_spectra.c
 *
 *  Usage: compute_gm <Ring> <start>
 *             where <start> is YYYYMMDDHHMMSS
 *
 *	Change History:
 *			5/03/11	Started source
 *	
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <earthworm.h>
#include <earthworm_defs.h>
#include <math.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <ew_spectra_io.h>

char *progname;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */

static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

MSG_LOGO  GetLogo;     			/* requesting module,type,instid */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */
	

int main(int argc, char **argv)
{
/* Other variables: */
   MSG_LOGO      logo;   /* logo of retrieved message             */
   char outMsg[100];
   double start;
   unsigned char gmModId;
   int getm, gett, getm2;
	
	/* Look up installations of interest
	************************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
	  fprintf( stderr, "error getting local installation id; exiting!\n" );
	  exit( -1 );
	}
	
	if ((progname= strrchr(*argv, (int) '/')) != (char *) 0)
		progname++;
	else
		progname= *argv;
	
    /* Check command line arguments */
    if (argc != 3) {
		fprintf (stderr, "Usage: %s ring start\n", progname);
		return EW_FAILURE;
    }
    	
	if ( ( OutRingKey = GetKey(argv[1]) ) == -1 ) {
		fprintf( stderr, "Invalid ring name <%s>; exiting!\n", argv[1]);
		exit( -1 );
   	}

	start = atof( argv[2] );
	if ( start < 0 ) { 
		start += time(NULL);
	} else if ( EWSConvertTime (argv[2], &start) == EW_FAILURE ) {
		fprintf( stderr, "illegal start time (%s); exiting!\n", argv[2]);
		exit( -1 );
	}

	/* Build logo
	*************/
	logo.instid = InstId;
	getm = GetModId( "MOD_WILDCARD", &(logo.mod) );
	getm2 = GetModId( "MOD_GMEW", &gmModId );
	gett = GetType( "TYPE_ACTIVATE_MODULE", &(logo.type) );
	if ( getm || getm2 || gett ) {
		if ( getm )
			fprintf( stderr, "MOD_WILDCARD unknown\n" );
		if ( getm2 )
			fprintf( stderr, "MOD_GMEW unknown\n" );
		if ( gett )
			fprintf( stderr, "TYPE_ACTIVATE_MODULE unknown\n" );
		fprintf( stderr, "one or more missing definitions; exiting!\n" );
		exit( -1 );
	}

	/* Attach to output ring
	************************/
	tport_attach( &OutRegion, OutRingKey );
	
	/* Write out message
	********************/
	sprintf( outMsg, "%d %s%c", gmModId, argv[2], 0 );
   	if( tport_putmsg( &OutRegion, &logo, (long) (strlen(outMsg)+1), outMsg ) != PUT_OK )
        fprintf( stderr, "Error posting message\n" );
    else
    	printf( "Writing '%s'\n", outMsg );
		
	/* Clean up after ourselves
	************************/
	tport_detach( &OutRegion );
        exit(0);
}
