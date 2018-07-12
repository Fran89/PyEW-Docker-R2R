
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined (_WINNT)
#include <io.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "geolib.h"

#if defined (_SOLARIS)
#define stricmp(a,b) strcasecmp((a),(b))
#endif



/****************************************************************************
 * check_dir
 *
 * Description: check that the given string points to a directory
 *
 * Input parameters: dir - the string to check
 * Output parameters: none
 * Returns: TRUE for a directory, FALSE for an error
 *
 * Comments:
 *
 ***************************************************************************/
int check_dir (char *dir)

{

  struct stat info;


  if (stat (dir, &info)) return FALSE;
  else if (! (info.st_mode & S_IFDIR)) return FALSE;
  return TRUE;

}


