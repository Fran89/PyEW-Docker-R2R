
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: log_everything.c,v 1.1 2000/02/14 17:12:03 lucky Exp $
 *
 *    Revision history:
 *     $Log: log_everything.c,v $
 *     Revision 1.1  2000/02/14 17:12:03  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <earthworm.h>

int main( int argc, char *argv[] )
{
   FILE *outfile;
   int   type = 0;
   int   res;
   unsigned char TypeKill;

   static char msg[MAX_BYTES_PER_EQ];

   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
      fprintf( stderr,
              "log_everything: Invalid message type <TYPE_KILL>; exitting!\n" );
      exit( -1 );
   }

   outfile = fopen( "junkfile", "w" );
   if ( outfile == (FILE *)NULL )
   {
      fprintf( stderr, "log_everything: Error opening output file\n" );
      exit( 0 );
   }

   do
   {
      res = pipe_get( msg, MAX_BYTES_PER_EQ, &type );
      if( res == -1 )
      {
         fprintf(stderr, "log_everything: pipe msg too long; skip it.\n" );
         continue;
      }
      else if( res == -2 )
      {
         fprintf(stderr, "log_everything: pipe_get EOF; exitting\n" );
         break;
      }

      fprintf( stderr,  "type %3d:\n%s", type, msg );
      fprintf( outfile, "type %3d:\n%s", type, msg );
      fflush( outfile );
   } while( type != TypeKill );

  fprintf( stderr, "log_everything: terminating\n" );
  fclose( outfile );
  return 0;
}
