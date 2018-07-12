// Wave.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSocket.h"
#include "Wave.h"
#include "WaveDoc.h"
#include "surf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWave
static CString cErr;
static int RequestQueueMaxSize = 15;

IMPLEMENT_DYNCREATE(CWave, CAsyncSocket)



/* Useless Default constuctor
********************************************************************/
CWave::CWave()
{

}


/* Useful constructor that supplies server info and CWaveDoc pointer.
********************************************************************/
CWave::CWave(CString * cServer, int nPort, CWaveDoc * pWaveDoc)
{
  this->pWaveDoc = pWaveDoc;
  this->sServer  =*cServer;
  this->nPort    = nPort;
  iSock = WAVESOCKET_STATUS_NOT_CREATED;
  this->iMsgLen = 0;
  this->bWaitingForReply = FALSE;
  this->iMsg = 0;

  // We need three locks for the mutex that protects the queue:
  //  HNRLock is used by HandleNextRequest()
  //  GMLock is a lock for GetMenu()
  //  GSCNLLock is a lock for GetSCNL()
  // There are three unique mutually exclusive locking
  //  mechanisms that are all operating on the same mutex:
  //  the queue mutex.  Each method that can lock the queue
  //  for a different reason, gets its own lock, because
  //  of the way MFC handles mutex locking.
  psHNRLock =  new CSingleLock(&(m_mutex));
  psGMLock =   new CSingleLock(&(m_mutex));
  psGSCNLLock = new CSingleLock(&(m_mutex));

  // We are all set, now try to get a connection from the server.
  Connect();
}


/* Default destructor method.  It clears out all of the params.  I
   think this method just pretends to do something.  It's not 
   actually useful.
********************************************************************/
CWave::~CWave()
{
  this->pWaveDoc = NULL;
  this->sServer  = "";
  this->nPort    = 0;
  iSock = WAVESOCKET_STATUS_NOT_CREATED;
}



// Define the two CWave request types
#define CWAVE_REQUEST_TYPE_MENU 1
#define CWAVE_REQUEST_TYPE_SCNL  2


/* CWaveRequest Class
  We cheat and declare a new class within this file.  It's instantiaton
  is private to CWave.  It has no methods only data members.  It would
  have been implemented as a Struct, but it needed to be derived
  from CObject, so that it could be used in a CObject array.
  The class represents a waveform request, including a request type,
  a start and end time, and channel information via a CSite.
********************************************************************/
class CWaveRequest : public CObject
{
public:
  int   iRequestType;
  CSite Site;
  double tStart;
  double tEnd;
};


CObArray RequestQueue;
CWaveRequest * pCurrentRequest;


/* Method to request trace data for a channel.  This method should be
   called by CWaveDoc or someone affilliated with it, as we will 
   return the results to the CWaveDoc Object that created us.  
   We assume any requests have already been broken up into GULP
   sized chunks.
********************************************************************/
void CWave::GetSCNLTraceData(CSite * pSite, double tStart, double tEnd)
{
  // Sanity check
  if(tStart > tEnd)
  {
    return;
  }
  // if tStart == tEnd, then we are asking for one sample.
  // current incarnations of wave_server don't take too well to
  // one-sample requests, so we have to adjust to their limitations.
  // when the original request is 1 sample, add an arbitrary amount
  // of time to the request in order to accomodate wave_server 
  // behavior defects.
  else if(tStart == tEnd)
  {
    tEnd = tStart + 1.0;
    //return;  // This should be a legal request since it is 1 sample, 
               // but wave_server has behavioral problems.
  }

  // Lock the request queue, cause we're gonna add one.
  psGSCNLLock->Lock();

  // Create a new CWaveRequest and initialize it with the parameters
  // of the current request.
  CWaveRequest * pRequest = new CWaveRequest;
  pRequest->iRequestType = CWAVE_REQUEST_TYPE_SCNL;
  pRequest->Site = *pSite;
  pRequest->tStart = tStart;
  pRequest->tEnd = tEnd;

  // If the request queue is not full, add the request,
  // otherwise delete it and pretend like we never saw it.
  if(RequestQueue.GetSize() < RequestQueueMaxSize)
  {
    RequestQueue.Add(pRequest);
  }
  else
  {
    delete(pRequest);
  }


  // release the request queue lock.
  psGSCNLLock->Unlock();

  // We just modified the request queue.  Run HandleNextRequest()
  // to see if there is any processing to be done on the queue.
  HandleNextRequest();

  // Issue a new status message indicating the current queue size.
  sprintf(sStatus, "Q: %d",RequestQueue.GetSize());
  pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_GROUP, (CObject *)&((CString)sStatus));

  return;
}  // end CWave::GetSCNLTraceData()


/* Method to report the number of requests currently in the queue.
********************************************************************/
int  CWave::GetRequestQueueSize(void)
{
  return(RequestQueue.GetSize());
}


void CWave::SetRequestQueueLen(int iLen)
{
  RequestQueueMaxSize = iLen;
}

/* Method to empty the request queue.  This is normally called when
   a new menu request(GetMenu) is submitted.  The idea is, the user has 
   shifted the display so severely, that the old requests probably
   are no longer of interest, and the new menu is very important.
   So the queue gets emptied, and the menu gets added, and then 
   the program is free to make new requests that will be immediately
   handled instead of having to wait for queue to clear.
********************************************************************/
void CWave::ClearRequestQueue(void)
{
  int iSize;

  iSize = RequestQueue.GetSize();
  for(int i=0; i < iSize; i++)
  {
    delete((CWaveRequest *)(RequestQueue.GetAt(i)));
  }
  RequestQueue.RemoveAll();
}


/* Method to request and updated/new menu from the wave_server.
   A GetMenu request takes precedence over all trace requests,
   so this method will clear the request queue and then place 
   itself at the front.
********************************************************************/
void CWave::GetMenu(void)
{
  CWaveRequest * pRequest;

  // Use our lock to secure the Request queue 
  psGMLock->Lock();

  // If we're grabbing a menu, then something major must
  // have happened, so abandon all existing requests.
  ClearRequestQueue();

  // Add the menu request to the empty queue
  pRequest = new CWaveRequest;
  pRequest->iRequestType = CWAVE_REQUEST_TYPE_MENU;
  RequestQueue.Add(pRequest);

  // Release the request request queue
  psGMLock->Unlock();
 
  // Call HandleNextRequest() since we've modified the queue.
  HandleNextRequest();
}  // end CWave::GetMenu()


/* Callback method, used by the asynchronous socket class CWaveSocket
   to alert us that we have connected to the server.
   Designed to be called by CWaveSocket::OnConnect()
********************************************************************/
void CWave::OnSocketConnect(int err)
{
  // If there was an error connecting, then reset the socket status
  // to closed and issue a status message to the viewer.
  if(err)
  {
    sprintf(sStatus, "ERROR Connecting");
    pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);
    iSock = WAVESOCKET_STATUS_CLOSED;
  }
  else
  {
    // Otherwise, we got a connection to the server.  Issue a status message
    // update the socket status, and call HandleNextRequest() to start 
    // processing the queue.
    sprintf(sStatus, "Connected To Server");
    pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);
    TRACE("CWave:OnSocketConnect(): SocketConnected!\n");
    iSock = WAVESOCKET_STATUS_READY;
    HandleNextRequest();
  }
}  // end CWave::OnSocketConnect()


/* Callback method, used by the asynchronous socket class CWaveSocket
   to alert us that our connection to the server has been closed for
   some reason.
   Designed to be called by CWaveSocket::OnClose()
********************************************************************/
void CWave::OnSocketClose(int err)
{
  // Update the status of the socket, and issue a message to the viewer.
  iSock = WAVESOCKET_STATUS_CLOSED;
  iMsgLen = 0;
  sprintf(sStatus, "Connection Closed");
  pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);

  // Get rid of the old socket, it is cleaner just to start over.
  delete(pWaveSocket);

  // reinitialize some variables.
  bWaitingForReply = FALSE;
  iSock = WAVESOCKET_STATUS_NOT_CREATED;

  // Call HandleNextRequest() to restart processing.
  HandleNextRequest();
}


/* Method to examine the request queue, the request state, and the
   connection state, and perform any processing deemed neccessary.
   This is the workhorse that keeps everything in order, and submits
   the requests to the server via the socket.  This IS the 900LB 
   gorrila of the CWave class.
********************************************************************/
void CWave::HandleNextRequest(void)
{
  UINT iBytesSent;

  // first, check the request state.  
  // We have either submitted a request to the server and are waiting
  //  for a reply
  // or
  // We are free to submit a request.
  if(!bWaitingForReply)
  {
    // We are free to make a request.

    // Secure the request queue, so that we can pull the next request
    // off of it.
    psHNRLock->Lock();

    // Issue a status message telling the current size of the queue
    sprintf(sStatus, "Q: %d",RequestQueue.GetSize());
    pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_GROUP, (CObject *)&((CString)sStatus));

    // If we have a left-over existing request, then delete it, since
    // we are already done with it.
    if(pCurrentRequest != NULL)
    {
      delete(pCurrentRequest);
      pCurrentRequest = NULL;
    }

    // Check to see if there is anything in the queue.
    if(RequestQueue.GetSize() > 0)
    {
      // Alright, we found something in the queue.
      // Now check the status of the socket to make
      // sure we are a go.
      if(iSock == WAVESOCKET_STATUS_READY)
      {
        // Good, the connection is setup.

        // Get and Remove the next request from the queue.
        pCurrentRequest = (CWaveRequest *)(RequestQueue.GetAt(0));
        RequestQueue.RemoveAt(0);

        // Now figure out what type of a request it is.
        if(pCurrentRequest->iRequestType == CWAVE_REQUEST_TYPE_MENU)
        {
          // Ah, it's a menu.  Formulate a menu request
          char szRequest[100];
          sprintf(szRequest, "MENU: MN%d SCNL\n", iMsg++);
          // Send the request to the server.
          if(pWaveSocket->Send(szRequest,strlen(szRequest)) == SOCKET_ERROR)
          {
            // Ah Crap!!!! We got a socket error sending the menu request.
            // Wait a while and try again???
            Sleep(500);
            if(pWaveSocket->Send(szRequest,strlen(szRequest)) == SOCKET_ERROR)
            {
              // Ugh!! We tried twice and got an error each time.  There must
              // be some kind of serious problem.  Issue an error message
              // via pWaveSocket!!
              pWaveSocket->Error("CWave::HandleNextRequest(MENU)");
            }
            else
            {
              //  Good, it worked the second time (we think).  Issue a status
              //  message and wait for a reply!!
              sprintf(sStatus, "MENU Sent, Waiting");
              pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);
              time(&tWaitingForReply);
              this->bWaitingForReply = TRUE;
            }
          }
          else
          {
            //  Good, it worked the first time (we think).  Issue a status
            //  message and wait for a reply!!
           sprintf(sStatus, "MENU Sent, Waiting");
            pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);
            time(&tWaitingForReply);
            this->bWaitingForReply = TRUE;
          }
        }
        else if(pCurrentRequest->iRequestType == CWAVE_REQUEST_TYPE_SCNL)
        {
          // OK, it's a waveform request

          // formulate the request
          char szRequest[200];
          if(pWaveDoc->iWaveServerType == WAVE_SERVER_TYPE_SCNL)
          {
            sprintf(szRequest, "GETSCNL: GS%d %s %s %s %s %.3f %.3f %s\n",
                    iMsg++, 
                    pCurrentRequest->Site.cSta, pCurrentRequest->Site.cChn, 
                    pCurrentRequest->Site.cNet, pCurrentRequest->Site.cLoc,
                    pCurrentRequest->tStart, pCurrentRequest->tEnd,
                    FILL_VALUE);

          }
          else if(pWaveDoc->iWaveServerType == WAVE_SERVER_TYPE_SCN)
          {
            sprintf(szRequest, "GETSCN: GS%d %s %s %s %.3f %.3f %s\n",
                    iMsg++, pCurrentRequest->Site.cSta, 
                    pCurrentRequest->Site.cChn, pCurrentRequest->Site.cNet, 
                    pCurrentRequest->tStart, pCurrentRequest->tEnd,
                    FILL_VALUE);

          }
          else
          {
            // uh, we're in trouble, it's a wave_server type that we don't understand
            szRequest[0] = 0x00;
            sprintf(sStatus, "ERROR: UNKNOWN WAVE SERVER TYPE!!");  
            pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, 
                                     (CObject *)&(CString)sStatus);
            return;
          }
          // Send the request
          if((iBytesSent = pWaveSocket->Send(szRequest,strlen(szRequest))) == SOCKET_ERROR)
          {
            // We got a socket error sending the request.  Report an
            // error via pWaveSocket()
            pWaveSocket->Error("CWave::HandleNextRequest(SCNL)");
          }
          else
          {
            // Alright we sent it (or atleast part).  Issue a status 
            // message to the viewer.
            sprintf(sStatus, "");
            pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_RECEIVE, (CObject *)&(CString)sStatus);
            
            // Did we send it all?  Update status indicating
            // what we were able to send.  So much for robustness
            // and error handling (atleast we complained)
            if(iBytesSent != strlen(szRequest))
            {
              sprintf(sStatus, "ERROR: Partial GETSCNL Sent");  
              pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, 
                                       (CObject *)&(CString)sStatus);
            }
            else
            {
              strcpy(sStatus,szRequest);
              pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, 
                                       (CObject *)&(CString)sStatus);
              this->bWaitingForReply = TRUE;
              time(&tWaitingForReply);
            }
          }
        }
        else  // Some Garbled Request Type, blow off this one and go to the next
        {
          HandleNextRequest();  // Ooh, recursion!!
        }
      }
      else //(iSock == WAVESOCKET_STATUS_CLOSED || NOT_CREATED)
      {
        Connect();
      }
    }
    else  // nothing in the request queue
    {
      // Unblock any waiting CTrace objects, so that they can make more
      // requests
      pWaveDoc->UnblockWaitingTraces();
      // Issue a status message that we are done.
      sprintf(sStatus, "Done");
      pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);
    }

    // Release the request queue.
    psHNRLock->Unlock();
  }  // end if !bWaitingForReply
  else
  {
    // We're waiting for a reply, but maybe the server got confused,
    // or there was a network problem, or there's a blue moon, etc.
    // So let's put a time limit on how long we will wait for a reply.
    if(time(NULL) - tWaitingForReply > CLIENT_SOCKET_TIMEOUT)
    {
      // We've exceeded the time limit.  Close the socket and start over.
      if(iSock == WAVESOCKET_STATUS_READY)
      {
        this->pWaveSocket->Close();
        this->OnSocketClose(WSAEWOULDBLOCK);
      }
    }
  }
}  // end CWave::HandleNextRequest() 


// Define a HUGE Receive buffer, since there is only one of these
// and we don't want to worry about ever overflowing it.
#define RECEIVE_BUFFER_LEN 80000
static char Buffer[RECEIVE_BUFFER_LEN];


/* Callback method, used by the asynchronous socket class CWaveSocket
   to alert us that there is data from the server in the socket buffer,
   and that we can call Receive to get at it.
   Designed to be called by CWaveSocket::OnReceive()
********************************************************************/
void CWave::OnSocketReceiveData(char * pBuffer, int BufLen)
{

  // Issue a status message saying that we got data, and how much we got.
//  sprintf(sStatus, "Received Data");
//  pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);
  sprintf(sStatus, "%d", BufLen);
  pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_RECEIVE, (CObject *)&(CString)sStatus);

  // Append this data to our existing buffer of received data.
  // First check to make sure it won't overflow the buffer
  if(RECEIVE_BUFFER_LEN <= iMsgLen + BufLen)
  {
    //  Ignore the new data, since it would overflow the buffer
    //  This code looks kinda buggy, but should never run because
    //  of our monstrous buffer. 
    BufLen = RECEIVE_BUFFER_LEN - iMsgLen - 1;
    if(BufLen < 0) 
      BufLen = 0;
    TRACE("ERROR!! Msg too long for buffer!! in CWave::OnSocketReceiveData\n");
  }
  else
  {
    // the new data plus the old will fit in the buffer.  Append
    // the new data to the existing data (if any) in the buffer.
    memcpy(&Buffer[iMsgLen],pBuffer,BufLen);
    for(int i=0; i < BufLen; i++)
    {
      // if we come across a null character, just convert it to a space.
      // this is an artifact of an old wave_server bug.
      if(Buffer[i+iMsgLen] == 0x00)
        Buffer[i+iMsgLen] = ' ';
    }
    // Note the new buffer length
    iMsgLen += BufLen;

    // if the new message ends in a linefeed, then we have received a full
    // message.  Process it.
    if(Buffer[iMsgLen-1] == '\n')
    {
      // We've got a completed message
      Buffer[--iMsgLen] = 0x00;  // Null terminate the string
      if(Buffer[0] == 'M' && Buffer[1] == 'N')
      {
        // Ah, it's a menu request, update status, and pass the
        // request back to our WaveDoc.
        char * pMenu;
        pMenu=strchr(Buffer,' ') + 1;
        sprintf(sStatus, "+%d",iMsgLen);
        pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_RECEIVE, (CObject *)&(CString)sStatus);
       pWaveDoc->HandleWSMenuReply(pMenu);
      }
      else if(Buffer[0] == 'G' && Buffer[1] == 'S')
      {
        // Ah, it's a waveform request, update status, and pass the
        // request back to our WaveDoc.
        char * pSCNL;
        pSCNL=strchr(Buffer,' ') + 1;
        sprintf(sStatus, "*%d",iMsgLen);
        pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_RECEIVE, (CObject *)&(CString)sStatus);
        pWaveDoc->HandleWSSCNLReply(pSCNL,&(pCurrentRequest->Site));
      }
      else
      {
        // Uh, no clue what kind of request it is. 
        // Give up and keep going!
        TRACE("We have a problem\n");
        sprintf(sStatus, "-%d",iMsgLen);
        pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_RECEIVE, 
                                 (CObject *)&(CString)sStatus);
      }

      // We got a reply, so update state variables.
      this->bWaitingForReply = FALSE;
      this->iMsgLen = 0;

      // Call HandleNextRequest() to process the next one.
      HandleNextRequest();
    }
  }  // We got some data in the buffer, but not enough to cause overflow

  return;
} // end CWave::OnSocketReceiveData()


/* Method to Connect to a wave_server via a CWaveSocket.
********************************************************************/
void CWave::Connect(void)
{
  BOOL bRetCode;

  // Issue a status message.
  sprintf(sStatus, "Attempting to Connect to Server");
  pWaveDoc->UpdateAllViews(NULL,ON_UPDATE_SB_STATUS, (CObject *)&(CString)sStatus);

  // If we have already requested a connection and it is currently
  // pending, then do nothing.
  if(iSock == WAVESOCKET_STATUS_CONNECTION_PENDING)
    return;

  // If the socked doesn't already exist, then create it.
  if(iSock == WAVESOCKET_STATUS_NOT_CREATED)
  {
    
    pWaveSocket = new CWaveSocket();

    bRetCode = pWaveSocket->Create(this);
    if(bRetCode)
      iSock = WAVESOCKET_STATUS_CLOSED;
    else
      return;
  }

  // Make sure we are not in some unexpected socket state,
  // like READY
  if(iSock != WAVESOCKET_STATUS_CLOSED)
  {
    // We're in an unexpected state, close the socket, and
    // move ot the closed state.
    pWaveSocket->Close();
    iSock = WAVESOCKET_STATUS_CLOSED;
  }

  // We're all set to try to connect.  Connect!
  if(pWaveSocket->Connect((LPCTSTR)(this->sServer), this->nPort))
  {
    // Adjust the socket state.  Note that we are currently trying
    // to get a connection.  Don't attempt to connect again, while
    // our current connection request is pending.
    iSock = WAVESOCKET_STATUS_CONNECTION_PENDING;
  }
  else
  {
    // The connect request bombed.  Destroy the socket and try again!!
    delete(pWaveSocket);
    iSock = WAVESOCKET_STATUS_NOT_CREATED;
    this->Connect();
  }

}  // end CWave::Connect()
