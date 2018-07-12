
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: index.c,v 1.1 2000/02/14 19:06:49 lucky Exp $
 *
 *    Revision history:
 *     $Log: index.c,v $
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>

  /***************************************************************
   *                         GetPickIndex()                      *
   *                                                             *
   *           Get initial pick index number from disk file      *
   ***************************************************************/

int GetPickIndex( void )
{
   FILE *fpIndex;
   const  char fname[40] = "pick_ew.ndx";   /* Name of pick index file */
   static int PickIndex = -1;

/* Get initial pick index from file
   ********************************/
   if ( PickIndex == -1 )
   {
      fpIndex = fopen( fname, "r" );        /* Fails if file doesn't exist */

      if ( fpIndex != NULL )
      {
         fscanf( fpIndex, "%d", &PickIndex );
         fclose( fpIndex );
      }
   }

/* Update the pick index
   *********************/
   if ( ++PickIndex > 9999 ) PickIndex = 0;

/* Write the pick index to disk
   ****************************/
   fpIndex = fopen( fname, "w" );
   fprintf( fpIndex, "%4d\n", PickIndex );
   fclose( fpIndex );

   return PickIndex;
}
