/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: cleandir.c 2291 2006-06-06 19:27:09Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/06/06 19:27:09  paulf
 *     added from hydra
 *
 *     Revision 1.4  2006/05/08 21:55:24  mark
 *     Removed debug call to cleandir_service_main (prevented cleandir from running as a service)
 *
 *     Revision 1.3  2006/04/05 15:57:27  davidk
 *     Added code to optionally parse a single environment variable from
 *     the start of a "Directory" entry, in order to support things like $EW_LOG
 *     and $EW_PARAMS\picks\temp.   (MS Specific)
 *
 *     Revision 1.2  2005/07/11 20:42:37  mark
 *     Properly handle deletion errors, log sharing violations correctly
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:38  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2005/05/09 21:32:12  mark
 *     Fixed directory parsing from config file
 *
 *     Revision 1.3  2005/05/09 21:09:48  mark
 *     Removed archaic file functions to get better error reporting
 *
 *     Revision 1.2  2005/04/11 19:43:43  mark
 *     Overhaul to incorporate Win32 services, config files, and multiple directories
 *
 */

// Utility to erase all files in directory <dir> which were last modified
// more than <nday> days ago.

// This program uses WIN32 API calls.

// Original version by Will Kohler 4/3/00


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <direct.h>        /* For _chdir() */
#include <sys/types.h>     /* For _stat() */
#include <sys/stat.h>      /* For _stat() */
#include <windows.h>
#include "service_ew.h"
#include "watchdog_client.h"
#include "earthworm_simple_funcs.h"

#define CLEANDIR_SERVICE_NAME	"cleandir"
#define CLEANDIR_DISPLAY_NAME	"Hydra cleandir"

// Maximum number of directories that can be specified in the config file.
#define MAX_DIRS	16

static char szDirectory[MAX_DIRS][256];
static int dirCount;
static int ndays = 100;
static int checkpoint;
static int done = 0;
static int SleepTime = 14400;	// Time btw. loops, in seconds
static BOOL bService;

static void cleandir_service_main(int argc, char *argv[]);
static void service_handler(unsigned int dwControl);
static BOOL ReadConfig();

int main( int argc, char *argv[] )
{
/* Set name of configuration file
	******************************/
	if (argc == 2)
	{
		if (stricmp(argv[1], "-install") == 0)
		{
			install_service(CLEANDIR_SERVICE_NAME, CLEANDIR_DISPLAY_NAME, NULL);
			return 0;
		}
		if (stricmp(argv[1], "-uninstall") == 0)
		{
			uninstall_service(CLEANDIR_SERVICE_NAME);
			return 0;
		}
		else
		{
			fprintf(stderr, "Usage:\t%s -install\n\t%s -uninstall\n\t%s <ndays> <directory>\n",
					argv[0], argv[0], argv[0]);
			return 1;
		}
	}

	reportErrorInit(1024, 1, argv[0]);

	fprintf(stderr, "Starting service...(this may take a while if the service isn't installed)\n");
	bService = TRUE;
	if (start_service(CLEANDIR_SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)cleandir_service_main) != 0)
	{
		// This is being run as something other than a service.  We need to get our parameters
		// from the command line, rather than a file.  Make sure they're there...
		if (argc != 3)
		{
			fprintf(stderr, "\nInvalid parameters\n");
			fprintf(stderr, "Usage:\t%s -install\n\t%s -uninstall\n\t%s <ndays> <directory>\n",
					argv[0], argv[0], argv[0]);
			reportErrorCleanup();
			return 1;
		}
		else
		{
			ndays = atoi(argv[1]);
			strcpy(szDirectory[0], argv[2]);
			dirCount = 1;
		}

		// Run this program directly, instead of through the service manager.
		fprintf(stderr, "Service not installed.  Running once, then exiting...\n");
		bService = FALSE;
		cleandir_service_main(argc, argv);
	}
	logit("e", "Finished executing %s\n", argv[0]);
	reportErrorCleanup();
	return 0;
}

void cleandir_service_main(int argc, char *argv[])
{
	char            *fname;
	FILETIME		currentTime;
	LARGE_INTEGER	curTimeInt, fileTimeInt, tempInt;
	SYSTEMTIME		sysTime;
	const char      fileMask[] = "*";
	static HANDLE   handle;
	WIN32_FIND_DATA findData;
	double          ageDays;
	time_t			now, last = 0;
	int				err = 0, fileerr;
	int				i;

	if (bService)
	{
		// Initialize the service.
		init_service(CLEANDIR_SERVICE_NAME, 0, (LPHANDLER_FUNCTION)service_handler);
		set_service_status(SERVICE_START_PENDING, 0, 0, 2000);

		// Read the config file.
		if (!ReadConfig())
		{
			// Error reading the file...stop now.
			set_service_status(SERVICE_STOPPED, -1, 0, 0);
			return;
		}

		reportError(WD_INFO, 0, "Service initialized successfully.\n");
		set_service_status(SERVICE_RUNNING, 0, 0, 2000);
	}

	// Main loop.  If this is a service, run until the SCM tells us to stop, or until
	// we encounter an error.  If this isn't running as a service, it will only run once.
	while (!done)
	{
		// Get the current time.
		now = time(NULL);
		if (now - last < SleepTime)
		{
			// Not time yet to delete the files...sleep a bit.
			sleep_ew(1000);
			continue;
		}

		// Get the current time as a LARGE_INTEGER struct (i.e. int64)
		GetSystemTime(&sysTime);
		SystemTimeToFileTime(&sysTime, &currentTime);
		curTimeInt.u.LowPart = currentTime.dwLowDateTime;
		curTimeInt.u.HighPart = currentTime.dwHighDateTime;

		// It's time to start deleting.  Step through each directory on our list.
		for (i = 0; i < dirCount; i++)
		{
			reportError(WD_INFO, 0, "Beginning delete check on %s\n", szDirectory[i]);

			// Change the current directory to the dir to examine.
			if (!SetCurrentDirectory(szDirectory[i]))
			{
				fileerr = GetLastError();
				reportError(WD_FATAL_ERROR, SYSERR, "Error %d changing current directory to: %s\n",
							fileerr, szDirectory[i] );
				fprintf(stderr, "Error %d changing current directory to: %s\n",
							fileerr, szDirectory[i] );
				break;
			}

			// Initialize our search handle, used to find all the files in this directory.
			handle = FindFirstFile(fileMask, &findData);
			if (handle == INVALID_HANDLE_VALUE)
				continue;

			// Delete loop.  This loops until all the files in the directory have been examined
			// and deleted, or an error happens.
			while (TRUE)
			{
				// Get the name of a file
				if (!FindNextFile(handle, &findData))
					break;

				// Look only at regular files which were
				// modified more than <nday> days ago.
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					continue;

				fname = findData.cFileName;

				// Convert the last-modified time to int64
				fileTimeInt.u.LowPart = findData.ftLastWriteTime.dwLowDateTime;
				fileTimeInt.u.HighPart = findData.ftLastWriteTime.dwHighDateTime;

				// Convert from 100-nanosecond intervals to days
				tempInt.QuadPart = (curTimeInt.QuadPart - fileTimeInt.QuadPart);
				tempInt.QuadPart = tempInt.QuadPart / 10000000;
				ageDays = (double)(tempInt.QuadPart / 86400.);

				if (ageDays < ndays)
					continue;

				// This file is old enough...remove the file
				logit("e", "Removing %s...\n", fname);
				if (!DeleteFile(findData.cFileName))
				{
					fileerr = GetLastError();
					if (fileerr == ERROR_SHARING_VIOLATION)
					{
						// Unless this happens a lot, this won't be a fatal error - we don't expect
						// too much of the hard drive to be taken up with files left open...
						reportError(WD_WARNING_ERROR, SYSERR, "Can't delete %s: File sharing violation.\n", 
									fname);
					}
					else
					{
						// Everything else, we should scream a bit about.
						reportError(WD_FATAL_ERROR, SYSERR, "Error removing file %s: %d\n", 
									fname, fileerr );
					}

					// Set the error code we'll quit with later, and continue on to the
					// next file.
					err = fileerr;
				}
			} // end of delete loop

			// Close our search handle for this directory.
			FindClose(handle);

			reportError(WD_INFO, 0, "Finished delete check on %s\n", szDirectory[i]);
		} // end of for loop

		// Remember the current time as the time we last ran.
		last = now;

		// Quit now if this isn't a service.
		if (!bService)
			done = 1;
	} // end of main loop

	// If this is a service, tell the SCM we're stopped.
	if (bService)
		set_service_status(SERVICE_STOPPED, err, 0, 0);
}

void service_handler(unsigned int dwControl)
{
	switch(dwControl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		reportError(WD_INFO, 0, "Stop service message received.\n");

		checkpoint = 0;
		set_service_status(SERVICE_STOP_PENDING, 0, 0, 3000);
		checkpoint = checkpoint + 1;

		done = 1;
		break;

	case SERVICE_CONTROL_INTERROGATE:
		logit("e", "Interrogate service message received.\n");
		set_service_status(0, 0, 0, 0);

	default:;
	}
}

BOOL ReadConfig()
{
	FILE *pFile;
	char *szToken;
	char szFile[256];
	char szBuffer[256];
  char szEnv[MAX_PATH];
  char * szEnvOut;
  char * szEnd;

	strcpy(szFile, getenv("EW_PARAMS"));
	if (strlen(szFile) == 0)
	{
		reportError(WD_FATAL_ERROR, SYSERR, "EW_PARAMS not defined!\n");
		return FALSE;
	}
	strcat(szFile, "\\cleandir.d");

	pFile = fopen(szFile, "r");
	if (pFile == NULL)
	{
		reportError(WD_FATAL_ERROR, SYSERR, "Error opening cleandir.d\n");
		return FALSE;
	}

	dirCount = 0;
	while (TRUE)
	{
		// Get the next line from the file.
		if (fgets(szBuffer, 256, pFile) == NULL)
			break;

		// Ignore comments and empty lines.
		if (szBuffer[0] == '#' || szBuffer[0] == '\r' || szBuffer[0] == '\n')
			continue;

		szToken = strtok(szBuffer, " \t\n");
		if (szToken == NULL)
			continue;

		if (stricmp(szToken, "Age") == 0)
		{
			szToken = strtok(NULL, " \t\n");
			ndays = atoi(szToken);
		}
		else if (stricmp(szToken, "CheckInterval") == 0)
		{
			szToken = strtok(NULL, " \t\n");
			SleepTime = atoi(szToken);
		}
		else if (stricmp(szToken, "Directory") == 0)
		{
			if (dirCount >= MAX_DIRS)
			{
				reportError(WD_FATAL_ERROR, CFGERR, "Too many directores specified in cleandir.d (max %d)\n",
							MAX_DIRS);
				fclose(pFile);
				return FALSE;
			}
      /* THE FOLLOWING CODE IS MS SPECIFIC
         DK 040506 */
			szToken = strtok(NULL, "\n");
			while (*szToken == ' ' || *szToken == '\t')
				szToken++;

      if(*szToken == '$')
      {
        szEnd=strchr(szToken, '\\');
        if(szEnd)
        {
          strncpy(szEnv, szToken+1, szEnd-szToken-1);
          szEnv[szEnd-szToken-1]=0x00;
        }
        else
        {
          strcpy(szEnv, szToken+1);
        }
        szEnvOut=getenv(szEnv);
        if(szEnvOut)
			    strcpy(szDirectory[dirCount], szEnvOut);
        else
        {
          reportError(WD_FATAL_ERROR, CFGERR, "Could not resolve dir entry <%s> into a meaningful directory.\n", szToken);
          fclose(pFile);
          return FALSE;
        }
        if(szEnd)
          strcat(szDirectory[dirCount], szEnd);
      }
      /* END MS SPECIFIC CODE */
      else
      {
			  strcpy(szDirectory[dirCount], szToken);
      }
      logit("","Adding directory to list: <%s>\n", szDirectory[dirCount]);
			dirCount++;
		}
		else
		{
			reportError(WD_WARNING_ERROR, CFGERR, "Unknown token \"%s\" in cleandir.d",
						szToken);
		}
	}

	fclose(pFile);
	return TRUE;
}

