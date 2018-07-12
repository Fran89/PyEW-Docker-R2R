
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getsysname_ew.c 10 2000-02-14 18:56:41Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

/*
 *  getsysname_ew.c  - OS/2 version
 *
 *  Earthworm utility for getting the system name from the system
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int getsysname_ew( char *sysname, int length )
{
   char *str;

/* Get system name from environment variable HOSTNAME
   **************************************************/
   str = getenv( "HOSTNAME" );

   if ( str == (char *) NULL )
   {
      fprintf( stderr, 
              "getsysname_ew: Environment variable HOSTNAME not defined.\n" );
      return( -1 );
   }

   if ( *str == '\0' ) 
   {
      fprintf( stderr, "getsysname_ew: Environment variable HOSTNAME" );
      fprintf( stderr, " defined, but has no value.\n" );
      return( -1 );
   }

/* Copy system name to target address
 ************************************/
   if( strlen( str ) >= (size_t) length ) 
   {
      fprintf( stderr, "getsysname_ew: HOSTNAME too long for target address.\n" ); 
      return( -1 );
   }

   strcpy( sysname, str );
   return( 0 );
}


