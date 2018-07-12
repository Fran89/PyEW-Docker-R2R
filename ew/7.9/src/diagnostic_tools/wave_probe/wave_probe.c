

/*
  wave_probe.c

  A program designed to send a request for one station to a waveserver.
  creating a debugging file for use in diagnosing waveserver problems.
    Patton
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <time.h>
#include <math.h>
#include <platform.h> /* includes system-dependent socket header files */
#include <chron3.h>

#include "ws_clientII.h"
#include <time_ew.h>

#include <parse_trig.h>

#define MAX_WAVESERVERS   10
#define MAX_ADRLEN        20
#define MAXTXT           150

/* Function prototypes
   *******************/
static int ConvertTime (char *, double *);
void LogWsErr( char [], int, FILE * );

/* Functions from other source files */
extern int     t_atodbl (char*, char*, double*);

/* Globals 
 *********/
char           wsIp[MAX_ADRLEN];       /* Waveserver Ip to test */
char           wsPort[MAX_ADRLEN];     /* Waveserver Port to test */

/* Constants
   *********/
const long     wsTimeout = 30000;      /* milliSeconds to wait for reply */
static long    MaxTraces = 50;         /* max traces per message we'll ever deal with  */

static char   *TraceBuffer;            /* where we store the trace snippet  */
static long    TraceBufferLen =100000; /* bytes of largest snippet we're up for - 
                                          from configuration file */
static WS_MENU_QUEUE_REC MenuList;     /* The menu */
static TRACE_REQ *TargetRequest;       /* The trace_request */

static char   targettime[20];

int main( int argc, char *argv[] )
{
   int     returncode;         /* Function return code */

   char    station[7];         /* Site name */
   char    component[9];       /* Component/channel code */
   char    network[9];         /* Network name */
 
   FILE    *Error_File;        /* File to write all output to */

   /* Check command line arguments
    ******************************/
   if ( argc != 7 )
   {
	 printf( "\n  wave_probe:\n");
	 printf( "    A program designed to send a trace request for one station to a waveserver,\n");
     printf( "    creating a debugging file for use in diagnosing waveserver problems.\n");
     printf( "  Usage: wave_probe <server> <port> <S> <C> <N> <yyyymmddhhmmss.ss>\n" );
     return -1;
   }

  /* Since our routines call logit, we must initialize it, although we don't
   * want to!
   *************************************************************************/
  logit_init( "wave_probe", (short) 0, 256, 0 );

   /* Allocate the trace snippet buffer
    ***********************************/
   if ((TraceBuffer = malloc ((size_t) TraceBufferLen)) == NULL)
   {
     fprintf( stderr,
             "Cannot allocate snippet buffer of %ld bytes. Exitting\n",
             TraceBufferLen);
     return EW_FAILURE;
   }

   /* Allocate the trace request structures 
    ***************************************/
   if ( (TargetRequest = (TRACE_REQ *)calloc ((size_t)MaxTraces, sizeof(TRACE_REQ))) == (TRACE_REQ *)NULL)
   {
     fprintf( stderr,
             "Out of memory for %d TRACE_REQ structures.\n",
             MaxTraces);
     return EW_FAILURE;
   }

   /* Get info from command line input
    **********************************/
   strcpy( wsIp,       argv[1]);
   strcpy( wsPort,     argv[2]);
   strcpy( station,    argv[3]);
   strcpy( component,  argv[4]);
   strcpy( network,    argv[5]);
   strcpy( targettime, argv[6]);
 
   /* Open the output file for writing
    **********************************/
   if ( (Error_File = fopen ("output.txt", "w")) == NULL)
    {
      fprintf( stderr, "Can't open output.txt\n");
      return EW_FAILURE;
    }

   fprintf(Error_File, "Attempting to probe wave server %s:%s\n\n", wsIp, wsPort);
   printf ("Attempting to probe wave server %s:%s\n", wsIp, wsPort);

   /* Set the waveserver debug flag
    *******************************/
   fprintf(Error_File, "Wave_Probe: Setting the waveserver debug flag\n");
   printf("Wave_Probe: Setting the waveserver debug flag\n");
   returncode = setWsClient_ewDebug(1, Error_File);

   /* Initialize the socket system
   *******************************/
   fprintf(Error_File, "Wave_Probe: Initializing the socket system\n");
   printf("Wave_Probe: Initializing the socket system\n");
   SocketSysInit();
 
   /* Attach to a waveserver
    ************************/
   fprintf(Error_File, "Wave_Probe: Attaching to waveserver\n");
   printf("Wave_Probe: Attaching to waveserver\n");
   returncode = wsAppendMenu( wsIp, wsPort, &MenuList, wsTimeout, Error_File );
   if ( returncode != WS_ERR_NONE )
   {
     LogWsErr( "wave_probe", returncode, Error_File );
     return -1;
   }
   
   if (MenuList.head == NULL )
   {
     fprintf( stderr, "wave_probe: nothing in server\n");
     exit( 0 );
   }

   /* Fill in TRACE_REQ structure
    *****************************/
   fprintf(Error_File, "Wave_Probe: Filling in TRACE_REQ structure\n");
   printf("Wave_Probe: Filling in TRACE_REQ structure\n");

   strcpy(TargetRequest->sta, station);
   strcpy(TargetRequest->chan, component);
   strcpy(TargetRequest->net, network);

   if (ConvertTime (targettime, &TargetRequest->reqStarttime) != EW_SUCCESS)
   {
     fprintf (Error_File, "Call to ConvertTime failed.\n");
     return EW_FAILURE;
   }

   TargetRequest->reqEndtime = (TargetRequest->reqStarttime + 60);
   TargetRequest->pBuf = TraceBuffer;
   TargetRequest->bufLen = TraceBufferLen;

   fprintf (Error_File, "Trace Request:\n  Sta: %s\n  Chan: %s\n  Net: %s\n  StartTime: %f\n  EndTime: %f\n  BufferLen: %d\n\n",
	        TargetRequest->sta, TargetRequest->chan, TargetRequest->net, TargetRequest->reqStarttime, TargetRequest->reqEndtime,
	        TargetRequest->bufLen);

   /* to retrieve the ASCII trace snippet specified in the structure TRACE_REQ 
    **************************************************************************/
   fprintf(Error_File, "Wave_Probe: Retrieveing ASCII trace snippet\n");
   printf("Wave_Probe: Retrieveing ASCII trace snippet\n");

   returncode = wsGetTraceAscii( TargetRequest, &MenuList, wsTimeout, Error_File);
   if ( returncode != WS_ERR_NONE )
   {
     LogWsErr( "wsGetTraceAscii", returncode, Error_File );
     return -1;
   }

   /* Release the linked list created by wsAppendMenu
    *************************************************/
   fprintf(Error_File, "Wave_Probe: Disconnecting from waveserver\n");
   printf("Wave_Probe: Disconnecting from waveserver\n");

   wsKillMenu( &MenuList, Error_File );

   /* We're done
    ************/
   fprintf(Error_File, "Wave_Probe: completed probe.\n");
   printf("Wave_Probe: completed probe, output file is: output.txt.\n");

   fclose(Error_File);
   return 0;
}


void LogWsErr( char fun[], int returncode, FILE *out_file )
{
  switch ( returncode )
    {
    case WS_ERR_INPUT:
      fprintf( out_file, "%s: Bad input parameters.\n", fun );
      break;

    case WS_ERR_EMPTY_MENU:
      fprintf( out_file,  "%s: Empty menu.\n", fun );
      break;

    case WS_ERR_SERVER_NOT_IN_MENU:
      fprintf( out_file,  "%s: Server not in menu.\n", fun );
      break;

    case WS_ERR_SCN_NOT_IN_MENU:
      fprintf( out_file,  "%s: SCN not in menu.\n", fun );
      break;

    case WS_ERR_BUFFER_OVERFLOW:
      fprintf( out_file,  "%s: Buffer overflow.\n", fun );
      break;

    case WS_ERR_MEMORY:
      fprintf( out_file,  "%s: Out of memory.\n", fun );
      break;

    case WS_ERR_BROKEN_CONNECTION:
      fprintf( out_file,  "%s: The connection broke.\n", fun );
      break;

    case WS_ERR_SOCKET:
      fprintf( out_file,  "%s: Could not get a connection (socket).\n", fun );
      break;

    case WS_ERR_NO_CONNECTION:
      fprintf( out_file,  "%s: Could not get a connection.\n", fun );
      break;

    default:
      fprintf( out_file,  "%s: unknown ws_client error: %d.\n", fun, returncode );
      break;
    }

  return;
}

/***********************************************************************
 * ConvertTime () - given pStart return a double representing          *
 *     number of seconds since 1970                                    *
 ***********************************************************************/
static int ConvertTime (char *pStart, double *start)
{
  char	YYYYMMDD[9];
  char	HHMMSS[12];

  if (pStart == NULL) 
  {
    fprintf( stderr, "Invalid parameters passed in.\n");
    return EW_FAILURE;
  }

  strncpy (YYYYMMDD, pStart, (size_t) 8);
  YYYYMMDD[8] = '\0';

  HHMMSS[0] = pStart[8];
  HHMMSS[1] = pStart[9];
  HHMMSS[2] = ':';
  HHMMSS[3] = pStart[10];
  HHMMSS[4] = pStart[11];
  HHMMSS[5] = ':';
  HHMMSS[6] = pStart[12];
  HHMMSS[7] = pStart[13];
  HHMMSS[8] = '.';
  HHMMSS[9] = '0';
  HHMMSS[10] = '0';
  HHMMSS[11] = '\0';

  if (t_atodbl (YYYYMMDD, HHMMSS, start) < 0)
  {
    fprintf( stderr, "Can't convert StartTime %s -> %s %s: t_atodbl failed.\n", 
           pStart, YYYYMMDD, HHMMSS);
    return EW_FAILURE;
  }
	
  return EW_SUCCESS;
}

