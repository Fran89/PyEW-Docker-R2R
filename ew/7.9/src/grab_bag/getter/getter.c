
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getter.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:30:07  lucky
 *     Initial revision
 *
 *
 */

  /*****************************************************************
   *                            getter.c                           *
   *                                                               *
   * Program to read picks & codas from shared memory (PICK_RING)  *
   * write to a disk file.  The system time (sec since 1970) is    *
   * written on the line before each pick or coda. This allows the *
   * picks and codas in the file to be "played back" into the      *
   * PICK_RING by the program, putter, with the same timing with   *
   * which they were originally received.                          *
   *                                                               *
   * Usage: getter <pickfile>                                      *
   *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <transport.h>
#include <earthworm.h>

#define BUFSIZE 200

int main( int argc, char *argv[] )
{
   FILE     *fp;
   SHM_INFO region;
   long     RingKey;         /* Key of the transport ring to read from */
   MSG_LOGO getlogo[2], logo;
   char     *gotmsg;
   long     gotsize;
   int      res;
   unsigned char instwildcard;
   unsigned char typepick2k;
   unsigned char typecoda2k;
   unsigned char modwildcard;


/* Check command line arguments
   ****************************/
   if ( argc < 2 )
   {
      printf( "Usage: getter <new pick file>\n" );
      return -1;
   }

/* Open file to write picks into
   *****************************/
   fp = fopen( argv[1], "w" );
   if ( fp == NULL )
   {
      printf( "getter: Cannot open new pick file: %s\n", argv[1] );
      return -1;
   }

/* Attach to ring
   **************/
   if ( ( RingKey = GetKey("PICK_RING") ) == -1 )
   {
      printf( "getter: Invalid RingName <PICK_RING>; exiting!\n" );
      return -1;
   }
   tport_attach( &region, RingKey );

/* Allocate message buffer
   ***********************/
   gotmsg = (char *) malloc( BUFSIZE+1 );
   if ( gotmsg == NULL )
   {
      printf( "getter: Error allocating gotmsg.\n" );
      return -1;
   }

/* Specify logos to get
   ********************/
   if ( GetInst( "INST_WILDCARD", &instwildcard ) != 0 )
   {
      printf( "getter: Error getting INST_WILDCARD\n" );
      return -1;
   }

   if ( GetModId( "MOD_WILDCARD", &modwildcard ) != 0 )
   {
      printf( "getter: Error getting MOD_WILDCARD\n" );
      return -1;
   }

   if ( GetType( "TYPE_PICK2K", &typepick2k ) != 0 )
   {
      printf( "getter: Error getting TYPE_PICK2K\n" );
      return -1;
   }

   if ( GetType( "TYPE_CODA2K", &typecoda2k ) != 0 )
   {
      printf( "getter: Error getting TYPE_CODA2K\n" );
      return -1;
   }

   getlogo[0].instid = instwildcard;
   getlogo[0].mod    = modwildcard;
   getlogo[0].type   = typepick2k;

   getlogo[1].instid = instwildcard;
   getlogo[1].mod    = modwildcard;
   getlogo[1].type   = typecoda2k;

/* Ignore all old picks in ring
 ******************************/
   while( tport_getmsg(&region, getlogo, (short)2,
                       &logo, &gotsize, (char *)gotmsg, BUFSIZE-1) != GET_NONE );

/* Here's the working loop
 *************************/
idle:
   sleep_ew( 100 );

   if ( tport_getflag( &region ) == TERMINATE )
   {
      tport_detach( &region );
      return 0;
   }

   while ( 1 )
   {
      res = tport_getmsg( &region, getlogo, (short)2,
                          &logo, &gotsize, (char *)gotmsg, BUFSIZE-1 );

      switch( res )
      {
      case GET_NONE:
           goto idle;

      case GET_TOOBIG:
           printf( "getter: Retrieved message too big (%ld) for gotmsg\n",
                       gotsize );
           goto idle;

      case GET_NOTRACK:
           printf ( "getter: NTRACK_GET exceeded\n" );

      case GET_MISS:
           printf ( "getter: Missed message(s)\n" );

      case GET_OK:

/* Print the logo
   **************/
/*       printf( "typ:%1d mid:%2d inst:%1d size:%d     ",
                  logo.type, logo.mod, logo.instid, gotsize ); */

/* Write the system time to file
   *****************************/
         fprintf( fp, "%ld\n", (long)time(NULL) );

/* Write the message to file
   *************************/
         gotmsg[gotsize] = '\0';  /* null terminate it first */
         printf( "%s", gotmsg );         /* to the screen    */
         fprintf( fp, "%s", gotmsg );    /* to the pick file */
         fflush( fp );
         break;
      }
   }
}
