/******************************************************************************
 *
 *	File:			sniffspectra.c
 *
 *	Function:		Program to read spectra  messages from shared memory
 *                  and write to a disk file or stdout.
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			Started anew.
 *
 *  Usage: sniffspectra  <Ring>
 *           (to write headers to stdout)
 *    or: sniffspectra  <Ring> -d
 *           (to write headers & data to stdout)
 *    or: sniffspectra  <Ring> -f [dir]
 *           (to write each message to a file in dir if provided,
 *            current directory if not)
 *
 *	Change History:
 *			4/26/11	Started source
 *	
 *****************************************************************************/



  /*****************************************************************
   *                            sniffspectra.c                     *
   *                                                               *
   * Program to read spectra  messages from shared memory          *
   * and write to a disk file or stdout.                           *
   *                                                               *
   *                                                               *
   *                                                               *
   *****************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <transport.h>
#include <swap.h>
#include <time_ew.h>
#include <earthworm.h>
#include <ew_spectra.h>
#include <ew_timeseries.h>
#include <ew_spectra_io.h>

#define NLOGO 4
#define VERSION "1.0.1 2012-02-13"

int IsWild( char *str )
{
  if( (strcmp(str,"wild")==0)  ) return 1;
  if( (strcmp(str,"WILD")==0)  ) return 1;
  if( (strcmp(str,"*")   ==0)  ) return 1;
  return 0;
}

/************************************************************************************/
/* Definition for checking SCNL gaps and latencies                                  */
/************************************************************************************/

/* Max length for a SCNL string */
#define MAX_LEN_STR_SCNL 30

/* Structure for storing channel information, last scnl time and last packet time */
typedef struct {
    char scnl[MAX_LEN_STR_SCNL];
    double last_scnl_time;
    double last_packet_time;
} SCNL_LAST_SCNL_PACKET_TIME;
/* Max number of channels to check */
#define MAX_NUM_SCNL 2000

#define MAX_SPECTRA_SIZE 256000

/************************************************************************************/

void usage( char *pgmName ) 
  {
     fprintf(stderr, "%s version %s\n", pgmName, VERSION);
     fprintf(stderr,"Usage: %s <ring name> [-d] [-f [dir_path]]\n", pgmName);
     fprintf(stderr, "     No flags: headers of ring's messages printed. \n");
     fprintf(stderr, "     -d flag:  data from ring's messages printed. \n");
     fprintf(stderr, "     -f flag:  content from ring's messages each written to a file.\n");
     fprintf(stderr, "               If dir_path is provided, files created there; otherwise in pwd\n");
     fprintf(stderr, "     Flags -d and -f may not both be present\n");
  }


static void Swap( char *data, int size )
{
   char temp, *cdat = (char*)data;
   int i,j;
   for ( i=0, j=size-1; i<j; i++, j-- ) {
   		temp = cdat[i]; cdat[i] = cdat[j]; cdat[j] = temp;
   }

}

#define SwapInt(x) Swap((char*)(x), sizeof(int))
#define SwapDouble(x) Swap((char*)(x), sizeof(double))

static void MakeLocal( void* msg ) {
	EW_SPECTRA_HEADER *ewsh = (EW_SPECTRA_HEADER*)msg;
	double *data = (double*)((char*)msg + sizeof(EW_SPECTRA_HEADER));
	int i;
	
#if defined (_SPARC)
	if ( ewsh->units[0]=='A' || ewsh->units[0]=='C' )
		return;
	ewsh->units[0] = tolower( ewsh->units[0] );
#elif defined (_INTEL)
	if ( ewsh->units[0]=='a' || ewsh->units[0]=='c' )
		return;
	ewsh->units[0] = toupper( ewsh->units[0] );
#else
#error "_INTEL or _SPARC must be set before compiling"
#endif
		
	SwapInt( &(ewsh->nsamp) );
	SwapDouble( &(ewsh->starttime) );
	SwapDouble( &(ewsh->delta_frequency) );
	SwapInt( &(ewsh->numPeaks) );
	
	for ( i=0; i<ewsh->nsamp*2; i++ )
		SwapDouble( data+i );
}

int main( int argc, char **argv )
{
   SHM_INFO        region;
   long            RingKey;         /* Key to the transport ring to read from */
   MSG_LOGO        getlogo[NLOGO], logo;
   long            gotsize;
   char            msg[MAX_SPECTRA_SIZE];
   char           *inRing;
   unsigned char   Type_Spectra_Data;
   unsigned char   InstWildcard, ModWildcard;
   double         *ewsh_data;
   EW_SPECTRA_HEADER  *ewsh = (EW_SPECTRA_HEADER*)msg;
   char            orig_datatype[3];
   char            stime[256];
   char            etime[256];
   int             dataflag;
   int             i;
   int             rc;
   int             nLogo = NLOGO;
   static unsigned char InstId;        /* local installation id              */
   unsigned char   sequence_number = 0;
   long			   packet_total = 0, packet_total_size = 0;
   char *outDirPath = NULL;
   double ewsh_endtime;
   
  /* Initialize pointers
  **********************/
  ewsh  = (EW_SPECTRA_HEADER *) msg;
  ewsh_data  =  (double *)( msg + sizeof(EW_SPECTRA_HEADER) );

  /* Check command line argument
  *****************************/
  if ( !(argc==2 || argc==3 || argc==4) ) {
  	  usage( argv[0] );
  	  exit(1);
  }

  dataflag = (argc != 2);
  if ( dataflag ) {
  	 outDirPath = argv[3];
  	 if ( strcmp(argv[2],"-d") && strcmp(argv[2], "-f") ) {
  	 	usage( argv[0] );
  	 	exit(1);
  	 }
  	 if ( outDirPath == NULL && strcmp(argv[2], "-f")==0 )
  	 	outDirPath = ".";
  }
  	 	
  inRing  = argv[1];

  /* logit_init but do NOT WRITE to disk, this is needed for WaveMsg2MakeLocal() which logit()s */
  logit_init("sniffspectra", 200, 200, 0);

  /* Attach to ring
  *****************/
  if ((RingKey = GetKey( inRing )) == -1 )
  {
     fprintf( stderr, "Invalid RingName; exiting!\n" );
     exit( -1 );
  }
  tport_attach( &region, RingKey );

  /* Look up local installation id
  *****************************/
  if ( GetLocalInst( &InstId ) != 0 )
  {
     fprintf(stderr, "sniffspectra: error getting local installation id; exiting!\n" );
     return -1;
  }


  /* Specify logos to get
  ***********************/
  nLogo = 1;
  if ( GetType( "TYPE_SPECTRA_DATA", &Type_Spectra_Data ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_SPECTRA_DATA>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid moduleid <MOD_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid instid <INST_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }

  for( i=0; i<nLogo; i++ ) {
      getlogo[i].instid = InstWildcard;
      getlogo[i].mod    = ModWildcard;
  }
  getlogo[0].type = Type_Spectra_Data;

  /* Flush the ring
  *****************/
  while ( tport_getmsg( &region, getlogo, nLogo, &logo, &gotsize,
            (char *)&msg, MAX_SPECTRA_SIZE ) != GET_NONE )
  {
         packet_total++;
         packet_total_size+=gotsize;
  }
  fprintf( stderr, "sniffspectra: inRing flushed %ld packets of %ld bytes total.\n",
		packet_total, packet_total_size);

  while ( tport_getflag( &region ) != TERMINATE ) 

  {

    rc = tport_copyfrom( &region, getlogo, nLogo,
               &logo, &gotsize, msg, MAX_SPECTRA_SIZE, &sequence_number );
    if ( rc == GET_NONE )
    {
      sleep_ew( 200 );
      continue;
    }

    if ( rc == GET_TOOBIG )
    {
      fprintf( stderr, "sniffspectra: retrieved message too big (%ld) for msg\n",
        gotsize );
      continue;
    }

    if ( rc == GET_NOTRACK )
      fprintf( stderr, "sniffspectra: Tracking error.\n");

    if ( rc == GET_MISS_LAPPED )
      fprintf( stderr, "sniffspectra: Got lapped on the ring.\n");

    if ( rc == GET_MISS_SEQGAP )
      fprintf( stderr, "sniffspectra: Gap in sequence numbers\n");

    if ( rc == GET_MISS )
      fprintf( stderr, "sniffspectra: Missed messages\n");

    /* Check SCNL of the retrieved message */

    if ( 1 )
    {
      FILE *fp = stdout;
      
      if ( outDirPath != NULL ) {
      	char fullpath[500];
      	char date[100];
      	EWSUnConvertTime (date, ewsh->starttime);
      	date[14] = 0;
      	sprintf( fullpath, "%s/%s_%s.%s.%s.%s_spectra", outDirPath, date, 
      		ewsh->sta, ewsh->chan, ewsh->net, ewsh->loc );
      	fp = fopen( fullpath, "w" );
      	if ( fp == NULL ) {
      		logit( "e", "sniffspectra: Could not create output file '%s'; writing to stdout\n", fullpath );
      		fp = stdout;
      	}
      }
      
      strcpy(orig_datatype, ewsh->units);
      MakeLocal( ewsh );

	  ewsh_endtime = ewsh->starttime + ewsh->delta_frequency * ewsh->nsamp * 2;

      /* Check for gap and in case output message */
      /*check_for_gap_overlap_and_output_message(trh, scnl_time, &n_scnl_time);*/

      datestr23 (ewsh->starttime, stime, 256);
      datestr23 (ewsh_endtime,   etime, 256);

      fprintf( fp, "%5s.%s.%s.%s ", ewsh->sta, ewsh->chan, ewsh->net, ewsh->loc );

      fprintf( fp, "%s %3d ", orig_datatype, ewsh->nsamp); 
      if (ewsh->delta_frequency < 1.0) { /* more decimal places for slower sample rates */
          fprintf( fp, "%6.4f", ewsh->delta_frequency);
      } else {
          fprintf( stdout, "%5.1f", ewsh->delta_frequency);
      }
      fprintf( fp, " %s (%.4f) %s (%.4f) ",
                         stime, ewsh->starttime,
                         etime, ewsh_endtime);
      fprintf( fp, "i%d m%d t%d len%4ld",
                 (int)logo.instid, (int)logo.mod, (int)logo.type, gotsize );

      fprintf( fp, "\n");
      fflush (fp);

      if (dataflag == 1)
      {
      	write_spectra_to_file( ewsh, (double*)(msg+sizeof(*ewsh)), "sniffspectra", fp );
	  }
      fflush (fp);
	  
      if ( fp != stdout ) {
      	fclose( fp );
      }
	}
  }
  exit (0);
}

