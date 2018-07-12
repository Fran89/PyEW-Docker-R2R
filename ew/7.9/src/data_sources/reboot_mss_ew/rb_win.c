
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rb_win.c 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.3  2004/06/25 18:27:27  dietz
 *     modified to work with TYPE_TRACEBUF2 and location code
 *
 *     Revision 1.2  2001/04/27 00:55:54  kohler
 *     Implemented option for MSS serial port logout.
 *
 *     Revision 1.1  2001/04/26 17:49:07  kohler
 *     Initial revision
 *
 *
 *
 */
       /****************************************************
        *                     rb_win.c                     *
        *                                                  *
        *  This file contains the SpawnRebootMSS and       *
        *  TestChild functions                             *
        ****************************************************/


#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <earthworm.h>
#include "reboot_mss_ew.h"


      /*****************************************************
       *                   SpawnRebootMSS()                *
       *            Spawn the reboot_mss command.          *
       *                                                   *
       *  Returns  0 if successful                         *
       *          -1 if the process couldn't be spawned    *
       *****************************************************/

int SpawnRebootMSS( char ProgName[], SCNL *scnlp, int Logout )
{
   char cmdStr[100];
   int  success;

   STARTUPINFO         startUpInfo;
   PROCESS_INFORMATION procInfo;

   strcpy( cmdStr, ProgName );
   strcat( cmdStr, " " );
   strcat( cmdStr, scnlp->mss_ip );
   strcat( cmdStr, " " );
   strcat( cmdStr, scnlp->mss_apwd );
   strcat( cmdStr, " " );
   strcat( cmdStr, scnlp->mss_ppwd );
   strcat( cmdStr, " -q" );              /* Quiet mode */

   if ( Logout == 1 )                    /* Log out MSS100 serial port */
      strcat( cmdStr, " -l" );

/* Get STARTUPINFO structure for current process
   *********************************************/
   GetStartupInfo( &startUpInfo );

/* Create the child process
   ************************/
   success = CreateProcess( 0, cmdStr, 0, 0, FALSE,
                            DETACHED_PROCESS, 0, 0,
                            &startUpInfo, &procInfo );
   if ( !success )
   {
      logit( "t", "Error spawning reboot process: %d\n",
                   GetLastError() );
      return -1;
   }
   scnlp->hProcess = procInfo.hProcess;   /* Save process handle */
   return 0;                             /* Reboot succeeded */
}


      /****************************************************
       *                    TestChild()                   *
       *       See if a child process has completed       *
       *                                                  *
       *  Returns  2 if child process completed.          *
       *             In this case, exitCode is set.       *
       *           0 if child process hasn't completed    *
       ****************************************************/

int TestChild( SCNL *scnlp, int *exCode )
{
   int exitCode;

   GetExitCodeProcess( scnlp->hProcess, &exitCode );
   if ( exitCode == STILL_ACTIVE ) return 0;
   *exCode = exitCode;
   return 2;
}
