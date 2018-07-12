
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sendmail.c 4511 2011-08-09 18:23:40Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2006/05/19 23:51:58  dietz
 *     Added "from" argument which was required in Windows version. It's a
 *     dummy argument in this Solaris version
 *
 *     Revision 1.5  2004/04/12 22:27:51  dietz
 *     included string.h
 *
 *     Revision 1.4  2001/03/15 23:53:31  alex
 *     removed "free( cmnd )" commands, since cmnd is not malloced! Alex
 *
 *     Revision 1.3  2001/03/14 21:11:37  lucky
 *     Changed the way that the message is built and sent -- no more
 *     standard input method -- just echo the message into the mail program.
 *
 *     Revision 1.2  2000/06/30 17:35:12  lombard
 *     Added call to stat() to check for existence of mail program
 *
 *     Revision 1.1  2000/02/14 18:46:17  lucky
 *     Initial revision
 *
 *
 */

                     /*******************************
                      *          sendmail           *
                      *                             *
                      *   Function to send email.   *
                      *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <earthworm.h>


#define DEFAULT_MAIL_PROG	"/usr/ucb/Mail"
#define MAXLEN_MAIL_PROG_LINE	512

/*
 * SendMail ()
 *   list of receipients
 *   number of emails to send
 *   mail program to use
 *   subject of the message
 *   message to send
 *   message prefix
 *   message suffix
 *   mail server to use
 *
 *
 * Modified by Lucky Vidmar Tue Jan 19 16:05:00 MST 1999
 *  to include the message and the configurable mail program
 *
 */


int SendMail( char person[][60], int nmail, char *mailProg, 
                      char *subject, char *msg, char *msgPrefix, 
                      char *msgSuffix, char *mailServer,
                      char *from )
{
   FILE *f;
   char MsgText[MAX_EMAIL_MSG_SIZE]; 
   char cmnd[2*MAX_EMAIL_MSG_SIZE]; 
   char *msg_ptr;	
   int  i, max_char;
   struct stat mailstat;
   char mailProgLine[MAXLEN_MAIL_PROG_LINE];
   char mailProgstat[MAXLEN_MAIL_PROG_LINE];

   mailProgLine[MAXLEN_MAIL_PROG_LINE-1] = 0;
   mailProgstat[MAXLEN_MAIL_PROG_LINE-1] = 0;

   /* Check incomming arguments */
   if (mailServer == NULL)
   {
      logit( "et", "sendmail: Invalid arguments passed in\n" );
      return( -1 );
   }

   if (mailProg != NULL) {
      strncpy (mailProgLine, mailProg, MAXLEN_MAIL_PROG_LINE-1);
   } else {
      strncpy (mailProgLine, DEFAULT_MAIL_PROG, MAXLEN_MAIL_PROG_LINE-1);
   }

   /* Make sure we can find the mail program */
   strncpy(mailProgstat, mailProgLine, MAXLEN_MAIL_PROG_LINE-1);
   i = 0;
   while(mailProgstat[i] != ' '  &&  i<strlen(mailProgstat)) {
	   i++;
   }
   mailProgstat[i] = 0;
   if ( stat(mailProgstat, &mailstat) < 0)
   {
     logit("et", "SendMail: error accessing mail program <%s>: %s\n",
             mailProgstat, strerror(errno));
     return( -1 );
   }
   /* Maybe we should check for execute permissions but that takes more work */


   /* Build the message */
   if (msgPrefix != NULL)
       sprintf (MsgText, "%s%s", msgPrefix, msg);
   else 
		sprintf (MsgText, "%s", msg);
   
   if (msgSuffix != NULL)
        strcat (MsgText, msgSuffix);

   for ( i = 0; i < nmail; i++ )
   {
      /* Mail message without a subject */
      if (subject == NULL)
      {
         (void) sprintf( cmnd, "echo \"%s\" | %s %s", 
								MsgText, mailProgLine, person[i] );
      }
      /* Mail message with the given subject */
      else
      {
         (void) sprintf( cmnd, "echo \"%s\" | %s -s \"%s\" %s", 
									MsgText, mailProgLine, subject, person[i] );
      }

      if ( ( f = popen( cmnd, "w" ) ) == NULL )
      {
         logit( "et", "sendmail: Can't run command \"%s\"\n", cmnd );
         return( -1 );
      }
      pclose( f );
   }
   return( 0 );
}

