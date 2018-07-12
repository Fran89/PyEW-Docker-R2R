/* convert a wave_serverV tank file to a tankplayer tankbuffer file for
   playback 
	
	Paul Friberg - June 23, 2011
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>


#include <earthworm.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>

#define MAX_LINE 1024
#define INDEX_MALLOC_INCR 10

int main(int argc, char **argv)
{
  char tankName[MAX_LINE];  /* path name of tank file */
  char tbName[MAX_LINE+5];  /* path name of tank file */
  TRACE_HEADER th;
  FILE *tfp, *ofp;
  long recSize;
  long offset = 0l;     /* Offset into tank file */
  long recs_processed=0;
  char byteOrder, data_buf[4096];
  int dataSize;
  int numBytes;
  
  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s recSize tankfile\n", 
            argv[0]);
    exit( 1 );
  }
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s recSize tankfile\n", 
            argv[0]);
    exit( 1 );
  }
  
  if (sscanf(argv[1], "%ld", &recSize) != 1 || recSize == 0)
  {
    fprintf(stderr, "Error scanning recSize: %s\n", argv[1]);
    exit( 1 );
  }
  
  strcpy(tankName, argv[2]);
  strcpy(tbName, argv[2]);
  strcat(tbName, ".tb");
  
  if ( (ofp = fopen(tbName, "wb")) == (FILE*) NULL)
  {
    fprintf(stderr, "Error opening output tb file %s: %s\n", tbName, 
            strerror(errno));
    exit( 1 );
  }
  if ( (tfp = fopen(tankName, "rb")) == (FILE*) NULL)
  {
    fprintf(stderr, "Error opening tank file %s: %s\n", tankName, 
            strerror(errno));
    exit( 1 );
  }
  
  /* Read in all the tank records, one at a time. */
  while( 1 )
  {
    if (fread(&th, sizeof(TRACE_HEADER), 1, tfp) != 1)
    {
      if (feof(tfp))
      {
        fprintf(stderr, "Read to end of file\n");
        break;
      }
      else
      {
        fprintf(stderr, "Error reading from tank: %s\n", strerror(errno));
        exit( 1 );
      }
    }
    if (th.nsamp != 0 && (th.datatype[0] == 's' || th.datatype[0]=='i')) {
    	if (fwrite((void *) &th, sizeof(TRACE_HEADER), 1, ofp) != 1)
    	{
        	fprintf(stderr, "Error writing to tb file: %s %s\n", 
			tbName, strerror(errno));
        	exit( 1 );
    	}
    	/* See what sort of data it carries
     	**********************************/
    	dataSize=0;
    	if ( strcmp(th.datatype, "s4")==0)
    	{
         	dataSize=4; byteOrder='s';
    	}
    	else if ( strcmp(th.datatype, "i4")==0)
    	{
         	dataSize=4; byteOrder='i';
    	}
    	else if ( strcmp(th.datatype, "s2")==0)
    	{
         	dataSize=2; byteOrder='s';
    	}
    	else if ( strcmp(th.datatype, "i2")==0)
    	{
         	dataSize=2; byteOrder='i';
    	}
#if defined(_SPARC)
    	if (byteOrder == 'i') 
    	{
		SwapInt( &(th.nsamp) );
    	}
#endif
#if defined(_INTEL)
    	if (byteOrder == 's') 
    	{
		SwapInt( &(th.nsamp) );
    	}
#endif
    	numBytes = th.nsamp * dataSize;
    	if (fread(data_buf, numBytes, 1, tfp) != 1) 
    	{
        	fprintf(stderr, "Error reading %d bytes data from tank: %s\n", numBytes, strerror(errno));
        	exit( 1 );
    	}
    	if (fwrite((void *) data_buf, numBytes, 1, ofp) != 1)
    	{
        	fprintf(stderr, "Error writing %d bytes data to tb file: %s %s\n", 
			numBytes, tbName, strerror(errno));
        	exit( 1 );
    	}
    	recs_processed ++;
    }
    offset += recSize;
    if ( fseek(tfp, offset, SEEK_SET) != 0 )
    {
      fprintf(stderr, "Error seeking to next offset %ld: %s\n", offset,
                strerror(errno));
      break;
    }
  } /* while (reading tank file) */
  fprintf(stdout, "%ld tracebuf records processed\n", recs_processed);
  fclose(tfp);
  fclose(ofp);
  exit( 0 );
}
