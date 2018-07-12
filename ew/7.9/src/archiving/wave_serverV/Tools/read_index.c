
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: read_index.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2000/07/08 18:59:23  lombard
 *     Bug fixes from Chris Wood; read_struct now lists all struct file entries
 *
 *     Revision 1.2  2000/06/28 23:44:11  lombard
 *     Added -g flag to list gaps instead of index entries
 *
 *     Revision 1.1  2000/02/14 20:00:08  lucky
 *     Initial revision
 *
 *
 */

/* Read a tank index file and write some data to stdout in same format as
   inspect_tank */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <earthworm.h>
#include <transport.h>  /* To keep wave_serverV.h happy */
#include <wave_serverV.h>

int main(int argc, char **argv)
{
  char indexName[MAX_TANK_NAME]; /* path name of index file */
  DATA_CHUNK index;
  int numChunks, i;
  time_t stamp1, stamp2;
  FILE *ifp;
  double oldtime;
  int gap;
  
  gap =0;
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s [-g] indexfile\n", 
            argv[0]);
    exit( 1 );
  }
  
  if(argc == 3) gap=1;
  strncpy(indexName, argv[argc-1], MAX_TANK_NAME);
  
  if ( (ifp = fopen(indexName, "rb")) == (FILE*) NULL)
  {
    fprintf(stderr, "Error opening index file %s: %s\n", indexName, 
            strerror(errno));
    exit( 1 );
  }
  
  if (fread( &stamp1, sizeof(stamp1), 1, ifp) != 1)
  {
    fprintf(stderr, "Error reading first stamp: %s\n", strerror(errno));
    exit( 1 );
  }
  
  if (fread( &numChunks, sizeof(numChunks), 1, ifp) != 1)
  {
    fprintf(stderr, "Error reading numChunks: %s\n", strerror(errno));
    exit( 1 );
  }

  fprintf(stderr, "Index data read from index file: %s\n", indexName);
  if(gap) {
	fprintf(stderr, "  Start         gap length (sec)\n");
  }else {
	fprintf(stderr, "  n    Offset      Start            End\n");
  }
  oldtime=0.;

  for (i = 0; i < numChunks; i++)
  {
    if (fread(&index, sizeof(index), 1, ifp) != 1)
    {
      fprintf(stderr, "Error reading entry: %s\n", strerror(errno));
      exit( 1 );
    }
    if(gap) {
	if(oldtime < 1.) {
		oldtime=index.tEnd;
		continue;
	}
	printf("% 14.3lf % 9.3lf\n",index.tStart, index.tStart - oldtime);
	oldtime=index.tEnd;
    } else {
    printf("% 3d % 9ld % 14.3lf % 14.3lf\n", i, index.offset, index.tStart, 
           index.tEnd);
    }
  }
  
  if (fread( &stamp2, sizeof(stamp2), 1, ifp) != 1)
  {
    fprintf(stderr, "Error reading second stamp: %s\n", strerror(errno));
    exit( 1 );
  }
  fclose(ifp);
  if (stamp1 != stamp2)
    fprintf(stderr, "Time stamps don't match: %lld %lld\n", (long long)stamp1, (long long)stamp2);
  
  exit( 0 );
}
