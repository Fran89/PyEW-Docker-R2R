
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rb_mss.c 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.2  2001/04/26 22:31:33  kohler
 *     Added -l option to logout MSS100 serial port.
 *
 *     Revision 1.1  2001/04/25 23:43:05  kohler
 *     Initial revision
 *
 *
 *
 */
       /****************************************************
        *                     rb_mss.c                     *
        *                                                  *
        *   This file contains the RebootMSS100 function   *
        ****************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>   /* for sleep_ew() */

int  ConnectToMSS( char [] );
int  SendToMSS( char *, int );
int  GetFromMSS( char *, int, int * );
void CloseSocketConnection( void );
int  GetPrompt( char [], int );

extern int Quiet;
extern int Logout;


int RebootMSS100( char ServerIP[], char aPassword[], char pPassword[] )
{
   char  command[80];
   const timeout = 10;     /* seconds */

/* Connect to the MSS100
   *********************/
   if ( ConnectToMSS( ServerIP ) == -1 )
   {
      if ( !Quiet ) printf( "Can't connect to MSS100.\n" );
      return -1;
   }

/* Get access to the remote console port
   *************************************/
   if ( GetPrompt( "# ", timeout ) < 0 )
   {
      if ( !Quiet ) printf( "Error getting # prompt from MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

   strcpy( command, aPassword );
   strcat( command, "\n" );
   if ( SendToMSS( command, strlen(command) ) == -1 )
   {
      if ( !Quiet ) printf( "Error sending user name to MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }


/* Send user name to the MSS100
   ****************************/
   if ( GetPrompt( "Enter username> ", timeout ) < 0 )
   {
      if ( !Quiet ) printf( "Error getting Username prompt from MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

   if ( SendToMSS( "x\n", 2 ) == -1 )
   {
      if ( !Quiet ) printf( "Error sending user name to MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

/* Send command to log in as a priviledged user
   ********************************************/
   if ( GetPrompt( "Local> ", timeout ) < 0 )
   {
      if ( !Quiet ) printf( "Error getting first Local> prompt from MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

   if ( SendToMSS( "set priv\n", 9 ) == -1 )
   {
      if ( !Quiet ) printf( "Error sending <set priv> to MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

/* Send the password of the priviledged user
   *****************************************/
   if ( GetPrompt( "Password> ", timeout ) < 0 )
   {
      if ( !Quiet ) printf( "Error getting Password> prompt from MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

   strcpy( command, pPassword );
   strcat( command, "\n" );
   if ( SendToMSS( command, strlen(command) ) == -1 )
   {
      if ( !Quiet ) printf( "Error sending password to MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

/* Send the reboot command
   ***********************/
   if ( GetPrompt( "Local> ", timeout ) < 0 )
   {
      if ( !Quiet ) printf( "Error getting second Local> prompt from MSS100.\n" );
      CloseSocketConnection();
      return -1;
   }

   if ( Logout == 0 )            /* Reboot the MSS100 */
   {
      if ( SendToMSS( "i d 0\n", 6 ) == -1 )
      {
         if ( !Quiet ) printf( "Error sending reboot command to MSS100.\n" );
         CloseSocketConnection();
         return -1;
      }
      CloseSocketConnection();   /* Reboot successful */
      return 0;
   }

/* Log out the serial port and log out of the MSS100
   *************************************************/
   if ( Logout == 1 )            /* Log out the MSS100 serial port */
   {
      if ( SendToMSS( "logout port 1\n", 14 ) == -1 )
      {
         if ( !Quiet ) printf( "Error sending logout command to MSS100.\n" );
         CloseSocketConnection();
         return -1;
      }
      if ( GetPrompt( "Local> ", timeout ) < 0 )
      {
         if ( !Quiet ) printf( "Error getting third Local> prompt from MSS100.\n" );
         CloseSocketConnection();
         return -1;
      }
      if ( SendToMSS( "logout\n", 7 ) == -1 )
      {
         if ( !Quiet ) printf( "Error sending logout command to MSS100.\n" );
         CloseSocketConnection();
         return -1;
      }
      CloseSocketConnection();   /* Log out successful */
      return 0;
   }

   if ( !Quiet ) printf( "Unknown value of Logout: %d\n", Logout );
   CloseSocketConnection();
   return -1;
}


        /*******************************************************
         *                    GetPrompt()                      *
         *                                                     *
         *  Wait for timeout seconds for the MSS 100 to send   *
         *  the desired prompt.                                *
         *                                                     *
         *  timeout = maximum time to wait, in seconds         *
         *  Returns  0 if we received the prompt               *
         *          -1 if there was a socket read error        *
         *          -2 if prompt wasn't received               *
         *          -3 if buffer is full                       *
         *******************************************************/

int GetPrompt( char prompt[], int timeout )
{
   static char buf[256];
   int         nReceived;
   int         totalReceived = 0;
   time_t      start         = time(0);
   int         spaceAvail    = sizeof(buf);
   int         lenPrompt     = strlen(prompt);

   while ( (time(0) - start) < timeout )
   {
      int j;

      sleep_ew( 100 );                 /* Wait for bytes to show up */

      if ( spaceAvail < 1 )            /* Input buffer is full */
         return -3;

      if ( GetFromMSS( &buf[totalReceived],
                       spaceAvail,
                       &nReceived ) == -1 )
         return -1;                    /* Socket read error */

      for ( j = 0; j < nReceived; j++ )     /* Replace null bytes with carets */
      {
         if ( buf[totalReceived +j] == 0 )
            buf[totalReceived + j] = '^';
/*       putchar( buf[totalReceived + j] ); */    /* Debug code */
      }

      totalReceived += nReceived;
      spaceAvail    -= nReceived;
      buf[totalReceived] = '\0';

      if ( strstr( buf, prompt ) != NULL )
         return 0;                     /* Success! We found the prompt. */
   }
   return -2;                          /* Timed out */
}
