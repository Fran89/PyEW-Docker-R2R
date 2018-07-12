
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: wave_serverV.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2010/03/18 16:24:38  paulf
 *     added client_ip echoing to ListenForMsg() as per code from DK
 *
 *     Revision 1.8  2005/03/17 17:28:48  davidk
 *     Changes to enforce a maximum tanksize of 1GB.
 *     Added a MAX_TANK_SIZE #define of 1GB-1.
 *
 *     Revision 1.7  2004/06/10 23:14:09  lombard
 *     Fixed logging of invalid packets
 *     Fixed compareSCNL function.
 *
 *     Revision 1.6  2004/05/18 22:30:19  lombard
 *     Modified for location code
 *
 *     Revision 1.5  2001/08/11 16:56:29  davidk
 *     replaced the original GRACE_PERIOD concept with one that handles timestamp
 *     slop.  Set the acceptable timestamp slop to 30%(0.3).
 *
 *     Revision 1.4  2001/01/18 02:30:01  davidk
 *     Added ability to issue status messages from server_thread.c,
 *     which entailed the following changes:
 *     1. Moved #define constants for status message types to wave_serverV.h
 *     so that they can be used by all .c files.
 *     2. Moved wave_serverV_status() prototype to wave_serverV.h so
 *     that other .c files can issue status messages.
 *
 *     Added bAbortOnSingleTankFailure extern, so that all .c
 *     files can process based upon AbortOnSingleTankFailure
 *     flag.
 *
 *     Revision 1.3  2000/07/08 19:01:07  lombard
 *     Numerous bug fies from Chris Wood
 *
 *     Revision 1.2  2000/06/28 23:46:44  lombard
 *     added signal handler for graceful shutdowns; numerous bug fixes
 *     See README.changes for complete list
 *
 *     Revision 1.1  2000/02/14 19:58:27  lucky
 *     Initial revision
 *
 *
 */


/* Header for wave_serverV. Alex 1/13/97 */

#ifndef _WAVE_SERVERV_
#define _WAVE_SERVERV_

#include <trace_buf.h>
#define MAX_TANK_NAME   128      /* characters in full tank name */

/* Structure prototypes
***********************/
typedef struct                  /*** Data Chunk descriptor. ***/
/*The element of the Chunk Index. Desribes a chunk of data in the tank file */
{
  double tStart;                /* Start time of this chunk */
  double tEnd;                  /* End time of this chunk */
  long int offset;              /* file offset of start of this chunk */
} DATA_CHUNK;

typedef char szIP16[16];	/* used for storing IP string of clients */

#ifdef _WSVMAIN_
szIP16 * client_ip;
#else
extern szIP16 * client_ip;	/* array of client IP address 1 per srver thread */
#endif

typedef struct _RedundantFile
{
  FILE * pCurrentFile;
  unsigned int NumOfFiles;
  char Filename1[MAX_TANK_NAME];
  char Filename2[MAX_TANK_NAME];
  unsigned int CurrentFile;
} RedundantFile, * RedundantFilePtr;

typedef RedundantFilePtr IndexFilePtr;
typedef RedundantFilePtr TANKListFilePtr;

typedef struct                  /*** Tank descriptor ***/
{
  char          tankName[MAX_TANK_NAME];/* full path name of tank file */
  FILE*         tfp;            /* tank file pointer */
  IndexFilePtr ifp;             /* index file pointer (the name's derVed from the tankName)*/
  mutex_t               mutex;  /* our mutex */
  MSG_LOGO      logo;           /* logo kept in this tank (from transport.h) */
  int           pin;            /* pin number kept in this tank */
  char          sta[TRACE2_STA_LEN];   /* zero terminated string */
  char          net[TRACE2_NET_LEN];   /* zero terminated string */
  char          chan[TRACE2_CHAN_LEN]; /* zero terminated string */
  char          loc[TRACE2_LOC_LEN];   /* zero terminated string */
  char          datatype[3];    /* zero terminated string e.g. i2,s4,.. */
  double        samprate;       /* Sample rate; nominal */
  long int      tankSize;       /* size of  tank file in bytes */
  DATA_CHUNK*   chunkIndex;     /* pointer to start of data chunk array */
  unsigned int indxStart;       /* array offset of first (oldest) entry in
                                 * index */
  unsigned int indxFinish;      /* array offset of last (youngest) entry in
                                 * index */
  long int      indxMaxChnks;   /* max data chunks in index */
  long int      recSize;        /* message record size. Each msg gets that
                                 * much disk space */
  long int      nRec;           /* number of records in tank file */
  long int      inPtOffset;     /* Insertion point of this tank */
  int           isConfigured;   /* 0 if the tank hasn't been configured yet, 
                                 * 1 otherwise */
  int           firstPass;      /* 1 => first time through tank. 0 otherwise */
  int           firstWrite;     /* 1 => first write to brand new tank. 0
                                 * otherwise */
  time_t lastIndexWrite;        /* Last time the index was written to disk */
  int lappedIndexEntries;    /* The number of lapped index entries since
                                last successful index entry creation */
  int reserved[2];      /* Reserved for future modifications.  So that mods
                           can be made to the structure without changing the
                           size and rendering the previous TANK structs
                           useless */
} TANK;

/**************************
        Type Declarations
   **************************/


typedef struct _TANKList
{
  TANK * pFirstTS;
  unsigned int NumOfTS;
  TANKListFilePtr pTSFile;
  int redundantIndexFiles;      /* 1 => using redundant index files */
  int redundantTANKFiles;       /* 1 => using redundant tank struct files */
} TANKList;

typedef struct _CHUNKS_BUFFER
{
  DATA_CHUNK * pBuffer;
  int NumOfChunks;
} CHUNKS_BUFFER;

struct TankFileInfo
{
  time_t TimeStamp;
  unsigned int NumOfTanks;
};

struct IndexInfo
{
  time_t TimeStamp;
  unsigned int NumOfChunks;
};

/* This belongs more in server_thread.h
   than here, but it is needed by wave_server.c
   and wave_server.c does not currently include
   server_thread.h.  Instead of creating a possible
   inclusion mess, I will just place it here.  
   DavidK 9/24/98
*/

typedef struct _ServerThreadInfoStruct
{
  int Status;
  SOCKET ActiveSocket; /* Socket descriptors; active sockets    */
  int ClientsAccepted;
  int ClientsProcessed;
  int RequestsProcessed;
  int Errors;
} ServerThreadInfoStruct;

/* Server Thread States.  */
#define SERVER_THREAD_ERROR -1
#define SERVER_THREAD_IDLE 0
#define SERVER_THREAD_START 1
#define SERVER_THREAD_WAITING 2
#define SERVER_THREAD_MSG_RCVD 3
#define SERVER_THREAD_MSG_PARSED 4
#define SERVER_THREAD_MSG_PROCESSED 5
#define SERVER_THREAD_DISCONN 6
#define SERVER_THREAD_COMPLETED 10

/* Error messages used by wave_server
 ************************************/
#define  ERR_MISSMSG                 0 /* message missed in transport ring   */
#define  ERR_TOOBIG                  1 /* retreived msg too large for buffer */
#define  ERR_NOTRACK                 2 /* severe weirdness in the cellar     */
#define  ERR_QUEUE                   3 /* error queueing message for sending */
#define  ERR_OVERWRITE_INDEX         4 /* error ran out of free indexes, overwriting old ones */
#define  ERR_RECOVER_OVERWRITE_INDEX 5 /* no longer running out of free indexes */
#define  ERR_TANK_CORRUPTION         6 /* tank structure in memory is corrupted */
#define  ERR_FILE_IO                 7 /* hard i/o error on tank or index file  */
#define  ERR_FAILED_TANK_SUMMARY     8 /* failed to write tank summary for file */


/**************************
      End Type Declarations
   **************************/

    /**************************
    #define Absolute Constants
   **************************/
#define MAX_TANKS       512             /* largest number of  tanks we're capable of */
#define WAIT_FOR_SERVER_THREAD  3000    /* milliseconds to wait for a thread to come free */
#define MY_TYPE         TYPE_TRACEBUF2  /* the only message type we deal with */
#define INDEX_EXTENTION  ".inx"         /* appended to the tank file name to create the index file name */
#define MAX_CMD_LEN     100             /* Max #chars in command from client  */
#define MAX_FILES_IN_REDUNDANT_SET 2
#define MAX_TANK_SIZE  1073741823       /* Maximum size of a wave_serverV tank file */
/**************************
           End Constants
   **************************/


   /**************************
             Externs
   **************************/
extern int PleaseContinue;
extern volatile int Debug;
extern int MaxServerThreads;
extern int ClientTimeout;

extern unsigned char InstId;         /* local installation id      */
extern unsigned char TypeHeartBeat;
extern unsigned char TypeError;
extern unsigned char TypeWaveform;

extern int           bAbortOnSingleTankFailure;


/**************************
  End Externs
  **************************/


   /**************************
        Function Prototypes
   **************************/

  /* Index manipulation routine prototypes */
void        IndexInit( TANK* );
int         IndexAdd( TANK*, double, double, long int );
void        IndexDel( TANK*, DATA_CHUNK* );
DATA_CHUNK* IndexNext( TANK*, DATA_CHUNK* );
DATA_CHUNK* IndexOldest( TANK* );
DATA_CHUNK* IndexYoungest( TANK* );
DATA_CHUNK* AddALink( TANK* t, int *pStatus );

int         GetLatestTankStructures(TANKList * pTANKList);
int         TankIsInList(TANK * pTSPtr, TANKList * pConfigTankList);
int         OpenTankFile(TANK * pTSPtr);
int         BuildLIndex(TANK * pTSPtr);
int         WriteLIndex(TANK * pTSPtr, int bUseMutexes, time_t CurrentTime);
int         CopyTANKFilePtr(TANKList * pSrcList, TANKList * pDestList);
int         TankAlreadyConfigured(TANK * pTSPtr);
int         TankIsNew(TANK * pTSPtr);
int         CreateTankFile(TANK * pTSPtr);
int         OpenIndexFile(TANK * pTSPtr, int UseRedundantIndexFiles,
                          int CreateNew);
int         MarkTankAsBad(TANK * pTSPtr);
int         RemoveBadTanksFromList(TANKList * pTANKList);
TANK *      ConfigTANK(TANK * pTSPtr, TANKList * pConfigTankList);

/* ConfigTANK() configures a GetLatestTankStructures in one list with
 * paramaters taken from a comparable TANK structure.  It returns a
 * pointer to the TANK structure that was modified.
 */

int         CreatePersonalIndexBuffer(CHUNKS_BUFFER * pCB);
TANKList *  CreatePersonalTANKBuffer(TANKList * pSrcList);
int         GetTANKs(TANK * SrcTanks, TANK * DestTanks, int NumOfTanks,
                     int UseMutexes);
int         WriteTANKList(TANKList * pMyTANKList,char * pTANKListBuffer);
int         GetIndex(TANK * pSrcTank, TANK * pDestTank, CHUNKS_BUFFER * pCB);
int         WriteIndex(TANK * pTank, DATA_CHUNK * pIndexBuffer);
int         CleanupIndexThread(TANKList * pTANKList);
char*       GetRedundantFileName(RedundantFilePtr);

  /* Config routines */
int         InitTankList(TANKList ** ppConfigTankList,
                         TANK * pTankArray,
                         int RedundantTankStructFiles,
                         int RedundantIndexFiles,
                         int NumberOfTanks,
                         char * TankStructFile,
                         char * TankStructFile2);

/* Other routines */
int         InitializeFiles(RedundantFilePtr rfPtr);
int         ReadRFile(RedundantFilePtr rfPtr,unsigned int Offset,
                      int BytesToRead,char * pBuffer);
int         InvalidateRFile(RedundantFilePtr rfPtr);
int         WriteRFile(RedundantFilePtr rfPtr,unsigned int Offset,
                       int BytesToWrite, char* pBuffer, int FinishedWithFile);
int         CopyLIndexintoBuffer(TANK * pTSPtr, char* pBuffer);
int         CheckForValidAndGreaterThanIndexEnd(char * pCurrentRecord,
                                                double IndexEnd, 
                                                double LastEnd,
                                                int CurrentPosition, 
                                                int TankOffset);
int         UpdateIndex(TANK * pTSPtr, TRACE2_HEADER * pCurrentRecord, 
                        unsigned int CurrentOffset, int CheckOverwrite);
int         GetNumOfFiles(RedundantFilePtr rfPtr);

RedundantFilePtr CreateFilePtr(char* FName1, char* FName2);
RedundantFilePtr CreateRFile(char *Filename1, char *Filename2, int size);
RedundantFilePtr TestRFile(char *Filename1, char *Filename2, int size);

/* function to report file i/o errors */
int IssueIOStatusError(char * szFunction, int iOffset, 
                       int iRecordSize, char * szTankName); 

/* function to issue status messages */
void    wave_serverV_status( unsigned char, short, char * );

int CompareTankSCNLs( const void *s1, const void *s2 );
TANK * FindSCNL(TANK * Tanks, int nTanks, char * sta, char * chan, char * net,
	       char * loc);

/**************************
  End Function Prototypes
**************************/

#include <socket_ew.h>
/* Externs */
extern int SocketTimeoutLength;

/* Timestamp slop acceptability used 
   while examining data in tanks, and determining which samples
   to start and end with in request replies.   */
/* Timestamp slop was previously handled only within SendReqDataAscii()
   when determining if/when fill values should be inserted.  It was 
   handled by a hardcoded factor at the top of <if I should fill> 
   conditionals.  Depending on the wave_server release, the value ranged
   between 1.0 and 1.5, representing the number of samples that were 
   allowed to be missing before a sample would be filled with a default
   FILL value.  The factor is now called MINIMUM_GAP_SIZE (defined in
   serve_trace.c) and is broken up into 1.0 + ACCEPTABLE_TIMESTAMP_SLOP,
   where ACCEPTABLE_TIMESTAMP_SLOP represents the percentage of slop
   allowed in timestamps, and expressed as MaximumSlopTime as a percentage
   of sample time or (MaximumSlopTime*SampleRate).  The value of 
   ACCEPTABLE_TIMESTAMP_SLOP should be between 0 and 50%.  If it exceeds
   50%, then it is possible for timestamps to overlap, since sample 1
   could hit at t1+50% and sample 2 could hit at t2-50%, thus putting
   both at t1.5.
   DavidK 08/10/01
************************************************************************/
/* Factor 1.5 below changed to 1.0; otherwise one fill value would be
 * missed where needed: PNL 6/25/00 */
/* ACCEPTABLE_TIMESTAMP_SLOP is the maximum sample rate
   time that can pass before we assume a missing sample 
   and insert a fill value.  So if the 
   ACCEPTABLE_TIMESTAMP_SLOP was 0.3, then that means the
   timestamps can drift up to 30% of the sample rate.  With
   100hz data, that means the timestamps can be off by 
   3 milliseconds, without causing a gap to be detected.
   The current value of 0.3 (30%) was arbitrarily chosen.
   DK 08/09/2001
*/
#define ACCEPTABLE_TIMESTAMP_SLOP 0.3


# define GRACE_PERIOD (ACCEPTABLE_TIMESTAMP_SLOP/t->samprate)

/* GRACE_PERIOD was developed based on a misunderstanding of its author.  It's
   author thought of time samples as resembling continuums of data. 
   If a tracebuf contained data from time 10 to time 10.995, then 10 was
   the start of the packet and 10.995 was the end of the packet, and if you
   asked for data for 10.995 there wasn't any, because that was the end time
   of the packet.  So if Data.starttime = Data.endtime, then there was no
   data.

   GRACE_PERIOD now serves ONLY to provide a mechanism for accomodating sloppy
   timestamps.  The author is operating under the belief that a 
   trace_buf from 10.00 to 10.90, containing 10 samples of 10hz data,
   contains 10 descrete datapoint samples from 
   (10.00, 10.10, 10.20, 10.30, 10.40, 10.50, 10.60, 10.70, 10.80, 10.90), and
   if a request is made for data in the range (10.93 - 11.93), then as long 
   as ACCEPTABLE_TIMESTAMP_SLOP is atleast 30%, then there is one valid 
   data sample in the current trace_buf that fits the request criteria(10.90).
   If Data.starttime = Data.endtime, then there is a single sample of data,
   available for the request.  The departure from the original GRACE_PERIOD 
   concept described below, will probably cause a performance slowdown, but it
   will make additional relevant samples available.  DavidK 081001
*/
/* THIS COMMENT IS OUT OF DATE AND SOME OF THE LOGIC WITHIN IT IS FLAWED!
   IT STILL EXISTS ONLY TO PROVIDE DOCUMENTATION OF THE ASSUMPTIONS THAT
   THE AUTHOR OF THE ORIGINAL GRACE_PERIOD, WAS WORKING UNDER, AND POSSIBLY
   TO PROVIDE AN EXPLANATION FOR ANY GRACE_PERIOD RELATED BUGS THAT ARE STILL
   IN THE CODE.
   
   GRACE_PERIOD is a time buffer used for checking whether a starttime or
   endtime is equal to or < or > respectively, the requested time (reqt).
   It's purpose is to prevent extra record checking and marking by allowing
   records with Start or End times that are very close to reqt, to satisfty
   the requirements check for being equal to reqt.  GRACE_PERIOD is used to
   improve performance.  It should also provide functionality, by preventing
   records that meet comparital requirements, but would contain no useful
   data, from being returned by LocateExactOffset().  An example, is
   wave_server searching for data that runs from time 9.995 to 10.995.
   Before, if it came across a record that ran from 9.000 to 10.000, it would
   believe that that record contained useful data, and would return it as the
   starting record.  The problem with this, is that even though 9.995 is <
   10.000, there are no data samples available during time 9.995 - 10.000.
   GRACE_PERIOD is used to adjust the comparisons so that 9.995 is compared to
   10.000 - GRACE_PERIOD instead of just 10.000.  GRACE_PERIOD fixes a problem
   that arose before, when LocateExactOffset() would report starting and
   ending records to WriteTraceDataAscii(), that met the aforementioned
   conditions, but that WriteTraceDataAscii() evaluated to be completely
   useless.  WriteTraceDataAscii() couldn't figure out what to do with useless
   records, and it tended to blowup, due to calculations that it performed.  */

# ifndef TRUE
#  define TRUE 1
#  define FALSE 0
# endif /* !def TRUE */

#endif /* _WAVE_SERVERV_ */
