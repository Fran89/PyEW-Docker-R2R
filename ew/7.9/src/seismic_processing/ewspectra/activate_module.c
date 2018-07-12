/******************************************************************************
 *
 *	File:			activate_module.c
 *
 *	Function:		Post a message to a specified module
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			Started anew.
 *
 *  Usage: activate_module <ModuleID> <Ring> [<arg> [<arg> ...]]
 *
 *	Change History:
 *			5/05/11	Started source
 *	
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <earthworm.h>
#include <math.h>
#include <string.h>
#include <kom.h>
#include <ew_spectra.h>
#include <transport.h>
#include <ew_spectra_io.h>

char *progname;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */

static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

MSG_LOGO  GetLogo;     			/* requesting module,type,instid */
/*pid_t MyPid;         Our own pid, sent with heartbeat for restart purposes */
	
int static long    OutMsgSize;          /* size for output msgs    */

int main(int argc, char **argv)
{
/* Other variables: */
   	MSG_LOGO      logo;   /* logo of retrieved message             */
   	char *outMsg, *sptr, midStr[20];
   	int i;
   	unsigned char outModID;
	int getm, gett;
	
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
    if (argc < 3) {
		fprintf (stderr, "Usage: %s moduleid ring [arg [arg ...]]\n", progname);
		return EW_FAILURE;
    }
    
    if ( GetModId( argv[1], &outModID ) ) {
		fprintf( stderr, "ModuleID '%s' unknown; exiting!\n", argv[1] );
		exit( -1 );
   	}

	if ( ( OutRingKey = GetKey(argv[2]) ) == -1 ) {
		fprintf( stderr, "Invalid ring name <%s>; exiting!\n", argv[2]);
		exit( -1 );
   	}

	/* Build logo
	*************/
	logo.instid = InstId;
	getm = GetModId( "MOD_ACTIVATE", &(logo.mod) );
	gett = GetType( "TYPE_ACTIVATE_MODULE", &(logo.type) );
	if ( getm || gett ) {
		if ( getm )
			fprintf( stderr, "MOD_ACTIVATE unknown, set it in your earthworm.d to use this module\n" );
		if ( gett )
			fprintf( stderr, "TYPE_ACTIVATE_MODULE unknown\n" );
		fprintf( stderr, "one or more missing definitions; exiting!\n" );
		exit( -1 );
	}

	/* Attach to output ring
	************************/
	tport_attach( &OutRegion, OutRingKey );
	
	/* Build & write out message
	****************************/
	sprintf( midStr, "%d", outModID );
	OutMsgSize = strlen(midStr);
	for ( i=3; i<argc; i++ ) {
		if ( strchr( argv[i], ' ' ) || strchr( argv[i], '\t' ) ) {
			fprintf( stderr, "Arg %d contains whitespace; exiting!\n", i-2 );
			exit(-1);
		}
		OutMsgSize += strlen(argv[i])+1;
	}
	outMsg = malloc( OutMsgSize );
	if ( outMsg == NULL ) {
		fprintf( stderr, "Failed to allocate space for message; exiting!\n" );
		exit(-1);
	}
	sprintf( outMsg, "%d", outModID );
	sptr = outMsg+strlen(midStr);
	for ( i=3; i<argc; i++ ) {
		char *aptr = argv[i];
		*(sptr++) = ' ';
		for ( ; *aptr; aptr++, sptr++ )
			*sptr = *aptr;
	}
	*sptr = 0;
   	if( tport_putmsg( &OutRegion, &logo, OutMsgSize+1, outMsg ) != PUT_OK )
        fprintf( stderr, "Error posting message\n" );
    else
    	printf( "Writing '%s'\n", outMsg );
		
	/* Clean up after ourselves
	************************/
	tport_detach( &OutRegion );
	return( 0 );
}
