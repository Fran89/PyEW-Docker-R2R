
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: putter.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/17 16:29:05  dietz
 *     modified to wrok with TYPE*SCNL picks/codas
 *
 *     Revision 1.1  2000/02/14 19:08:08  lucky
 *     Initial revision
 *
 *
 */

  /*****************************************************************
   *                           putter.c                            *
   *                                                               *
   * Program to read a time-stamped pick file and copy picks       *
   * to shared memory.                                             *
   * This is good for replaying picks into binder.                 *
   *                                                               *
   * Usage: putter <pickfile> <opt:RING_NAME>                      *
   *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <transport.h>
#include <earthworm.h>
#define  MAX_CHAR 200

int main( int argc, char *argv[] )
{
   FILE *     fp;
   char       ringname[50];
   char       line[MAX_CHAR];
   char       tmp[4];
   SHM_INFO   region;
   MSG_LOGO   logo;
   long       RingKey;         /* Key to the transport ring to write to */
   int        type, mod, inst;
   int        d1=0, d2, d3;
   time_t     t1, t2;

/* Check command line arguments
   ****************************/
   if ( argc < 2 )
   {
      printf( "Usage: putter <pickfile> <opt:RING_NAME>\n" );
      exit( -1 );
   }

/* Open pick file
   **************/
   fp = fopen( argv[1], "r" );
   if ( fp == NULL )
   {
      printf( "putter: Cannot open pick file: %s; exiting!\n", argv[1] );
      exit( 0 );
   }

/* Attach to transport ring
   ************************/
   if( argc == 3 )  strcpy( ringname, argv[2] );
   else             strcpy( ringname, "PICK_RING" );

   if ( ( RingKey = GetKey(ringname) ) == -1 )
   {
      printf( "putter: Invalid RingName <%s>; exiting!\n", ringname );
      exit( -1 );
   }
   tport_attach( &region, RingKey );
   printf( "putter: Writing to <%s>\n", ringname );

/* Get current time
   ****************/
   time( &t1 );

/* Copy from pick file to transport ring
   *************************************/
   while ( fgets( line, MAX_CHAR, fp ) != NULL )
   {
      if ( tport_getflag( &region ) == TERMINATE )  break;

/* Get the time stamp, sleep until it's time to read pick/coda
   ***********************************************************/
      if ( sscanf( line, "%d", &d2 ) < 1 )
         continue;
      if ( d1 == 0 )
         d1 = d2;
      d3 = d2 - d1;
      while ( 1 )
      {
         time( &t2 );
         if ( (t2 - t1) >= d3 ) break;
         sleep_ew( 200 );
      }

/* Get the pick or coda; read its logo & check for valid values
   ************************************************************/
      if( fgets( line, MAX_CHAR, fp ) == NULL )  break;

      if( sscanf( line, "%d %d %d", &type, &mod, &inst ) != 3 ) continue;
      if( type<0 || type>255 ) continue; 
      if(  mod<0 || mod>255  ) continue;
      if( inst<0 || inst>255 ) continue;

      printf( "%s", line );

      logo.type   = (unsigned char) type;
      logo.mod    = (unsigned char) mod;
      logo.instid = (unsigned char) inst;

      if ( tport_putmsg( &region, &logo, strlen(line), line ) != PUT_OK )
         printf( "putter: Error sending message to region %ld\n",region.key );
   }

/* We're done
   **********/
   tport_detach( &region );
   fclose( fp );
   printf( "putter: Reached end of pick file: %s; exiting.\n", argv[1] );
   return 0;
}
