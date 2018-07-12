/***************************************************************
 *                     pipeconsole.c                           *
 *                                                             *
 *      Supports creation and use of a console window with     *
 *      stdin/stdout redirected to named pipes.                *
 ***************************************************************/
 
 // 10/29/2014 -- [ET]  Module extended from code found at
 //                     http://www.ivorykite.com/winpipe.html

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <tlhelp32.h>
#include <pipeconsole.h>

#define PIPE_IN_NAME "\\\\.\\pipe\\ewConInPipe"       //pipe for con stdin
#define PIPE_OUT_NAME "\\\\.\\pipe\\ewConOutPipe"     //pipe for con stdout
#define PIPE_BUF_SIZE 4096             //size for pipe in/out buffers
#define PIPE_TIMEOUT 120000            //default-timeout value for pipes
#define KILL_CONSOLE_CMD "KillPipeConsole"       //kill cmd for control pipe

BOOL TerminateProcessAndChildren(DWORD pid, BOOL alwaysTermParentFlag);

char pipeInName[256], pipeOutName[256];
HANDLE pipeInHandle=NULL, pipeOutHandle=NULL;
SECURITY_ATTRIBUTES sa;
STARTUPINFO si;
PROCESS_INFORMATION pi;

char lastCmdBuff[1024];
char *consoleCtrlPipePathname = NULL;
HANDLE consoleCtrlPipeHandle = NULL;

// -------- First set of functions are for startstop / server side --------

//Creates in/out pipes and process for 'cmd.exe' console (with 'stdin' and
// 'stdout' redirected to pipes).
// pTagStr:  tag string for in/out pipes.
// Returns NULL if successful; returns an error message if failure.
const PTSTR startPipeConsole(char *pTagStr)
{
  DWORD ew = 0;
  PTSTR cmdStr = _tcsdup(_T("cmd.exe"));    //command string to be executed
  
         //create pipe for console 'stdin':
  strcpy(pipeInName,PIPE_IN_NAME);          //start with base pipe name
  strcat(pipeInName,pTagStr);               //append tag string
  pipeInHandle = CreateNamedPipe(pipeInName,
                           PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE|PIPE_WAIT, 1,
                             PIPE_BUF_SIZE, PIPE_BUF_SIZE, PIPE_TIMEOUT, 0);
  if(pipeInHandle == INVALID_HANDLE_VALUE)
    return "Unable to create pipe for console 'stdin'";
  SetHandleInformation(pipeInHandle, HANDLE_FLAG_INHERIT,
                                                       HANDLE_FLAG_INHERIT);
         //create pipe for console 'stdout' and 'stderr':
  strcpy(pipeOutName,PIPE_OUT_NAME);        //start with base pipe name
  strcat(pipeOutName,pTagStr);              //append tag string
  pipeOutHandle = CreateNamedPipe(pipeOutName,
                         PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE|PIPE_WAIT, 1, 
                            PIPE_BUF_SIZE, PIPE_BUF_SIZE, PIPE_TIMEOUT, 0);
  if(pipeOutHandle == INVALID_HANDLE_VALUE)
    return "Unable to create pipe for console 'stdout'";
  SetHandleInformation(pipeOutHandle, HANDLE_FLAG_INHERIT,
                                                       HANDLE_FLAG_INHERIT);
//  printf("DEBUG:  Waiting for connections to pipes\n");
         //wait for client-side connections to pipes:
  ConnectNamedPipe(pipeInHandle,NULL);
  ConnectNamedPipe(pipeOutHandle,NULL);
//  printf("DEBUG:  Connections to pipes established\n");

         //setup SECURITY_ATTRIBUTES fields:
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;
         //setup STARTUPINFO fields:
  si.cb = sizeof(si);
  si.lpReserved = NULL;
  si.lpDesktop = NULL;
  si.lpTitle = NULL;
  si.dwFlags = STARTF_USESTDHANDLES;
  si.cbReserved2 = 0;
  si.lpReserved2 = NULL;
  si.hStdInput = pipeInHandle;
  si.hStdOutput = pipeOutHandle;
  si.hStdError = pipeOutHandle;
  
         //create new process to run 'cmd.exe' console:
  if(!CreateProcess(NULL, cmdStr, &sa, NULL, TRUE,
              CREATE_NO_WINDOW|NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
  {  //error value returned
    return "Unable to create process for 'cmd.exe' console";
  }
  Sleep(100);
         //check if new process is still running:
  if(GetExitCodeProcess(pi.hProcess, &ew) && ew != STILL_ACTIVE)
    return "Process for 'cmd.exe' console terminated early";
//  printf("DEBUG:  pi.pid=%d, ew=%d, STILL_ACTIVE=%d\n", (int)(pi.dwProcessId),
//                                                (int)ew, (int)STILL_ACTIVE);
                                                
  return NULL;
}

//Terminates the pipe-console and/or its child process(es).
// termAllFlag:  TRUE to terminate console and child process(es); FALSE to
//  terminate child process(es) and only terminate console if there were
//  no child process(es) running.
// Returns TRUE if successful; FALSE if not.
BOOL terminatePipeConsole(BOOL termAllFlag)
{
  if(pipeInHandle != NULL)   //if 'startPipeConsole()' was called then term
    return TerminateProcessAndChildren(pi.dwProcessId,termAllFlag);
  return FALSE;
}

//Returns TRUE if pipe-console is running.
BOOL isPipeConsoleRunning()
{
  DWORD ew;             //return TRUE if 'startPipeConsole()' was called
                        // and console process is still running:
  return (pipeInHandle != NULL && GetExitCodeProcess(pi.hProcess, &ew) &&
                                                        ew == STILL_ACTIVE);
}

//Blocks until pipe-console is not running, up to the given timeout time.
// timeoutMs:  time-out interval, in millisecond, or INFINITE for no timeout.
void waitForPipeConsoleStopped(DWORD timeoutMs)
{
  if(isPipeConsoleRunning())
    WaitForSingleObject(pi.hProcess,timeoutMs);
}

//Closes handles to pipe-console pipes.
void closePipeConsoleHandles()
{
  CloseHandle(pipeInHandle);
  CloseHandle(pipeOutHandle);
}

// -------- Functions below here are for user / client side --------

//Creates and returns a file handle for the console 'stdin' pipe.  Data
// for the console is written into this handle.  The pipe needs to have
// been created before this function is called.
// pTagStr:  tag string for pipe.
// Returns the handle if successful; returns INVALID_HANDLE_VALUE if failure.
HANDLE getPipeInFileHandle(char *pTagStr)
{
  char pipeName[160];

  strcpy(pipeName,PIPE_IN_NAME);       //start with base pipe name
  strcat(pipeName,pTagStr);            //append tag string
  return CreateFile(pipeName, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
}

//Creates and returns a file handle for the console 'stdout'/'stderr' pipe.
// Data from the console is read from this handle.  The pipe needs to have
// been created before this function is called.
// pTagStr:  tag string for pipe.
// Returns the handle if successful; returns INVALID_HANDLE_VALUE if failure.
HANDLE getPipeOutFileHandle(char *pTagStr)
{
  char pipeName[160];

  strcpy(pipeName,PIPE_OUT_NAME);      //start with base pipe name
  strcat(pipeName,pTagStr);            //append tag string
  return CreateFile(pipeName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, NULL);
}

//Control handler function; intercepts Ctrl-C from user and directs it
// to the pipe-console input pipe.
// fdwCtrlType:  specifies type of control event.
BOOL pipeCtrlHandler(DWORD fdwCtrlType)
{
  DWORD numBytes;
  HANDLE localHandle;

  switch(fdwCtrlType)
  {
    case CTRL_C_EVENT:
      if(consoleCtrlPipePathname != NULL)
      {  //pipe handle was setup; send "KillPipeConsole" to control pipe
        if(consoleCtrlPipeHandle != NULL)        //if pipe handle open then
          CloseHandle(consoleCtrlPipeHandle);    //close handle to pipe
                   //connect to "control" pipe for pipe-console:
        consoleCtrlPipeHandle = CreateFile(consoleCtrlPipePathname,
                       GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                             SECURITY_IDENTIFICATION, NULL);
        if(consoleCtrlPipeHandle != INVALID_HANDLE_VALUE)
        {  //connected OK; write command string out to "control" pipe
          if(WriteFile(consoleCtrlPipeHandle, KILL_CONSOLE_CMD,
                          (DWORD)strlen(KILL_CONSOLE_CMD), &numBytes, NULL))
          {  //write was successful
            printf("^C\n");
            Sleep(10);     //delay to let msg transfer before closing handle
                   //set local copy of handle in case other
                   // thread clears 'consoleCtrlPipeHandle':
            localHandle = consoleCtrlPipeHandle;
            if(localHandle != NULL)         //if not already closed then
              CloseHandle(localHandle);     //close handle to pipe
            return TRUE;     //indicate Ctrl-C was handled here
          }
        }
      }
      printf("\nUnable to send control command to pipe console\n");
      return FALSE;     //indicate Ctrl-C not handled here

    case CTRL_CLOSE_EVENT:
      return TRUE;      //indicate program may be closed

    default:
      return FALSE;     //indicate signal not handled here
  } 
} 

//Installs control handler function.  (Handler intercepts Ctrl-C from
// user and directs it to the pipe-console "control" pipe.)
// ctrlPipePathname:  path name for pipe-console "control" pipe.
BOOL installPipeCtrlHandler(char *ctrlPipePathname)
{
  consoleCtrlPipePathname = ctrlPipePathname;    //save pipe name
                                                 //install handler:
  return SetConsoleCtrlHandler((PHANDLER_ROUTINE)pipeCtrlHandler, TRUE);
}

//Runs loop that receives keyboard input and sends it to the 'stdin' pipe.
// (A control handler is installed to redirect the Ctrl-C input to the
// pipe-console.)
// pTagStr:  tag string for pipe.
// ctrlPipePathname:  path name for pipe-console "control" pipe.
// If successful then blocks indefinitely; if failure then returns FALSE.
BOOL doPipeInLoop(char *pTagStr, char *ctrlPipePathname)
{
  HANDLE pipeHandle;
  static char sBuff[1000];
  DWORD q, r;
  
         //create/get handle for console 'stdin' pipe:
  if((pipeHandle=getPipeInFileHandle(pTagStr)) == INVALID_HANDLE_VALUE)
    return FALSE;
         //install Ctrl-C handler:
  if(!installPipeCtrlHandler(ctrlPipePathname))
    printf("Unable to install Ctrl-C handler\n");
  
  while(TRUE)
  {
    if(ReadFile(GetStdHandle(STD_INPUT_HANDLE), sBuff, 888, &r, 0))
    {  //read from 'stdin' succeeded
      sBuff[r] = '\0';                 //make sure has terminating null
      strncpy(lastCmdBuff,sBuff,r);    //save command for 'doPipeOutLoop()'
      lastCmdBuff[r] = '\0';           //append terminating null
      WriteFile(pipeHandle, sBuff, r, &q, 0);
    }
    Sleep(10);   // (call to ReadFile() should block, but delay just in case)
  }
}

//Runs loop that receives data from the pipe-console 'stdout'/'stderr'
// pipe and sends it to the local-console 'stdout' stream.
// pTagStr:  tag string for pipe.
// If successful then blocks until pipe-console is closed and then
// returns TRUE; if failure then returns FALSE.
BOOL doPipeOutLoop(char *pTagStr)
{
  HANDLE pipeHandle;
  OVERLAPPED ol = { 0 };
  char cBuff[4096];
  int pos,sLen;
  DWORD r,q;
  BOOL rFlag;
  
         //create/get handle for console 'stdout'/'stderr' pipe:
  if((pipeHandle=getPipeOutFileHandle(pTagStr)) == INVALID_HANDLE_VALUE)
    return FALSE;
    
  ol.hEvent = CreateEvent(0, 0, 0, 0);
  lastCmdBuff[0] = '\0';     //initialize last-command buffer
  while(TRUE)
  {  //loop until pipe-console 'stdout' pipe is closed
    if((rFlag=ReadFile(pipeHandle, cBuff, sizeof(cBuff)-2, &r, &ol)) != FALSE)
    {  //data received OK from pipe-console 'stdout' pipe
      cBuff[r] = '\0';       //make sure null terminated
              //if beginning of echoed data matches last command
              // then trim last-command characters from data:
      pos = 0;
      if(lastCmdBuff[0] != '\0')
      {  //last-command buffer not empty
        sLen = (int)strlen(lastCmdBuff);
        if(strncmp(cBuff,lastCmdBuff,sLen) == 0)
        {  //beginning of received data matches last command
          pos = sLen;        //skip matching characters
          r -= sLen;         //reduce used-buffer size
        }
        lastCmdBuff[0] = '\0';    //clear buffer
      }
      WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &cBuff[pos], r, &q, 0);
    }
    else      //ReadFile() returned FALSE; exit loop (and function)
      break;
    Sleep(10);   // (call to ReadFile() should block, but delay just in case)
  }
  if(consoleCtrlPipeHandle != NULL)
  {  //connection to "control" pipe is open
    CloseHandle(consoleCtrlPipeHandle);     //close handle to pipe
    consoleCtrlPipeHandle = NULL;           //indicate closed
  }
  return TRUE;
}

// -------- Functions below here are utility --------

//Recursively terminate the given process and all of it's child processes.
// alwaysTermParentFlag:  TRUE to always terminate given process; FALSE to
//  only terminate given process if it had no child processes.
// Returns TRUE if successful; FALSE if error.
BOOL TerminateProcessAndChildren(DWORD pid, BOOL alwaysTermParentFlag)
{
  HANDLE snap;
  PROCESSENTRY32 proc;
  HANDLE process;
  BOOL childFlag = FALSE;
  BOOL retFlag = FALSE;

  if(pid)     //terminate child process(es) of given process:
  {  //given PID is nonzero; get snapshot of processes
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(snap != INVALID_HANDLE_VALUE)
    {  //successfully fetched snapshot of processes
      proc.dwSize = sizeof(proc);
      if(Process32First(snap, &proc))
      {  //successfully fetch info on first process
        do
        {  //loop through list of processes
          if(proc.th32ParentProcessID == pid && (alwaysTermParentFlag ||
                               (strcmp("conhost.exe",proc.szExeFile) != 0 &&
                                  strcmp("csrss.exe",proc.szExeFile) != 0)))
          {  //process is child of given PID and, if not terminating parent
             // then don't terminate if console "helper" process
             // (if terminating parent then should terminate all children)
                        //terminate child process:
            retFlag = TerminateProcessAndChildren(proc.th32ProcessID, TRUE);
            childFlag = TRUE;          //indicate child process(es) found
          }
        }
        while(Process32Next(snap, &proc));
      }
      CloseHandle(snap);
    }
    if(alwaysTermParentFlag || !childFlag)
    {  //always terminating given process or no child process(es) found
              //terminate given (parent) process:
      process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
      if (process)
      {
        retFlag = TerminateProcess(process, 0);
        WaitForSingleObject(process, 30000);
        CloseHandle(process);
      }
    }
  }
  return retFlag;
}
