/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: watchdog_client.h 2322 2006-06-12 04:36:36Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/06/12 04:35:41  stefan
 *     hydra resources
 *
 *     Revision 1.13  2005/06/30 02:10:43  davidk
 *     Attempting to reform logging.
 *     1) Modified the available severity levels (added MAJOR_INFO) ,
 *     2) Added a shortdesc for informational messages
 *     3) Added a function to allow program to control whether INFO
 *     severity levels are logged to disk.
 *
 *     Revision 1.12  2005/01/12 21:43:08  davidk
 *     Changed the behavior of the WD_DEBUG flag in reportError() calls,
 *     so that it is now runtime adjustable, instead of a compile flag.
 *     Removed #define, and added a function prototype for turning on/off the
 *     logging of debug statements to file
 *
 *     Revision 1.11  2005/01/03 16:45:20  mark
 *     Changed EnableErrorBoxes param
 *
 *     Revision 1.10  2004/12/28 20:15:20  mark
 *     Added code to disable those annoying Windows error boxes
 *
 *     Revision 1.9  2004/11/02 06:18:25  davidk
 *     Added includes of stdlib.h and stdarg.h for va_list support.
 *     Added function reportErrorVA() which is identical to reportError,
 *     except that it accepts supplemental arugments as a "va_list" instead
 *     of as "..."
 *
 *     Revision 1.8  2004/10/22 19:50:07  davidk
 *     Add TTERR for traveltime library errors.
 *
 *     Revision 1.7  2004/08/05 23:37:33  michelle
 *     added #define for REPORT_ERROR_DEBUG as a compile time flag to control
 *     logging of info/debug messages
 *
 *     Revision 1.6  2004/07/15 18:28:22  davidk
 *     Added DBNODATA error code.
 *
 *     Revision 1.5  2004/07/12 15:56:47  mark
 *     Added reportGeneralError function
 *
 *     Revision 1.4  2004/07/01 16:43:36  mark
 *     Removed "red" and "blue" error ports
 *
 *     Revision 1.3  2004/06/17 18:20:53  mark
 *     Added #defines for commonly-used error-reporting ports
 *
 *     Revision 1.2  2004/05/20 21:43:11  mark
 *     Removed multicast IP and port params; init function now gets them from environment vars
 *
 *     Revision 1.1  2004/05/18 21:43:47  mark
 *     Moved from include (need C++ compile for Multicast socket)
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:18  michelle
 *     New Hydra Import
 *
 *     Revision 1.8  2004/03/24 01:26:53  michelle
 *     added define of PPINVAL
 *
 *     Revision 1.7  2003/06/24 20:30:23  dhanych
 *     added NORESULT
 *
 *     Revision 1.6  2003/06/24 19:30:52  dhanych
 *     added PPINVAL, changed GENFATERR code to clear the ceiling
 *
 *     Revision 1.5  2003/06/24 19:28:56  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2003/05/22 17:16:46  lucky
 *     Changed prototypes to return int;  Added some more short strings
 *
 *     Revision 1.3  2003/05/22 16:28:06  dhanych
 *     Added Waveserver error descriptions
 *
 *     Revision 1.2  2003/05/22 15:47:09  michelle
 *     made messageFormat string a const in reportError signature
 *
 *     Revision 1.1  2003/05/21 15:00:33  michelle
 *     Initial revision
 *
 *     Revision 1.2  2003/05/20 22:53:41  michelle
 *     made short descs ints, for ease of comparison, may need to add corresponding set of strings for dumping out to log file and in theory to email sent as notifications to humans
 *
 *     Revision 1.1  2003/05/20 21:15:41  michelle
 *     Initial revision
 *
 *
 */

#ifndef WATCHDOG_CLIENT_H
#define WATCHDOG_CLIENT_H

#include <stdlib.h>
#include <stdarg.h>

/* Prototypes for functions in watchdog.c
 ***********************************/

/*****************************
 * reportErrorInit
 * initializes the error reporting feature,
 * which includes calling logit_Init with 
 * appropriate parameters.  Also initializes a
 * multicast socket if appropriate.
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
                       const char *callingProgramName);


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
int   registerMe(int bRegisterFlag
                  , long pid   
                  , long heartBeatInterval);


/*****************************
 * EnableErrorBoxes
 *
 * Enables or disables those annoying Windows message boxes that pop up when memory gets
 * stepped on, or the stack gets overwritten, etc.  This allows us to quit an application
 * without waiting for the user to click OK.
 *
 * NOTE:  Error boxes are disabled by default in reportErrorInit().  You need to call this
 *        again as EnableErrorBoxes(TRUE) if you want these boxes (e.g. for displays).
 *
 * @param bEnable - TRUE to allow message boxes to appear, FALSE to prevent them.
 *****************************/
void EnableErrorBoxes(int bEnable);

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
int   reportError(int severityLevel
                 , int shortDesc
                 , const char *messageFormat
                 , ... );  

int   reportErrorVA(int severityLevel
                 , int shortDesc
                 , const char *messageFormat
                 , va_list ap);  

/*****************************
 * reportGeneralError
 * Logs errors via UDP broadcasts only, and does not log errors to a file.
 * reportGeneralError is used when the program that calls reportErrorInit (e.g. statmgr)
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
int reportGeneralError(const char *szModuleName,
					   int severityLevel,
					   int shortDesc,
					   const char *messageFormat,
					   ... );

int   reportErrorCleanup();

int   reportErrorSetDebug(int bDebugOn);

int   reportErrorSetLogInfo(int bLogInfoOn);



/*****************************
 * Severity Levels
 *******************************************/
enum WD_SEVERITY_LEVEL 
{
    WD_DEBUG = 1            /* Plethora of debug information(think kitchen sink).
                               OFF by default,
                               Only logged if reportErrorSetDebug(1) is called. 
                               Timestamped: NO
                               EXAMPLE:  "Top of for loop for each passport Entry" */

  , WD_INFO = 0             /* Lots of program information and processing milestones, 
                               ON - logged to file by default.  Turn off with reportErrorSetLogInfo(0).
                               Timestamped: NO
                               EXAMPLE:   "Filter coefficients:   1 -6.39235757e-003"*/

  , WD_MAJOR_INFO = 2       /* Major milestones, (such as per channel results in mag calculators), 
                               and expected minor errors (such as couldn't get data for a channel.
                               ON - logged to file 
                               Timestamped: NO
                               EXAMPLE:   "<YKW3:BHZ:CN:--> not on the sanctioned stations list; skipping! "*/

  , WD_WARNING_ERROR = -1   /* Minor problems/errors.  
                               ON - ERROR Wire, Screen, Logged to file. 
                               Timestamped: YES
                               EXAMPLE:   "Could not connect to wave_server 192.168.0.1:16024"
                               EXAMPLE:   "Channel <YKW3:BHZ:CN:--> not found in DB." "*/

  , WD_FATAL_ERROR = -2     /* MAJOR PROBLEMS/ERRORS.  - Show stoppers!
                               ON - ERROR Wire, Screen, Logged to file. 
                               Timestamped: YES
                               EXAMPLE:   "Could not connect to DB: ewdb_main@eqs.red"
                               EXAMPLE:   "ewdb_api_CreateMagnitude() failed for  Mwp 6.5, 20 channels" */

};

/*****************************
 * Error Short Descriptions
 *******************************************/

#define GENFATERR   1      /* Generic fatal error (ha!) */

#define NORESULT    90     /* Unable to obtain result */
#define INFOONLY    91     /* Informational Message */

/****** Station List Error Short Descriptions */
#define STAERR   100     /* Generic Station List Error */
#define STAERRS  "STAERR"     /* Generic Station List Error */
#define STANFND  101     /* Station Not Found */

/****** Database Error Short Descriptions */
#define DBERR    200     /* Generic Database Error */
#define DBCONN   201     /* Cannot Connect to DB */
#define DBNODATA 202     /* Data Not Found in DB */

/****** Passport Error Short Descriptions */
#define PPERR    300     /* passport read problem */
#define PPREAD   301     /* passport read problem */
#define PPUNKN   302     /* unknown passport line */
#define PPMISS   303     /* missing a required passport line */
#define PPINVAL  304     /* invalid passport */

/****** Command Line Args Error Short Descriptions */
#define CMDLNERR 400     /* command line parameter problem */
#define CFGERR   420     /* Configuration file error */

/****** System Error Short Descriptions */
#define SYSERR   500     /* memory allocation failure */
#define MEMALLOC 501     /* memory allocation failure */
#define CLASINST 502     /* failed to instantiate a working class */

/****** Misc Error Short Descriptions */
#define TTERR    550       /* TravelTime library error */

/****** WaveServer Error Short Descriptions */
#define WSERR    600       /* Waveserver error */
#define WSCONN   601       /* WS connection error */

/****** Earthworm Related Errors Short Descriptions */
#define EWERR    700       /* Generic earthworm error */

/****** OTHER NEEDED Short descriptions */


#endif /* WATCHDOG_CLIENT_H */
