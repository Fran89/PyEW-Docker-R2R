
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: read_struct.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2001/04/17 16:41:34  davidk
 *     Removed an unused local variable in one of the functions (tp).
 *
 *     Revision 1.2  2000/07/08 18:59:23  lombard
 *     Bug fixes from Chris Wood; read_struct now lists all struct file entries
 *
 *     Revision 1.1  2000/02/14 20:00:08  lucky
 *     Initial revision
 *
 *
 */

/* Read a tank structure file and write some data to stdout */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <earthworm.h>
#include <transport.h>  /* To keep wave_serverV.h happy */
#include <wave_serverV.h>

int main(int argc, char **argv)
{
  TANK tank;
  char structName[MAX_TANK_NAME]; /* path name of index file */
  char atim[32];
  int numTanks, i;
  time_t stamp1, stamp2;
  FILE *sfp;
  
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s structfile\n", 
            argv[0]);
    exit( 1 );
  }
  
  strncpy(structName, argv[1], MAX_TANK_NAME-1);
  
  if ( (sfp = fopen(structName, "rb")) == (FILE*) NULL)
  {
    fprintf(stderr, "Error opening struct file %s: %s\n", structName, 
            strerror(errno));
    exit( 1 );
  }
  
  if (fread( &stamp1, sizeof(stamp1), 1, sfp) != 1)
  {
    fprintf(stderr, "Error reading first stamp: %s\n", strerror(errno));
    exit( 1 );
  }
  
  if (fread( &numTanks, sizeof(numTanks), 1, sfp) != 1)
  {
    fprintf(stderr, "Error reading numTanks: %s\n", strerror(errno));
    exit( 1 );
  }

  strcpy(atim,ctime(&stamp1));
  atim[24] = '\0';
  printf("%s (%d tanks): %s\n",argv[1],numTanks,atim);
  printf("S    C   N   PIN TYP MOD IID DT SPS IStart IFinish  Offset C P W recSz MaxChnk    nRec  lapped           lastIndexWrite tankName\n");
  
  for (i = 0; i < numTanks; i++)
  {
    if (fread(&tank, sizeof(tank), 1, sfp) != 1)
    {
      fprintf(stderr, "Error reading entry: %s\n", strerror(errno));
      exit( 1 );
    }
    strcpy(atim,ctime(&tank.lastIndexWrite));
    atim[24] = '\0';
    printf("%s.%s.%s %4d %3d %3d %3d %2s %3.0f %6d %6d %9ld %1d %1d %1d %5ld %7ld %7ld %7d %s %s\n",
           tank.sta, tank.chan, tank.net, tank.pin,
           tank.logo.type, tank.logo.mod, tank.logo.instid, tank.datatype, tank.samprate,
           tank.indxStart, tank.indxFinish, tank.inPtOffset, tank.isConfigured, tank.firstPass,
           tank.firstWrite, tank.recSize, tank.indxMaxChnks, tank.nRec,
           tank.lappedIndexEntries, atim, tank.tankName);

  }
  
  if (fread( &stamp2, sizeof(stamp2), 1, sfp) != 1)
  {
    fprintf(stderr, "Error reading second stamp: %s\n", strerror(errno));
    exit( 1 );
  }
  fclose(sfp);
  if (stamp1 != stamp2)
    fprintf(stderr, "Time stamps don't match: %lld %lld\n", (long long)stamp1, (long long)stamp2);
  
  exit( 0 );
}
