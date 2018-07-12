       /**********************************************************
        *                  StartstopPipeConsole.c                *
        *                                                        *
        *     Program to send command to 'startstop_service'     *
        *     to created piped command console, and then         *
        *     interact with it.                                  *
        **********************************************************/

#include <stdio.h>
#include <windows.h>
#include <startstop_winlib.h>
#include <pipeconsole.h>

#define DEF_PIPE_NAME "EarthwormStartstopPipe"
#define DEF_CMD_STR "NewPipeConsole"

char *pipeTagStr = "";                      //tag string for in/out pipes
char pipePathStr[1024] = "\\\\.\\pipe\\";   //path name for "control" pipe

#pragma warning(disable : 4100)   /* suppress unreferenced formal parameter warning */

//Runs input loop for pipe console.
void doPipeInput(void *dummy)
{
  if(!doPipeInLoop(pipeTagStr,pipePathStr))
    printf("Error running PipeInLoop\n");
}

//Program entry point.  Usage:
//  StartstopPipeConsole [pipeName] [command]
int main(int argc, char *argv[])
{
  char *argStr;
  char *pNameStr;
  HANDLE hNamedPipe;
  char *pStr;
  char cmdStr[1024];
  DWORD numBytes;
  unsigned tid;
  static char szNamedPipeBuffer[512];

  if(argc > 1)
  {  //at least on command-line argument
    argStr = argv[1];
    if(strlen(argStr) >= 2 && (argStr[0] == '-' || argStr[0] == '/') &&
                 (argStr[1] == 'h' || argStr[1] == 'H' || argStr[1] == '?'))
    {  //argument is help request; show info and exit
      printf("StartstopPipeConsole:  Sends command to 'startstop_service'\n"
             "  to create pipe command console, and use it.  Usage:\n"
             "    StartstopPipeConsole [pipeName] [command]\n"
             "  Defaults:  pipeName=%s, command=%s\n",
                                                DEF_PIPE_NAME, DEF_CMD_STR);
      return 0;
    }
    pNameStr = argStr;       //use given pipe name
              //if second argument then use it as command string:
    pStr = (argc > 2) ? argv[2] : DEF_CMD_STR;
  }
  else
  {  //no command-line arguments; use defaults
    pNameStr = DEF_PIPE_NAME;
    pStr = DEF_CMD_STR;
  }
  if(strcmp(pStr,DEF_CMD_STR) == 0)
  {  //command argument was "NewPipeConsole"
         //get time in ms and append as pipe-tag string:
    sprintf(cmdStr,"%s%ld",pStr,(long)time(NULL));
         //set pointer to tag string for in/out pipes:
    pipeTagStr = &cmdStr[strlen(DEF_CMD_STR)];
  }

  strcat(pipePathStr,pNameStr);   //append pipe name to path string for OS
              //open connection to pipe:
  hNamedPipe = CreateFile(pipePathStr, GENERIC_WRITE, FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, SECURITY_IDENTIFICATION, NULL);
  if(hNamedPipe == INVALID_HANDLE_VALUE)
  {  //connection failed; show error message and abort
    printf("Unable to connect to pipe ('%s'), error code = %d\n", pNameStr,
                                                            GetLastError());
    Sleep(3000);             //delay in case console will be disappearing
    return 1;
  }
              //write command string out to pipe:
  if(!WriteFile(hNamedPipe, cmdStr, (DWORD)strlen(cmdStr), &numBytes, NULL))
  {  //error flag returned; show error message
    printf("Error writing to pipe ('%s'), error code = %d\n", pNameStr,
                                                            GetLastError());
  }
  else if(numBytes < strlen(cmdStr))
  {  //sent-byte count mismatch; show error message
    printf("Not all data written to pipe ('%s'), error code = %d\n",
                                                  pNameStr, GetLastError());
  }
  else  //success; show message
    printf("Sent '%s' command to pipe ('%s')\n", pStr, pNameStr);

  Sleep(100);      //delay to let msg transfer before closing handle
  CloseHandle(hNamedPipe);        //close handle to pipe
  
  if(strcmp(pStr,DEF_CMD_STR) == 0)
  {  //command argument was "NewPipeConsole"
     //run loops for transferring I/O between this console and pipe console:
    printf("Starting pipe console\n");
    StartThread(doPipeInput, 0, &tid);   //run input loop in separate thread
    if(!doPipeOutLoop(pipeTagStr))       //run output loop in this thread
    {
      printf("Error running PipeOutLoop\n");
      Sleep(2500);           //delay in case console will be disappearing
    }
    printf("Exiting pipe console\n");
    Sleep(500);
  }

  exit(0);
  return 0;
}
