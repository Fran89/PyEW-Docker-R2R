/*
 * Standalone program to fire a DataCenter java module to get mseeds, convert them to tank, remux them
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#include <errno.h>
#include "earthworm.h"
#include "trace_buf.h"
#include "sachead.h"
#include "swap.h"
#include "time_ew.h"

#include "libmseed.h"
#include "mseed2tank.h"
#include "remux.h"

#define TRACE2_UNDEF_STRING "  "

/* Internal Function Prototypes */
void usage( char * );

#define MAX_4BYTE_SAMPS_TBUF 1008
#define DEFAULT_MAX_SAMPS 200
#define MAX_CMDLINE 255
#define BIGNAMES 2048

#define DC2TANK_VERSION_NUMBER "0.0.2 2007.09.24"

int main(int argc, char **argv)
{
  int arg;
  int verbose=0;
  int retcode;
  int max_samps = DEFAULT_MAX_SAMPS;
  char command[2*MAX_CMDLINE+1];
  char mseedfile[2*MAX_CMDLINE+1];
  char fullname[BIGNAMES];
  char temp_tank[BIGNAMES];
  char tank_name[BIGNAMES];
  char *cptr;

  int duration_min;
  int duration_seconds = 600; 	/* duration in seconds to get */
  char *scnl_file=0;		/* file containing space sep SCNL (in that order) */
  char *working_directory=0;	/* name of working directory to use, will be created if it doesn't exist */
  char *starttime_string=0;	/* start time YYYY/MM/DD HH:MM:SS */
  char *properties_file=0;	/* for the dhi2mseed, contains CORBA server */
  char *datacenter_type=0;	/* type = c for cwb or d for dhi */
  char *output_tank=0;		/* name of the tankfile to write to */
  TRACE2X_HEADER t2header;

  memset((void*) &t2header, 0, sizeof(TRACE2X_HEADER));


  
  if (argc < 2)
    usage( argv[0] );

  arg = 1;
  while (arg < argc && argv[arg][0] == '-')
  {
    switch(argv[arg][1])
    {
    case 'o':
      arg++;
      output_tank = argv[arg];
      break;
    case 'p':
      arg++;
      properties_file = argv[arg];
      break;
    case 'D':
      arg++;
      working_directory = argv[arg];
      break;
    case 'f':
      arg++;
      scnl_file = argv[arg];
      break;
    case 's':
      arg++;
      starttime_string = argv[arg];
      break;
    case 'v':
      verbose=1;
      break;
    case 't':
      arg++;
      datacenter_type = argv[arg];
      break;
    case 'd':
      arg++;
      duration_seconds = atoi(argv[arg]);
      if (duration_seconds <= 0) {
 		fprintf(stderr, "-d secs: %d must be positive seconds!\n", duration_seconds);
		usage(argv[0]);
      }
      break;
    case 'n':
      arg++;
      max_samps = atoi(argv[arg]);
      if (max_samps > MAX_4BYTE_SAMPS_TBUF) {
 		fprintf(stderr, "-n max samps too large, cannot be greater than %d\n" , MAX_4BYTE_SAMPS_TBUF);
		usage(argv[0]);
      }
      break;
    default:
      fprintf(stderr, "Unknown argument/option %s\n", argv[arg]);
      usage( argv[0] );
    }
    arg++;
  }
  if (argc - arg != 0) {
    fprintf(stderr, "Unknown argument/option %s\n", argv[arg]);
    usage( argv[0] );
  }

/* check for required args */
  if (working_directory == 0) {
 	fprintf(stderr, "-D dir not provided, a working dir must be provided\n");
	usage(argv[0]);
  }
  if (datacenter_type == 0) {
 	fprintf(stderr, "-d type: not provided, a datacenter type must be provided\n");
	usage(argv[0]);
  }
  if (output_tank == 0) {
 	fprintf(stderr, "-o tank: not provided, a output tank must be provided\n");
	usage(argv[0]);
  }
  if (datacenter_type[0] == 'c') {
 	fprintf(stderr, "datacenter type c (CWB) not supported yet.\n");
	exit(1);
  }
  if (datacenter_type[0] == 'd' && properties_file==0) {
 	fprintf(stderr, "-p props: not provided, for DHI datacenter this must be provided.\n");
	usage(argv[0]);
  }
  if (datacenter_type[0] == 'd' && scnl_file==0) {
 	fprintf(stderr, "-f scnlfile: not provided, for DHI datacenter this must be provided.\n");
	usage(argv[0]);
  }



/* create the working dir */
  if (CreateDir(working_directory) != EW_SUCCESS) {
 	fprintf(stderr, "-D dir %s could not be created or does not exist\n", working_directory);
	exit(1);
  }
  logit_init("dc2tank", 0, 1024, 1);                                


/* first generate the mseeds */
	/* check for java in path */
	/* check for needed jar file */
	/* build appropriate command line */
  switch(datacenter_type[0]) {
	case 'd':
		duration_min = duration_seconds/60;

		sprintf(command, "java -jar DHI2mseed.jar -p %s -duration %d -exist ignore -f %s -o %s -starttime \"%s\"> %s/dmc.log",
					properties_file, duration_min, scnl_file, working_directory, 
					starttime_string, working_directory);

		if (strlen(command) > MAX_CMDLINE) {
			fprintf(stderr, "Error Java command line too long for operating system:\n %s", command);
			exit(1);
		}
		if (verbose)
			fprintf(stdout, "running DHI2mseed to get mseed files\n %s ", command);
		system(command);
		break;
	case 'c':
		break;
	default:
		fprintf(stderr, "unsupported data center type %s\n", datacenter_type);
		usage(argv[0]);
		break;
  }

  /* if output_tank is ABSOLUTE PATH'ED, then just get end of string */
  strcpy(tank_name, output_tank);
  if ( (cptr=strrchr(output_tank, '\\')) != NULL ) {
     cptr++;
     strcpy(tank_name, cptr);
  } else if ( (cptr=strrchr(output_tank, '/')) != NULL ) {
     cptr++;
     strcpy(tank_name, cptr);
  }
  /* create the temp tank file (unremuxed)  */
  sprintf(temp_tank, "%s/%s.tmp", working_directory, tank_name);

/* cd to working dir, and grab files one at a time */
  if (OpenDir(working_directory)) {
     fprintf(stderr, "Unable to open working directory %s to get miniseed files", working_directory);
  }
  while (GetNextFileName(mseedfile)==0) {
        if (strstr(mseedfile, ".mseed") == NULL) {
		continue;
        }
        if (verbose)
	    fprintf(stdout, "Processing %s\n", mseedfile);
        sprintf(fullname, "%s/%s", working_directory, mseedfile);
        retcode = convert_mseed_to_tank(fullname, temp_tank, max_samps, &t2header);
  }

/* now remux the sucker */
  if (verbose)
      fprintf(stdout, "Remuxing tank in time order %s\n", output_tank);
  remux_tracebufs(temp_tank, output_tank, 0.0, 0.0);

  unlink(temp_tank);
  return( 0 );
}


void usage( char *argv0 )
{
  fprintf(stderr, "Usage: %s [-n maxsamples] [-d secs] -D dir -t [d|c] -f scnlfile -p props -o tank\n", argv0);
  fprintf(stderr, "Version %s\n", DC2TANK_VERSION_NUMBER);
  fprintf(stderr, " -n maxsamples - the max number of samps per tracebuf in tank\n\tdefaults to 200 per tbuf, cannot be larger than 1008\n");
  fprintf(stderr, " -D workingdir - the working dir where mseeds and intermediate tanks are put\n");
  fprintf(stderr, " -p props - properties file needed for DHI type\n");
  fprintf(stderr, " -f scnlfile - SCNL file (space separated) needed for DHI type DC\n");
  fprintf(stderr, " -o tankfile - name of remuxed tank to write\n");
  fprintf(stderr, " -s YYYY/MM/DD HH:MM:SS - start time  to grab (put in quotes) required.\n");
  fprintf(stderr, " -d secs - optional duration in secs to grab (default=600secs).\n");
  fprintf(stderr, " -t d or c - type of data center d=DMC/DHI, c=CWB (not supported yet)\n");
  exit( 1 );
}
