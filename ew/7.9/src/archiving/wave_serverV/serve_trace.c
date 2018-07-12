
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: serve_trace.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.16  2010/03/13 16:24:25  paulf
 *     end of tank read fix from Dave Kragness, needs testing
 *
 *     Revision 1.15  2007/11/30 18:40:16  paulf
 *     sqlite stuff added
 *
 *     Revision 1.14  2007/03/28 18:02:34  paulf
 *     fixed flags for MACOSX and LINUX
 *
 *     Revision 1.13  2005/07/21 21:05:51  friberg
 *     Added _LINUX ifdef to roundup to prevent macro use
 *
 *     Revision 1.12  2005/03/17 17:30:05  davidk
 *     Changes to enforce a maximum tanksize of 1GB.
 *     Non-functional change.
 *     Added a  Long comment describing the bug in LocateRoughOffset
 *     that limits WaveServerV tank sizes to 1GB.
 *
 *     Revision 1.11  2004/05/18 22:30:19  lombard
 *     Modified for location code
 *
 *     Revision 1.10  2003/01/28 00:42:27  alex
 *     changed logit message around line 2269. Alex
 *
 *     Revision 1.9  2002/10/11 21:44:08  davidk
 *     Fixed a bulk send fill statement that miscalulated the number
 *     of fill values.  It sent one too few.  This fixes a fix to the
 *     same statement, from 08/2001, when the statement was issuing
 *     one too many fill samples.  The 08/2001 fix removed two(2) samples
 *     instead of just 1.
 *
 *     Revision 1.8  2002/10/11 21:31:29  davidk
 *     Checking in change from 08/2001.  Change had never been checked in.
 *     Checking it in, so that I can fix a bug in it.
 *
 *     Revision 1.7  2001/08/11 17:16:45  davidk
 *     modified GRACE_PERIOD code in locating offsets, so that it now
 *     accomodates sloppy timestamps, instead of doing whatever it
 *     did before.  The previous GRACE_PERIOD logic was flawed as it
 *     was based on misunderstandings about the interpretation of
 *     trace_buf packets by the GRACE_PERIOD author(me).
 *
 *     Modified SendReqDataAscii().  Moved the bulk of the code out
 *     into a separate function SendReqDataAscii_AndThisTimeWeMeanIt().
 *     Fixed ascii reply code so that it better handled sloppy timestamps
 *     and fill values.
 *     See wave_serverV/README.changes for more detailed info.
 *
 *     Revision 1.6  2001/06/29 22:23:59  lucky
 *     Implemented multi-level debug scheme where the user can specify how detailed
 *     (and large) the log files should be. If no Debug is specified, only
 *     errors are reported and logged.
 *
 *     Revision 1.5  2001/01/17 21:44:25  davidk
 *     Added code to report hard(disk) read/write errors via status messages.
 *
 *     Revision 1.4  2000/10/02 18:06:48  lombard
 *     Fixed two places where unnecessary calls to fseek() could cause an
 *     endless loop if fseek() failed, such as the tank file having been
 *     closed by the main thread. The fseek() was intended to restore the
 *     tank file position to that needed by the main thread.
 *
 *     Revision 1.3  2000/07/08 19:01:07  lombard
 *     Numerous bug fies from Chris Wood
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


/* serve_trace.c: created April 25, 1997 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <errno.h>
#include        <earthworm.h>
#include        <transport.h>           /* needed for wave_serverV.h */
#include        <trace_buf.h>
#include        <socket_ew.h>
#include        "wave_serverV.h"
#include        "server_thread.h"
#include		"tb_packet_db.h"		/* ronb041007 - used to read back out-of-sync packets*/
   

#define FLAG_L    0
#define FLAG_R    1

#define D_SIZE_I2 2
#define D_SIZE_I4 4

#define MAX_TRACE_INT_LEN   16
#define MAX_TRACE_DBL_LEN  128
#define MAX_TRACE_HDR_LEN  256
#define MAX_TRACE_SOC_LEN 8192/*4096*/
#define DBL_FLOAT_NA      ((double) -1.0)
#define DBL_FLOAT_EPSILON ((double) 0.00010) /* << 100 samples/sec */

#define SPACE_STR (char *) " "
#define LN_FD_STR (char *) "\n"

#define NO_NEWLINE 0
#define YES_NEWLINE 1

/* return codes used in SendReqDataAscii() */
#define SRDA_ABORT     -1
#define SRDA_MEM_ABORT -2



/* MINIMUM_GAP_SIZE is the smallest time gap between samples
   that can be considered to contain a gap in data.  It is
   given in terms relative to sample rate.  If it is 1.3, then
   that means there must be atleast a (1.3/SAMPLERATE) second
   difference between the last sample and the next before a
   GAP can be declared.  This is not related to GAPSIZE and
   wave_serverV indexes.
   DK 08/09/2001
*/
#define MINIMUM_GAP_SIZE (1.0 + ACCEPTABLE_TIMESTAMP_SLOP)



extern int bUsePacketSyncDb;	/* config flag to use packet db to sync data. ronb011007*/
extern int bTankOverSyncDbData; /* config buffer precedence. */
extern TBDB_STORE* pPacketDb;	/* ref to OutOfSync Packet Db. ronb041007*/

int _writeTraceDataRaw( SOCKET, TANK*, CLIENT_MSG* );
int _writeTraceDataAscii( SOCKET, TANK*, CLIENT_MSG* );

static int SendReqDataRaw( SOCKET, TANK*, double, double, double, double,
                           double, double,
                           char*, long int, char* );
static int SendReqDataAscii( SOCKET, TANK*, double, double, double, double,
                             double, double, char*,
                             char*, long int, char* );

static int GetOffsetLeft( TANK*, double, long int*, double* );
static int GetOffsetRight( TANK*, double, long int*, double* );
static int LocateExactOffset( TANK*, DATA_CHUNK*, double, int,
                              long int*, double*, DATA_CHUNK * );
static int LocateRoughOffset( TANK*, double, double, double,
                              long int, long int, int, long int*, double* );
static int LocateStartOfTank( TANK*, long int*, double* );
static int LocateEndOfTank( TANK*, long int*, double* );

static int CopyReqData( TANK*, long int, long int, char**, long int* );
static int ReadBlockData( FILE*, long int, long int, char* );
static int FreeReqData( char* );

static int BulkSendFill( SOCKET, char*, long int );
static int SendHeader( SOCKET soc, TANK* t, char* reqid, char* flag, double tS,
		       double tE, int binSize, int flagNewLine);
static int SendStrData( SOCKET soc, char* str );
static int MergeAsyncPackets(
     TANK* t,
 	char** data,
 	long* datasize,
 	char* AsyncData,
 	long AsyncDataSize,
 	long AsyncPacketCount
);


static int _fDBG = -1;  /* equal to -1 upon first entry to ws_client.c */
static int _fSUB = 2;
static int _fALG = 16;
static int _fPRT = 32;

/* set _fDBG to be non-zero for extended debug logit() */
static int _fINI = 0;
static char _pSYSTEM[32] = "ew_debug.ini";
static char _pMODULE[32] = "serve_trace";

static int ewInitDebugFlag( char* pModule, int* flagp )
{
  FILE *fid = NULL;
  char buf[128] = "";

  *flagp = _fINI;
  fid = fopen(_pSYSTEM,"r");
  if (!fid) goto done;

  while (fgets(buf,127,fid))
  {
    if (buf[0] == '#') continue;
    if (!strstr(buf,pModule)) continue;
    {
      char *ptr = buf;
      int flag = 0;

      while (*ptr && (*ptr != '='))
        ++ptr;
      if (*ptr == '=') ++ptr;
      if (!(*ptr) || (sscanf(ptr,"%d",&flag) != 1))
      {
        logit("", "ewInitDebugFlag(): sscanf() failed! buf[%s]\n", buf);
        goto done;
      }
      *flagp = flag;
      break;
    }
  }

 done:
  if (fid) fclose(fid);
  if (*flagp) logit("e","ewInitDebugFlag(): %s[%d]\n", pModule, *flagp);
  return 0;
}

static int trace_log( TRACE2_HEADER* );

/* function: _writeTraceDataRaw()
 *    input: SOCKET soc, socket descriptor;
 *           TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           CLIENT_MSG* msg, which contains the following info:
 *               double tS, starting time of reqested period;
 *               double tE, ending time of reqested period;
 *               char* fill, ascii string that fills sampling gaps
 *   return: R_SKIP if tE <= tS;
 *           R_FAIL or R_DONE otherwise
 *
 *  purpose: to serve as one of the two entry points from server_thread.c,
 *           by responding to user requests of raw binary data for a given
 *           period of time.  In doing so, the following events occur:
 *           1) tank file is locked,
 *           2) left and right offsets in tank file are found,
 *           3) data within this range is copied to memory,
 *           4) the lock is released,
 *           5) data is sent to socket with proper header information
 */

int _writeTraceDataRaw( SOCKET soc, TANK* t, CLIENT_MSG* msg )
{
  int ret = R_FAIL;
  long int oS = -1L;          /* starting offset for CopyReqData() */
  long int oE = -1L;          /* ending offset for CopyReqData() */
  double tS = 0.0;
  double tE = 0.0;
  double acttS = 0.0;         /* actual time stamp at oS */
  double acttE = 0.0;         /* actual time stamp at oE */
  long int oL = -1L;          /* starting tank offset for SendReqDataRaw() */
  long int oR = -1L;          /* ending tank offset for SendReqDataRaw() */
  double tL = 0.0;            /* time stamp on left edge of the tank */
  double tR = 0.0;            /* time stamp on right edge of the tank */
  char* data = (char*) NULL;
  long int datasize = 0L;
  int mutexLocked = 0;
  long int saveOff = 0;      /* saving offset of file */
  long AsyncDataSize = 0;
  long AsyncPacketCount= 0;
  long AsyncPageSize;
  char* AsyncData = NULL;
 
  if ( _fDBG == -1 )
    ewInitDebugFlag( _pMODULE, &_fDBG );

  if ( t == NULL || t->tfp == NULL ) goto abort;
  if ( !msg ) goto abort;
  tS = msg->starttime;
  tE = msg->endtime;
  if ( tS < 0.0 ) goto abort;
  if ( tS + DBL_FLOAT_EPSILON >= tE ) goto skip;

  if ( _fDBG & _fSUB )
    logit( "","\n--------------------------------\n" );

  RequestSpecificMutex(&(t->mutex));
  mutexLocked = 1;
  if ( (saveOff = ftell(  t->tfp )) < 0 )
  {
    logit( "","_writeTraceDataRaw(): ftell[%ld] failed for tank %s errno=%d %s\n",
           saveOff, t->tankName, errno, strerror(errno) );
    goto abort;
  }
    
  /* safe to get delimiters of tank once mutex is locked */
  if ( LocateStartOfTank( t, &oL, &tL ) )
    goto abort;
  if ( LocateEndOfTank( t, &oR, &tR ) )
    goto abort;
    
   /* Check to see if we have async packets in requested range.*/
   if(bUsePacketSyncDb) {
   	tbdb_get_all_packets(pPacketDb,t->sta, t->net, 
 		t->chan, t->loc, tS, tE, (void*) &AsyncData, &AsyncDataSize, 
 		&AsyncPageSize, &AsyncPacketCount);
   }
 
  if ( _fDBG & _fSUB )
  {
    logit( "","_writeTraceDataRaw(): [tS,tE] [%f,%f]\n",tS,tE );
    logit( "","                      [tL,tR] [%f,%f]\n",tL,tR );
    logit( "","                      [oL,oR] [%ld,%ld]\n",oL,oR );
  }
    
  if ( GetOffsetLeft( t, tS, &oS, &acttS ) != R_DONE ) 
  {
     logit( "","_writeTraceDataRaw(): GetOffsetLeft failed for tS=%f\n",tS );
     goto abort;
  }
  if ( GetOffsetRight( t, tE, &oE, &acttE ) != R_DONE ) 
  {
     logit( "","_writeTraceDataRaw(): GetOffsetRight failed for tE=%f\n",tE );
     goto abort;
  }

  /* SendReqDataRaw() will, if necessary, set FL, FR or FG based on
   * acttS and acttE in comparison with tS and tE,
   * hence don't goto skip here even if ( acttE <= acttS );
   *
   * however, since the wrapping nature of the tank, it is likely
   * oS > oE, hence CopyReqData() need be guarded by the comparison
   * of acctS and acctE;
   *
   * when [tS,tE] falls entirely in a gap, acttE < tS < tE < acttS,
   * it is clear that acttE < acttS, and CopyReqData() need not be called
   *
   * when [tS,tE] falls entirely in index range, or when there is an
   * overlap of [tS,tE] and [acttS,acttE], one of following is true:
   *   1) acttS < tS < tE < acttE,
   *   2) tS < acttS < acttE < tE,
   *   3) acttS < tS < acttE < tE,
   *   4) tS < acttS < tE < acttE,
   * which is equivalent to ( acttS < tE && tS < acttE );
   *
   */

  if ( ( acttS < tE && tS < acttE ) &&
       ( acttS + DBL_FLOAT_EPSILON > acttE ) )
  {
    logit( "","_writeTraceDataRaw(): warning! acttS[%f] acttE[%f] for tank %s\n",
           acttS,acttE, t->tankName );
  }

  if ( ( acttS < tE && tS < acttE ) &&
       ( acttS + DBL_FLOAT_EPSILON <= acttE) )
  {
     if ( (CopyReqData( t, oS, oE, (char**) &data, &datasize ) == R_FAIL) && !AsyncPacketCount )
     {
       logit( "","_writeTraceDataRaw(): CopyReqData failed for oS=%ld oE=%ld\n", oS,oE );
       goto abort;
     }
  }
    
  /* resync packets if we have any*/
  if(AsyncPacketCount && AsyncData) {
 	  MergeAsyncPackets(t, &data, &datasize, AsyncData, AsyncDataSize, AsyncPacketCount);
 	  free(AsyncData);
  }
  ReleaseSpecificMutex(&(t->mutex));
  mutexLocked = 0;

  if ( SendReqDataRaw( soc, t, tS, tE, tL, tR, acttS, acttE,
		       data, datasize, msg->reqId ) != R_DONE)
  {
    logit( "","_writeTraceDataRaw(): SendReqDataRaw failed for tS=%f tE=%f\n", tS, tE );
    goto abort;
  }
  ret = R_DONE;
 skip:
  if ( ret != R_DONE)
  {
    ret = R_SKIP;
    if ( _fDBG & _fSUB )
      logit( "et","_writeTraceDataRaw(): skips processing\n" );
  }
 abort:
  if ( ret == R_FAIL )
  {
    logit( "et","_writeTraceDataRaw(): failed\n" );
  }
  if ( mutexLocked ) 
    ReleaseSpecificMutex(&(t->mutex));

  if ( data )
    FreeReqData( data );
  return ret;
}

/* function: _writeTraceDataAscii()
 *    input: SOCKET soc, socket descriptor;
 *           TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           CLIENT_MSG* msg, which contains the following info:
 *               double tS, starting time of reqested period;
 *               double tE, ending time of reqested period;
 *               char* fill, ascii string that fills sampling gaps;
 *           char* reqid, the client's request ID
 *   return: R_SKIP if tE <= tS;
 *           R_FAIL or R_DONE otherwise
 *
 *  purpose: to serve as one of the two entry points from server_thread.c,
 *           by responding to user requests of raw ASCII data for a given
 *           period of time.  In doing so, the following events occur:
 *           1) tank file is locked,
 *           2) left and right offsets in tank file are found,
 *           3) data within this range is copied to memory,
 *           4) the lock is released,
 *           5) data is sent to socket with proper header information
 */

int _writeTraceDataAscii( SOCKET soc, TANK* t, CLIENT_MSG* msg )
{
  int ret = R_FAIL;
  long int oS = -1L;          /* starting offset for CopyReqData() */
  long int oE = -1L;          /* ending offset for CopyReqData() */
  double tS = 0.0;
  double tE = 0.0;
  char* fill = (char*) NULL;
  double acttS = 0.0;         /* actual time stamp at oS */
  double acttE = 0.0;         /* actual time stamp at oE */
  long int oL = -1L;          /* starting tank offset for SendReqDataAscii() */
  long int oR = -1L;          /* ending tank offset for SendReqDataAscii() */
  double tL = 0.0;            /* time stamp on left edge of the tank */
  double tR = 0.0;            /* time stamp on right edge of the tank */
  char* data = (char*) NULL;
  long int datasize = 0L;
  int mutexLocked = 0;
  long int saveOff = 0;      /* saving offset of file */
  long AsyncDataSize = 0;
  long AsyncPacketCount= 0;
  long AsyncPageSize;
  char* AsyncData = NULL;

  if ( _fDBG == -1 )
    ewInitDebugFlag( _pMODULE, &_fDBG );

  if ( t == NULL || t->tfp == NULL ) goto abort;
  if ( !msg ) goto abort;
  tS = msg->starttime;
  tE = msg->endtime;
  fill = msg->fillvalue;
  if ( tS < 0.0 ) goto abort;
  if ( tS + DBL_FLOAT_EPSILON >= tE ) goto skip;

   if ( _fDBG & _fSUB )
    logit( "","\n================================\n" );

  RequestSpecificMutex(&(t->mutex));
  mutexLocked = 1;
  if( (saveOff = ftell( t->tfp )) < 0 )
  {
    logit( "","_writeTraceDataAscii(): ftell[%ld] failed for tank %s\n",
           saveOff, t->tankName );
    goto abort;
  }
    
  /* safe to get delimiters of tank once mutex is locked */
  if ( LocateStartOfTank( t, &oL, &tL ) )
    goto abort;
  if ( LocateEndOfTank( t, &oR, &tR ) )
    goto abort;
    
  /* Check to see if we have async packets in requested range.*/
  if(bUsePacketSyncDb) {
  	tbdb_get_all_packets(pPacketDb,t->sta, t->net, 
		t->chan, t->loc, tS, tE, (void*) &AsyncData, &AsyncDataSize, 
		&AsyncPageSize, &AsyncPacketCount);
  }

  if ( _fDBG & _fSUB )
  {
    logit( "","_writeTraceDataAscii(): [tS,tE] [%f,%f]\n",tS,tE );
    logit( "","                   [tL,tR] [%f,%f]\n",tL,tR );
    logit( "","                   [oL,oR] [%ld,%ld]\n",oL,oR );
  }
    
  if ( GetOffsetLeft( t, tS, &oS, &acttS ) != R_DONE ) goto abort;
  if ( GetOffsetRight( t, tE, &oE, &acttE ) != R_DONE ) goto abort;

  /* SendReqDataAscii() will, if necessary, set FL or FR based on
   * acttS and acttE in comparison with tS and tE,
   * hence don't goto skip here even if ( acttE <= acttS );
   *
   * however, since the wrapping nature of the tank, it is likely
   * oS > oE, hence CopyReqData() need be guarded by the comparison
   * of acctS and acctE;
   *
   * when [tS,tE] falls entirely in a gap, acttE < tS < tE < acttS,
   * it is clear that acttE < acttS, and CopyReqData() need not be called
   *
   * when [tS,tE] falls entirely in index range, or when there is an
   * overlap of [tS,tE] and [acttS,acttE], one of following is true:
   *   1) acttS < tS < tE < acttE,
   *   2) tS < acttS < acttE < tE,
   *   3) acttS < tS < acttE < tE,
   *   4) tS < acttS < tE < acttE,
   * which is equivalent to ( acttS < tE && tS < acttE );
   *
   */

  if ( ( acttS < tE && tS < acttE ) &&
       ( acttS + DBL_FLOAT_EPSILON > acttE ) && !AsyncPacketCount)
  {
    logit( "","_writeTraceDataAscii(): warning! acttS[%f] acttE[%f], no data available for request in tank %s\n",
           acttS,acttE,t->tankName );
  }

  if ( ( acttS < tE && tS < acttE ) &&
       ( acttS + DBL_FLOAT_EPSILON <= acttE) )
  {
    if ( (CopyReqData( t, oS, oE, (char**) &data, &datasize ) == R_FAIL) && !AsyncPacketCount)
       goto abort;
  }
    

  /* resync packets if we have any*/
  if(AsyncPacketCount && AsyncData) {
	  MergeAsyncPackets(t, &data, &datasize, AsyncData, AsyncDataSize, AsyncPacketCount);
 	  free(AsyncData);
  }

  ReleaseSpecificMutex(&(t->mutex));
  mutexLocked = 0;

  if ( SendReqDataAscii( soc, t, tS, tE, tL, tR, acttS, acttE,
			 fill, data, datasize, msg->reqId ) != R_DONE)
    goto abort;
  ret = R_DONE;
 skip:
  if ( ret != R_DONE)
  {
    ret = R_SKIP;
    if ( _fDBG & _fSUB )
      logit( "","_writeTraceDataAscii(): skips processing\n" );
  }
 abort:
  if ( ret == R_FAIL )
  {
    logit( "","_writeTraceDataAscii(): failed\n" );
  }
  if ( mutexLocked ) 
    ReleaseSpecificMutex(&(t->mutex));
   
  if ( data )
    FreeReqData( data );
  return ret;
}


/* function: GetOffset()
 *    input: TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           double reqtS, starting time of reqested period
 *   output: long int* offp, the offset that covers reqtS, which is set
 *               to the start of next index when reqtS falls in a gap;
 *           double* acttp, actual time that matches *offp;
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to find  offset in tank file (including associated
 *           time stamp) for the requested time period;
 *           called  from GetOffsetLeft() and GetOffsetRight();
 *           Used to illiminate redundancy between Left() and Right()
 *           Could improve performance by using #defines
 */
static int GetOffset( TANK* t, double reqTime,
                      long int *offp, double *acttp,
                      int DirectionFlag)
{
  int ret = -1;
  int curr = -1;  /* iterator */
  int prev = -1;  /* look back */
  int next = -1;  /* look forward */
  DATA_CHUNK * pCurr = (DATA_CHUNK *) 0;
  DATA_CHUNK * pNext = (DATA_CHUNK *) 0;
  char szFuncName[2][20] = {"GetOffsetLeft", "GetOffsetRight"}; 

  *offp = -1L;
  *acttp = 0.0;

  if ( t == NULL || t->tfp == NULL  || t->samprate < DBL_FLOAT_EPSILON )
  {
    logit( "","%s(): bad parameters\n",szFuncName[DirectionFlag] );
    logit( "","%s(): failed\n",szFuncName[DirectionFlag] );
    ret=R_FAIL;
  }

  /* We are not using pointers now, we are using array indexes.  This seems kind
     of messy, because the list is not encapsulated.  It would probably be a 
     good idea to encapsulate the list, and use GetNextChunk(TANK *,DATA_CHUNK *),
     but for now, I will just code the index. Cleanup: 
  */

  /* Make sure the tank is not empty */
  if ( IndexOldest( t )->tEnd == 0.0 )
  {
    logit( "","%s(): Empty tank %s\n",szFuncName[DirectionFlag], t->tankName);
    logit( "","%s(): failed\n",szFuncName[DirectionFlag]);
    ret=R_FAIL;
  }

  /* Set curr and next */
  curr=t->indxStart;
  if(t->indxStart >= ((unsigned int)t->indxMaxChnks)) /* t->indxStart is an unsigned int and cannot be <0 */
  {
    /* t->indxStart is out of whack */
    logit("et","wave_serverV.GetOffset(): Bad t->indxStart %d, for tank %s\n",
          t->indxStart, t->tankName );
  }

  /* Make sure there is a next */
  if(curr != (int)t->indxFinish)
  {
    next=curr+1;
    if(next == t->indxMaxChnks)
    {
      next=0;
    }
  }

  while(prev != (int)t->indxFinish && ret != R_DONE  && ret !=R_FAIL)
  {
    pCurr=&((t->chunkIndex)[curr]);
    /* check to see if the current index doesn't contain data that is too young.
       Remember to allow for timestamp slop. */
    if( (reqTime < (pCurr->tEnd + GRACE_PERIOD)  && 
        DirectionFlag == FLAG_L) 
       ||
        (reqTime <= (pCurr->tEnd + GRACE_PERIOD) && 
        DirectionFlag == FLAG_R)
      )
    {
      /* We've found a piece of trace that is not too young */
      /* check to see if the current index doesn't isn't too old
         Remember to allow for timestamp slop. */
      if( (reqTime <= (pCurr->tStart - GRACE_PERIOD) && 
          DirectionFlag == FLAG_L) 
         ||
          (reqTime < (pCurr->tStart - GRACE_PERIOD) && 
          DirectionFlag == FLAG_R)
        )
      { 
        /* !!oops, either too young, or in the gap just before this junk */
        /* set the offset to here, and note that it occured in GAP */
        if (Debug > 1)
        {
             logit("t","Found gap, or too young at curr=%d, setting offset %d\n",
																curr,pCurr->offset);
             logit("t","Curr-Start %f,Curr->End %f,reqTime %f!\n",
													pCurr->tStart,pCurr->tEnd,reqTime);
             logit("t","Tank %s  ",t->tankName );
             if(DirectionFlag == FLAG_L) 
                logit("","Called for start of request.\n***************************\n");
             else
                logit("","Called for end of request.\n***************************\n");
        }

        if(DirectionFlag == FLAG_L)
        {
          *offp = pCurr->offset;
          *acttp = pCurr->tStart;
        }
        else
        {
          *offp = pCurr->offset;
          if(prev < 0)
          {
            /* DK Cleanup:  logit("t","Wave_serverV.GetOffset():  Requested trace is left of tank.\n");
             */
            *acttp = 0.0;
          }
          else
          {
            *acttp=t->chunkIndex[prev].tEnd;
          }
        }
          
        if ( _fDBG & _fALG )
        {
          if ( prev == -1 )
            logit( "","  %s(GAP): no prev!\n",szFuncName[DirectionFlag]);
          else
            logit("","  %s(GAP): previous tEnd[%f]\n",
                  szFuncName[DirectionFlag], t->chunkIndex[prev].tEnd);
          logit( "","                      off[%ld] actt[%lf]\n",*offp,*acttp );
        }
        ret = R_DONE;
      }
      else
      {
        /* OK, it's in this chunk somewhere, sick LocateExactOffset on it */
        if ( _fDBG & _fALG )
          logit( "","  %s(): tS in   [%f,%f]\n", szFuncName[DirectionFlag],
                 pCurr->tStart, pCurr->tEnd );
        /* First, create a pNext */
        if(curr != (int)t->indxFinish)
        {
          next=curr+1;
          if(next == t->indxMaxChnks)
          {
            next=0;
          }
          pNext=&(t->chunkIndex[next]);
        }
        else
        {
          pNext=0;
        }
        /* Then call LocateExactOffset */
        ret = LocateExactOffset( t, pCurr, reqTime, DirectionFlag, offp, acttp,
                                 pNext );
        if(ret == R_FAIL)
        {
          logit("et","LocateExactOffset failed for tank %s.\n Reqt %f,Start %f, Finish %f,offset %d, InsrtPt %d,FLAG_L %d, GRACE_PERIOD %f\n",
                t->tankName, reqTime,pCurr->tStart,pCurr->tEnd,pCurr->offset,t->inPtOffset,DirectionFlag==FLAG_L,GRACE_PERIOD);
        }
      }
    }
    else
    {
      /* Not there yet, keep going */
      prev = curr;
      curr+=1;
      if(curr == t->indxMaxChnks)
      {
        curr=0;
      }
    }
  }  /* End of while() not finished loop */
  if(prev == (int)t->indxFinish)
  {
    /* Request was too young, crap out, or do something to indicate failure */
    if ( _fDBG & _fALG )
      logit( "","  %s(): tS requested too young\n", szFuncName[DirectionFlag]);
    ret = LocateEndOfTank( t,offp,acttp );
  }
  return(ret);
}

/* function: GetOffsetLeft()
 *    input: TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           double reqtS, starting time of reqested period
 *   output: long int* offp, the offset that covers reqtS, which is set
 *               to the start of next index when reqtS falls in a gap;
 *           double* acttp, actual time that matches *offp;
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to find left offset in tank file (including associated
 *           time stamp) for the requested time period;
 *           called only once from _writeTraceDate();
 *           see also symmetric function GetOffsetRight()
 */
static int GetOffsetLeft( TANK* t, double reqtE,
			  long int *offp, double *acttp )
{
  return(GetOffset(t,reqtE,offp,acttp,FLAG_L));
}


/* function: GetOffsetRight()
 *    input: TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           double reqtE, ending time of reqested period;
 *   output: long int* offp, the offset that covers reqtE, which is set
 *               to the end of previous index when reqtE falls in a gap;
 *           double* acttp, actual time that matches *offp;
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to find right offset in tank file (including associated
 *           time stamp) for the requested time period;
 *           called only once from _writeTraceDate();
 *           see also symmetric function GetOffsetLeft()
 */

static int GetOffsetRight( TANK* t, double reqtE,
                           long int *offp, double *acttp )
{
  return(GetOffset(t,reqtE,offp,acttp,FLAG_R));
}

/* function: LocateExactOffset()
 *    input: TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           DATA_CHUNK* idx, which meets both conditions listed below:
 *             1) idx->tStart <= reqt,
 *             2) idx->tEnd >= reqt;
 *           double reqt, requested time;
 *           int flag, either FLAG_L or FLAG_R;
 *   output: long int* offp, offset located within idx;
 *           double* acttp, actual time that matches *offp;
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to find exact offset in a given index structure that is
 *           to the immediate left/right of the requested time;
 *           could be called no more than once from GetOffsetLeft() or
 *           GetOffsetRight() respectively (denoted by FLAG_L or FLAG_R)
 */

static int LocateExactOffset( TANK* t, DATA_CHUNK* idx, double reqt, int flag,
                              long int* offp, double* acttp, DATA_CHUNK* next )
{
  int ret = R_FAIL;
  long int off = -1L;
  double actt = 0.0;
  long int oE = -1L;
  FILE* tfp = (FILE*) NULL;
  TRACE2_HEADER trace_h = { 0 };
  int rsz = 0;
  int tsz = 0;
  int DFIP;
  int tmp_point;
  int RoughOff;
  double RoughActt;

  /* Variables to handle recording of File I/O errors */
  int iFileOffset = 0, iFileBlockSize = 0;
  char * szFileTankName = NULL;
  char szFileFunction[15];

  *offp = off;
  *acttp = actt;
    
  if ( t == NULL || ( tfp = t->tfp ) == NULL || ( rsz = t->recSize ) == 0 ||
       ( tsz = t->tankSize) == 0 || idx == NULL )
  {
    logit( "","LocateExactOffset(): t[%0x] tfp[%0x] tsz[%0x] idx[%0x]\n",
           t, tfp, tsz, idx );
    goto abort;
  }
    
  if ( _fDBG & _fALG )
  {
    logit( "","    LocateExactOffset(%c):  [%f,%f]\n",
           flag == FLAG_L ? 'L':'R', idx->tStart, idx->tEnd );
    logit( "","                           [%f]\n", reqt );
  }

  if ( next )
  {
    oE = next->offset;
  }
  else
  {
    double tE = 0.0;
    if ( LocateEndOfTank( t, &oE, &tE ) != R_DONE )
      goto abort;
  }
  if ( LocateRoughOffset( t, idx->tStart, idx->tEnd, reqt,
			  idx->offset, oE, flag, &off, &actt ) != R_DONE )
  {
    logit("et","Wave_serverV:LocateRoughOffset failed for tank %s within LocateExactOffset\n",
	  t->tankName );
    goto abort;
  }
    
  RoughOff=off;
  RoughActt=actt;
  /* wrap oE for ease of processing in the following while loop */
  if ( oE == tsz )
    oE = 0L;

  /* FLAG_L and FLAG_R are distinguished by LocateRoughOffset();
   * off and actt both mark the beginning of a record with FLAG_L,
   * and they mark the end of a record with FLAG_R
   */
    
  /* Calculate the distance from the current offset to the insertion point
     and then monitor it as to prevent us from crossing it forwards or backwards.
  */
  /* First adjust to middle of the current record as opposed to the front or end */
  if(flag == FLAG_L)
  {
    tmp_point=off+(int)(0.5*rsz);
  }
  else
  {
    tmp_point=off-(int)(0.5*rsz);
  }

  /* Next Calculate the Distance From Insertion Point */
  DFIP=t->inPtOffset-tmp_point;
  while(DFIP < 0)
  {
    DFIP+=tsz;
  }
  while(DFIP >tsz)
  {
    DFIP-=tsz;
  }
  /* Now, everytime the offset is moved, adjust the DFIP */


  /*  Okay, here's the new plan.  We're gonna overhaul this, so that we start checking
      time windows for each record, instead of just checking the start or
      end of the record.

      The logic is:
      starting with the record that is returned from LocateRoughOffset(), read
      the start and end times of the current record.  If the reqt is within
      the record start and endtime (non-inclusive), then we're done, ship the
      starttime and offset for that record (for FLAG_L, for FLAG_R, ship the
      endtime and offset+recsz).  If the record does not contain the reqt, then
      if the reqt < the starttime of the record, throw the search engine in reverse,
      keeping track of the current offset and starttime or offset and endtime.
      While the reqt is greater or equal to the endtime of the record, keep going
      until we hit the index or tank wall(Insertion point).  If we hit the wall, 
      blow up, and goto OHCRAP!!!

      Reverse:
      Go back the other direction until we pass the reqt going the other way.
      Then return the previous offset and Start or EndTime depending on L or R
  */

  /* need search back and forth */
  if ( flag == FLAG_L )
  {
    if ( ReadBlockData(tfp, off, (long int) sizeof(TRACE2_HEADER),
                       (char*) &trace_h ) != R_DONE )
    {
      strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
      szFileFunction[sizeof(szFileFunction)-1] = '\0';
      iFileOffset     = off;
      iFileBlockSize  = sizeof(TRACE2_HEADER);
      szFileTankName  = t->tankName;   /* don't copy just point */
      goto ReadBlockDataFailed;
    }
    /* if reqt is to the right keep going */
    while ((trace_h.endtime + GRACE_PERIOD) < reqt)  
    {
      off += rsz;
      DFIP-=rsz;
      if(DFIP <= 0 || DFIP >= tsz)
      {
        logit("t","Failed DFIP comparison #1\n");
        goto OHCRAP;
      }

      /* for the left boundary, set off = 0 if off == tsz */
      if ( off >= tsz )
        off =0;

      if ( ReadBlockData( tfp, off, (long int) sizeof(TRACE2_HEADER),
                          (char*) &trace_h ) != R_DONE )
      {
        strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
        szFileFunction[sizeof(szFileFunction)-1] = '\0';
        iFileOffset     = off;
        iFileBlockSize  = sizeof(TRACE2_HEADER);
        szFileTankName  = t->tankName;   /* don't copy just point */
        goto ReadBlockDataFailed;
      }
    }  /* End while(acct < reqt) */

    if(trace_h.starttime  - GRACE_PERIOD <= reqt)
    {
      /* We've found our spot.  trace_h.starttime <= reqt <= trace_h.endtime.
         Pack it up and go home */
      *acttp=trace_h.starttime;
      *offp=off;
    }
    else
    {
      /* There was some sort of a minor snaffu.  Throw it in reverse,
         but keep track of what we've got.
      */
      int tempOff=-1;
      double tempStart=-1.0;

      while (trace_h.starttime - GRACE_PERIOD > reqt )
        /* Use GRACE_PERIOD to account for sloppy timestamps */
      {
        /* Keep going backwards until we hit the wall, or get to a good time */
        tempOff=off;
        tempStart=trace_h.starttime;
        off -= rsz;
        DFIP += rsz;
        if(DFIP <= 0 || DFIP >= tsz)
        {
          logit("t","Failed DFIP comparison #2\n");
          goto OHCRAP;
        }
        if ( off < 0L )
          off += tsz;
        if ( ReadBlockData( tfp, off, (long int) sizeof(TRACE2_HEADER),
                            (char*) &trace_h ) != R_DONE )
        {
          strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
          szFileFunction[sizeof(szFileFunction)-1] = '\0';
          iFileOffset     = off;
          iFileBlockSize  = sizeof(TRACE2_HEADER);
          szFileTankName  = t->tankName;   /* don't copy just point */
          goto ReadBlockDataFailed;
        }
      }
      /* OK, we're here, I guess */
      if(trace_h.starttime - GRACE_PERIOD <= reqt && 
         reqt <= tempStart - GRACE_PERIOD)
      {
        /* OK, we are definitely here now. 
           We just need to figure out where we are.
        */
        if(reqt < trace_h.endtime + GRACE_PERIOD )
        {
          /* We can get atleast one sample from this rec, so
             use it.
          */
          *offp=off;
          *acttp=trace_h.starttime;
        }
        else
        {
          /* We can't get any data out of this sample, so let's
             go back to the one where we just were.
          */
          *offp=tempOff;
          *acttp=tempStart;
        }
      }
      else  /* if(trace_h.starttime - GRACE_PERIOD < reqt  
               && reqt <= tempStart - GRACE_PERIOD) */
      {
        /* Anything that lands here would be a bug */
        logit("t","Lost in LocateExactOffset for tank %s\n", t->tankName );
        logit("t","th.start %f th.end %f off %d tempOff %d tempStart %f reqt %f\n",
              trace_h.starttime,trace_h.endtime,off,tempOff,tempStart,reqt);
      }
    } /* End else from if(trace_h.starttime <= reqt) */
  }  /* End   if ( flag == FLAG_L ) */
  else  /*  flag == FLAG_R */
  {
    long int tmp = off-rsz;

    if(tmp < 0)
      tmp+=tsz;

    if ( ReadBlockData(tfp, tmp, (long int) sizeof(TRACE2_HEADER),
                       (char*) &trace_h ) != R_DONE )
    {
      strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
      szFileFunction[sizeof(szFileFunction)-1] = '\0';
      iFileOffset     = tmp;
      iFileBlockSize  = sizeof(TRACE2_HEADER);
      szFileTankName  = t->tankName;   /* don't copy just point */
      goto ReadBlockDataFailed;
    }
    while (trace_h.starttime > reqt)  /* if reqt is to the left keep going */
    {
      off=tmp;
      tmp-=rsz;
      DFIP+=rsz;
      if(DFIP <= 0 || DFIP >= tsz)
      {
        logit("t","Failed DFIP comparison #3\n");
        goto OHCRAP;
      }

      /* for the left boundary, set off = 0 if off == tsz */
      if ( tmp < 0 )
        tmp += tsz;
      
      if ( ReadBlockData( tfp, tmp, (long int) sizeof(TRACE2_HEADER),
                          (char*) &trace_h ) != R_DONE )
      {
        strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
        szFileFunction[sizeof(szFileFunction)-1] = '\0';
        iFileOffset     = tmp;
        iFileBlockSize  = sizeof(TRACE2_HEADER);
        szFileTankName  = t->tankName;   /* don't copy just point */
        goto ReadBlockDataFailed;
      }
    }  /* End while(acct < reqt) */

    if(trace_h.endtime + GRACE_PERIOD >= reqt)
    {
      /* We've found our spot.  trace_h.starttime < reqt <= trace_h.endtime.
         Pack it up and go home */
      *acttp=trace_h.endtime;
      *offp=off;
    }
    else
    {
      /* There was some sort of a minor snaffu.  Throw it in reverse,
         but keep track of what we've got.
      */
      double tempEnd=trace_h.endtime;

      while (trace_h.endtime + GRACE_PERIOD < reqt)
        /* Use GRACE_PERIOD to account for sloppy timestamps */
      {
        /* Keep going backwards until we hit the wall, or get to a good time */
        /* LocateRoughOffset can return off = tsz, and if the rough-offset return
           was ahead of our target time, we could read off the end of the tank.
           DK 20100311 */
        if ( off >=tsz ) 
          off=0;
        tmp=off;
        off += rsz;
        DFIP -= rsz;
        if(DFIP <= 0 || DFIP >= tsz)
        {
          logit("t","Failed DFIP comparison #4\n");
          goto OHCRAP;
        }
        if ( off >=tsz )
          off=0;
        tempEnd=trace_h.endtime;
        if ( ReadBlockData( tfp, tmp, (long int) sizeof(TRACE2_HEADER),
                            (char*) &trace_h ) != R_DONE )
        {
          strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
          szFileFunction[sizeof(szFileFunction)-1] = '\0';
          iFileOffset     = tmp;
          iFileBlockSize  = sizeof(TRACE2_HEADER);
          szFileTankName  = t->tankName;   /* don't copy just point */
          goto ReadBlockDataFailed;
        }
      }
      /* OK, we're here, I guess */
      if(tempEnd + GRACE_PERIOD <= reqt && reqt <= trace_h.endtime + GRACE_PERIOD)
      {
        /* OK, we are definitely here now. 
           We just need to figure out where we are.
        */
        if(reqt >= trace_h.starttime + GRACE_PERIOD)
        {
          /* We can get atleast one sample from this rec, so
             use it.
          */
          *offp=off;
          *acttp=trace_h.endtime;
        }
        else
        {
          /* We can't get any data out of this record, so let's
             go back to the one where we just were.
          */
          *offp=tmp;
          *acttp=tempEnd;
        }
      }
      else  /* if(tempEnd < reqt && reqt < trace_h.endtime + GRACE_PERIOD) */
      {
        /* Anything that lands here would be a bug */
        logit("t","Lost in LocateExactOffset for tank %s\n", t->tankName);
        logit("t","th.start %f th.end %f off %d tempOff %d tempEnd %f\n",
              trace_h.starttime,trace_h.endtime,off,tmp,tempEnd);
      }
    } /* End else from if(trace_h.endtime >= reqt) */
  }  /* end else from if FLAG_L */

  ret = R_DONE;

 ReadBlockDataFailed:
  if ( ret != R_DONE )
  {
    IssueIOStatusError(szFileFunction,iFileOffset,iFileBlockSize,szFileTankName);
    goto abort;
  }
 OHCRAP:
  if ( ret != R_DONE )
  {
    logit("t","Wave_server: LocateExactOffset(): reached the index boundary without reaching reqt.\n reqt %f off %d oE %d inspt %d idxofst %d\n",
          reqt,off,oE,t->inPtOffset,idx->offset);
    logit("t","Wave_server: LocateExactOffset(): DFIP %d,tsz %d, RoughOff %d, RoughActt %f, Tank %s\n",DFIP,tsz,RoughOff,RoughActt,t->tankName);
  }
 abort:
  if ( ret != R_DONE )
    logit( "","LocateExactOffset(): failed\n" );
  return ret;
}

/* function: LocateRoughOffset()
 *    input: TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           double tS, starting time denoting beginning of range,
 *           double tE, ending time denoting end of range,
 *           double reqt, user requested time,
 *           long int oS, starting offset corresponding to tS,
 *           long int oE, ending offset corresponding to tE;
 *           int flag, either FLAG_L or FLAG_R;
 *   output: long int* offp, offset located within the range of [oS,oE],
 *           double* acttp, actual time that matches *offp;
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to find rough offset in a given index structure that is
 *           close to the requested time, so that sequential search
 *           in this neighborhood would be inexpensive;
 *           called only by GetExactOffset() or GetExactOffset(), and
 *           FLAG_L or FLAG_R is passed as flag for differentiation
 *
 *     note: tanksize is multiple of record size, otherwise disastous
 *           alignment of block data read error ensues;
 */

/*******************************************************************************
*******************************************************************************
WARNING!!! BUG IN LocateRoughOffset()

 At first glance, there appears to be a bug in wave_serverV that affects tank
files over 1,073,741,823 bytes in size.  Code within LocateRoughOffset() in
serve_trace.c, does a buffer calculation, that potentially overflows a signed
integer value when the tanksize is >= 1 GB.

The error affects only data retrieval, and only when the offset of the
chronological-start of the tank is near the end of the tank.  Specifically,
if  TankSize + oS (offset of chronological-start) >= 2GB, then an error
will result, and no data will be returned for the tank.  (Once the offset
wraps around to the beginning of the tank again, everything should
function normally)

Since your tank is just barely over 1 GB, you are seeing the errors only
when the chronological-start offset gets near the end of the tank.  If you
were using tank sizes ~ 2GB, you would experience the problem
constantly.

The problem appears to result from a signed integer overflow of tmpoE
at Line 1155 of serve_trace.c  (LocateRoughOffset()).
An example from your logfile:

> LocateRoughOffset(): warning! oE[1029091392] tmpoE[-2145876160] off[2145508992] for tank i:\nano1_MDH1_EP1_NC.tnk
>    ==> offset == [-2145877024] < 0


oE = the chronological-end of the tank  (once the tank has wrapped, the chronological-end and chronological-start are adjacent.)
tmpoE = the chronological-end of the tank + the tanksize
tmpoE = 1029091392(oE) + 1,119,999,744 (tanksize) =   0x801883E0 which as an unsigned int = -2145877024
which is < 0. This is treated as an error, and an error flag is returned to the client instead of the requested data.


This is my analysis based upon 30 minutes of inspection.  Besides the error that I describe here,
there may be other issues with tanksizes > 1 GB.  A reliable code fix would require further inspection
and thorough testing.  Also, I may have mis-analyzed the code and error, and the problem could
potentially be something different.

An immediate fix, would be to limit your tank files to < 1 GB.  (This may be impractical for you, if
you have a good deal of data in the current tanks that you do not wish to lose, and have no way
to transfer it.)

Unless someone disagrees with my diagnosis, or wishes to immeidately have the problem investigated
further, it would probably be a good idea to send a message out to the EW list, warning against
using tanksizes >= 1GB.

DK 2005/03/09  (comment from email 2004/01/29)
*******************************************************************************
*******************************************************************************/

static int LocateRoughOffset( TANK* t, double tS, double tE, double reqt,
                              long int oS, long int oE, int flag,
                              long int* offp, double* acttp )
{
  int ret = R_FAIL;
  long int off = -1L;
  double actt = 0.0;
  long int tmpoS = oS;
  long int tmpoE = oE;
  FILE* tfp = (FILE*) NULL;
  TRACE2_HEADER trace_h = { 0 };
  long int nRec = 0L;

  /* Variables to handle recording of File I/O errors */
  int iFileOffset = 0, iFileBlockSize = 0;
  char * szFileTankName = NULL;
  char szFileFunction[15];
  int  bIssueIOStatusError = FALSE;

  *offp = off;
  *acttp = actt;
  if ( t == NULL || ( tfp = t->tfp ) == NULL || !( t->recSize ) )
  {
    logit( "","LocateRoughOffset(): t[%0x] tfp[%0x] recSize[%d]\n",
           t, t?t->tfp:0, t?t->recSize:0 );
    goto abort;
  }
  if ( tE - tS < DBL_FLOAT_EPSILON )
  {
    logit( "","LocateRoughOffset(): [tE,tS] range too small for tank %s\n",
           t->tankName );
    goto abort;
  }
  if ( ( reqt < tS - GRACE_PERIOD) || ( reqt > tE + GRACE_PERIOD) )
  {
    logit( "","LocateRoughOffset(): reqt[%f] out of range for tank %s\n",
           reqt, t->tankName );
    goto abort;
  }
    
  if ( oS == t->tankSize ) tmpoS = 0L;
  if ( oE <= oS ) tmpoE += t->tankSize;
  nRec = (long int)( ( tmpoE-tmpoS ) / t->recSize * ( reqt-tS ) / ( tE-tS ) );
  off = tmpoS + nRec * t->recSize;
  if ( off > tmpoE )
  {
    logit( "","LocateRoughOffset(): warning! oE[%ld] tmpoE[%ld] off[%ld] for tank %s\n",
           oE, tmpoE, off, t->tankName );
    off = tmpoE - t->recSize;
  }
  if ( off >= t->tankSize ) off -= t->tankSize;

  if ( _fDBG & _fALG ) logit( "","      LocateRoughOffset( ): " );
  if ( ReadBlockData( tfp, off, (long int) sizeof(TRACE2_HEADER),
		      (char*) &trace_h ) != R_DONE )
  {
    logit( "","LocateRoughOffset(): ERROR ReadBlockData() failed\n" );
    strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
    szFileFunction[sizeof(szFileFunction)-1] = '\0';
    iFileOffset     = off;
    iFileBlockSize  = sizeof(TRACE2_HEADER);
    szFileTankName  = t->tankName;   /* don't copy just point */
    bIssueIOStatusError = TRUE;
    goto abort;
  }
  if ( _fDBG & _fALG ) logit( "","                            off[%ld], tS[%f]\n",
			      off,trace_h.starttime );

  if (flag == FLAG_L)
  {
    *offp = off;
    *acttp = trace_h.starttime;
  }
  else
  {
    *offp = off + t->recSize;
    *acttp = trace_h.endtime;
  }

  ret = R_DONE;
 abort:
  if ( ret != R_DONE )
  {
    if(bIssueIOStatusError)
    {
      IssueIOStatusError(szFileFunction,iFileOffset,iFileBlockSize,szFileTankName);
      bIssueIOStatusError = FALSE;
    }

    logit( "","LocateRoughOffset(): failed\n" );
  }
  return ret;
}

/* function: LocateStartOfTank()
 *  purpose: get offset and time stamp at the start of oldest index
 *    input: TANK* t
 *   output: long int* offp, which is set to t->indxStart->offset;
 *           double* acttp, which is set to t->indxStart->tStart
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to find offset and timestamp at beginning of tank
 *           called by various functions
 */

static int LocateStartOfTank( TANK* t, long int* offp, double* acttp )
{
  int ret = R_FAIL;
  DATA_CHUNK* idx = NULL;
    
  *offp = -1L;
  *acttp = 0.0;

  if ( t == NULL || ( idx = IndexOldest( t ) ) == NULL ) goto abort;

  *offp = idx->offset;
  *acttp = idx->tStart;

  if ( _fDBG & _fPRT )
    logit( "","    LocateStartOfTank():    indxS->off[%ld] indxS->sT[%lf]\n",
	   *offp, *acttp );
  ret = R_DONE;
 abort:
  if ( ret != R_DONE )
    logit( "","LocateStartOfTank(): failed\n" );
  return ret;
}

/* function: LocateEndOfTank()
 *  purpose: get offset and time stamp at the end of youngest index
 *    input: TANK* t, with following assumptions:
 *               t->tfp opened in "+br" mode;
 *               t->inPtOffset is valid insertion point;
 *   output: long int* offp, which is set to t->inPtOffset;
 *           double* acttp, which is set to t->indxStart->tStart
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to find offset and timestamp at end of tank;
 *           called by various functions
 */

static int LocateEndOfTank( TANK* t, long int* offp, double* acttp )
{
  int ret = R_FAIL;
  FILE* tfp = (FILE*) NULL;
  TRACE2_HEADER trace_h = { 0 };
  long int off = -1L;
  double actt = 0.0;

  /* Variables to handle recording of File I/O errors */
  int iFileOffset = 0, iFileBlockSize = 0;
  char * szFileTankName = NULL;
  char szFileFunction[15];
  int  bIssueIOStatusError = FALSE;
  
  *offp = off;
  *acttp = actt;

  if ( t == NULL || ( tfp = t->tfp ) == NULL ) goto abort;

  if ( _fDBG & _fPRT )
    logit( "","    LocateEndOfTank():      ");
  off = t->inPtOffset;
  /* in the case off == 0L, the tank has just wrapped to 0L, it is
   * convenient to set off = t->tankSize for subsequent processing
   */
  if ( !off ) off = t->tankSize;
  if ( off % t->recSize )
  {
    logit( "","LocateEndOfTank(): misaligned TRACE2_HEADER records for tank %s\n",
           t->tankName );
    goto abort;
  }

  if ( ReadBlockData( tfp, off - t->recSize, (long int) sizeof(TRACE2_HEADER),
		      (char*) &trace_h ) != R_DONE )
  {
    logit( "","LocateEndOfTank(): ERROR ReadBlockData() failed\n" );
    strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
    szFileFunction[sizeof(szFileFunction)-1] = '\0';
    iFileOffset     = off - t->recSize;
    iFileBlockSize  = sizeof(TRACE2_HEADER);
    szFileTankName  = t->tankName;   /* don't copy just point */
    bIssueIOStatusError = TRUE;
    goto abort;
  }

  *offp = off;
  *acttp = trace_h.endtime;
  if ( (_fDBG & _fALG) &&
       ( trace_h.endtime + DBL_FLOAT_EPSILON < IndexYoungest( t )->tEnd ||
	 trace_h.endtime - DBL_FLOAT_EPSILON > IndexYoungest( t )->tEnd ) )
  {
    logit( "","LocateEndOfTank(): warning! young->tEnd[%f] != eT[%f] for tank %s\n", 
           IndexOldest( t )->tEnd, *acttp, t->tankName );
  }

  if ( _fDBG & _fPRT )
    logit( "","                            inPt [%ld] endtime[%lf]\n",
	   off, *acttp );
  ret = R_DONE;

 abort:
  if ( ret != R_DONE )
  {
    if(bIssueIOStatusError)
    {
      IssueIOStatusError(szFileFunction,iFileOffset,iFileBlockSize,szFileTankName);
      bIssueIOStatusError = FALSE;
    }

    logit( "","LocateEndOfTank(): failed\n" );
  }
  return ret;
}

/* function: CopyReqData()
 *    input: TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           long int oS, starting offset for reqested period;
 *           long int oE, ending offset for reqested period,
 *             due to the bias flag in GetOffset, oE extends beyond
 *             the message with ending time tE in _writeTraceData{Raw|Ascii}(),
 *             i.e. even if [tS,tE] is very small, oS != oE,
 *             in other words, if tS == tE, the entire tank need be copied;
 *   output: char** data, records of t->recSize bytes each;
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to copy data from specified range in tank to memory;
 *           called only once from main _writeTraceData{Raw|Ascii}()
 */

static int CopyReqData( TANK* t, long int oS, long int oE,
                        char** datap, long int* sizep )
{
  int ret = R_FAIL;
  long int tsz = 0;          /* tank size */
  long int sz1 = 0, sz2 = 0; /* 2 fread()'s needed when oS >= oE */
  FILE* tfp = (FILE*) NULL;

  /* Variables to handle recording of File I/O errors */
  int iFileOffset = 0, iFileBlockSize = 0;
  char * szFileTankName = NULL;
  char szFileFunction[15];
  int  bIssueIOStatusError = FALSE;

  if ( _fDBG & _fALG )
    logit( "","CopyReqData():  offsets[%ld,%ld] for tank %s\n",oS,oE,
	   t->tankName);

  *datap = (char*) NULL;
  *sizep = 0;
  /* assume tank file already opened in mode "rb+" */
  if ( t == NULL || ( tfp = t->tfp ) == NULL || ( tsz = t->tankSize ) == 0 ) goto abort;
  if ( oS < 0 || oE < 0 ) goto abort;
  if ( oS > tsz || oE > tsz ) goto abort;
    
  if ( oS < oE )
  {
    sz1 = oE - oS;
    sz2 = 0;
  }
  else /* ( oS >= oE ) */
  {
    sz1 = tsz - oS;
    sz2 = oE;
  }
  if ( !sz1 && !sz2 ) goto mem_abort;
  *datap = (char*)malloc( ( sz1 + sz2 ) * sizeof( char ) );
  if ( !( *datap ) ) goto mem_abort;

  if ( sz1 && ReadBlockData( tfp, oS, sz1, &( (*datap)[0] ) ) != R_DONE )
  {
    logit( "", "CopyReqData(): read [%ld] bytes failed for tank %s\n",sz1,
           t->tankName );
    strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
    szFileFunction[sizeof(szFileFunction)-1] = '\0';
    iFileOffset     = oS;
    iFileBlockSize  = sz1;
    szFileTankName  = t->tankName;   /* don't copy just point */
    bIssueIOStatusError = TRUE;
    goto abort;     
  }
   
  if ( sz2 && ReadBlockData( tfp, 0L, sz2, &( (*datap)[sz1] ) ) != R_DONE )
  {
    logit( "", "CopyReqData(): read [%ld] bytes failed for tank %s\n",sz1,
           t->tankName );
    strncpy(szFileFunction,"ReadBlockData",sizeof(szFileFunction));
    szFileFunction[sizeof(szFileFunction)-1] = '\0';
    iFileOffset     = 0L;
    iFileBlockSize  = sz2;
    szFileTankName  = t->tankName;   /* don't copy just point */
    bIssueIOStatusError = TRUE;
    goto abort;     
  }

  *sizep = sz1 + sz2;
  ret = R_DONE;
 mem_abort:
  if ( ret == R_FAIL )
    logit( "","CopyReqData(): failed to malloc [%ld] bytes\n", sz1+sz2 );
 abort:
  if ( ret != R_DONE ) 
  {
    if(bIssueIOStatusError)
    {
      IssueIOStatusError(szFileFunction,iFileOffset,iFileBlockSize,szFileTankName);
      bIssueIOStatusError = FALSE;
    }

    if ( *datap )
      FreeReqData( *datap );
    logit( "","CopyReqData(): failed\n" );
  }
  return ret;
}

/* function: ReadBlockData()
 *    input: FILE* file, file opened in "br+" mode
 *           long int offset, start of block to be read
 *           long int length, size of block to be read
 *   output: char* data, data read in from file
 *   return: R_SKIP, if length == 0,
 *           R_FAIL or R_DONE otherwise
 *
 *  purpose: to read data in specified range in tank to memory;
 *           called by various functions
 */

static int ReadBlockData( FILE* file, long int offset, long int length,
                          char* data )
{
  int ret = R_FAIL;

  if ( _fDBG & _fPRT )
    logit( "","ReadBlockData(): offset[%ld] length[%ld]\n",
	   offset,length );
  /* assume tank file already opened in mode "rb+" */
  if ( !file )
  {
    logit( "","    ==> filep == NULL\n" );
    goto abort;
  }
  if ( offset < 0 )
  {
    logit( "","    ==> offset == [%ld] < 0\n",offset );
    goto abort;
  }
  if ( length <= 0 )
  {
    logit( "","    ==> length == 0\n" );
    goto skip;
  }
  if ( !data )
  {
    logit( "","    ==> data == NULL\n" );
    goto abort;
  }

  if ( fseek( file, offset, SEEK_SET ) != 0 )
  {
    logit( "","ReadBlockData(): fseek failed on offset [%ld]\n",offset );
    goto abort;
  }
  if( fread( (char*)data, length, 1, file ) != 1 ) 
  {
    logit( "", "ReadBlockData(): fread failed on length [%ld]\n",length );
    goto abort;
  }

  ret = R_DONE;
 skip:
  if ( ret == R_FAIL )
  {
    logit( "","ReadBlockData(): warning! skips upon 0 length\n" );
    ret = R_SKIP;
  }
 abort:
  if ( ret == R_FAIL )
    logit( "","ReadBlockData(): failed\n");

  return ret;
}

/* function: FreeReqData()
 *    input: char* data, requested data to be freed
 *   output: None
 *   return: R_SKIP, upon NULL pointer;
 *           R_DONE, otherwise
 *
 *  purpose: to free memory allocated in CopyReqData();
 *           called only once from main _writeTraceData{Raw|Ascii}()
 */

static int FreeReqData( char* data )
{
  int ret = R_SKIP;
  if ( data ) {
    free( data );
    ret = R_DONE;
  }
  return ret;
}



#if defined(_LINUX) || defined(_MACOSX)
#undef roundup
#else
static
#endif

int roundup(double dIn)
{
  if(dIn < 0)
    return((int)(dIn - 0.5));
  else
    return((int)(dIn + 0.5));
}  /* end roundup() */

int SendReqDataAscii_AndThisTimeWeMeanIt(char * data, int iDataByteLen, 
                                         double tReqStart, double tReqEnd,
                                         double tTankStart, double tTankEnd,
                                         int iRecordSize, TANK * t, SOCKET soc,
                                         char * reqid, char * fill,
                                         double acttS, double acttE)
{
  /* We know we have data, so now we need to process the data.
	 * First find the start time.
  */

  double dActStartTime = 0.0;
  double dSampRate;
  int    iNumSamplesApart;
  int    bFirstBlock;
  double tLastSample = 0.0;
  int    iDataSize = 0;
  int    iLastSample;
  int    iFirstSample;
  int    samp_pos;
  int    hsz = sizeof(TRACE2_HEADER);
  char*  blk_buf = (char*) NULL;
  int    blk_pos = 0L;
  int    i;
  int    ival = 0;
  int    retVal;
  TRACE2_HEADER * pTH = NULL;
  char * pCurrentRecord;
  int    nsamp;



  /* Loop through all the trace_buf in the buffer, sending the samples 
   * requested plus any fill needed in between */
  for(pCurrentRecord = data; 
      pCurrentRecord < data+iDataByteLen; 
      pCurrentRecord += iRecordSize) 
  {
    if(pCurrentRecord  == data)
      bFirstBlock = TRUE;
    else
      bFirstBlock = FALSE;

    pTH = (TRACE2_HEADER *) pCurrentRecord;


    if ( _fDBG & _fPRT ) trace_log( pTH );

    if (bFirstBlock)  
    {       /* First time through the loop: set values for starttime and 
             * samplerate for ascii reply header */
      dActStartTime = DBL_FLOAT_NA;  /* -1.0 */
      dSampRate = pTH->samprate;

      /*  THIS CODE ASSUMES THAT THERE WERE NO RECORDS(trace_bufs) BEFORE
          THIS ONE WHERE:
          pTH->endtime > (tReqStart - MINIMUM_GAP_SIZE/samprate)
          GIVEN THIS ASSUMPTION, WE CAN ENFORCE STRICT TIMESTAMPS FOR 
          DETERMININING IF A FILL SAMPLE SHOULD BE INSERTED BEFORE THE 
          FIRST SAMPLE IN THE FIRST PACKET
          DK 08/10/01
       *********************************************************************/

      

      if ( pTH->samprate < DBL_FLOAT_EPSILON )
      {
        logit( "","SendReqDataAscii_AndThisTimeWeMeanIt(): samprate too small for tank %s\n",
               t->tankName );
        goto abort;
      }

      /* if the request starts left of the tank */
      if ( tReqStart < tTankStart )   
        dActStartTime = pTH->starttime;  /* dActStartTime = tank start time */
      else
      {
        /* The requested start time is within the tank */
        /* set ActStartTime = [start time of this packet] - (X samples * dSampRate) */
        dActStartTime = pTH->starttime;
        
        iNumSamplesApart = roundup((tReqStart - pTH->starttime) * dSampRate);

        dActStartTime += (iNumSamplesApart/dSampRate);
      }

      tLastSample = dActStartTime;

      /* Determine datatype size */
      if ( strcmp( pTH->datatype, "i2" ) == 0 ||
           strcmp( pTH->datatype, "s2" ) == 0 )
      {
        iDataSize = D_SIZE_I2;
      }
      else if ( strcmp( pTH->datatype, "i4" ) == 0 ||
                strcmp( pTH->datatype, "s4" ) == 0 )
      {
        iDataSize = D_SIZE_I4;
      }
      else
      {
        logit( "","SendReqDataAscii_AndThisTimeWeMeanIt(): unexpected data type for tank %s\n",
               t->tankName );
        goto abort;
      }

      /* Send the header */
      if ( SendHeader( soc, t, reqid, "F", dActStartTime, dSampRate, 0,
                       NO_NEWLINE ) != R_DONE )
      {
        logit("et","SendReqDataAscii_AndThisTimeWeMeanIt(): SendHeader #4 failed for tank %s\n",
              t->tankName );
        goto abort;
      }
    }  /*end if (bFirstBlock)*/

    /* make sure the trace_buf is not unreasonably sized */
    if((hsz + pTH->nsamp * iDataSize) > iRecordSize)
    {
      logit("t","ERROR: SendReqDataAscii_ATWMI(): The following tracebuf exceeded the "
                "maximum record size[%d] for this tank! Returning!\n",
            iRecordSize);
      trace_log( pTH );
      goto abort;
    }


    /* Check for fill, unless this is the first record AND the request is
       left of the tank.  So if the first requested sample is in a GAP, we
       will issue fill as the first sample. DK 08/09/2001 */
    if (bFirstBlock) 
    {
      if (tLastSample + (1.0/pTH->samprate ) <= 
          pTH->starttime ) 
      {
        if ( fill )
        {
         nsamp = roundup( pTH->samprate *
                           ( pTH->starttime - tLastSample ));
          if ( BulkSendFill( soc, fill,nsamp ) != R_DONE )
          {
            logit("et","SendReqDataAscii_AndThisTimeWeMeanIt(): BulkSendFill #2 failed for tank %s\n",
                  t->tankName );
            goto abort;
          }
        }
      }
    }
    else
    {
      if (tLastSample + ( MINIMUM_GAP_SIZE / pTH->samprate ) <= 
          pTH->starttime ) 
      {
        if ( fill )
        {
          /* We need to set nsamp to the number of samples between
             the last sampletime and the next sampletime, but remember
             to take into account that there should always be a
             1/samprate gap between samples.  This fixes a bug
             that caused 1 extra fill value to be inserted.
             DavidK 08/24/01 */
          /* Removed (-1) from end of statement.  The 08/24/01 fix was
             done because we were ending up with one(1) too many samples
             of fill when we missed a tracebuf and had to fill it in.
             Unfotunately that fix removed 2 samples of fill, instead of
             just one(1).  It removed MINIMUM_GAP_SIZE(~1.3) and then removed
             one(1) additional at the end of the statement.  The (-1) is now
             removed so that we theoretically end up with the correct number
             of samples.
             This fix was made in response to a complaint by Carol Bryant
             about problems with 99.7 Hz data.
             DavidK 10/11/02 */
          nsamp = roundup(pTH->samprate *
                          (pTH->starttime -
                           (tLastSample + ( MINIMUM_GAP_SIZE / pTH->samprate ))
                          )
                         );
          if ( BulkSendFill( soc, fill,nsamp ) != R_DONE )
          {
            logit("et","SendReqDataAscii_AndThisTimeWeMeanIt(): BulkSendFill #4 failed for tank %s\n",
                  t->tankName );
            goto abort;
          }
        }
      }
    }

    iFirstSample = 0;

    if ( bFirstBlock && tReqStart > pTH->starttime)
    {
      /* we may need to skip forward when pCurrentRecord is the first record */
      /* Round iFirstSample up to next sample as we did for dActStartTime above */
      iFirstSample = roundup(pTH->samprate * (tReqStart - pTH->starttime ));
      if ( _fDBG & _fPRT ) logit( "","SendReqDataAscii_AndThisTimeWeMeanIt(): adj leading iFirstSample to [%d]\n",iFirstSample );
    }

    iLastSample = (long int) pTH->nsamp;
    /* if the length of this record excedes our databuffer, or 
       the current record ends after the requested endtime, then
       chomp some records off at the end.  */
    if ((pCurrentRecord + iRecordSize) > (data + iDataByteLen)) 
    {
      /* may need cut trailing samples when trace_h is the last record */
     iLastSample -= (int)(((pCurrentRecord + iRecordSize) - (data + iDataByteLen)
                + iDataSize - 1)/iDataSize);
      if ( _fDBG & _fPRT ) 
        logit( "","SendReqDataAscii_AndThisTimeWeMeanIt(): WARNING!: record goes past end of buffer\n");
    }
    if (pTH->endtime > tReqEnd) 
    {
      /* may need cut trailing samples when trace_h is the last record */
      iLastSample -= roundup( pTH->samprate * ( pTH->endtime - tReqEnd ));
      if ( _fDBG & _fPRT ) 
        logit( "","SendReqDataAscii_AndThisTimeWeMeanIt(): adj trailing nsamp to [%d]\n",iLastSample );
    }
    /* update to what we are about to send */
    tLastSample = pTH->starttime + (iLastSample - 1)/pTH->samprate;

    /* Added by davidk 3/12/98, to prevent a fatal error from occuring, when
       SendReqDataAscii_AndThisTimeWeMeanIt() gets a wrong starting record, such that the actual
       requested starttime occurs after tEnd for the first record in <data>.
    */
    if (iFirstSample > iLastSample)
    {
      int temp_pCurrentRecord;
      TRACE2_HEADER * tempHP;
      logit("t","SendReqDataAscii_AndThisTimeWeMeanIt handed premature starting record.  Requested start time is past the end of current record\n");
      logit( "","SendReqDataAscii_AndThisTimeWeMeanIt() tank %s\n", t->tankName );
      logit( "","               tReqStart[%f]    tReqEnd[%f]\n", tReqStart, tReqEnd );
      logit( "","               tTankStart[%f]    tTankEnd[%f]\n", tTankStart, tTankEnd );
      logit( "","            acttS[%f] acttE[%f]\n", acttS, acttE );
      logit( "","              len[%d]\n", iDataByteLen );
      logit( "","             fill[%s]\n", fill );
      logit(""," thp->nsamp %d,thp->samprate %f, thp->et %f, thp->st %f\n",
            pTH->nsamp,pTH->samprate,pTH->endtime,pTH->starttime);
      for ( temp_pCurrentRecord = 0; temp_pCurrentRecord < iDataByteLen; temp_pCurrentRecord += iRecordSize )
      {
        tempHP=(TRACE2_HEADER *)(data+temp_pCurrentRecord);
        logit("","Record # %d :\n",temp_pCurrentRecord/iRecordSize + 1);
        logit("","StartTime  %f      EndTime %f\n",tempHP->starttime,tempHP->endtime);
        logit("","Samples    %d      SampleRate %f\n",tempHP->nsamp,tempHP->samprate);
      }
      continue;
    }


    blk_pos = 0L;
    blk_buf = (char*)malloc( (iLastSample - iFirstSample ) * ( MAX_TRACE_INT_LEN ) + 1 );
    if ( !blk_buf ) 
    {
      logit("et","Wave_server:Memory allocation failed in SendReqDataAscii_AndThisTimeWeMeanIt\n");
      logit("et","Begin SendReqDataAscii_AndThisTimeWeMeanIt dump from wave_server\n");
      logit("et","Nsamp = %d, iFirstSample=%d,MAX_TRACE_INT_LEN=%d\n",
           iLastSample,iFirstSample,MAX_TRACE_INT_LEN);
      logit( "","SendReqDataAscii_AndThisTimeWeMeanIt() for tank %s\n", t->tankName );
      logit( "","               tReqStart[%f]    tReqEnd[%f]\n", tReqStart, tReqEnd );
      logit( "","               tTankStart[%f]    tTankEnd[%f]\n", tTankStart, tTankEnd );
      logit( "","            acttS[%f] acttE[%f]\n", acttS, acttE );
      logit( "","              len[%d]\n", iDataByteLen );
      logit( "","             fill[%s]\n", fill );
      logit(""," thp->nsamp %d,thp->samprate %f, thp->et %f, thp->st %f\n",
            pTH->nsamp,pTH->samprate,pTH->endtime,pTH->starttime);
      goto mem_abort;
    }
    /* Finally, we convert the requested samples to ascii */
    for (i=iFirstSample; i<iLastSample; i++)
    {
      samp_pos = hsz + i * iDataSize;
      if ( iDataSize == D_SIZE_I2 )
        ival = (int) *((short int*)&pCurrentRecord[samp_pos]);
      else /* iDataSize == D_DIZE_I4 */
        ival = (int) *((int32_t*)&pCurrentRecord[samp_pos]);
      sprintf( &blk_buf[ blk_pos ], "%d ", ival );
      blk_pos = (int)strlen( blk_buf );
    }

    if ( _fDBG & _fPRT ) 
      logit("","Record being transmitted, Start %f End %f, length %d\n",pTH->starttime,pTH->endtime,blk_pos);

    if ( (retVal=send_ew( soc, blk_buf, blk_pos, 0, SocketTimeoutLength )) != blk_pos )
    {
      logit("et","SendReqDataAscii_AndThisTimeWeMeanIt(): send_ew was not able to send %d bytes.  send_ew() returned %d. Treated as failure.\n",blk_pos,retVal);
      goto abort;          
    }
    free( blk_buf );
    blk_buf = (char*) NULL;

    /* Need to fill in gaps at the end of last record,
     * but not end of tank */

  } /* end of loop though data buffer */
  /* Final fill: pulled out of above loop, since it only is used when the 
   * loop is completed: PNL, 6/25/00 */
  if ( (MINIMUM_GAP_SIZE / pTH->samprate ) < tReqEnd - pTH->endtime && tReqEnd < tTankEnd )
  {
    nsamp = (long) ( pTH->samprate * ( tReqEnd - tLastSample ) ); 
    /* sampt replaces pTH->starttime above, to get rid of
     * excess fill values: PNL 6/25/00 */
    if ( fill )
    {
      if ( BulkSendFill( soc, fill, nsamp ) != R_DONE )
      {
        logit("et","SendReqDataAscii_AndThisTimeWeMeanIt(): BulkSendFill #3 failed for tank %s\n",
              t->tankName );
        goto abort;          
      }
    }
  }
  if ( _fDBG & _fPRT ) 
    logit("","SRDA: dActStartTime, tLastSample [%.3f - %.3f]\n",
          dActStartTime, tLastSample);
  return(0);
mem_abort:
  return(SRDA_MEM_ABORT);
abort:
  if ( blk_buf )
    free( blk_buf );
  return(SRDA_ABORT);
}  /* end SendReqDataAscii_AndThisTimeWeMeanIt() */


/* function: SendReqDataAscii()
 *    input: SOCKET soc, socket descriptor supplied by caller
 *           TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           double tS, starting time of reqested period;
 *           double tE, ending time of reqested period;
 *           double tL, starting time of tank;
 *           double tR, ending time of tank;
 *           double acttS, starting time of period where data is available;
 *           double acttE, ending time of period where data is available;
 *           char* fill, fill value for sampling gaps;
 *           char* data, records of t->recSize bytes each
 *     note: upon entering SendReqDataAscii(), acttS == trace_h.starttime,
 *           where trace_h is the header record located at &data[0];
 *           upon leaving SendReqDataAscii(), acttE == trace_h.endtime,
 *           where trace_h is the header record located at &data[datasize-rsz]
 *   output: None
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to send data retrieved in CopyReqData() to socket;
 *           called only once from main _writeTraceData{Raw|Ascii}()
 */

static int SendReqDataAscii( SOCKET soc, TANK* t, double tS, double tE,
                             double tL, double tR,
                             double acttS, double acttE, char* fill,
                             char* data, long int datasize, char* reqid )
{
  int ret = R_FAIL;
  long int rsz = 0;
  int rc;

  if ( _fDBG & _fSUB ) 
  {
    logit( "","SendReqDataAscii()\n" );
    logit( "","               tS[%f]    tE[%f]\n", tS, tE );
    logit( "","               tL[%f]    tR[%f]\n", tL, tR );
    logit( "","            acttS[%f] acttE[%f]\n", acttS, acttE );
    logit( "","              len[%ld]\n", datasize );
    logit( "","             fill[%s]\n", fill );
  }

  if ( _fDBG & _fPRT ) 
    logit("","SRDA: tS,tE [%.3f,%.3f], acttS, acctE[%.3f,%.3f]\n",
          tS,tE,acttS,acttE);

  if ( t == NULL || ( rsz = t->recSize ) == 0 )
  {
    logit("et","SendReqDataAscii(): Tank  or Tank->recSize is NULL.  t=%d, t->recSize=%d\n",t,t->recSize);
    goto abort;
  }


  if ( !data && !datasize )
  {
    if ( tE <= tL - GRACE_PERIOD )	/* Davidk 3/27/98 added GRACE_PERIOD */
    {
      /* this is when request falls entirely left of tank range [tL,tR],
       * and acttS is the time stamp that marks beginning of tank at tL
       */
      if ( SendHeader( soc, t, reqid, "FL", acttS, DBL_FLOAT_NA,
                       0, YES_NEWLINE ) != R_DONE )
      {
        logit("et","SendReqDataAscii(): SendHeader #1 failed for tank %s\n",
	      t->tankName );
        goto abort;          /* Line added by WMK  971108 */
      }
      goto done;
    }  /* end if tE < tL */
    else if ( tS >= tR + GRACE_PERIOD ) /* Davidk 3/27/98 added GRACE_PERIOD */
    {
      /* this is when request falls entirely right of tank range [tL,tR],
       * and acttE is the time stamp that marks end of tank at tR
       */
      if ( SendHeader( soc, t, reqid, "FR", DBL_FLOAT_NA, acttE,
                       0, YES_NEWLINE ) != R_DONE )
      {
        logit("et","SendReqDataAscii(): SendHeader #2 failed for tank %s\n",
	      t->tankName );
        goto abort;          /* Line added by WMK  971108 */
      }
      goto done;
    }  /* end else if tS > tR */
    else
    {
      /* this is when the request falls entirely in a gap,
       * note the difference between SendReqData{Ascii|Raw()}: 
       * here in SendReqDataAscii() tS is the 1st double, and
       * t->samprate is the 2nd double passed to SendFlgData();
       * while in SendReqDataRaw() neither acttS nor acttE is passed
       */
      double dbl_2 = t->samprate;

      if ( _fDBG & _fALG )
        logit( "","SendReqDataAscii(): entirely in a gap\n" );
      if ( SendHeader( soc, t, reqid, "FG", tS, dbl_2, 0, 
                       NO_NEWLINE ) != R_DONE )
      {
        logit("et","SendReqDataAscii(): SendHeader #3 failed for tank %s\n",
	      t->tankName );
        goto abort;          
      }
      if ( fill )
      {
        long int fill_cnt = (long int) (( tE - tS )*( t->samprate ));

        if ( BulkSendFill( soc, fill, fill_cnt ) != R_DONE )
        {
          logit("et","SendReqDataAscii(): BulkSendFill #1 failed for tank %s\n",
		t->tankName );
          goto abort;          
        }
      }
      goto terminate;
    }  /* end else (! tS > tR && ! tE < tL) */
  }     /* end if ( !data && !datasize ) */

  if ( !data || !datasize || ( datasize % rsz != 0 ) )
  {
    logit( "","SendReqDataAscii(): data[0x%x] size[%ld]\n", data, datasize );
    goto abort;
  }
    


  rc =SendReqDataAscii_AndThisTimeWeMeanIt(data, datasize, 
                                           tS, tE, tL, tR,
                                           rsz, t, soc, 
                                           reqid, fill, acttS, acttE);

  if(rc)
  {
    if(rc == SRDA_ABORT)
      goto abort;
    else if(rc == SRDA_MEM_ABORT)
      goto mem_abort;
    else
    {
      logit("","SendReqDataAscii(): unexpected return code[%d] from %s\n",
            rc, "SendReqDataAscii_AndThisTimeWeMeanIt()");
      goto abort;
    }
  }

 terminate:
  if ( SendStrData( soc, LN_FD_STR ) != R_DONE )
  {
    logit("et","SendReqDataAscii(): SendStrData failed for tank %s\n", t->tankName );
    goto abort;          
  }
 done:
  ret = R_DONE;
 mem_abort:
  if ( ret != R_DONE )
    logit( "","SendReqDataAscii(): out of memory!\n" );
 abort:
  if ( ret != R_DONE )
    logit( "","SendReqDataAscii(): failed\n" );
  return(ret);
}

/* function: SendStrData()
 *    input: SOCKET soc, socket descriptor supplied by caller,
 *           char* str, string value to be sent
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to send string to socket, called from various places
 */

static int SendStrData( SOCKET soc, char* str )
{
  int ret = R_FAIL;
  long int slen = 0L;

  if ( str == NULL || ( slen = (int)strlen( str ) ) == 0 )
    goto abort;
  if ( send_ew(soc, str, slen, 0, SocketTimeoutLength) != slen )
    goto abort;
    
  if ( _fDBG & _fPRT ) logit( "","%s",str );

  ret = R_DONE;
 abort:
  if ( ret != R_DONE )
    logit( "","SendStrData(): failed\n" );
  return ret;
}

/* function: trace_log()
 *    input: SOCKET soc, socket descriptor supplied by caller,
 *           char* str, string value to be sent
 *   return: R_FAIL or R_DONE
 *
 *  purpose: for debug use only, called only from SendReqData()
 */

static int trace_log( TRACE2_HEADER* trace_hp )
{
  int ret = R_FAIL;

  if ( !trace_hp ) goto abort;

  logit( "","\n--------------------------------\n" );
  logit( "","starttime<%f>\n",trace_hp->starttime );
  logit( "","    nsamp<%d>\n",    trace_hp->nsamp );
  logit( "","    pinno<%d>\n",    trace_hp->pinno );
  logit( "","      sta<%s>\n",      trace_hp->sta );
  logit( "","     chan<%s>\n",     trace_hp->chan );
  logit( "","      net<%s>\n",      trace_hp->net );
  logit( "","      loc<%s>\n",      trace_hp->loc );
  logit( ""," datatype<%s>\n", trace_hp->datatype );
  logit( ""," samprate<%f>\n", trace_hp->samprate );
  logit( "","\n--------------------------------\n" );
  ret = R_DONE;
 abort:
  return ret;
}

static const char fakeFillValue[20] = "20";

static int BulkSendFill( SOCKET soc, char* fill, long int fill_cnt )
{
  int ret = R_FAIL;
  char fill_buf[kFILLVALUE_LEN + 2];
  char* blk_buf = (char*)NULL;
  long int blk_pos = 0;
  long int ii = 0;
  int len;
  
#ifdef FAKE_FILL
  fill = fakeFillValue;
#endif

  if (strlen(fill) == 0)
    goto abort;
  blk_buf = (char*)malloc( fill_cnt * ( strlen(fill) + 1 ) + 1 );
  if ( !blk_buf ) goto abort;
  blk_pos = 0L;

  strcpy(fill_buf, fill);
  strcat(fill_buf, " ");
  len = (int)strlen(fill_buf);
  
  for ( ii = 0; ii < fill_cnt; ii++)
  {
    memcpy( &blk_buf[blk_pos], fill_buf, len);
    blk_pos += len;
  }
  blk_buf[blk_pos] = '\0';  /* was blk_pos++, but we don't want to send
                               the null character to the client: PNL 6/23/00 */
  
  if ( send_ew( soc, blk_buf, blk_pos, 0, SocketTimeoutLength ) != blk_pos )
    goto abort;
  ret = R_DONE;
 
 abort:
  if ( blk_buf )
    free( blk_buf );
  if ( ret != R_DONE )
    logit( "","BulkSendFill(): failed\n" );
  return ret;
}

/* function: SendHeader()
 *    input: SOCKET soc, socket descriptor supplied by caller;
 *           TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           char* reqid, ascii string from the client;
 *           char* flag, "FL"/"FR"/"FG";
 *           double tS, starting time, negative DBL_FLOAT_NA when N/A;
 *           double tE, ending time, negative DBL_FLOAT_NA when N/A;
 *           int binSize, size in bytes for binary header; 0 otherwise;
 *           int flagNewLine, 1 to end header with newline; 0 otherwise;
 *   output: None
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to send the header preceeding binary and ascii trace data.
 *           called from _SendReqData{Raw|Ascii}()
 */

static int SendHeader( SOCKET soc, TANK* t, char* reqid, char* flag, double tS,
		       double tE, int binSize, int flagNewLine)
{
  size_t len;
  char buffer[kMAX_CLIENT_MSG_LEN];
    
  if ( t == NULL || flag == NULL ) return R_FAIL;
    
  sprintf( buffer, "%s %d %s %s %s %s %s %s ", reqid, t->pin, t->sta, t->chan,
           t->net, t->loc, flag, t->datatype);
  if ( tS > DBL_FLOAT_NA + DBL_FLOAT_EPSILON )
    sprintf( &buffer[strlen(buffer)], "%f ", tS);
  if ( tE > DBL_FLOAT_NA + DBL_FLOAT_EPSILON )
    sprintf( &buffer[strlen(buffer)], "%f ", tE);
  if (binSize > 0)
    sprintf( &buffer[strlen(buffer)], "%d", binSize); /* no space after size */
  if (flagNewLine)
    strcat(buffer, "\n");
  
  len = strlen(buffer);
  if ( send_ew(soc, buffer, (int)len, 0, SocketTimeoutLength) != (int)len )
  {
    logit("et", "SendHeader: failed for tank %s\n", t->tankName );
    return R_FAIL;
  }
  return R_DONE;
}


/* function: SendReqDataRaw()
 *    input: SOCKET soc, socket descriptor supplied by caller;
 *           TANK* t, with the assumption t->tfp opened in "br+" mode;
 *           double tS, starting time of reqested period;
 *           double tE, ending time of reqested period;
 *           double tL, starting time of tank;
 *           double tR, ending time of tank;
 *           double acttS, starting time of period where data is available;
 *           double acttE, ending time of period where data is available;
 *           char* data, records of t->recSize bytes each;
 *           char* reqid, the client's request ID
 *     note: upon entering SendReqDataRaw(), acttS == trace_hp->starttime,
 *           where trace_hp is pointer to header record located at &data[0];
 *           upon leaving SendReqDataRaw(), acttE == trace_hp->endtime,
 *           where trace_hp is pointer to header record located at &data[datasize-rsz]
 *   output: None
 *   return: R_FAIL or R_DONE
 *
 *  purpose: to send data retrieved in CopyReqData() to socket;
 *           called only once from main _writeTraceDataRaw()
 */

static int SendReqDataRaw( SOCKET soc, TANK* t, double tS, double tE,
                           double tL, double tR,
                           double acttS, double acttE,
                           char* data, long int datasize, char* reqid )
{
  int ret = R_FAIL;
  long int rsz = 0;
  long int hsz = sizeof(TRACE2_HEADER);
  long int rec_pos = 0L;
  TRACE2_HEADER* trace_hp = NULL;
  long int nsamp = 0; /* total count of sample data */
  int size;
  int dsz = 0;

  if ( _fDBG & _fSUB ) {
    logit( "","SendReqDataRaw()\n" );
    logit( "","               tS[%f]    tE[%f]\n", tS, tE );
    logit( "","               tL[%f]    tR[%f]\n", tL, tR );
    logit( "","            acttS[%f] acttE[%f]\n", acttS, acttE );
    logit( "","              len[%ld]\n", datasize );
  }

  if ( t == NULL || ( rsz = t->recSize ) == 0 ) goto abort;
  if ( !data && !datasize )
  {
    if ( tE < tL )
    {
      /* this is when request falls entirely left of tank range [tL,tR],
       * and acttS is the time stamp that marks beginning of tank at tL
       */
      if ( SendHeader( soc, t, reqid, "FL", acttS, DBL_FLOAT_NA, 0,
                       YES_NEWLINE) != R_DONE )
        goto abort;
    }
    else if ( tS > tR )
    {
      /* this is when request entirely right of tank range [tL,tR],
       * and acttE is the time stamp that marks end of tank at tR
       */
      if ( SendHeader( soc, t, reqid, "FR", DBL_FLOAT_NA, acttE, 0,
                       YES_NEWLINE ) != R_DONE )
        goto abort;
    }
    else
    {
      /* this is when the request falls entirely in a gap */
      if ( SendHeader( soc, t, reqid, "FG", DBL_FLOAT_NA, DBL_FLOAT_NA,
                       0, YES_NEWLINE) !=
           R_DONE )
        goto abort;
    }
    goto done;
  }

  if ( !data || !datasize || ( datasize % rsz != 0 ) )
  {
    logit( "","SendReqDataRaw(): data[0x%x] size[%ld] for tank %s\n", data,
           datasize, t->tankName );
    goto abort;
  }
    
  for ( rec_pos = 0; rec_pos < datasize; rec_pos += rsz )
  {
    trace_hp = (TRACE2_HEADER* ) &data[rec_pos];
    if ( _fDBG & _fPRT ) trace_log( trace_hp );
    nsamp += (long int) trace_hp->nsamp;
  }

  /* since datasize != 0 && datasize % rsz == 0, we know that 
   * trace_h now is the last record copied from data
   */
  if ( strcmp( trace_hp->datatype, "i2" ) == 0 ||
       strcmp( trace_hp->datatype, "s2" ) == 0 )
    dsz = D_SIZE_I2;
  else if ( strcmp( trace_hp->datatype, "i4" ) == 0 ||
	    strcmp( trace_hp->datatype, "s4" ) == 0 )
    dsz = D_SIZE_I4;
  else
  {
    logit( "","SendReqData(): unexpected data type for tank %s[%s] - datasize %d\n", t->tankName, trace_hp->datatype, datasize );
    goto abort;
  }

  if ( SendHeader( soc, t, reqid, "F", acttS, acttE,
		   datasize/rsz*hsz+dsz*nsamp, YES_NEWLINE) != R_DONE )
    goto abort;

  for ( rec_pos = 0; rec_pos < datasize; rec_pos += rsz )
  {
    trace_hp = (TRACE2_HEADER *) &data[rec_pos];
    size = hsz + dsz * trace_hp->nsamp;
	  
    if ( send_ew( soc, &data[rec_pos], size, 0, SocketTimeoutLength ) !=
         size )
      goto abort;
  }

 done:
  ret = R_DONE;
 abort:
  if ( ret != R_DONE )
    logit( "","SendReqDataRaw(): failed\n" );
  return ret;
}

int serve_trace_c_init()
{
  if (Debug > 1)
      logit("t","serve_trace.c:Version 1084816782\n");
  return(0);
}
/******************************************************
  MergeAsyncPackets - used to resync packets written to 
  db.
 
*******************************************************/
int MergeAsyncPackets(
     TANK* t,
	char** data,
 	long* datasize,
 	char* AsyncData,
	long AsyncDataSize,
	long AsyncPacketCount
 ) {

 	char* pData;
	char* pDataEnd;
 	char* pLast;
 	char* pAsync;
	char* pAsyncEnd;
	char* pMerge;
 	char* p;
 	long MergeSize;
 	TRACE2_HEADER* pTb;
 	int rc = FALSE;

   /* merge async data if any. It would be better to include this
   someplace lower for performance reasons but since this should be
   a rare event so we'll optimize the merge here so all the other code
   that examines the tank data on the way out include the async packets as well.*/
   if(AsyncPacketCount) {
 
 	  if(*datasize && *data) {
 
 
 		  logit( "","MergeAsycPackets(): merging async packet data.\n" );
 
		  /* reallocate data for combined buffers */
 		  MergeSize = *datasize + (AsyncPacketCount * t->recSize);
		  if((p = pMerge = (char*) malloc(MergeSize)) != NULL) {
 
 			  /* init buf*/
 			  memset(pMerge, 0, MergeSize);

			  /* walk through tank data*/
 			  pLast = pData = *data;
 			  pDataEnd = pData + *datasize;
 			  pAsync = AsyncData;
			  pAsyncEnd = pAsync + AsyncDataSize;
 			  while(pData < pDataEnd && pAsync < pAsyncEnd) {
 				  /**/
 				  pTb = (TRACE2_HEADER*) (pAsync + sizeof(int32_t));
 				  if(pTb->starttime < ((TRACE2_HEADER*)pData)->starttime) {
 					  /*copy any data up to this tank record */
 					  if(pData - pLast) {
 						  memcpy(p, pLast, pData - pLast);
						  p += pData - pLast;
 						  pLast = pData;
 					  }
 					  /* copy async data */
					  memcpy(p, pTb, *((int32_t*)pAsync));
 					  p += t->recSize;
 					  pAsync += *((int32_t*)pAsync) + sizeof(int32_t);
 				  }
				  pData += t->recSize;
 			  }
 			  /* write any leftover. Should only be tankdata because the query shouldn't
 			  return any records beyond tE drom db.*/
 			  if(pDataEnd - pLast) {
 
 				  memcpy(p, pLast, pDataEnd - pLast); 
 			  }
 			  /* free old data*/
 			  FreeReqData(*data);
 			  /* updata data pointer and size and fall through to old code. */
 			  *data = pMerge;
 			  *datasize = MergeSize;
 			  rc = TRUE;
 		  } else {
 			  logit( "e","MergeAsyncPackets(): Error reallocating packet buffer for async packet data.\n" );
 		  }
 
 	  } else {
 	     
 		 /* just async data. Normalize buffer (make sure packets are recsize.
 		 this too should be a rare occasion also but seemed possible. */
 	     MergeSize = AsyncPacketCount * t->recSize;
 		 if((pMerge = (char*) malloc(MergeSize)) != NULL) {
 
 			 memset(pMerge, 0, MergeSize);
 			 pAsync = AsyncData;
 			 pAsyncEnd = pAsync + AsyncDataSize;
			 p = pMerge;
 			 while(pAsync < pAsyncEnd) {
				memcpy(p, pAsync + sizeof(int32_t), *((int32_t*)pAsync));
 				pAsync += *((int32_t*)pAsync) + sizeof(int32_t);
 				p += t->recSize;
 			 }
 			 *data = pMerge;
 			 *datasize = MergeSize;
 			 rc = TRUE;
 		 } else {
 			  logit( "e","MergeAsyncPackets(): Error allocating packet normalization buffer for async packet data.\n" );
 		 }
 
 
 	  }
   }
   return rc; 
 }
