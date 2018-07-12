/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ew2liss.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2007/02/26 19:03:46  paulf
 *     cleaned up warning for matchSCN()
 *
 *     Revision 1.4  2007/02/26 19:00:07  paulf
 *     yet more warnings fixed related to time_t
 *
 *     Revision 1.3  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.2  2000/07/24 19:06:48  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/03/05 21:45:29  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * ew2liss: the Earthworm Live Internet Seismic Server module
 * This module reads TRACE_BUF packets from a single transport ring,
 * selects SCN packets for a single station, converts the data into
 * miniSEED format with Steim level-2 compression, and serves them to
 * one or more clients connected by TCP. Pete Lombard, 2/18/2000
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include "ew2liss.h"


int matchSCN (TRACE_HEADER *,  WORLD *);

main (int argc, char **argv)
{
  WORLD E2L;                       /* Our main data structure       */
  MSG_LOGO logoMsg;                /* logo of received message      */  
  long  sizeMsg;                   /* size of retrieved message     */
  time_t  timeNow;                   /* current time                  */ 
  time_t  timeLastBeat;              /* time last heartbeat was sent  */
  unsigned  tidProcess;            /* Processor thread id           */
  unsigned  tidServer;             /* Server thread id              */
  char *inBuf;                     /* Pointer to the input message buffer. */
  int inBufLen;                    /* Size of input message buffer  */
  TRACE_HEADER  *WaveHeader;
  int   ret;
  char  msgText[MAXMESSAGELEN];    /* string for log/error messages */

  /* Check command line arguments 
         ******************************/
  if (argc != 2)
  {
    fprintf (stderr, "Usage: ew2liss <configfile>\n");
    exit (EW_FAILURE);
  }

  /* Read config file and configure ourself */
  Configure(&E2L, argv);
  
  /* We will put the station index infrom of the trace message, so we   *
   * don't have to look up the SCN again at the other end of the queue. */
  inBufLen = MAX_TRACEBUF_SIZ + sizeof( int );
  if ( ! ( inBuf = (char *) malloc( (size_t) inBufLen ) ) )
  {
    logit( "e", "%s: Memory allocation failed - initial message buffer!\n",
        argv[0] );
    exit( EW_FAILURE );
  }
  WaveHeader = (TRACE_HEADER *) (inBuf + sizeof(int));
  
  /* Force a heartbeat to be issued in first pass thru main loop  */
  timeLastBeat = time (&timeNow) - E2L.Param.heartbeatInt - 1;


  /* Flush the incoming transport ring */
  while (tport_getmsg (&E2L.regionIn, &E2L.waveLogo, 1, &logoMsg,
                       &sizeMsg, inBuf, inBufLen) != GET_NONE);

  /* Start processor thread which will read messages from   *
   * the Queue, make them into miniSEED, and dispose of them on the output 
   * queue */
  if (StartThreadWithArg (ProcessThread, (void *) &E2L, 
                          (unsigned) THREAD_STACK, &tidProcess) == -1)
  {
    logit( "e", 
           "ew2liss: Error starting Processor thread.  Exitting.\n");
    E2L.terminate = 1;
    tport_detach (&E2L.regionIn);
    KillSelfThread();
  }

  /* Start server thread which will open a socket for listening,
   * accept connections, and send miniSEED from the output queue to
   * the connected host. */
  if (StartThreadWithArg (ServerThread, (void *) &E2L, 
                          (unsigned) THREAD_STACK, &tidServer) == -1)
  {
    logit( "e", 
           "ew2liss: Error starting Server thread.  Exitting.\n");
    tport_detach (&E2L.regionIn);
    E2L.terminate = 1;
    KillSelfThread();
  }



/*--------------------- setup done; start main loop -------------------------*/

  while (tport_getflag (&E2L.regionIn) != TERMINATE)
  {
    /* send ew2liss' heartbeat */
    if (time (&timeNow) - timeLastBeat >= E2L.Param.heartbeatInt) 
    {
      timeLastBeat = timeNow;
      StatusReport (&E2L, E2L.Ewh.typeHeartbeat, 0, ""); 
    }

    if (E2L.ProcessStatus < 0)
    {
      logit ("et", 
             "ew2liss: Processor thread died. Exitting\n");
      exit (EW_FAILURE);
    }

    ret = tport_getmsg (&E2L.regionIn, &E2L.waveLogo, 1, &logoMsg,
                        &sizeMsg, inBuf + sizeof(int), inBufLen - sizeof(int));

    /* Check return code; report errors */
    if (ret != GET_OK)
    {
      if (ret == GET_TOOBIG)
      {
        sprintf(msgText, "msg[%ld] i%d m%d t%d too long for target",
                 sizeMsg, (int) logoMsg.instid,
                 (int) logoMsg.mod, (int)logoMsg.type);
        StatusReport (&E2L, E2L.Ewh.typeError, ERR_TOOBIG, msgText);
        continue;
      }
      else if (ret == GET_MISS)
      {
        sprintf (msgText, "missed msg(s) i%d m%d t%d in %s",
                 (int) logoMsg.instid, (int) logoMsg.mod, 
                 (int)logoMsg.type, E2L.Param.ringIn);
        StatusReport (&E2L, E2L.Ewh.typeError, ERR_MISSMSG, msgText);
      }
      else if (ret == GET_NOTRACK)
      {
        sprintf (msgText, "no tracking for logo i%d m%d t%d in %s",
                 (int) logoMsg.instid, (int) logoMsg.mod, 
                 (int)logoMsg.type, E2L.Param.ringIn);
        StatusReport (&E2L, E2L.Ewh.typeError, ERR_NOTRACK, msgText);
      }
      else if (ret == GET_NONE)
      {
        sleep_ew(500);
      }
      continue;
    }

    /* Check to see if msg's SCN code is desired. Note that we don't need *
     * to do byte-swapping before we can read the SCN.                    */
    if ((ret = matchSCN (WaveHeader, &E2L )) < -1 )
    {
      logit ("et", "Ew2liss: Call to matchSCN failed; exitting.\n");
      exit (EW_FAILURE);
    }
    else if ( ret == -1 )
      /* Not an SCN we want */
      continue;
    
    /* stick the SCN number as an int at the front of the message */
    *((int*)inBuf) = ret; 

    /* If necessary, swap bytes in the wave message */
    if (WaveMsgMakeLocal (WaveHeader) < 0)
    {
      logit ("et", "Ew2liss: Unknown waveform type.\n");
      continue;
    }

    /* Queue retrieved msg */
    RequestSpecificMutex(&E2L.tbQMutex);
    ret = enqueue (&(E2L.tbQ), inBuf, sizeMsg + sizeof(int), logoMsg); 
    ReleaseSpecificMutex(&E2L.tbQMutex);

    if (ret != 0)
    {
      if (ret == -1)
      {
        sprintf (msgText, 
                 "Message too large for queue; Lost message.");
        StatusReport (&E2L, E2L.Ewh.typeError, ERR_QUEUE, msgText);
        continue;
      }
      if (ret == -3) 
      {
        sprintf (msgText, "Queue full. Old messages lost.");
        StatusReport (&E2L, E2L.Ewh.typeError, ERR_QUEUE, msgText);
        continue;
      }
    } /* problem from enqueue */

  } /* wait until TERMINATE is raised  */  

  /* Termination has been requested */
  
  logit ("t", "Termination requested; exitting!\n" );
  E2L.terminate = 1;
  sleep_ew(1000);  /* Give our siblings a chance to shut down. */
  tport_detach (&E2L.regionIn);
  KillSelfThread();
  exit(0);  /* not really */
}

/* StatusReport: Send error and hearbeat messages to transport ring     */
void StatusReport( WORLD* pE2L, unsigned char type, short code, 
                   char* message )
{
  char          outMsg[MAXMESSAGELEN];  /* The outgoing message.        */
  time_t        msgTime;        /* Time of the message.                 */

  /*  Get the time of the message                                       */
  time( &msgTime );
  
  /*  Build & process the message based on the type                     */
  if ( pE2L->Ewh.typeHeartbeat == type )
  {
    sprintf( outMsg, "%ld %ld\n", (long) msgTime, (long) pE2L->MyPid );
    
    /*Write the message to the output region                            */
    if ( tport_putmsg( &(pE2L->regionIn), &(pE2L->hrtLogo), 
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                       */
      logit( "et", "ew2liss: Failed to send a heartbeat message (%d).\n",
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
    if ( tport_putmsg( &(pE2L->regionIn), &(pE2L->errLogo), 
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                    */
      logit( "et", "ew2liss: Failed to send an error message (%d).\n",
             code );
    }
    
  }
}


/*
 * Jumbo configuration routine: read config file, look up EW parameters.
 * initialize a bunch of values, start up logit and attach to the transport
 * ring. Exits on failure, returns nothing.
 */
void Configure( WORLD *pE2L, char **argv)
{
  int i, nfiles, foundit = 0;
  char *com;
  FILE *sfp;
  
  /* Some important initial values or defaults */
  pE2L->sockTimeout = 0;
  pE2L->pscnB = (SCN_BUFFER*) NULL;
  pE2L->Nscn = 0;
  pE2L->terminate = 0;
  pE2L->ServerStatus = SS_DISCO;
  
  /* Read config file and configure ew2liss */
  if (ReadConfig(pE2L, argv[1]) == EW_FAILURE)
  {
    fprintf (stderr, "%s: configure() failed \n", argv[0]);
    exit (EW_FAILURE);
  }

  /*    Look up important info from earthworm tables                  */
  if ( ReadEWH( pE2L ) == EW_FAILURE )
  {
    fprintf( stderr, "%s error in ReadEWH.\n", argv[0] );
    exit( EW_FAILURE );
  }

  /* Set up logos for outgoing messages */
  pE2L->hrtLogo.instid = pE2L->Ewh.myInstId;
  pE2L->hrtLogo.mod    = pE2L->Ewh.myModId;
  pE2L->hrtLogo.type   = pE2L->Ewh.typeHeartbeat;

  pE2L->errLogo.instid = pE2L->Ewh.myInstId;
  pE2L->errLogo.mod    = pE2L->Ewh.myModId;
  pE2L->errLogo.type   = pE2L->Ewh.typeError;

  pE2L->waveLogo.instid = pE2L->Ewh.readInstId;
  pE2L->waveLogo.mod    = pE2L->Ewh.readModId;
  pE2L->waveLogo.type   = pE2L->Ewh.typeWaveform;

  /* Get my process ID so I can let statmgr restart me */
  pE2L->MyPid = getpid();
  
  /* Initialize name of log-file & open it */
  logit_init( argv[1], pE2L->Ewh.myModId, MAXMESSAGELEN, pE2L->Param.logSwitch );
  logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );

  /* Initialize SCN_BUFFER structures */
  for (i = 0; i < pE2L->Nscn; i++)
  {
    pE2L->pscnB[i].pb_endtime = 0.0;
    pE2L->pscnB[i].bestSR = -1.0;

    /* Allocate the space for writing SEED header and data */
    if ( (pE2L->pscnB[i].sdp = (SEED_data_record *)malloc(LISS_SEED_LEN)) ==
         (SEED_data_record *)NULL)
    {
      fprintf(stderr, "Error allocating memory for SEED buffer; exitting\n");
      exit( EW_FAILURE );
    }

    /* Set up the compression structures. This also allocates space for
     * the peek buffer. Note that PEEKELEMS (number of data elements in peek
     * buffer) is specified in steim.h */
    if ( (pE2L->pscnB[i].gdp = (gdptype) init_generic_compression(STEIM_DIFF,
          (MAXFRAMES + 8)/8, MAXFRAMES, STEIM_LEVEL, FLIP, 
          (generic_data_record *)pE2L->pscnB[i].sdp)) == (gdptype) NULL)
    {
      fprintf(stderr, "Error allocating memory for SEED buffer; exitting\n");
      exit( EW_FAILURE );
    }
    CleanSeed(&(pE2L->pscnB[i]));
  }
  
  /* Attach to input transport ring */
  tport_attach( &(pE2L->regionIn), pE2L->Ewh.ringInKey);
  
  /* Initialize input and output queues and their mutexes */
  initqueue (&pE2L->tbQ, IN_QUEUE_SIZE, MAX_TRACEBUF_SIZ + sizeof(int));
  initqueue (&pE2L->seedQ, OUT_QUEUE_SIZE, LISS_SEED_LEN);
  CreateSpecificMutex(&pE2L->tbQMutex);
  CreateSpecificMutex(&pE2L->seedQMutex);

  /* Read the sequence number file or create a new one */
  nfiles = k_open (pE2L->seqFile); 
  if (nfiles == 0) 
  {
    logit("e", "ew2liss: sequence file <%s> not found; creating new one\n",
          pE2L->seqFile);
    sfp = fopen( pE2L->seqFile, "w" );
    pE2L->seqNo = 0;
    if ( sfp != (FILE *) NULL )
    {
      fprintf( sfp,
               "# Next available LISS sequence number:\n" );
      fprintf( sfp, "%s %ld\n", SEQ_COMMAND, pE2L->seqNo );
      fclose( sfp );
    } 
    else
    {
      logit("e", "ew2liss: unable to create new sequence file; eixting\n");
      exit(EW_FAILURE);
    }
  } 
  else
  {
    while (k_rd())   /* Read next line of file; included files not allowed */
    {
      com = k_str();   /* Get the first token from line */
      
      /* Ignore blank lines & comments */
      if (!com) continue;
      if (com[0] == '#') continue;

      if(k_its(SEQ_COMMAND)) 
      {
        pE2L->seqNo = k_long();
        pE2L->seqNo = (pE2L->seqNo++) % MAXSEQ;
        foundit = 1;        
      }
      /* Unknown command */ 
      else 
      {
        fprintf (stderr, "ew2liss: <%s> Unknown command in <%s>.\n", 
                 com, pE2L->seqFile);
        continue;
      }
      
      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        fprintf (stderr, 
                 "ew2liss: Bad command in <%s>; exitting!\n\t%s\n",
                 pE2L->seqFile, k_com());
        exit( EW_FAILURE);
      }
    } /* while(k_rd()) */
    if (foundit == 0)
    {
      logit("e", "ew2liss: sequence number not found in seq. file; assuming 0\n");
      pE2L->seqNo = 0;
    }
  }
  return;
}

 
/*  Read some values from earthworm master ID file */
int ReadEWH( WORLD* pE2L )
{

  if ( GetLocalInst( &(pE2L->Ewh.myInstId)) != 0 )
  {
    fprintf(stderr, "ew2liss: Error getting myInstId.\n" );
    return EW_FAILURE;
  }
  
  if ( GetModId( pE2L->Param.myModName, &(pE2L->Ewh.myModId)) != 0 )
  {
    fprintf( stderr, "ew2liss: Error getting myModId.\n" );
    return EW_FAILURE;
  }

  if ((pE2L->Ewh.ringInKey = GetKey (pE2L->Param.ringIn) ) == -1) 
  {
    fprintf (stderr,
             "ew2liss:  Invalid ring name <%s>; exitting!\n", 
             pE2L->Param.ringIn);
    return EW_FAILURE;
  }

  /* Look up message types of interest */
  if ( GetInst( pE2L->Param.readInstName, &(pE2L->Ewh.readInstId)) != 0)
  {
    fprintf( stderr, "decimate: Error getting readInstId.\n" );
    return EW_FAILURE;
  }
  
  if ( GetModId( pE2L->Param.readModName, &(pE2L->Ewh.readModId)) != 0 )
  {
    fprintf( stderr, "decimate: Error getting readModId.\n" );
    return EW_FAILURE;
  }

  if (GetType ("TYPE_HEARTBEAT", &(pE2L->Ewh.typeHeartbeat)) != 0) 
  {
    fprintf (stderr, 
             "ew2liss: Invalid message type <TYPE_HEARTBEAT>; exitting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_ERROR", &(pE2L->Ewh.typeError)) != 0) 
  {
    fprintf (stderr, 
             "ew2liss: Invalid message type <TYPE_ERROR>; exitting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_TRACEBUF", &(pE2L->Ewh.typeWaveform)) != 0) 
  {
    fprintf (stderr, 
             "ew2liss: Invalid message type <TYPE_TRACEBUF>; exitting!\n");
    return EW_FAILURE;
  }

  return EW_SUCCESS;

} 

#define NUMREQ         10       /* Number of parameters that MUST be    */
                                /*   set from the config file.          */
/*      Function: ReadConfig                                            */
int ReadConfig (WORLD* pE2L, char *configfile )
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
             "ew2liss: Error opening command file <%s>; exitting!\n", 
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
                   "ew2liss: Error opening command file <%s>; exitting!\n", 
                   &com[1]);
          return EW_FAILURE;
        }
        continue;
      }

      /* Process anything else as a command */
      /*0*/ 	if (k_its ("MyModId")) 
      {
        if ( str = k_str () )
        {
          if (strlen(str) >= MAX_MOD_STR)
          {
            fprintf(stderr, "MyModId too long; max is %d\n", MAX_MOD_STR -1);
            return EW_FAILURE;
          }
          strcpy (pE2L->Param.myModName, str);
          init[0] = 1;
        }
      }
      /*1*/	else if (k_its ("InRing")) 
      {
        if ( str = k_str () )
        {
          if (strlen(str) >= MAX_RING_STR)
          {
            fprintf(stderr, "InRing name too long; max is %d\n", 
                    MAX_RING_STR - 1);
            return EW_FAILURE;
          }
          strcpy (pE2L->Param.ringIn, str);
          init[1] = 1;
        }
      }
      /*2*/	else if (k_its ("HeartBeatInterval")) 
      {
        pE2L->Param.heartbeatInt = k_long ();
        init[2] = 1;
      }

      /*3*/     else if (k_its ("LogFile"))
      {
        pE2L->Param.logSwitch = k_int();
        init[3] = 1;
      }

      /* 4 */else if (k_its ("LISSaddr")) 
      {
        if ( str = k_str () )
        {
          if (strlen(str) >= MAXADDRLEN)
          {
            fprintf(stderr, "LISSaddr too long; max is %d\n", MAXADDRLEN);
            return EW_FAILURE;
          }
          strcpy (pE2L->LISSaddr, str);
          init[4] = 1;
        }
      }

      /* 5 */ else if ( k_its ( "LISSport"))
      {
        pE2L->LISSport = k_int();
        if (pE2L->LISSport < 1)
        {
          fprintf(stderr, "LISSport is 0; this won't work.\n");
          return EW_FAILURE;
        }
        init[5] = 1;
      }
      
      /* Enter installation & module types to get */
      /* 6 */	else if (k_its ("GetWavesFrom")) 
      {
        if ((str = k_str()) != NULL) 
          strcpy(pE2L->Param.readInstName, str);

        if ((str = k_str()) != NULL) 
          strcpy(pE2L->Param.readModName, str);

        init[6] = 1;
      }

      /* Sequence number file name */
      /* 7 */ else if (k_its ("SeqFile"))
      {
        if ( str = k_str () )
        {
          if (strlen(str) >= MAXFILENAMELEN)
          {
            fprintf(stderr, "SeqFile too long; max is %d\n", MAXFILENAMELEN -1);
            return EW_FAILURE;
          }
          strcpy (pE2L->seqFile, str);
          init[7] = 1;
        }
      }

      /* 8 */    else if (k_its ("MaxSCNs"))
      {
        if (pE2L->Nscn != 0)
        {
          fprintf(stderr, "Multiple MaxSCNs commands found; exitting\n");
          exit( -1 );
        }
        pE2L->Nscn = k_int();
        if (pE2L->Nscn <= 0 || pE2L->Nscn > 256)
        {
          fprintf(stderr, "Unreasonable value for MaxSCNs: %d\n", pE2L->Nscn);
          return EW_FAILURE;
        }
        if ( (pE2L->pscnB = (SCN_BUFFER *)
              malloc(pE2L->Nscn* sizeof(SCN_BUFFER))) == (SCN_BUFFER *) NULL)
        {
          fprintf(stderr, "Error allocating SCN buffers; exiting\n");
          exit( -1 );
        }
        init[8] = 1;
      }
          
      /*9*/   else if( k_its("ServeSCN") ) 
      {
        if (pE2L->Nscn == 0)
        {
          fprintf(stderr, "MaxSCNs command must come before ServeSCN commands\n");
          exit( -1 );
        }
        if( Iscn >= pE2L->Nscn )
        {
          fprintf( stderr,
                   "ew2liss: Too many <ServeSCN> commands in <%s>;\n",
                   configfile );
          fprintf( stderr, 
                   "               limited by command-line-arg <-# %d>; exiting!\n",
                   pE2L->Nscn );
          exit( -1 );
        }
        str = k_str();   /* read station code */
        if (str != (char *)NULL)
        {
          if( strlen(str) < TRACE_STA_LEN ) {
            strcpy( pE2L->pscnB[Iscn].sta, str );
          } else {
            fprintf( stderr,"ew2liss: error in <ServeSCN> command in <%s>:\n",
                     configfile );
            fprintf( stderr,"               sta code <%s> too long; maxchar=%d; exiting!\n",
                     str, TRACE_STA_LEN-1 );
            exit( -1 );
          };
          str = k_str();   /* read component code */
          if (str != (char *) NULL)
          {
            if( strlen(str) < TRACE_CHAN_LEN ) {
              strcpy( pE2L->pscnB[Iscn].chan, str );
            } else {
              fprintf( stderr,"ew2liss: error in <ServeSCN> command in <%s>:\n",
                       configfile );
              fprintf( stderr,"               channel code <%s> too long; maxchar=%d; exiting!\n",
                       str, TRACE_CHAN_LEN-1 );
              exit( -1 );
            };
            str = k_str();  /* read network code */
            if (str != (char *)NULL)
            {
              if( strlen(str) < TRACE_NET_LEN ) {
                strcpy( pE2L->pscnB[Iscn].net, str );
              } else {
                fprintf( stderr,"ew2liss: error in <ServeSCN> command in <%s>:\n",
                         configfile );
                fprintf( stderr,"               network code <%s> too long; maxchar=%d; exiting!\n",
                         str, TRACE_NET_LEN-1 );
                exit( -1 );
              };
              str = k_str();  /* read location code */
              if (str != (char *)NULL)
              {
                if( strlen(str) < LC_LEN ) {
                  strcpy( pE2L->pscnB[Iscn].locID, str );
                } else {
                  fprintf( stderr,"ew2liss: error in <ServeSCN> command in <%s>:\n",
                           configfile );
                  fprintf( stderr,"               location code <%s> too long; maxchar=%d; exiting!\n",
                           str, LC_LEN-1 );
                  exit( -1 );
                };
              }
              else
                pE2L->pscnB[Iscn].locID[0] = '\0';
            }
            Iscn++;
            init[9] = 1;
          }
        }
      }

      /* Optional: */ else if ( k_its ( "SocketTimeout"))
      {
        pE2L->sockTimeout = k_int(); /* Convert to milliseconds below */
      }
      
      /* Optional: debug command */
      else if (k_its( "Debug") )
      {
        pE2L->Param.debug = 1;
      }
      
      /* Unknown command */ 
      else 
      {
        fprintf (stderr, "ew2liss: <%s> Unknown command in <%s>.\n", 
                 com, configfile);
        continue;
      }

      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        fprintf (stderr, 
                 "ew2liss: Bad command in <%s>; exitting!\n\t%s\n",
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
    fprintf (stderr, "ew2liss: ERROR, no ");
    if (!init[0])  fprintf (stderr, "<MyModId> "        );
    if (!init[1])  fprintf (stderr, "<InRing> " );
    if (!init[2])  fprintf (stderr, "<HeartBeatInterval> "     );
    if (!init[3])  fprintf (stderr, "<LogFIle> "     );
    if (!init[4])  fprintf (stderr, "<LISSaddr> "     );
    if (!init[5])  fprintf (stderr, "<LISSport> "     );
    if (!init[6])  fprintf (stderr, "<GetWavesFrom> " );
    if (!init[7])  fprintf (stderr, "<SeqFile> "      );
    if (!init[8])  fprintf (stderr, "<MaxSCNs> "      );
    if (!init[9])  fprintf (stderr, "<ServeSCN> "    );
    
    fprintf (stderr, "command(s) in <%s>; exitting!\n", configfile);
    return EW_FAILURE;
  }

  /* We don't actually need to relate sockTimeout to heartbeatInt in this
   * version of ew2liss, but it's a convenient default value: PNL, 3/1/2000 */
  if (pE2L->sockTimeout > pE2L->Param.heartbeatInt || pE2L->sockTimeout <= 0)
    pE2L->sockTimeout = pE2L->Param.heartbeatInt;
  
  pE2L->sockTimeout *= 1000; /* We use milliseconds internally */

  return EW_SUCCESS;
}

int matchSCN (TRACE_HEADER *WaveHead, WORLD *pE2L )
{
  int i;
  
  if ((WaveHead->sta == NULL) || (WaveHead->chan == NULL) || 
      (WaveHead->net == NULL) )
  {
    logit ("et",  "ew2liss: invalid parameters to matchSCN\n");
    return (-2);
  }
  
  for (i = 0; i < pE2L->Nscn; i++ )
  {
    /* try to match explicitly */
    if ((strcmp (WaveHead->sta, pE2L->pscnB[i].sta) == 0) &&
        (strcmp (WaveHead->chan, pE2L->pscnB[i].chan) == 0) &&
        (strcmp (WaveHead->net, pE2L->pscnB[i].net) == 0))
      return (i);
  }
  /* No match */
  return -1;
}
  
