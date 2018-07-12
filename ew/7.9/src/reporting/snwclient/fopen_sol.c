/* fopen_sol.c */

      /****************************************************************
       *                          fopen_excl()                        *
       *                        Solaris version                       *
       *                                                              *
       *  The purpose of this function is to open a disk file         *
       *  exclusively.  I haven't yet figured out how to do this yet  *
       *  in the Solaris operating environment.                       *
       *  Does anyone else know how?  WMK 11/24/99                    *
       ****************************************************************/

#include <stdio.h>


FILE *fopen_excl( const char *fname, const char *mode )
{
   return fopen( fname, mode );
}
