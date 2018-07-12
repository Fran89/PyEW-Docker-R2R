
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: remote_copy.c 10 2000-02-14 18:56:41Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

/***************************************************************************
 *                                remote_copy.c                            *
 *                              Windows NT version                         *
 *                                                                         *
 *     Copies a file from local machine to a remote machine using rcp.     *
 *                                                                         *
 *  NOTE: This has been modified from the library version of copyfile !!!  *
 *        The destination path is constructed externally and the file is   *
 *        copied with  a name change following the copy.  The file names   *
 *        are all given with directory paths to provide greater            *
 *        versatility. This routine is used primarily to copy GIF and html *
 *        files to webservers.                                             *
 *************************************************************************** 
 *     For this function to work, make sure that the following files are   *
 *     set up properly on the remote machine:                              *
 *       /etc/hosts         must contain address and localhostname         *
 *       /etc/hosts.equiv   must contain local_hostname                    *
 *       .rhosts            in <userid>'s home directory must contain a    *
 *                          line: local_hostname local_username            *
 *                          describing who is running this program.        *
 *                                                                         *
 *                                                                         *
 * fullname   full name of file to copy. /dir/filename                     *
 * tname      temporary name of file to copy. /dir/filename                *
 * fname      final name of file to copy. /dir/filename                    *
 * host       remote machine to copy file to                               *
 * userid     use this user name on remote machine                         *
 * passwd     userid's password on remote machine (not needed by NT)       *
 *                                                                         *
 * errtxt     string to return error message in                            *
 * mypid      external process id to preserve re-entrancy                  *
 * mystat     external status variable to preserve re-entrancy             *
 *                                                                         *
 *     Also make sure that entries for the remote host are in the          *
 *     local machine's \winnt\system32\drivers\etc\hosts file.             *
 *                                                                         *
 *     Returns 0 if all is ok                                              *
 *             1 if error creating the child process                       *
 *             2 if error waiting for the child process to complete        *
 *             3 if the child process ended abnormally                     *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <earthworm.h>


int remote_copy( char  *fullname, /* Name of /dir/file to copy               */
                 char  *tname,    /* Temporary remote file name              */
                 char  *fname,    /* Final remote file name                  */
                 char  *host,     /* Remote machine to copy file to          */
                 char  *dir,      /* Directory on remote machine             */
                 char  *userid,   /* Use this user name on remote machine    */
                 char  *passwd,   /* Userid's password on remote machine     */
                 char  *errtxt,   /* String to return error message in       */
                 pid_t *mypid,    /* ext. process id to preserve re-entrancy */
                 int   *mystat )  /* ext. status variable for re-entrancy    */


{
   char        rcpPath[175];         /* Copy the file to this path           */
   char        tmpName[100];         /* Temporary file name                  */
   char        finalName[100];       /* Final name for the copied file       */
   char        commandLine[100];     /* Command that invokes the child proc  */
   DWORD       display;
   BOOL        success;
   DWORD       priorityClass;
   DWORD       rc;
   DWORD       exitCode;

   STARTUPINFO         startUpInfo;
   PROCESS_INFORMATION procInfo;

/* Build temporary and final path names on the target system
   *********************************************************/
   sprintf( rcpPath,  "%s.%s:%s%s",  host, userid, dir, tname );
   sprintf( tmpName,  "%s%s",        dir, tname );
   sprintf( finalName, "%s%s",       dir, fname );

/* Retrieve the STARTUPINFOR structure for the current process.
   Use this structure for the child processes.
   ***********************************************************/
   GetStartupInfo( &startUpInfo );

/* Copy the file using rcp
   ***********************/
   display = 0;                                /* Do not create a new console window */

   priorityClass = GetPriorityClass( GetCurrentProcess() );

   sprintf( commandLine, "rcp %s %s", fullname, rcpPath );

   success = CreateProcess( 0,
                            commandLine,       /* Command line to invoke child   */
                            0, 0,              /* No security attributes         */
                            FALSE,             /* No inherited handles           */
                            display |          /* Child does not get a window    */
                            priorityClass,     /* Same as priority of parent     */
                            0,                 /* Not passing environmental vars */
                            0,                 /* Current dir same as parent     */
                            &startUpInfo,      /* Attributes of process window   */
                            &procInfo );       /* Attributes of child process    */
   if ( !success ) {
      sprintf( errtxt, "Error starting the rcp command: %d", GetLastError() );
      return 1;
   }

/* Wait 2 minutes for the remote copy to complete.
   Check the process exit code for abnormal termination.
   ****************************************************/
   rc = WaitForSingleObject( procInfo.hProcess, 120000 );

   if ( rc == WAIT_FAILED ) {
      sprintf( errtxt, "rcp WaitForSingleObject() failed with error: %d", GetLastError() );
      return 2;
   }
   if ( rc == WAIT_TIMEOUT ) {
      sprintf( errtxt, "Error. Remote copy not completed within 2 minutes." );
      return 2;
   }
   success = GetExitCodeProcess( procInfo.hProcess, &exitCode );
   if ( !success ) {
      sprintf( errtxt, "Error getting the rcp exit code: %d", GetLastError() );
      return 3;
   }
   if ( exitCode != 0 ) {
      strcpy( errtxt, "Remote copy failed." );
      return 3;
   }

/* Close the process and thread handles for rcp
   ********************************************/
   success = CloseHandle( procInfo.hProcess );
   if ( !success ) {
      sprintf( errtxt, "Error closing the rcp process handle: %d", GetLastError() );
      return 3;
   }
   success = CloseHandle( procInfo.hThread );
   if ( !success ) {
      sprintf( errtxt, "Error closing the rcp thread handle: %d", GetLastError() );
      return 3;
   }

/* Rename the remote file using rsh
   ********************************/
   sprintf( commandLine, "rsh %s -l %s /usr/bin/mv %s %s",
                          host, userid, tmpName, finalName );

   success = CreateProcess( 0,
                            commandLine,       /* Command line to invoke child   */
                            0, 0,              /* No security attributes         */
                            FALSE,             /* No inherited handles           */
                            display |          /* Child does not get a window    */
                            priorityClass,     /* Same as priority of parent     */
                            0,                 /* Not passing environmental vars */
                            0,                 /* Current dir same as parent     */
                            &startUpInfo,      /* Attributes of process window   */
                            &procInfo );       /* Attributes of child process    */
   if ( !success ) {
      sprintf( errtxt, "Error starting the rsh command: %d", GetLastError() );
      return 1;
   }

/* Wait 2 minutes for the remote move command to complete.
   Check the process exit code for abnormal termination.
   ******************************************************/
   rc = WaitForSingleObject( procInfo.hProcess, 120000 );

   if ( rc == WAIT_FAILED ) {
      sprintf( errtxt, "rsh WaitForSingleObject() failed with error: %d", GetLastError() );
      return 2;
   }
   if ( rc == WAIT_TIMEOUT ) {
      sprintf( errtxt, "Error. Remote mv command not completed within 2 minutes." );
      return 2;
   }
   success = GetExitCodeProcess( procInfo.hProcess, &exitCode );
   if ( !success ) {
      sprintf( errtxt, "Error getting the rsh exit code: %d", GetLastError() );
      return 3;
   }
   if ( exitCode != 0 ) {
      strcpy( errtxt, "Remote mv command failed." );
      return 3;
   }

/* Close the process and thread handles for rsh
   ********************************************/
   success = CloseHandle( procInfo.hProcess );
   if ( !success ) {
      sprintf( errtxt, "Error closing the rsh process handle: %d", GetLastError() );
      return 3;
   }
   success = CloseHandle( procInfo.hThread );
   if ( !success ) {
      sprintf( errtxt, "Error closing the rsh thread handle: %d", GetLastError() );
      return 3;
   }

/* Everything went smoothly
   ************************/
   return 0;
}
