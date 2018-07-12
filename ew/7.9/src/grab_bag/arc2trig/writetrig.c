/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: writetrig.c 500 2001-04-08 00:55:38Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2001/04/08 00:55:38  lombard
 *     made fclose() conditional on Disk.
 *
 *     Revision 1.2  2000/08/08 18:26:14  lucky
 *     Lint cleanup
 *
 *     Revision 1.1  2000/02/14 16:04:49  lucky
 *     Initial revision
 *
 *
 */

  /************************************************************
   *                          writetrig.c                     *
   *                                                          *
   *         Functions for maintaining trigger files.         *
   *                  (based on logit.c)                      *
   *                                                          *
   *     First, call writetrig_Init.  Then, call writetrig.   *
   *     Call writetrig_close before exitting!                *
   *                                                          *
   *       These functions are NOT MT-Safe, since they store  *
   *     data in static buffers: PNL, 12/8/98                 *
   ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <time_ew.h>
#include "arc2trig.h"

static FILE   *fp;
static char   date[9];
static char   date_prev[9];
static time_t now;
static char   trigName[100];
static char   trigfilepath[70];
static char   template[25];
static struct tm res;
static int    Init  = 0;        /* 1 if writetrig_Init has been called */
static int    Disk  = 1;        /* 1 if output goes to Disk file       */
void   logit( char *, char *, ... );  /* logit.c      sys-independent  */
/*************************************************************************
 *                             writetrig_Init                            *
 *                                                                       *
 *      Call this function once before the other writetrig routines.     *
 *                                                                       *
 *************************************************************************/
int writetrig_init( void )
{
   char *str;
   char baseName[50];
   int  lastchar;

/* Set time zone using the TZ environmental variable.
   This is not required under Solaris.
   In OS2 v2 or v3.0, use _tzset().
   In OS2 v3.0, use tzset().
   *************************************************/
#if defined(_OS2) || defined(_WINNT)
   if ( getenv( "TZ" ) != NULL ) _tzset();
#endif

/* Make sure we should write a trigger file
   ****************************************/
   if     ( strncmp(TrigFileBase, "none",4)==0 )  Disk=0;
   else if( strncmp(TrigFileBase, "NONE",4)==0 )  Disk=0;
   else if( strncmp(TrigFileBase, "None",4)==0 )  Disk=0;
   else if( strncmp(OutputDir,    "none",4)==0 )  Disk=0;
   else if( strncmp(OutputDir,    "NONE",4)==0 )  Disk=0;
   else if( strncmp(OutputDir,    "None",4)==0 )  Disk=0;
   if( Disk==0 ) return( 0 );

/* Truncate everything beyond and
   including "." in the base file name
   ***********************************/
   strcpy( baseName, TrigFileBase );
   str = strchr( baseName, '.' );
   if ( str != NULL ) *str = '\0';

/* Check Init flag
   ***************/
   if( Init ) return( 0 );
   Init = 1;

/* Get path & base file name from config-file parameters
   *****************************************************/
   strcpy ( trigfilepath, OutputDir );
   lastchar = strlen(OutputDir)-1;

#if defined(_OS2) || defined(_WINNT)
   if( OutputDir[lastchar] != '\\' &&  OutputDir[lastchar] != '/' )
      strcat( trigfilepath, "\\" );
#endif
#ifdef _SOLARIS
   if( OutputDir[lastchar] != '/' ) strcat( trigfilepath, "/" );
#endif

   sprintf( template, "%s.trg_", baseName );

/* Build trigger file name by appending time
   *****************************************/
   time( &now );
   gmtime_ew( &now, &res );
   sprintf( date, "%04d%02d%02d", (res.tm_year+1900), (res.tm_mon+1),
            res.tm_mday );

   strcpy( trigName,  trigfilepath );
   strcat( trigName,  template );
   strcat( trigName,  date );
   strcpy( date_prev, date );

/* Open trigger list file
   **********************/
   fp = fopen( trigName, "a" );
   if ( fp == NULL )
   {
      logit("e",
            "arc2trig: Error opening triglist file <%s>\n",
             trigName );
      return( -1 );
   }

/* Print startup message to trigger file
   *************************************/
   fprintf( fp, "\n-------------------------------------------------\n" );
   fprintf( fp, "arc2trig: startup at UTC_%s_%02d:%02d:%02d",
                 date, res.tm_hour, res.tm_min, res.tm_sec );
   fprintf( fp, "\n-------------------------------------------------\n" );
   fflush ( fp );

/* Log a warning message
   *********************/
#if defined(_OS2) || defined(_WINNT)
   if ( getenv( "TZ" ) == NULL )
   {
      writetrig("WARNING: The TZ environmental variable is not set.\n" );
      writetrig("         Roll-over dates of trigger files may be bogus.\n" );
   }
#endif

   return( 0 );
}


/*****************************************************************
 *                            writetrig                          *
 *                                                               *
 *          Function to log a message to a Disk file.            *
 *                                                               *
 *  flag: A string controlling where output is written:          *
 *        If any character is 'e', output is written to stderr.  *
 *        If any character is 'o', output is written to stdout.  *
 *        If any character is 't', output is time stamped.       *
 *                                                               *
 *  The rest of calling sequence is identical to printf.         *
 *****************************************************************/
int writetrig( char *note )
{
   int rc;

/* Check Init flag
   ***************/
   if ( !Init )
   {
     rc = writetrig_init();
     if( rc != 0 ) return( rc );
   }
   if ( !Disk ) return( 0 );

/* Get current system time
   ***********************/
   time( &now );
   gmtime_ew( &now, &res );

/* See if the date has changed.
   If so, create a new trigger file.
   *********************************/
   sprintf( date, "%04d%02d%02d", (res.tm_year+1900), (res.tm_mon+1),
            res.tm_mday );

   if ( strcmp( date, date_prev ) != 0 )
   {
      fprintf( fp,
              "UTC date changed; trigger output continues in file <%s%s>\n",
               template, date );
      fclose( fp );
      strcpy( trigName, trigfilepath );
      strcat( trigName, template );
      strcat( trigName, date );
      fp = fopen( trigName, "a" );
      if ( fp == NULL )
      {
         fprintf( stderr, "Error opening trigger file <%s%s>!\n",
                  template, date );
         return( -1 );
      }
      fprintf( fp,
              "UTC date changed; trigger output continues from file <%s%s>\n",
               template, date_prev );
      strcpy( date_prev, date );

/* Send a warning message to the new log file
   ******************************************/
#if defined(_OS2) || defined(_WINNT)
      if ( getenv( "TZ" ) == NULL )
      {
         fprintf( fp, "WARNING: The TZ environmental variable is not set.\n" );
         fprintf( fp, "         Roll-over dates of trigger files may be bogus.\n" );
      }
#endif
   }

/* write the message to the trigger file
 ***************************************/
   fprintf( fp, "%s", note );
   fflush( fp );

   return( 0 );
}

void writetrig_close()
{
   if (Disk)
     fclose( fp );
   return;
}
