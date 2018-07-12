
/*************************************************************
 *                           log.c                           *
 *                                                           *
 *           This is a standalone version of logit.          *
 *                                                           *
 *  First, call log_init().  Then, call log().               *
 *  These functions are NOT multi-thread safe.               *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "sendfile_srv.h"

#define DATE_LEN 10              /* Length of the date field */

static char logname[128];        /* Name from the config file */
static char fname[256];
static char *buf;
static FILE *fp;
static char date[DATE_LEN];
static char date_prev[DATE_LEN];
static int  init   = 0;           /* 1 if log_init has been called */
static int  screen = 0;           /* 1 if output goes to screen */
static int  disk   = 0;           /* 1 if output goes to disk file   */


/*************************************************************************
 *                              log_init                                 *
 *                                                                       *
 *       Call this function once before the other log routines.          *
 *                                                                       *
 *   LogName : Name of the log file, including path.                     *
 *             Date and time will be appended to LogName.                *
 *                                                                       *
 *   tz      : Time zone of system clock (eg GMT, PST8PDT)               *
 *             Required under Windows; ignored under Solaris             *
 *                                                                       *
 *   bufSize : Size of buffer to be allocated by log_init.               *
 *             This buffer must be large enough to accomodate the        *
 *             largest log message to be written.                        *
 *                                                                       *
 *   logflag : If 0, don't log to screen or disk.                        *
 *             If 1, log to screen only.                                 *
 *             If 2, log to disk only.                                   *
 *             If 3, log to screen and disk.                             *
 *************************************************************************/

void log_init( char *LogName, char *tz, int bufSize, int logflag )
{
    struct tm *res;
    time_t now;

    /* Check init flag
***************/
    if ( init )
    {
	fprintf( stderr, "Error: log_init called more than once. Exiting.\n" );
	exit( -1 );
    }
    init = 1;

    /* Set up flag values
******************/
    if ( (logflag < 0) || (logflag > 3) )
    {
	fprintf( stderr, "Error. logflag <%d> out of range. Exiting.\n",
		 logflag );
	exit( -1 );
    }
    if ( (logflag == 1) || (logflag == 3) ) screen = 1;
    if ( (logflag == 2) || (logflag == 3) ) disk   = 1;
    if ( logflag == 0 ) return;

    /* Set time zone using the TZ environmental variable.
       TzSet() is a dummy function under Solaris.
    *************************************************/
    TzSet( tz );

    /* Allocate buffer from heap
*************************/
    buf = (char *) malloc( (size_t)bufSize );
    if ( buf == (char *)NULL )
    {
	fprintf( stderr, "log_init: malloc error. Exiting.\n" );
	exit( -1 );
    }

    /* The rest of this function is for disk files only
************************************************/
    if ( disk == 0 ) return;

    /* Save the logfile name, including path
*************************************/
    strcpy( logname, LogName );

    /* Build log file name by appending the current date
*************************************************/
    time( &now );
    res = gmtime( &now );
    sprintf( date, "%04d%02d%02d", (res->tm_year + 1900), (res->tm_mon + 1),
	     res->tm_mday );
    strcpy( fname, logname );
    strcat( fname, "_" );
    strcat( fname, date );
    strcpy( date_prev, date );

    /* Open log file
*************/
    fp = fopen( fname, "a" );
    if ( fp == NULL )
    {
	fprintf( stderr, "Error opening log file <%s>. Exiting\n", fname );
	exit( -1 );
    }

    /* Print startup message to log file
*********************************/
    fprintf( fp, "\n----------------------------------------------------\n" );
    fprintf( fp, "Startup at UTC_%s_%02d:%02d:%02d\n",
	     date, res->tm_hour, res->tm_min, res->tm_sec );
    fprintf( fp, "----------------------------------------------------\n" );
    fflush ( fp );
    return;
}


/*****************************************************************
 *                             Log                               *
 *                                                               *
 *          Function to log a message to a disk file.            *
 *                                                               *
 *  flag: A string controlling where output is written:          *
 *        If any character is 'e', output is written to stderr.  *
 *        If any character is 'o', output is written to stdout.  *
 *        If any character is 't', output is time stamped.       *
 *                                                               *
 *  The rest of calling sequence is identical to printf.         *
 *****************************************************************/

void log( char *flag, char *format, ... )
{
    struct tm *res;
    time_t    now;
    auto va_list ap;
    static char   *fl;
    int    stout      = 0;      /* 1 if output is also to stdout   */
    int    sterr      = 0;      /* 1 if output is also to stderr   */
    int    time_stamp = 0;      /* 1 if output is time-stamped     */

    /* Check init flag
     ***************/
    if ( !init )
    {
	fprintf( stderr, "Error. Call log_init() before log(). Exiting.\n" );
	return;
    }

    /* Check logging flags
     *******************/
    if ( (disk == 0) && (screen == 0) ) return;

    /* Check flag argument
     *******************/
    fl = flag;
    while ( *fl != '\0' )
    {
	if ( *fl == 'o' ) stout      = 1;
	if ( *fl == 'e' ) sterr      = 1;
	if ( *fl == 't' ) time_stamp = 1;
	fl++;
    }

    /* Construct a string containing current date
     ******************************************/
    time( &now );
    res = gmtime( &now );
    sprintf( date, "%4d%02d%02d", (res->tm_year + 1900), (res->tm_mon + 1),
	     res->tm_mday);

    /* See if the date has changed.
       If so, create a new log file.
    *****************************/
    if ( disk && (strcmp( date, date_prev ) != 0) )
    {
	strcpy( fname, logname );
	strcat( fname, "_" );
	strcat( fname, date );
	fprintf( fp, "UTC date changed; log output continues in file <%s>\n",
		 fname );
	fclose( fp );

	fp = fopen( fname, "a" );
	if ( fp == NULL )
	{
	    fprintf( stderr, "Error opening log file <%s>. Exiting.\n", fname );
	    exit( -1 );
	}
	strcpy( fname, logname );
	strcat( fname, "_" );
	strcat( fname, date_prev );
	fprintf( fp, "UTC date changed; log output continues from file <%s>\n",
		 fname );
	strcpy( date_prev, date );
    }

    /* Write UTC time and argument list to buffer
     ******************************************/
    va_start( ap, format );
    buf[0] = 0;             /* NULL terminate the empty buf */

    if ( time_stamp )
	sprintf( buf+strlen(buf), "%s_UTC_%02d:%02d:%02d ", date,
		 res->tm_hour, res->tm_min, res->tm_sec );

    vsprintf( buf+strlen(buf), format, ap );
    va_end( ap );

    /* Write buffer to standard output and standard error
     **************************************************/
    if ( screen && stout )
	printf( "%s", buf );

    if ( screen && sterr )
	fprintf( stderr, "%s", buf );

    /* Write buffer to disk file
     *************************/
    if ( disk )
    {
	fprintf( fp, "%s", buf );      /* If fprintf fails, we won't know it */
	fflush( fp );
    }
    return;
}

