/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: e2l_server.c 5701 2013-08-05 00:10:26Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/05 21:45:22  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * Routines for ServerThread, a part of ew2liss.
 */

#include <earthworm.h>
#include <socket_ew.h>
#include "ew2liss.h"
#ifndef _WINNT
#include <signal.h>
#endif

/*
 * ServerThread: Listens on a socket for a LISS client; servers miniSEED
 * data from the output queue to the client. Only a single client is handled
 * here.
 */
thr_ret ServerThread(void *E2L)
{
  WORLD *pE2L = (WORLD *)E2L;
  SOCKET listenSocket;
  SOCKET sendSocket;
  struct sockaddr_in  sin;
  struct sockaddr_in  client;
  int on;
  int clientLen;
  char client_ip[16];
  int sockErr;
  
#ifndef _WINNT
   (void)sigignore(SIGPIPE);
#endif
  
   /* Initialize the socket system
   *******************************/
   SocketSysInit();
   if ( SetAddress( &sin, *pE2L ) )
   {
     logit("e", "ew2liss: error setting LISS address. Exiting\n");
     pE2L->terminate = 1;
     KillSelfThread();
   }
   pE2L->ServerStatus = SS_DISCO;
   
   /* Our main loop */
   while( pE2L->terminate != 1)
   {
     if ( ( listenSocket = socket_ew( PF_INET, SOCK_STREAM, 0) ) 
          == INVALID_SOCKET )
     {
       logit( "et", "ew2liss: Error listening socket; <%d> exiting!\n",
              socketGetError_ew());
       pE2L->terminate = 1;
       KillSelfThread();
     }
   
     /* Allows the server to be stopped and restarted */
     on=1;
     if( setsockopt( listenSocket, SOL_SOCKET, SO_REUSEADDR, 
                     (char *)&on, sizeof(char *) ) != 0 )
     {
       logit( "et", "ew2liss: Error on setsockopt; <%d>; exiting!\n", 
              socketGetError_ew());
       pE2L->terminate = 1;
       KillSelfThread();
     }

     /* Bind socket to a name */
     if ( bind_ew( listenSocket, (struct sockaddr *) &sin, sizeof(sin)) )
     {
       logit("et", "ew2liss: error binding socket <%d>; exiting.\n", 
             socketGetError_ew());
       perror("Export bind error");
       closesocket_ew(listenSocket,SOCKET_CLOSE_IMMEDIATELY_EW);
       pE2L->terminate = 1;
       KillSelfThread();
     }
     
     /* Prepare for connect requests */
     if ( listen_ew( listenSocket, 0))
     {
       logit("et", "ew2liss: socket listen error <%d>; exiting!\n", 
             socketGetError_ew() );
       closesocket_ew( listenSocket,SOCKET_CLOSE_IMMEDIATELY_EW );
       pE2L->terminate = 1;
       KillSelfThread();
     }
     
     /* Accept a connection */
     clientLen = sizeof( client );
     
     if ( (sendSocket = accept_ew( listenSocket, 
                                   (struct sockaddr*) &client,
                                   &clientLen, -1 ) ) == -1 )
     {
       sockErr = socketGetError_ew();
       logit("et", "ew2liss: error on accept: %d\n\t; exiting!\n", sockErr);
       closesocket_ew( listenSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
       closesocket_ew( sendSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
       pE2L->terminate = 1;
       KillSelfThread();
     }
     pE2L->ServerStatus = SS_CONN;
     strcpy( client_ip, inet_ntoa(client.sin_addr) );
     logit("et", "ew2liss: Connection accepted from IP address %s\n", client_ip);
     
     if (ServiceSocket(sendSocket, pE2L) < 0)
     {
       /* Lost connection; try again */
       closesocket_ew( listenSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
       closesocket_ew( sendSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
       pE2L->ServerStatus = SS_DISCO;
     }
     /* Sockets closed; go back for another connection */     
     
   }  /* end of while (!terminate) */
   logit("t", "ServerThread: termination requested\n");
   KillSelfThread();
   return(NULL);
}


/*
 * Given an open socket, send some LISS data to it from the output queue.
 * Returns: -1 on error or connection closed; 0 if terminate flag is set.
 */
int ServiceSocket(SOCKET sendSocket, WORLD *pE2L)
{
  char seedBuf[LISS_SEED_LEN];
  int ret;
  long bufLen;
  MSG_LOGO reclogo;
  
  while( pE2L->terminate != 1)
  {
    
    /* Grab the next miniSEED record from the output queue */
    RequestSpecificMutex(&pE2L->seedQMutex);
    ret = dequeue (&pE2L->seedQ, seedBuf, &bufLen, &reclogo);
    ReleaseSpecificMutex(&pE2L->seedQMutex);
    
    if (ret < 0)
    {                                 /* empty queue */
      sleep_ew (1000);
      continue;
    }
    
    /* Send the message */
    ret = send_ew(sendSocket, seedBuf, LISS_SEED_LEN, 0, pE2L->sockTimeout);
    if (ret == -1)
    {
      logit("et", "ew2liss: error sending message: %d\n", socketGetError_ew());
      return( -1 );
    }
    else if (ret == 0)
    {
      logit("et", "ew2liss: connection closed\n");
      return( -1 );
    }
    
  }
  return( 0 );
}



/* SetAddress: given a LISS domain name or IP address and port number,
   set up the socket address structure. Returns 0 on success, -1 on failure */
int SetAddress( struct sockaddr_in *sinP, WORLD E2L)
{
  unsigned long addr;
  struct hostent* hp;
  
  memset((void*)sinP, 0, sizeof(struct sockaddr_in));
  sinP->sin_family = AF_INET;
  sinP->sin_port = htons((unsigned short)E2L.LISSport);

  /* Assume we have an IP address and try to convert to network format.
   * If that fails, assume a domain name and look up its IP address.
   * Can't trust reverse name lookup, since some place may not have their
   * name server properly configured. 
   */
  if ( (addr = inet_addr(E2L.LISSaddr)) != INADDR_NONE )
  {
    sinP->sin_addr.s_addr = addr;
  }
  else
  {       /* it's not a dotted quad IP address */
    if ( (hp = gethostbyname(E2L.LISSaddr)) == NULL) 
    {
      logit("e", "bad server address <%s>\n", E2L.LISSaddr );
      return( -1 );
    }
    memcpy((void *) &(sinP->sin_addr), (void*)hp->h_addr, hp->h_length);
  }
  return( 0 );
  
}
