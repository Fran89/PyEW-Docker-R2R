
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: watchdog_client.cpp 2323 2006-06-12 04:55:51Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/06/12 04:55:39  stefan
 *     hydra resources
 *
 *     Revision 1.20  2005/08/29 21:39:50  davidk
 *     Added code that complains if reportError() is called
 *     without first calling reportErrorInit().
 *     Code now complains to stderr and attempts a debug-halt of the program.
 *
 *     Revision 1.19  2005/06/30 03:20:22  davidk
 *     Modified default behavior for WD_DEBUG calls to NOT LOG.
 *     Set default behavior for WD_INFO calls to LOG.
 *     Modified reportError() so that it is a wrapper for reportErrorVA(), that
 *     way all SeverityLevel checking is done in one place in the code
 *     (with the exception of reportGeneralError() which I didn't understand and
 *      want to touch)
 *     Removed SeverityLevel() checks from HandleUDPBroadcast().
 *     Reformatted whitespace in HandleUDPBroadcast().
 *
 *     Revision 1.18  2005/06/16 15:44:16  mark
 *     Removed DEBUG messages from all UDP broadcasts
 *
 *     Revision 1.17  2005/01/12 21:44:27  davidk
 *     Changed the behavior of the WD_DEBUG flag in reportError() calls,
 *     so that it is now runtime adjustable, instead of a compile flag.
 *     Added a function reportErrorSetDebug() for turning on/off the
 *     logging of debug statements to file
 *
 *     Revision 1.16  2005/01/03 16:45:28  mark
 *     Changed EnableErrorBoxes param
 *
 *     Revision 1.15  2004/12/28 20:18:30  mark
 *     Added code to disable those annoying Windows error boxes
 *
 *     Revision 1.14  2004/11/02 06:21:51  davidk
 *     Added function reportErrorVA() which is identical to reportError,
 *     except that it accepts supplemental arugments as a "va_list" instead
 *     of as "..."
 *
 *     Revision 1.13  2004/10/18 16:53:20  mark
 *     Changed szMachineName to first try SYS_NAME environment var; moved UDP messages to HandleUDPBroadcast
 *
 *     Revision 1.12  2004/10/14 16:51:54  mark
 *     Fixed re-initialization bug
 *
 *     Revision 1.11  2004/08/05 23:38:22  michelle
 *     added check for REPORT_ERROR_DEBUG to control
 *     logging and sending of info/debug messages
 *
 *     Revision 1.10  2004/07/12 15:56:54  mark
 *     Added reportGeneralError function
 *
 *     Revision 1.9  2004/07/01 16:45:04  mark
 *     Changed error reporting to use UDP broadcasts and CDatagramSocket instead of multicast
 *
 *     Revision 1.8  2004/06/17 18:29:13  mark
 *     Added socket "types" (error-reporting or non-error-reporting) for CMulticastSocket
 *
 *     Revision 1.7  2004/06/15 17:56:32  mark
 *     Fix for when reportError is called before multicast socket is properly initialized
 *
 *     Revision 1.6  2004/06/09 19:00:18  mark
 *     Added initialization logits to verify multicast ports
 *
 *     Revision 1.5  2004/06/01 15:09:23  mark
 *     Re-added crashing to verify reportErrorInit was called for each executable
 *
 *     Revision 1.4  2004/05/28 20:36:12  mark
 *     Added sanity checks to verify reportErrorInit was properly called...
 *
 *     Revision 1.3  2004/05/20 23:02:03  labcvs
 *     Moved gethostname call to after sockets have been initialized
 *
 *     Revision 1.2  2004/05/20 21:44:05  mark
 *     Removed multicast IP and port params; init function now gets them from environment vars
 *
 *     Revision 1.1  2004/05/18 21:44:38  mark
 *     Moved from libsrc (need C++ compile for Multicast socket); added multicast code to reportError
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:19  michelle
 *     New Hydra Import
 *
 *     Revision 1.4  2003/06/11 17:32:52  michelle
 *     added guts to reportError to write tp logit
 *
 *     Revision 1.3  2003/05/27 20:38:22  lucky
 *     Added cast to (char *) in logit_init call to comply with prototype
 *
 *     Revision 1.2  2003/05/22 23:33:46  michelle
 *     added call in reportErrorInit to logit_init
 *
 *     Revision 1.1  2003/05/22 21:47:31  michelle
 *     Initial revision
 *
 *
 *
 */

extern "C"
{
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <stdarg.h>
 #include <time.h>
 #include <earthworm.h>
 #include <watchdog_client.h>
}
#include <DatagramSocket.h>

static int HandleUDPBroadcast(int severityLevel, int shortDesc);

static CDatagramSocket *pSocket = NULL;
static char szMachineName[64];
static char szProgramName[64];
static char *szMulticastStr = NULL;	// declare this static so we don't have to keep allocating it
static char *msgStr = NULL;

static bool bInitialized = FALSE;

static int REPORT_ERROR_DEBUG = 0;
static int REPORT_ERROR_LOG_INFO = 1;

/*****************************
 * reportErrorInit
 * initializes the error reporting feature,
 * which includes calling logit_Init with
 * appropriate parameters.
 * reportErrorInit also determines the local machine name
 * via local system commands/environment variables.
 * the machine name will be used along with the
 * callingProgramName to uniquely identify applications.
 *
 * @param iBufferSize - is the max size of an
 *        error description that will be supplied
 *        to reportError for logging and for
 *        UDP distribution
 * @param logToLocalFileFlag - indicates whether to
 *        log to a local file or not.
 * @param callingProgramName - is used in the
 *        error report and log to identify where
 *        the error was logged from
 *******************************************/

int   reportErrorInit( int iBufferSize,
                       int bLogToLocalFileFlag,
                       const char *callingProgramName)
{
	char *ErrIP, *ErrPort, *MachineName;
	unsigned short port = 0;

	if (!bInitialized)
	{
		/* what should the mid value be??? */
		logit_init ((char *) callingProgramName, 1, iBufferSize, bLogToLocalFileFlag);

		// Disable error boxes by default, since most apps don't really want them...
		EnableErrorBoxes(FALSE);

		msgStr = (char *)malloc(iBufferSize);

		if (callingProgramName != NULL)
			strcpy(szProgramName, callingProgramName);
		else
			strcpy(szProgramName, "Unknown");

		ErrIP = getenv("HYDRA_ERR_IP");
		ErrPort = getenv("HYDRA_ERR_PORT");
		if (ErrPort != NULL)
			port = atoi(ErrPort);

		// Set up our multicast port, if we're set up for such things...
		if (pSocket == NULL && ErrIP != NULL && port != 0)
		{
			pSocket = new CDatagramSocket(ErrIP, port);
			if (pSocket->InitForSend() != 0)
			{
				// Error setting up our socket...delete it and don't use UDP.
				delete pSocket;
				pSocket = NULL;

				logit("", "\tError creating UDP port; reportError() will not use UDP broadcasts.\n");
			}
			else
			{
				strcpy(msgStr, "\tUDP error broadcasts enabled on ");
				strcat(msgStr, ErrIP);
				strcat(msgStr, ":");
				strcat(msgStr, ErrPort);
				strcat(msgStr, "\n");
				logit("", msgStr);

				szMulticastStr = (char *)malloc(iBufferSize + 256);
			}

			// Attempt to find out the machine name here.  Try the SYS_NAME environment variable first,
			// then ask the OS what the local hostname is.
			MachineName = getenv("SYS_NAME");
			if (MachineName != NULL)
				strcpy(szMachineName, MachineName);
			else if (gethostname(szMachineName, sizeof(szMachineName)) != 0)
				strcpy(szMachineName, "Unknown");
			logit("e", "Our machine name is: %s\n", szMachineName);
		}
		else
			logit("", "\tHYDRA_ERR_IP and HYDRA_ERR_PORT not defined; UDP broadcasts will be disabled.\n");

	}

	bInitialized = TRUE;
	return EW_SUCCESS;
}

int   reportErrorCleanup()
{
	free(msgStr);
	if (szMulticastStr)
		free(szMulticastStr);
	if (pSocket)
		delete pSocket;
	pSocket = NULL;

	bInitialized = FALSE;
	return EW_SUCCESS;
}

void EnableErrorBoxes(int bEnable)
{
// This whole mess only applies to Windows machines...
#ifdef WIN32
	DWORD dwMode;

	dwMode = SetErrorMode(0);
	if (bEnable != 0)
	{
		SetErrorMode( dwMode & (~SEM_NOGPFAULTERRORBOX) & (~SEM_FAILCRITICALERRORS) );
	}
	else
	{
		SetErrorMode( dwMode | SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS );
	}
#endif
}

/*****************************
 * registerMe
 * Broadcasts a UDP message intended for the
 * Watch Dog(s) to pick up and thus add the module
 * to its list of monitored applications.
 * registerMe determines the local machine name
 * via local system commands/environment variables.
 * the machine name will be used along with the
 * callingProgramName and pid (process id) to uniquely
 * identify applications.
 * Note that the Watch Dog should log/indicate
 * which applications it is monitoring, thus
 * to confirm monitoring or lack there of is
 * occurring the person configurating the
 * system should manually check the watch dog
 * accordingly
 *
 * @param registerFlag - indicates to register
 *        or to unregister the calling program
 *        for monitoring
 * @param pid - process id to uniquely identify
 *        the calling program/application
 * @param heartBeatInterval - the interval in secs
 *        of how often the calling program or
 *        application's heartbeat should be
 *        distributed
 *******************************************/
int   registerMe(int bRegisterFlag, long pid,
                  			long heartBeatInterval)
{
	/************************************
	 *
	 * This function doesn't actually do anything, nor will it for the foreseeable future.
	 * Our methodology now is to broadcast UDP error messages on the UDP broadcast address
	 * for a subnet (e.g. 192.168.12.255).  Any applications receiving the messages listen on
	 * the broadcast address.  This way, we don't have to register anything.
	 *
	 ************************************/

   int r_status = 0;
 
 
   return r_status;
}


/*****************************
 * reportError
 * Logs errors to a local log file based on directives set by reportErrorInit,
 * and it broadcasts a UDP message of the error. reportError will include (based on
 * initialization) the callingProgramName and machine name in the distributed UDP
 * error messages in order to uniquely identify the source of the reported error. 
 * Each processing machine will spew errors through the dedicated error wire subnet.
 *
 * @param severityLevel - is WD_SEVERITY_LEVEL
 *        indicates debug, info, warning, fatal
 * @param shortDesc - is a defined int that
 *        indicates type of error and will be used
 *        by the error notification system to
 *        determine who should be notified
 *        note these are ints for ease of comaprison
 * @param messageFormat - identical to printf
 *        indicates format of message string
 * @param remainder of params map to messageFormat
 *        just as is done in printf
 *******************************************/
int   reportError(int severityLevel, int shortDesc,
                 const char *messageFormat, ... )
{
	va_list ap;
  int     rc;

  if(!bInitialized)
  {
    fprintf(stderr, "CALL reportErrorInit() before calling reportError!!!!!!\n");
    DebugBreak();
  }

	/* put the variable arg list into a formated string for passing onto logit */ 
	va_start(ap, messageFormat);
	/*vsprintf(msgStr, messageFormat, ap); */
  rc=reportErrorVA(severityLevel, shortDesc, messageFormat, ap);
	va_end(ap);

  return(rc);
}


/*****************************
 * reportErrorVA
 * Same as reportError, but takes a va_list instead of ...
 * See reportError() for more information.
 *
 * @param severityLevel - is WD_SEVERITY_LEVEL
 *        indicates debug, info, warning, fatal
 * @param shortDesc - is a defined int that
 *        indicates type of error and will be used
 *        by the error notification system to
 *        determine who should be notified
 *        note these are ints for ease of comaprison
 * @param messageFormat - identical to printf
 *        indicates format of message string
 * @param ap - variable list (va_list) of params that map to messageFormat
 *        just as is done in printf
 *******************************************/
int   reportErrorVA(int severityLevel
                 , int shortDesc
                 , const char *messageFormat
                 , va_list ap)
{
	// Return early if this is a message we don't want to report...
	if( !REPORT_ERROR_DEBUG && severityLevel == WD_DEBUG)
	{
		return 0;
	}

	// Return early if this is a message we don't want to report...
	if( !REPORT_ERROR_LOG_INFO && severityLevel == WD_INFO)
	{
		return 0;
	}

   // Double-check that reportErrorInit got called; if not, there's not much we can do.
   // Return now to at least keep from crashing.
/* On second thought, let's crash.  All of our executables are expected to log to a file,
 * and if reportErrorInit is never called, then logging can't happen.  If we crash, it's
 * immediately clear what the problem is; if we just don't log, it could take much longer
 * before the problem is even discovered... (MMM 6/1/04)
   if (msgStr == NULL)
	   return 1;
*/

	/* put the variable arg list into a formated string for passing onto logit */ 
	vsprintf(msgStr, messageFormat, ap);

	// Log to the file.
  if(severityLevel ==  WD_FATAL_ERROR || severityLevel ==  WD_WARNING_ERROR)
  {
    logit("et", msgStr); 
	// Handle the UDP broadcast.
    return(HandleUDPBroadcast(severityLevel, shortDesc));
  }
  else if(severityLevel ==  WD_MAJOR_INFO)
  {
    logit("e", msgStr); 
    return(0);
  }
  else
  {
    logit("", msgStr); 
    return(0);
  }

}  // end reportErrorVA()


/*****************************
 * reportGeneralError
 * Logs errors via UDP broadcasts only, and does NOT log errors to a file.
 * This function is used when the program that calls reportErrorInit (e.g. statmgr)
 * is different than the program that generates the error (e.g. raypicker).  In this
 * example, the raypicker will generate an error, which will get passed up to statmgr.
 * statmgr will then call this function, passing "raypicker" as the calling module.
 *
 * @param szModuleName - name of the module that generated this message
 * @param severityLevel - is WD_SEVERITY_LEVEL
 *        indicates debug, info, warning, fatal
 * @param shortDesc - is a defined int that
 *        indicates type of error and will be used
 *        by the error notification system to
 *        determine who should be notified
 *        note these are ints for ease of comaprison
 * @param messageFormat - identical to printf
 *        indicates format of message string
 * @param remainder of params map to messageFormat
 *        just as is done in printf
 *******************************************/
int reportGeneralError(const char *szModuleName, int severityLevel, int shortDesc,
                 const char *messageFormat, ... )
{
   va_list ap; 

	// Return early if this is a message we don't want to report...
	if( !REPORT_ERROR_DEBUG &&
			(severityLevel != WD_WARNING_ERROR && severityLevel != WD_FATAL_ERROR) )
	{
		return 0;
	}

	// Double-check that reportErrorInit got called; if not, there's not much we can do.
	// Return now to at least keep from crashing.
/* On second thought, let's crash.  All of our executables are expected to log to a file,
 * and if reportErrorInit is never called, then logging can't happen.  If we crash, it's
 * immediately clear what the problem is; if we just don't log, it could take much longer
 * before the problem is even discovered... (MMM 6/1/04)
	if (msgStr == NULL)
	   return 1;
*/

	// put the variable arg list into a formated string for passing via UDP
	va_start(ap, messageFormat);
	vsprintf(msgStr, messageFormat, ap);
	va_end(ap);

	// Copy the module name into the string that will be reported later.  We can do this because
	// no programs will call both reportError and reportGeneralError; only statmgr2 should ever
	// call reportGeneralError.  If this changes, then this methodology needs to change!
	strcpy(szProgramName, szModuleName);

	// Handle the UDP broadcast.
	return HandleUDPBroadcast(severityLevel, shortDesc);
}


int HandleUDPBroadcast(int severityLevel, int shortDesc)
{
	char tempstr[32];
	unsigned int bytesSent;

	// Don't send debug messages; they just clutter up the landscape.
	if (severityLevel == WD_DEBUG)
		return 0;

	// Check that a socket has been set up.
	if (pSocket != NULL && szMulticastStr != NULL)
	{
    //
    // Assemble our message.  It will have the format:
    // <time> <callingProgramName> <machineName> <severityLevel> <shortDesc> <formatted message>
    //
    itoa(time(NULL), tempstr, 10);
    strcpy(szMulticastStr, tempstr);
    strcat(szMulticastStr, " ");
    strcat(szMulticastStr, szProgramName);
    strcat(szMulticastStr, " ");
    strcat(szMulticastStr, szMachineName);
    strcat(szMulticastStr, " ");
    itoa(severityLevel, tempstr, 10);
    strcat(szMulticastStr, tempstr);
    strcat(szMulticastStr, " ");
    itoa(shortDesc, tempstr, 10);
    strcat(szMulticastStr, tempstr);
    strcat(szMulticastStr, " ");
    strcat(szMulticastStr, msgStr);
    
    // Send the message, and log any errors.
    bytesSent = pSocket->Send((const unsigned char*)szMulticastStr, strlen(szMulticastStr));
    if (bytesSent < strlen(szMulticastStr))
    {
      logit("t", "\tUnable to send UDP error message");
      return -1;
    }
  }  /* if pSocket and szMulticastStr are valid */

	return 0;
}


/*****************************
 * reportErrorSetDebug
 * Set debug logging
 *
 * @param bDebugOn - set to true to turn on logging
 *        of debugging statements, otherwise WD_DEBUG
 *        level statements will be ignored
 *******************************************/
int   reportErrorSetDebug(int bDebugOn)
{
  if(bDebugOn)
    REPORT_ERROR_DEBUG = 1;
  else
    REPORT_ERROR_DEBUG = 0;

  return(0);
}

/*****************************
 * reportErrorSetDebug
 * Set debug logging
 *
 * @param bDebugOn - set to true to turn on logging
 *        of debugging statements, otherwise WD_DEBUG
 *        level statements will be ignored
 *******************************************/
int   reportErrorSetLogInfo(int bLogInfoOn)
{
  if(bLogInfoOn)
    REPORT_ERROR_LOG_INFO = 1;
  else
    REPORT_ERROR_LOG_INFO = 0;

  return(0);
}

