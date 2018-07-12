#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "geolib.h"

/***********************************************************************
 * copy_file
 *
 * Description: Copies any (single) file
 *
 * Input parameters: source_file - the source path / file name
 *		     dest_file - the destination path / file name
 *		     overwrite - flag - TRUE = overwrite existing files
 *		      			FALSE = don't
 * Output parameters: none
 * Returns: Error codes as follows:
 *	 	COPY_OK	- copy worked OK
 *		COPY_SELF - cannot copy file onto itself
 *			    (ie. source_file == dest_file)
 *		COPY_BAD_SOURCE - could not find source file
 *		COPY_DEST_EXISTS - destination file already exists
 *				   (only used when overwrite == FALSE)
 *		COPY_BAD_DEST - could not create destination file
 *		COPY_WRITE_ERR - write error - porbably not enough space
 *				 for destination file
 *
 * Comments: 
 *
 ***********************************************************************/
int copy_file (char *source_file, char *dest_file, int overwrite)

{

  int status, ret_val = COPY_OK, buffer;
  FILE *source_fp = (FILE *) 0, *dest_fp = (FILE *) 0;
  struct stat info;


  /* check source and destination files are different */
  if (! stricmp2 (source_file, dest_file)) ret_val = COPY_SELF;

  /* get attributes from source file */
  if (stat (source_file, &info)) ret_val = COPY_BAD_SOURCE;

  /* check if destination file already exists */
  if ((ret_val == COPY_OK) && (! overwrite))
  {
    if (access (dest_file, 0)) ret_val = COPY_DEST_EXISTS;
  }

  /* open source file, create destination file */
  if (ret_val == COPY_OK)
  {
    source_fp = fopen (source_file, "rb");
    dest_fp = fopen (dest_file, "wb");
    if (! source_fp) ret_val = COPY_BAD_SOURCE;
    if (! dest_fp) ret_val = COPY_BAD_DEST;
  }

  /* copy from source to destination */
  if (ret_val == COPY_OK)
  {
    for (;;)
    {
      buffer = fgetc (source_fp);
      if (buffer == EOF) break;
      status = fputc (buffer, dest_fp);
    }
    if (status == EOF) ret_val = COPY_WRITE_ERR;
  }

  /* close the files */
  if (source_fp) fclose (source_fp);
  if (dest_fp) fclose (dest_fp);

  /* apply the stored attributes to the destination file */
  if (ret_val == COPY_OK) chmod (dest_file, info.st_mode);

  return ret_val;

}

