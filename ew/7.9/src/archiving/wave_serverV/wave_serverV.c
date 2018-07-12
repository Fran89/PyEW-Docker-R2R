
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: wave_serverV.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.41  2010/03/18 16:37:11  paulf
 *     minor minor code clean up
 *
 *     Revision 1.40  2010/03/18 16:24:38  paulf
 *     added client_ip echoing to ListenForMsg() as per code from DK
 *
 *     Revision 1.39  2007/11/30 18:40:16  paulf
 *     sqlite stuff added
 *
 *     Revision 1.38  2007/05/31 16:11:44  paulf
 *     minor change to add in version number as #define, and update ver so it is clear from the logs which is running
 *
 *     Revision 1.37  2007/05/29 13:42:37  paulf
 *     patched some checks in checking data_type and also some memory alignment issues in calls to TraceBufIsValid()
 *
 *     Revision 1.36  2007/03/28 18:02:34  paulf
 *     fixed flags for MACOSX and LINUX
 *
 *     Revision 1.35  2007/02/26 14:47:38  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.34  2007/02/23 15:24:33  paulf
 *     fixed long to time_t declaration
 *
 *     Revision 1.33  2007/02/19 20:45:38  stefan
 *     tweak on last revision
 *
 *     Revision 1.32  2007/02/19 20:43:36  stefan
 *     patch for gcc 3.3.6 else if defined structure per Matt.VanDeWerken@csiro.au
 *
 *     Revision 1.31  2006/10/23 21:20:29  paulf
 *     updated for linux makes
 *
 *     Revision 1.30  2006/09/05 13:46:52  hal
 *     added _LINUX ifdefs so that signals under linux are handled the same as under solaris
 *
 *     Revision 1.29  2006/07/20 16:18:29  stefan
 *     ifdef fixes for Linux
 *
 *     Revision 1.28  2006/07/11 22:28:53  stefan
 *     The problem was that the size of the tankfile is not an integer multiple
 *     of the packet size and so occasionally there is a mess up at the end of
 *     the file (mine were 3 in this example).  The result was that the data
 *     requested from the server was not returned and an event file failed to
 *     be written.
 *     [Fix was to add an else to wave_serverV_config]
 *     Richard R Luckett, BGS
 *
 *     Revision 1.27  2006/03/27 17:16:49  davek
 *     Updated the wave_serverV version and timestamp.
 *
 *     Revision 1.26  2006/03/27 17:12:07  davek
 *     Added code to check the return value from call to WaveMsg2MakeLocal(),
 *     and reject packets which for which WaveMsg2MakeLocal() reported and error.
 *
 *     Changed a >= to a > comparison in the packet filtering logic that was causing
 *     wave_serverV to reject tracebufs that only contained 1 sample.
 *
 *     Revision 1.25  2005/07/21 21:03:41  friberg
 *     added in one _LINUX ifdef directive for sin_addr struct
 *
 *     Revision 1.24  2005/04/21 22:48:46  davidk
 *     Updated the version timestamp.  (It had not been properly updated in some time.)
 *     Current version supports SCNL menu protocol adjustment.
 *
 *     Revision 1.23  2005/04/07 15:23:48  davidk
 *     Separated the signal handling logic for Solaris(UNIX) and Windows, due
 *     to problems using the UNIX logic in wave servers started from a
 *     Windows Service.
 *     Wave server should behave the same as before except when started from
 *     a service.  Now the service based wave server should not shutdown when
 *     users log in/out.
 *     Added call to WIN32 API SetConsoleCtrlHandler() to setup a windows signal
 *     handler to handle close messages, while using the default service logic to
 *     ignore logoff messages.
 *
 *     Revision 1.22  2005/03/17 17:37:39  davidk
 *     Changes to enforce a maximum tanksize of 1GB.
 *     Added code to enforce 1GB limits on wave_server tanks.
 *     If TruncateTanksTo1GBAndContinue is set in the config file,
 *     then tanks listed in the config file at >= 1GB will be truncated to
 *     just under 1GB, and wave server operation will continue normally.
 *     If TruncateTanksTo1GBAndContinue is not set,
 *     wave server will issue an Error and EXIT if it encounters a tank
 *     >= 1GB in the config file.
 *     This change was made because a bug was discovered that limits
 *     the safe limit of a wave server tank file to 1GB.
 *
 *     Revision 1.21  2004/09/14 16:23:33  davidk
 *     Fixed bug in main() function, where bAbortMainLoop flag was never
 *     initialized.  Initialized bAbortMainLoop to FALSE.
 *     Update wave_serverV.c timestamp.
 *
 *     Revision 1.20  2004/06/10 23:14:09  lombard
 *     Fixed logging of invalid packets
 *     Fixed compareSCNL function.
 *
 *     Revision 1.19  2004/06/09 21:35:58  lombard
 *     Added code to validate a trace_buf packet before it gets into the tanks.
 *     Added comments about GETPIN request not working correctly.
 *
 *     Revision 1.18  2004/05/18 22:30:19  lombard
 *     Modified for location code
 *
 *     Revision 1.17  2003/04/15 18:48:27  dhanych
 *     replaced commented changes with just comments
 *
 *     Revision 1.16  2003/04/14 19:13:58  dhanych
 *     made changes to fix multi-home bug (removed gethomebyaddr)
 *
 *     Revision 1.15  2001/10/03 21:17:02  patton
 *     Made logit changes due to new logit code.
 *     Disk logging is now hardcoded to on
 *     during config file reading, and set
 *     to the user's choice afterwards.
 *     JMP 10/3/2001.
 *
 *     Revision 1.14  2001/08/10 21:49:09  dietz
 *     changed tport_getmsg to tport_copyfrom so we can distinguish between
 *     sequence gaps and truly missed msgs in the transport ring.
 *
 *     Revision 1.13  2001/06/29 22:23:59  lucky
 *     Implemented multi-level debug scheme where the user can specify how detailed
 *     (and large) the log files should be. If no Debug is specified, only
 *     errors are reported and logged.
 *
 *     Revision 1.12  2001/06/18 23:43:46  alex
 *     *** empty log message ***
 *
 *     Revision 1.11  2001/05/08 00:09:47  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.10  2001/01/24 00:01:06  dietz
 *     *** empty log message ***
 *
 *     Revision 1.9  2001/01/20 02:26:39  davidk
 *     resolved a bug where multiple confusing timestamps were issued for wave_serverV.c
 *
 *     Revision 1.8  2001/01/18 02:26:59  davidk
 *     Added ability to issue status messages from server_thread.c,
 *     which entailed the following changes:
 *     1. Changed status message constants and vars from static to
 *     full globals so that status messages could be issued from
 *     other files.
 *     2. Moved #define constants for status message types to wave_serverV.h
 *     so that they can be used by all .c files.
 *     3. Moved wave_serverV_status() prototype to wave_serverV.h so
 *     that other .c files can issue status messages.
 *
 *     Changed the timestamp.
 *
 *     Revision 1.6  2001/01/08 22:06:07  davidk
 *     Modified portion of wave_server that handles client requests,
 *     so that it replies to client when an error occurs while handling
 *     the request.(Before, no response was given to the client when an
 *     error occured.)  Added flags FC,FN, and FB.  Moved the wave_server
 *     version logging into a separate function, such as the one used in
 *     serve_trace.c.
 *
 *     Revision 1.5  2000/07/24 18:46:58  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.4  2000/07/08 19:01:07  lombard
 *     Numerous bug fies from Chris Wood
 *
 *     Revision 1.3  2000/07/08 16:56:21  lombard
 *     clean up logging for server thread status
 *
 *     Revision 1.2  2000/06/28 23:45:27  lombard
 *     added signal handler for graceful shutdowns; numerous bug fixes
 *     See README.changes for complete list
 *
 *     Revision 1.1  2000/02/14 19:58:27  lucky
 *     Initial revision
 *
 *
 */

  /*
   wave_serverV.c:

*****************************************************************************
   Warning:  This file has been victim to multiple Global replace executions,
   changing "wave_serverIII" to "wave_serverIV" and then to "wave_serverV".
   As a result some of the documentation will read a little funny, such as:
   "Wave_serverV represents an upgrade to the Wave_serverV code with
   the following mods:".  The sentence originally looked like "Wave_serverIV
   represents an upgrade to the Wave_serverIII code with the following mods:"
*****************************************************************************

MOTVATION
wave_serverV is being written for the following reason:
 Provide a means of saving indexes and tank structures to disk, such that
 recovery from a crash becomes a simple, quick, turnkey experience, while
 keeping the wave_server performance close to that ov wave_serverV without
 index updates.  Same low performance price, now with improved crash recovery
 and racing stripes.  DK


MOTVATION:
wave_serverV is being written for the following reasons:
* Large storage space per trace: We have to store several days worth of stuff
  from 600+ stations.
* Conurrently serve a number of clients without ever overloading the system.
* Support the new trace format, including variable sample size, rate, and
  variable message length.
* Support interrupted data, either short telemetry dropouts or segmented data
  with weeks between segments.
* Be able to survive being turned on and off: don't loose the data you had,
  and become operational quickly after coming up. (Older wave servers took a
  long time reading their tank files on restart)
* Have an extensible protocol for future clients.

HISTORY:
The original wave server was written by Will in a rather spectacularly short
time: We came in on a Monday to find Will comatose, all waste cans full of
expresso cups, and a working wave server. The motivation was to support
Alaska's interest in writing trace data into DataScope, and to provide a
playback facility for testing real-time algorithms.

Lynn then proceeded to enhance the thing a number of ways. Kent produced a
variation, introducing the idea of segmenting the tank into one partition for
each trace and feeding it demuxed data. Things then diverged, based on some
misuderstanding of what code from Alaska could be integrated into the
Earhtworm release. This is an effort to meet Menlo Park requriements, meet
Tsunami needs, and bring things back together. Several authors were involved
in wave_serverV: Alex wrote the main thread (wave_serverV.c); Mac McKenzie
wrote the parser of the client thread (server_thread.c). Eureka Young wrote
the code to send trace data (serve_trace.c).

LEFT TO DO:
1. Separate thread to pull trace data messages from public ring. (done)
2. Compute oldest time in tank rather than reading it.
3. Index compaction when out of entires
4. Security scheme to limit access to blessed clients.
5. Maybe compute actual digitization rate rather than nominal.

PROGRAM STRUCTURE:

Based on wave_server.c. It deals only with TYPE_TRACEBUF2 messages. It operates
a disk tank for each pin it's told to serve. Each tank is a circular disk
file, an associated TANK structure, and an index (list in memory).  The idea
is that wave_serverV can deal reasonably with fragmentray data
"CHUNKs". Chunks are arbitrarily long sequences of TYPE_TRACEBUF2 messages
without breaks (that is, there are no time gaps longer than 1.5 times the
stated sampling period). Such chunks can be separated by any amount of
time. Thus, telemetry interruptions as well as triggered event data can be
handled.

	The tank file is thought of as being divided into 'records' of
specified length (from the parameter file).  Each write of a TYPE_TRACEBUF2
message is placed into such a 'record'. The number of records in the tank is
computed from the parameters in the config file.

	The TANK structure holds various pointers describing the tank file and
the associated index. This lives in memory, and is the 'living' set of
pointers for the tank. There is one TANK structure for each tank maintained.
Notably, included here is the current value of the insertion point into the
tank file, and a pointer to the start of the index.

	The index is a memory-based list of CHUNK descriptors. At startup, an
array of such descriptors is allocated and initialized. The size of this array
is read from the configuration file. Each such descriptor contains the chunk
start time, end time, and offset into the tank file where the beginning of the
chunk lives; (and of course, a pointer to the next element of the list){This
is now inherited from the array form, instead of being separately maintained.


The scheme (not implemented), is to perform index squeezing: If the number of
data chunks should exceed the available chunk descriptors, the smallest chunks
will be removed from the index, and thus be no longer available to
servers. The motivation is to survive observed telemetry drop-outs with DST's,
where a large number of very small chunks are produced for some period of
time. The motivation is to preserve older, long chunks, and sacrifice what is
probably useless data. Note that it is possible to recover such de-indexed
chunks by brute force reading of the tank file.  When wave_serverV shuts down,
either by request or due to error, it writes the TANK structure and index
array to a companion file to the tank. The name of this companion file is
created by appending an extention (currently ".inx") to the tank file name. On
startup, wave_serverV reads its configuration file, which tells it what tanks
to operate. It then sees if the specified tank files exist already. If so, an
attempt is made of open the associated companion file, and to read the TANK
structure and index from there. A check is made to verify that the TANK
structure from disk is consistent with what was read from the configuration
file. wave_serverV exits with horror if there is a miss-match.

Wave_serverV Improvements (over wave_serverIV)
With wave_serverV, the indexes are periodically written to disk at a frequency
determined by the config file.  The TANK structures, which include the
insertion point for each tank file, are also saved to disk periodically, at a
frequency determined by another param in the config file.  If a crash occurs,
then on restart, wave_server attempts to rebuild the indexes using the
insertion point stored in the TANK structures file, and the indexes, stored in
the index files, for each tank.  On startup, the Tank file is considered to be
only as up to date as the insertion point that is stored on disk.  Therefore,
the insertion point should be saved often.  The index files on disk are not
required to rebuild indexes, but large tanks can require long index rebuild
times, if the must be built from scratch.

wave_serverV also introduces the use of redundant index and tank structures
files.  The redundant files are used to prevent file corruption errors from
disabling the crash recovery process.  The theory behind the redundant files
is that if a system suddenly crashes due to a power outage, or other problem,
and a vital file is being written, then that file could become corrupt, and
would be unusable.  By using alternating redundant files, the recovery files
are better protected, because only one file can be written to at a time, and
thus only one could be corrupted by an interruption during writing.


THREAD STRUCTURE

	Wave_serverV consists of four fulltime threads: The main thread reads
the configuration file, sets things up, and starts a thread which picks up
messages of interest and stuffs them into a memory-based queue. It then drops
into a working loop which picks up trace data messages from the queue and
writes them into the appropriate tank. This thread also maintains the index
lists and keeps the variables in the TANK structure up to date. It also starts
a server manager thread which listens for connect requests over the
network. When a connect rerquest from some client is detected, this thread
starts a copy of the server thread, and gives it the socket descriptor to the
client. This server thread is responsible for getting client requests and
providing the replies. This server thread quits if the client breaks the
socket connection, or if some horrible error is detected. Otherwise the server
thread keeps dealing with the client indefinitely. The server manager thread
will start up to some fixed number (coded-in constant) of server threads to
prevent overloading the machine.  The final thread is the index manager.
Started by main, it intermittently writes the contents of the index lists and
tank structures to disk, to enable recovery from a crash.

THREAD CONCURRENCY AND TANK FILE IO:

	The tank file is used by both the main writing thread and the reading
server threads. Interlock is as follows: There is one mutex for each tank. The
main writing thread takes it on itsself to interlock each tank file operation
and each index operation with a mutex, save the original offset of the tank
file, do the io operation, restore the offset and release the mutex. The
server threads must wait for, hold, and release the mutex whenever it looks at
the tank file or the index list.  The index manager thread must also deal with
mutexes when interacting with tank data.

PROTOCOL:
Notes:
	* The expectation is that additional protocols will be added as
	  needed. Specifically, a 'regurgitate raw' should be defined soon to
	  provide a play-back capability.

	* A value of 'nada' for fill-value indicates no fill which is not
	  supported in the first version. We don't even know how we'd handle
	  that...

	* <s><c><n><l> is short-hand for site code, channel code, network and
	  location id.

	* <flags> :: F | F<letter> ... <letter>

    Currently Supported Flags:

    R  All requested data is right of the tank (after the tank).
    L  All requested data is left of the tank (before the tank).
    G  All requested data is in a gap in the tank.
    B  The Client's request was bad.  It contained incorrect syntax.
    C  The tank from which the client requested data is corrupt.
       (Data may or may not be available for other time intervals
        in the tank.  WARNING!  This flag indicates that the tank
        is corrupt, this means data from other time intervals may
        be inaccurate)
    N  The requested channel(tank) was not found in this wave_server.
    U  An unknown error occurred.
    DK 010501  Added additional flags: FB, FC, FN, FU

	* <datatype> is two character code ala CSS.  Initially, we will
	  support i2, i4, s2, and s4. i for Intel; s for Sparc. 2 meaning
	  two-bytes per integer, 4 ...

	* Replies are ascii, including trace data.

	* the <request id> was added later to permit the wave viewer to keep
	  track of which reply belongs to which query. The idea is that the
	  client sends this as part of it's request. To us it's an arbitrary
	  ascii string.  We simply echo this string as the first thing in our
	  reply. We don't care what it is. The motivation is that the
	  waveviewr would occasionally get confused as to which reply belonged
	  to which of it's requests, and it would sit there, listening for a
	  reply which never came.

				*** NOTE NOTE NOTE ***
          The request id is echoed as a fixed length 12 character string, null
          padded on the right if required.


Description of Requests and Responses

MENU: <request id>
	returns one line for each tank it has:
	<request id>
	pin#  <s><c><n><l> <starttime> <endtime>  <datatype>
	  .      .       .         .          .
	pin#  <s><c><n><l> <starttime> <endtime>  <datatype>
	\n

MENUPIN: <request id>  <pin#>
	returns as above, but only for specified pin number:
	<request id> <pin#>  <s><c><n><l>  <starttime>  <endtime>  <datatype> <\n>

MENUSCN: <request id>  <s><c><n><l>
	returns as above, but for specified <s><c><n><l> name:
	<request id> <pin#>  <s><c><n><l>  <starttime>  <endtime>  <datatype> <\n>

GETPIN:  <request id> <pin#> <starttime>  <endtime> <fill-value>
	returns trace data for specified pin and time interval. Gaps filled
	 with <fill-value>.  <request id> <pin#> <s><c><n><l> F <datatype>
	 <starttime> <sampling rate> sample(1) sample(2)... sample(nsamples)
	 <\n> {the samples are ascii}

	If the requested time is older than anything in the tank, the reply
	is: <request id> <pin#> <s><c><n><l> FL <datatype> <oldest time in tank>
	<sampling rate> \n for the case when the requested interval is younger
	than anything in the tank, the reply is <request id> <pin#> <s><c><n><l>
	FR <datatype> <youngest time in tank> <sampling rate> \n

	NOTE: the GETPIN request has never worked in wave_serverV. As pin
	numbers fade into the distance, it is unlikely that this request will
	ever be supported.

GETSCN: <request id> <s><c><n><l> <starttime>  <endtime> <fill-value>
	returns as above, but for specified scn name.
	<request id> <pin#> <s><c><n><l> F <datatype> <starttime> <sampling-rate >
		 sample(1) sample(2)... sample(nsamples) <\n>


GETSCNRAW: <request id> <s><c><n><l> <starttime>  <endtime>
	returns trace data in the original binary form it which it was put
	into the tank. Whole messages will be supplied, so that the actual
	starttime may be older than requested, and the end time may be younger
	than requested. The reply is part ascii, terminated by a "\n",
	followed by binary messages:

	<request id> <pin#> <s><c><n><l> F <datatype> <starttime> <endtime>
	<bytes of binary data to follow> \n The line above is all in
	ascii. All below is binary, byte order as found in the tank:
	<trace_buf msg> ... <trace_buf msg>

	If the requested time is older than anything in the tank, the reply is:
	 <request id> <pin#> <s><c><n><l> FL <datatype> <oldest time in tank> \n

	For the case when the requested interval is younger than anything in
	the tank, the reply is:
        <request id> <pin#> <s><c><n><l> FR <datatype> <youngest time in tank> \n

	For the case when the requested interval falls completely in a gap,
	the reply is:
	 <request id> <pin#> <s><c><n><l> FG <datatype> \n

*/

#define _WSVMAIN_


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <data_buf.h>
#include <trace_buf.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <mem_circ_queue.h>
#include <time_ew.h>
#include "wave_serverV.h"
#include "tb_packet_db.h"

/* Function Prototypes
 *********************/
void    wave_serverV_config( char * );
void    wave_serverV_lookup( void );
int     ReportServerThreadStatus(void);
void    signal_hdlr(int signum);
int     TraceBufIsValid(TANK * pTSPtr, TRACE2_HEADER * pCurrentRecord);

#define IPADDRLEN 20
#define FILENAMELEN 80

/* Global socket things
***********************/
SOCKET         PassiveSocket;     /* Socket descriptor; passive socket     */
static char    ServerIPAdr[IPADDRLEN];  /* IP address of wave_server machine */
static int     ServerPort;        /* Server port for requests & replies    */

/* Pointers to things and actual allocations
********************************************/
TANK *  Tanks=0;                /* Pointer to the array of configured tank descriptors */
MSG_LOGO        ServeLogo[MAX_TANKS];   /* worst possible case is each pin from a different logo */
mutex_t         QueueMutex;             /* mutex handles. We suspect they're long integers. */
						/* The last one is for the input queue */
int             NLogos;                         /* number of distinct logos to deal with */

/* Parameters to be read from a configuration file (defaults given)
*******************************************************************/
static char	     RingName[MAX_RING_STR];         /* name of transport ring for i/o    */
static long          RingKey;              /* key to transport ring to read from */
static unsigned char MyModId;              /* wave_server's module id            */
static int           LogSwitch;            /* Log-to-disk switch                 */
double               GapThresh;            /* Theshhold factor for gap declaration */
static int           IndexUpdate;            /* should we write the index after each tank write. */
int                  SocketTimeoutLength;  /* Length of Timeouts on SOCKET_ew calls */
int                  SOCKET_ewDebug=0;     /* Set to 1 for socket debugging */
int                  ClientTimeout = -1;   /* Time to wait for any activity
                                              from client; -1: wait forever */
static int           TankStructUpdate;     /* Seconds between TS disk updates */
static int           IndexUpdate;          /* Seconds between index disk updates */
int                  PleaseContinue=0;     /* Indicates whether to continue or not after a tank has failed. */
int                  ReCreateBadTanks=0;   /* Indicates whether a tank should be born-again from scratch
                                              if it is found to be bad during initialization. */
int                  SecondsBetweenQueueErrorReports;
                                           /* Indicates the minimum amount of time between error reports
                                              to statmgr, related to internal message queue lapping. */
int                  MaxServerThreads=10; /* largest number of  server threads we'll start up */
int                  bAbortOnSingleTankFailure=TRUE;
                                          /* set to true in config file if you wish wave_server
                                             to continue if there is an I/O error on a single
                                             tank file. DK 01/08/01      */
int                  bTruncateTanksTo1GBAndContinue=FALSE;
                                          /* set to true in config file if you wish wave_server
                                             to truncate tanks that are listed in the config file at
                                             over 1GB, down to 1GB and continue.  Default behavior
                                             is to complain and exit if a >1GB tank is given.DK 2005/03/17  */

int					bUsePacketSyncDb = 0;	 /* Set to 1 in config file if we should use tb_packet_db module to 
										     store/retrieve out of sync packets ronb040307 */

char				sPacketSyncDbFile[TB_MAX_DB_NAME];	/* name of db file holding sync data ronb041307 */		

int					bTankOverSyncDbData = 0;		/* how to resync data or which has priority. ronb041607*/

int  				bPurgePacketSyncDb = 1;	       /* On initialization of the db should we purge all
												   packets(1) or leave alone (0). Note that the db will be
												   purged of records older than the tank file every nPurgeTankFreq
												   Heartbeats.
 													ronb052007 */

/*  I am not sure if SocketTimeoutLength should be volatile.  It should only
    be modified by the main thread, but it is read in other threads.  I am
    leaving it as a normal global int, but based on the problems that Alex
    had, I would be cautious of it.  DavidK
*/


/* Server thread stuff
**********************/
#define THREAD_STACK  8096
static unsigned int *  tidSockSrv;       /* id of socket-serving threads */
static unsigned int   tidServerMgr;                     /* id of dispatcher of socket-serving threads */
ServerThreadInfoStruct * ServerThreadInfo;
ServerThreadInfoStruct * ServerThreadInfoPrevious;

thr_ret ServerThread( void*  );
thr_ret ServerMgr( void*  );


/* Message stacker thread stuff
*******************************/
thr_ret MessageStacker( void * );
int	MessageStackerStatus;				/* as above, but for message stacking thread */
static unsigned int   tidStacker;
static int     InputQueueLen;   /* max messages in input buffer */
QUEUE OutQueue;                 /* from queue.h, queue.c; sets up linked */
                                /*   list via malloc and free */
static int QueueHighWaterMark,QueueLowWaterMark;  /* For polling queue depth */


/* IndexMgr thread stuff
************************/
thr_ret IndexMgr( void *);
static unsigned int tidIndexMgr;
static mutex_t IndexTerminationMutex;
int terminate=0;   /* Changed to global to server threads: PNL 6/20/00 */
static int QueueReportInterval=30;
/* Heartbeat Stuff
******************/
time_t        timeLastBeat;             /* system time of last heartbeat sent */
static long   HeartBeatInt;             /* seconds between heartbeats */

/* Look up from earthworm.h
 **************************/
unsigned char InstId;         /* local installation id      */
unsigned char TypeHeartBeat;
unsigned char TypeError;
unsigned char TypeWaveform;

/* Other globals
 ***************/
static SHM_INFO       Region;       /* Info structure for shared memory */
volatile        int   nTanks;       /* number of tanks in operation     */
volatile        int   Debug =0;     /* Can be set to one by an optional configuration file command */
static volatile int   MaxMsgSiz =0; /* the largest message expected */
pid_t                 myPid;        /* for restarts by startstop */

/**************************/
/*  Global TANK and TANKList Variables */
TANKList * pConfigTankList;
TANKList * pTANKList;
TANK * pTSPtr;
TANK * pTSTemp;
/**************************/

TBDB_STORE	*pPacketDb =NULL;	/* ponter to TBDB_STORE for temp packet storage*/

unsigned int nPurgeDbFreq = 20;	/* purge db of records older than oldest tank trace_buf
								every nPuregeDbFreq Heartbeat.*/

static char  Text[250];        /* string for log/error messages          */

int serve_trace_c_init();  /* used by serve_trace.c to report it's
                              internal-internal-internal-version.
                              Added by davidk, due to the number of
                              serve trace.c versions being put out.
                           */

int index_util_c_init();   /* same as serve_trace_c_init(), except for
                              index_util.c.
                           */

int wave_serverV_c_init();   /* same as serve_trace_c_init(), except for
                              wave_serverV.c.
                           */
int server_thread_c_init();   /* same as serve_trace_c_init(), except for
                              server_thread.c.
                           */

/* Put this here to avoid having to ifdef a prototype. */
#ifdef _WINNT
BOOL CtrlHandler( DWORD fdwCtrlType )
{
  switch( fdwCtrlType )
  {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
      signal_hdlr(0);
      return(TRUE);

    default:
      return FALSE;
  }
}
#endif /* _WINNT */

/* this next #define removes a degenerate No Heartbeat condition found in 2013 */
#define CHECK_HB_AFTER_NUM_PACKETS 1000		/* after this many packets force a Heartbeat check */
#define PACKET_CHECK_FLOOR 100			/* allow adjustment to Heartbeat check down to this number of packets */

#define WSV_VERSION "5.1.45 - 2014-03-31"

int main( int argc, char **argv )
{
  time_t        timeNow;        /* current system time                      */
  int           result;
  int           i,j;
  char*		     deQMsg;         /* for picking messages from the memory queue   */
  char*         inMsg;          /* naked message from queue after remvoing "t"  */
  MSG_LOGO      logo;           /* of the incoming message                      */
  char*         scrMsg;         /* scratch pad: like figuring out who's nexting */
  long          msgInLen;       /* length of retrieved message                  */
  int           t;              /* running index over tanks.                    */
  int           grabit;         /* silly little flag for extracting unique logos */
  unsigned int  tmpOffset;
  time_t        CurrentTime;
  int           bAbortMainLoop;  /* flag to abort the main loop */
  int           flag;            /* transport ring flag value */
  int           rc;
  int           packet_counter = 0; /* count packets processed and do heartbeat check if more than CHECK_HB_AFTER_NUM_PACKETS */
  int           packet_check_limit = CHECK_HB_AFTER_NUM_PACKETS;

  /* Variables to handle recording of File I/O errors */
  int iFileOffset, iFileBlockSize;
  char * szFileTankName;
  char szFileFunction[15];
  int  bIssueIOStatusError = FALSE;
  unsigned int nPurgeDbCount = 0;
  double PurgeTime = 0.0;
  double LastPurgeTime = 0.0;

  TRACE2_HEADER *trh;	/* paulf addition 1/07/1999 mem alignment bug fix*/

  TracePacket   Tpkt;   /* paulf addition 5/24/2007 another mem alignment fix!
                                        ironic no? */
  timeLastBeat = 0;   /* was unitialized and could have caused heartbeats to fail */

  /* Catch broken socket signals
   ******************************/
#if defined(_SOLARIS) || defined(_LINUX) || defined(_MACOSX)
  (void)sigignore(SIGPIPE);
#endif

  /* Not enough arguments
   ********************/
  if ( argc != 2 )
  {
    fprintf( stderr, "Usage: wave_serverV <configfile>\n" );
    fprintf( stderr, "Version: %s\n", WSV_VERSION );
    exit( -1 );
  }

  /* Initialize name of log-file & open it
     force disk logging until after config
     file is read.  JMP 10/3/2001
    ***************************************/
  logit_init( argv[1], 0, 256, 1 );


  /* debug: set up periodic dump logic
   ************************************/


  /*
     wave_server_V_config() needs to set the following variables
     pConfigTankList: a pointer to a list of tank structs created
     from the config file, including a pointer to the TANK
     file for the waveserver.
     PleaseContinue: a flag indicating weather waveserver should
     go down in flames if a single tank fails, or whether it
     should please continue until an error occurs that renders
     all tanks useless.
  ***********************/
  wave_serverV_config( argv[1] );
  wave_serverV_lookup( );

  /* Reset logging to desired level
   ***************************************/
  logit_init( argv[1], 0, 256, LogSwitch );
  if (Debug > 1)
    logit( "" , "NOTE: This is the post-v5.2 patch to not write the empty tank messages\n");

  logit( "" , "wave_serverV: Read command file <%s>\n", argv[1] );

  /* Give serve_trace.c a chance to init.
     this is probably just a version stamp, but
     who knows?  Also index_util.c
   *********************************************/
  wave_serverV_c_init();
  serve_trace_c_init();
  index_util_c_init();
  server_thread_c_init();


/* Get process ID for heartbeat messages */
  myPid = getpid();
  if( myPid == -1 )
  {
    logit("e","wave_serverV: Cannot get pid. Exiting.\n");
    exit (-1);
  }

  /* initialize packet db ronb 041207*/
  if(bUsePacketSyncDb) {
 	  if(tbdb_open(&pPacketDb, sPacketSyncDbFile)) {
 		 /* delete packets based on ldSyncRetentionTime config setting */
 		  if(bPurgePacketSyncDb) {
			  tbdb_purge_all(pPacketDb);
		  }
 	  }	else {
		  /*error opening packet db*/
		  logit("e","wave_serverV: failed to open packet db.\n");
	  }
   }
 
 /* Attach to transport ring (Must already exist)
   *********************************************/
  tport_attach( &Region, RingKey );

  /* Right now, everything we have is in pConfigTankList, it contains all
   * tank info and filenames from the config file, but no valid file pointers..
   */
     /* Start of new Initialization */
  if((pTANKList=malloc(sizeof(TANKList))) == NULL)
  {
    logit("et","wave_serverV:  malloc() failed for tank list.  Exiting\n");
    exit(-1);
  }

  /* Copy the header information from the ConfigTankList to the Tank Structure
     file list.
   ***********************/
  *pTANKList=*pConfigTankList;

  /* Retrieve the TANK structs from disk */
  if(GetLatestTankStructures(pTANKList) == -1)
  {
    logit("et","Failed in call to GetLatestTankStructures().  Exiting\n");
    exit( -1 );
  }

  /* We now have pTANKList, which is a list of TANK structures
     retrieved from the TANK file(s), and pConfigTankList, a list of TANK
     structures retrieved from the config file.  We need to merge the two.
     Use the tanks in pTANKList to configure the tanks in the pConfigTankList
  */

  time(&CurrentTime);
  /* For each tank in the list retrieved from disk */
  for(pTSPtr=pTANKList->pFirstTS;
      pTSPtr != pTANKList->pFirstTS + (pTANKList->NumOfTS);
      /*Not EOL*/
      pTSPtr= pTSPtr+1  /* move to next struct */)
  {

    /* Configure the tank if it's in the config list, with the info from the
       tank struct file.  ConfigTANK() returns a ptr to the just configured
       tank in the Config Tank List.
    */
    wave_serverV_status( TypeHeartBeat, 0, "" ); /* issue a heartbeat in tank handling */
    if( (pTSTemp=ConfigTANK(pTSPtr,pConfigTankList)) != (TANK *) NULL )
    {
      /* Tank is in the list, so open it, and Index files, update the
         living index, and write a copy of the living index to file.
      */

      if(OpenTankFile(pTSTemp) < 0)
      {
        if(!ReCreateBadTanks) MarkTankAsBad(pTSTemp); else pTSTemp->isConfigured=0;
        continue;
      }
      if(OpenIndexFile(pTSTemp,pConfigTankList->redundantIndexFiles,0) < 0)
      {
        if(!ReCreateBadTanks) MarkTankAsBad(pTSTemp); else pTSTemp->isConfigured=0;
        continue;
      }
      if(BuildLIndex(pTSTemp) < 0)
      {
        if(!ReCreateBadTanks) MarkTankAsBad(pTSTemp); else pTSTemp->isConfigured=0;
        continue;
      }
      if(WriteLIndex(pTSTemp,0,CurrentTime) < 0)
      {
        if(!ReCreateBadTanks) MarkTankAsBad(pTSTemp); else pTSTemp->isConfigured=0;
        continue;
      }
    }
    else
    {
      logit("t","Tank %s,%s,%s,%s found in %s, but not listed in config file\n",
            pTSPtr->sta,pTSPtr->chan,pTSPtr->net,pTSPtr->loc,
            GetRedundantFileName(pTANKList->pTSFile));
    }
  }  /* End for each tank from Tank structures file */

  /* We don't need pTANKList anymore, since we've pilfered anything useful
     in it while configuring pConfigTankList, our new hero */
  if(pTANKList->pFirstTS)
  {
    free(pTANKList->pFirstTS);
  }
  free(pTANKList);

  /* for each tank in the list read from the config file */
  for(pTSPtr=pConfigTankList->pFirstTS;
      pTSPtr != (pConfigTankList->pFirstTS) + (pConfigTankList->NumOfTS);/*!EOL*/
      pTSPtr= pTSPtr+1  /* move to next struct */)
  {
    if(!pTSPtr->isConfigured)
      /* Tank is marked as configured during BuildLIndex() */
    {
      wave_serverV_status( TypeHeartBeat, 0, "" ); /* issue a heartbeat in tank handling */
      if(CreateTankFile(pTSPtr) < 0)
      {
        MarkTankAsBad(pTSPtr);
        continue;
      }
      if(OpenIndexFile(pTSPtr,pConfigTankList->redundantIndexFiles,1) < 0)
      {
        MarkTankAsBad(pTSPtr);
        continue;
      }
      if(BuildLIndex(pTSPtr) < 0)
      {
        MarkTankAsBad(pTSPtr);
        continue;
      }
      if(WriteLIndex(pTSPtr,0,CurrentTime) < 0)
      {
        MarkTankAsBad(pTSPtr);
        continue;
      }
      pTSPtr->isConfigured = 1;
    }  /* End if !Tank Already Configured (for better or worse) */
  }  /* End for tank in configTankList */

  /* Throw out the bad tanks, so that we are not carrying excess baggage.
     If PleaseContinue is not set, and a bad tank is found,
     RemoveBadTanksFromList() will exit() wave_server
  */
  if(RemoveBadTanksFromList(pConfigTankList))
  {
    logit("et","wave_server: RemoveBadTanksFromList() failed.  Exiting\n");
    exit(-1);
  }

  /* Set the nTanks and Tank variables used throughout the program */
  nTanks=pConfigTankList->NumOfTS;
  Tanks=pConfigTankList->pFirstTS;

  /* All tanks now configured
     List of tanks is in pConfigTankList
   **********************************/

  /* Sort tanks using CRTLIB func qsort().
     Davidk 10/5/98
   **********************************/
  qsort(Tanks,nTanks,sizeof(TANK), CompareTankSCNLs);

  /* Turn Socket level debugging On/Off
   **********************************/
  setSocket_ewDebug(SOCKET_ewDebug);

  /* Create one mutex for each tank
    ********************************/
  for(i=0;i<nTanks;i++)
  {
    CreateSpecificMutex(&(Tanks[i].mutex));
  }

  /* Create a mutex fot IndexMgr Termination */
  CreateSpecificMutex(&(IndexTerminationMutex));


  /* Allocate message buffers
  **************************/
  for (i=0;i<nTanks;i++)       /* find the largest record size in any tank */
    if(Tanks[i].recSize > MaxMsgSiz) MaxMsgSiz=Tanks[i].recSize;

  if(Debug > 1)
       logit("","MaxMsgSiz: %d\n",MaxMsgSiz);

  deQMsg = (char*)malloc( MaxMsgSiz+sizeof(int) );
  if ( deQMsg == (char *) NULL )
  {
    logit( "e","wave_serverV: Error allocating inMsg buffer; exiting!\n" );
    exit( -1 );
  }
  inMsg = deQMsg + sizeof(int);   /* recall that the tank number is pasted on the front of the
                                     queued message by the stacker thread */
  scrMsg = (char*)malloc( MaxMsgSiz );
  if ( scrMsg == (char *) NULL )
  {
    logit( "e","wave_serverV: Error allocating scrMsg buffer; exiting!\n" );
    exit( -1 );
  }

  /* Create a mutex for the queue
  ******************************/
  CreateSpecificMutex( &QueueMutex );

  /* Initialize the message queue
  *******************************/
  initqueue ( &OutQueue, (unsigned long)InputQueueLen,(unsigned long)(MaxMsgSiz+sizeof(int)) );

  /* Load array of logos to get
    ****************************/
  /* the deal here is to construct an array of distinct logos to be gotten. The Tank structures
     contain logos for each tank, but many of them are likely the same. No point loading down
     tport with all that */
  for (i=0;i<nTanks;i++)  ServeLogo[i].instid = 0; /* clear  array */
  ServeLogo[0]=Tanks[0].logo; /* hand load the first logo. */
  NLogos=1;  /* Number of entries in ServeLogo */

  for (i=0;i<nTanks;i++)
  {
    grabit=1;
    for(j=0; j<NLogos; j++) /* look at all the logos we have so far */
    {
      if ( Tanks[i].logo.instid == ServeLogo[j].instid &&
           Tanks[i].logo.mod    == ServeLogo[j].mod    &&
           Tanks[i].logo.type   == ServeLogo[j].type     )  /* then it's old hat */
      {
        grabit=0;
      }
    }
    if( grabit == 1)
    {
      ServeLogo[NLogos++]=Tanks[i].logo;
      printf("ServeLogo: %d %d %d \n",ServeLogo[NLogos-1].instid,
             ServeLogo[NLogos-1].mod,
             ServeLogo[NLogos-1].type);
    }
  }  /* for < NLogos */
  if(Debug > 1)
  {
    logit("","Acquiring: %d logos:\n",NLogos);
    for(i=0;i<NLogos;i++)
      logit("","Logo:  %d %d %d \n",ServeLogo[i].instid,ServeLogo[i].mod,ServeLogo[i].type);
  }

  /* Set up the signal handler so we can shut down gracefully */
#if defined(_SOLARIS) || defined(_LINUX) || defined(_MACOSX)
  signal(SIGINT, signal_hdlr);     /* <Ctrl-C> interrupt */
  signal(SIGTERM, signal_hdlr);    /* program termination request */
 #ifdef SIGBREAK
  signal(SIGBREAK, signal_hdlr);   /* keyboard break */
 #endif  /* SIGBREAK */
  signal(SIGABRT, signal_hdlr);    /* abnormal termination */
#else 
#ifdef _WINNT 
  SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ); 
#endif /* _WINNT */
#endif /* _SOLARIS */

  /* Create a server thread info array before starting any threads.  The info array
     information is set in the serverthread and servermgr, but is read by the
     indexmgr thread.  */
  ServerThreadInfo=(ServerThreadInfoStruct *)
    malloc(MaxServerThreads * sizeof(ServerThreadInfoStruct));
  memset(ServerThreadInfo,0,MaxServerThreads * sizeof(ServerThreadInfoStruct));
  /* Create a previous array for comparisons done by ReportServerThreadStatus() */
  ServerThreadInfoPrevious=(ServerThreadInfoStruct *)
    malloc(MaxServerThreads * sizeof(ServerThreadInfoStruct));
  memset(ServerThreadInfoPrevious,0,MaxServerThreads * sizeof(ServerThreadInfoStruct));


  /* Start the Server Manager Thread */
  /***********************************/
  if (StartThread(ServerMgr, (unsigned)THREAD_STACK, &tidServerMgr) == -1)
  {
    logit("e", "wave_serverV: error starting ServerMgr. Exiting.\n");
    exit(-1);
  }


  /* Start the Index Manager Thread */
  /***********************************/
  if (StartThread(IndexMgr, (unsigned)THREAD_STACK, &tidIndexMgr) == -1)
  {
    logit("e", "wave_serverV: error starting IndexMgr. Exiting.\n");
    exit(-1);
  }


  /* Start the message stacking Thread */
  /*************************************/

  MessageStackerStatus =-1; /* it should announce life to us */
  if (StartThread(MessageStacker, (unsigned)THREAD_STACK, &tidStacker) == -1)
  {
    logit("e", "wave_serverV: error starting MessageStacker. Exiting.\n");
    exit(-1);
  }

  /* Initialize bAbortMainLoop flag to FALSE */
  /*******************************************/
  bAbortMainLoop = FALSE;

  /* Working loop: get messages from ring, and plop into tank
   ***********************************************************/
  wave_serverV_status( TypeHeartBeat, 0, "" ); /* issue a heartbeat before entering the main loop */
  while(!terminate)
    /***********************************
      start of loop over messages ***/
  {

    /* Get message from queue
     *************************/
    RequestSpecificMutex( &QueueMutex );
    result=dequeue( &OutQueue, deQMsg, &msgInLen, &logo);
    if(QueueLowWaterMark > OutQueue.NumOfElements)
      QueueLowWaterMark = OutQueue.NumOfElements;
    ReleaseSpecificMutex( &QueueMutex );

    /* See if it's time to stop */
    flag = tport_getflag(&Region);
    if( flag == TERMINATE  ||  flag == myPid )  break;

    if( result < 0  || packet_counter > packet_check_limit ) /* -1 means empty queue */
    {
      /* Send wave_server's heartbeat */
      if( time(&timeNow)-timeLastBeat >= HeartBeatInt )
      {
        if ( packet_counter >= packet_check_limit  && packet_check_limit > PACKET_CHECK_FLOOR ) 
	{
		/* we landed here because of packet check, not dequeue() returning empty result */
		packet_check_limit = packet_check_limit/2; /* lower the check limit */
		if ( packet_check_limit < PACKET_CHECK_FLOOR) packet_check_limit = PACKET_CHECK_FLOOR;
		if (Debug) logit("et", "Debug: wave_serverV packet count check limit for HB lowered to %d\n", packet_check_limit);
	}
        timeLastBeat = timeNow;
        wave_serverV_status( TypeHeartBeat, 0, "" );

		/* check for db purge if using db at all*/
		if(bUsePacketSyncDb && !(++nPurgeDbCount % nPurgeDbFreq)) {
			/* get oldest tank record*/
			PurgeTime = IndexOldest(&Tanks[0])->tStart;
			for(i=1; i < nTanks; i++) {
				if(IndexOldest(&Tanks[i])->tStart < PurgeTime) {
					PurgeTime = IndexOldest(&Tanks[i])->tStart;
				}
			}
			/* if needs purging then purge*/
			if(LastPurgeTime != PurgeTime) {
				tbdb_purge_prior_packets(pPacketDb, PurgeTime);
				LastPurgeTime = PurgeTime;
			}
		}
      }
      packet_counter = 0;

      sleep_ew(100);
      continue;
    }
    packet_counter++;

    /* Extract the tank number
     *************************/
    /* Recall, it was pasted as an int on the front of the message by the MessageStacker thread */
    t = *((int*)deQMsg);
    if(Debug > 1)
       logit("","From queue: msg of %ld bytes for tank %d\n",msgInLen-sizeof(int),t);

    msgInLen = msgInLen -sizeof(int);  /* correct message length */


    /* Swap the message to local notation AND MEMORY ALIGN!!!
     *************************************/
    memcpy((void *)&Tpkt, (void *)inMsg, msgInLen);
    trh = (TRACE2_HEADER *) &Tpkt;

    /*  from here on in, only use Tpkt to refer to the msg,
        or trh to the header, this is mem aligned
        *************************************************/
    rc = WaveMsg2MakeLocal( trh );

    if(rc < 0)
    {
      logit("et","WARNING: WaveMsg2MakeLocal() rejected tracebuf.  Discarding: (%s.%s.%s.%s)\n",
        trh->sta, trh->chan, trh->net, trh->loc);
      logit("e", "\t%.2f - %.2f nsamp=%d samprate=%6.2f endtime=%.2f. datatype=[%s]\n",
        trh->starttime, trh->endtime, trh->nsamp, trh->samprate, 
        IndexYoungest(&Tanks[t])->tEnd, trh->datatype);
      goto ReleaseMutex;
    }

    /* Grab the tank file
     ********************/
    RequestSpecificMutex( &(Tanks[t].mutex) );

    if(!TraceBufIsValid(&Tanks[t],trh))
    {
	logit("et","WARNING: Tracebuf fails validity check.  Discarding: (%s.%s.%s.%s)\n",
	      trh->sta, trh->chan, trh->net, trh->loc);
	logit("e", "\tpacket start=%.2f end=%.2f nsamp=%d samprate=%6.2f Tank-endtime=%.2f. datatype=[%s]\n",
	      trh->starttime, trh->endtime, trh->nsamp, trh->samprate,
	      IndexYoungest(&Tanks[t])->tEnd, trh->datatype);
        if (IndexYoungest(&Tanks[t])->tEnd > trh->starttime) 
        {
		char tank_time_string[24];
		char packet_start_time_string[24];
		datestr23(IndexYoungest(&Tanks[t])->tEnd, tank_time_string, 24);
		datestr23(trh->starttime, packet_start_time_string, 24);
 		logit("e", "\tpacket's start (%s) is before end of tank's last good packet received time (%s)\n", packet_start_time_string, tank_time_string);
        }
	goto ReleaseMutex;
    }

	/* ronb 040507 - check if incoming tracebuf overlaps tank.
 	if it does then save overlapped chunk and write portion that
 	doesn't overlap to tank*/
 	if((trh->starttime < IndexYoungest(&Tanks[t])->tEnd) && bUsePacketSyncDb) {
 
		/* tracebuf partially overlaps. write to db.*/
 		logit("e", "WARNING: Trace %s.%s.%s.%s at %.2f overlaps tankdata, saving to db.\n", 
			trh->starttime, trh->sta, trh->chan, trh->net, trh->loc);
 		
		/* currently write entire trace buffer to packet db.*/
 		if(tbdb_put_packet(pPacketDb, trh, inMsg, msgInLen) != TBDB_OK) {
			logit("e", "ERROR: failed to write async packet to db\n");	
 		}
		goto ReleaseMutex;
 	}

	 /* Seek to insertion point
     ********************/
    if ( fseek( Tanks[t].tfp, Tanks[t].inPtOffset, SEEK_SET ) != 0 )  /* move to insertion point */
    {
      logit( "et", "wave_serverV: Error on fseek in tank [%s].\n",Tanks[t].tankName );
      strncpy(szFileFunction,"fseek",sizeof(szFileFunction));
      szFileFunction[sizeof(szFileFunction)-1]= '\0';
      iFileOffset     = Tanks[t].inPtOffset;
      iFileBlockSize  = 0;
      szFileTankName  = Tanks[t].tankName;   /* don't copy just point */
      if(bAbortOnSingleTankFailure)
        goto abortAndReleaseMutex;
      else
        goto ReleaseMutex;
    }
    /* Be sure message is not too big */
    if ( msgInLen > Tanks[t].recSize )
    {
      sprintf( Text, "tank %s.%s.%s.%s msg too big (%ld > %ld).\n",
	       Tanks[t].sta, Tanks[t].chan, Tanks[t].net, Tanks[t].loc,
	       msgInLen, Tanks[t].recSize);
      wave_serverV_status(TypeError, ERR_TOOBIG, Text);
      goto ReleaseMutex;
    }
    /* Write message to Tank file */
    if ( (long) fwrite( (void*)&Tpkt, Tanks[t].recSize, 1, Tanks[t].tfp ) != 1 )
    {               /* NOTE: we always write a FULL record */
      logit( "et", "wave_serverV: Error writing to tank %s; exiting!\n",Tanks[t].tankName );
      strncpy(szFileFunction,"fwrite",sizeof(szFileFunction));
      szFileFunction[sizeof(szFileFunction)-1]= '\0';
      iFileOffset     = Tanks[t].inPtOffset;
      iFileBlockSize  = Tanks[t].recSize;
      szFileTankName  = Tanks[t].tankName;   /* don't copy just point.  The original should be
                                                around for a while, and we aren't modifying */
      bIssueIOStatusError = TRUE;
      if(bAbortOnSingleTankFailure)
        goto abortAndReleaseMutex;
      else
        goto ReleaseMutex;
    }
    /* Update the insertion point
     *****************************/
    tmpOffset=Tanks[t].inPtOffset;
    if ( ftell(Tanks[t].tfp) != Tanks[t].inPtOffset + Tanks[t].recSize )
    {
      logit("et"," wave_serverV: bad offset %ld from ftell updating insertion point on %s\n",
            Tanks[t].inPtOffset,Tanks[t].tankName);
      logit("et"," wave_serverV:  ftell(Tanks[t].tfp): %ld\n",  ftell(Tanks[t].tfp));
      strncpy(szFileFunction,"ftell",sizeof(szFileFunction));
      szFileFunction[sizeof(szFileFunction)-1]= '\0';
      iFileOffset     = Tanks[t].inPtOffset;
      iFileBlockSize  = Tanks[t].recSize;
      szFileTankName  = Tanks[t].tankName;   /* don't copy just point */
      if(bAbortOnSingleTankFailure)
        goto abortAndReleaseMutex;
      else
        goto ReleaseMutex;
    }
    Tanks[t].inPtOffset = Tanks[t].inPtOffset + Tanks[t].recSize;

    /* Check for wrap of tank
     *****************************/
    /* Davidk 4/9/98, changed this comparison from > to >=, with the mindset
       that once the offset point hits the tank length, it should not write anymore
       records, it should instead wrap back to the beginning of the tank.  If it waits
       until the offset is > than the tanksize, then if everything is on the up and up,
       it should write 1 record passed the logical end of the tank, because the last
       record will be written starting at inPtOffset == tanksize, since tanksize is
       a multiple of record size.  Records should be written at (0,+rsz,0+2*rsz,..,(tsz/rsz)*rsz),
       where the last write in the sequence, (tsz/rsz)*rsz = tsz.
    */
    if ( Tanks[t].inPtOffset >= Tanks[t].tankSize )
    {
      Tanks[t].inPtOffset=0;
      Tanks[t].firstPass=0; /* we've been here before */
    }

    if(UpdateIndex(&Tanks[t],trh,tmpOffset, TRUE))
    {
      strncpy(szFileFunction,"UpdateIndex",sizeof(szFileFunction));
      szFileFunction[sizeof(szFileFunction)-1]= '\0';
      iFileOffset     = Tanks[t].inPtOffset;
      iFileBlockSize  = Tanks[t].recSize;
      szFileTankName  = Tanks[t].tankName;   /* don't copy just point */
      if(bAbortOnSingleTankFailure)
        goto abortAndReleaseMutex;
      else
        goto ReleaseMutex;
    }

    ReleaseSpecificMutex( &(Tanks[t].mutex) );
    continue;

  abortAndReleaseMutex:
    bAbortMainLoop=1;

  ReleaseMutex:
    ReleaseSpecificMutex( &(Tanks[t].mutex) );
    if(bIssueIOStatusError)
    {
      IssueIOStatusError(szFileFunction,iFileOffset,iFileBlockSize,szFileTankName);
      bIssueIOStatusError = FALSE;
    }

    if(bAbortMainLoop)
    {
      break;
    }
  } /*** end of working loop over messages ***/



  /* We're here either due to some error, or we're supposed to shut down.
   **************************************************************************/
  logit ("et","wave_serverV: Shutdown initiated.\n");

  /* Tell the other threads that it is time to shutdown */
  terminate=1;

  for (i=0;i<nTanks;i++)
  {
    if (Debug > 1)
       logit("et","wave_serverV: Closing tank file %s.\n",Tanks[i].tankName);

    if( fclose(Tanks[i].tfp) != 0)
      logit(""," Error on fclose on tank file %s\n", Tanks[i].tankName);
    /* End of comment */
  }

  /* At startup, the IndexMgr thread grabs the IndexTerminationMutex,
      periodically, it checks to terminate, to see if it is 1, meaning
      it should shutdown.  Upon seeing the terminate flag, it writes
      the current indexes to disk, and then writes the TANK structures
      to disk.  Then it closes all file pointers, and finally releases
      the IndexTerminationMutex, to indicate to the main thread that it
      has completed.
   */

  if (Debug > 2)
     logit("et","wave_serverV:main(): Waiting for IndexTerminationMutex\n");

  RequestSpecificMutex(&IndexTerminationMutex);

   /* Just to be clean, release the mutex */
  if (Debug > 2)
     logit("et","wave_serverV:main(): Got IndexTerminationMutex\n");

  ReleaseSpecificMutex(&IndexTerminationMutex);

  if (Debug > 2)
     logit("et","wave_serverV:main(): Released IndexTerminationMutex\n");

  /* It is now OK to terminate */

  if (Debug > 1)
    logit("et","wave_server: Detaching from shared memory.\n");

 
  tport_detach( &Region );
  
  /* close packet db if its open. ronb041207
 	NOTE: this should be null if bUsePacketSyncDb = 0*/
   if(pPacketDb) {
 		tbdb_close(&pPacketDb);
 		pPacketDb = NULL;
   }

   logit("et","wave_serverV: Shutdown complete. Indexes saved to disk. Exiting\n");
  exit(0);
}  /* End of main() */
/*---------------------------------------------------------------------------*/


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
  static char        *msg;            /* retrieved message               */
  static int          res;
  static long         msgInLen;       /* size of retrieved message             */
  static MSG_LOGO     reclogo;        /* logo of retrieved message             */
  static int          ret;
  static int          t;
  static int	         sayItOnce=0;
  static int          NumOfTimesQueueLapped=0;
  static time_t       tLastQLComplaint;
  static int          NumLastReportedQueueLapped=0;
  static TANK        *pTempTank;
  unsigned char       seq;


  /* Set time of last queue lapped complaint to now, that way there is an X second
     buffer during startup.
  *******************************************/
  time(&tLastQLComplaint);

  /* Allocate space for input/output messages
   *******************************************/
  /* note that we allocate an int's worth at the start of the message: this
     will contain the tank number this message is headed for. To save doing
     a second set of string compares when the message is pulled from the queue */
  if ( ( msg = (char *) malloc(sizeof(int)+MaxMsgSiz) ) == (char *) NULL )
  {
    logit( "e", "wave_serverV MessageStacker: error allocating msg; exiting!\n" );
    goto error;
  }

  /* Tell the main thread we're ok
   ********************************/
  MessageStackerStatus=0;

  /* Flush the transport ring
   ************************/
  while ( tport_copyfrom( &Region, ServeLogo, (short)NLogos, &reclogo,
                        &msgInLen, msg+sizeof(int), MaxMsgSiz, &seq ) != GET_NONE );

  /* Loop (almost) forever getting messages from transport
  ***********************************************/
  while( !terminate )
  {
    /* Get a message from transport ring
     ************************************/
    res = tport_copyfrom( &Region, ServeLogo, (short)NLogos, &reclogo,
                        &msgInLen, msg+sizeof(int), MaxMsgSiz, &seq );

    if( res == GET_NONE ) {sleep_ew(100); continue;} /*wait if no messages for us */

    /* Check return code; report errors
     ***********************************/
    if( res != GET_OK )
    {
      if( res==GET_TOOBIG )
      {
        if(sayItOnce==0)
        {
          sprintf( Text, "tport msg[%ld] i%d m%d t%d too big. Max is %d",
                   msgInLen, (int) reclogo.instid, (int) reclogo.mod,
                   (int)reclogo.type, MaxMsgSiz );
          wave_serverV_status( TypeError, ERR_TOOBIG, Text );
          sayItOnce =1;
        }
        continue;
      }
      else if( res==GET_MISS_LAPPED )
      {
        sprintf( Text, "missed msgs from i%d m%d t%d sq%d in %s (overwritten)",
                 (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                 (int) seq, RingName );
        wave_serverV_status( TypeError, ERR_MISSMSG, Text );
      }
      else if( res==GET_MISS_SEQGAP )
      {
        sprintf( Text, "saw sequence gap before i%d m%d t%d sq%d in %s",
                 (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                 (int) seq, RingName );
        wave_serverV_status( TypeError, ERR_MISSMSG, Text );
      }
      else if( res==GET_NOTRACK )
      {
        sprintf( Text, "no tracking for logo i%d m%d t%d in %s",
                 (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                 RingName );
        wave_serverV_status( TypeError, ERR_NOTRACK, Text );
      }
    }  /* end if res ! GET_OK */

    /* See if we want this SCN name
     *******************************/

    pTempTank=FindSCNL(Tanks,nTanks,
		       ((TRACE2_HEADER*)(msg+sizeof(int)))->sta,
		       ((TRACE2_HEADER*)(msg+sizeof(int)))->chan,
		       ((TRACE2_HEADER*)(msg+sizeof(int)))->net,
		       ((TRACE2_HEADER*)(msg+sizeof(int)))->loc);
    if(pTempTank)
    {
      t = (int)(pTempTank-Tanks);  /* both are TANK *'s so sizeof(TANK) is
                                      already factored in */
    }
    else
    {
      continue;
    }


    /* This message is headed for Tanks "t" */
    /***************************************/
    /* stick the tank number as an int at the front of the message */
    *((int*)msg) = t;
    RequestSpecificMutex( &QueueMutex );
    ret=enqueue( &OutQueue, msg, msgInLen+sizeof(int), reclogo );
    if(QueueHighWaterMark < OutQueue.NumOfElements)
      QueueHighWaterMark = OutQueue.NumOfElements;
    ReleaseSpecificMutex( &QueueMutex );
    if ( ret!= 0 )
    {
      if (ret==-2)  /* Serious: quit */
      {
        sprintf(Text,"internal queue error. Terminating.");
        wave_serverV_status( TypeError, ERR_QUEUE, Text );
        goto error;
      }
      if (ret==-1)
      {
        sprintf(Text,"queue cannot allocate memory. Lost message.");
        wave_serverV_status( TypeError, ERR_QUEUE, Text );
        continue;
      }
      if (ret==-3)
      {
        /* sprintf(Text,"Circular queue lapped. Message lost.");
           wave_serverV_status( TypeError, ERR_QUEUE, Text );
        */
        /* Queue is lapped too often to be logged to screen.  Log
           circular queue laps to logfile.  Maybe queue laps should
           not be logged at all.
        */

        /* What we want to do is to selectively log queue laps to the screen and statmgr.
           The selection that we want to log is, the first occurance of queue laps, and
           the number of queue laps over X seconds, as long as the number is > 1.  To do
           this, we need to keep a running total of queue lapped messages, a timer indicating
           the last time we complained, and a config file value that indicates how often to
           complain.
        */
        NumOfTimesQueueLapped++;
        if(time(NULL) - tLastQLComplaint >= SecondsBetweenQueueErrorReports)
        {
          /* Complain to stat_mgr and to screen */
          sprintf(Text,"Circular queue lapped.  %d messages lost.",
                  NumOfTimesQueueLapped-NumLastReportedQueueLapped);
          NumLastReportedQueueLapped=NumOfTimesQueueLapped;
          wave_serverV_status( TypeError, ERR_QUEUE, Text );
          time(&tLastQLComplaint);
        }

        continue;
      }
    }

    if(Debug > 1)
       logit("", "queued msg len:%ld for tank %d\n",msgInLen,*((int*)msg) );
  }
  /* we're quitting
   *****************/
 error:
  MessageStackerStatus=-1; /* file a complaint to the main thread */
  KillSelfThread(); /* main thread will restart us */
  return THR_NULL_RET;
}


/*---------------------------------------------------------------------------*/
/************************** ServerMgr Thread *******************************
                Sets up the passive socket and listens for connect requests.
                Starts Server threads, gives them the active sockets, and
                wishes them luck. Keeps track of active threads, and does not
                start too many threads.
******************************************************************************/
thr_ret ServerMgr( void *dummy )
{
  int                  on = 1;
  int                  clientLen;
#ifdef TO_BE_USED_IN_FUTURE
  char                 sysname[40];                    /* system name; filled by getsysname_ew  */
#endif
  typedef char char16[16];
  struct sockaddr_in   skt;
  struct sockaddr_in * client;
  int                  freeThrd;
  int                  i;



#define ACCEPTABLE_TIME_TO_WAIT_FOR_SERVER_THREAD_TO_START 15
#ifndef SocketSysInit_after_gethostbyaddr

  if (Debug > 2)
     logit("et","wave_serverV: SocketSysInit...\n");

  /* Initialize the socket system
   ******************************/
  SocketSysInit();

#endif

  if (Debug > 2)
    logit("", "Wave_serverV: Beginning ServerMgr.\n");


  /* Allocate data for variables based on MaxServerThreads, which is defined
     in the config file, and meta-initialize any data neccessary.
  ******************************************************************/
  tidSockSrv=(unsigned int *) malloc(MaxServerThreads * sizeof(int));

  /*ActiveSocket=(SOCKET *) malloc(MaxServerThreads * sizeof(int));*/
  client_ip=malloc(MaxServerThreads * sizeof(char)* 16);
  client=(struct sockaddr_in *)malloc(MaxServerThreads * sizeof(struct sockaddr_in));

  /* End of malloc and initialization.
   *******************************************************************/

  /* Initialize all ServerThread Info
   **************************************/
  for(i=0; i< MaxServerThreads; i++)
  {
    ServerThreadInfo[i].Status=SERVER_THREAD_IDLE; /* Set status to idle */
    ServerThreadInfo[i].ActiveSocket=0; /* Set all Sockets to NULL */
    ServerThreadInfo[i].ClientsAccepted=0;
    ServerThreadInfo[i].ClientsProcessed=0;
    ServerThreadInfo[i].RequestsProcessed=0;
    ServerThreadInfo[i].Errors=0;
  }

  /* 2003.04.14 dbh -- removed inet_addr() and gethostbyaddr() here */

  /************************/
 HeavyRestart:
  /************************/
  /* Come here after something broke the passive socket. This should not happen, but it does.
     In that case, we're going to re-initialize the socket system, probably drop all clients,
     but start the service all over again.
  */
  if (Debug > 2)
    logit("et","wave_serverV: (Re)starting socket system\n");

#ifdef SocketSysInit_after_gethostbyaddr

  if (Debug > 2)
     logit("et","wave_serverV: SocketSysInit...\n");

  /* Initialize the socket system
   ******************************/
  SocketSysInit();

#endif

  if ( ( PassiveSocket = socket_ew( PF_INET, SOCK_STREAM, 0) ) == -1 )
  {
    logit( "et", "wave_serverV: Error opening socket. Exiting.\n" );
    exit(-1);
  }

  /* Fill in server's socket address structure
   *******************************************/
  memset( (char *) &skt, '\0', sizeof(skt) );
  /* 2003.04.14 dbh -- formerly used memcpy to set skt.sin_addr from gethostbyaddr() result */

#if defined(_LINUX) || defined(_MACOSX)
  /* the S_un substruct is not something linux knows about */
  if ((int)(skt.sin_addr.s_addr = inet_addr(ServerIPAdr)) == INADDR_NONE)
#else
  if ((int)(skt.sin_addr.S_un.S_addr = inet_addr(ServerIPAdr)) == INADDR_NONE)
#endif
  {
      logit( "e", "wave_serverV: inet_addr failed for ServerIPAdr <%s>; exiting!\n",ServerIPAdr );
      exit( -1 );
  }

  skt.sin_family = AF_INET;
  skt.sin_port   = htons( (short)ServerPort );

  /* Allows the server to be stopped and restarted
   *********************************************/
  on=1;
  if(setsockopt( PassiveSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(char *) )!=0)
  {
    logit( "et", "wave_serverV: Error on setsockopt. Exiting.\n" );
    perror("Export_generic setsockopt");
    closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
    goto HeavyRestart;
  }

  /* Bind socket to a name
   ************************/
  if ( bind_ew( PassiveSocket, (struct sockaddr *) &skt, sizeof(skt)) )
  {
    sprintf(Text,"wave_serverV: error binding socket; Exiting.\n");
    perror("wave_serverV bind error");
    logit("et","%s",Text );
    closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
    exit (-1);
  }
  /* and start listening */
  if ( listen_ew( PassiveSocket, MaxServerThreads) )
  {
    logit("et","wave_serverV: socket listen error; exiting!\n" );
    closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
    exit(-1);
  }

  while (!terminate)
  {
    /* Prepare for connect requests
     *******************************/
    /* Find an idle thread
     **********************/
    while(1)
    {
      for(freeThrd=0; freeThrd<MaxServerThreads; freeThrd++)
      {
        if (Debug > 2)
          logit("","ServerMgr: ServerThreadStatus[%d]=%d\n",
                freeThrd,ServerThreadInfo[freeThrd].Status);

        if (ServerThreadInfo[freeThrd].Status == SERVER_THREAD_IDLE
            || ServerThreadInfo[freeThrd].Status == SERVER_THREAD_COMPLETED
            || ServerThreadInfo[freeThrd].Status == SERVER_THREAD_ERROR)
          goto gotthrd;
        /* As long as the thread is not busy, grab it. */
      }
      logit("et","Cannot accept client: out of server threads. Waiting.\n");
      sleep_ew(WAIT_FOR_SERVER_THREAD); /* get in line and wait */
    }
  gotthrd:

    /*  First Check thread state */
    switch(ServerThreadInfo[freeThrd].Status)
    {
    case SERVER_THREAD_COMPLETED:
      {
        /* Everything is OK, the thread has completed a previous task */
        ServerThreadInfo[freeThrd].Status=SERVER_THREAD_IDLE;
      }  /* fall through to next case... */
    case SERVER_THREAD_IDLE:
      {
        /* The thread is ready, break */
        break;
      }
    case SERVER_THREAD_ERROR:
      {
        /* The thread died on a previous task.  Do whatever cleanup
           that is neccessary.  Proclaim senseless thread death!
        */
        logit("et","Wave_serverV:Server thread %d was found to be dead.  Recycling.\n\t%s\n",
              freeThrd,"This is a VERY BAD THING!");
        ServerThreadInfo[freeThrd].Status=SERVER_THREAD_IDLE;
        break;
      }
    default:
      {
        /* The thread is still busy.  We shouldn't be here,
           self-destruct
        */
        logit("et","Wave_serverV:Thread %d was selected as free, "
              "but status shows that it is still busy.\n", freeThrd);
        goto HeavyRestart;
      }
    }

    /* Accept a connection (blocking)
     *********************************/
    clientLen = sizeof( client[freeThrd] );

    if (Debug > 1)
      logit( "","wave_serverV: Waiting for new connection for thread %d.\n",
             freeThrd );

    if( (ServerThreadInfo[freeThrd].ActiveSocket =
         accept_ew( PassiveSocket, (struct sockaddr*) &client[freeThrd],
                    &clientLen, -1 /*SocketTimeoutLength*/) ) == INVALID_SOCKET )
    {
      logit("et","wave_serverV: error on accept. Exiting\n" );
      closesocket_ew( ServerThreadInfo[freeThrd].ActiveSocket,
                      SOCKET_CLOSE_IMMEDIATELY_EW );
      goto HeavyRestart;
    }
    strcpy( client_ip[freeThrd], inet_ntoa(client[freeThrd].sin_addr) );

    if (Debug > 1)
      logit("et", "wave_serverV: Connection accepted from IP address %s\n", 
            client_ip[freeThrd] );

    /* Start the Server thread
     **************************/
    if (Debug > 1)
      logit("","Starting ServerThread %d \n",freeThrd);

    if ( StartThreadWithArg(  ServerThread, &freeThrd, (unsigned)THREAD_STACK, &tidSockSrv[freeThrd] )  == -1 )
    {
      logit( "e", "wave_serverV: Error starting  SocketSender thread. Exiting.\n" );
      closesocket_ew( ServerThreadInfo[freeThrd].ActiveSocket,
                      SOCKET_CLOSE_IMMEDIATELY_EW );
      goto HeavyRestart;
    }
    /* Wait for it to wake up */
    /**************************/
    /* otherwise we'll see it as idle on the next pass through this loop */
    if (Debug > 1)
      logit("","ServerMgr waiting for thread %d to come alive\n",freeThrd);


    { /* Begin of block to check for thread start status */
      time_t WaitForThreadStartTime, WaitForThreadCurrTime;
      time(&WaitForThreadStartTime);
      while( ServerThreadInfo[freeThrd].Status == SERVER_THREAD_IDLE
             && (WaitForThreadStartTime + ACCEPTABLE_TIME_TO_WAIT_FOR_SERVER_THREAD_TO_START > time(&WaitForThreadCurrTime)) )
      {
        sleep_ew(1);
      }
      if(WaitForThreadStartTime + ACCEPTABLE_TIME_TO_WAIT_FOR_SERVER_THREAD_TO_START
         < time(&WaitForThreadCurrTime))
      {
        logit("et","Thread %d never went to busy.  Marking thread as dead, and continuing.\n",freeThrd);
        ServerThreadInfo[freeThrd].Status = SERVER_THREAD_ERROR;
        /* We now have this confused thread running around out there,
           detached from normal life.  We should probably do something
           besides wave goodbye.
        */
      }
    }  /* End of block to check for thread start status */
  }
  closesocket_ew(PassiveSocket, SOCKET_CLOSE_GRACEFULLY_EW);
  return THR_NULL_RET;
}                    /*** end of ServerMgr thread ***/

/*---------------------------------------------------------------------------*/


/************************** IndexMgr Thread *******************************
Periodically writes the current TANK structures, and memory indexes to disk.
******************************************************************************/
thr_ret IndexMgr( void *dummy )
{


  TANKList * pMyTANKList;
  unsigned int i;
  time_t CurrentTime, LastTANKListWrite, LastTime, LastQueueReport;
  time_t * pFirstTimeStamp, * pSecondTimeStamp;
  unsigned int TANKsSize,TANKListBufferSize;
  char * pTANKsInBuffer, *pTANKListBuffer;
  int * pNumOfTANKs;

  /* Create a buffer to store our personal snapshot of the
     tank buffers.  The idea is to frequently take snapshots
     of the TANKs and save them to disk without slowing down
     the main threads tank update mechanism
  */
  if((pMyTANKList=CreatePersonalTANKBuffer(pConfigTankList)) == NULL)
  {
    logit("et","wave_serverV:IndexMgr():  failed to create tank buffer.  Exiting\n");
    exit(-1);
  }

  RequestSpecificMutex(&IndexTerminationMutex);

  /* Before starting, write a current copy of the TANKs to
     the TANKs file.
  */
  time(&LastTANKListWrite);
  /* Note: GetTANKSs uses tank mutexes here (last argument is TRUE). If
   * we get the terminate flag in the middle of this call to GetTANKS,
   * it is possible that a Server Thread could get killed holding a
   * tank mutex. This would block GetTANKS forever until wave_serverV gets
   * killed by startstop. PNL 1/10/00 */
  if(GetTANKs(Tanks,pMyTANKList->pFirstTS,pMyTANKList->NumOfTS,TRUE))
  {
    logit("et","wave_serverV:IndexMgr():  failed to get copy of tank structures.  Exiting\n");
    exit(-1);
  }

  TANKListBufferSize = sizeof(time_t) + sizeof(int)
    + (pMyTANKList->NumOfTS * sizeof(TANK))
    + sizeof(time_t);
  if((pTANKListBuffer=malloc(TANKListBufferSize)) == NULL)
  {
    logit("et","wave_serverV:IndexMgr():  failed to malloc tank list buffer.  Exiting\n");
    exit(-1);
  }

  /* Write the number of tanks to the correct position in the buffer.
     The buffer should look like:
     TimeStamp1 NumOfTanks <TANKs> TimeStamp2
  */
  pNumOfTANKs=(int *)(pTANKListBuffer + sizeof(time_t));
  *pNumOfTANKs=pMyTANKList->NumOfTS;

  /* Create Shortcuts to the TimeStamp and TANKs locations */
  pFirstTimeStamp=(time_t *)pTANKListBuffer;
  pTANKsInBuffer=pTANKListBuffer+sizeof(time_t)+sizeof(int);
  pSecondTimeStamp=(time_t *)(pTANKListBuffer+TANKListBufferSize-sizeof(time_t));
  TANKsSize=pMyTANKList->NumOfTS * sizeof(TANK);

  /*WriteTANKList()*/
  time(pFirstTimeStamp);
  memcpy(pTANKsInBuffer,pMyTANKList->pFirstTS,TANKsSize);
  memcpy(pSecondTimeStamp, pFirstTimeStamp, sizeof(time_t));
  if(WriteRFile(pMyTANKList->pTSFile,0,TANKListBufferSize,pTANKListBuffer,1) != 1)
  {
    logit("et","wave_serverV:IndexMgr():  failed to write tank list to file.  Exiting\n");
    exit(-1);
  }

  CurrentTime=LastTANKListWrite;

  {
    LastQueueReport=CurrentTime;

    if (Debug > 0)
       logit("et","Queue Report: High Mark %d, Low Mark %d\n",
 								QueueHighWaterMark,QueueLowWaterMark);

    QueueLowWaterMark=QueueHighWaterMark;
    QueueHighWaterMark=0;
  }

  while(!terminate)
  {
    LastTime=CurrentTime;

    time(&CurrentTime);
    if(CurrentTime-LastQueueReport >= QueueReportInterval)
    {
      LastQueueReport=CurrentTime;

      if (Debug > 0)
          logit("et","Queue Report: High Mark %d, Low Mark %d\n",
									QueueHighWaterMark,QueueLowWaterMark);

      QueueLowWaterMark=QueueHighWaterMark;
      QueueHighWaterMark=0;

      /* Do A Server Thread Status Report */
      if (Debug > 0)
          ReportServerThreadStatus();
    }
    if(LastTime==CurrentTime)
    {
      sleep_ew(1000);
      continue;
    }
    /* else */
    if(CurrentTime-TankStructUpdate >= LastTANKListWrite)
    {
      LastTANKListWrite=CurrentTime;
      if(GetTANKs(Tanks,pMyTANKList->pFirstTS,pMyTANKList->NumOfTS,TRUE))
      {
        logit("et","wave_serverV:IndexMgr():  failed to get copy of tank structures.  Exiting\n");
        exit(-1);
      }

      /*WriteTANKList()*/
      *pFirstTimeStamp=CurrentTime;
      memcpy(pTANKsInBuffer,pMyTANKList->pFirstTS,TANKsSize);
      *pSecondTimeStamp=*pFirstTimeStamp;
      if(WriteRFile(pMyTANKList->pTSFile,0,TANKListBufferSize,pTANKListBuffer,1) != 1)
      {
        logit("et","wave_serverV:IndexMgr():  failed to write tank list to file.  Exiting\n");
        exit(-1);
      }
    }

    for(i=0;i<(pMyTANKList->NumOfTS);i++)
    {
      if(CurrentTime-IndexUpdate >= Tanks[i].lastIndexWrite)
      {
        if(WriteLIndex(&(Tanks[i]),1,CurrentTime))
        {
          logit("t","WriteLIndex() failed for %s\n", Tanks[i].tankName);
        }
      }
      if(terminate) break;
    }  /* End for loop through all tank indexes */

  }  /* End: while !terminated */

  /* We got terminated.  Save the women, children,
     and indexes first, and then save the TANKList file
  */

  if (Debug > 1)
     logit("t","IndexMgr: Got terminate flag!\n");

  time(&CurrentTime);
  /* Saving indexes
   ********************/
  for(i=0;i<(pMyTANKList->NumOfTS);i++)
  {
    if (Debug > 1)
       logit("t","IndexMgr: Writing Tank %s!\n",Tanks[i].tankName);

    WriteLIndex(&(Tanks[i]),1,CurrentTime);
  }

  /* Getting and Saving TANKList.  Copy it into
     the Tank list buffer, so that we can execute a single write..
     Do NOT use mutexes.
  ********************/
  if (Debug > 2)
    logit("t","IndexMgr: Getting Tanks!\n");

  GetTANKs(Tanks,pMyTANKList->pFirstTS,pMyTANKList->NumOfTS,FALSE);
  /*WriteTANKList()*/

  time(pFirstTimeStamp);
  memcpy(pTANKsInBuffer,pMyTANKList->pFirstTS,TANKsSize);
  memcpy(pSecondTimeStamp, pFirstTimeStamp, sizeof(time_t));

  if (Debug > 2)
     logit("t","IndexMgr: Writing TANKlist to file!\n");

  WriteRFile(pMyTANKList->pTSFile,0,TANKListBufferSize,pTANKListBuffer,1);

  /* HandleAllFinishingTouches
   **********************/
  if (Debug > 2)
     logit("t","IndexMgr: Cleaning up!\n");

  CleanupIndexThread(pMyTANKList);

  /* Allow the main thread to proceed
   **********************/
   if (Debug > 2)
     logit("t","IndexMgr: Releasing IndexTerminationMutex!\n");

  ReleaseSpecificMutex(&IndexTerminationMutex);
  return THR_NULL_RET;
}                    /*** end of IndexMgr thread ***/
/*---------------------------------------------------------------------------*/

#define NCOMMAND 13
/***********************************************************************
 * wave_serverV_config()  processes command file using kom.c functions  *
 *                       exits if any errors are encountered           *
 ***********************************************************************/
void wave_serverV_config(char *configfile)
{
  int      ncommand = NCOMMAND; /* # of required commands to process   */
  char     init[NCOMMAND]; /* init flags, one for each required command */
  int      nmiss;      /* number of required commands that were missed   */
  char    *comm;
  char    *str;
  int      nfiles;
  int      success;
  int      i;
  char     TankStructFile[FILENAMELEN],TankStructFile2[FILENAMELEN];
  int      RedundantTankStructFiles=0;
  int      RedundantIndexFiles=0;


  /* Initialization */
  TankStructFile[0]=0;
  TankStructFile2[0]=0;

  SecondsBetweenQueueErrorReports=60;  /* default value of 60 sec. */

  /* Set to zero one init flag for each required command
   *****************************************************/
  for( i=0; i<ncommand; i++ )  init[i] = 0;

  /* Open the main configuration file
   **********************************/
  nfiles = k_open( configfile );
  if ( nfiles == 0 ) {
    logit ("e",
             "wave_serverV: Error opening command file <%s>; exiting!\n",
             configfile );
    exit( -1 );
  }

  /* Process all command files
   ***************************/
  while(nfiles > 0)   /* While there are command files open */
  {
    while(k_rd())        /* Read next line from active file  */
    {
      comm = k_str();         /* Get the first token from line */

      /* Ignore blank lines & comments
       *******************************/
      if( !comm )           continue;
      if( comm[0] == '#' )  continue;

      /* Open a nested configuration file
       **********************************/
      if( comm[0] == '@' )
      {
        success = nfiles+1;
        nfiles  = k_open(&comm[1]);
        if ( nfiles != success ) {
          logit ("e",
                   "wave_serverV: Error opening command file <%s>; exiting!\n",
                   &comm[1] );
          exit( -1 );
        }
        continue;
      }

      /* Process anything else as a command
       ************************************/
      /* Read the transport ring name
       ******************************/
      /*0*/    if( k_its( "RingName" ) )
      {
        if ( (str=k_str()) ) {
          strncpy(RingName,str,20);
          if ( (RingKey = GetKey(str)) == -1 ) {
            logit ("e",
                     "wave_serverV: Invalid ring name <%s>; exiting!\n",
                     str );
            exit( -1 );
          }
        }
        init[0] = 1;
      }
      /* Read the log file switch
       **************************/
      /*1*/    else if( k_its( "LogFile" ) )
      {
        LogSwitch = k_int();
        init[1] = 1;
      }
      /* Read wave_server's module id
       ******************************/
      /*2*/    else if( k_its( "MyModuleId" ) )
      {
        if ( (str=k_str()) ) {
          if ( GetModId( str, &MyModId ) != 0 ) {
            logit ("e",
                     "wave_serverV: Invalid module name <%s>; exiting!\n",
                     str );
            exit( -1 );
          }
        }
        init[2] = 1;
      }

      /* Read the port for requests and replies
       ****************************************/
      /*3*/    else if( k_its( "ServerPort" ) )
      {
        ServerPort = k_int();
        init[3] = 1;
      }
      /* Read the Internet address of wave_server machine
       ***************************************************/
      /*4*/    else if( k_its( "ServerIPAdr" ) )
      {
        if ( (str = k_str()))
        {
          if (strlen(str) < IPADDRLEN)
          {
            strcpy( ServerIPAdr, str );
            init[4] = 1;
          }
          else
          {
            logit ("e", "ServerIPAdr name too long, max %d\n", IPADDRLEN-1);
            exit( -1 );
          }
        }

      }
      /* Read heartbeat interval (seconds)
       ***********************************/
      /*5*/    else if( k_its("HeartBeatInt") )
      {
        HeartBeatInt = k_long();
        init[5] = 1;
      }

      /* Read gap threshhold
       ***********************************/
      /*6*/    else if( k_its("GapThresh") )
      {
        GapThresh = k_val();
        init[6] = 1;
      }

      /* Read index update duration
       **************************************/
      /*7*/    else if( k_its("IndexUpdate") )
      {
        IndexUpdate = k_int();
        init[7] = 1;
      }

      /* Read tank struct update duration
       **************************************/
      /*8*/    else if( k_its("TankStructUpdate") )
      {
        TankStructUpdate = k_int();
        init[8] = 1;
      }

      /* Maximum number of messages in outgoing circular buffer
       ********************************************************/
      /*9*/   else if( k_its("InputQueueLen") )
      {
        InputQueueLen = k_long();
        init[9] = 1;
      }

      /* Timeout in milliseconds for IP Socket routines
       ***********************************/
      /*10*/   else if(k_its("SocketTimeout") )
      {
        SocketTimeoutLength = k_int();
        init[10]=1;
      }

      /* Location of TankStruct File
       ***********************************/
      /*11*/   else if(k_its("TankStructFile") )
      {
        if((str=k_str()) != NULL)
        {
          if (strlen(str) < FILENAMELEN)
          {
            strcpy(TankStructFile, str);
            init[11]=1;
          }
          else
          {
            logit ("e", "wave_serverV: TankStructFile name too long, max is %d\n",
                    FILENAMELEN-1);
            exit(-1);
          }
        }
        else
        {
          logit ("e", "wave_serverV: TankStructFile name missing\n");
          exit( -1 );
        }
      }

      /* Read Tank descriptor lines
       ****************************/
      /*12*/    else if( k_its("Tank") )
      {
        if( nTanks == 0)
        {
          Tanks=(TANK *)malloc(MAX_TANKS * sizeof(TANK));
          memset(Tanks,0,MAX_TANKS * sizeof(TANK));
        }

        if( nTanks >= MAX_TANKS)
        {
          logit ("e", "wave_serverV: More than %d Tank descriptor lines. Exit\n",MAX_TANKS);
          exit(-1);
          logit ("e", "wave_serverV: Too many tanks in config file.  Maximum of %d Tanks permitted. Exit\n"
                ,MAX_TANKS);
          exit(-1);

        }
        if((str=k_str()) != NULL)	strncpy(Tanks[nTanks].sta, str, TRACE2_STA_LEN);
        else 	{
          logit ("e", "wave_serverV: bad station name\n");
          exit(-1);
        }
        if((str=k_str()) != NULL)	strncpy(Tanks[nTanks].chan, str, TRACE2_CHAN_LEN);
        else 	{
          logit ("e", "wave_serverV: bad component name\n");
          exit(-1);
        }
        if((str=k_str()) != NULL)	strncpy(Tanks[nTanks].net, str, TRACE2_NET_LEN);
        else 	{
          logit ("e", "wave_serverV: bad network name\n");
          exit(-1);
        }
        if((str=k_str()) != NULL)	strncpy(Tanks[nTanks].loc, str, TRACE2_LOC_LEN);
        else 	{
          logit ("e", "wave_serverV: bad location name\n");
          exit(-1);
        }

        Tanks[nTanks].recSize = k_int(); /* we'll chop the tank into such slots */
        if ( Tanks[nTanks].recSize % 4 != 0 ) {
          logit ("e", "wave_serverV: record size for <%s><%s><%s><%s> not a multiple of 4\n",
		 Tanks[nTanks].sta, Tanks[nTanks].chan, Tanks[nTanks].net,
		 Tanks[nTanks].loc);
          exit(-1);
        }
        if ( Tanks[nTanks].recSize > MAX_TRACEBUF_SIZ ) {
          logit ("e", "wave_serverV: record size for <%s><%s><%s><%s> is larger than MAX_TRACEBUF_SIZ <%d>\n",
		 Tanks[nTanks].sta, Tanks[nTanks].chan, Tanks[nTanks].net,
		 Tanks[nTanks].loc, MAX_TRACEBUF_SIZ );
          exit(-1);
        }

        if( (str=k_str()) )
        {
          if( GetInst( str, &Tanks[nTanks].logo.instid ) != 0 )
          {
            logit ("e", "wave_serverV: Invalid installation name <%s>", str );
            exit( -1 );
          }
        }
        if( (str=k_str()) )
        {
          if( GetModId( str, &Tanks[nTanks].logo.mod ) != 0 )
          {
            logit ("e", "wave_serverV: Invalid module name <%s>", str );
            logit ("e", " in <GetWavesFrom> cmd; exiting!\n" ); exit( -1 );
          }
        }

        if( GetType( "TYPE_TRACEBUF2", &Tanks[nTanks].logo.type ) != 0 )
        {
          logit ("e", "wave_serverV: PANIC: TYPE_TRACEBUF2 not recognized\n" );
          exit( -1 );
        }

        Tanks[nTanks].tankSize = k_int()*1000000 ; /* user gives us tank size in megabytes */
        if(Tanks[nTanks].tankSize > MAX_TANK_SIZE)
        {
          if(bTruncateTanksTo1GBAndContinue)
          {
            logit("e", "wave_serverV: WARNING:  Tanksize for (%s:%s:%s:%s) is too large: %d bytes.\n"
                       "Tank truncated to maximum allowable size: %d bytes.\n",
                  Tanks[nTanks].sta, Tanks[nTanks].chan, Tanks[nTanks].net, Tanks[nTanks].loc,
                  Tanks[nTanks].tankSize,
                  MAX_TANK_SIZE);

             /* limit tank size to largest multiple of recSize that is smaller than MAX_TANK_SIZE */
             Tanks[nTanks].tankSize =   (MAX_TANK_SIZE / Tanks[nTanks].recSize)
                                      * Tanks[nTanks].recSize ;
          }
          else
          {
            logit("et", "wave_serverV: ERROR:  Tanksize for (%s:%s:%s:%s) is too large: %d bytes.\n"
                       "Tanks must be <=  %d bytes.  EXITING!\n",
                  Tanks[nTanks].sta, Tanks[nTanks].chan, Tanks[nTanks].net, Tanks[nTanks].loc,
                  Tanks[nTanks].tankSize,
                  MAX_TANK_SIZE);
            exit(-1);
          }
        }
        else {
             /* RL set tanksize to multiple of recsize to avoid ReadBlockData errors */
             Tanks[nTanks].tankSize = (Tanks[nTanks].tankSize / Tanks[nTanks].recSize) * Tanks[nTanks].recSize;
        }

        Tanks[nTanks].indxMaxChnks = k_int(); /*number of segments, not byte count */
        Tanks[nTanks].nRec = Tanks[nTanks].tankSize / Tanks[nTanks].recSize;  /* how many records can we store */
        Tanks[nTanks].tankSize = Tanks[nTanks].recSize * Tanks[nTanks].nRec;
        Tanks[nTanks].datatype[0]=0;


        /* When do we open FILE Ptrs, maybe in ConfigTank() */

        if((str=k_str()) != NULL)
        {
          if (strlen(str) < MAX_TANK_NAME)
          {
            strncpy(Tanks[nTanks].tankName, str,MAX_TANK_NAME);
          }
          else
          {
            logit ("e", "wave_serverV: tank name too long; max is %d\n",
                    MAX_TANK_NAME-1);
            exit( -1 );
          }
        }
        else
        {
          logit ("e", "wave_serverV: bad tank name\n");
          exit(-1);
        }

        init[12] = 1;
        nTanks++;
      }

      /* Read debug flag - optional
       ****************************/
      else if( k_its("Debug") ){
        Debug =  k_int();
      }

      /* Optional cmd: ClientTimeout */
      else if(k_its("ClientTimeout") ) {
        ClientTimeout = k_int();
        if (ClientTimeout <= 0 && ClientTimeout != -1)
        {
          logit ("e", "wave_serverV: bad ClientTimeout value <%d>, using -1\n",
                  ClientTimeout);
          ClientTimeout = -1;
        }
      }

      /* Optional cmd: Turn on socket debug logging -> BIG LOG FILES!!
       ***********************************/
      else if(k_its("SocketDebug") ) {
        SOCKET_ewDebug = k_int();
      }

      /* Optional cmd: RedundantTankStructFiles
         Should we use redundant files for saving Tank Structs
      ***********************************/
      else if(k_its("RedundantTankStructFiles") )
      {
        RedundantTankStructFiles=k_int();
      }

      /* Optional cmd: RedundantIndexFiles
         Should we use redundant files for saving Indexes
      ***********************************/
      else if(k_its("RedundantIndexFiles") )
      {
        RedundantIndexFiles=k_int();
      }


      /* Optional:  Location of second TankStruct File
         for use only with redundant tank struct files
      ***********************************/
      else if(k_its("TankStructFile2") )
      {
        if((str=k_str()) != NULL)
        {
          if (strlen(str) < FILENAMELEN)
          {
            strcpy(TankStructFile2, str);
          }
          else
          {
            logit ("e", "wave_serverV: TankStructFile2 name too long, max is %d\n",
                    FILENAMELEN-1);
            exit( -1 );
          }
        }
        else
        {
          logit ("e", "wave_serverV: bad TankStructFile2 name\n");
        }
      }

      /* Optional cmd: PleaseContinue
         Should we continue on Tank failures,
         providing that some tanks are still functioning
         (Do your best)
      ***********************************/
      else if(k_its("PleaseContinue") )
      {
        PleaseContinue=k_int();
      }

      /* Optional cmd: MaxMsgSize
         In case trace_buf messages could be larger than any in this config file
      ********************************/
      else if (k_its("MaxMsgSize") )
      {
        MaxMsgSiz=k_int();
      }

      /* Optional cmd: ReCreateBadTanks
         In case the operator desires to have bad tanks re-created
         from scratch.
      ********************************/
      else if (k_its("ReCreateBadTanks") )
      {
        ReCreateBadTanks=k_int();
      }


      /* Optional cmd: SecondsBetweenQueueErrorReports
         To set the minimum period of time between error reports
         to statmgr due to the internal message queue being lapped,
         and thus messages being lost.
      ********************************/
      else if (k_its("SecondsBetweenQueueErrorReports") )
      {
        SecondsBetweenQueueErrorReports=k_int();
      }


      /* Optional cmd: MaxServerThreads
         To set the maximum number of server threads to be created
         to handle client requests.
      ********************************/
      else if (k_its("MaxServerThreads") )
      {
        MaxServerThreads=k_int();
      }


      /* Optional cmd: QueueReportInterval
         To set the minimum number of seconds between
         reports on internal queue high and low water marks.
      ********************************/
      else if (k_its("QueueReportInterval") )
      {
        QueueReportInterval=k_int();
      }


      /* Optional cmd: RedundantIndexFiles
         Should we use redundant files for saving Indexes
      ***********************************/
      else if(k_its("AbortOnSingleTankFailure") )
      {
        bAbortOnSingleTankFailure=k_int();
      }

      /* Optional cmd: RedundantIndexFiles
         Should we use redundant files for saving Indexes
      ***********************************/
      else if(k_its("TruncateTanksTo1GBAndContinue") )
      {
        bTruncateTanksTo1GBAndContinue=TRUE;
      }

 	  /* ronb040307 - Optional cmd: UseSequenceDb
	   Use packet db to store/retrieve out of order Packets
	   ********************************/
	  else if(k_its("UsePacketSyncDb")) {
		bUsePacketSyncDb = k_int();
	  }
 	  
 	  /* get database file name from config file ronb040307*/
 	  else if(k_its("PacketSyncDbFile")) {
 		strncpy(sPacketSyncDbFile, k_str(), TB_MAX_DB_NAME);
	  }

	  /* get priority for out of sync data overlapping tank data ronb041207*/
	  else if(k_its("TankOverSyncDbData")) {
		bTankOverSyncDbData = k_int();
 	  }

	  /* get number of epoch seconds worth of data to retain in
	  database. ronb041207*/
	  else if(k_its("PurgePacketSyncDb")) {
 		bPurgePacketSyncDb = k_int();
 	  }
 
    /* Command is not recognized
       ****************************/
      else
      {
        logit ("e", "wave_serverV: <%s> unknown command in <%s>.\n",
                comm, configfile );
        continue;
      }



      /* See if there were any errors processing the command
       *****************************************************/
      if( k_err() ) {
        logit ("e",
                 "wave_serverV: Bad <%s> command in <%s>; exiting!\n",
                 comm, configfile );
        exit( -1 );
      }
    }
    nfiles = k_close();
  }
  /* After all files are closed, check init flags for missed commands
   ******************************************************************/
  nmiss = 0;
  for ( i=0; i < ncommand; i++ )  if( !init[i] ) nmiss++;
  if ( nmiss ) {
    logit ("e", "wave_serverV: ERROR, no " );
    if ( !init[0] )  logit ("e", "<RingName> "     );
    if ( !init[1] )  logit ("e", "<LogFile> "      );
    if ( !init[2] )  logit ("e", "<MyModuleId> "   );
    if ( !init[3] )  logit ("e", "<ServerPort> "   );
    if ( !init[4] )  logit ("e", "<ServerIPAdr> "  );
    if ( !init[5] )  logit ("e", "<HeartBeatInt> " );
    if ( !init[6] )  logit ("e", "<GapThresh> "    );
    if ( !init[7] )  logit ("e", "<IndexUpdate> "  );
    if ( !init[8] )  logit ("e", "<TankStructUpdate> "   );
    if ( !init[9] )  logit ("e", "<InputQueueLen> "      );
    if ( !init[10])  logit ("e", "<SocketTimeoutLength> ");
    if ( !init[11])  logit ("e", "<TankStructFile> "     );
    if ( !init[12])  logit ("e", "<Tank> "               );
    logit ("e", "command(s) in <%s>; exiting!\n", configfile );
    exit( -1 );
  }

  /* Real quick: define duties of config relative to indexing:
     read in the config parameters.
     When we find the number of tanks, malloc() a list for the tanks, and
     then as the tanks are read in, add each tank to the list.  Call
     InitTankList() to create a TANKFilePtr for the list, to copy the location
     of the tank array, to indicate redundant index or TANKstruct files, to set
     the number of tanks.

     We will set the file ptrs for each tank index when we config each tank
  */

  if(InitTankList(&pConfigTankList,Tanks,
                  RedundantTankStructFiles,
                  RedundantIndexFiles,nTanks,
                  TankStructFile,TankStructFile2) )
  {
    /* This is not good!!, we could not init the list.
       Quit and go home */
    logit ("e", "ERROR:  InitTankList failed!!!\nFailure Terminal, Exiting\n");
    exit(-1);
  }

  /* At this point, we have a working configured tank list, generated from
     the config file, with the exception that no files are open. DK
  */

  return;
}
/*---------------------------------------------------------------------------*/
/***************************************************************************
 *  wave_serverV_lookup( )  Look up important info from earthworm.h tables  *
 ***************************************************************************/
void wave_serverV_lookup( void )
{
  if ( GetLocalInst( &InstId ) != 0 ) {
    fprintf( stderr,
             "wave_serverV: error getting local installation id; exiting!\n" );
    exit( -1 );
  }
  if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
    fprintf( stderr,
             "wave_serverV: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
    exit( -1 );
  }
  if( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
    fprintf( stderr,
             "wave_serverV: Invalid message type <TYPE_ERROR>; exiting!\n" );
    exit( -1 );
  }
  if( GetType( "TYPE_TRACEBUF2", &TypeWaveform ) != 0 ) {
    fprintf( stderr,
             "wave_serverV: Invalid message type <TYPE_TRACEBUF2>; exiting!\n" );
    exit( -1 );
  }
  return;
}
/*---------------------------------------------------------------------------*/

/******************************************************************************
 * wave_serverV_status() builds a heartbeat or error message & puts it into    *
 *                     shared memory.  Writes errors to log file & screen.    *
 ******************************************************************************/
void wave_serverV_status( unsigned char type, short ierr, char *note )
{
  MSG_LOGO    logo;
  char        msg[256];
  long        size;
  time_t      t;

  /* Build the message
   *******************/
  logo.instid = InstId;
  logo.mod    = MyModId;
  logo.type   = type;

  time( &t );

  if( type == TypeHeartBeat )
  {
    sprintf( msg, "%ld %ld\n", (long) t, (long) myPid);
  }
  else if( type == TypeError )
  {
    sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
    logit( "et", "wave_serverV: %s\n", note );
  }

  size = (long)strlen( msg );   /* don't include the null byte in the message */

  /* Write the message to shared memory
   ************************************/
  if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
  {
    if( type == TypeHeartBeat ) {
      logit("et","wave_serverV:  Error sending heartbeat.\n" );
    }
    else if( type == TypeError ) {
      logit("et","wave_serverV:  Error sending error:%d.\n", ierr );
    }
  }
  if (Debug) {
    if( type == TypeHeartBeat ) {
	logit("t", "wave_serverV: Debug, sent heartbeat message: %s\n", msg);
    } else {
	logit("t", "wave_serverV: Debug, sent error message: %s\n", msg);
    }
  }

  return;
}


/*-----------------------------------------------------------------------*/
/***********************************************************************
 * UpdateIndex() Updates the memmory index list with a new sample of   *
 *  trace data.                                                        *
 ***********************************************************************/
int UpdateIndex(TANK * pTSPtr, TRACE2_HEADER * pCurrentRecord,
                unsigned int CurrentOffset, int CheckOverwrite)
{
  /*
     Purpose:
     Updates the memmory index list with a new sample of trace data.

     Arguments:
         pTSPtr: pointer to the TANK structure we are dealing with
         pCurrentRecord: pointer to the trace record that was just
            copied into the tank
         CurrentOffset: the current offset to the tank file, updated after
            the current record was inserted.
         CheckOverwrite: flag to update the index for the old record that
            was overwritten by the current record. This check is needed
            when wave_serverV is running; unneccesary when UpdateIndex is
            called during wave_serverV startup.

     Return Values:
     0: Successful update.
     -1: Error occured during update.

     Functions Called:
     (not analyzed)

     Additional Remarks:
     Apologia: we resort to making a special case of the first write
     (and the first pass)to keep the initialization logic from becoming
     too baroque and hard to debug. We gotta ship this
  */

  char scrMsg[MAX_TRACEBUF_SIZ];
  static char * MyFuncName = "UpdateIndex()";
  int RetVal;
  DATA_CHUNK *dc;


  if(Debug > 2)
  {
    logit("t","Entering %s\n",MyFuncName);
    logit("et", "S %s,C %s,N %s,L %s,StartTime %f,EndTime %f,CurrentOffset %u\n",
          pCurrentRecord->sta,pCurrentRecord->chan,pCurrentRecord->net,
	  pCurrentRecord->loc,
          pCurrentRecord->starttime,pCurrentRecord->endtime,CurrentOffset
          );
  }

  /* Special case: first write ever
   ********************************/
  if ( pTSPtr->firstWrite == 1 )
  {
    /* CHECK:  This assumes that initialization of the index sets
       IndexYoungest, and probably IndexOldest, to valid places, with
       memory allocated at those places  DK
    */
    if(Debug > 2)
      logit("et","UpdateIndex: First Write\n");

    dc=IndexYoungest(pTSPtr);
    dc->tStart = pCurrentRecord->starttime;
    dc->tEnd   = pCurrentRecord->endtime;

    /* First write is always at beginning */
    IndexYoungest(pTSPtr)->offset = 0;

    /* Move the data type, pin number, and sampling rate */
    strncpy(pTSPtr->datatype,  pCurrentRecord->datatype,  3);
    pTSPtr->samprate   = pCurrentRecord->samprate;
    pTSPtr->pin   = pCurrentRecord->pinno;

    pTSPtr->firstWrite =0; /* no longer the first write */

    /* All done. Release this tank */
  } /* End if first write */
  else  /* Not first write */
  {

    /* Update index's newest time
     *****************************/
    /* philosophy: we just wrote a bunch of data points to the tank. If
       the oldest of those is  more than one and a half sample intervals
       (looks like 1.0 now) younger than the youngest we had before this
       write, we declare a gap has occurred. We create a new chunk in the
       index. We assume the insertion point has not been updated yet
    */

    if(Debug > 2)
      logit("et","UpdateIndex: Normal Write\n");

    /* Is there a Gap
     ********************************/
    if( IndexYoungest(pTSPtr)->tEnd +
        GapThresh*(1.0/(pCurrentRecord->samprate)) <
        pCurrentRecord->starttime )
    {
      /* Add a new index entry
       ********************************/
      if(Debug > 2)
        logit("et","UpdateIndex:  adding index, start,finish: %u,%u\n",
              pTSPtr->indxStart,pTSPtr->indxFinish);

      if(( RetVal = IndexAdd( pTSPtr, pCurrentRecord->starttime,
                              pCurrentRecord->endtime,
                              CurrentOffset            )) != 0)
      {
        /* Davidk 9/8/98 */
        /*  We have a problem, either overwrote an index, or
            something really serious happened. */
        if(RetVal > 0)
        {
          /* we're out of index entires*/
          if(pTSPtr->lappedIndexEntries == 0)
          {
            sprintf(Text,"Ran out of free indexes.  Overwriting "
                    "oldest indexes in tank: %s,%s,%s,%s\n",
                    pTSPtr->sta,pTSPtr->chan,pTSPtr->net,pTSPtr->loc);
            wave_serverV_status(TypeError, ERR_OVERWRITE_INDEX, Text);
          }
          pTSPtr->lappedIndexEntries++;
          /* goto abortUpdateIndex; */
          /* Overwritten index entries is a problem handled
             internally by this function.  It provides no
             notice to its caller that something has gone
             wrong.  This is to prevent adverse logging.
             When an index is overwritten, everything still
             completes successfuly with the transaction, the
             problem is that old data has now been overwritten.
             Thus the update did not fail, it just didn't go
             as well as planned.  DavidK 9/8/98
          */
        }
        else
        {
          /* This is bad.  Something internal went kaboom.
             Something is corrupted in the tank.  Scream
             for help!!!!! and then abort.
          **********************************************/
          sprintf(Text,"Corruption problems in memory tank structure"
                  "for tank: %s,%s,%s,%s\n",
                  pTSPtr->sta,pTSPtr->chan,pTSPtr->net,pTSPtr->loc);
          wave_serverV_status(TypeError, ERR_TANK_CORRUPTION, Text);
          goto abortUpdateIndex;
        }
      }

      else /* IndexAdd() completed successfully */
      {
        if(pTSPtr->lappedIndexEntries != 0)
        {
          sprintf(Text,"wave_serverV:  Found Free Index.  No longer overwriting"
                  " free indexes.  Overwrote %u "
                  "indexes in tank: %s,%s,%s,%s\n",
                  pTSPtr->lappedIndexEntries,
                  pTSPtr->sta,pTSPtr->chan,pTSPtr->net,pTSPtr->loc);
          wave_serverV_status(TypeError, ERR_RECOVER_OVERWRITE_INDEX, Text);
          pTSPtr->lappedIndexEntries=0;
        }
      }

      if(Debug > 2)
        logit("et","UpdateIndex:  after adding index, start,finish: %u,%u\n",
              pTSPtr->indxStart,pTSPtr->indxFinish);

    }
    else  /* No Gap */
      /* if a new chunk was not created by this write, we update the index by re-writing the
         end time of the youngest chunk */
    {
      IndexYoungest(pTSPtr)->tEnd = pCurrentRecord->endtime;
      if(Debug > 2)
        logit("et","UpdateIndex:  extending index, new endtime %f,%f\n",
              pCurrentRecord->endtime,IndexYoungest(pTSPtr)->tEnd);

    }

    if( pTSPtr->firstPass != 1 && CheckOverwrite)
    {
      /* Update index's oldest chunk.
         Concern is whether we just
         wiped out an old chunk
      ******************************/

      /* for now, brute force: read it out of the file. If that's too
         much disc action, we could compute it from the index - somehow,
         some day
      */

      if(Debug > 2)
        logit("et","Update Index: Not first pass, doing other junk\n");


      /* seek to the current position.  This is neccessary between reads and writes */
      if( fseek( pTSPtr->tfp, pTSPtr->inPtOffset, SEEK_SET ) != 0 )
      {
        logit( "e", "wave_serverV: error on fseek on tank %s; exiting!\n",pTSPtr->tankName );
        goto abortUpdateIndex;
      }
      if( fread( (void *)scrMsg, pTSPtr->recSize, 1, pTSPtr->tfp ) != 1 )
      {
        logit( "e", "wave_serverV: error on fread tank %s; exiting!\n",pTSPtr->tankName );
        goto abortUpdateIndex;
      }
      /* We peeked ahead into the tank, and picked up the oldest message into scrMsg */

      /* If we just wiped out a chunk, get rid of its index entry
       ***********************************************************/
      if( ((TRACE2_HEADER*)scrMsg)->starttime > IndexOldest(pTSPtr)->tEnd )
      {
        IndexDel(pTSPtr,IndexOldest(pTSPtr));

        if (Debug > 2)
          logit("","After deletion:\n");
      }
      else
      {
        IndexOldest(pTSPtr)->tStart = ((TRACE2_HEADER*)scrMsg)->starttime;
        IndexOldest(pTSPtr)->offset = pTSPtr->inPtOffset;
      }
    } /* End !FirstPass */
  }  /* End else !FirstWrite */

  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);

  return(0);

 abortUpdateIndex:
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
  }
  return(-1);
}  /* End UpdateIndex */


/*****************************************************************************/
/* int ReportServerThreadStatus() */
/*****************************************************************************/
int ReportServerThreadStatus(void)
{
  int i;
  char StatusString[8];

  logit("","  Clnts Accp    Clnts Prcsd   Reqs Prcsd    Errors    Status  \n");
  logit("","==============================================================\n");
  for(i=0;i<MaxServerThreads;i++)
  {
    switch (ServerThreadInfo[i].Status)
    {
    case SERVER_THREAD_IDLE:      strcpy(StatusString," IDLE "); break;
    case SERVER_THREAD_COMPLETED: strcpy(StatusString,"CMPLTD"); break;
    case SERVER_THREAD_ERROR:     strcpy(StatusString," ERROR"); break;
    case SERVER_THREAD_WAITING:   strcpy(StatusString,"WTG4CL"); break;

    default :
      {
        if(!memcmp(&(ServerThreadInfo[i]),&(ServerThreadInfoPrevious[i]),
                   sizeof(ServerThreadInfoStruct))
           )
        {
          /* We are busy and nothing has changed since the last check.
             We are not waiting for the client.
             I think the thread is hung!!!!
          */
          strcpy(StatusString," HUNG ");
          logit("et","ServerThread %d appears to be hung in state %d.\n",
                i,ServerThreadInfo[i].Status);
        }
        else
        {
          strcpy(StatusString," BUSY ");
        }
      }
    } /* End of Switch() */

    logit("",
          "%2d  %6d         %6d       %6d      %6d    %s\n",i,
          ServerThreadInfo[i].ClientsAccepted,
          ServerThreadInfo[i].ClientsProcessed,
          ServerThreadInfo[i].RequestsProcessed,
          ServerThreadInfo[i].Errors,
          StatusString);
  }  /* end for (each server thread) loop */

  /* Copy the current ServerThreadInfo to ServerThreadInfoPrevious. */
  memcpy(ServerThreadInfoPrevious,ServerThreadInfo,
         MaxServerThreads * sizeof(ServerThreadInfoStruct));

  return(0);
}  /* ReportServerThreadStatus */

TANK * FindSCNL(TANK * Tanks, int nTanks, char * sta, char * chan, char * net,
		char * loc)
{
  TANK TempTank;

  /* Use CRTLIB func bsearch() to
     find SCN.  Davidk 10/5/98
  **********************************/

  strcpy(TempTank.sta,sta);
  strcpy(TempTank.chan,chan);
  strcpy(TempTank.net,net);
  strcpy(TempTank.loc,loc);

  return((TANK *) bsearch(&TempTank, Tanks, nTanks,
                          sizeof(TANK), CompareTankSCNLs));
}  /* End FindSCNL() */

/**************************************************************
 *  signal_hdlr: sets the terminate flag for orderly shutdown *
 **************************************************************/
void signal_hdlr(int signum)
{
  terminate = 1;                 /* set flag to terminate program */
}


int wave_serverV_c_init()
{
  logit("t","wave_serverV.c:Version %s\n", WSV_VERSION);
  return(0);
}

int IssueIOStatusError(char * szFunction, int iOffset,
                       int iRecordSize, char * szTankName)
{
  sprintf(Text, "%s failed for tank [%s], with offset[%d] "
          "and record size[%d]! errno[%d]\nError: %s\n",
          szFunction, szTankName, iOffset,
          iRecordSize, errno, strerror(errno));

  wave_serverV_status( TypeError, ERR_FILE_IO, Text );
  return(0);
}

int TraceBufIsValid(TANK * pTSPtr, TRACE2_HEADER * pCurrentRecord)
{
    /* check that time goes forward within the tracebuf */
    if(pCurrentRecord->starttime > pCurrentRecord->endtime) {
	if (Debug > 1)
	    logit("e", "TraceBufIsValid: TB start time not before end time\n");
	return(FALSE);
    }

    /* check that there are samples in the tracebuf */
    if(pCurrentRecord->nsamp <= 0) {
	if (Debug > 1)
	    logit("e", "TraceBufIsValid: no samples\n");
	return(FALSE);
    }
    /* added check for datatype! */
    if (strcmp(pCurrentRecord->datatype, "i4") == 0 ||
        strcmp(pCurrentRecord->datatype, "i2") == 0 ||
        strcmp(pCurrentRecord->datatype, "s2") == 0 ||
        strcmp(pCurrentRecord->datatype, "s4") == 0 ) {
        /* do nothing here for now, its good */
    } else {
	logit("e", "TraceBufIsValid: bad datatype found in packet '%s'\n", pCurrentRecord->datatype);
        return(FALSE);
    }

    /* if the tracebuf internals look OK, and this is the first tracebuf,
       then send it on it's merry way. */
    if ( pTSPtr->firstWrite == 1 )
	return(TRUE);

    /* check that time goes forward between the previous tracebuf and this
       one */

    if((pCurrentRecord->starttime < IndexYoungest(pTSPtr)->tEnd) && !bUsePacketSyncDb) {
     	if (Debug > 1)
	      logit("e", "TraceBufIsValid: packet time not advancing\n");
	    return(FALSE);
     }

    /* check samprate ???? */

    /* it passed all the tests, so pass it on. */
    return(TRUE);

}  /* end TraceBufIsValid() */
