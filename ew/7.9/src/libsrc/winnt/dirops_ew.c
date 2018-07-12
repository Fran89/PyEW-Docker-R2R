
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dirops_ew.c 4845 2012-06-01 01:27:46Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2007/07/19 19:20:25  paulf
 *     fixed escape of dir sep
 *
 *     Revision 1.5  2007/07/19 19:17:26  paulf
 *     added in OpenDir and GetNextFileName funcs
 *
 *     Revision 1.4  2001/04/05 18:19:31  cjbryan
 *     added RecursiveCreateDir function to recursively create all
 *     directories in a specifed path
 *
 *     Revision 1.3  2000/09/28 23:19:38  dietz
 *     added function rename_ew
 *
 *     Revision 1.2  2000/03/10 23:36:48  davidk
 *     added an include of <direct.h> to resolve a compiler warning about
 *     Create_dir()
 *
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

         /************************************************************
          *            dirops_ew.c  Windows NT version               *
          *                                                          *
          *  Contains system-specific functions for dealing with     *
          *  directories                                             *
          ************************************************************/

#include <windows.h>
#include <sys/stat.h>
#include <errno.h>
#include <earthworm.h>
#include <stdio.h>
#include <direct.h>


           /*******************************************************
            *  chdir_ew( )  changes current working directory     *
            *                                                     *
            *  Returns 0 if all went well                         *
            *         -1 if an error occurred                     *
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



/*****************************************************************************
 *  CreateDir ()  Creates a directory. Solaris version.                      *
 *                                                                           *
 *  if dirname exists and is accessible, EW_SUCCESS is returned. Otherwise,  *
 *  we attempt to create it. If it all goes well, we return EW_SUCCESS,      *
 *  otherwise we report error and return EW_FAILURE.                         *
 *       LV 8/16/1999                                                        *
 *****************************************************************************/
int CreateDir (char *dirname)
{
   struct stat buf;

   if (dirname == NULL)
   {
      logit ("e", "Invalid argument passed in; exiting!\n");
      return EW_FAILURE;
   }

/* does it already exist? */
   if (stat (dirname, &buf) != 0)
   {
      if (errno == ENOENT)
      {
         if (mkdir (dirname) != 0)
         {
            logit ("e", "CreateDir: Cannot create %s: %s\n",
                    dirname, strerror(errno) );
            return( EW_FAILURE);
         }
      }
      else
      {
         logit ("e", "CreateDir: Cannot stat %s - %s \n", 
                 dirname, strerror (errno));
         return EW_FAILURE;
      }

   }
   else
   {
      /* do nothing - hope that this is a directory */
   }

   return EW_SUCCESS;

}

/*****************************************************************************
 *  Recursive CreateDir ()  Recursively creates all directories in a	     *
 *  specified path. First checks to see if only directory to create is last  *
 *  one in path; if not creates directories one at a time through calls to   *
 *  to CreateDir().		
 *                                                                           *
 *  if dirname exists and is accessible, EW_SUCCESS is returned. Otherwise,  *
 *  we attempt to create it. If it all goes well, we return EW_SUCCESS,      *
 *  otherwise we report error and return EW_FAILURE.                         *
 *       CJB 3/29/01                                                         *
 *****************************************************************************/
int RecursiveCreateDir(char *dirname) {
	
	struct	stat	buf;
	char		*dname;
	char		*where;
	int		place;
	int 		len;
	char		find[] = "\\/";
	char		directory[MAX_DIR_LEN];

	/* check to make sure directory name is not too long */
	if (strlen(dirname) > (MAX_DIR_LEN - 1)) 
	{
		logit("e", "RecursiveCreateDir: directory name %s exceeds allowed length %d \n",
				dirname, MAX_DIR_LEN);
		return EW_FAILURE;
	}

	strcpy(directory, "");
	if (dirname == NULL)
	{
		return EW_FAILURE;
	}

	/* does it already exist? */
	if (stat (dirname, &buf) != 0)
	{
		if (errno == ENOENT)
		{
			/* first lets just try and make it; if this doesn't work
				we'll start at the beginning and make each subdirectory */
			if (mkdir (dirname) != 0)
			{
				dname = dirname;
				while ((where = strpbrk(dname, find)) != NULL) {
					place = where - dname;
					len = strlen(directory);
					strncat(directory, dname, place);
					directory[len + place] = '\0';
					if (directory[strlen(directory) - 1] != ':')
					{
						if(CreateDir(directory) != EW_SUCCESS)
						{
							logit ("e", "RecursiveCreateDir: Cannot create %s\n",
								directory);
							return EW_FAILURE;
						}
					}
					/* add directory delimiter '\'  */
					strcat(directory, "\\");
					dname += place + 1;
					
				} /* end of while */
				/* finally add name of final subdirectory */
				strcat(directory, dname);
				if(CreateDir(directory) != EW_SUCCESS)
				{
					logit ("e", "RecursiveCreateDir: Cannot create %s\n",
						directory);
					return EW_FAILURE;
				}
			}
		}
		else
		{
			return EW_FAILURE;
		}

	}
	else
	{
		/* do nothing - hope that this is a directory */
	}

   return EW_SUCCESS;
}

/***************************************************************
 *  GetFileName   (from Will Kohler's sendfile)                *
 *                                                             *
 *  Function to get the name of a file in directory "Path".    *
 *                                                             *
 *  Returns 0 if all ok                                        *
 *          1 if no files were found                           *
 ***************************************************************/

int GetFileName( char fname[] )
{
   extern char Path[80];     /* Directory containing files to be sent */
   char            fileMask[] = "*";
   WIN32_FIND_DATA findData;
   HANDLE          fileHandle;
   FILE            *fp;

   strcpy( fileMask, "*" );

/* Get the name of the first file.
   The file may be a directory or a partially-written file.
   If so, skip this file and look for others.
   *******************************************************/
   fileHandle = FindFirstFile( fileMask, &findData );
   if ( fileHandle == INVALID_HANDLE_VALUE )         /* No files found */
      return 1;

   fp = fopen( findData.cFileName, "rb" );
   if ( fp != NULL )                 /* File can be opened */
   {
      fclose( fp );
      strcpy( fname, findData.cFileName );
      FindClose( fileHandle );
      return 0;
   }

/* First file is a directory or it is otherwise unopenable.
   Find another file, if any.
   *******************************************************/
   while ( FindNextFile( fileHandle, &findData ) )
   {
      fp = fopen( findData.cFileName, "rb" );

      if ( fp != NULL )                 /* File can be opened */
      {
         fclose( fp );
         strcpy( fname, findData.cFileName );       /* Found a file */
         FindClose( fileHandle );
         return 0;
      }
   }

   FindClose( fileHandle );          /* No files found */
   return 1;
}
/**********************************************************************
int OpenDir(char * path) - opens a directory to be used by
	GetNextFileName() function
	returns 0 upon success
	returns 1 upon problems openining directory specified by path
 **********************************************************************/
   static WIN32_FIND_DATA open_findData;
   static char * open_dir_name=NULL;
   static int on_first_file=0;
   static HANDLE first_fileHandle;

int OpenDir(char * path) 
{
 char fullpath_with_wildcard[2048];
   
   on_first_file=0;
   open_dir_name = path;
   sprintf(fullpath_with_wildcard, "%s\\*", path);
   first_fileHandle = FindFirstFile( fullpath_with_wildcard, &open_findData );
   if ( first_fileHandle == INVALID_HANDLE_VALUE )         /* No files found */
   {
      return 1;
   }
   on_first_file=1;
   return 0;
}

/**********************************************************************
int GetNextFileName(char fname[]) to be called after OpenDir
	returns 0 upon success (with file in fname)
	returns 1 upon problems  or no more files
 **********************************************************************/
int GetNextFileName(char fname[])
{

   if (on_first_file) {
	strcpy(fname, open_findData.cFileName);
	on_first_file=0;
	return 0;
   }
   if (FindNextFile(first_fileHandle, &open_findData)) {
	strcpy(fname, open_findData.cFileName);
	return 0;
   }

   FindClose(first_fileHandle);
   return 1;
}


/**********************************************************************
 *  rename_ew( )  Moves a file                   Windows version      *
 *  path1 = name of file to be moved.  path2 = destination name       *
 *  Returns -1 if an error occurred; 0 otherwise                      *
 **********************************************************************/

int rename_ew( char *path1, char *path2 )
{
   if ( MoveFileEx( path1, path2, MOVEFILE_REPLACE_EXISTING ) == 0 )
      return -1;

   return 0;
}

