
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: server_thread.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.21  2010/03/25 18:40:59  paulf
 *     minor fix to remove bad program name from debug print
 *
 *     Revision 1.20  2010/03/18 16:24:38  paulf
 *     added client_ip echoing to ListenForMsg() as per code from DK
 *
 *     Revision 1.19  2007/03/28 18:02:34  paulf
 *     fixed flags for MACOSX and LINUX
 *
 *     Revision 1.18  2006/10/09 17:03:17  davek
 *     Modified logging, so that when client disconnects between transactions, it
 *     no longer gets logged/recorded as a socket error.
 *     Updated verson timestamp.
 *
 *     Revision 1.17  2005/09/12 17:38:17  davidk
 *     Fixed a bad assertion in server_thread.c. (that aborted when _getField() was
 *     called for the 9th (kMAX_CLIENT_FIELDS) field.
 *     Fixed ew_assert so that it reports the proper line number of the failed assertion.
 *     Updated the server_thread.c version string, and modified the code so that it
 *     ALWAYS logs the version string at startup.
 *
 *     Revision 1.16  2005/09/08 00:25:48  davidk
 *     Fixed field number used to parse the Ascii fill value.
 *     Was 8, (which is the endtime), is now 9.
 *     This looks like it was a bug introduced during SCNL,
 *     which only affects gappy channel / ascii client combinations.
 *
 *     Revision 1.15  2005/07/21 21:00:59  friberg
 *     added in _LINUX ifdef directive to not include synch.h
 *
 *     Revision 1.14  2005/04/21 22:52:36  davidk
 *     Updated code to support SCNL menu protocol adjustment (v5.1.24).
 *     Modified code to handle menu requests from SCN and SCNL clients differently.
 *     The server only deals(well) with SCNL clients.  SCN clients are handed an
 *     empty menu and sent on their way.
 *     SCNL clients are handed a proper menu.
 *     The two client types are differentiated by an "SCNL" at the end of the menu
 *     request.
 *     Updated the version timestamp.
 *
 *     Revision 1.13  2004/08/31 01:04:58  lombard
 *     Corrected check for error return from _writeTankSummary for MENUSCNL request.
 *
 *     Revision 1.12  2004/06/09 21:35:58  lombard
 *     Added code to validate a trace_buf packet before it gets into the tanks.
 *     Added comments about GETPIN request not working correctly.
 *
 *     Revision 1.11  2004/06/03 22:23:22  lombard
 *     Corrected parsing of start and end times for GETSNCL and GETSCNLRAW.
 *
 *     Revision 1.10  2004/05/18 22:30:19  lombard
 *     Modified for location code
 *
 *     Revision 1.9  2001/06/29 22:23:59  lucky
 *     Implemented multi-level debug scheme where the user can specify how detailed
 *     (and large) the log files should be. If no Debug is specified, only
 *     errors are reported and logged.
 *
 *     Revision 1.8  2001/06/18 23:23:06  davidk
 *     Moved logging statement(_writeTankSummary: Warning: no data in tank) inside
 *     a DEBUG conditional, so that it would not cause HUGE log files to be
 *     generated under normal(HEAVY) operations where multiple tanks were
 *     not being fed data.  This fix was performed due to problems with a
 *     wave_server in Golden that was generating 4GB logfiles in a single day.
 *
 *     Revision 1.7  2001/01/18 02:04:57  davidk
 *     Added ability to issue status messages from server_thread.c
 *
 *     Changed ERR_XXX constants defined in server_thread.h
 *     to RET_ERR_XXX because they were overlapping with the
 *     ERR_XXX constants in wave_serverV.c/wave_serverV.h
 *     which are used for issuing status messages to statmgr.
 *
 *     Added code to issue status message when _writeTankSummary()
 *     fails with a significant error.
 *
 *     Added AbortOnsSingleTankFailure conditional logic,
 *     to prevent wave_server from halting or halting a response
 *     when one tank fails.
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
 *     Revision 1.4  2000/08/17 21:35:54  lombard
 *     Fixed bugs where ProcessMsg where return value from _writeTankSummary
 *     was not handled properly for an empty tank. The result was that no more
 *     memory entries would be returned after the empty tank.
 *
 *     Revision 1.3  2000/07/08 19:01:07  lombard
 *     Numerous bug fies from Chris Wood
 *
 *     Revision 1.2  2000/07/08 16:56:21  lombard
 *     clean up logging for server thread status
 *
 *     Revision 1.1  2000/02/14 19:58:27  lucky
 *     Initial revision
 *
 *
 */

/*
Server_Thread:

	Written by: K. R. Mackenzie,    February 8, 1997
	Cleaned up by: Pete Lombard,    February 1998

There is another file "serve_trace.c", written by Eureka Young which processes
the GETSCNL, GETSCNLRAW, and GETPIN queries.

NOTE: the GETPIN request has never worked in wave_serverV, and probably 
never will.

This is the thread that is launched by the Wave_ServerV to process requests
from one particular socket client.  The thread's behavior is as follows:

1.  When the thread is bound to the client socket, it is guaranteed by the
	listener that nothing has been read from the socket, so the thread may
	reasonably assume that what it will see coming down the pipe properly
	obeys the protocol sequence for a client request.  (Or else is an
	error-in-progress :)

2.  The thread will continue to live and own the socket connection until one
	of the following happens:

	A: <Timeout> seconds have elapsed since the last client request on
		this socket.  The thread may assume that the client has become
		inoperant, and may close the socket and die.

	B: The socket breaks.  In this case the thread may assume that the
		client has ceased communication and may close the socket and
		die.

	C: (Optional...) The client makes <iMaxRequests> for data during a
		single connection.  The thread may close the socket and die so
		that other clients have a better chance to contend for server
		resources.

3.  The thread expects requests to come from the client in a variable length
	format, with fields delimited by spaces (ascii 32) and the entire
	request record terminated by a newline character.

4.  The protocol we follow is as stated in introductory comments in
	wave_serverV.c)

*/

/* if defined, will cause debug logging via logit
#define DEBUG
*/

/*              ANSI System headers: */
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <ctype.h>
#ifdef _WINNT
#include        <windows.h>
#define mutex_t HANDLE
#else

#if defined(_SOLARIS)
	/* linux and macosx do not need this include */
#include        <synch.h>            /* for solaris mutex's */
#endif 

#endif
#include        <earthworm.h>
/*              Project definitions... */
#include        <transport.h>        /*  ..needed for wave_serverV.h */
#include        <trace_buf.h>
#include        "wave_serverV.h"
#include        "server_thread.h"

/*      NONSTATIC GLOBALS from wave_server... */
extern int      nTanks;              /*  ..number of active message tanks. */
extern TANK *   Tanks;               /*  ..array of tank descriptors. */
extern ServerThreadInfoStruct * ServerThreadInfo;

/*              Some systems don't define BOOL as a type: */
#if !defined(BOOL)
typedef  int    BOOL;
#if !defined(FALSE)
#define  FALSE  0
#endif
#if !defined(TRUE)
#define  TRUE   (!FALSE)
#endif
#endif


/*  #defines */
#define CLIENT_MESSAGE_PARSE_SUCCESS                   0
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_MESSAGE        1
#define CLIENT_MESSAGE_PARSE_ERROR__UNKNOWN_REQ_TYPE   2
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_REQUESTID      3
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_PIN_NUM        4
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_SITE           5
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_CHAN           6
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_NET            7   
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_LOC            8
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_STARTTIME      9
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_ENDTIME       10
#define CLIENT_MESSAGE_PARSE_ERROR__BAD_FILLVALUE     11

/* Globals for this file */
static const char ClientMessageParseErrorArray[][25]=
  {
    "Success",
    "Bad Message",
    "Unknown Request Type",
    "Bad RequestID",
    "Bad Pin#",
    "Bad Site Code",
    "Bad Chan Code",
    "Bad Net Code",
    "Bad Loc Code",
    "Bad Starttime",
    "Bad Endtime",
    "Bad FillValue"
  };


/* Forward declarations for routines in this file... */

void ew_assert( BOOL fExp, int iLineNum);
int ListenForMsg( SOCKET iSocketDesc, char *pRcvBuffer , int MyThrdID);
int ParseMsg( char *pRcvBuffer, CLIENT_MSG *pClientReq );
BOOL ProcessMsg( SOCKET iSocketDesc, CLIENT_MSG *pClientReq );
BOOL ProcessParseError( SOCKET iSocketDesc, CLIENT_MSG *pClientReq, int ParseError);
int _getField( char *pRcvBuffer, int iField, char *pSBuffer );
int _writeTankSummary( SOCKET iSocketDesc, TANK *pTank );

void _xvertToUpper( char *pBuffer );
extern int _writeTraceDataAscii( SOCKET iSoc, TANK *pTank, CLIENT_MSG
				 *pClientReq );
extern int _writeTraceDataRaw( SOCKET iSoc, TANK *pTank, CLIENT_MSG *pClientReq );



    /* space delimited message terminated by \n"      */
    /* ERROR <ERROR TYPE: {REQUEST,PROCESSING}> <request id> <request type> */
    /*  <request id> <pin#> <s><c><n><l> FX \n 

     DK send to client  <request id> <pin#> <s><c><n><l> FB|FC|FN | FR|FL|FG <datatype>\n 
          FB = BAD REQUEST
          FC = CORRUPT TANK
          FN = TANK NOT FOUND
          FU = UNKNOWN TANK ERROR
    ***********************************************************/


BOOL ProcessParseError( SOCKET iSocketDesc, CLIENT_MSG *pClientReq, int ParseError)
{
  char szBuffer[100];
  int len;
  
  if(ParseError == CLIENT_MESSAGE_PARSE_ERROR__BAD_REQUESTID ||
     ParseError == CLIENT_MESSAGE_PARSE_ERROR__BAD_MESSAGE ||
     ParseError == CLIENT_MESSAGE_PARSE_ERROR__UNKNOWN_REQ_TYPE)
  {
    /* We think the client's request was complete garbage! */
    logit("et", "wave_server_thread: Could not decipher client message.\n");
    sprintf(szBuffer,"ERROR REQUEST \n");
  }
  else if(ParseError == CLIENT_MESSAGE_PARSE_ERROR__BAD_STARTTIME ||
          ParseError == CLIENT_MESSAGE_PARSE_ERROR__BAD_ENDTIME ||
          ParseError == CLIENT_MESSAGE_PARSE_ERROR__BAD_FILLVALUE)
  {
    /* We got a bunch of information from the client request, but part
       of it was hosed, probably the times. */
    /* Use a standard reply, but set the FB flag to indicate a bad request */
    logit("et", "wave_server_thread: Could not decipher client message: "
          "reqid(%s), reqtype(%d), site(%s), chan(%s), net(%s), loc(%s), pin#(%d)\n",
          pClientReq->reqId, pClientReq->fMsg, pClientReq->site, 
          pClientReq->channel, pClientReq->network, pClientReq->location,
	  pClientReq->pin);

    sprintf(szBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId, 
            pClientReq->pin, pClientReq->site, pClientReq->channel,
	          pClientReq->network, pClientReq->location, "FB");
  }
  else
  {
    logit("et", "wave_server_thread: Could not decipher client "
      "message(%s).\n",
      pClientReq->reqId);

    sprintf(szBuffer, "ERROR REQUEST %s %d \n", 
            pClientReq->reqId, pClientReq->fMsg);
  }

  len = (int)strlen(szBuffer);
  if ( send_ew(iSocketDesc, szBuffer, len, 0, SocketTimeoutLength) != (int)len )
  {
    return FALSE;
  }

  return(TRUE);
  
}  /* End ProcessParseError() */


/*  This is the thread that is launched by the wave_serverV to process
    requests from one particular socket client.  The argument is a
    'thread index' generated by the ServerMgr thread which creates us.
    We use it to find our socket and status variable */

thr_ret ServerThread( void *pThrdId )
{
  SOCKET          iSocketDesc = 0;
  CLIENT_MSG      msgRec;
  int             myThrdId;
  char            pRcvBuffer[kMAX_CLIENT_MSG_LEN+1];
  int             rc;  /* function return code */

  /* Validate param: */
  ew_assert(pThrdId!=NULL, __LINE__);
  myThrdId = *((int *)pThrdId);
  iSocketDesc = ServerThreadInfo[myThrdId].ActiveSocket;

  ServerThreadInfo[myThrdId].Status = SERVER_THREAD_START;
  ServerThreadInfo[myThrdId].ClientsAccepted++;

  if(Debug > 1)
    logit("et", "Setting thread status to busy for thread %d on socket %ld.\n",
	        myThrdId,(long)iSocketDesc);

  while (1)
  {
    ServerThreadInfo[myThrdId].Status= SERVER_THREAD_WAITING; 
      /* declares us waiting for a message from the client. */


    if ((rc=ListenForMsg(iSocketDesc, pRcvBuffer, myThrdId)) <= 0)
    {
      if(rc < 0)
        ServerThreadInfo[myThrdId].Errors++;
      if (Debug > 1)
         logit("", "wave_serverV: closing thread %d.\n", myThrdId);
      break;
    }

    ServerThreadInfo[myThrdId].Status = SERVER_THREAD_MSG_RCVD;
    if(Debug > 2)
      logit("", "ServerThread: Here's the rec'vd msg: .%s.\n", pRcvBuffer);

    if ((rc=ParseMsg(pRcvBuffer, &msgRec)) != CLIENT_MESSAGE_PARSE_SUCCESS )
    {
      if (!ProcessParseError(iSocketDesc, &msgRec,rc))
      {
        logit("", "ServerThread: Could not supply error reply to client.\n");
      }
      ServerThreadInfo[myThrdId].Errors++;
	    break;
    }

    ServerThreadInfo[myThrdId].Status = SERVER_THREAD_MSG_PARSED;
    if(Debug > 2)
    {
      logit("", "ServerThread: Here is the data we parsed from the incoming client request:\n");
      logit("", "field: fMsg .%d.\n", msgRec.fMsg);
      logit("", "field: reqId .%s.\n", msgRec.reqId);
      logit("", "field: pin .%d.\n", msgRec.pin);
      logit("", "field: site .%s.\n", msgRec.site);
      logit("", "field: channel .%s.\n", msgRec.channel);
      logit("", "field: network .%s.\n", msgRec.network);
      logit("", "field: location .%s.\n", msgRec.location);
      logit("", "field: starttime .%lf.\n", msgRec.starttime);
      logit("", "field: endtime .%lf.\n", msgRec.endtime);
      logit("", "field: fillvalue .%s.\n", msgRec.fillvalue);
    }
    if (!ProcessMsg(iSocketDesc, &msgRec))
    {
      ServerThreadInfo[myThrdId].Errors++;
      logit("", "ServerThread: Could not supply requested answer "
            "to client.  Sent error message instead.\n");
    }
    ServerThreadInfo[myThrdId].Status = SERVER_THREAD_MSG_PROCESSED;
    ServerThreadInfo[myThrdId].RequestsProcessed++;
  }  /* end of loop over requests */

  ServerThreadInfo[myThrdId].Status = SERVER_THREAD_DISCONN;
  /* Shut down: either an error, or too many requests (used to be that way) */
  if (Debug > 1)
    logit("et","Server thread %d terminating\n\n",myThrdId);

  closesocket_ew(iSocketDesc, SOCKET_CLOSE_IMMEDIATELY_EW);
  ServerThreadInfo[myThrdId].ClientsProcessed++;
  ServerThreadInfo[myThrdId].Status= SERVER_THREAD_COMPLETED;

  /* Is this kill neccessary.  Can't we just return(0)? */
  (void) KillSelfThread( );
  return THR_NULL_RET;     /* should never reach here */
}



int ListenForMsg( SOCKET iSocketDesc, char *pRcvBuffer , int myThrdID)
     /* Returns <0 if error; Reads individual characters from a file
      * descriptor <iSocketDesc> into the supplied buffer <pRcvBuffer> until
      * either we run out of buffer space (returning ERR_OVRFLOW) or we find a
      * newline character on the * input stream (returning the number of
      * characters read, including the newline)...  */

      /* This function should not be reading in one char at a time.
         This is a lot of overhead.
         DavidK 9/21/98
      */

{
  int             iCIndex;
  int             iReadResult;

  /*              Validate params: */
  ew_assert(iSocketDesc > 0, __LINE__);
  ew_assert(pRcvBuffer != NULL, __LINE__);

  if (Debug > 1)
     logit("et", "ListenForMsg: listening at the socket [%ld] for thread [%d] client_ip[%s]... \n",
	(long)iSocketDesc, myThrdID, client_ip[myThrdID] );

  /* Listen to the socket for incoming characters until our buffer is full... */
  /* Note: pRcvBuffer now declared as kMAX_CLIENT_MSG_LEN+1 so we can't overrun
     buffer with the appended null byte (null byte is not part of message) */
  for (iCIndex = 0 ; iCIndex < kMAX_CLIENT_MSG_LEN ; iCIndex++)
  {
    iReadResult = recv_ew(iSocketDesc, &pRcvBuffer[iCIndex], 1, 0, 
                          ClientTimeout);
    if (iReadResult==1 && pRcvBuffer[iCIndex] == '\n')
    {
      /* If we got a newline, we're done gathering */
      pRcvBuffer[iCIndex + 1] = '\0';
      return 1;
    }
    if (iReadResult <= 0)
    {
      if(iReadResult == 0)
      {
        logit("et","Wave_serverV:  Client at ip %s closed socket.\n", client_ip[myThrdID]);
        return -1;
      }
      else
      {
        if(iCIndex == 0)
        {
          logit("t","Wave_serverV:  Client at ip %s closed socket (improperly).\n", client_ip[myThrdID]);
          return 0;
        }
        logit("et","Wave_serverV:ListenForMsg: Bad socket read. From client at ip %s"
                   "iReadResult = %d\n", client_ip[myThrdID], iReadResult);
        /* Broken socket.  Client must have gone away... */
        return -2;
      }
    }  /* end if (iReadResult <= 0) */
  }  /* end for (iCIndex < kMAX_CLIENT_MSG_LEN) */ 
  logit( "","ListenForMsg: Error - message buffer overflow, setting %d is too small in server code\n", kMAX_CLIENT_MSG_LEN);
  return -3;
}



int ParseMsg( char *pRcvBuffer, CLIENT_MSG *pClientReq )
/* Walks through the message buffer <pRcvBuffer> and parses it into
* individual fields depending on the type of client message it is recognized
* to be.  Writes the fields into <pClientReq> for easy access by other parts
* of the Server Thread, and does basic sanity checks on the field values.  */
{
  BOOL            fRval = TRUE;
  char            szSBuffer[kMAX_CLIENT_FIELD_LEN];
  int             iParseError=CLIENT_MESSAGE_PARSE_SUCCESS;
  
  /*              Validate params: */
  ew_assert(pRcvBuffer != NULL, __LINE__);
  ew_assert(pClientReq != NULL, __LINE__);
  /*
  *              Process the Message Type field...
  */
  if ( _getField(pRcvBuffer, 1, szSBuffer) >0 )
  {
    pClientReq->fMsg = kMSG_UNKNOWN;
    _xvertToUpper(szSBuffer);
    if (strcmp(kMENU_TOKEN, szSBuffer) == 0)
      pClientReq->fMsg = kSCNMENU;
    else if (strcmp(kMENUPIN_TOKEN, szSBuffer) == 0)
      pClientReq->fMsg = kMENUPIN;
    else if (strcmp(kMENUSCNL_TOKEN, szSBuffer) == 0)
      pClientReq->fMsg = kMENUSCNL;
    else if (strcmp(kGETPIN_TOKEN, szSBuffer) == 0)
      pClientReq->fMsg = kGETPIN;
    else if (strcmp(kGETSCNL_TOKEN, szSBuffer) == 0)
      pClientReq->fMsg = kGETSCNL;
    else if (strcmp(kGETSCNLRAW_TOKEN, szSBuffer) == 0)
      pClientReq->fMsg = kGETSCNLRAW;
    else
    {
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__UNKNOWN_REQ_TYPE;
      logit("t","Bad request type received from client: (%s)\n",
        szSBuffer);
      fRval = FALSE;
    }
  }
  else
  {
    iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_MESSAGE;
    fRval = FALSE; 
    /* don't return here, we will handle all errors in bottom section. */
    /* return fRval; DK 010501*/
  }
  
  if(fRval == FALSE)
    goto PARSEMSG_ERROR;
  
  /* Now we do the request id; alex 4/12/97 */
  if (_getField(pRcvBuffer, 2, szSBuffer))
  {
    strncpy(pClientReq->reqId, szSBuffer, kREQID_LEN-1);
    pClientReq->reqId[kREQID_LEN-1] = '\0';
  }
  else
  {
    fRval = FALSE;
    iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_REQUESTID;
  }
  
  if(fRval == FALSE)
    goto PARSEMSG_ERROR;

   /* 
   *  For menu requests, look for SCNL token at end  
  */
  if(pClientReq->fMsg == kSCNMENU)
  {
    if (_getField(pRcvBuffer, 3, szSBuffer))
    {
      if(strcmp(szSBuffer, kMENU_SCNL_TOKEN) == 0)
         pClientReq->fMsg = kMENU;
    }
  }

    /*
    *              Process the Pin field...
  */
  switch (pClientReq->fMsg)
  {
  case kMENUPIN:
  case kGETPIN:
    if (_getField(pRcvBuffer, 3, szSBuffer))
    {
      pClientReq->pin = atoi(szSBuffer);
      if (pClientReq->pin <= 0)
        fRval = FALSE;
    }
    else
      fRval = FALSE;
    
    if(fRval == FALSE)
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_PIN_NUM;
    break;
    
  default:
    pClientReq->pin = 0;
    break;
  }
  
  if(fRval == FALSE)
    goto PARSEMSG_ERROR;
  
    /*
    *              Process the SCNL field...
  */
  switch (pClientReq->fMsg)
  {
  case kMENUSCNL:
  case kGETSCNL:
  case kGETSCNLRAW:
    if (_getField(pRcvBuffer, 3, szSBuffer))
    {
      strncpy(pClientReq->site, szSBuffer, TRACE2_STA_LEN-1);
      pClientReq->site[TRACE2_STA_LEN-1] = '\0';
    }
    else
    {
      fRval = FALSE;
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_SITE;
    }
    
    if (_getField(pRcvBuffer, 4, szSBuffer))
    {
      strncpy(pClientReq->channel, szSBuffer, TRACE2_CHAN_LEN-1);
      pClientReq->channel[TRACE2_CHAN_LEN-1] = '\0';
    }
    else
    {
      fRval = FALSE;
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_CHAN;
    }
    
    if (_getField(pRcvBuffer, 5, szSBuffer))
    {
      strncpy(pClientReq->network, szSBuffer, TRACE2_NET_LEN-1);
      pClientReq->network[TRACE2_NET_LEN-1] = '\0';
    }
    else
    {
      fRval = FALSE;
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_NET;
    }

    if (_getField(pRcvBuffer, 6, szSBuffer))
    {
      strncpy(pClientReq->location, szSBuffer, TRACE2_LOC_LEN-1);
      pClientReq->location[TRACE2_LOC_LEN-1] = '\0';
    }
    else
    {
      fRval = FALSE;
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_LOC;
    }
    break;
  default:
    pClientReq->site[0] = '\0';
    pClientReq->channel[0] = '\0';
    pClientReq->network[0] = '\0';
    pClientReq->location[0] = '\0';
    break;
  }
  
  if(fRval == FALSE)
    goto PARSEMSG_ERROR;
  
    /*             Process the Starttime field...
  */
  switch (pClientReq->fMsg)
  {
  case kGETPIN:
      /* the GETPIN request parsing is wrong here. */
  case kGETSCNL:
  case kGETSCNLRAW:
    if (_getField(pRcvBuffer, 7, szSBuffer))
    {
      pClientReq->starttime = atof(szSBuffer);
    }
    else
    {
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_STARTTIME;
      fRval = FALSE;
    }
    break;
  default:
    pClientReq->starttime = 0;
    break;
  }
  
  if(fRval == FALSE)
    goto PARSEMSG_ERROR;
  
    /*             Process the Endtime field...
  */
  switch (pClientReq->fMsg)
  {
  case kGETPIN:
      /* the GETPIN request parsing is wrong here. */
  case kGETSCNL:
  case kGETSCNLRAW:
    if (_getField(pRcvBuffer, 8, szSBuffer))
    {
      pClientReq->endtime = atof(szSBuffer);
    }
    else
    {
      fRval = FALSE;
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_ENDTIME;
    }
    break;
  default:
    pClientReq->endtime = 0;
    break;
  }
  
  if(fRval == FALSE)
    goto PARSEMSG_ERROR;
  
    /*             Process the Fillvalue field...
  */
  switch (pClientReq->fMsg)
  {
  case kGETPIN:
      /* the GETPIN request parsing is wrong here. */
  case kGETSCNL:
    if (_getField(pRcvBuffer, 9, szSBuffer))
    {
      strncpy(pClientReq->fillvalue, szSBuffer, kFILLVALUE_LEN-1);
      pClientReq->fillvalue[kFILLVALUE_LEN-1] = '\0';
    }
    else
    {
      fRval = FALSE;
      iParseError = CLIENT_MESSAGE_PARSE_ERROR__BAD_FILLVALUE;
    }
    break;
  default:
    pClientReq->fillvalue[0] = '\0';
    
    break;
  }
  
  return iParseError;
  
PARSEMSG_ERROR:
  /* handle errors, and send error response to client. or alert caller than
  an error needs to be sent
  */
  logit("t","Error in ParseMsg, while attempting to parse client's request: (%s)\n",
  ClientMessageParseErrorArray[iParseError]);
  return(iParseError);
}  /* End ParseMsg() */



BOOL ProcessMsg( SOCKET iSocketDesc, CLIENT_MSG *pClientReq )
     /* This is the Server_Thread's bread-and-butter.  Here is where we
      * actually figure out what the client wants from us and do it for 'em.  */
{
  BOOL            fRval = TRUE;
  char            buffer[kREQID_LEN + 2];
  int             buf_len, x, rc;
  TANK           *pTempTank;
  char            szErrorBuffer[96];
  int             bSendErrorBuffer=FALSE;
  int             ErrorBufferLen;
  char            Text[256];
  /*              Validate param: */
  ew_assert(pClientReq != NULL, __LINE__);

  if (Debug > 2)
    logit("", "ProcessMsg: Processing client request message.\n");
	
  /* Process the client message: */
  sprintf(buffer, "%s ", pClientReq->reqId);
  buf_len = (int)strlen(buffer);

  /* Figure out which type of request it is. */
  switch (pClientReq->fMsg)
  {
  case kMENU:
  case kSCNMENU:
    {
      if ( send_ew(iSocketDesc, buffer, buf_len, 0, 
                   SocketTimeoutLength) != buf_len )
      {
        fRval = FALSE;
        logit("et", "Failed to write request id.\n");
        return fRval;
      }

      /* only dump out a menu if this request is from an SCNL client,
       * as that is the only kind we talk to!
      */
         
      if(pClientReq->fMsg == kMENU)
      {
        /* Walk the list of tanks sending a summary of each one... */
        for (x = 0; x < nTanks; x++)
        {
          if ( (rc = _writeTankSummary(iSocketDesc, &Tanks[x])) < 0 && 
               rc != RET_ERR_NODATA )
          {
            sprintf(Text, "Failed to write tank summary for [%s,%s,%s,%s]\n",
                    Tanks[x].sta, Tanks[x].chan, Tanks[x].net, Tanks[x].loc);
            wave_serverV_status(TypeError,  ERR_FAILED_TANK_SUMMARY, 
                                Text);
            if(bAbortOnSingleTankFailure)
            {
              /* only break if bAbortOnSingleTankFailure is set */
              fRval = FALSE;
              break;
            }
          }
        }  /* end for nTanks */
      }  /* end if menu request from SCNL client */

      /*           Shove the end-of-data ("\n") out on the socket: */
      if ( send_ew(iSocketDesc, "\n", 1, 0, SocketTimeoutLength) != 1 )
      {
        fRval = FALSE;
        logit("et", "Failed to write EOF after tank summary.\n");
        break;
      }
    }  /* End case kMENU: */
    break;

  case kMENUPIN:
    {
      if ( send_ew(iSocketDesc, buffer, buf_len, 0,
                   SocketTimeoutLength) != buf_len )
      {
        fRval = FALSE;
        logit("et", "Failed to write request id.\n");
        return fRval;
      }
      
      /* Walk the list of tanks looking for a matching pin#... */
      for (x = 0; x < nTanks; x++)
      {
      /*      When we find the first occurance of our pin#, 
        *      send the tank summary and stop... */
        if (Tanks[x].pin == pClientReq->pin)
        {
          if ( (rc = _writeTankSummary(iSocketDesc, &Tanks[x])) < 0 &&
               rc != RET_ERR_NODATA )
          {
            sprintf(Text, "Failed to write tank summary for [%s,%s,%s,%s]\n",
                    Tanks[x].sta, Tanks[x].chan, Tanks[x].net, Tanks[4].loc);
                    wave_serverV_status(TypeError,  ERR_FAILED_TANK_SUMMARY, 
                    Text);
            if(bAbortOnSingleTankFailure)
            {
              /* only break if bAbortOnSingleTankFailure is set */
              fRval = FALSE;
              break;
            }
          } /* if _writeTankSummary failed with significant error */
        }
      }  /* End for nTanks */
      if(x==nTanks)
      {
        /* Tank not found */
        sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId, 
          pClientReq->pin, pClientReq->site, pClientReq->channel,
          pClientReq->network, pClientReq->location, "FN");
        bSendErrorBuffer=TRUE;
      }

      /* Shove the end-of-data ("\n") out on the socket: */
      if ( send_ew(iSocketDesc, "\n", 1, 0, SocketTimeoutLength) != 1 )
      {
        fRval = FALSE;
        break;
      }
    }  /* End case kMENUPIN: */
    break;

  case kMENUSCNL:
    {
      fRval = FALSE;
      if ( send_ew(iSocketDesc, buffer, buf_len, 0,
        SocketTimeoutLength) != buf_len )
      {
        logit("et", "Failed to write request id.\n");
        return fRval;
      }
      
      pTempTank = FindSCNL(Tanks,nTanks,pClientReq->site, pClientReq->channel,
			   pClientReq->network, pClientReq->location);
      if(pTempTank)
      {
        if ( ( rc = _writeTankSummary(iSocketDesc, pTempTank)) < 0 &&
          rc != RET_ERR_NODATA )
        {
          sprintf(Text, "Failed to write tank summary for [%s,%s,%s,%s]\n",
            pTempTank->sta, pTempTank->chan, pTempTank->net, pTempTank->loc);
          wave_serverV_status(TypeError,  ERR_FAILED_TANK_SUMMARY, 
            Text);
          if(bAbortOnSingleTankFailure)
          {
            /* only break if bAbortOnSingleTankFailure is set */
            fRval = FALSE;
            break;
          }
        } /* if _writeTankSummary failed with significant error */
        
        /*  Shove the end-of-data ("\n") out on the socket: */
        if ( send_ew(iSocketDesc, "\n", 1, 0, SocketTimeoutLength) != 1 )
        {
          fRval = FALSE;
          break;
        }
      }
      else
      {
        /* Tank not found */
        sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId, 
          pClientReq->pin, pClientReq->site, pClientReq->channel,
          pClientReq->network, pClientReq->location, "FN");
        bSendErrorBuffer=TRUE;
      }
      
    }  /* End case kMENUSCNL: */
    break;
    
  case kGETPIN:
    {
      /*             Walk the list of tanks looking for a matching pin#... */
      for (x = 0; x < nTanks; x++)
      {
      /* When we find the first occurance of our pin#, send the tank
       *        summary and stop... */
        if (Tanks[x].pin == pClientReq->pin)
        {
          if (_writeTraceDataAscii(iSocketDesc, &Tanks[x], 
            pClientReq) < 0)
          {
            logit("et", "server_thread.c: _writeTrDAsc1() failed on tank %s\n", Tanks[x].tankName );
            fRval = FALSE;
            sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId, 
              pClientReq->pin, pClientReq->site, pClientReq->channel,
              pClientReq->network, pClientReq->location, "FC");
            bSendErrorBuffer=TRUE;
          }
          break;
        }
      }
      if(x == nTanks)
      {
        /* Tank not found */
        sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId, 
          pClientReq->pin, pClientReq->site, pClientReq->channel,
          pClientReq->network, pClientReq->location, "FN");
        bSendErrorBuffer=TRUE;
      }
    }
    break;

  case kGETSCNL:
    {
      fRval = FALSE;

      pTempTank=FindSCNL(Tanks,nTanks,pClientReq->site,
                       pClientReq->channel, pClientReq->network,
			pClientReq->location);
      if(pTempTank)
      {
        fRval = TRUE;
        if (Debug > 2)
            logit("","ProcessMsg: Found tank matching our scn. Calling writeTraceDataAscii\n");

        if (_writeTraceDataAscii(iSocketDesc, pTempTank, pClientReq) < 0)
        {
          logit("et", "server_thread.c: _writeTrDAsc2() failed on tank[%d] for %s.%s.%s.%s\n", 
            pTempTank-Tanks, 
            pClientReq->site, pClientReq->channel, pClientReq->network, pClientReq->location);
          fRval = FALSE;

          /*  Tank is corrupt */
          sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId,
            pClientReq->pin, pClientReq->site, pClientReq->channel,
            pClientReq->network, pClientReq->location, "FC");
          bSendErrorBuffer=TRUE;
        }
      }
      else
      {
        /* Tank not found */
        sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId, 
          pClientReq->pin, pClientReq->site, pClientReq->channel,
          pClientReq->network, pClientReq->location, "FN");
        bSendErrorBuffer=TRUE;
      }
    }
    break;


  case kGETSCNLRAW:
    {
      fRval = FALSE;

      pTempTank=FindSCNL(Tanks,nTanks,pClientReq->site,pClientReq->channel, 
			pClientReq->network, pClientReq->location);
      if(pTempTank)
      {
        fRval = TRUE;

        if (Debug > 2)
          logit("","ProcessMsg: Found tank matching our scn. Calling writeTraceDataRaw\n");

        if (_writeTraceDataRaw(iSocketDesc, pTempTank, pClientReq) < 0)
        {
          logit("et", "server_thread.c: _writeTrDRaw() failed on tank[%d] for %s.%s.%s.%s\n", 
                pTempTank-Tanks,
                pClientReq->site, pClientReq->channel, pClientReq->network, pClientReq->location);
          fRval = FALSE;
          /* DK Send status message writeTraceDataRaw() failed for tank %d.
             Tank may be corrupt  */
          sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId,
            pClientReq->pin, pClientReq->site, pClientReq->channel,
            pClientReq->network, pClientReq->location, "FC");
          bSendErrorBuffer=TRUE;
        }
      }  /* if (pTempTank) */
      else
      {
        /* Tank not found */
        sprintf(szErrorBuffer, "%s %d %s %s %s %s %s \n", pClientReq->reqId, 
          pClientReq->pin, pClientReq->site, pClientReq->channel,
          pClientReq->network, pClientReq->location, "FN");
        bSendErrorBuffer=TRUE;
      }

    }
    break;
    
  default:
    {
      /* Internal error:  unrecognized reqtype */
      sprintf(szErrorBuffer, "ERROR PROCESSING %s %d \n",
              pClientReq->reqId,pClientReq->fMsg);
      bSendErrorBuffer=TRUE;

      fRval = FALSE;
      break;
    }
  }  /* End switch(Request type) */

  if(bSendErrorBuffer)
  {
    ErrorBufferLen = (int)strlen(szErrorBuffer);
    if (send_ew(iSocketDesc, szErrorBuffer, ErrorBufferLen, 0, SocketTimeoutLength) 
        != (int)ErrorBufferLen )
    {
      logit("t","Error sending error message reply to client.\n");
    }
  }

  return fRval;
}  /* End ProcessMsg() */



int _getField( char *pRcvBuffer, int iField, char *pSBuffer )
     /* Scans through supplied input string <pRcvBuffer> until it identifies
      * the token occupying the <iField>th field in the record.  Uses
      * whitespace as the field delimiters, and permits a newline to be the
      * last whitespace character in the input string.  The identified token
      * string is then written into the semantic value buffer <pSBuffer> for
      * return to the calling routine.  <pRcvBuffer> is NOT modified.  Returns
      * number of characters in identified token.
      *	
      * <iField> is a 1-based ordinal index.  */
{
  int                     iRval = 0;
  char            *pStart;
  int                     span;
  int                     x;
  char                    szWorkBuffer[kMAX_CLIENT_MSG_LEN+1];

	
  /*              Validate params: */
  ew_assert(pRcvBuffer != NULL, __LINE__);
  ew_assert((iField > 0) && (iField <= kMAX_CLIENT_FIELDS), __LINE__);
  ew_assert(pSBuffer != NULL, __LINE__);

  /* Assume an arbitrary minimum length message, to make the algorithm
   * simpler: */
  if ((int)strlen(pRcvBuffer) > 3)
    {
      /*              Need this because strtok() hacks up our parameter... */
      strcpy(szWorkBuffer, pRcvBuffer);

      /* If we aren't looking for the first field, offset ourselves into the
       * buffer to the start of the <iField>th field... */
      pStart = strtok(szWorkBuffer, " \n");
      if (iField > 1)
	for (x = 1; (x <= iField - 1) && pStart; x++)
	  pStart = strtok(NULL, " \n");
		
      /* Now find the end of the field and copy it to the semantic value
       * buffer... */
      if (pStart)
	{
	  span = (int)strcspn(pStart, " \n");
	  if (span > 0)
	    {
	      strncpy(pSBuffer, pStart, span);
	      pSBuffer[span] ='\0';
	      iRval = span;
	    }
	}
    }

  return iRval;
}



int _writeTankSummary( SOCKET iSocketDesc, TANK *pTank )
     /* For a given tank, extracts pertinent summary data, formats it, and
      * writes it out down the socket to the eagerly awaiting client.  */
{
  int                             iRval = 1;
  char                    szMsgBuf[kMAX_CLIENT_MSG_LEN+1];
  int                             iMsgLen;
  DATA_CHUNK              *pYoungest;
  DATA_CHUNK              *pOldest;

  /*              Validate params: */
  if (Debug > 2)
     logit("", "_writeTankSummary: iSocketDesc = %ld, ", (long)iSocketDesc);

  ew_assert(iSocketDesc > 0, __LINE__);
  ew_assert(pTank != NULL, __LINE__);

  /*              Access the first and last chunks in the tank... */
  pYoungest = IndexYoungest(pTank);
  pOldest = IndexOldest(pTank);

  if (pYoungest && pOldest && pTank->datatype[0] 
      && (pYoungest->tEnd > 0.0) && (pYoungest->tEnd > 0.0)
      && pTank->sta[0] && pTank->chan[0] && pTank->net[0] && pTank->loc[0]
     )
    {
      if (Debug > 2)
         logit("", "_writeTankSummary: There is data in the tank in question.\n");

      /*              Read our SCNL from the tank in question... */
      /*              Format our output: */
      sprintf(szMsgBuf, " %d %s %s %s %s %lf %lf %s ", pTank->pin, pTank->sta, 
	      pTank->chan, pTank->net, pTank->loc, pOldest->tStart, 
	      pYoungest->tEnd, pTank->datatype);
      iMsgLen = (int)strlen(szMsgBuf);
      /*              Push it out to the client: */
      if (Debug > 2)
          logit("e", "_writeTankSummary: Reply to Menu: \n.%s.\n\n", szMsgBuf);

      if ( ( iRval = send_ew(iSocketDesc, szMsgBuf, iMsgLen, 0,
			     SocketTimeoutLength)) != iMsgLen )
	{
	  logit("et", "_writeTankSummary: error sending menu reply\n");
	  return RET_ERR_SOCKET;
	}
    }
  else
    {
      if (Debug > 2)
         logit("t", "_writeTankSummary: Warning: no data in tank %s for pin %d \n",
   							 	    pTank->tankName,pTank->pin);

      iRval = RET_ERR_NODATA;
    }

  return iRval;
}

void _xvertToUpper( char *pBuffer )
     /* Windows NT gives you a routine that does this for you, but alas, it
      * isn't yet part of the ANSI standard lexicon.  Huh.  Finally something
      * good we can say about Microsoft!
      *	
      * This routine converts all of the characters in <pBuffer> into upper
      * case.  Simple 'n' stupid.  */
{
  int                     iStrLen;
  int                     x;

	
  /*              Validate param: */
  ew_assert(pBuffer != NULL, __LINE__);

  /* Walk the string converting each character to uppercase.  (Assumes ANSI
   * version of STDLIB which does if(islower(c)) then toupper(c) implicitly.
   * If this has to port to preANSI, add call to islower()). */
  iStrLen = (int)strlen(pBuffer);
  for (x = 0; x < iStrLen; x++)
    pBuffer[x] = toupper(pBuffer[x]);
}



void ew_assert( BOOL fExp, int iLineNum )
{
  if (!fExp)
    {
      logit("", "Assertion failed!  File: %s  Line: %d\n", __FILE__, iLineNum);
      exit(-1);
    }
}

int server_thread_c_init()
{
  logit("t","server_thread.c:Version 1160412407\n");
  return(0);
}
