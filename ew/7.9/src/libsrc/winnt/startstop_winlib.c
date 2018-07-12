/********************************************************************
 *                 startstop_lib.c    for   Windows 32              *
 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <startstop_winlib.h>
#include <earthworm_complex_funcs.h>
#include <ew_nevent_message.h>
#include <pipeconsole.h>

void StopChildThread( void* );
void RestartChildThread( void* );
int StopChild( char*, METARING*, CHILD[], int*, int* );
void StartNewConsolePipeThread();
void StopNewConsolePipeThread();

HANDLE pipeConsoleHandle = NULL;  //handle for pipe-console "control" pipe
BOOL newConsolePipeThreadFlag = TRUE;  //set FALSE to terminate thread
char pipeConsoleTagStr[130];      //tag string for pipe-console in/out pipes


/********************************************************************
  * GetConfig()  Processes command file using kom.c functions        *
  *                     Exits if any errors are encountered          *
  *                     Commands are expected in a given order       *
  *                     Nesting is not allowed for startstop.cnf!    *
  ********************************************************************/
int GetConfig( METARING *metaring, CHILD    child[MAX_CHILD], int *nChild )
{
   int      state;        /* which part of the config file you are reading   */
   char     missed[30];
   char    *ptr;
   char    *com;
   char    *str;
   char     basePipeName[PIPE_NAME_MAX] = "\\\\.\\pipe\\\0";
   int      nfiles;
   int      ir = 0;
   int      i;
   int      ichild;
   boolean  firstTimeThrough = FALSE;
   boolean  duplicate = FALSE;

/* Definitions for state while reading file
   ****************************************/
   #define READ_NRING             0
   #define READ_RINGINFO          1
   #define READ_MODID             2
   #define READ_HEARTBEAT         3
   #define READ_MYPRIORITYCLASS   4
   #define READ_LOGSWITCH         5
   #define READ_KILLDELAY         6
   #define READ_OPTIONAL          7
   #define READ_PROCESS           8
   #define READ_PRIORITYCLASS     9
   #define READ_THREADPRIORITY   10
   #define READ_DISPLAY          11


/* Initialize some things
   **********************/
   state     = READ_NRING;
   missed[0] = '\0';

   if (*nChild == 0){
       firstTimeThrough = TRUE;
       oldNChild = 0;
   }
   metaring->maxStatusLineLen = 80;
   /* set default pipeName for launch-new-console commands. */
   strcpy( metaring->pipeName, "\\\\.\\pipe\\EarthwormStartstopPipe\0");
/* Open the main configuration file
   ********************************/
   nfiles = k_open( metaring->ConfigFile );
   if ( nfiles == 0 )
   {
        logit( "e" , "startstop: error opening command file <%s>; exiting!\n",
                 metaring->ConfigFile );
        return ERROR_FILE_NOT_FOUND;
   }


/* Process all command files
   *************************/
   while(nfiles > 0)   /* While there are command files open */
   {

        while(k_rd())        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
           *****************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Process anything else as a command;
           Expect commands in a certain order!
           ***********************************/
            switch (state)
            {
            /* Read the number of transport rings
            *************************************/
            case READ_NRING:
                if ( !k_its( "nRing" ) )
                {
                    strcpy( missed, "nRing" );
                    break;
                }
                if (firstTimeThrough) {/* Only if we're reading the config for the first time do we keep track */
                    metaring->nRing  = k_int();
                }
                state  = READ_RINGINFO;
                if (firstTimeThrough) {
                    ir = 0;
                } else {
                    ir = metaring->nRing;
                }
                break;

            /* Read the transport ring names & keys & size
             *********************************************/
            case READ_RINGINFO:
                if( !k_its( "Ring" ) )
                {
                    if ((k_its( "MyModuleId" )) && (!firstTimeThrough)) {
                        /* OK to skip MyModuleId on 2nd time through; we already know it */
                        state = READ_HEARTBEAT;
                        /* if this isn't the first time through we want to add the rings we just found to nRing */
                        newNRing = ir;
                    } else {
                       strcpy( missed, "Ring" );
                    }
                    break;
                }
                if( ir == MAX_RING )
                {
                    logit( "e" ,
                           "Too many transport rings, max=%d; exiting!\n",
                            MAX_RING );
                    return ERROR_OUTOFMEMORY;
                }
                str = k_str();
                if(!str)  break;

                for ( i = 0; i < ir; i++ ) {
                    if ( strcmp (str, metaring->ringName[i]) == 0 ){
                        duplicate = TRUE;
                    }
                }
                if (duplicate) {
                    if (firstTimeThrough) {
                        logit( "e", "Duplicate ring name <%s>; exiting!\n", str);
                        return ERROR_INVALID_NAME;
                    }
                    duplicate = FALSE;
                } else {
                    strcpy( metaring->ringName[ir], str );
                    metaring->ringSize[ir] = k_int();
                    if( (metaring->ringKey[ir]= GetKey(metaring->ringName[ir])) == -1 )
                    {
                       logit( "e" , "Invalid ring name <%s>; exiting!\n",
                               metaring->ringName[ir] );
                       return ERROR_INVALID_NAME;
                    }
                    if( ++ir == metaring->nRing )  state = READ_MODID;
                }
                break;

            /* Read stuff concerning startstop itself
               **************************************/
            case READ_MODID:
                if ( !k_its("MyModuleId") )
                {
                    strcpy( missed, "MyModuleId" );
                    break;
                }
                str = k_str();
                if(!str)  break;
                strcpy( metaring->MyModName, str );
                if( GetModId(metaring->MyModName, &(metaring->MyModId)) == -1 )
                {
                   logit( "e" , "Invalid MyModuleId <%s>; exiting!\n",
                           metaring->MyModName );
                   return ERROR_INVALID_MODULETYPE;
                }
                state = READ_HEARTBEAT;
                break;

            case READ_HEARTBEAT:
                if ( !k_its("HeartbeatInt") )
                {
                    strcpy( missed, "HeartbeatInt" );
                    break;
                }
                metaring->HeartbeatInt = k_int();
                state = READ_MYPRIORITYCLASS;
                break;

            case READ_MYPRIORITYCLASS:
                if ( !k_its("MyPriorityClass") )
                {
                    strcpy( missed, "MyPriorityClass" );
                    break;
                }
                str = k_str();
                if ( str ) strcpy( metaring->MyPriorityClass, str );
                state  = READ_LOGSWITCH;
                break;

            case READ_LOGSWITCH:
                if ( !k_its("LogFile") )
                {
                    strcpy( missed, "LogFile" );
                    break;
                }
                metaring->LogSwitch = k_int();
                state     = READ_KILLDELAY;
                break;

            case READ_KILLDELAY:
              if ( !k_its("KillDelay") )
              {
                strcpy( missed, "KillDelay" );
                break;
              }
              metaring->KillDelay = k_int();
              state     = READ_OPTIONAL;
              break;

         /* Optional command to tell startstop to wait a number
          * of seconds after starting statmgr (John Patton)
          * Two valid command names: statmgrDelay or StartupDelay */
            case READ_OPTIONAL:
              if ( k_its("statmgrDelay") || k_its("StartupDelay") )
              {
                metaring->statmgr_sleeptime = (k_int() * 1000);
              }
              else if ( k_its("maxStatusLineLen") )
              {
                int len = k_int();
                metaring->maxStatusLineLen = (80 > len ? 80 : len);
              }
              else if ( k_its("PipeName") )
              {
                  str = k_str();
                  if ( str ) strcat( basePipeName, str );
                  strcat (basePipeName, "\0");
                  strcpy (metaring->pipeName, basePipeName);
              }
              else
              {
                state     = READ_PROCESS;

                /* since current command isn't optional, jump to the next */
                /*  command in line instead of declaring an error */
                goto not_optional;
              }
              break;

           /* Read a command to start a child
              *******************************/
            case READ_PROCESS:
                not_optional:
                if ( !k_its("Process") )
                {
                    strcpy( missed, "Process" );
                    break;
                }
                if ( *nChild == MAX_CHILD )
                {
                    logit( "e" ,
                           "Too many child processes, max=%d; exiting!\n",
                            MAX_CHILD );
                    return ERROR_TOO_MANY_MODULES;
                }
                str = k_str();
                if ( str )
                {
                   duplicate = FALSE;
                   for ( ichild = 0; ichild < *nChild; ichild++ ) {
                       if ( strcmp (str, child[ichild].commandLine) == 0 ) {
                           logit ( "", "Skipping twin, no duplicate children allowed: %s\n", str);
                           duplicate = TRUE;
                       }
                   }
                   if (duplicate) {
                       state = READ_PRIORITYCLASS;
                       break;
                   }

                   strcpy( child[*nChild].commandLine, str );


                   ptr = strtok( str, " \t" );            /* Get program name */
                   if ( ptr == NULL )
                   {
                      logit( "e" , "Bad program name.\n");
                      break;
                   }
                   strcpy( child[*nChild].progName, ptr );
                }

                /* If this process is statmgr, store it's location in the child array
                 so that we can find it again later */
                /* Modified by Alex (WOW) to support searching for
                 statmgr2 for hydra
                 */
                if ((strcmp( child[*nChild].progName, "statmgr") == 0)||
                    (strcmp( child[*nChild].progName, "statmgr2") == 0))
                {
                    metaring->statmgr_location = *nChild;
                }

                state = READ_PRIORITYCLASS;
                break;

            /* Read the child's priority class
               *******************************/
            case READ_PRIORITYCLASS:
                if ( !k_its("PriorityClass") )
                {
                    strcpy( missed, "PriorityClass" );
                    break;
                }
                str = k_str();
                if (( str ) && (!duplicate))
                   strcpy( child[*nChild].priorityClass, str );
                state = READ_THREADPRIORITY;
                break;

            /* Read the child's thread priority
               ********************************/
            case READ_THREADPRIORITY:
                if ( !k_its("ThreadPriority") )
                {
                    strcpy( missed, "ThreadPriority" );
                    break;
                }
                str = k_str();
                if (( str ) && (!duplicate))
                   strcpy( child[*nChild].threadPriority, str );
                state = READ_DISPLAY;
                break;

            /* Read the child's startup state
               ******************************/
            case READ_DISPLAY:
                if ( !k_its("Display") )
                {
                    strcpy( missed, "Display" );
                    break;
                }
                str = k_str();
                if (( str ) && (!duplicate)) {
                   strcpy( child[*nChild].display, str );

                   CreateSpecificMutex( &(child[*nChild].mutex) );

                   (*nChild)++;
                }
                state = READ_PROCESS;
                break;



            }

        /* Complain if we got an unexpected command
           ****************************************/
            if( missed[0] )
            {
                logit( "e" , "Incorrect command order in <%s>; exiting!\n",
                         metaring->ConfigFile );
                logit( "e" , "Expected: <%s>  Found: <%s>\n",
                         missed, com );
                logit( "e" , "Offending line: %s\n", k_com() );
               return ERROR_BAD_CONFIGURATION;
            }

        /* See if there were any errors processing this command
           ****************************************************/
            if( k_err() )
            {
               logit( "e" , "Bad <%s> command in <%s>; exiting!\n",
                        com, metaring->ConfigFile );
               logit( "e" , "Offending line: %s\n", k_com() );
               return ERROR_BAD_COMMAND;
            }
        }
        nfiles = k_close();
   }
   return 0;
}   /* End GetConfig */



 /******************************************************************
 *                   AddArgsToStatusLine()                        *
 *                                                                *
 *  Add elements of args to line, using truncation as necessary   *
 *  to make sure line is no longer than metaring.maxStatusLineLen *
 *  chars long                                                    *
 ******************************************************************/
void AddArgsToStatusLine( char* args[], char* line, int hardcap )
{
  int j;
  int line_len = (int)strlen(line);
  for (j = 1; j < MAX_ARG && args[j] != NULL; j++) { 
    int arg_len = (int)strlen( args[j] );
    int notlast_arg = ( args[j+1] != NULL ) ? 1 : 0;
    int cap = hardcap - notlast_arg;
    int d_arg = (arg_len>2 && args[j][arg_len-1]=='d' && args[j][arg_len-2]=='.') ? 1 : 0;
    int main_len = arg_len;
    int over = (line_len + main_len + 1) - cap;

    while ( over > 0 )
    {
      if ( d_arg ) {
        /* Knock off trailing .d */
        main_len -= 2;
        over = (line_len + main_len + 1) - cap;
        d_arg = 0;
        continue;
      }
      else if (main_len-over-3 > 1-notlast_arg && main_len > 3)
      {
        /* Trim arg from front with < */
        line[line_len] = ' ';
        strncpy( line+line_len+1, args[j]+over, main_len-over );
        if ( over!=0 )
          line[++line_len] = '<';
        if ( notlast_arg ) {
          line[hardcap-1] = '>';
          line_len++;
        }
        line[hardcap] = 0;
      }
      else
      {
        /* Can't fit enough; just tack on a > */
        int shift = (line_len < hardcap-1) ? 1 : 0;
        line[line_len] = ' ';
        line[line_len+shift] = '>';
        while ( line_len+shift < hardcap-1 )
          line[++line_len+shift] = ' ';
        line[hardcap] = 0;
      }
      return;
    }
    line[line_len] = ' ';
    strncpy( line+line_len+1, args[j], main_len );
    line[line_len+1+main_len] = 0;
    line_len += (main_len+1);
  }
  while ( line_len < hardcap )
    line[line_len++] = ' ';
  line[line_len] = 0;
}

void parseCmdLine( char *args[], char *commandLine, char *parg_line )  
{
      char *pargs;
      char TERM  = '\0', QUOTE = '\"';
      int argc = 0;

      args[0] = NULL;
      sprintf( parg_line, " %-80s", commandLine );
      pargs = parg_line;
        while (*pargs)
        {
            char bInQuotes;
            while (isspace (*pargs))        /* skip leading whitespace */
                pargs++;

            bInQuotes = (*pargs == QUOTE);  /* see if this token is quoted */

            if (bInQuotes)                  /* skip leading quote */
                pargs++; 

            args[argc++] = pargs;           /* store position of current token */
            if ( argc < MAX_ARG )
                args[argc] = NULL;

            /* Find next token.
               NOTE: Args are normally terminated by whitespace, unless the
               arg is quoted.  That's why we handle the two cases separately,
               even though they are very similar. */
            if (bInQuotes)
            {
                /* find next quote followed by a space or terminator */
                while (*pargs && 
                      !(*pargs == QUOTE && (isspace (pargs[1]) || pargs[1] == TERM)))
                    pargs++;
                if (*pargs)
                {
                    *pargs = TERM;  /* terminate token */
                    if (pargs[1])   /* if quoted token not followed by a terminator */
                        pargs += 2; /* advance to next token */
                }
            }
            else
            {
                /* skip to next non-whitespace character */
                while (*pargs && !isspace (*pargs)) 
                    pargs++;
                if (*pargs && isspace (*pargs)) /* end of token */
                {
                   *pargs = TERM;    /* terminate token */
                    pargs++;         /* advance to next token or terminator */
                }
            }
        } /* while (*pargs) */   
        args[--argc] = NULL;
}

/***************************** EncodeStatus ***********************
 *                    Encode the status message                   *
 ******************************************************************/
#define MAX_STATUS_LINE_LEN 80 /* the maximum length of a status message line */

/* truncate a line longer than len, at len chars with a newline */
void _truncate_with_newline(char *buffer, int len) {
   if ((int) strlen(buffer) > len) {
	buffer[len]='\n';
	buffer[len+1]=0;
   }
}

void EncodeStatus( char *statusMsg, METARING *metaring, CHILD   child[MAX_CHILD], int *nChild  )
{
   char   line[100];
   char   buffer[100+FILENAME_MAX];
   char   timenow[26];
   int    i;
   char *procInfo;
   int  pn_width = 16;
   char *args[MAX_ARG], parg_line[100], field[3][20];

   sprintf( statusMsg, "                    EARTHWORM SYSTEM STATUS\n\n" );

   sprintf( line,    "        Start time (UTC):       %s", metaring->tstart );
   strcat( statusMsg, line );

   GetCurrentUTC( timenow );
   sprintf( line,    "        Current time (UTC):     %s", timenow );
   strcat( statusMsg, line );

   for ( i = 0; i < metaring->nRing; i++ )
   {
      /* if you change "name/key/size:" then statmgr.c which looks for this key needs to change too */
      sprintf( line, "        Ring %2d name/key/size:  %s / %d / %d kb\n", i+1,
               metaring->ringName[i], metaring->ringKey[i], metaring->ringSize[i] );
      strcat( statusMsg, line );
   }
   sprintf( buffer,    "        Startstop's Config File:    %s\n", metaring->ConfigFile );
   _truncate_with_newline(buffer, MAX_STATUS_LINE_LEN);
   strcat( statusMsg, buffer );
   sprintf( buffer,    "        Startstop's Log Dir:        %s\n", metaring->LogLocation );
   _truncate_with_newline(buffer, MAX_STATUS_LINE_LEN);
   sprintf( buffer,    "        Startstop's Params Dir:     %s\n", metaring->ParamLocation );
   _truncate_with_newline(buffer, MAX_STATUS_LINE_LEN);
   strcat( statusMsg, buffer );
   sprintf( buffer,    "        Startstop's Bin Dir:        %s\n", metaring->BinLocation );
   _truncate_with_newline(buffer, MAX_STATUS_LINE_LEN);
   strcat( statusMsg, buffer );

   sprintf( line,    "        Startstop's Priority Class: %s\n", metaring->MyPriorityClass );
   strcat( statusMsg, line );
   sprintf( line,    "        Startstop Version:          %s\n", metaring->Version );
   strcat( statusMsg, line );

   if ( metaring->maxStatusLineLen == MAX_STATUS_LINE_LEN )
       pn_width = 16;
   else {
       int width, arg_width = 8;
	   pn_width = 7;
	   for ( i = 0; i < *nChild; i++ ) {
		  int j;
          parseCmdLine( args, child[i].commandLine, parg_line );

		  width = (int)strlen( args[0] );
		  if ( width > pn_width )
			 pn_width = width;
	      width = -1;
		  for ( j=1; args[j]; j++ )
		     width += ( 1 + (int)strlen(args[j]) );
		  if ( width > arg_width )
		     arg_width = width;
		  if ( width > arg_width )
   		     arg_width = width;
	   }
	   width = pn_width + 36 + arg_width;
	   if ( pn_width > 16 && width > metaring->maxStatusLineLen ) {
	      width = pn_width - (width + 16 - metaring->maxStatusLineLen)/2;
	   	  pn_width = 16 < width ? width : 16;
	   }
   } 

/* Print stuff about each child process
   ************************************/
   procInfo = statusMsg + strlen(statusMsg);
   sprintf( line, "\n%*s  Process               Class/\n", pn_width, "Process" );
   strcat( statusMsg, line );
   sprintf( line,   "%*s    Id     Status      Priority      Console  Argument\n", pn_width, "Name  " );
   strcat( statusMsg, line );
   sprintf( line,   "%*s  -------  ------      --------      -------  --------\n", pn_width, "-------" );
   strcat( statusMsg, line );

   for ( i = 0; i < *nChild; i++ )
   {
      DWORD status;
      char  statusStr[10];

/* See if the child process has died
   *********************************/
      statusStr[0] = '\0';

     if( strcmp( child[i].status, "Stopped" ) == 0 ) {
        strcpy( statusStr, "Stop" );
     } else if( strcmp( child[i].status, "DOA" ) == 0 ) {
        strcpy( statusStr, "NoExec" );
     } else {
         status = WaitForSingleObject( child[i].procInfo.hProcess, 0 );
         if ( status == WAIT_OBJECT_0 ) {
              strcpy( statusStr, "Dead" );
         } else if ( status == WAIT_TIMEOUT ) {
              strcpy( statusStr, "Alive" );
         }
     }

/* Parse the child's the command line
 ***********************************/
     parseCmdLine( args, child[i].commandLine, parg_line );

/* Add other info about the child
 ********************************/
      line[0] = 0;
      /* Enforce width restrictions on each of these fields */
      sprintf( field[0], "%s", statusStr );
      field[0][6] = 0;
      sprintf( field[1], "%s", child[i].priorityClass );
      field[1][9] = 0;
      sprintf( field[2], "%s", child[i].threadPriority );
      field[2][8] = 0;
      
      sprintf( line, "%*s %7d   %-6s %9s/%-8s", pn_width, args[0], child[i].procInfo.dwProcessId,
      			field[0], field[1], field[2] );
      if( strcmp( child[i].display, "NoNewConsole" ) == 0 )
           strcat( line, "  NoNew " );
      else if( strcmp( child[i].display, "MinimizedConsole" ) == 0 )
           strcat( line, "  Minim " );
      else
           strcat( line, "  New   " );
      AddArgsToStatusLine( args, line, metaring->maxStatusLineLen-1 );
      strcat( statusMsg, line );

/* Append a newline character
 ****************************/
      strcat( statusMsg, "\n" );
   }
}/* end EncodeStatus */

  /******************************************************************
   *                           StartChild                           *
   *                                                                *
   * Start a specific child given by the index in the child array   *
   ******************************************************************/
int StartChild( int ich, METARING *metaring, CHILD  child[MAX_CHILD]  )
{
   STARTUPINFO startUpInfo;
   BOOL  success;
   DWORD priorityClass;
   int   threadPriority;
   DWORD display = CREATE_NEW_CONSOLE;
   char title[80];
   DWORD previousPID;

/* Retrieve the STARTUPINFO structure for the current process.
   Use this structure for the child processes.
   ***********************************************************/
      GetStartupInfo( &startUpInfo );


      previousPID = child[ich].procInfo.dwProcessId;
/* Set the priority class
   **********************/
      if ( strcmp( child[ich].priorityClass, "Idle"     ) == 0 )
         priorityClass = IDLE_PRIORITY_CLASS;
      else if ( strcmp( child[ich].priorityClass, "Normal"   ) == 0 )
         priorityClass = NORMAL_PRIORITY_CLASS;
      else if ( strcmp( child[ich].priorityClass, "High"     ) == 0 )
         priorityClass = HIGH_PRIORITY_CLASS;
      else if ( strcmp( child[ich].priorityClass, "RealTime" ) == 0 )
         priorityClass = REALTIME_PRIORITY_CLASS;
      else
      {
         logit( "et", "Invalid PriorityClass: %s  Exiting.\n",
                child[ich].priorityClass );
         return -1;
      }

/* Set the display type
   ********************/
      if ( strcmp( child[ich].display, "NewConsole" ) == 0 )
         display = CREATE_NEW_CONSOLE;
      if ( strcmp( child[ich].display, "NoNewConsole" ) == 0 )
         display = 0;
      if ( strcmp( child[ich].display, "MinimizedConsole" ) == 0 )
      {
         display = CREATE_NEW_CONSOLE;
         startUpInfo.dwFlags |= STARTF_USESHOWWINDOW;
         startUpInfo.wShowWindow = SW_SHOWMINNOACTIVE;
      }

/* Set the title of the child's window to the child's name
   *******************************************************/
      if ( display == CREATE_NEW_CONSOLE )
      {
         strcpy( title, "Console Window for <" );
         strcat( title, child[ich].commandLine );
         strcat( title, ">" );
         startUpInfo.lpTitle = (LPTSTR)title;

         /* Tell Windows to use the "standard" station and desktop, rather than what's inherited.
          * Otherwise, console's won't be displayed at all, which can be frustrating...
          * This is only needed for service startstop */
         startUpInfo.lpDesktop = "WinSta0\\Default";
      }
      else
         startUpInfo.lpTitle = NULL;



/* Start a child process
   *********************/
      success = CreateProcess( 0,
                               child[ich].commandLine, /* Command line to invoke child */
                               0, 0,                   /* No security attributes */
                               FALSE,                  /* No inherited handles */
                               display |               /* Child may get its own window */
                               priorityClass,          /* Priority class of child process */
                               0,                      /* Not passing environmental vars */
                               0,                      /* Current dir same as calling process */
                               &startUpInfo,           /* Attributes of process window */
                               &(child[ich].procInfo) ); /* Attributes of child process */
      if ( !success )
      {
         LPVOID lpMsgBuf;
         DWORD  lasterror = GetLastError();

         FormatMessage(
             FORMAT_MESSAGE_ALLOCATE_BUFFER |
             FORMAT_MESSAGE_FROM_SYSTEM |
             FORMAT_MESSAGE_IGNORE_INSERTS,
             NULL,
             lasterror,
             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
             (LPTSTR) &lpMsgBuf,
             0,
             NULL );
         child[ich].procInfo.dwProcessId = previousPID;
         strcpy( child[ich].status, "DOA" ); /* Dead On Arrival */
         logit( "et", "Trouble creating child process <%s>; Error %d: %s\n",
                child[ich].commandLine, lasterror, lpMsgBuf );

         LocalFree( lpMsgBuf );
         return -1;
      }

/* Set the thread priority of the child process
   ********************************************/
      if ( strcmp( child[ich].threadPriority, "Lowest"     ) == 0 )
         threadPriority = THREAD_PRIORITY_LOWEST;
      else if ( strcmp( child[ich].threadPriority, "BelowNormal"   ) == 0 )
         threadPriority = THREAD_PRIORITY_BELOW_NORMAL;
      else if ( strcmp( child[ich].threadPriority, "Normal"     ) == 0 )
         threadPriority = THREAD_PRIORITY_NORMAL;
      else if ( strcmp( child[ich].threadPriority, "AboveNormal" ) == 0 )
         threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
      else if ( strcmp( child[ich].threadPriority, "Highest" ) == 0 )
         threadPriority = THREAD_PRIORITY_HIGHEST;
      else if ( strcmp( child[ich].threadPriority, "TimeCritical" ) == 0 )
         threadPriority = THREAD_PRIORITY_TIME_CRITICAL;
      else if ( strcmp( child[ich].threadPriority, "Idle" ) == 0 )
         threadPriority = THREAD_PRIORITY_IDLE;
      else
      {
         logit( "et", "Invalid ThreadPriority: %s  Exiting.\n",
                child[ich].threadPriority );
         return -1;
      }
      success = SetThreadPriority( child[ich].procInfo.hThread, /* Thread handle */
                                   threadPriority );
      if ( !success )
      {
         logit( "et", "Error setting child thread priority.  Exiting.\n" );
         return -1;
      }
      strcpy( child[ich].status, "Alive" ); /* this just changes the flag, when status checks later it'll confirm this */
      return 0;
} /* end StartChild */

  /******************************************************************
   *                      TerminateChild                            *
   *                                                                *
   * Kill a specific child given by the index in the child array.   *
   ******************************************************************/
int TerminateChild( int ich, METARING *metaring, CHILD  child[MAX_CHILD] )
{
   int status = 0;
   int wait_timeout = metaring->KillDelay;

/* Terminate the child process.  NT allows this to be done, but warns:
   "Use it only in extreme circumstances.  The state of global data
    maintained by dynamic-link libraries (DLLs) may be compromised if
    TerminateProcess is used rather than ExitProcess"
 *********************************************************************/
   TerminateProcess( child[ich].procInfo.hProcess, (UINT)-1 );

   status = WaitForSingleObject( child[ich].procInfo.hProcess, wait_timeout*1000 );
   if ( status != WAIT_OBJECT_0 ) return -1;

   return 0;
} /* end TerminateChild */

  /******************************************************************
   *                           RestartChild                         *
   *                                                                *
   *     Restart a specific child given a TYPE_RESTART message      *
   ******************************************************************/

void RestartChild( char *restartmsg, METARING *metaring, CHILD  child[MAX_CHILD], int *nChild  )
{

   int ich = 0, ir, ret;
   DWORD currentPID;
   char ErrText[512];
   ret = StopChild( restartmsg, metaring, child, nChild, &ich );
   if (ret == EXIT) {
       return;
   }


/* Restart the child!
 ********************/
   if( StartChild( ich, metaring, child ) != 0 )
   {
      sprintf( ErrText, "restart of <%s> failed\n",
               child[ich].commandLine );
      ReportError( ERR_STARTCHILD, ErrText, metaring );
      for( ir=0; ir<metaring->nRing; ir++ ) 
      	  tport_detachFromFlag( &(metaring->Region[ir]), child[ich].procInfo.dwProcessId );
   }
   currentPID = child[ich].procInfo.dwProcessId;
   if( currentPID < 0 ) {
       strcpy( child[ich].status, "Alive" );
   }
   logit( "et", "startstop: restarted <%s>\n",
          child[ich].commandLine );

   /* Release the mutex that StopChild acquired but didn't release */
   ReleaseSpecificMutex( &(child[ich].mutex) );

} /* end RestartChild */

  /******************************************************************
   *                     RestartChildThread                         *
   *                                                                *
   *  Wrapper around RestartChild so it can be called in a thread   *
   ******************************************************************/

void RestartChildThread( void *args[] )
{
    char *restartmsg = (char*)(args[0]);
    METARING *metaring = (METARING*)(args[1]);
    CHILD *child = (CHILD*)(args[2]);
    int *nChild = (int*)(args[3]);

    RestartChild( restartmsg, metaring, child, nChild );
}


  /******************************************************************
   *                           StopChild                            *
   *                                                                *
   *     Stop a specific child given a TYPE_RESTART or              *
   *     TYPE_STOP message                                          *
   ******************************************************************/

int StopChild( char *restartmsg, METARING *metaring, CHILD  child[MAX_CHILD], int *nChild, int *ich )
{
   int procId = 0;
   int status = 0;
   int ir, childNum;
   char ErrText[512];


/* Find this process id in the list of children
 **********************************************/
   procId = atoi(restartmsg);

   for( childNum = 0; childNum < *nChild; childNum++ )
   {
      if( (int) child[childNum].procInfo.dwProcessId == procId ) break;
   }

   if( childNum == *nChild )
   {
      logit( "et", "startstop: Cannot stop process_id %d; "
                    "it is not my child!\n", procId );
      return EXIT;
   }

/* Get exclusive access to this child
 *************************************/
   RequestSpecificMutex( &(child[childNum].mutex) );


  /* our records show this child has already been stopped. so we'll not try to stop it again */
   if( strcmp( child[childNum].status, "Stopped" ) == 0 ) {
       goto cleanup;
   }
   /* if we have a negative process ID, we never successfully started this child */
   if ( procId < 0 ) {
      /* strcpy( child[childNum].status, "Stopped" ); */
      goto cleanup;
   }

/* Give child a chance to shut down gracefully...
 ************************************************/
   if ( tport_newModule( procId ) )
       tport_putflag( NULL, procId );
   else
	   for( ir=0; ir<metaring->nRing; ir++ ) tport_putflag( &(metaring->Region[ir]), procId );
   status = WaitForSingleObject( child[childNum].procInfo.hProcess, metaring->KillDelay*1000 );

/* ...but if it takes too long, then kill the child process
 **********************************************************/
   if( status != WAIT_OBJECT_0 )
   {
      logit( "et", "startstop: <%s> (pid=%d) did not shut down in %d sec;"
             " terminating it!\n", child[childNum].commandLine, procId, metaring->KillDelay );

      if( TerminateChild( childNum, metaring, child ) != 0 )
      {
         sprintf( ErrText, "termination of <%s> failed; "
                       "waited %d seconds; cannot restart!\n",
                  child[childNum].commandLine, metaring->KillDelay );
         ReportError( ERR_STARTCHILD, ErrText, metaring );
         ReleaseSpecificMutex( &(child[childNum].mutex) );
         return EXIT; /* we don't want a start to happen if the stop failed */
      }
   }

   /* Close child process and thread handles
    * NB: These CloseHandle () calls used to be
    * in TerminateChild().  But, this causes
    * a very nasty, little bug whereby the handles
    * would not be cleaned up if the child terminated
    * itself, i.e., upon not being able to connect
    * to a K2.  This caused a slow handle leak which,
    * alas, caused the system to crash eventually.
    *
    *   Lucky Vidmar 19 April 2002
    *************************************************/
   if( CloseHandle( child[childNum].procInfo.hProcess ) == 0 ) {
      logit("e","startstop: error closing child process handle: %s\n",
            GetLastError() );
      ReleaseSpecificMutex( &(child[childNum].mutex) );
      return EXIT; /* we don't want a start to happen if the stop failed */
   }
   if( CloseHandle( child[childNum].procInfo.hThread ) == 0 ) {
      logit("e","startstop: error closing child thread handle: %s\n",
            GetLastError() );
      ReleaseSpecificMutex( &(child[childNum].mutex) );
      return EXIT; /* we don't want a start to happen if the stop failed */
   }

   strcpy( child[childNum].status, "Stopped" );

cleanup:
   for( ir=0; ir<metaring->nRing; ir++ ) 
      	  tport_detachFromFlag( &(metaring->Region[ir]), child[childNum].procInfo.dwProcessId );
   if ( ich != NULL )
       *ich = childNum;
   else {
        ReleaseSpecificMutex( &(child[childNum].mutex) );
    }

   return SUCCESS;
} /* end StopChild */


  /******************************************************************
   *                       StopChildThread                          *
   *                                                                *
   *    Wrapper around StopChild so it can be called in a thread    *
   ******************************************************************/

void StopChildThread( void *args[] )
{
    char *restartmsg = (char*)(args[0]);
    METARING *metaring = (METARING*)(args[1]);
    CHILD *child = (CHILD*)(args[2]);
    int *nChild = (int*)(args[3]);

    StopChild( restartmsg, metaring, child, nChild, NULL );
}

int StartstopSetup ( METARING *metaring, volatile int *checkpoint, boolean service, CHILD   child[MAX_CHILD], int *nChild ) {
    HANDLE  MyHandle;
    char    *runPath;
    int     i;
    int     ichild;
    BOOL    success;
    int     err;

    badProcessID = (DWORD)-100;

    /* Change working directory to environment variable EW_PARAMS value
   ***************************************************************/
   runPath = getenv( "EW_PARAMS" );

   if ( runPath == NULL )
   {
      logit( "e" ,
              "Environment variable EW_PARAMS not defined; exiting!\n" );
      if ( service ) {
        set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }

   if ( *runPath == '\0' )
   {
      logit( "e" , "Environment variable EW_PARAMS " );
      logit( "e" , "defined, but has no value; exiting!\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }
   success = SetCurrentDirectory( runPath );

   if ( !success )
   {
      logit( "e" , "Params directory not found: %s\n", runPath );
      logit( "e" ,
             "Please reset environment variable EW_PARAMS; exiting!\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }

/* Read configuration parameters
   *****************************/
   *nChild = 0;
   metaring->nRing = 0;
   err = GetConfig( metaring, child, nChild  );
   if ( service ) {
       if (err != 0)
       {
           set_service_status(SERVICE_STOPPED, err, 0, 0);
           return -1;
       }
   }
   logit( "e" , "startstop: Read command file <%s>\n", metaring->ConfigFile );
   if ( service ) {
       set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
       *checkpoint = *checkpoint + 1;
   }

/* Reinitialize the logging level
   *************************************/
   logit_init( metaring->ConfigFile, 0, 1024, metaring->LogSwitch );
   if ( service ) {
        set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
        *checkpoint = *checkpoint + 1;
   }

/* Get the local installation id from the
   environmental variable EW_INSTALLATION
   **************************************/
   if ( GetLocalInst( &(metaring->InstId) ) < 0 )
   {
      logit( "e" , "Error getting the local installation id; exiting!\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }

/* Look up module id and message type numbers
   ******************************************/
   if ( GetModId( "MOD_WILDCARD", &(metaring->ModWildcard) ) != 0 )
   {
      printf( "startstop: Can't find number for <MOD_WILDCARD> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MODULETYPE, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_HEARTBEAT", &(metaring->TypeHeartBeat) ) != 0 )
   {
      printf( "startstop: Can't find number for <TYPE_HEARTBEAT> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_ERROR", &(metaring->TypeError) ) != 0 )
   {
      printf( "startstop: Can't find number for <TYPE_ERROR> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_RESTART", &(metaring->TypeRestart) ) != 0 )
   {
      printf( "startstop: Can't find number for <TYPE_RESTART> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_STOP", &(metaring->TypeStop) ) != 0 )
   {
      printf( "startstop: Can't find number for <TYPE_STOP> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_REQSTATUS", &(metaring->TypeReqStatus) ) != 0 )
   {
      printf( "startstop: Can't find number for <TYPE_REQSTATUS> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_STATUS", &(metaring->TypeStatus) ) != 0 )
   {
      printf( "startstop: Can't find number for <TYPE_STATUS> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_RECONFIG", &(metaring->TypeReconfig) ) != 0 )
   {
      printf( "startstop: Can't find number for <TYPE_RECONFIG> in earthworm*.d. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }




/* Set startstop's priority class
   ******************************/
   MyHandle = GetCurrentProcess();
   if ( strcmp( metaring->MyPriorityClass, "Idle" ) == 0 )
      SetPriorityClass( MyHandle, IDLE_PRIORITY_CLASS );
   else if ( strcmp( metaring->MyPriorityClass, "Normal" ) == 0 )
      SetPriorityClass( MyHandle, NORMAL_PRIORITY_CLASS );
   else if ( strcmp( metaring->MyPriorityClass, "High" ) == 0 )
      SetPriorityClass( MyHandle, HIGH_PRIORITY_CLASS );
   else if ( strcmp( metaring->MyPriorityClass, "RealTime" ) == 0 )
      SetPriorityClass( MyHandle, REALTIME_PRIORITY_CLASS );
   else
   {
      logit( "et", "Invalid MyPriorityClass: %s  Exiting.\n", metaring->MyPriorityClass );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_PRIORITY, 0, 0);
      }
      return -1;
   }

/* Get UTC start time and convert to a 26-char string
   **************************************************/
   GetCurrentUTC( metaring->tstart );

/* Allocate region structures for transport rings
   **********************************************/

   /* from memory.h: typedef unsigned int size_t; */
   /* allocating MAX_RING worth of region space rather than metaring->nRing so we can */
   /* add more rings later */
   metaring->Region = (SHM_INFO *) calloc( (size_t)MAX_RING, sizeof(SHM_INFO) );
   if ( metaring->Region == NULL )
   {
      logit( "et", "Error allocating region structures. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_NOT_ENOUGH_MEMORY, 0, 0);
      }
      return -1;
   }

  if ( service ) {
   set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
   *checkpoint = *checkpoint + 1;
  }

/* Create the transport rings
   **************************/
   tport_createFlag();
   tport_createNamedEventRing();
   for ( i = 0; i < metaring->nRing; i++ )
      tport_create( &(metaring->Region[i]), 1024 * metaring->ringSize[i], metaring->ringKey[i] );

/* Start the heart beating
   ***********************/
   Heartbeat( metaring );

   if ( service ) {
       set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
       *checkpoint = *checkpoint + 1;
   }

    /* start statmgr child first if present */
   if (metaring->statmgr_location != (MAX_CHILD + 1)) {
        ichild = metaring->statmgr_location;

        if ( StartChild( ichild, metaring, child ) != 0 )
        {
            StartError(ichild, child[metaring->statmgr_location].commandLine, metaring, nChild);
        }

        logit("et","startstop: process <%s> started.\n",
                child[metaring->statmgr_location].commandLine );

        /* Sleep after starting statmgt to allow statmgr to come up */
        logit("et","startstop: sleeping <%d> second(s) for statmgr startup.\n",
                (metaring->statmgr_sleeptime/1000) );

       if ( service ) {
        set_service_status(SERVICE_START_PENDING, 0, *checkpoint, metaring->statmgr_sleeptime + 10000);
        *checkpoint = *checkpoint + 1;
       }
        sleep_ew(metaring->statmgr_sleeptime);

   } // Done  starting statmgr
   /* start the other children that aren't statmanager */
   SpawnChildren( metaring, child, nChild, service, checkpoint);

   return 0;

} /* end StartstopSetup */

void SpawnChildren (METARING *metaring, CHILD child[MAX_CHILD], int *nChild, boolean service, volatile int *checkpoint  ){
    /* Start the child processes
    *************************/
    int ichild;

    /* The following changes were made by John Patton to fix the
     processes dying before statmgr comes up bug */
   for ( ichild = oldNChild; ichild < *nChild; ichild++ )
   {
        /* To prevent starting statmgr a second time, we'll just skip over it
          If it's not there, then the index to skip defaults to one past MAXCHILD, so
          no other modules will be skipped. */
        if (ichild != metaring->statmgr_location)
        {
            if ( StartChild( ichild, metaring, child ) != 0 )
            {
                StartError(ichild, child[ichild].commandLine, metaring, nChild);
                child[ichild].procInfo.dwProcessId = badProcessID--;
                strcpy( child[ichild].status, "DOA" ); /* Dead On Arrival */
            } else {
                logit("et","startstop: process <%s> started.\n",
                  child[ichild].commandLine );
            }

            if ( service ) {
                set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
                *checkpoint = *checkpoint + 1;
            }
        }
   }
} /* end SpawnChildren */

int FinalLoop ( METARING *metaring, volatile int *done, char ewstat[MAX_STATUS_LEN], volatile int *checkpoint, boolean service, CHILD child[MAX_CHILD], int *nChild  ) {
/* Wait for the Interactive thread to set the done flag
   or for a TERMINATE flag to show up in any memory ring.
   In the meantime, check for any ReqStatus or Restart messages...
   ***************************************************************/
   MSG_LOGO  getLogo[4]; /* if you change this 4 here, you need to change the parameter to tport_getmsg as well */
   MSG_LOGO  msglogo;
   int       i, j;
   int       rc, placeholder;
   int       err;
   long      msgsize = 0L;
   char      msg[512];

   for (i = 0; i < 4; i++) {
       getLogo[i].instid = metaring->InstId;
       getLogo[i].mod    = metaring->ModWildcard;
   }
   getLogo[0].type   = metaring->TypeRestart;
   getLogo[1].type   = metaring->TypeReqStatus;
   getLogo[2].type   = metaring->TypeReconfig;
   getLogo[3].type   = metaring->TypeStop;


   /* Create a named pipe.  This is used to receive commands to create a new console in
      the LocalService space. */
   pipeConsoleHandle = CreateNamedPipe(metaring->pipeName,
                                PIPE_ACCESS_INBOUND,
                                PIPE_TYPE_BYTE,
                                1,
                                1024,
                                1024,
                                100,
                                NULL);
   if (pipeConsoleHandle == INVALID_HANDLE_VALUE)
   {
        err = GetLastError();
        logit( "et", "Unable to create named pipe: Error %d.  Exiting.\n", err);
        set_service_status(SERVICE_STOPPED, err, 0, 0);
        return -1;
   }
   StartNewConsolePipeThread();   //run thread for launch-new-console pipe

   logit("et", "Beginning main loop.\n");

   while( !*done )
   {

   /* Send hearbeat, if it's time
    *****************************/
      Heartbeat( metaring );
      /* Is "kill flag" there?
       ***********************/
      for ( i = 0; i < metaring->nRing; i++ )
      {

      /* Look for any interesting new messages
       ***************************************/
         rc = tport_getmsg( &(metaring->Region[i]), getLogo, 4,
                            &msglogo, &msgsize, msg, sizeof(msg)-1 );

         if( rc==GET_NONE || rc==GET_TOOBIG ) continue;
      /* Process a new message
       ***********************/
         msg[msgsize]='\0';  /* Null-terminate it for easy handling */
         if     ( msglogo.type == metaring->TypeRestart   ) {
            int tid;
            void *args[4];             /* set values separately instead   */
            args[0] = (void*)msg;      /*  of aggregate initializer to    */
            args[1] = (void*)metaring; /*  avoid compiler complaint about */
            args[2] = (void*)child;    /*  non-standard extension         */
            args[3] = (void*)nChild;
            if ( StartThreadWithArg( RestartChildThread, (void*)args, 0, (unsigned*)&tid ) == -1 ) {
                logit( "et", "startstop: couldn't do restart in background\n");
                RestartChild( msg, metaring, child, nChild  );
            }
         /* sees TYPE_STOP and runs StopChild. This should kill the child
          * if it isn't already marked internally with  a "Stopped" status
          * If successful it sets the status for the module as "Stopped"
          ********************************************************************/
         } else if( msglogo.type == metaring->TypeStop ) {
            int tid;
            void *args[4];             /* set values separately instead   */
            args[0] = (void*)msg;      /*  of aggregate initializer to    */
            args[1] = (void*)metaring; /*  avoid compiler complaint about */
            args[2] = (void*)child;    /*  non-standard extension         */
            args[3] = (void*)nChild;
            if ( StartThreadWithArg( StopChildThread, (void*)args, 0, (unsigned*)&tid ) == -1 ) {
                logit( "et", "startstop: couldn't do restart in background\n");
                StopChild( msg, metaring, child, nChild, &placeholder);
            }
         } else if( msglogo.type == metaring->TypeReqStatus )  SendStatus( i, metaring, child, nChild);
         else if( msglogo.type == metaring->TypeReconfig ) {
             int rv = GetUtil_LoadTableCore(0); /* reread earthworm.d and earthworm_global.d */
             if ( rv == -1 )
                logit( "et", "startstop: reconfigure rejected due to error\n" );
             else {
                 logit( "et" , "startstop: Reconfigure: Re-reading command file <%s>\n", metaring->ConfigFile );
               /* We're re-reading the startstop*.d file here, and adding any new modules or rings */
               /* Even if this new GetConfig returns an error, we don't want to bail out on our running earthworm */
               oldNChild = *nChild;
               GetConfig( metaring, child, nChild );

               for ( j = metaring->nRing; j < (newNRing); j++ ) {
                    logit( "et" , "tport_create: creating ring number <%d>\n", j );
                    tport_create( &(metaring->Region[j]),
                        1024 * metaring->ringSize[j],
                        metaring->ringKey[j] );
                    metaring->nRing ++;
               }

               if (*nChild > oldNChild ) {
                    logit( "et" ,
                       "startstop: Adding child, oldNChild=%d, nChild=%d, statMgrLoc=%d\n",
                           oldNChild, *nChild, metaring->statmgr_location );
               } else {
                    logit( "et" ,
                       "startstop: Not adding child, oldNChild=%d, nChild=%d, statMgrLoc=%d\n",
                           oldNChild, *nChild, metaring->statmgr_location );
               }
               SpawnChildren( metaring, child, nChild, service, checkpoint );
               if (metaring->statmgr_location != (MAX_CHILD + 1)) {
                    logit( "et" , "startstop: Final reconfigure step: Restart statmgr\n" );
                    sprintf (msg, "%d", child[metaring->statmgr_location].procInfo.dwProcessId);
                    RestartChild( msg, metaring, child, nChild );
               }
            }
         } else
            logit("et","Got unexpected msg type %d from transport region: %s\n",
                        (int) msglogo.type, metaring->ringName[i] );
      }

      if ( tport_getflag( NULL ) == TERMINATE )
      {
          logit("et", "'done' set from kill flag.\n");
          *done = 1;
      } else
          sleep_ew( 1000 );
   }
   logit("et", "Main loop finished.\n");
   goto ShutDown;

ShutDown:

/* Set the TERMINATE flag in each memory ring
   ******************************************/
   for ( i = 0; i < metaring->nRing; i++ )
      tport_putflag( &(metaring->Region[i]), TERMINATE );

   logit( "et","Earthworm will stop in %d seconds...\n", metaring->KillDelay );

   StopNewConsolePipeThread();    //stop thread for launch-new-console pipe
   terminatePipeConsole(TRUE);    //terminate any running pipe-console

/* Wait for all processes to terminate
   ***********************************/
   {
      int nChildActive; /* Number of children that actually ever had a real process ID.
                           Dead On Arrival children missing binaries or whatever never
                           got a process ID, so we won't consider them active. (We can't
                           wait for them to quit, since they never started.) */
      DWORD  status;
      HANDLE *handles;
      const BOOL waitForAll = TRUE;
      const DWORD timeout = metaring->KillDelay*1000;       /* milliseconds */

      nChildActive = *nChild;

      for ( i = 0; i < *nChild; i++ ) {
          if (strcmp( child[i].status, "DOA") == 0) {
              nChildActive --;
          }
      }

      handles = (HANDLE *)malloc( nChildActive * sizeof(HANDLE) );
      if ( handles == NULL )
      {
         logit( "et", "Cannot allocate the handles array.\n" );

        if ( service ) {
            set_service_status(SERVICE_STOPPED, ERROR_NOT_ENOUGH_MEMORY, 0, 0);
            return 0;
        }
        return -1;

      }
      j = 0;
      for ( i = 0; i < *nChild; i++ ) {
          if (strcmp( child[i].status, "DOA") != 0) {
              handles[j] = child[i].procInfo.hProcess;
              j++;
          }
      }

      if ( service ) {
          set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, timeout + 500);
          *checkpoint = *checkpoint + 1;
      }
      status = WaitForMultipleObjects( (DWORD)nChildActive, handles, waitForAll, timeout );
      if ( status == WAIT_FAILED )
      {
         logit( "e", "WaitForMultipleObjects() failed. Error: %d\n", GetLastError() );
      }
      else if ( status == WAIT_TIMEOUT )
      {
         if ( service ) {
            set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, timeout + 500);
            *checkpoint = *checkpoint + 1;
         }
         logit( "e", "\nThe following child processes did not self-terminate:\n" );
         for ( i = 0; i < *nChild; i++ )
         {
            if ( WaitForSingleObject( child[i].procInfo.hProcess, 0 ) == WAIT_TIMEOUT )
            {
               if( TerminateChild( i, metaring, child ) == 0 )
               {
                  logit( "e", " <%s>  KILLED!\n", child[i].commandLine );
                  /* Close child process and thread handles
                   * NB: These CloseHandle () calls used to be
                   * in TerminateChild().  But, this causes
                   * a very nasty, little bug whereby the handles
                   * would not be cleaned up if the child terminated
                   * itself, i.e., upon not being able to connect
                   * to a K2.  This caused a slow handle leak which,
                   * alas, caused the system to crash eventually.
                   *
                   *   Lucky Vidmar 19 April 2002
                   *************************************************/
                   if( CloseHandle( child[i].procInfo.hProcess ) == 0 )
                       logit("e","startstop: error closing child process handle: %s\n",
                             GetLastError() );
                   if( CloseHandle( child[i].procInfo.hThread ) == 0 )
                       logit("e","startstop: error closing child thread handle: %s\n",
                             GetLastError() );
                   if ( service ) {
                       set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, timeout + 500);
                       *checkpoint = *checkpoint + 1;
                   }
               }
               else
                  logit( "e", " <%s>  Failed to kill\n", child[i].commandLine );
            }
         }
      }
      else
      {
         logit( "e", "\nAll child processes have terminated" );
      }
   }
   if ( service ) {
       set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, 2000);
       *checkpoint = *checkpoint + 1;
   }
/* Destroy shared memory regions
   *****************************/
   for( i = 0; i < metaring->nRing; i++ )
        tport_destroy( &(metaring->Region[i]) );
   tport_destroyFlag();
   tport_destroyNamedEventRing();
   logit( "e", ", done destroying shared memory regions" );

/* Destroy the mutexen
   ********************/
   for( i = 0; i < *nChild; i++ )
        CloseSpecificMutex( &(child[i].mutex) );

/* Free allocated space
   ********************/
   free( metaring->Region );
   /*****************
    * child is not an allocated pointer but a static array.  It cannot be free'd.
    * DK 2006/05/30
    * free( child  );
    ************************************************/

   logit( "e", ", freeing allocated space.\n" );
   if ( service ) {
        set_service_status(SERVICE_STOPPED, 0, 0, 0);
   }

   return 0;

} /* end FinalLoop */

/******************************************************************
 *                            SendStatus()                        *
 *    Build a status message and put it in a transport ring       *
 ******************************************************************/
void SendStatus( int iring, METARING *metaring, CHILD child[MAX_CHILD], int *nChild )
{
   MSG_LOGO logo;
   int length;
   char ewstat[MAX_STATUS_LEN];

   logo.instid = metaring->InstId;
   logo.mod    = metaring->MyModId;
   logo.type   = metaring->TypeStatus;

   EncodeStatus( ewstat, metaring, child, nChild );
   length = (int)strlen( ewstat );


   if ( tport_putmsg( &(metaring->Region[iring]), &logo, length, ewstat ) != PUT_OK )
      logit("t", "startstop: Error sending status msg to transport region: %s\n",
             metaring->ringName[iring] );
   return;
} /* end SendStatus */


/*****************************************************************************
 *                       NewConsolePipeRunLoop()                             *
 *    Manages launch-new-console pipe and closes handles on exit.            *
 *****************************************************************************/
void NewConsolePipeRunLoop(void *dummy)
{
    DWORD numBytes;
    static char cBuff[130];

    while(newConsolePipeThreadFlag)
    {  //loop until 'StopNewConsolePipeThread()' is called
        ConnectNamedPipe(pipeConsoleHandle,NULL);  //wait for client connect
        if(!newConsolePipeThreadFlag)       //if stop called then
            break;                          //exit loop (and function)
        if(ReadFile(pipeConsoleHandle, cBuff, sizeof(cBuff)-2,
                                                           &numBytes, NULL))
        {  //data received from pipe OK
            cBuff[numBytes] = '\0';    //make sure null terminated
                   //match command and perform function:
            if (strncmp(cBuff, "NewPipeConsole", 14) == 0)
            {  //command is ""NewPipeConsole#######"; save tag string
                strcpy(pipeConsoleTagStr,&cBuff[14]);
                LaunchPipeConsole();
            }
            else if (strcmp(cBuff, "KillPipeConsole") == 0)
                DoTerminatePipeConsole();
            else if (strcmp(cBuff, "NewConsole") == 0)
                LaunchNewConsole();
            else
            {
                logit("t",
                     "Unknown command received via new-console pipe:  %s\n",
                                                                     cBuff);
            }
        }
        else
        {  //no data from pipe; disconnect so next connect can happen
            DisconnectNamedPipe(pipeConsoleHandle);
            if(!newConsolePipeThreadFlag)   //if stop called then
                break;                      //exit loop (and function)
        }
        Sleep(10);      // (pipe calls should block, but delay just in case)
    }
    CloseHandle(pipeConsoleHandle);
}


/*****************************************************************************
 *              StartNewConsolePipeThread()                                  *
 *    Runs thread to manage launch-new-console "control" pipe.               *
 *****************************************************************************/
void StartNewConsolePipeThread()
{
    unsigned tid;

    newConsolePipeThreadFlag = TRUE;        //set FALSE to terminate thread
    StartThread(NewConsolePipeRunLoop, 0, &tid);
}


/*****************************************************************************
 *              StopNewConsolePipeThread()                                   *
 *    Stops thread that manages launch-new-console "control" pipe.           *
 *****************************************************************************/
void StopNewConsolePipeThread()
{
    newConsolePipeThreadFlag = FALSE;       //set FALSE to terminate thread
    if(pipeConsoleHandle != NULL && pipeConsoleHandle != INVALID_HANDLE_VALUE)
        CancelIo(pipeConsoleHandle);        //try to wake up thread
}


/*****************************************************************************
 *                            LaunchNewConsole()                             *
 *    Hydra incorporation that launches a C++ named pipes console window     *
 *    to allow control of startstop service without administrator password   *
 *****************************************************************************/
void LaunchNewConsole()
{
    STARTUPINFO startUpInfo;
    PROCESS_INFORMATION procinfo;
    BOOL  success;
    DWORD priorityClass;
    int   threadPriority;
    DWORD display = CREATE_NEW_CONSOLE;
    char title[80];
    DWORD lasterror;

    logit("", "Creating new console window\n");

    //
    // To create a new console window, we'll use the procedure from StartChild, with
    // the command-line interpreter as the program to run.
    //

    // Get the startupinfo for this process; use this structure for the console window.
    GetStartupInfo( &startUpInfo );

    // Set the priority class
    priorityClass = NORMAL_PRIORITY_CLASS;

    // Set the display type
    display = CREATE_NEW_CONSOLE;

    strcpy(title, "Startstop console");
    startUpInfo.lpTitle = (LPTSTR)title;

    // Tell Windows to use the "standard" station and desktop, rather than what's inherited.
    // Otherwise, consoles won't be displayed at all, which can be frustrating...
    startUpInfo.lpDesktop = "WinSta0\\Default";

    // Start a child process
    success = CreateProcess( 0,
                            "cmd.exe",              /* Command line to invoke child */
                            0, 0,                   /* No security attributes */
                            FALSE,                  /* No inherited handles */
                            display |               /* Child may get its own window */
                                priorityClass,      /* Priority class of child process */
                            0,                      /* Not passing environmental vars */
                            0,                      /* Current dir same as calling process */
                            &startUpInfo,           /* Attributes of process window */
                            &procinfo );            /* Attributes of child process */
    if (!success)
    {
        LPVOID lpMsgBuf;
        lasterror = GetLastError();

        FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                lasterror,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL );

        logit("et", "Trouble creating console window; Error %d: %s\n",
                lasterror, lpMsgBuf );
        LocalFree( lpMsgBuf );
        return;
    }

    // Set the thread priority of the child process
    threadPriority = THREAD_PRIORITY_NORMAL;
    success = SetThreadPriority(procinfo.hThread, threadPriority);
    if (!success)
    {
        lasterror = GetLastError();
        logit("et", "Error %d setting child thread priority.  Exiting.\n",
                    lasterror);
        return;
    }

    // Close these handles as soon as we get them.  We don't really care about keeping them
    // around; if we are told to shut down, the user will just have to close any open console
    // windows...
    CloseHandle(procinfo.hProcess);
    CloseHandle(procinfo.hThread);
}


/*****************************************************************************
 *                       PipeConsoleRunLoop()                                *
 *    Starts pipe-console and closes handles after it terminates.            *
 *****************************************************************************/
void PipeConsoleRunLoop(void *dummy)
{
    PTSTR pMsg;
    
    logit("t", "Creating pipe console window\n");
    if((pMsg=startPipeConsole(pipeConsoleTagStr)) != NULL)
    {  //error message returned; log message and abort
        logit("et", "Error creating pipe console window:  %s\n", pMsg);
        closePipeConsoleHandles();
        return;
    }
    logit("t", "Pipe console window started and client connected\n");
  
    waitForPipeConsoleStopped(INFINITE);    //wait for pipe-console exit
    closePipeConsoleHandles();
    logit("t", "Pipe console window closed\n");
}


/*****************************************************************************
 *                            LaunchPipeConsole()                            *
 *    Launches command window with stdin/stdout redirected to pipe.          *
 *****************************************************************************/
void LaunchPipeConsole()
{
    unsigned tid;
    
    if(isPipeConsoleRunning())
    {  //previous instance of console-pipe is still running; terminate it
        logit("t", "Terminating previous instance of pipe console\n");
        if(!terminatePipeConsole(TRUE))
            logit("et", "Unable to terminate previous pipe console\n");
        waitForPipeConsoleStopped(500);     //let previous pipe-console exit
    }
         //start/run console separate thread (otherwise crashes on exit):
    StartThread(PipeConsoleRunLoop, 0, &tid);
}


/*****************************************************************************
 *      DoTerminatePipeConsole()                                             *
 *    Terminates the pipe-console.                                           *
 *****************************************************************************/
void DoTerminatePipeConsole()
{
    if(!terminatePipeConsole(FALSE))
        logit("et", "Unable to terminate pipe console\n");
}
