/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: liss2ew.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.7  2003/06/18 20:03:30  patton
 *     Added warning message, see below.
 *
 *     Revision 1.6  2003/06/17 19:25:03  patton
 *     *** empty log message ***
 *
 *
 *     Revision 1.5  2003/06/16 22:07:35  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.4  2000/03/13 23:50:24  lombard
 *     Removed some DCC code that had a memory leak (Decode_SEED())
 *     Added initialization of decompression struct to Configure()
 *     Initialized Param.debug.
 *     Added soem error checking for Steim decompression.
 *
 *     Revision 1.3  2000/03/10 18:40:50  lombard
 *     moved memset of SCNL; best not to memset things after you have read
 *     into them!
 *
 *     Revision 1.2  2000/03/09 20:13:03  lombard
 *     Fixed  initialized memory in pscnB.
 *     Added check for `keepalive' record from LISS.
 *
 *     Revision 1.1  2000/03/05 21:45:50  lombard
 *     Initial revision
 *
 *
 *
 */

/****************************************************************************
 * WARNING: liss2ew has been observed intermittantly producing malformed    *
 * TRACE_BUF messages.  Currently the conditions for causing this problem   *
 * are unknown.  Due to this, liss2ew should be treated as suspect.  Use at *
 * your own risk.  JMP 6-18-2003                                            *
 ***************************************************************************/ 


/****************************************************************************
 * LISS2ew module: reads miniSEED from one LISS server and writes TRACE_BUF *
 * messages to earthworm transport ring. Pete Lombard, 1/31/2000            *
 *                                                                          *
 * WARNING: Much of this code is based on dumpseed from the DCC at ASL.     *
 * Their byte-swapping conventions are confusing at best! I make no         *
 * apologies for their code, either. PNL                                    *
 ****************************************************************************/

/* System includes */
#include <stdio.h>

/* Earthworm includes */
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <socket_ew.h>
#include "liss2ew.h"

int main( int argc, char **argv)
{
  WORLD     L2e;                   /* My global structure             */
  STATUS state, reported;          /* current and reported state      */
  SOCKET sock = 0;                 /* Socket to the LISS              */
  int   conn_retry = 0;            /* Counter for connection attempts */
  int   read_retry = 0;            /* Counter for read() attempts     */
  int   nread, err;
  /*char  msgText[MAXMESSAGELEN];     string for log/error messages   */
  struct sockaddr_in sin;
  time_t  timeNow;                   /* current time                  */ 
  time_t  timeLastBeat;              /* time last heartbeat was sent  */
  
  /* Check command line arguments 
         ******************************/
  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s <configfile>\n", argv[0]);
    exit (EW_FAILURE);
  }

  /* Configure me! */
  Configure(&L2e, argv);

  /* Initialize our state indicators */
  state = closed;
  reported = closed;
  
  /* Initialize the socket system */
  SocketSysInit();
  if ( SetAddress( &sin, L2e ) )
  {
    logit("e", "%s: error setting LISS address. Exiting\n", argv[0]);
    tport_detach(&(L2e.regionOut));
    exit( EW_FAILURE );
  }
    
  /*    Force a heartbeat to be issued in first pass thru main loop     */
  timeLastBeat = time( &timeNow ) - L2e.Param.heartbeatInt - 1;

  /*    Main message processing loop                                    */
  while ( tport_getflag( &(L2e.regionOut) ) != TERMINATE )
  {
    /*  Check for need to send heartbeat message                        */
    if ( time( &timeNow ) - timeLastBeat >= L2e.Param.heartbeatInt )
    {
      timeLastBeat = timeNow;
      StatusReport( &L2e, L2e.Ewh.typeHeartbeat, 0, "" ); 
    }

    /* Figure out what to do next based on our current state */
    switch (state)
    {
    case closed:   /* Open a nonblocking socket for the LISS connection */
      if ( ( sock = socket_ew( AF_INET, SOCK_STREAM, 0)) == -1 )
      {
        logit("et", "%s: error opening socket. Exiting\n", argv[0]);
        tport_detach(&(L2e.regionOut));
        exit( EW_FAILURE );
      }
      if ( connect_ew( sock, (struct sockaddr*) &sin, sizeof(sin), 
                       L2e.sockTimeout ) == -1 )
      {
        if ( (err = socketGetError_ew() ) != CONNECT_WOULDBLOCK_EW)
        {
          logit("et", "%s: Error connecting to LISS: %s; exiting\n", argv[0],
                strerror(err));
          tport_detach(&(L2e.regionOut));
          exit( EW_FAILURE );
        }
        
        if (L2e.Param.debug)
          logit("et", "no connection\n");
        
        conn_retry++;
        state = closed;   /* connect_ew closed the socket */
        if (conn_retry > RETRIES_EXIT)
        {
          logit("et", "%s: Still no connection after %d tries, exiting\n",
                argv[0], conn_retry);
          tport_detach(&(L2e.regionOut));
          exit( EW_FAILURE );
        }

        if (conn_retry > RETRIES_LOG)
        {
          if (reported == conn_fail1)
          {
            logit("et", "%s: no connection after %d tries; still trying.\n",
                  argv[0], conn_retry);
            reported = conn_fail2;
          }
        }
        else
        {
          reported = conn_fail1;
        }
      } 
      else
      {
        conn_retry = 0;
        state = connected;
        logit("et", "%s: connected to LISS\n", argv[0]);
        reported = connected;
      }
      break;

    case connected:  /* Got a connection, now try to use it */
      if ( (nread = recv_all ( sock, L2e.inBuf, L2e.lenSEED, 0, 
                               L2e.sockTimeout) )
           < L2e.lenSEED )
      {
        err = socketGetError_ew();
        read_retry++;
        if (err == WOULDBLOCK_EW)
        {
          if (read_retry == 1)
          {
            logit("et", "%s: timed out reading from LISS; trying again.\n",
                  argv[0]);
          }
          else if (read_retry == RETRIES_RECONN)
          {
            logit("et", "%s: nothing read in %d minutes; reopening socket\n",
                  argv[0], read_retry * L2e.sockTimeout / 60000);
            (void) closesocket_ew(sock, SOCKET_CLOSE_IMMEDIATELY_EW);
            reported = state = closed;
          }
          break;
        }
        else
        {
          logit("et", "%s: fatal error occured while reading from LISS: %s\n",
                argv[0], strerror(err));
          (void) closesocket_ew(sock, SOCKET_CLOSE_IMMEDIATELY_EW);
          tport_detach(&(L2e.regionOut));
          exit( EW_FAILURE );
        }
      }
      /* We finally read something! */
      read_retry = 0;
      if (ProcessData(&L2e) != EW_SUCCESS)
      {
        logit("et", "%s: fatal error processing LISS data.\n", argv[0] );
        (void) closesocket_ew(sock, SOCKET_CLOSE_IMMEDIATELY_EW);
        tport_detach(&(L2e.regionOut));
        exit( EW_FAILURE );
      }
      if (reported != receiving)
      {
        logit("et", "%s: processing LISS data\n", argv[0]);
        reported = receiving;
      }
      break;
    default:
      logit("et", "%s: unknown state value: %d; exitting\n", argv[0], state);
      if (sock != 0)
        (void) closesocket_ew(sock, SOCKET_CLOSE_IMMEDIATELY_EW);
      tport_detach(&(L2e.regionOut));
      exit( EW_FAILURE );
    }  /* End of switch(state) */
  }  /* End of main loop; time to close up shop. */

  (void) closesocket_ew(sock, SOCKET_CLOSE_GRACEFULLY_EW);
  tport_detach(&(L2e.regionOut));
  logit("t","%s terminated\n", argv[0]);
  exit( 0 );
}

/* SetAddress: given a LISS domain name or IP address and port number,
   set up the socket address structure. Returns 0 on success, -1 on failure */
int SetAddress( struct sockaddr_in *sinP, WORLD L2e)
{
  unsigned long addr;
  struct hostent* hp;
  /*int port;*/
  
  memset((void*)sinP, 0, sizeof(struct sockaddr_in));
  sinP->sin_family = AF_INET;
  sinP->sin_port = (u_short)htons((u_short)L2e.LISSport);

  /* Assume we have an IP address and try to convert to network format.
   * If that fails, assume a domain name and look up its IP address.
   * Can't trust reverse name lookup, since some place may not have their
   * name server properly configured. 
   */
  if ( (addr = inet_addr(L2e.LISSaddr)) != INADDR_NONE )
  {
    sinP->sin_addr.s_addr = addr;
  }
  else
  {       /* it's not a dotted quad IP address */
    if ( (hp = gethostbyname(L2e.LISSaddr)) == NULL) 
    {
      logit("e", "bad server address <%s>\n", L2e.LISSaddr );
      return( -1 );
    }
    memcpy((void *) &(sinP->sin_addr), (void*)hp->h_addr, hp->h_length);
  }
  return( 0 );
  
}

  
/* StatusReport: Send error and hearbeat messages to transport ring     */
void StatusReport( WORLD* pL2e, unsigned char type, short code, 
                   char* message )
{
  char          outMsg[MAXMESSAGELEN];  /* The outgoing message.        */
  time_t        msgTime;        /* Time of the message.                 */

  /*  Get the time of the message                                       */
  time( &msgTime );
  
  /*  Build & process the message based on the type                     */
  if ( pL2e->Ewh.typeHeartbeat == type )
  {
    sprintf( outMsg, "%ld %ld\n", (long) msgTime, (long) pL2e->MyPid );
    
    /*Write the message to the output region                            */
    if ( tport_putmsg( &(pL2e->regionOut), &(pL2e->hrtLogo), 
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                       */
      logit( "et", "liss2ew: Failed to send a heartbeat message (%d).\n",
             code );
    }
  }
  else
  {
    if ( message ) 
    {
      sprintf( outMsg, "%ld %hd %s\n", (long) msgTime, code, message );
      logit("t","Error:%d (%s)\n", code, message );
    }
    else 
    {
      sprintf( outMsg, "%ld %hd\n", (long) msgTime, code );
      logit("t","Error:%d (No description)\n", code );
    }
    
    /*Write the message to the output region                         */
    if ( tport_putmsg( &(pL2e->regionOut), &(pL2e->errLogo), 
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                    */
      logit( "et", "liss2ew: Failed to send an error message (%d).\n",
             code );
    }
    
  }
}


/* Jumbo configuration routine: read config file, look up EW parameters.
 * initialize a bunch of values, start up logit and attach to the transport
 * ring. Exits on failure, returns nothing. */
void Configure( WORLD *pL2e, char **argv)
{
  int i;
  
  /* Some important initial values or defaults */
  pL2e->lenSEED = DEF_LEN_SEED;
  pL2e->LISSport = DEF_LISS_PORT;
  pL2e->sockTimeout = 0;
  pL2e->Nscn = 0;
  pL2e->inBuf = (char *)0;
  pL2e->outBuf = (long*)0;
  pL2e->outBufLen = 0;
  pL2e->Param.debug = 0;
  
  /* Read config file and configure liss2ew */
  if (ReadConfig(pL2e, argv[1]) == EW_FAILURE)
  {
    fprintf (stderr, "%s: configure() failed \n", argv[0]);
    exit (EW_FAILURE);
  }

  /*    Look up important info from earthworm tables                  */
  if ( ReadEWH( pL2e ) == EW_FAILURE )
  {
    fprintf( stderr, "%s error in ReadEWH.\n", argv[0] );
    exit( EW_FAILURE );
  }

  /* Set up logos for outgoing messages */
  pL2e->hrtLogo.instid = pL2e->Ewh.myInstId;
  pL2e->hrtLogo.mod    = pL2e->Ewh.myModId;
  pL2e->hrtLogo.type   = pL2e->Ewh.typeHeartbeat;

  pL2e->errLogo.instid = pL2e->Ewh.myInstId;
  pL2e->errLogo.mod    = pL2e->Ewh.myModId;
  pL2e->errLogo.type   = pL2e->Ewh.typeError;

  pL2e->waveLogo.instid = pL2e->Ewh.myInstId;
  pL2e->waveLogo.mod    = pL2e->Ewh.myModId;
  pL2e->waveLogo.type   = pL2e->Ewh.typeWaveform;

  /* Get my process ID so I can let statmgr restart me */
  pL2e->MyPid = getpid();
  
  /* Initialize name of log-file & open it */
  logit_init( argv[1], pL2e->Ewh.myModId, MAXMESSAGELEN, pL2e->Param.logSwitch );
  logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );

  /* Allocate memory for our input buffer */
  if ( (pL2e->inBuf = (char *)malloc(pL2e->lenSEED)) == (char *)NULL)
  {
    logit("e", "%s: error allocating input buffer.\n", argv[0]);
    exit( EW_FAILURE );
  }

  /* Initialize SCN_BUFFER structures */
  for (i = 0; i < pL2e->Nscn; i++)
  {
    pL2e->pscnB[i].quality[0] = pL2e->pscnB[i].quality[1] = 0;
    pL2e->pscnB[i].starttime = 0.0;
    pL2e->pscnB[i].endtime = 0.0;
    pL2e->pscnB[i].samplerate = -1.0;
    pL2e->pscnB[i].writeP = 0;
    if ( (pL2e->pscnB[i].buf = (long *)malloc(pL2e->traceLen * sizeof(long)))
         == (long *)NULL)
    {
      fprintf(stderr, "Error allocating memory for trace buffer; exitting\n");
      exit( EW_FAILURE );
    }
  }
  
  if ( (pL2e->dcp = init_generic_decompression ()) == NULL)
  {
    fprintf(stderr, "Error allocating memory for decompression structure; exitting\n");
    exit( EW_FAILURE );
  }

  /* Attach to Output transport ring */
  tport_attach( &(pL2e->regionOut), pL2e->Ewh.ringOutKey);
  
  return;
}

 
/*  Read some values from earthworm master ID file */
int ReadEWH( WORLD* pL2e )
{

  if ( GetLocalInst( &(pL2e->Ewh.myInstId)) != 0 )
  {
    fprintf(stderr, "liss2ew: Error getting myInstId.\n" );
    return EW_FAILURE;
  }
  
  if ( GetModId( pL2e->Param.myModName, &(pL2e->Ewh.myModId)) != 0 )
  {
    fprintf( stderr, "liss2ew: Error getting myModId.\n" );
    return EW_FAILURE;
  }

  if ((pL2e->Ewh.ringOutKey = GetKey (pL2e->Param.ringOut) ) == -1) 
  {
    fprintf (stderr,
             "liss2ew:  Invalid ring name <%s>; exitting!\n", 
             pL2e->Param.ringOut);
    return EW_FAILURE;
  }

  /* Look up message types of interest */
  if (GetType ("TYPE_HEARTBEAT", &(pL2e->Ewh.typeHeartbeat)) != 0) 
  {
    fprintf (stderr, 
             "liss2ew: Invalid message type <TYPE_HEARTBEAT>; exitting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_ERROR", &(pL2e->Ewh.typeError)) != 0) 
  {
    fprintf (stderr, 
             "liss2ew: Invalid message type <TYPE_ERROR>; exitting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_TRACEBUF", &(pL2e->Ewh.typeWaveform)) != 0) 
  {
    fprintf (stderr, 
             "liss2ew: Invalid message type <TYPE_TRACEBUF>; exitting!\n");
    return EW_FAILURE;
  }

  return EW_SUCCESS;

} 

#define NUMREQ          9       /* Number of parameters that MUST be    */
                                /*   set from the config file.          */
/*      Function: ReadConfig                                            */
int ReadConfig (WORLD* pL2e, char *configfile )
{
  char     		init[NUMREQ];     
  /* init flags, one byte for each required command */
  int      		nmiss;
  /* number of required commands that were missed   */
  char    		*com;
  char    		*str;
  int      		nfiles;
  int      		success;
  int      		i;
  int                   Iscn = 0;

  /* Set to zero one init flag for each required command */
  for (i = 0; i < NUMREQ; i++)
    init[i] = 0;

  /* Open the main configuration file 
   **********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    fprintf (stderr,
             "liss2ew: Error opening command file <%s>; exitting!\n", 
             configfile);
    return EW_FAILURE;
  }

  /* Process all command files
   ***************************/
  while (nfiles > 0)   /* While there are command files open */
  {
    while (k_rd ())        /* Read next line from active file  */
    {  
      com = k_str ();         /* Get the first token from line */

      /* Ignore blank lines & comments
       *******************************/
      if (!com)
        continue;
      if (com[0] == '#')
        continue;

      /* Open a nested configuration file */
      if (com[0] == '@') 
      {
        success = nfiles + 1;
        nfiles  = k_open (&com[1]);
        if (nfiles != success) 
        {
          fprintf (stderr, 
                   "liss2ew: Error opening command file <%s>; exitting!\n", 
                   &com[1]);
          return EW_FAILURE;
        }
        continue;
      }

      /* Process anything else as a command */
      /*0*/ 	if (k_its ("MyModId")) 
      {
        if ( (str = k_str ()) != NULL )
        {
          if (strlen(str) >= MAXMODNAMELEN)
          {
            fprintf(stderr, "MyModId too long; max is %d\n", MAXMODNAMELEN -1);
            return EW_FAILURE;
          }
          strcpy (pL2e->Param.myModName, str);
          init[0] = 1;
        }
      }
      /*1*/	else if (k_its ("OutRing")) 
      {
        if ( (str = k_str ()) != NULL )
        {
          if (strlen(str) >= MAXRINGNAMELEN)
          {
            fprintf(stderr, "OutRing name too long; max is %d\n", 
                    MAXRINGNAMELEN - 1);
            return EW_FAILURE;
          }
          strcpy (pL2e->Param.ringOut, str);
          init[1] = 1;
        }
      }
      /*2*/	else if (k_its ("HeartBeatInterval")) 
      {
        pL2e->Param.heartbeatInt = k_long ();
        init[2] = 1;
      }

      /*3*/     else if (k_its ("LogFile"))
      {
        pL2e->Param.logSwitch = k_int();
        init[3] = 1;
      }

      /* 4 */else if (k_its ("LISSaddr")) 
      {
        if ( (str = k_str ()) != NULL )
        {
          if (strlen(str) >= MAXADDRLEN)
          {
            fprintf(stderr, "LISSaddr too long; max is %d\n", MAXADDRLEN);
            return EW_FAILURE;
          }
          strcpy (pL2e->LISSaddr, str);
          init[4] = 1;
        }
      }

      /* 5 */ else if ( k_its ( "LISSport"))
      {
        pL2e->LISSport = k_int();
        if (pL2e->LISSport < 1)
        {
          fprintf(stderr, "LISSport is 0; this won't work.\n");
          return EW_FAILURE;
        }
        init[5] = 1;
      }
      
      /* 6 */ else if ( k_its ( "TraceLength"))
      {
        pL2e->traceLen = k_int();
        if (pL2e->traceLen < 10 && 
            pL2e->traceLen > (MAX_TRACEBUF_SIZ - sizeof(TRACE_HEADER))/sizeof(long))
        {
          fprintf(stderr, "TraceLength %d is outside of range (%d - %d.\n",
                  pL2e->traceLen, 10, (int)((MAX_TRACEBUF_SIZ
                                      - sizeof(TRACE_HEADER))/sizeof(long)));
          return EW_FAILURE;
        }
        init[6] = 1;
      }
      
      /* 7 */    else if (k_its ("MaxSCNs"))
      {
        pL2e->Nscn = k_int();
        if (pL2e->Nscn <= 0 || pL2e->Nscn > 80)
        {
          fprintf(stderr, "Unreasonable value for MaxSCNs: %d\n", pL2e->Nscn);
          return EW_FAILURE;
        }
        if ( (pL2e->pscnB = (SCN_BUFFER *)
              malloc(pL2e->Nscn* sizeof(SCN_BUFFER))) == (SCN_BUFFER *) NULL)
        {
          fprintf(stderr, "Error allocating SCN buffers; exiting\n");
          exit( -1 );
        }
        /* Make sure the SCNL strings are clean before we use them; *
         * the other scnB fields get initialized in Configure().    */
        for (i = 0; i < pL2e->Nscn; i++)
        {
          memset(pL2e->pscnB[i].sta, 0, TRACE_STA_LEN);
          memset(pL2e->pscnB[i].chan, 0, TRACE_CHAN_LEN);
          memset(pL2e->pscnB[i].net, 0, TRACE_NET_LEN);
          memset(pL2e->pscnB[i].lc, 0, LC_LEN);
        }
        init[7] = 1;
      }
          
      /*8*/   else if( k_its("AcceptSCNL") ) 
      {
        if (pL2e->Nscn == 0)
        {
          fprintf(stderr, "MaxSCNs command must come before AcceptSCNL commands\n");
          exit( -1 );
        }
        if( Iscn >= pL2e->Nscn )
        {
          fprintf( stderr,
                   "liss2ew: Too many <AcceptSCNL> commands in <%s>;\n",
                   configfile );
          fprintf( stderr, 
                   "               limited by command-line-arg <-# %d>; exiting!\n",
                   pL2e->Nscn );
          exit( -1 );
        }
        str = k_str();   /* read station code */
        if (str != (char *)NULL)
        {
          if( strlen(str) < TRACE_STA_LEN ) {
            strcpy( pL2e->pscnB[Iscn].sta, str );
          } else {
            fprintf( stderr,"liss2ew: error in <AcceptSCNL> command in <%s>:\n",
                     configfile );
            fprintf( stderr,"               sta code <%s> too long; maxchar=%d; exiting!\n",
                     str, TRACE_STA_LEN-1 );
            exit( -1 );
          };
          str = k_str();   /* read component code */
          if (str != (char *) NULL)
          {
            if( strlen(str) < TRACE_CHAN_LEN ) {
              strcpy( pL2e->pscnB[Iscn].chan, str );
            } else {
              fprintf( stderr,"liss2ew: error in <AcceptSCNL> command in <%s>:\n",
                       configfile );
              fprintf( stderr,"               channel code <%s> too long; maxchar=%d; exiting!\n",
                       str, TRACE_CHAN_LEN-1 );
              exit( -1 );
            };
            str = k_str();  /* read network code */
            if (str != (char *)NULL)
            {
              if( strlen(str) < TRACE_NET_LEN ) {
                strcpy( pL2e->pscnB[Iscn].net, str );
              } else {
                fprintf( stderr,"liss2ew: error in <AcceptSCNL> command in <%s>:\n",
                         configfile );
                fprintf( stderr,"               network code <%s> too long; maxchar=%d; exiting!\n",
                         str, TRACE_NET_LEN-1 );
                exit( -1 );
              };
              str = k_str();  /* read location code */
              if (str != (char *)NULL)
              {
                if( strlen(str) < LC_LEN ) {
                  strcpy( pL2e->pscnB[Iscn].lc, str );
                } else {
                  fprintf( stderr,"liss2ew: error in <AcceptSCNL> command in <%s>:\n",
                           configfile );
                  fprintf( stderr,"               location code <%s> too long; maxchar=%d; exiting!\n",
                           str, LC_LEN-1 );
                  exit( -1 );
                };
                pL2e->pscnB[Iscn].pinno = k_int();   /* read pin number */
                if( (pL2e->pscnB[Iscn].pinno < 0)    ||
                    (pL2e->pscnB[Iscn].pinno > 32767)   )   /* Invalid pin number */
                {  
                  fprintf( stderr,"liss2ew: error in <AcceptSCNL> command in <%s>:\n",
                           configfile );
                  fprintf( stderr,"               pin number <%d> outside allowed range (0-32767); exiting!\n",
                           pL2e->pscnB[Iscn].pinno );
                  exit( -1 );
                }  
                Iscn++;
                init[8] = 1;
              }
            }
          }
        }
      }

      /* Optional: */ else if ( k_its ( "lenSEED"))
      {
        pL2e->lenSEED = k_int();
        if (pL2e->lenSEED < 1)
        {
          fprintf(stderr, "lenSEED is 0; this won't work.\n");
          return EW_FAILURE;
        }
      }
      
      /* Optional: */ else if ( k_its ( "SocketTimeout"))
      {
        pL2e->sockTimeout = k_int(); /* Convert to milliseconds below */
      }
      
      /* Optional: debug command */
      else if (k_its( "Debug") )
      {
        pL2e->Param.debug = 1;
      }
      
      /* Unknown command */ 
      else 
      {
        fprintf (stderr, "liss2ew: <%s> Unknown command in <%s>.\n", 
                 com, configfile);
        continue;
      }

      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        fprintf (stderr, 
                 "liss2ew: Bad command in <%s>; exitting!\n\t%s\n",
                 configfile, k_com());
        return EW_FAILURE;
      }

    } /** while k_rd() **/

    nfiles = k_close ();

  } /** while nfiles **/

  
  /* After all files are closed, check init flags for missed commands */
  nmiss = 0;
  for (i = 0; i < NUMREQ; i++)  
    if (!init[i]) 
      nmiss++;

  if (nmiss) 
  {
    fprintf (stderr, "liss2ew: ERROR, no ");
    if (!init[0])  fprintf (stderr, "<MyModId> "        );
    if (!init[1])  fprintf (stderr, "<OutRing> " );
    if (!init[2])  fprintf (stderr, "<HeartBeatInterval> "     );
    if (!init[3])  fprintf (stderr, "<LogFIle> "     );
    if (!init[4])  fprintf (stderr, "<LISSaddr> "     );
    if (!init[5])  fprintf (stderr, "<LISSport> "     );
    if (!init[6])  fprintf (stderr, "<TraceLength> "  );
    if (!init[7])  fprintf (stderr, "<MaxSCNs> "      );
    if (!init[8])  fprintf (stderr, "<AcceptSCNL> "    );
    
    fprintf (stderr, "command(s) in <%s>; exitting!\n", configfile);
    return EW_FAILURE;
  }

  if (pL2e->sockTimeout > pL2e->Param.heartbeatInt || pL2e->sockTimeout <= 0)
    pL2e->sockTimeout = pL2e->Param.heartbeatInt;
  
  pL2e->sockTimeout *= 1000; /* We use milliseconds internally */

  return EW_SUCCESS;
}



/* Process miniSEED data in buffer, fill TRACE_BUF message and send to 
   transport ring.
   Based on dumpseed from ASL DCC; modified for earthworm: PNL 2/2/2000 */

#define LW(a) LocGM68_WORD(a)
#define LL(a) LocGM68_LONG(a)
struct generic_blk {
  UDCC_WORD	blktype;
  UDCC_WORD	blkxref;
};

int ProcessData(WORLD *pL2e )
{
  static char tmpBuf[100];      /* Some scratch space                */
  SEED_DATA *seed_rec;          /* pointer to incoming miniSEED      */
  char n[10],s[10],l[10],c[10]; /* net, station, location, comp buffers */
  double starttime;             /* SEED start time with corrections  */
  double srate;                 /* sample rate                       */
  int a,b;
  struct generic_blk *blkhdr;   /* pointer to generic SEED blockette */
  int format = 0;               /* Format of data_only blockette     */
  int nsamps;                   /* Number of samples, from header    */
  int act_samps;                /* Number of samples decoded         */
  int Iscn;
  SCN_BUFFER *this;             /* Pointer to SCN buffer of interest */
  short level, dframes, firstframe, dstat;
  int rec_length = 0;
  
  seed_rec = (SEED_DATA*) pL2e->inBuf;

  /* Make sure we are looking at a Data packet */
  /* We might have lost sync with the miniSEED messages; if we die
     and restart, maybe we'll have better luck */
  if (seed_rec->Record_Type != 'D') 
  {
    /* Maybe a `keepalive' packet */
    if (seed_rec->Record_Type == ' ') 
    {
       if (pL2e->Param.debug)
	   logit("et", "liss2ew: received keepalive packet\n");
       return EW_SUCCESS;
    }
    
    logit("et", "liss2ew: non-data packet found: <%d>; exitting\n",
          seed_rec->Record_Type);
    return EW_FAILURE;
  }

  SH_Get_Idents(seed_rec,n,s,l,c);
  TrimString(n);
  Upcase(n);
  TrimString(s);
  Upcase(s);
  TrimString(l);
  Upcase(l);
  TrimString(c);
  Upcase(c);

  /* Get index of this S-C-N in pscnB */
   for( Iscn=0; Iscn <pL2e->Nscn; Iscn++ )   
   {  /* The logic is: break on a match; continue on no match */
      if( strcmp( pL2e->pscnB[Iscn].sta, s    ) != 0 ) continue;
      if( strcmp( pL2e->pscnB[Iscn].chan, c   ) != 0 ) continue;
      if( strcmp( pL2e->pscnB[Iscn].net,  n ) != 0 ) continue;
      if( pL2e->pscnB[Iscn].lc[0] == LC_WILD )
        break;   /* WILD matches any LC */
      else if( strcmp( pL2e->pscnB[Iscn].lc, l ) != 0) 
        continue;
      break;
   }
   if( Iscn == pL2e->Nscn ) /* this SCN is not in our AcceptSCNL list */
   {
     if (pL2e->Param.debug)
       logit("t", "Ignoring <%s %s %s %s>; not in AcceptSCNL list!\n", s, c, n, l );
     return EW_SUCCESS;
   }
   if (pL2e->Param.debug)
     logit("t", "Processing <%s %s %s %s> packet\n", s,c,n,l);
   
   this = &(pL2e->pscnB[Iscn]);
   
   starttime = ST_GetDblUnix( SH_Start_Time(seed_rec) );

  /* Check for time correction */
  if ((seed_rec->Activity_Flags & ACTFLAG_CLKFIX) == 0)
    starttime += 0.0001 * LL(seed_rec->Time_Correction);
  
  /* Sample rate */
  a = LW(seed_rec->Rate_Factor);
  b = LW(seed_rec->Rate_Mult);

  if (b == 0)
  {
    logit("et", "liss2ew: illegal rate multiplier 0; skipping packet\n");
    return EW_SUCCESS;
  }
  if (a == 0)
  {
    logit("et", "liss2ew: illegal rate factor 0; skipping packet\n");
    return EW_SUCCESS;
  }
  if ( a < 0) srate = -1.0 / (double) a;
  else srate = (double) a;
  if ( b < 0) srate /= -((double) b);
  else srate *= (double) b;

  /* Check for changed sample rate; could be caused by different location code */
  if ( this->samplerate > 0.0  && this->samplerate != srate )
    logit("et", "sample rate changed for <%s.%s.%s.%s>; current is %f; last is %f\n",
          s,c,n,l, srate, this->samplerate);
  this->samplerate = srate;
  
  nsamps = LW(seed_rec->Number_Samps);
  this->quality[0] = seed_rec->Qual_Flags; this->quality[1] = 0;
  
  /* Handle the individual miniSEED blockettes */
  a = LW(seed_rec->First_Blockette);  
  while(a > 0 && a < pL2e->lenSEED)
  {                       /* walk the list of blockettes */
    blkhdr = (struct generic_blk *) (pL2e->inBuf + a);
    b = LW(blkhdr->blktype);
    switch (b)
    {
    case 1000:	
      {
        struct Data_only *dat = (struct Data_only *) blkhdr;
        format = dat->Encoding;
        rec_length = (int) dat->Length;
        break;
      }
    case 1001:	
      {
        struct Data_ext *dat = (struct Data_ext *) blkhdr;
        /* Time shim in microseconds */
        starttime += 0.000001 * dat->Usec;
        break;
      }
    default:
      if (pL2e->Param.debug) logit("t", "skipping non-data blockette %d\n", b);
      break;
    }
    /* Point to the next blockette */
    a = LW(blkhdr->blkxref);
  }  /* End of while(a > 0...) */
  
  if (pL2e->outBufLen < nsamps)
  {
    if (pL2e->Param.debug)
      logit("t","ProcessData outBuf old %d new %d\n", pL2e->outBufLen, 
            nsamps + 100);
    pL2e->outBufLen = nsamps + 100;   /* So we don't keep asking for a little more */
    if (pL2e->outBuf) free(pL2e->outBuf);
    if ( (pL2e->outBuf = (long *)malloc(pL2e->outBufLen * sizeof(long))) 
         == (long *)NULL)
    {
      logit("et", "liss2ew: error alloacting memory for SEED output buffer.\n");
      return EW_FAILURE;
    }
  }
  
  /* And now for the trace data... */
  switch (format)
  {
  case FMT_STEIM_1:
    level = 1;
    break;
  case FMT_STEIM_2:
    level = 2;
    break;
  case FMT_STEIM_3:
    level = 3;
    break;
  default:
    logit("et", "Skipping unsupported data format %d\n", format);
    return (EW_SUCCESS);
  }
  
  switch (rec_length)
  {
  case 12 :
    dframes = 63 ;
    break ;
  case 11 :
    dframes = 31 ;
    break ;
  case 10 :
    dframes = 15 ;
    break ;
  case  9 :
    dframes = 7 ;
    break ;
  case 8 :
    dframes = 3 ;
    break ;
  case 7 :
    dframes = 1 ;
    break ;
  default :
    logit("et", "Skipping unsupported record length %d\n", rec_length);
    return EW_SUCCESS;
    break ;
  }
  
  firstframe = LW(seed_rec->Data_Start) / sizeof(compressed_frame) - 1;
  
  act_samps = decompress_generic_record((generic_data_record *)seed_rec,
                                        pL2e->outBuf, &dstat, pL2e->dcp, 
                                        firstframe, nsamps, level, FLIP, 
                                        dframes);
  /* non-fatal advisories: see steim/steim.h */
  if (pL2e->Param.debug)
  {
    if (dstat & EDF_INTEGRESYNC)
      logit("et","decompress frame: expansion error at integration constant check.\n") ;
    if (dstat & EDF_LASTERROR)
      logit("et","decompress record: last sample does not agree with decompression.\n") ;
    if (dstat & EDF_COUNTERROR)
      logit("et","decompress record: sample count disagreement.\n") ;
  }
  /* Fatal decompression errors: */
  if (dstat & EDF_SECSUBCODE)
    logit("et","decompress frame: illegal secondary subcode spanning two blocks.\n") ;
  if (dstat & EDF_TWOBLOCK)
    logit("et","decompress frame: illegal two-block code at end of frame.\n") ;
  if (dstat & EDF_OVERRUN)
    logit("et","decompress frame: unpacked buffer overrun.\n") ;
  if (dstat & EDF_REPLACEMENT)
    logit("et","decompress frame: illegal flag-word replacement subcode.\n") ;
  if (dstat & EDF_INTEGFAIL)
    logit("et","decompress frame: expansion failed. frame internally damaged.\n") ;
  if ((dstat & EDF_FATAL) != 0)
    return EW_SUCCESS;

  if (act_samps != nsamps)
  {
    logit("et", "liss2ew: sample mismatch: decoded (%d) advertised (%d)\n",
          act_samps, nsamps);
    nsamps = act_samps;
  }
  if (act_samps > pL2e->outBufLen)
  {
    logit("et", "liss2ew: SEED decoder overflowed output buffer; results undefined.\n");
    return EW_FAILURE;
  }
  
  return( DisposePacket(pL2e, this, nsamps, starttime ));
  
}


/*
 * Dispose of the trace data into the assigned SCN buffer, fill up TRACE_BUF
 * packets, and send them on their way. */
int DisposePacket( WORLD *pL2e, SCN_BUFFER *this, int nsamps, 
                   double starttime)
{
  int i;   /* Sample counter */
  int hunk;
  
  /* SCN buffer has data and there's a gap, send the old data immediately */
  if (this->writeP)
  {
    if (starttime > this->endtime + 1.5/this->samplerate)
    {
      logit("et", "liss2ew: gap detected in <%s.%s.%s>\n", this->sta,
            this->chan, this->net);
      SendTrace( pL2e, this );
    }
    else if (starttime < this->endtime)
    {
      logit("et", "liss2ew: overlap detected in <%s.%s.%s>; dumping old data\n", 
            this->sta, this->chan, this->net);
      this->writeP = 0;
    }
  }

  /* Transfer trace data from outBuf into the SCN buffer */
  i = 0;
  while (i < nsamps)
  {
    hunk = pL2e->traceLen - this->writeP;
    if (hunk > nsamps - i) hunk = nsamps - i;
    memcpy((void*)&(this->buf[this->writeP]), (void*)&(pL2e->outBuf[i]), 
           hunk * sizeof(long));
    if (this->writeP == 0)    /* SCN buffer was empty, need new starttime */
      this->starttime = starttime + i / this->samplerate;
    this->writeP += hunk;
    this->endtime = this->starttime + (this->writeP - 1) / this->samplerate;
    if ( this->writeP == pL2e->traceLen)
      if ( SendTrace( pL2e, this ) == EW_FAILURE)
        return EW_FAILURE;    /* Bad problem; we need to die */
    i += hunk;
  }

  return EW_SUCCESS;
}


int SendTrace( WORLD *pL2e, SCN_BUFFER *this)
{
  TracePacket   tbuf;      /* Earthworm trace data buffer (trace_buf.h) */
  int len;
  long *waveBuf;
  
  
  waveBuf = (long *)((char *)&tbuf + sizeof(TRACE_HEADER));
  
  tbuf.trh.pinno = this->pinno;
  strncpy(tbuf.trh.sta, this->sta, TRACE_STA_LEN);
  strncpy(tbuf.trh.chan, this->chan, TRACE_CHAN_LEN);
  strncpy(tbuf.trh.net, this->net, TRACE_NET_LEN);
  tbuf.trh.starttime = this->starttime;
  tbuf.trh.endtime = this->endtime;
  tbuf.trh.nsamp = pL2e->traceLen;
  tbuf.trh.samprate = this->samplerate;
  
  /* The decoders always write long integers */
#ifdef _SPARC
  strcpy(tbuf.trh.datatype, "s4");
#endif
#ifdef _INTEL
  strcpy(tbuf.trh.datatype, "i4");
#endif

  tbuf.trh.quality[0] = this->quality[0];
  tbuf.trh.quality[1] = this->quality[1];
  
  memcpy(waveBuf, this->buf, pL2e->traceLen * sizeof(long));
  this->writeP = 0;  /* Mark the buffer as empty */
  
  len = pL2e->traceLen * sizeof(long) + sizeof(TRACE_HEADER);

  if ( tport_putmsg( &(pL2e->regionOut), &(pL2e->waveLogo), len, (char*)&tbuf ) 
       != PUT_OK )
   {
     logit("et", "liss2ew: Error sending message via transport.\n");
     return EW_FAILURE;
   }
  
  return EW_SUCCESS;
}
