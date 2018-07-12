#include <string.h>
#include <stdlib.h> 
#include <ctype.h> 
#include <dirent.h>

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

  struct stat info2;
  struct dirent *dirent;
  char filename [200];

  static char stored_path [200];
  static DIR *dir_handle;
  static int dir_open = FALSE;
  static struct File_info info;


  do
  {
    /* get information on the next file */
    if (path)
    {
      if (dir_open)
      {
        closedir (dir_handle);
        dir_open = FALSE;
      }
      strcpy (stored_path, path);
      terminate_path (stored_path, TRUE);
      dir_handle = opendir (stored_path);
      if (! dir_handle) return 0;
      dir_open = TRUE;
    }
    else if (! dir_open) return 0;
    else if (! (dirent = readdir (dir_handle)))
    {
      closedir (dir_handle);
      dir_open = FALSE;
      return 0;
    }

    /* get information on the file */
    sprintf (filename, "%s%s", stored_path, dirent->d_name);
  }
  while (stat (filename, &info2));

  /* convert the information to local format */
  strcpy (info.name, dirent->d_name);
  info.size = info2.st_size;
  info.creation_time = info2.st_ctime;
  info.modification_time = info2.st_mtime;
  info.access_time = info2.st_atime;
  if (info2.st_mode & S_IFDIR) info.file_type = FILE_DIR;
  else if (info2.st_mode & S_IFREG) info.file_type = FILE_NORMAL;
  else info.file_type = FILE_OTHER;

  return &info;

}

/*************************************************************************
 * terminate_path
 *
 * Input parameters: path - path to terminate
 *		     flag - TRUE = force a terminating /
 *		            FALSE = remove any terminating /
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
  count = strlen (path);
  if (! count) return path;

  /* add or remove ?? */
  if (flag)
  {
    if (*(path +count -1) == '/') add = FALSE;
    if (add) strcat (path, "/");
  }
  else
  {
    /* remove trailing directory delimiter (except root directory) */
    if (count == 1 && *path == '/') remove = FALSE;
    if (remove)
    {
      if (*(path +count -1) == '/') *(path +count -1) = '\0';
    }
  }

  return path;

}


