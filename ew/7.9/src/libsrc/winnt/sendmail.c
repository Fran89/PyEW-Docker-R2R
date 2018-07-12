
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sendmail.c 4026 2010-09-24 17:45:03Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2007/02/27 04:55:11  stefan
 *     null username check per Alex
 *
 *     Revision 1.6  2006/05/19 23:47:51  dietz
 *     Added argument to SendMail() so user can specify the "From" field of
 *     outgoing email. Defaults to %USERNAME%@%COMPUTERNAME% if a null pointer
 *     or empty string is passed in the argument. Previously defaulted to
 *     "root@mailserver".
 *
 *     Revision 1.5  2003/06/11 19:13:14  kohler
 *     Increased BUFFSIZE from 200 to 250 in define statment.
 *
 *     Revision 1.4  2002/09/30 19:34:52  alex
 *     changed 'printf' to 'logit'
 *     Alex
 *
 *     Revision 1.3  2001/10/02 18:15:18  dietz
 *     Changed to use COMPUTERNAME in the email sender field
 *     instead of Earthworm.
 *
 *     Revision 1.2  2000/05/23 18:05:31  dietz
 *     Changed blat sender to root@<MailHost>
 *
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

     /*****************************************************************
      *                            sendmail                           *
      *                                                               *
      *         Function to send email.  Windows NT version.          *
      *                                                               *
      *  This function requires a mail server computer.               *
      *                                                               *
      *  person:     List of email recipients                         *
      *  nmail:      The number of recipients                         *
      *  mailProg:   Mail program to use - for Solaris compatibility  *
      *  subject:    Subject of the message                           *
      *  msg  :      The body of the email message                    *
      *  msgPrefix : Prefix to the body of the message                *
      *  msgSuffix : Suffix to the body of the message                *
      *  mailServer: Computer that sends the mail message.            *
      *                                                               *
      *  Returns -1 if an error occurred, 0 otherwise                 *
      *                                                               *
      *  mailProg, subject, msgPrefix, and msgSuffix added by         *
      *    Lucky Vidmar  Tue Jan 19 16:17:01 MST 1999                 *
      *                                                               *
      *****************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <earthworm.h>

#define BUFFSIZE 250
#define NO_SENDMAIL_ERROR  0
#define SENDMAIL_ERROR    -1

static char Sender[BUFFSIZE];
static int  SenderInit = 0;

int SendMail( char person[][60], int nmail, char *mailProg, char *subject,
              char *msg, char *msgPrefix, char *msgSuffix, char *mailServer,
              char *from )
{
   FILE                *fp;
   DWORD               pathSize;
   char                tmp[BUFFSIZE];
   char                pathBuffer[BUFFSIZE];
   char                tempFilename[MAX_PATH];
   char                commandLine[BUFFSIZE*4];
   STARTUPINFO         startUpInfo;
   BOOL                success;
   DWORD               priorityClass;
   PROCESS_INFORMATION procInfo;
   DWORD               exitCode;
   DWORD               rc;
   int                 i;
   int                 return_code = SENDMAIL_ERROR;

/* Check arguments
   ***************/
   if ( nmail < 1 ) return SENDMAIL_ERROR;

/* Do once: Load Sender with thisuser@thiscomputer
   ***********************************************/
   if ( !SenderInit )
   {
      char nobody[]  = "nobody";
      char *user     = getenv( "USERNAME" );
      char *computer = getenv( "COMPUTERNAME" );
      int  len;

      if ( user == NULL ) user = nobody;

      len = strlen(user) + strlen(computer) + 1;
      if ( len < BUFFSIZE )
         sprintf( Sender, "%s@%s", user, computer );
      else if ( strlen(computer) < BUFFSIZE )
         strcpy( Sender, computer );
      else
         strcpy( Sender, "unknown" );
      SenderInit = 1;
   }

/* Get the path of the temporary file directory
   ********************************************/
   pathSize = GetTempPath( BUFFSIZE, pathBuffer );
   if ( pathSize < BUFFSIZE )
      pathBuffer[pathSize] = 0;
   else
   {
      logit( "et", "Error getting path of temporary directory for blat message." );
      return SENDMAIL_ERROR;
   }

/* Get a unique file name in the temporary file directory
   ******************************************************/
   if ( GetTempFileName( pathBuffer, "mai", 0, tempFilename ) == 0 )
   {
      logit( "et", "Error getting name of temporary file for blat message.\n" );
      return SENDMAIL_ERROR;
   }

/* Write the mail message to the temporary file
   ********************************************/
   fp = fopen( tempFilename, "w" );
   if ( fp == NULL )
   {
      logit( "et", "Error opening temporary file for blat message.\n" );
      return SENDMAIL_ERROR;
   }

   if ( msgPrefix != NULL )
       fputs( msgPrefix, fp );

   fputs( msg, fp );

   if ( msgSuffix != NULL )
       fputs( msgSuffix, fp );

   fclose( fp );

/* Build a command line for the Blat program, which sends
   the temporary file to the mail server computer.
   ******************************************************/
   strcpy( commandLine, "blat " );
   strcat( commandLine, tempFilename );                 // Name of message file

   if( subject != NULL )                                // Optional subject line
   {
       sprintf( tmp, " -s \"%s\"", subject );
       strcat( commandLine, tmp );
   }
                                                        // Specify the sender
   strcat( commandLine, " -f " );
   if ( from && strlen(from)!=0 ) strcat( commandLine, from );
   else                           strcat( commandLine, Sender );

   strcat( commandLine, " -server " );                  // Specify the SMTP server
   strcat( commandLine, mailServer );

// strcat( commandLine, " -log blat.log -timestamp" );  // For debugging

   strcat( commandLine, " -ti 60" );                    // Time out in 60 seconds

   strcat( commandLine, " -t " );                       // List of recipients
   strcat( commandLine, &person[0][0] );
   for ( i = 1; i < nmail; i++ )
   {
      strcat( commandLine, "," );
      strcat( commandLine, &person[i][0] );
   }
// logit( "et", "blat command line:\n.%s.\n",commandLine );  // For debugging

/* Invoke the blat program
   ***********************/
   GetStartupInfo( &startUpInfo );
   priorityClass = GetPriorityClass( GetCurrentProcess() );

   success = CreateProcess( 0,
                            commandLine,     // Command line to invoke child
                            0, 0,            // No security attributes
                            FALSE,           // No inherited handles
                            0 |              // Child does not get a window
                            priorityClass,   // Same as priority of parent
                            0,               // Not passing environmental vars
                            0,               // Current dir same as parent
                            &startUpInfo,    // Attributes of process window
                            &procInfo );     // Attributes of child process
   if ( !success )
   {
      logit( "et", "CreateProcess error starting blat: %d\n", GetLastError() );
      goto DELETE_TEMPFILE;
   }

/* If blat doesn't complete in 120 seconds, kill the process.
   This should never occur, because the blat timeout is set
   to 60 seconds using the -ti option above.
   *********************************************************/
   rc = WaitForSingleObject( procInfo.hProcess, 120000 );
   if ( rc == WAIT_FAILED )
   {
      logit( "et", "Blat WaitForSingleObject() failed with error: %d\n", GetLastError() );
      goto CLOSE_HANDLES;
   }
   if ( rc == WAIT_TIMEOUT )
   {
      logit( "et", "Error. The blat process did not complete within 120 seconds.\n" );

      success = TerminateProcess( procInfo.hProcess, 0 );
      if ( success )
         logit( "et", "Forcefully terminated the blat process.\n" );
      else
         logit( "et", "Error terminating the blat process.\n" );
      goto CLOSE_HANDLES;
   }

/* The blat process completed.
   Check its exit code for abnormal termination.
   ********************************************/
   success = GetExitCodeProcess( procInfo.hProcess, &exitCode );
   if ( !success )
   {
      logit( "et", "Error getting the Blat exit code: %d\n", GetLastError() );
      goto CLOSE_HANDLES;
   }
   if ( exitCode != 0 ) logit( "et", "Blat error.  Exit code: %d\n", exitCode );

   switch ( exitCode )
   {
   case 13:
      logit( "et", "Blat error opening temporary file in temp directory.\n" );
      break;
   case 12:
      logit( "et", "Blat -server or -f option not specified.\n" );
      break;
   case 5:
      logit( "et", "Blat error reading message text file.\n" );
      break;
   case 4:
      logit( "et", "Mail file not of type FILE_TYPE_DISK.\n" );
      break;
   case 3:
      logit( "et", "Blat error reading message text file or attachment file.\n" );
      break;
   case 2:
      logit( "et", "Likely cause of blat error: Mail server denied our connection.\n" );
      logit( "et", "See also: http://www.blat.net/examples/blat_return_codes\n" );
      break;
   case 1:
      logit( "et", "Likely cause of blat error: Network problem or mail server down.\n" );
      logit( "et", "See also: http://www.blat.net/examples/blat_return_codes\n" );
      break;
   case 0:
      logit( "et", "Blat completed without errors.  Mail sent.\n" );
      return_code = NO_SENDMAIL_ERROR;
      goto CLOSE_HANDLES;
   default:
      logit( "et", "Unknown blat exit code.\n" );
   }

/* Close process and thread handles
   ********************************/
CLOSE_HANDLES:
   CloseHandle( procInfo.hProcess );
   CloseHandle( procInfo.hThread );

/* Delete the temporary mail file
   ******************************/
DELETE_TEMPFILE:
   success = DeleteFile( tempFilename );
   if ( !success )
      logit( "et", "Error deleting blat temporary file: %s\n", tempFilename );
   return return_code;
}
