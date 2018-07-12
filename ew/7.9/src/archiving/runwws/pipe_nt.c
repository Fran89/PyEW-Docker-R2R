
/***************************************************************
 *                          pipe_nt.c                          *
 *                                                             *
 *      Routines for writing to a pipe under Windows NT.       *
 ***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <earthworm.h>

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;

PROCESS_INFORMATION piProcInfo;


/*****************************************************************/
/* Pipe_Init() starts a child process and opens a pipe to it.    */
/*             The pipe replaces stdin of the new process.       */
/*    Returns:   0 on success                                    */
/*              -1 on failure                                    */
/*****************************************************************/
int Pipe_Init( char  *child )    // Command to start new process
{
   SECURITY_ATTRIBUTES saAttr;
   TCHAR *szCmdline = child;
   STARTUPINFO siStartInfo;

   BOOL bSuccess = FALSE;

// Set the bInheritHandle flag so pipe handles are inherited
// ---------------------------------------------------------
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

// Create a pipe for the child process's STDIN
// -------------------------------------------
   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
   {
      logit( "et", "pipe_init() error creating stdin pipe for child process.\n" );
      return -1;
   }

// Ensure the write handle to the pipe for STDIN is not inherited
// --------------------------------------------------------------
   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
   {
      logit( "et", "pipe_init() error setting stdin handle information.\n" );
      return -1;
   }

// Set up members of the PROCESS_INFORMATION structure
// ---------------------------------------------------
   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

// Set up members of the STARTUPINFO structure.
// Stdin for the child comes from a pipe from the parent.
// Stdout and stderr for the child goes to the parent's stdout/stderr.
// ------------------------------------------------------------------
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
   siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
   siStartInfo.hStdInput  = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

// Create the child process
// ------------------------
   bSuccess = CreateProcess(NULL,
      szCmdline,     // command line
      NULL,          // process security attributes
      NULL,          // primary thread security attributes
      TRUE,          // handles are inherited
      0,             // creation flags
      NULL,          // use parent's environment
      NULL,          // use parent's current directory
      &siStartInfo,  // STARTUPINFO pointer
      &piProcInfo);  // receives PROCESS_INFORMATION

// An error occurred creating the child process
// --------------------------------------------
   if ( ! bSuccess )
   {
      logit( "et", "pipe_init() error creating child process.\n" );
      return -1;
   }
   return 0;
}


/*****************************************************************/
/* Child_Dead()                                                  */
/* Returns 0 if the child process is alive.                      */
/*         1 if the child process is dead.                       */
/*****************************************************************/
int ChildDead( void )
{
   int exitCode;

// We obtained the handle of the child when we created the process
// ---------------------------------------------------------------
   if ( !GetExitCodeProcess( piProcInfo.hProcess, &exitCode ) )
   {
      logit( "et", "ChildDead() error getting exit code of child process.\n" );
      return 1;
   }
   return (exitCode == STILL_ACTIVE) ? 0 : 1;
}


/******************************************************************
 * KillChildProcess()                                             *
 * If the child process did not die cleanly, bonk it on the head. *
 * Returns 1 if successful                                        *
 *         0 if not successful                                    *
 ******************************************************************/
int KillChildProcess( void )
{
   return (TerminateProcess(piProcInfo.hProcess, 0) == 0) ? 0 : 1;
}


/******************************************************************
 * WaitForChildToDie()                                            *
 * Wait for the child process to die after sending it a quit      *
 * command.                                                       *
 ******************************************************************/
void WaitForChildToDie( int msecToWait )
{
   WaitForSingleObject( piProcInfo.hProcess, msecToWait );
   return;
}


/*****************************************************************/
/* Pipe_Put() writes a msg to a pipe, terminating with null byte */
/* Returns 0 if there were no errors,                            */
/*         some number if there were errors                      */
/*****************************************************************/
int Pipe_Put( char *chBuf )   // Null-terminated char string
{
   DWORD dwWrite = strlen( chBuf );
   DWORD dwWritten;
   BOOL  bSuccess = FALSE;

// Write message string to pipe
// ----------------------------
   bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwWrite, &dwWritten, NULL);
   if ( ! bSuccess )
   {
      logit( "et", "Pipe write error.\n" );
      return( -1 );
   }
   return( 0 );
}


/*******************************************************************/
/* Pipe_Close()  Close the pipe handle so the child stops reading. */
/* Returns 0 if there were no errors,                              */
/*         some number if there were errors                        */
/*******************************************************************/
int Pipe_Close( void )
{
   if ( ! CloseHandle(g_hChildStd_IN_Wr) )
   {
      logit( "et", "StdInWr CloseHandle error.\n" );
      return( -1 );
   }
   return 0;
}
