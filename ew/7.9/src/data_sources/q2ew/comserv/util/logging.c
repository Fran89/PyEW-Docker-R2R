#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#include "dpstruc.h"
#include "cfgutil.h"

/* much of this is borrowed from Earthworm logit and logit_init() but I
  did not want to link comserv to EW and thus, this pair of 
  functions is born - Paul Friberg 2005-11-29
*/

#define         DATE_LEN                10   /** Length of the date field **/
 
static FILE   *fp = NULL;
static char   date[DATE_LEN];
static char   date_prev[DATE_LEN];
static char  log_path[1024];
static char  log_name[1024];
static char  log_fullpath[2048+1+16];
static int   log_bufsize;
static int   log_mode;
static char * buf;
static int   init=0;

#define NETWORK_FILE 	"/etc/network.ini"
#define CLIENT_NAME     "NETM"


void GetLogParamsFromNetwork(char *logdir, char *logtype) {
config_struc network_cfg;           /* structure for config file.   */
char str1[SECWIDTH], str2[SECWIDTH];

    /* Scan network initialization file.                                */
    if (open_cfg(&network_cfg,NETWORK_FILE,CLIENT_NAME)) {
        fprintf (stderr, "Could not find network file %s\n", NETWORK_FILE);
        exit(1);
    }
    while (1) {
            read_cfg(&network_cfg,&str1[0],&str2[0]);
            if (str1[0] == '\0') break;
            /* Mandatory daemon configuration parameters.               */
            else if (strcmp(str1,"LOGDIR")==0) {
                strcpy(logdir,str2);
            }
            else if (strcmp(str1,"LOGTYPE")==0) {
                strcpy(logtype,str2);
            }
   }
}

/* *************************************************
LogInit initializes the logging file for output.
	mode -  a flag to indicate the output mode,
		can be  CS_LOG_MODE_TO_STDOUT or CS_LOG_MODE_TO_LOGFILE

	path - the directory to put logs into

	logname - a unique name to identify this logfile from any other, left
		as an exercise to the user (e.g, station_name+prog_name etc...).

			logfiles will be placed in path and have the name:
			path/logname_YYYYMMDD.log

	buf_size - the size in bytes of the largest message to log

	RETURNS 0 upon success, -1 upon failure;
*/
int LogInit(int mode, char *path, char *logname, int buf_size)
{

struct tm *gmt_now;
time_t    now_epoch;

    /* check all input args */
    if (mode==CS_LOG_MODE_TO_LOGFILE && path == NULL) 
    {
	fprintf(stderr, "LogInit(): Fatal Error, path to log dir not provided\n");
	return -1;
    }
    if (logname == NULL) 
    {
	fprintf(stderr, "LogInit(): Fatal Error, logname not provided\n");
	return -1;
    }
    if (buf_size <= 36) 
    {
	fprintf(stderr, "LogInit(): Fatal Error, buf_size of %d needs to be positive (and large)\n", buf_size);
	return -1;
    }
    if (mode == CS_LOG_MODE_TO_STDOUT || mode == CS_LOG_MODE_TO_LOGFILE) 
    {
        log_mode=mode;
    } 
    else 
    {
	fprintf(stderr, "LogInit(): Fatal Error, mode of %d needs to be one of CS_LOG_MODE_TO_STDOUT or CS_LOG_MODE_TO_LOGFILE/n", mode);
	return -1;
    }

    /* store all args  for later */
    if (mode == CS_LOG_MODE_TO_LOGFILE) 
    {
    	strcpy(log_path, path);
    	strcpy(log_name, logname);
    }
    log_bufsize = buf_size;

    /* allocate the message buffer */
    if ( (buf = (char *) malloc( (size_t) log_bufsize)) == NULL)
    {
	fprintf(stderr, "LogInit(): Fatal Error, malloc() of %d bytes failed.\n", buf_size);
	return -1;
    }
    switch (mode) 
    {
  	case CS_LOG_MODE_TO_STDOUT:
		fp=stdout;
		break;
  	case CS_LOG_MODE_TO_LOGFILE:
		time(&now_epoch);
		gmt_now = gmtime(&now_epoch);
		sprintf(date, "%04d%02d%02d", gmt_now->tm_year + 1900, gmt_now->tm_mon, gmt_now->tm_mday);
		strcpy(date_prev, date);
		sprintf(log_fullpath,"%s/%s_%s.log", log_path, log_name, date);
		if ( (fp = fopen(log_fullpath, "a")) == NULL )
   		{
			fprintf(stderr, "LogInit(): Fatal Error, could not open %s for writing\n", log_fullpath);
			return -1;
		}
    		fprintf(fp,"%s_%02d:%02d:%02d - START - Logfile %s opened\n", date, gmt_now->tm_hour, 
			gmt_now->tm_min, gmt_now->tm_sec, log_fullpath);
    		fflush(fp);

		break;
    }
    init = 1;
    return 0;
}
/* *************************************************

	LogMessage() - logs the message as per instructed with a format of the following:
		YYYYMMDD_00:00:00 - type - message
		where type is one of INFO, DEBUG, ERROR

	The second argument to LogMessage() is the sprintf style string followed by
	a variable argument list.

	RETURNS 0 upon success, -1 upon failure
*/

#define LOG_MAX_TYPE 3
char log_type_string[LOG_MAX_TYPE][10]= {"INFO", "DEBUG", "ERROR"};

int LogMessage(int type, char *format, ...)
{

auto va_list ap;
struct tm *gmt_now;
time_t    now_epoch;
int  ret;


    if (!init)
    {
	fprintf(stderr, "LogMessage(): Fatal Error, LogInit() not called first\n");
	return -1;
    }
    if (type < 0 || type >= LOG_MAX_TYPE)
    {
	fprintf(stderr, "LogMessage(): Fatal Error, type of %d provided not in range\n", type);
	return -1;
    }

    /* get date for comparison */
    time(&now_epoch);
    gmt_now = gmtime(&now_epoch);
    sprintf(date, "%04d%02d%02d", gmt_now->tm_year + 1900, gmt_now->tm_mon, gmt_now->tm_mday);
    if (fp != stdout && strcmp(date, date_prev) != 0)
    {
    	fprintf(fp,"%s_%02d:%02d:%02d - END - UTC date changed; Logfile continues in file with date %s\n", date, gmt_now->tm_hour, 
			gmt_now->tm_min, gmt_now->tm_sec, date);
    	close(fp);
	sprintf(log_fullpath,"%s/%s_%s.log", log_path, log_name, date);
	if ( (fp = fopen(log_fullpath, "a")) == NULL )
   	{
		fprintf(stderr, "LogMessage(): Fatal Error, could not open new file %s for writing\n", log_fullpath);
		return -1;
	}
    	fprintf(fp,"%s_%02d:%02d:%02d - START - UTC date changed; Logfile continues from with date %s\n", date, gmt_now->tm_hour, 
			gmt_now->tm_min, gmt_now->tm_sec, date_prev);
	strcpy(date_prev, date);
    }

    /* now write to the buffer */
    va_start( ap, format );
    ret = vsnprintf(buf, log_bufsize, format, ap);
    va_end(ap);

    if (ret > log_bufsize || ret == -1)
    {
    	fprintf(fp,"%s_%02d:%02d:%02d - ERROR - LogMessage() called with too big a mesage for buffer specified.\n", date, gmt_now->tm_hour, 
			gmt_now->tm_min, gmt_now->tm_sec);
	return -1;
    }
	
    fprintf(fp,"%s_%02d:%02d:%02d - %s - %s\n", date, gmt_now->tm_hour, 
			gmt_now->tm_min, gmt_now->tm_sec, log_type_string[type], buf);
    fflush(fp);
    return 0;
}
