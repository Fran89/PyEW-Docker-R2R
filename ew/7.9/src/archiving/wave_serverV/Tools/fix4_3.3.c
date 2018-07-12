
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: fix4_3.3.c 15 2000-02-14 20:06:34Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 20:00:08  lucky
 *     Initial revision
 *
 *
 */

/* a little program to convert a v3.2 tank structure file for version 3.3
 * Pete Lombard, UW Geophysics, 8/25/98
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WINNT
# include <io.h>
#endif

#include <earthworm.h>
#include <transport.h>
#include "wave_serverV.h"


#ifndef ssize_t
# define ssize_t int
#endif

typedef struct			/*** Tank descriptor ***/
{
  char 		tankName[MAX_TANK_NAME];/* full path name of tank file */
  FILE*		tfp;		/* tank file pointer */
  IndexFilePtr  ifp;		/* index file pointer (the name's derVed from the tankName)*/
  mutex_t	mutex;		/* our mutex */
  MSG_LOGO	logo;		/* logo kept in this tank (from transport.h) */
  int		pin;		/* pin number kept in this tank */
  char		sta[7];		/* station name */
  char		net[9];		/* network name */
  char		chan[9];	/* component name */
  char		datatype[3];	/* zero terminated string e.g. i2,s4,.. */
  double	samprate;	/* Sample rate; nominal */
  long int 	tankSize;	/* size of  tank file in bytes */
  DATA_CHUNK*	chunkIndex;	/* pointer to start of data chunk array */
  unsigned int  indxStart;      /* array offset of first (oldest) entry in
				 * index */
  unsigned int  indxFinish;     /* array offset of last (youngest) entry in
				 * index */
  long int 	indxMaxChnks;	/* max data chunks in index */
  long int	recSize;	/* message record size. Each msg gets that
				 * much disk space */
  long int 	nRec;		/* number of records in tank file */
  long int 	inPtOffset;	/* Insertion point of this tank */
  int           isConfigured;   /* 0 if the tank hasn't been configured yet, 
				 * 1 otherwise */
  int		firstPass;	/* 1 => first time through tank. 0 otherwise */
  int		firstWrite;	/* 1 => first write to brand new tank. 0
				 * otherwise */
  time_t        lastIndexWrite; /* Last time the index was written to disk */
} oldTANK;

main( int argc, char ** argv )
{
  time_t starttime, endtime;
  char buffer[1000];     /* a hunk of space */
  int NumOfTanks, i;
  oldTANK * pOldTank;
  TANK * pTank;
  char oldfile[MAX_TANK_NAME], newfile[MAX_TANK_NAME + 5];
  int ofd, nfd;
  ssize_t nread;
  
  pOldTank = (oldTANK *) buffer;
  pTank = (TANK *) buffer;
  
  if (argc != 2 )
  {
    fprintf( stderr, "Usage: %s tank_struct_file\n", argv[0] );
    exit( -1 );
  }
  
  strcpy( oldfile, argv[1] );
  strcpy( newfile, oldfile );
  strcat( newfile, ".new" );
  
  ofd = open( oldfile, O_RDONLY );
  if ( ofd == -1 )
  {
    fprintf( stderr, "unable to open %s\n", oldfile );
    exit( -1 );
  }
  
  nfd = open( newfile, O_WRONLY | O_CREAT, 0664 );
  if ( nfd == -1 )
  {
    fprintf( stderr, "unable to create %s\n", newfile );
    exit( -1 );
  }
  
  nread = read( ofd, &starttime, sizeof(starttime));
  if (nread != sizeof(starttime))
  {
    fprintf( stderr, "error reading starttime\n" );
    close( ofd );
    close( nfd );
    unlink( newfile );
    exit( -1 );
  }
  read( ofd, &NumOfTanks, sizeof(int));
  fprintf( stderr, "%s holds %d tanks\n", oldfile, NumOfTanks );
  
  write( nfd, &starttime, sizeof(starttime));
  write( nfd, &NumOfTanks, sizeof(int));
  
  for (i = 0; i < NumOfTanks; i++ )
  {
    nread = read( ofd, buffer, sizeof( oldTANK ) );
    if (nread != sizeof( oldTANK ) )
    {
      fprintf( stderr, "error reading tank struct %d\n", i );
      exit( -1 );
    }
    pTank->lappedIndexEntries = 0;
    
    write( nfd, buffer, sizeof( TANK ) );
  }
  nread = read( ofd, &endtime, sizeof(endtime) );
  if ( starttime != endtime )
  {
    fprintf(stderr, "starttime %ld does not match endtime %ld\n", starttime,
	    endtime );
  }
  
  write( nfd, &endtime, sizeof(endtime));
  close(ofd);
  close(nfd);
  fprintf(stderr, "%d tank structures transfered to %s\n", NumOfTanks, 
	  newfile );

  return(0);
}
