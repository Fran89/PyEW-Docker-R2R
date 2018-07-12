#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "geolib.h"


/***********************************************************************
 * get_next_file
 *
 * Description: get the next file in a directory listing
 *
 * Input parameters: path - the path for the directory listing OR
 *                          NULL to continue with the previous path
 * Output parameters: none
 * Returns: information on the file OR NULL if no more files available
 *
 * Comments: Send this routine a path (e.g. c:\one\two) not a file
 * spec with wildcards (e.g. c:\one\two\*)
 *
 ***********************************************************************/
struct File_info *get_next_file (char *path)

{

  static char stored_path [200];
  static struct _finddata_t dir_info;
  static intptr_t dir_handle;
  static int dir_open = FALSE;
  static struct File_info info;


  /* get information on the next file */
  if (path)
  {
    if (dir_open)
    {
      _findclose (dir_handle);
      dir_open = FALSE;
    }
    strcpy (stored_path, path);
    terminate_path (stored_path, TRUE);
    strcat (stored_path, "*");
    dir_handle = _findfirst (stored_path, &dir_info);
    if (dir_handle < 0) return 0;
    dir_open = TRUE;
  }
  else if (! dir_open) return FALSE;
  else if (_findnext (dir_handle, &dir_info) < 0)
  {
    _findclose (dir_handle);
    dir_open = FALSE;
    return 0;
  }

  /* convert the information to local format */
  strcpy (info.name, dir_info.name);
  info.size = (long) dir_info.size;
  info.creation_time = dir_info.time_create;
  info.modification_time = dir_info.time_write;
  info.access_time = dir_info.time_access;
  if (dir_info.attrib & _A_HIDDEN) info.file_type = FILE_OTHER;
  else if (dir_info.attrib & _A_RDONLY) info.file_type = FILE_OTHER;
  else if (dir_info.attrib & _A_SYSTEM) info.file_type = FILE_OTHER;
  else if (dir_info.attrib & _A_SUBDIR) info.file_type = FILE_DIR;
  else info.file_type = FILE_NORMAL;

  return &info;

}

/*************************************************************************
 * terminate_path
 *
 * Input parameters: path - path to terminate
 *		     flag - TRUE = force a terminating \
 *		            FALSE = remove any terminating \
 * Output parameters: none
 * Returns: path
 *
 * Comments:
 *
 *************************************************************************/
char *terminate_path (char *path, int flag)

{

  int count, remove = TRUE, add = TRUE;

  /* get the length of the path */
  count = (int)strlen (path);
  if (! count) return path;

  /* add or remove ?? */
  if (flag)
  {
    /* add trailing directory delimiter (except to disk only specifier) */
    if (count == 2 && *(path +1) == ':') add = FALSE;
    if (*(path +count -1) == '\\') add = FALSE;
    if (add) strcat (path, "\\");
  }
  else
  {
    /* remove trailing directory delimiter (except root directory) */
    if (count == 3 && *(path + 1) == ':') remove = FALSE;
    if (count == 1 && *path == '\\') remove = FALSE;
    if (remove)
    {
      if (*(path +count -1) == '\\') *(path +count -1) = '\0';
    }
  }

  return path;

}


