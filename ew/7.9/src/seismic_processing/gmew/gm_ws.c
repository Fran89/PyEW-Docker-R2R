/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_ws.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2006/06/06 20:34:52  paulf
 *     mods to get this to work on WINDOWS
 *
 *     Revision 1.5  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.4  2001/08/03 19:56:33  lucky
 *     Fixed to make gmew work for the synthetic Utah test:
 *     Both are in getTraceFromWS dealing with how the data returned
 *      from the wave_server got copied into the pTrace buffer:
 *     - we now check nRaw and lenRaw to make sure that we only copy
 *     the allowed number of samples.
 *     - there was a problem with the code if WS returned much more data
 *     than requested. It would work the first time, but not for subsequent
 *     packets.
 *
 *     Revision 1.3  2001/08/02 01:00:15  davidk
 *     Modified code in getGMFromWS(), so that wave_server errors are no longer
 *     fatal.  Added a warning message when no channels are successfully processed
 *     for an Event.
 *
 *     Revision 1.2  2001/07/07 21:49:16  lombard
 *     Fixed handling of bad server connection
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_ws.c: utility local-mag functions that access the wave_server.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#ifdef _WINNT
#include <windows.h>
/*  #include <winsock2.h> */
#else
#include <sys/socket.h>  /* This and next three for server hostname lookup */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#endif
#include <earthworm.h>
#include <chron3.h>
#include <kom.h>
#include <rw_strongmotionII.h>
#include <swap.h>
#include <trace_buf.h>
#include <ws_clientII.h>
#include "gm.h"
#include "gm_sac.h"
#include "gm_util.h"
#include "gm_ws.h"


static WS_MENU_QUEUE_REC queue = {NULL,NULL}; /* points to the list of menus */
static char *gtbBuf;   /* Storage for TRACEBUF messages from the wave_server */
static long gtbBufLen; /* The buffer's length when allocated */

static int Debug = 0;  /* DEBUG */

/* Allowable change in sample rate in a trace */
#define SR_TOL 1.0

/* Internal Function Prototypes */
static void fillTraceReq(STA *, COMP3 *, GMPARAMS *, EVENT *, TRACE_REQ *);
static int getTraceFromWS(TRACE_REQ *, DATABUF *, int );

/*
 * getGMFromWS: Derives ground-motion peak amplitudes from traces
 *    obtained from a list of wave_servers.
 *    The traces have already been selected in the EVENT structure.
 *  Returns: 0 on success
 *          -1 on fatal errors
 */
int getGMFromWS( EVENT *pEvt, GMPARAMS *pgmParams, DATABUF *pTrace, 
                 SM_INFO *pSMs)
{
  WS_MENU       menu = NULL;    /* pointer to WaveServer menu, as built by 
                                   ws_client routines; from ws_clientII.h */
  WS_PSCNL       pscnl;
  TRACE_REQ req;
  COMP3 *pComp;
  SERVER *this = pgmParams->pWSV->pList;
  STA *pSta;
  char date[22];
  int rc = 0;
  int iNumChannelsProcessed=0;
  char *displayEventID = pEvt->eventId;

  
  date20(pEvt->origin_time + GSEC1970, date);
  if ( pEvt->eventId[0] ) {
	logit("et", "Event id: <%s> origin: %s\n", pEvt->eventId, date);
  	logit("e", "\tlat: %10.6lf lon: %10.6lf depth: %6.2lf\n", pEvt->lat,
       	pEvt->lon, pEvt->depth);
  } else {
  	logit("et", "Request from ring to process %s\n", date );
  	logit("e", "\t(will use date for Event id)\n");
  	displayEventID = date;
  }

  /* Build the current wave server menues */
  while (this != (SERVER *)NULL)
  {
    if (queue.head != NULL) /* Clean up from the last menu */
      wsKillMenu(&queue);
    
    rc=wsAppendMenu(this->IPAddr, this->port, &queue, 
                     pgmParams->wsTimeout);
    if (rc != WS_ERR_NONE)
    {
      switch (rc) 
      {
      case WS_ERR_INPUT:
        logit("e", "getWsSCNList: missing input to wsAppendMenu()\n");
        break;
      case WS_ERR_MEMORY:
        logit("e", "getWsSCNList: wsAppendMenu failed allocating memory\n");
        goto CleanUp;
        break;
      case WS_ERR_NO_CONNECTION:
        logit("e","getWsSCNList: wsAppendMenu could not connect to %s:%s\n",
              this->IPAddr, this->port);
        break;
      case WS_ERR_EMPTY_MENU:
        logit("e","getWsSCNList: wsAppendMenu %s:%s returned empty menu!\n",
              this->IPAddr, this->port);
        break;
      case WS_ERR_SOCKET:
        logit("e", "getWsSCNList: wsAppendMenu socket error for %s:%s\n",
              this->IPAddr, this->port);
        break;
      case WS_ERR_TIMEOUT:
        logit("e","getWsSCNList: timeout getting menu from %s:%s\n",
              this->IPAddr, this->port);
        break;
      case WS_ERR_BROKEN_CONNECTION:
        logit("e","getWsSCNList: connection to %s:%s broke during menu\n",
              this->IPAddr, this->port);
        break;
      default:
        logit("e", "getWsSCNList: wsAppendMenu returned unknown error %d\n",
              rc);
      }
      goto NextServer;         /* Try the next server */
    }

    /* Parse the current menu if we got one */
    menu = queue.head;

    if(menu == (WS_MENU) NULL)
    {
      logit("e","No menu retrieved from wave_server [%s:%s] for Event %s\n",
            this->IPAddr, this->port, displayEventID);
    }
    else /*(menu != (WS_MENU) NULL) */
    {
      pscnl = menu->pscnl;
      /* Loop through all the scns in this menu, adding possible 
         stations to the list */
      while (pscnl != NULL)
      {
        if ( (rc = addCompToEvent(pscnl->sta, pscnl->chan, pscnl->net, pscnl->loc, pEvt,
                                  pgmParams, &pSta, &pComp)) < 0)
          goto CleanUp;
        else if (rc == 0)
        {
          if (pgmParams->debug)
            logit("e", "Processing <%s.%s.%s.%s>\n", pSta->sta, pComp->name, 
                  pSta->net, pComp->loc);

          /* Estimate phase arrivals, to be used for start-time calculations */
	      if ( pEvt->eventId[0] ) 
          	EstPhaseArrivals(pSta, pEvt, pgmParams->debug & GM_DBG_TIME);
    
          fillTraceReq(pSta, pComp, pgmParams, pEvt, &req);
      
          /* Get the trace for this component */
          rc = getTraceFromWS(&req, pTrace, pgmParams->wsTimeout);
          if (rc < 0)
            goto CleanUp;   /* Fatal error; already logged */
          else if (rc > 0)
            goto NextPscnl;   /* non-fatal errror; no data; already logged */
          /* Maybe someone could do more robust error handling here; good luck */
          /* Clean up the trace: demean, remove gaps */        
          prepTrace(pTrace, pSta, pComp, pgmParams);

          /* Now we have a generic trace; turn it into a Wood-Anderson trace */
          rc = makeGM( pSta, pComp, pgmParams, pEvt, pTrace );
          if (rc > 0)  
            goto NextPscnl;   /* non-fatal error, logged by makeWA */
          else if (rc < 0)
            goto CleanUp;  /* fatal error, logged by makeWA */
        
          getPeakGM( pTrace, pComp, pSta, pgmParams, pEvt, pSMs );
        
          if (pgmParams->saveTrace == GM_ST_SAC)
            saveSACGMTraces( pTrace, pSta, pComp, pEvt, pgmParams, pSMs);

          if (send_gm(pSMs, pgmParams->pEW) < 0)
            return -1;
          iNumChannelsProcessed++;
        }
      NextPscnl:
        pscnl = pscnl->next;
      }
    }
  NextServer:
    /* First clear out any tainted return codes.  Codes bad enough to
       cause the function to fail, should have caused processing to 
       goto CleanUp, so reset the code to clean/success here. DK 08/01/01 */
    rc = 0;
    this = this->next;
  }

  if(!iNumChannelsProcessed)
    logit("e","getGMFromWS():  WARNING!  No channels successfully processed "
              "for Event %s\n",
          pEvt->eventId);

  /* Cleanup any loose ends */  
 CleanUp:
  wsKillMenu( &queue );
  if (rc > 0) rc = 0;
  return rc;
}


/*
 * fillTraceReq: fill in the trace-req structure in preparation for
 *   obtaining traces from the wave_servers.
 */
static void fillTraceReq(STA *pSta, COMP3 *pComp, GMPARAMS *pgmParams, 
                         EVENT *pEvt, TRACE_REQ *pReq)
{

  strcpy(pReq->sta, pSta->sta);
  strcpy(pReq->net, pSta->net);
  strcpy(pReq->chan, pComp->name);
  strcpy(pReq->loc, pComp->loc);
  pReq->pinno = 0;
  if ( pEvt->eventId[0]!=0 && pEvt->eventId[0]!='-' ) {
	  pReq->reqStarttime = traceStartTime(pSta, pgmParams);
	  pReq->reqEndtime = traceEndTime(pSta, pgmParams);
  } else {
	  pReq->reqStarttime = pEvt->origin_time;
	  if ( pEvt->eventId[0]==0 ) {
	     logit("it","Using alarm  duration: %d\n", pgmParams->alarmDuration );
    	  pReq->reqEndtime = pReq->reqStarttime + pgmParams->alarmDuration;
      } else {
	     logit("it","Using thresh duration: %d\n", pgmParams->threshDuration);
          pReq->reqStarttime -= pgmParams->preTriggerSeconds;
          pSta->p_est = pEvt->origin_time - pgmParams->preTriggerSeconds*0.5;
          pSta->s_est = pEvt->origin_time;
    	  pReq->reqEndtime = pEvt->origin_time + pgmParams->threshDuration;
      }
  }  	
  pReq->partial = 1;  /* Not implemented in ws_clientII */
  pReq->pBuf = gtbBuf;
  pReq->bufLen = gtbBufLen;
  pReq->timeout = pgmParams->wsTimeout / 1000;
  pReq->fill = 0;     /* Not used for binary requests */

  return;
}

  
/*
 * getTraceFromWS: get tracedata from a wave_server as specified in req;
 *         Convert the data to local byte order and then copy into 
 *         the DATABUF pTrace.
 *   Returns: 0 on success
 *           +1 if no data available for this request
 *           -1 if fatal error occurred
 */
static int getTraceFromWS(TRACE_REQ *req, DATABUF *pTrace, int wsTimeout)
{
  GAP *pGap = NULL, *newGap;
  double traceEnd, samprate;
  int rc;
  TRACE2_HEADER *pTH;
  int isamp, nsamp;
  int32_t *longPtr;
  short *shortPtr;
  
  /* Initialize the trace buffer */
  cleanTrace(pTrace);

  /* get this trace; rummage through the servers we've been told about */
  rc = wsGetTraceBinL( req, &queue, wsTimeout ); 
  /* Handle possible error conditions. It's not clear what the caller *
   * should do about some of these: try this call again, try another  *
   * server, try getting a new menu? Ugh!                             */
  switch (rc) 
  {
  case WS_ERR_NONE:
    if (Debug) 
      logit("et", "getTraceFromWS: trace <%s.%s.%s.%s>: went ok. Got %ld bytes\n",
            req->sta, req->chan, req->net, req->loc, req->actLen); 
    break;
  case WS_WRN_FLAGGED:
    logit("et", "getTraceFromWS: server has no data for <%s.%s.%s.%s>\n", 
          req->sta, req->chan, req->net, req->loc);
    return +1;
  case WS_ERR_EMPTY_MENU:
    /* something is seriously wrong; give up */
    logit("et", "getTraceFromWS: wsGetTraceBinL reports no menues\n");
    return -1;
  case  WS_ERR_SCNL_NOT_IN_MENU:
    /* Might happen if we have to renew menu after making EVENT.Sta list */
    logit("et",
          "getTraceFromWS: wsGetTraceBinL reports <%s.%s.%s.%s> not in menu\n",
          req->sta, req->chan, req->net, req->loc);
    return +1;
  case WS_ERR_NO_CONNECTION:
    /* socket to server was marked as dead, so skip it and move on */
    return +1;
  case WS_ERR_BUFFER_OVERFLOW:
    logit("et"," getTraceFromWS: trace %s.%s.%s.%s truncated to fit buffer\n",
          req->sta, req->chan, req->net, req->loc); 
    /* There might still be useful data in the buffer, so proceed */
    break;
    
    /* For the following error conditions, the connection has failed. */
  case WS_ERR_BROKEN_CONNECTION:
    logit("e", "getTraceFromWS: broken connection while getting %s.%s.%s.%s\n",
          req->sta, req->chan, req->net, req->loc);
    return +1;
  case WS_ERR_TIMEOUT:
    logit("e", "getTraceFromWS: timeout while getting %s.%s.%s.%s\n",
          req->sta, req->chan, req->net, req->loc);
    return +1;
  default:
    logit("e", "getTraceFromWS: wsGetTraceBinL returned unknown error %s\n",
          rc);
    return -1;
  }
      
  /* Transfer trace data from TRACE_BUF packets into Trace buffer */
  traceEnd = (req->actEndtime < req->reqEndtime) ? req->actEndtime :
    req->reqEndtime;               /* This is the latest data we want */
  pTH = (TRACE2_HEADER *)req->pBuf;
  /*
   * Swap to local byte-order. Note that we will be calling this function
   * twice for the first TRACE_BUF packet; this is OK as the second call
   * will have no effect.
   */
  if (WaveMsg2MakeLocal(pTH) < 0)
  {
    logit("et", "getTraceFromWS: %s.%s.%s.%s unknown datatype <%s>; skipping\n",
          pTH->sta, pTH->chan, pTH->net, pTH->loc, pTH->datatype);
    return +1;
  }

  if (pTH->samprate < 0.1)
  {
    logit("et", "getTraceFromWS: %s.%s.%s.%s has zero samplerate (%g); skipping trace\n",
          pTH->sta, pTH->chan, pTH->net, pTH->loc, pTH->samprate);
    return +1;
  }
  pTrace->delta = 1.0/pTH->samprate;
  /* Save rate of first packet to compare with later packets */
  samprate = pTH->samprate;   
  pTrace->starttime = req->reqStarttime;
  /* Set Trace endtime so it can be used to test for gap at start of data */
  pTrace->endtime = ( (pTH->starttime < req->reqStarttime) ?
    pTH->starttime : req->reqStarttime) - pTrace->delta ;

  /* Look at all the retrieved TRACE_BUF packets */
  while (( pTH < (TRACE2_HEADER *)(req->pBuf + req->actLen)) &&
								(pTrace->nRaw < pTrace->lenRaw))
  {
    /* Swap bytes to local order */
    if (WaveMsg2MakeLocal(pTH) < 0)
    {
      logit("et", "getTraceFromWS: %s.%s.%s unknown datatype <%s>; skipping\n",
            pTH->sta, pTH->chan, pTH->net, pTH->datatype);
      return +1;
    }
    
    if ( fabs(pTH->samprate - samprate) > SR_TOL)
    {
      logit("et", "getTraceFromWS: <%s.%s.%s.%s samplerate change: %f - %f; discarding trace\n",
            pTH->sta, pTH->chan, pTH->net, pTH->loc, samprate, pTH->samprate);
      return +1;
    }
    
    /* Check for gap */
    if (pTrace->endtime + 1.5 * pTrace->delta < pTH->starttime)
    {
      if ( (newGap = (GAP *)calloc(1, sizeof(GAP))) == (GAP *)NULL)
      {
        logit("et", "getTraceFromWS: out of memory for GAP struct\n");
        return -1;
      }
      newGap->starttime = pTrace->endtime + pTrace->delta;
      newGap->gapLen = pTH->starttime - newGap->starttime;
      newGap->firstSamp = pTrace->nRaw;
      newGap->lastSamp = pTrace->nRaw + 
        (long)( (newGap->gapLen * samprate) - 0.5);
      /* Put GAP struct on list, earliest gap first */
      if (pTrace->gapList == (GAP *)NULL)
        pTrace->gapList = newGap;
      else
        pGap->next = newGap;
      pGap = newGap;  /* leave pGap pointing at the last GAP on the list */
      pTrace->nGaps++;
      
      /* Advance the Trace pointers past the gap; maybe gap will get filled */
      pTrace->nRaw = newGap->lastSamp + 1;
      pTrace->endtime += newGap->gapLen;
    }
    
    if (pTrace->starttime > pTH->starttime)
      isamp = (int)( 0.5 + (pTrace->starttime - pTH->starttime) * samprate);
    else
      isamp = 0;

    if (req->reqEndtime < pTH->endtime)
    {
      if (pTH->starttime < req->reqEndtime)
      {
          nsamp = pTH->nsamp - (int)( 0.5 * (pTH->endtime - req->reqEndtime) *
                                       samprate);
          pTrace->endtime = req->reqEndtime;
      }
      else
        nsamp = 0;
    }
    else
    {
      nsamp = pTH->nsamp;
      pTrace->endtime = pTH->endtime;
    }

    /* Assume trace data is integer valued here, long or short */    
    if (pTH->datatype[1] == '4')
    {
      longPtr=(int32_t*) ((char*)pTH + sizeof(TRACE2_HEADER) + isamp * 4);
/*
      for ( ;isamp < nsamp; isamp++)
*/
      for ( ;((isamp < nsamp) && (pTrace->nRaw < pTrace->lenRaw)); isamp++)
      {
        pTrace->rawData[pTrace->nRaw] = (double) *longPtr;
        longPtr++;
        pTrace->nRaw++;
      }
      /* Advance pTH to the next TRACE_BUF message */
      pTH = (TRACE2_HEADER *)((char *)pTH + sizeof(TRACE2_HEADER) + 
                             pTH->nsamp * 4);
    }
    else   /* pTH->datatype[1] == 2, we assume */
    {
      shortPtr=(short*) ((char*)pTH + sizeof(TRACE2_HEADER) + isamp * 2);
/*
      for ( ;isamp < nsamp; isamp++)
*/
      for ( ;((isamp < nsamp) && (pTrace->nRaw < pTrace->lenRaw)); isamp++)
      {
        pTrace->rawData[pTrace->nRaw] = (double) *shortPtr;
        shortPtr++;
        pTrace->nRaw++;
      }
      /* Advance pTH to the next TRACE_BUF packets */
      pTH = (TRACE2_HEADER *)((char *)pTH + sizeof(TRACE2_HEADER) + 
                             pTH->nsamp * 2);
    }
  }  /* End of loop over TRACE_BUF packets */
  
  return 0;
}

/*
 * initWsBuf: allocate the gtbBuf array; to be called at program startup
 *            reqLen is the number of trace data samples expected.
 *            864 = 200 samps/sec * 4 bytes/sample + 64 bytes header for
 *            a TRACEBUF message.
 *   Returns: 0 on success
 *           -1 on memory error
 */
int initWsBuf( long reqLen )
{
  /* Figure out how many TRACE_BUF packets we might need: round up, then
   * add 2 for overlap at begin and end of wave_server reply. */
  gtbBufLen = (reqLen + 199)/200 + 2;
  gtbBufLen *= 864;
  
  if ( (gtbBuf = (char *)malloc( gtbBufLen )) == (char *)NULL)
  {
    gtbBufLen = 0;
    return -1;
  }

  return 0;
}

/******************************************************************
 * Parse server address and port number in format "address:port"; *
 * or "address port" add them to the server list.                 *
 * Returns: 0 on success                                          *
 *         -1 on error (out of memory, bad address or port        *
 ******************************************************************/
int Add2ServerList( char * addr_port, GMPARAMS *pgmParams )
{
  char *c, *addr;
  char sep[] = " \t:";    /* space, tab and colon */
  int len;
  struct hostent  *host;
  struct in_addr addr_s;
  SERVER *new;
  extern int SOCKET_SYS_INIT;
  
  /* Initialize the socket system (for NT) if this is the first time through */
  if (SOCKET_SYS_INIT == 0)
    SocketSysInit();
  
  if (addr_port == NULL || (len = (int)strlen(addr_port)) == 0)
  {
    logit("et", "Add2ServerList: empty address/port string\n");
    return -1;
  }
  
  if ( (c = strpbrk(addr_port, sep)) == NULL)
  {
    logit("et", 
          "Add2ServerList: address and port must be separated by `:' or space\n");
    return -1;
  }
  *c = '\0';            /* terminate the address */
  c++;                  /* advance one */
  len = (int)strspn(c, sep); /* check for more separators */
  c += len;             /* advance past them */
  
  /* Assume we have an IP address and try to convert to network format.
   * If that fails, assume a domain name and look up its IP address.
   * Can't trust reverse name lookup, since some place may not have their
   * name server properly configured. 
   */
  memset((void *) &addr_s, 0, sizeof(addr_s));

  addr = addr_port;
  if ( inet_addr(addr_port) == INADDR_NONE )
  {        /* it's not a dotted quad address */
    if ( (host = gethostbyname(addr_port)) == NULL)
    {
      logit("e", "Add2ServerList: bad server address <%s>\n", addr_port );
      return -1;
    }
    memcpy((void *) &addr_s, host->h_addr, host->h_length);
    addr = inet_ntoa(addr_s);
  }
  
  if (strlen(addr) > 15 || strlen(c) > 5) 
  {
    logit("et", "Add2ServerList: ignoring dubious server <%s:%s>\n", addr, c);
    return 0;
  }
  if (pgmParams->pWSV == (WS_ACCESS *)NULL)
  {
    if ( (pgmParams->pWSV = 
          (WS_ACCESS *)calloc(1,sizeof(WS_ACCESS))) == NULL)
    {
      logit("et", "Add2ServerList: out of memory for WS_ACCESS\n");
      return -1;
    }
  }
  if ( (new = (SERVER *)calloc(1,sizeof(SERVER))) == NULL)
  {
    logit("et", "Add2ServerList: out of memory for SERVER\n");
    return -1;
  }
  
  strcpy( new->IPAddr, addr);
  strcpy( new->port, c);
  new->next = pgmParams->pWSV->pList;
  pgmParams->pWSV->pList = new;

  return 0;
}

/*
 * readServerFile: read tthe wave_server list file using kom routines,
 *    add the contents to the linked list of wave_servers .
 * Returns: 0 on success
 *         -1 on error
 */
int readServerFile( GMPARAMS *pgmParams )
{
  int nfiles;
  char *str;
  char server[PATH_MAX];

  if (pgmParams->pWSV->serverFile == NULL || 
      strlen(pgmParams->pWSV->serverFile) == 0)
  {
    logit("et", "readServerFile: missing serverFile name\n");
    return -1;
  }

  nfiles = k_open( pgmParams->pWSV->serverFile );
  if (nfiles == 0) 
  {
    logit("et", "readServerFile: unable to open server file <%s>.\n", 
          pgmParams->pWSV->serverFile);
    return -1;
  }
  
  while( k_rd())  /* Read next line from file */
  {
    char *str2;
    
    str = k_str();  /* Get the first token from Kom */
    /* Ignore blank lines and comments */
    if ( !str ) continue;
    if ( str[0] == '#' ) continue;
    str2 = k_str(); /* see if the Port number is the next token */
    if (str2 && str2[0] != '#')
      sprintf(server, "%s:%s", str, str2);
    else
      strcpy(server, str);  /* first token may be IP:PORT */
    if ( Add2ServerList(server, pgmParams) < 0)
      break;
    
    if (k_err() )
    {
      logit("et", "readServerFile: error parsing server file\n");
      return -1;
    }
  }
  nfiles = k_close();
  return 0;
}


