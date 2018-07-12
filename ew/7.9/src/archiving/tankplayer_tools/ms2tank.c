/*
 * Standalone program to read MSEED data files and write
 * earthworm TRACE_BUF2 messages.

 * this was based on ms2tb that used qlib2 functions, but was switched to libmseed (from Chad Trabant)
 * because that library is easily supported under windows.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <trace_buf.h>

#include "mseed2tank.h"

/* Internal Function Prototypes */
void usage( char * );
#define MAX_4BYTE_SAMPS_TBUF 1008
#define DEFAULT_MAX_SAMPS 200

#define MS2TANK_VERSION_NUMBER "0.0.3 2012.12.07"

int main(int argc, char **argv)
{
  char *MSfile;
  int max_samps = DEFAULT_MAX_SAMPS;
  int arg = 1;
  TRACE2X_HEADER t2header;

  memset((void*) &t2header, 0, sizeof(TRACE2X_HEADER));
  
  if (argc < 2)
    usage( argv[0] );

  while (arg < argc && argv[arg][0] == '-')
  {
    switch(argv[arg][1])
    {
    case 'n':
      arg++;
      max_samps = atoi(argv[arg]);
      if (max_samps > MAX_4BYTE_SAMPS_TBUF) {
		usage(argv[0]);
      }
      break;
    case 'N':
      arg++;
      if (strlen(argv[arg]) > 2) {
		fprintf(stderr, "Error forcing network code, must be limited to 2 chars\n");
		usage(argv[0]);
      }
      strcpy(t2header.net, argv[arg]);
      break;
    case 'S':
      arg++;
      if (strlen(argv[arg]) > 5) {
		fprintf(stderr, "Error forcing station name, must be limited to 5 chars\n");
		usage(argv[0]);
      }
      strcpy(t2header.sta, argv[arg]);
      break;
    case 'L':
      arg++;
      if (strlen(argv[arg]) > 2) {
		fprintf(stderr, "Error forcing location code, must be limited to 2 chars\n");
		usage(argv[0]);
      }
      strcpy(t2header.loc, argv[arg]);
      break;
    case 'C':
      arg++;
      if (strlen(argv[arg]) > 3) {
		fprintf(stderr, "Error forcing channel name, must be limited to 3 chars\n");
		usage(argv[0]);
      }
      strcpy(t2header.chan, argv[arg]);
      break;
    default:
      usage( argv[0] );
    }
    arg++;
  }
  if (argc - arg != 1)
    usage( argv[0] );
  
  MSfile = argv[arg];
  arg++;
  
  return(convert_mseed_to_tank(MSfile, "stdout",  max_samps, &t2header));
}


void usage( char *argv0 )
{
  fprintf(stderr, "Usage: %s [-n maxsamples] [-N NN] [-L LL] [-S SSSSS] [-C CCC] miniseed_inputfile >> outfile\n", argv0);
  fprintf(stderr, "Version %s\n", MS2TANK_VERSION_NUMBER);
  fprintf(stderr, " -n maxsamples - the max number of samps per tracebuf, \n\tdefaults to 200 per tbuf, cannot be larger than 1008\n");
  fprintf(stderr, " -N NN - override network code from mseed packets with this value\n");
  fprintf(stderr, " -S SSSSS - override station code from mseed packets with this value\n");
  fprintf(stderr, " -L LL - override location code from mseed packets with this value\n");
  fprintf(stderr, " -C CCC - override channel name from mseed packets with this value\n");

  exit( 1 );
}
  
