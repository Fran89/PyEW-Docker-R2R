/*
 * index.c
 * Modified from index.c, Revision 1.5 in src/seismic_processing/pick_ew
 *
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 *
 * (C) 2008-2012 Claudio Satriano <satriano@ipgp.fr>,
 * under the same license terms of the Earthworm software system. 
 */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>

  /***************************************************************
   *                         GetPickIndex()                      *
   *                                                             *
   *           Get initial pick index number from disk file      *
   ***************************************************************/

int GetPickIndex( unsigned char modid, char *dir )
{
   FILE      *fpIndex;
   char       fname[40];        /* Name of pick index file */
   static int PickIndex = -1;

/* Build name of pick index file
   *****************************/
   if (dir == NULL) 
   {
      sprintf( fname, "pick_FP_%03d.ndx", (int) modid );
   } else {
      sprintf( fname, "%s/pick_FP_%03d.ndx", dir, (int) modid );
   }

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
   if ( ++PickIndex > 999999 ) PickIndex = 0;

/* Write the pick index to disk
   ****************************/
   fpIndex = fopen( fname, "w" );
   fprintf( fpIndex, "%4d\n", PickIndex );
   fclose( fpIndex );

   return PickIndex;
}
