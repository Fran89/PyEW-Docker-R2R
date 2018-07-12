
         /************************************************************
          *            dirops_ew.c  Windows NT version               *
          *                                                          *
          *  Contains system-specific functions for dealing with     *
          *  directories                                             *
          ************************************************************/

#include <windows.h>


           /*******************************************************
            *  chdir_ew( )  changes current working directory     *
            *                                                     *
            *  Returns 0 if all went well                         *
            *          -1 if an error occurred                    *
            *******************************************************/

int chdir_ew( char *path )
{
   BOOL success;

   success = SetCurrentDirectory( path );

   if ( success )
      return 0;
   else
      return -1;
}

