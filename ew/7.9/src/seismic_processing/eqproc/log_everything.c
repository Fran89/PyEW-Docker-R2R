
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: log_everything.c 1485 2004-05-17 21:37:45Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/17 21:37:45  dietz
 *     added termination message
 *
 *     Revision 1.2  2004/05/17 20:22:50  dietz
 *     changed name of output file from junkfile to log_everything
 *
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
              "log_everything: Invalid message type <TYPE_KILL>; exiting!\n" );
      exit( -1 );
   }

   outfile = fopen( "log_everything", "w" );
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
         fprintf(stderr, "log_everything: pipe_get EOF; exiting\n" );
         break;
      }

      fprintf( stderr,  "type %3d:\n%s", type, msg );
      fprintf( outfile, "type %3d:\n%s", type, msg );
      fflush( outfile );
   } while( type != TypeKill );

  fprintf( stderr,  "log_everything: TYPE_KILL msg received; terminating\n" );
  fprintf( outfile, "log_everything: TYPE_KILL msg received; terminating\n" );
  fclose( outfile );
  return 0;
}
