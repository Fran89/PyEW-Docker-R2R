
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: inspect_tank.c 5816 2013-08-14 18:47:32Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2001/04/17 16:40:00  davidk
 *     Added an #include of time.h to get rid of a compiler warning for
 *     time() on NT.
 *
 *     Revision 1.2  2000/07/08 18:59:23  lombard
 *     Bug fixes from Chris Wood; read_struct now lists all struct file entries
 *
 *     Revision 1.1  2000/02/14 20:00:08  lucky
 *     Initial revision
 *
 *
 */

/* Inspect a wave_ServerV tank file and its companion index file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <wave_serverV.h>

#define INDEX_MALLOC_INCR 10

int main(int argc, char **argv)
{
  char tankName[MAX_TANK_NAME];  /* path name of tank file */
  char indexName[MAX_TANK_NAME]; /* path name of index file */
  TRACE_HEADER th;
  DATA_CHUNK *index, *newIndex;
  int numChunks = 0, maxChunks;
  FILE *tfp, *ifp;
  long recSize, nRec, tankSize, nRecRead;
  double lastEnd = 0.0;
  double thisStart, thisEnd;
  int first = 1; 
  long offset = 0l;     /* Offset into tank file */
  double samprate;      /* As read from first record in tank */
  double gapSec;        /* Gap in seconds */
  double gapSamp = 1.5; /* Gap in sample periods, default is 1.5 */
  double newestStart = 0.0;
  int newestChunk;
  int i,j;
  char yorn[2];
  time_t now;
  
  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s [ -g gap] recSize tankSize tankfile\n", 
            argv[0]);
    exit( 1 );
  }
  if (strcmp( argv[1], "-g" ) == 0)
  {
    gapSamp = atof(argv[2]);
    argc -= 2;
    argv += 2;
  }

  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s [ -g gap] recSize tankSize tankfile\n", 
            argv[0]);
    exit( 1 );
  }
  
  if (sscanf(argv[1], "%ld", &recSize) != 1 || recSize == 0)
  {
    fprintf(stderr, "Error scanning recSize: %s\n", argv[1]);
    exit( 1 );
  }
  
  if (sscanf(argv[2], "%ld", &tankSize) != 1)
  {
    fprintf(stderr, "Error scanning tankSize: %s\n", argv[2]);
    exit( 1 );
  }
  if (tankSize < 1000000) tankSize *= 1000000; /* millions of bytes */
  
  nRec = tankSize / recSize;
  strncpy(tankName, argv[3], MAX_TANK_NAME-1);
  
  if ( (tfp = fopen(tankName, "rb")) == (FILE*) NULL)
  {
    fprintf(stderr, "Error opening tank file %s: %s\n", tankName, 
            strerror(errno));
    exit( 1 );
  }
  
  if ( (index = (DATA_CHUNK *)malloc( INDEX_MALLOC_INCR * sizeof(DATA_CHUNK)))
       == (DATA_CHUNK *)NULL)
  {
    fprintf(stderr, "Error allocating first index: %s\n", strerror(errno));
    exit( 1 );
  }
  maxChunks = INDEX_MALLOC_INCR;
  
  nRecRead = 0;
  /* Read in all the tank records, one at a time. */
  while( nRecRead < nRec )
  {
    if (fread(&th, sizeof(TRACE_HEADER), 1, tfp) != 1)
    {
      if (feof(tfp))
      {
        fprintf(stderr, "Read to end of file\n");
        index[numChunks].tEnd = lastEnd;
        break;
      }
      else
      {
        fprintf(stderr, "Error reading from tank: %s\n", strerror(errno));
        exit( 1 );
      }
    }

    thisStart = th.starttime;
    thisEnd = th.endtime;
    if (thisStart > newestStart)
    {
      newestStart = thisStart;
    }
    
    if (first)
    {
      lastEnd = thisEnd;
      if ( (samprate = th.samprate) <= 0.01 ) /* impossibly small sample rate */
      {
        fprintf(stderr, "Sample rate too small: %lf; assuming 100.0\n", 
                samprate);
        samprate = 100.0;
      }
      gapSec = gapSamp / samprate;

      /* Call this a new chunk, although it may be a continuation from the
         chunk at the end of the tank (wraparound) */
      index[0].tStart = thisStart;
      index[0].offset = offset;
      first = 0;
    }
    else
    {
      /* Look for a gap */
      if ( thisStart + 10.0 * gapSec < lastEnd )
      {  /* A big step backward; maybe the end of new data? */
        index[numChunks].tEnd = lastEnd;
        numChunks++;
        if (numChunks == maxChunks)
        {
          if ( (index = 
                (DATA_CHUNK *)realloc( (void *)index, (size_t)((maxChunks +
                                               INDEX_MALLOC_INCR)
                                               * sizeof( DATA_CHUNK))))
               == (DATA_CHUNK *) NULL)
          {
            fprintf(stderr, "Error reallocating memory\n");
            exit( 1 );
          }
          maxChunks += INDEX_MALLOC_INCR;
        }
        index[numChunks].tStart = thisStart;
        index[numChunks].offset = offset;
      }
      else if ( thisStart < lastEnd )
      { /* time went backward a little; log it but don't declare a new chunk */
        fprintf(stderr, "Time rollback: this start %lf; last end: %lf; "
                "this offset %ld\n", thisStart, lastEnd, offset);
      }
      else if (thisStart > lastEnd + gapSec)
      { /* A gap is declared, which marks a chunk boundary */
        index[numChunks].tEnd = lastEnd;
        numChunks++;
        if (numChunks == maxChunks)
        {
          if ( (newIndex = 
                (DATA_CHUNK *)realloc( (void *)index, (size_t)((maxChunks +
                                               INDEX_MALLOC_INCR) 
                                               * sizeof( DATA_CHUNK))))
               == (DATA_CHUNK *) NULL)
          {
            fprintf(stderr, "Error reallocating memory\n");
            exit( 1 );
          }
          index = newIndex;
          maxChunks += INDEX_MALLOC_INCR;
        }
        index[numChunks].tStart = thisStart;
        index[numChunks].offset = offset;
      }
    }  /* if (first)... */

    /* Prepare for the next tank record */
    offset += recSize;
    if ( (offset >= tankSize) || (fseek(tfp, offset, SEEK_SET) != 0) )
    {
      if (offset < tankSize)
        fprintf(stderr, "Error seeking to next offset %ld: %s\n", offset,
                strerror(errno));
      /* We've read to (probably) the end of file */
      index[numChunks].tEnd = thisEnd;
      break;
    }
    lastEnd = thisEnd;
  } /* while (reading tank file) */
  fclose(tfp);
  index[numChunks].tEnd = lastEnd;
  numChunks++;
  
  /* See if the last Chunk is really unused tank records */
  if (index[numChunks-1].tStart < 10.0)
  {
    numChunks--;
    fprintf(stderr, "Empty records start at offset %ld\n", 
            index[numChunks].offset);
  }
        
  /* See if the last chunk in tank wraps around to the begining of tank */
  if (index[numChunks-1].tEnd < index[0].tStart && 
      index[numChunks-1].tEnd + gapSec > index[0].tStart)
  {
    /* Chunk wraps; merge last one into the first one */
    numChunks--;
    index[0].tStart = index[numChunks].tStart;
    index[0].offset = index[numChunks].offset;
    fprintf(stderr, "Last chunk wraps around to beginning of tank; merging\n");
    }
  
  /*
   *Try to figure out where the newest data ends. We look for chunk with
   * newest data;
   * This may not work, if tank was fed time-shifted or otherwise bogus data.
   */
  newestChunk = 0;
  for (i = 1; i < numChunks; i++)
  {
    if (index[i].tStart > index[newestChunk].tStart)
      newestChunk = i;
  }
  if (newestStart < index[newestChunk].tStart ||
      newestStart > index[newestChunk].tEnd )
  {
    fprintf(stderr, "Newest data start %f not on newest chunk %f - %f\n",
            newestStart, index[newestChunk].tStart, index[newestChunk].tEnd);
  }
  
  printf("Index data read from tank:\n");
  printf("  n    Offset      Start            End\n");
  j = newestChunk + 1;
  for (i = 0; i < numChunks; i++)
  {
    if (j >= numChunks) j -= numChunks;
    printf("% 3d % 9ld % 14.3lf % 14.3lf\n", i, index[j].offset, index[j].tStart, 
           index[j].tEnd);
    j++;
  }
  
  fprintf(stderr, "Write new index file? [yn]: ");
  scanf("%1c", yorn);
  if (yorn[0] != 'y' && yorn[0] != 'Y')
    exit( 0 );
  
  strcpy(indexName, tankName);
  strcat(indexName, "-X.inx");

  if ( (ifp = fopen(indexName, "wb") ) == (FILE *)NULL)
  {
    fprintf(stderr, "Error opening index file %s: %s\n", indexName, 
            strerror(errno));
    exit( 1 );
  }
  time(&now);
  if (fwrite(&now,sizeof(time_t),1,ifp) < 1)
  {
    fprintf(stderr, "Error writing timestamp to index file: %s\n",
            strerror(errno));
    exit( 1 );
  }
  if (fwrite(&numChunks,sizeof(numChunks),1,ifp) < 1)
  {
    fprintf(stderr, "Error writing to index file: %s\n",
            strerror(errno));
    exit( 1 );
  }
  if (fwrite(index,sizeof(DATA_CHUNK),numChunks,ifp) < 1)
  {
    fprintf(stderr, "Error writing to index file: %s\n",
            strerror(errno));
    exit( 1 );
  }
  if (fwrite(&now,sizeof(time_t),1,ifp) < 1)
  {
    fprintf(stderr, "Error writing timestamp to index file: %s\n",
            strerror(errno));
    exit( 1 );
  }
  fclose(ifp);
  exit( 0 );
}
