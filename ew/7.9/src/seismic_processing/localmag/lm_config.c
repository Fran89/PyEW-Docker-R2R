/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_config.c 6333 2015-05-06 04:53:22Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.19  2010/03/15 16:22:50  paulf
 *     merged from Matteos branch for adding version to directory naming
 *
 *     Revision 1.18.2.1  2010/03/15 15:15:55  quintiliani
 *     Changed output file and directory name
 *     Detailed description in ticket 22 from
 *     http://bigboy.isti.com/trac/earthworm/ticket/22
 *
 *     Revision 1.18  2008/01/09 01:06:43  paulf
 *     a fix from 1.11 version that got lost by me in the v6.3 EW release upgrade...
 *
 *     Revision 1.17  2007/07/20 13:54:06  withers
 *     Fixed saveXMLdir bug. plmParams->saveXMLdir is a pointer so need to be
 *     sure to malloc space for the directory string.  strdup does this.
 *
 *     Revision 1.16  2007/03/30 14:14:05  paulf
 *     added saveXMLdir option
 *
 *     Revision 1.15  2007/03/29 20:09:50  paulf
 *     added eventXML option from INGV. This option allows writing the Shakemap style event information out as XML in the SAC out dir
 *
 *     Revision 1.14  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.13  2005/08/08 18:38:14  friberg
 *
 *     Fixed bug from last version that had station corrections added in twice.
 *     Added in new directive require2Horizontals to require 2 components for a station Ml
 *     	example:   require2Horizontals 1
 *     	Note needs the 1 after the directive to be used
 *     Added in new directive useMedian
 *     	example:   useMedian
 *     	Note for this one, no flag is needed after the directive
 *     Also updated the Doc files.
 *
 *     Revision 1.12  2005/07/27 16:35:21  friberg
 *     minStationsMl changes
 *
 *
 *     Revision 1.10  2002/09/10 17:07:18  dhanych
 *     stable scaffold
 *
 *     Revision 1.9  2002/03/17 18:16:38  lombard
 *     Added LogFile command, added second logit_init call to conform to
 *       latest standard.
 *     Added SgSpeed, searchTimes, searchStartPhase commands in place
 *       of searchWindow command to support new search times calculation.
 *     Added extraDelay command to control localmag scheduling for realtime
 *       events.
 *
 *     Revision 1.8  2001/06/10 21:21:46  lombard
 *     Changed single transport ring to two rings, added allowance
 *     for multiple getEventsFrom commands.
 *     These changes necessitated several changes in the way config
 *     and earthworm*.d files were handled.
 *
 *     Revision 1.7  2001/05/31 17:41:13  lucky
 *     Added support for outputFormat = File. This option works only in
 *     standalone mode. It writes TYPE_MAGNITUDE message to a specified file.
 *     We need this for review.
 *
 *     Revision 1.6  2001/04/11 21:07:08  lombard
 *     "site.?" renamed to "lm_site.?" for clarity.
 *
 *     Revision 1.5  2001/03/01 05:25:44  lombard
 *     changed FFT package to fft99; fixed bugs in handling of SCNPars;
 *     changed output to Magnitude message using rw_mag.c
 *
 *     Revision 1.4  2001/01/15 03:55:55  lombard
 *     bug fixes, change of main loop, addition of stacker thread;
 *     moved fft_prep, transfer and sing to libsrc/util.
 *
 *     Revision 1.3  2000/12/31 17:27:25  lombard
 *     More bug fixes and cleanup.
 *
 *     Revision 1.2  2000/12/25 22:14:39  lombard
 *     bug fixes and development
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_config: routines for configuring the localmag program.
 * Configuration parameters can be specified in a config file using
 * Earthworm Kom-style commands, or on the command line. Settings frpm
 * the command line take precedence over the config file settings.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <kom.h>
#include <tlay.h>
#include <transfer.h>
#include <transport.h>
#include <ws_clientII.h>
#include <read_arc.h>
#include "lm.h"
#include "lm_config.h"
#include "lm_version.h"
#include "lm_sac.h"
#include "lm_util.h"
#include "lm_ws.h"
#include "lm_site.h"


/* Internal Function Prototypes */
static void InitConfig( LMPARAMS * );
static int ReadConfig (LMPARAMS *, char *, int * );
static void ParseCommand( LMPARAMS *, int, char **, char **, int *);
static void usage(char *);

/*
 * Configure: do all the configuration of localmag.
 * This includes initializing the LMPARAMS structure, parsing the command-line
 * arguments, parsing the config file, and checking configuration settings
 * for consistency.
 *    Returns: 0 on success
 *            -1 on failure
 */
int Configure( LMPARAMS *plmParams, int argc, char **argv, EVENT *pEvt)
{
  int rc, logSwitch = 1;
  char *configFile;
  char *str, servDir[LM_MAXTXT];
  
  /* Set initial values, then parse the command-line options */
  InitConfig( plmParams );
  ParseCommand( plmParams, argc, argv, &configFile, &logSwitch );

  /* Initialize the Earthworm logit system */
  logit_init(configFile, 0, MAX_BYTES_PER_EQ, logSwitch);

  /* Read the configuration file */
  if ( (rc = ReadConfig( plmParams, configFile, &logSwitch )) < 0)
    return rc;   /* Error; ReadConfig already complained */

  /* Reset the logSwitch in logit */
  logit_init(configFile, 0, MAX_BYTES_PER_EQ, logSwitch);
  
  /* Now check that all the parameters make sense */
  rc = 0;
  
  /* Set defaults if they weren't already set */
  if (plmParams->eventSource == LM_UNDEF)
    plmParams->eventSource = LM_ES_ARCH;  /* Default is hyp2000 archive msg */
  if (plmParams->fGetAmpFromSource == LM_UNDEF)  /* not set... */
    plmParams->fGetAmpFromSource = FALSE;        /* so set the default */
  if (plmParams->traceSource == LM_UNDEF )
    plmParams->traceSource = LM_TS_WS;
  if (plmParams->fWAsource == LM_UNDEF)    /* not set... */
    plmParams->fWAsource = FALSE;          /* so set the default */
  if (plmParams->saveTrace == LM_UNDEF)
    plmParams->saveTrace = LM_ST_NO;

  /* If we're running as an earthworm module... */
  if (plmParams->fEWTP == 1)
  {
    if ( plmParams->pEW->hrtLogo.mod == 0)
    {
      logit("e", "Configure: MyModId required when transport ring is used\n");
      rc = -1;
    }
    if ( plmParams->pEW->nGetLogo == 0 )
    {
      logit("e", "Configure: getEventsFrom required when transport ring is used\n");
      rc = -1;
    }
    if (plmParams->pEW->RingOutKey == 0l) 
    {
      logit("e", "Configure: RingOutKey required when RingInKey is given\n");
      rc = -1;
    }
    if (plmParams->minStationsMl <= 0)
    {
      logit("e", "Configure: minStationsML to something greater than 0\n");
      rc = -1;
    }
    if (plmParams->HeartBeatInterval == 0)
    {
      logit("e", "Configure: heartbeat interval required when transport ring is used\n");
      rc = -1;
    }
    if (plmParams->eventSource != LM_ES_ARCH)
    {
      logit("e", "Configure: input source must be `ARCH' when transport ring is used\n");
      rc = -1;
    }
    if (plmParams->traceSource != LM_TS_WS)
    {
      logit("e", "Configure: trace source must be `waveServer' when transport ring is used\n");
      rc = -1;
    }
    if (plmParams->outputFormat == LM_UNDEF)
      plmParams->outputFormat = LM_OM_LM;

    setMaxDelay(plmParams, pEvt);
    
  }
  else
  {   /* Not using earthworm transport */
    if (plmParams->outputFormat == LM_UNDEF)
      plmParams->outputFormat = LM_OM_SO;
    else if (plmParams->outputFormat == LM_OM_LM)
    {
      logit("e", "Configure: output sent to transport but no transport specified\n");
      rc = -1;
    }
  }
  
  /* Event Source */
  switch(plmParams->eventSource)
  {
  case LM_ES_SAC:
    if (plmParams->traceSource != LM_TS_SAC)
    {
      logit("e", "Configure: eventSource being SAC requires traceSource to be SAC\n");
      rc = -1;
    }
    break;
  case LM_ES_ARCH:
    /* Nothing to do for this */
    break;
#ifdef EWDB
  case LM_ES_EWDB:
    if (plmParams->pDBaccess == (DBACCESS *)NULL)
    {
      logit("e", "Configure: input source is EWDB but DBaccess not given\n");
      rc = -1;
    }
    break;
#endif
#ifdef UW
  case LM_ES_UW:
    /* Nothing to do for this */
    break;
#endif    
  default:
    logit("e", "Configure: unknown input source <%d>\n", 
          plmParams->eventSource);
    rc = -1;
  }
  
  /* Initialize the trace arrays.                                        *
   * We have to initialize at least one array for magArray even if       *
   * we won't be handling any trace data. This is a slight trick; sorry! */
  if ( initBufs( plmParams->maxTrace, plmParams->fGetAmpFromSource ) < 0)
  {     /* initBufs already complained, so be silent here */
    return -1;
  }
  
  /* Do we need to get traces? */
  if (plmParams->fGetAmpFromSource == FALSE)
  {   /* Yes, we need traces */
    switch( plmParams->traceSource)
    {
    case LM_TS_WS:
      if (plmParams->pWSV == (WS_ACCESS *)NULL)
      {   /* Set the default traceSource to wave_servers listed in *
           * ${EW_PARAMS}/servers                                  */
        if ( (str = getenv("EW_PARAMS")) == NULL)
        {
          logit("e", "Configure: environment variable EW_PARAMS not defined\n");
          return -1;
        }
        if (strlen(str) > LM_MAXTXT - (strlen(DEF_SERVER) + 2))
        {
          logit("e", "Configure: environment variable EW_PARAMS too long;"
                " increase LM_MAXTXT and recompile\n");
          return -1;
        }
        sprintf(servDir, "%s/%s", str, DEF_SERVER);
        if ( (plmParams->pWSV = 
              (WS_ACCESS *)calloc(1, sizeof(WS_ACCESS))) == 
             (WS_ACCESS *)NULL)
        {
          logit("e", "Configure: out of memory for WS_ACCESS\n");
          return -1;
        }
        if ( (plmParams->pWSV->serverFile = strdup(servDir)) == NULL)
        {
          logit("e", "Configure: out of memory for serverFile\n");
          return -1;
        }
      }
      if (plmParams->pWSV->pList == (SERVER *)NULL)
      {
        if (readServerFile(plmParams) < 0)
          return -1;
      }
      if (initWsBuf(plmParams->maxTrace) < 0)
      {
        logit("e", "Configure: out of memory for trace_buf buffer\n");
        return -1;
      }
      if (plmParams->wsTimeout == 0)
        plmParams->wsTimeout = 5000;  /* Default, 5 seconds */
      break;
#ifdef EWDB
    case LM_TS_EWDB:
      if (plmParams->pDBaccess == (DBACCESS *)NULL)
      {
        logit("e", 
                "Configure: traceSource is EWDB but EWDBaccess not given\n");
        rc = -1;
      }
      break;
#endif
    case LM_TS_SAC:
      if (plmParams->sacInDir == (char *)NULL)
      {
        logit("e", "Configure: traceSource is SAC but SACsource unknown\n");
        rc = -1;
      }
      break;
#ifdef UW
    case LM_TS_UW:
      if (plmParams->eventSource != LM_ES_UW)
      {
        logit("e", "Configure: event source must be UW if trace source is UW\n");
        rc = -1;
      }
      if (plmParams->saveTrace != LM_ST_NO && plmParams->saveTrace != LM_ST_UW)
      {
        logit("e", "Configure: UW trace data has no network names\n"
              "\tTherefore, UW WA traces cannot be saved to formats other than UW\n");
        rc = -1;
      }
      break;
#endif
    default:
      logit("e", "Configure: unknown trace source <%d>.\n", 
            plmParams->traceSource);
      rc = -1;
    }

    /* Are we going to synthesize Wood-Anderson traces? */
    if (plmParams->fWAsource == FALSE)
    {   /* yes, so we need instrument response data */
      switch( plmParams->respSource )
      {
      case LM_UNDEF:
        logit("e", "Configure: required response source not specified\n");
        rc = -1;
        break;
      case LM_RS_SAC:
        if (plmParams->sacInDir == (char *)NULL)
        {
          logit("e", 
                  "Configure: response source is SAC but SACsource unknown\n");
          rc = -1;
        }
        break;
      case LM_RS_FILE:
        /* Nothing to do for this */
        break;
#ifdef EWDB
      case LM_RS_EWDB:
        if (plmParams->pDBaccess == NULL)
        {
          logit("e", 
                  "Configure: response source is EWDB but EWDBaccess not given\n");
          rc = -1;
        }
        break;
#endif
#ifdef UW
      case LM_RS_UW:
        /* Nothing to do for this */
        break;
#endif        
      default:
        logit("e", "Configure: unknown response source %d\n", 
              plmParams->respSource);
        rc = -1;
      }

      /* Set up the Response structure for the Wood-Anderson */
      if ( getWAResp(plmParams->pWA) < 0)
      {
        logit("et", "Configure: out of memory for WA response\n");
        return -1;
      }
    }
    else   /* We are reading Wood-Anderson traces from the source */
    {
      /* Silently turn off trace saving */
      plmParams->saveTrace = LM_ST_NO;
    }
  }
  else   /* We're getting amplitudes directly from source, not from traces */
  {   /* plmParams->fGetAmpFromSource == TRUE */
    if (plmParams->traceSource == LM_TS_WS)
    {
      logit("e", "Configure: cannot read Amp from wave_server source\n");
      rc = -1;
    }
    /* Silently turn off trace saving */
    plmParams->saveTrace = LM_ST_NO;
  }

  /* Station Location Source: readConfig requires that it be set. */
  switch (plmParams->staLoc)
  {
  case LM_SL_SAC:
    if (plmParams->traceSource != LM_TS_SAC)
    {
      logit("e", 
              "Configure: staLoc being SAC requires that traceSource be SAC\n");
      rc = -1;
    }
    break;
  case LM_SL_HYP:
    if ( site_read(plmParams->staLocFile) < 0)
    {
       logit("e", "Configure: site_read() failed\n" );
       rc = -1;
    }
    break;
#ifdef EWDB
  case LM_SL_EWDB:
    if ( plmParams->pDBaccess == (DBACCESS *)NULL)
    {
      logit("e", "Configure: staLoc is EWDB but EWDBaccess not given\n");
      rc = -1;
    }
    break;
#endif
#ifdef UW
  case LM_SL_UW:
    if (plmParams->eventSource != LM_ES_UW)
    {
      logit("e", "Configure: can't use UW staloc unless event source is UW\n");
      rc = -1;
    }
    /* The Stafileif gets initialized when the pickfile is opened *
     * in getUWpick().                                            */
    break;
#endif
  default:
    logit("e", "Configure: unknown staLoc <%d>\n", plmParams->staLoc);
    rc = -1;
  }
  
  if (plmParams->traceSource == LM_TS_SAC || plmParams->saveTrace == LM_ST_SAC)
  {  /* Initialize the SAC data array */
    if ( initSACBuf( plmParams->maxTrace ) < 0)
    {
      logit("e", "Configure: out of memory for SAC data\n");
      return -1;
    }
  }

  if (rc == 0)
  {    /* Initialize the station list */
    if ( (pEvt->Sta = (STA *)calloc( plmParams->maxSta, sizeof(STA))) == 
         (STA *)NULL)
    {
      logit("et", "Configure: out of memory for STA array\n");
      return -1;
    }

    /* Initialize the ArcPck list */
    /* TODO set the properly value for maxArcPck */
    pEvt->maxArcPck = plmParams->maxSta * 2;
    pEvt->numArcPck = 0;
    if ( (pEvt->ArcPck = (struct Hpck *)calloc( pEvt->maxArcPck, sizeof(struct Hpck))) == 
	    (struct Hpck *)NULL)
    {
	logit("et", "Configure: out of memory for ArcPck array\n");
	return -1;
    }
    if(plmParams->SkipStationsNotInArc) {
	logit("", "Configure: skip stations not in ARC message. MinWeightPercent = %.2lf.\n", plmParams->MinWeightPercent);
    }
    if(plmParams->MLQpar1 > 0.0) {
	logit("", "Configure: enabled computation of magnitude quality. MLQpar1 = %.2lf.\n", plmParams->MLQpar1);
    }

    /* Initialize the logA0 table */
    if ( initLogA0(plmParams) < 0)
      return -1;

    /* Verify reasonable values */
    switch(plmParams->fDist)
    {
    case LM_LD_EPI:
    case LM_LD_HYPO:
      break;    /* These values are OK */
    default:
      logit("e", "Configure: unknown value (%d) for LogA0 distance rule\n",
            plmParams->fDist);
      rc = -1;
    }
    if (plmParams->nLtab < 1)
    {
      logit("e", "Configure: no entries read for LogA0 table\n");
      rc = -1;
    }
  }

  /* Turn on some debugging options */
  if (plmParams->debug & LM_DBG_WSC)
    (void)setWsClient_ewDebug(1);
  
  if (plmParams->debug & (LM_DBG_PZG | LM_DBG_TRS | LM_DBG_ARS))
    transferDebug(plmParams->debug >> 5);

  /* Sort the SCNLPAR array to make searching more efficient */
  if (plmParams->numSCNLPar > 0)
    qsort(plmParams->pSCNLPar, plmParams->numSCNLPar, sizeof(SCNLPAR),
          CompareSCNLPARs);
  
  return rc;
}

  
  
  

#define NUMREQ 6       /* Number of parameters that MUST be    */
                       /*   set from the config file.          */

/*      Function: ReadConfig                                            */
static int ReadConfig (LMPARAMS* plmParams, char *configfile, int *logSwitch )
{
  char     init[NUMREQ];  /* init flags, one byte for each required command */
  int      nmiss;         /* number of required commands that were missed   */
  char     *com;
  char     *str;
  char     *processor;
  int      nfiles, rc;
  int      i;
  int      err = 0;
  SCNLSEL   *newSel, *pAdd, *pDel;
  LMEW     *pEW;
  char     configPath[LM_MAXTXT], *paramsDir;
  
  pEW = plmParams->pEW;

  /* set defaults */
  plmParams->minStationsMl = MINIMUM_STATIONS_TO_REPORT;
  plmParams->require2Horizontals = 0;   /* do not require both horizontals*/
  plmParams->allowVerticals = 0;   /* do not allow verticals by default*/
  plmParams->SkipStationsNotInArc = 0;  /* do not skip stations that are not in the ARC message by default */
  plmParams->MinWeightPercent = 0.0;    /* MinWeightPercent when SkipStationsNotInArc is true.
					   Default 0%, all phases will be taken */
  plmParams->MLQpar1 = -1.0;           /* First parameter for computing the quality of magnitude.
					   Default -1.0, disable the computation of quality. */
  plmParams->eventXML = 0;   /* do not output XML event file */
  plmParams->saveSCNL = 0;   /* use SCN as default for TYPE_MAGNITUDE message (compatibility sake) */
  plmParams->saveXMLdir = NULL;   /* output XML event file here instead of sac dir */
  plmParams->LookAtVersion = vAll;   /* Look at all versions of all events */
  plmParams->useMedian = 0;   /* use MEAN by default */
  plmParams->MlmsgOutDir = NULL;   /* output Ml message here */
  // ERROR plmParams->saveTrace = LM_ST_NO;	 /* do not save WA traces by default */

  /* Set to zero one init flag for each required command */
  for (i = 0; i < NUMREQ; i++)
    init[i] = 0;

  /* Open the main configuration file 
   **********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    if ( (paramsDir = getenv("EW_PARAMS")) == NULL)
    {
      logit("e", "localmag: Error opening command file <%s>; EW_PARAMS not set\n", 
            configfile);
      return -1;
    }
    strcpy(configPath, paramsDir);
    if (configPath[strlen(configPath)-1] != '/' || 
        configPath[strlen(configPath)-1] != '\\')
      strcat(configPath, "/");
    strcat(configPath, configfile);
    nfiles = k_open (configPath); 
    if (nfiles == 0) 
    {
      logit("e", "localmag: Error opening command file <%s> or <%s>\n", 
            configfile, configPath);
      return -1;
    }
  }

  /* Process all command files
   ***************************/
  while (nfiles > 0)   /* While there are command files open */
  {
    while (k_rd ())        /* Read next line from active file  */
    {  
      com = k_str ();         /* Get the first token from line */

      processor = "ReadConfig";
      
      /* Ignore blank lines & comments
       *******************************/
      if (!com)
        continue;
      if (com[0] == '#')
        continue;

      /* Open a nested configuration file */
      if (com[0] == '@') 
      {
        if ( (rc = k_open (&com[1])) == 0)
        {
          logit("e", "localmag: Error opening command file <%s>\n", 
                   &com[1]);
          return -1;
        }
        nfiles = rc;
        continue;
      }

      /* Process anything else as a command */
      /* Input Source: optional */
      if (k_its ("eventSource")) 
      {
        if (plmParams->eventSource == LM_UNDEF)
        {                 /* Let command-line take precedence */
          if ( (str = k_str ()) )
          {
            if (k_its("ARCH"))
              plmParams->eventSource = LM_ES_ARCH;
            else if (k_its("SAC"))
            {
              plmParams->eventSource = LM_ES_SAC;
            }
#ifdef EWDB
            else if (k_its("EWDB"))
            {
              plmParams->eventSource = LM_ES_EWDB;
              if ( (str = k_str()) )
              {
                if ( (plmParams->eventID = strdup(str)) == NULL)
                {
                  logit("e", 
                          "ReadConfig: out of memory for EventID\n");
                  return -1;
                }
              }
              else
              {
                logit("e",
                        "ReadConfig: \"eventSource EWDB\" missing EventID\n");
                err = -1;
              }
            }
#endif
#ifdef UW
            else if (k_its("UW"))
            {
              plmParams->eventSource = LM_ES_UW;
              if ( (str = k_str()) )
              {
                if ( (plmParams->UWpickfile = strdup(str)) == NULL)
                {
                  logit("e", 
                          "ReadConfig: out of memory for pickfile\n");
                  return -1;
                }
              }
              else
              {
                logit("e",
                        "ReadConfig: \"eventSource UW\" missing pickfile\n");
                err = -1;
              }
            }
#endif
            else
            {
              logit("e", "ReadConfig: Unknown eventSource <%s>\n", str);
              err = -1;
            }
          }
          else
          {
            logit("e", "ReadConfig: Missing eventSource argument\n");
            err = -1;
          }
        } /* else already set from command-line */
      }
      
      /* Trace Source: optional */
      else if (k_its( "traceSource" ))
      {
        if (plmParams->traceSource == LM_UNDEF)
        {                 /* Let command-line take precedence */
          if ( (str = k_str()) )
          {
            if (k_its("waveServer"))
            {
              plmParams->traceSource = LM_TS_WS;
              if ( (str = k_str()) )
              {
                if ( (plmParams->pWSV = 
                      (WS_ACCESS *)calloc(1, sizeof(WS_ACCESS))) == NULL)
                {
                  logit("e", 
                          "ReadConfig: out of memory for SERVER\n");
                  return -1;
                }
                if (k_its("File"))
                {
                  if ( (str = k_str()) )
                  {
                    if ( (plmParams->pWSV->serverFile = strdup(str)) == NULL)
                    {
                      logit("e", 
                              "ReadConfig: out of memory for serverFile\n");
                      return -1;
                    }
                  }
                  else
                  {
                    logit("e", 
                            "ReadConfig: \"traceSource waveServer file\" missing filename\n");
                    err = -1;
                  }
                }
                else
                {
                  int ws_err;
                  while(str)
                  {
                    if (Add2ServerList(str, plmParams) < 0)
                      err = -1;  /* Add2ServerList already complained */
                    str = k_str();
                  }
                  /* We have to catch the kom error here since we are *
                   * intentionally trying to read to the end of the   *
                   * string.                                          */
                  ws_err = k_err();
                  if (ws_err == -17 && plmParams->pWSV->pList != 
                      (PSERVER) NULL)
                    continue;
                  else if (ws_err < 0)
                  {
                    logit("e", 
                          "localmag: Bad <%s> command in <%s>\n\t%s\n",
                          processor, configfile, k_com());
                    return -1;
                  }
                }
              }
              /* else default waveServer file is "servers" in $EW_PARAMS dir */
            }
            else if (k_its("SACFile"))
              plmParams->traceSource = LM_TS_SAC;
            else if (k_its("SACWAFile"))
            {
              plmParams->traceSource = LM_TS_SAC;
              plmParams->fWAsource = TRUE;
            }
#ifdef EWDB
            else if (k_its("EWDB"))
            {
              plmParams->traceSource = LM_TS_EWDB;
            }
#endif
#ifdef UW            
            else if (k_its("UWData"))
              plmParams->traceSource = LM_TS_UW;
            else if (k_its("UWWAData"))
            {
              plmParams->traceSource = LM_TS_UW;
              plmParams->fWAsource = TRUE;
            }
#endif
            else
            {
              logit("e", "ReadConfig: Unknown traceSource <%s>\n", str);
              err = -1;
            }
          }
          else
          {
            logit("e", "ReadConfig: Missing traceSource argument\n");
            err = -1;
          }
        } /* else already set from command-line */
      }
      /* number mapping 123 to ZNE  - default is ENZ - OPTIONAL*/
      else if (k_its("ChannelNumberMap") )
      {
        if ( (str = k_str()) )
        {
          if (strlen(str) > 3) 
          {
            logit("e", "ReadConfig: ChannelNumberMap is too long, only 3 chars allowed\n");
            err = -1;
          }
          else 
          {
            /* note this copies the string to position 1, so that numbers can be mapped to chars easily */
            strcpy(&plmParams->ChannelNumberMap[1], str);
            plmParams->ChannelNumberMap[0]='_'; 
          }
        }
      }
      /* SAC source: optional */
      else if (k_its("SACsource") )
      {
        if (plmParams->sacInDir == (char *)NULL)
        {
          if ( (str = k_str()) )
          {
            if ( (plmParams->sacInDir = strdup(str)) == NULL)
            {
              logit("e", "ReadConfig: out of memory for sacInDir\n");
              return -1;
            }
            if ( (str = k_str()) )
            {
              if ( (plmParams->sourceNameFormat = strdup(str)) == NULL)
              {
                logit("e", 
                      "ReadConfig: out of memory for sourceNameFormat\n");
                return -1;
              }
            }
            else
            {
              logit("e",
                    "ReadConfig: \"SACsource\" missing arguments\n");
              err = -1;
              free(plmParams->sacInDir);
              plmParams->sacInDir = NULL;
            }
          }
          else
          {
            logit("e",
                  "ReadConfig: \"SACsource\" missing arguments\n");
            err = -1;
          }
        }  /* else already set from command-line */
      }
      
      /* Station location source: required */
      else if (k_its("staLoc") )
      {
        if ( (str = k_str()) )
        {
          if (k_its("File") )
          {
            if ( (str = k_str()) )
            {
              if ( (plmParams->staLocFile = strdup(str)) == NULL)
              {
                logit("e", "ReadConfig: out of memory for staLoc\n");
                return -1;
              }
              plmParams->staLoc = LM_SL_HYP;
              init[0] = 1;
            }
            else
            {
              logit("e", "ReadConfig: \"staLoc File\" missing filename\n");
              err = -1;
            }
          }
#ifdef EWDB
          else if (k_its("EWDB") )
          {
            plmParams->staLoc = LM_SL_EWDB;
            init[0] = 1;
          }
#endif
          else if (k_its("SAC") )
          {
            plmParams->staLoc = LM_SL_SAC;
            init[0] = 1;
          }
#ifdef UW
          else if (k_its("UW") )
          {
            plmParams->staLoc = LM_SL_UW;
            init[0] = 1;
          }
#endif          
          else
          {
            logit("e", "ReadConfig: Unknown \"staLoc\": <%s>\n", str);
            err = -1;
          }
        }
        else
        {
          logit("e", "ReadConfig: \"staLoc\" missing argument\n");
          err = -1;
        }
      }

      /* minStationsMl: NOT required */
      else if (k_its("minStationsMl") )
      {
        plmParams->minStationsMl = k_int();
      }

      /* require2Horizontals: NOT required */
      else if (k_its("require2Horizontals") )
      {
        plmParams->require2Horizontals = k_int();
        if (k_err() != 0) {
            plmParams->require2Horizontals = 1;
        }
      }
      else if (k_its("allowVerticals") )
      {
        plmParams->allowVerticals = k_int();
        if (k_err() != 0) {
            plmParams->allowVerticals = 1;
        }
      }
      /* OPTIONAL */
      else if (k_its("saveSCNL") )
      {
        plmParams->saveSCNL = k_int();
      }

      /* eventXML: NOT required */
      else if (k_its("eventXML") )
      {
        plmParams->eventXML = k_int();
      }
      /* saveXMLdir: NOT required */
      else if (k_its("saveXMLdir") )
      {
        if ( (str = k_str()) )
        {
          if ( (plmParams->saveXMLdir = strdup(str)) == (char *)NULL)
          {
            logit("e", "readConfig: out of memory for saveXMLdir\n");
            return -1;
          }
        }
 	plmParams->eventXML = 1; 	/* automagically turn it on */
      }
      else if (k_its("ResponseInMeters") )
        setResponseInMeters( 1 );
      else if (k_its("useMedian") )
      {
        plmParams->useMedian = 1;
      }
      else if (k_its("MlmsgOutDir") )
      {
        if ( (str = k_str()) )
          {
             if ( (plmParams->MlmsgOutDir = strdup(str)) == (char *)NULL)
             {
                logit("e", "readConfig: out of memory for MlmsgOutDir\n");
                return -1;
              }
          }
      }
      /* LookAtVersion: NOT required */
      else if (k_its("LookAtVersion") )
      {
        if ( (str = k_str()) )
        {
	    if(strcmp(str, "All") == 0 ) {
		plmParams->LookAtVersion = vAll;   /* Look at all versions of all events */
		logit("t", "readConfig: looking at all versions of the events.\n");
	    } else if(strcmp(str, "Prelim") == 0 ) {
		plmParams->LookAtVersion = vPrelim;   /* Look only at version Prelim of all events */
		logit("t", "readConfig: looking only at version Prelim of the events.\n");
	    } else if(strcmp(str, "Rapid") == 0 ) {
		plmParams->LookAtVersion = vRapid;   /* Look only at version Rapid of all events */
		logit("t", "readConfig: looking only at version Rapid of the events.\n");
	    } else if(strcmp(str, "Final") == 0 ) {
		plmParams->LookAtVersion = vFinal;   /* Look only at version Final of all events */
		logit("t", "readConfig: looking only at version Final of the events.\n");
	    } else {
		logit("e", "readConfig: value for optional parameter LookAtVersion is not valid. Possible values are the string: All, Prelim, Rapid or Final.\n");
		return -1;
	    }
        }
      }

      
      /* MaxSta: required */
      else if (k_its("maxSta") )
      {
        plmParams->maxSta = k_int();
        /* tell lm_site.c about max size */
        set_maxsite( plmParams->maxSta );
        init[1] = 1;
      }
      
      /* MaxDist: required */
      else if (k_its("maxDist") )
      {
        plmParams->maxDist = k_val();
        init[2] = 1;
      }
      
      /* maxTrace: required */
      else if (k_its("maxTrace") )
      {
        plmParams->maxTrace = (long)k_int();
        init[3] = 1;
      }
      
      /* SgSpeed: required */
      else if (k_its("SgSpeed") )
      {
        plmParams->SgSpeed = k_val();
        if (plmParams->SgSpeed < 0.5) {
          logit("e", "ReadConfig: unreasonably small SgSpeed: %lf\n", 
                plmParams->SgSpeed);
          err = 1;
        }
        init[4] = 1;
      }

      /* Trace length (time): optional */
      else if (k_its("traceTimes") )
      {
        plmParams->traceStart = k_val();
        plmParams->traceEnd = k_val();
      }
      
      /* Peak Search window parameters: optional */
      else if (k_its("searchTimes") )
      {
        plmParams->peakSearchStart = k_val();
        plmParams->peakSearchEnd = k_val();
      }
      
      /* Search Start Phase: optional */
      else if (k_its("searchStartPhase") )
      {
        if ( (str = k_str()) != (char *)NULL)
        {
          switch(str[0]) 
          {
          case 'P':
            plmParams->searchStartPhase = LM_SSP_P;
            break;
          case 'S':
            plmParams->searchStartPhase = LM_SSP_S;
            break;
          default:
            logit("e", 
                  "ReadConfig: bad value for searchStartPhase: $s\n", str);
            err = -1;
            break;
          }
        }
        else
        {
          logit("e",
                "ReadConfig: \"searchStartPhase\" missing phase name\n");
          err = -1;
        }
        
      }
      
      /* Sliding window width: optional */
      else if (k_its("slideLength") )
        plmParams->slideLength = k_val();
      
      else if (k_its("extraDelay") )
      {
        plmParams->waitTime = k_val();
      }
      else if (k_its("waitNow") )
      {
        plmParams->waitNow = 1;
      }

      /* Zero-to-peak threshold */
      else if (k_its("z2pThresh") )
        plmParams->z2pThresh = k_val();
      
      /* Mean of component magnitudes of amplitudes? */
      else if (k_its("meanCompMags") )
        plmParams->fMeanCompMags = TRUE;
      
      /* LogA0 file: required */
      else if (k_its("logA0") )
      {
        if ( (str = k_str()) != (char *)NULL)
        {
          if ( (plmParams->loga0_file = strdup(str)) == NULL)
          {
            logit("e", 
                  "ReadConfig: out of memory for logA0 filename\n");
            return -1;
          }
        }
        else
        {
          logit("e",
                "ReadConfig: \"logA0\" missing filename\n");
          err = -1;
        }
        init[5] = 1;
      }

      /* SCNL Selectors: optional */
      else if (k_its("Add") )
      {
        if ( (str = k_str()) )  /* sta */
        {
          if ( (newSel = (SCNLSEL *)calloc(1, sizeof(SCNLSEL))) == NULL)
          {
            logit("e", "ReadConfig: out of memory for SCNLSEL\n");
            return -1;
          }
          strncpy(newSel->sta, str, 6);
          if ( (str = k_str()) )  /* comp */
          {
            strncpy(newSel->comp, str, 8);
            if ( (str = k_str()) )  /* net */
            {
              strncpy(newSel->net, str, 8);
              if ( (str = k_str()) )  /* loc */
              {
               strncpy(newSel->loc, str, 3);
               if (plmParams->pAdd == (SCNLSEL *)NULL)
               {
                 plmParams->pAdd = newSel;
                 pAdd = newSel;   /* Leave it pointing at the end of list */
               }
               else
               {
                 pAdd->next = newSel;
                 pAdd = newSel;
               }
              }
	      else
              {
                logit("e", "ReadConfig: \"Add\" missing 1 of 5 arguments\n");
                err = -1;
                free(newSel);
              }
            }
            else
            {
              logit("e", "ReadConfig: \"Add\" missing 2 of 5 arguments\n");
              err = -1;
              free(newSel);
            }
          }
          else
          {
            logit("e", 
                  "ReadConfig: \"Add\" missing 3 of 5 arguemtns\n");
            err = -1;
            free(newSel);
          }
        }
        else
        {
          logit("e", "ReadConfig: \"Add\" missing 4 of 5 arguments\n");
          err = -1;
        }
      }

      /* SCNL Deleteions: optional */
      else if (k_its("Del") )
      {
        if ( (str = k_str()) )  /* sta */
        {
          if ( (newSel = (SCNLSEL *)calloc(1, sizeof(SCNLSEL))) == NULL)
          {
            logit("e", "ReadConfig: out of memory for SCNLSEL\n");
            return -1;
          }
          strncpy(newSel->sta, str, 6);
          if ( (str = k_str()) )  /* comp */
          {
            strncpy(newSel->comp, str, 8);
            if ( (str = k_str()) )  /* net */
            {
              strncpy(newSel->net, str, 8);
              if ( (str = k_str()) )  /* loc */
              {
                strncpy(newSel->loc, str, 3);
                if (plmParams->pDel == (SCNLSEL *)NULL)
                {
                  plmParams->pDel = newSel;
                  pDel = newSel;   /* Leave it pointing at the end of list */
                }
                else
                {
                  pDel->next = newSel;
                  pDel = newSel;
                }
              }
              else
              {
                logit("e", "ReadConfig: \"Del\" missing 1 of 4 arguments\n");
                err = -1;
                free(newSel);
              }
            }
            else
            {
              logit("e", "ReadConfig: \"Del\" missing 2 of 4 arguments\n");
              err = -1;
              free(newSel);
            }
          }
          else
          {
            logit("e", 
                  "ReadConfig: \"Del\" missing 3 of 4 arguemtns\n");
            err = -1;
            free(newSel);
          }
        }
        else
        {
          logit("e", "ReadConfig: \"Del\" missing 4 arguments\n");
          err = -1;
        }
      }

      /* Response Source: optional */
      else if (k_its("respSource") )
      {
        if ( (str = k_str()) )
        {
          if (k_its("SAC") )
          {
            if ( (str = k_str()) )
            {
              if ( (plmParams->respNameFormat = strdup(str)) == NULL)
              {
                logit("e", "ReadConfig: out of memory for respNameFormat\n");
                return -1;
              }
              plmParams->respSource = LM_RS_SAC;
            }
            else
            {
              logit("e", 
                      "ReadConfig: \"respSource SAC\" missing pz-filename-format\n");
              err = -1;
            }
          }
#ifdef EWDB
          else if (k_its("EWDB") )
          {
            plmParams->respSource = LM_RS_EWDB;
          }
#endif
          else if (k_its("File") )
          {
            if ( (str = k_str()) )
            {
              if ( (plmParams->respDir = strdup(str)) == NULL)
              {
                logit("e", "ReadConfig: out of memory for respDir\n");
                return -1;
              }
              if ( (str = k_str()) )
              {
                if ( (plmParams->respNameFormat = strdup(str)) == NULL)
                {
                  logit("e", 
                          "ReadConfig: out of memory for respNameFormat\n");
                  free(plmParams->respDir);
                  plmParams->respDir = NULL;
                  return -1;
                }
                plmParams->respSource = LM_RS_FILE;
              }
              else
              {
                logit("e", 
                        "ReadConfig: \"respSource FILE\" missing pz-filename-format\n");
                err = -1;
              }
            }
            else
            {
              logit("e", 
                      "ReadConfig: \"respSource File\" missing 2 arguments\n");
              err = -1;
            }
          }
#ifdef UW
          else if (k_its("UW") )
            plmParams->respSource = LM_RS_UW;
#endif          
          else
          {
            logit("e", "ReadConfig: unknown \"respSource\" <%s>\n", str);
            err = -1;
          }
        }
        else
        {
          logit("e", "ReadConfig: \"respSource\" missing argument\n");
          err = -1;
        }
      }
      
      /* readAmpDirect flag: optional */
      else if (k_its("readAmpDirect") )
        plmParams->fGetAmpFromSource = TRUE;
      
      /* Wood-Anderson Coefficients: optional */
      else if (k_its("WoodAndersonCoefs") )
      {
        if ( (plmParams->pWA = 
              (WA_PARAMS *)calloc(1, sizeof(WA_PARAMS))) == NULL)
        {
          logit("e", "ReadConfig: out of memory for WA_PARAMS\n");
          return -1;
        }
        if ( (plmParams->pWA->period = k_val() ) <= 0.0)
        {
          logit("e", "ReadConfig: bad values for WoodAndersonCoefs\n");
          err = -1;
        }
        if ( (plmParams->pWA->damp = k_val() ) <= 0.0)
        {
          logit("e", "ReadConfig: bad values for WoodAndersonCoefs\n");
          err = -1;
        }
        if ( (plmParams->pWA->gain = k_val() ) <= 0.0)
        {
          logit("e", "ReadConfig: bad values for WoodAndersonCoefs\n");
          err = -1;
        }
      }
      
      /* Save Trace: optional */
      else if (k_its("saveTrace") )
      {
        if (plmParams->saveTrace == LM_UNDEF)
        {
          if ( (str = k_str()) )
          {
            if (k_its("None") )
              plmParams->saveTrace = LM_ST_NO;
            else if (k_its("SAC") )
            {
              plmParams->saveTrace = LM_ST_SAC;
              if ( (str = k_str()) )
              {
                if ( (plmParams->sacOutDir = strdup(str)) == (char *)NULL)
                {
                  logit("e", "readConfig: out of memory for sacOutDir\n");
                  return -1;
                } 
                if ( (str = k_str()) )
                {
                  if ( (plmParams->saveDirFormat = strdup(str)) == 
                       (char *)NULL)
                  {
                    logit("e", "readConfig: out of memory for saveDirFormat\n");
                    return -1;
                  }
                  if ( (str = k_str()) )
                  {
                    if ( (plmParams->saveNameFormat = strdup(str)) == NULL)
                    {
                      logit("e", 
                            "ReadConfig: out of memory for saveNameFormat\n");
                      return -1;
                    }
                  }
                  else
                  {
                    logit("e", 
                          "ReadConfig: \"saveTrace SAC\" missing 1 argument\n");
                    err = -1;
                  }
                }
                else
                {
                  logit("e",
                        "ReadConfig: \"saveTrace SAC\" missing 2 arguments\n");
                  err = -1;
                }
              }
              else
              {
                logit("e",
                      "ReadConfig: \"saveTrace SAC\" missing 3 arguments\n");
                err = -1;
              }
            }
#ifdef UW
            else if (k_its("UW") )
            {
              plmParams->saveTrace = LM_ST_UW;
            }
#endif
            else
            {
              logit("e", "ReadConfig: unknown \"saveTrace\" arg <%s>\n", str);
              err = -1;
            }
          }
          else
          {
            logit("e", "ReadConfig: \"saveTrace\" missing argument\n");
            err = -1;
          }
        }
      }
      

      /* output format: optional */
      else if (k_its("outputFormat"))
      {
        if ( (str = k_str()) )
        {
          if (k_its("LM") ) {
            plmParams->outputFormat = LM_OM_LM;
	  }
          else if (k_its("File") )
          {
            plmParams->outputFormat = LM_OM_FL;

            /* Retrieve file name */
            if ( (str = k_str()) )
            {
              if ( (plmParams->outputFile = strdup(str)) == (char *)NULL)
              {
                logit("e", "readConfig: out of memory for outputFile\n");
                return -1;
              } 
            }
          }
#ifdef EWDB
          else if (k_its("EWDB") )
            plmParams->outputFormat = LM_OM_EWDB;
#endif
#ifdef UW
          else if (k_its("UW") )
            plmParams->outputFormat = LM_OM_UW;
#endif
          else
          {
            logit("e", "ReadConfig: unknow output format <%s>\n", str);
            err = -1;
          }
        }
        else
        {
          logit("e", "ReadConfig: \"outputFormat\" missing argument\n");
          err = -1;
        }
      }
      

      /* SCNL Parameters: optional */
      else if (k_its("SCNLpar") )
      {
        if (plmParams->numSCNLPar >= plmParams->maxSCNLPar)
        {
          plmParams->maxSCNLPar += 10;
          if ( (plmParams->pSCNLPar = 
                (SCNLPAR *)realloc(plmParams->pSCNLPar, sizeof(SCNLPAR) *
                                  plmParams->maxSCNLPar)) == (SCNLPAR *)NULL)
          {
            logit("e", "ReadConfig: out of memory for %d SCNLPARs\n",
                  plmParams->maxSCNLPar);
            return -1;
          }
        }
        if ( (str = k_str()) == NULL)
        {
          logit("e", "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(plmParams->pSCNLPar[plmParams->numSCNLPar].sta, str, 
                TRACE_STA_LEN);
        if ( (str = k_str()) == NULL)
        {
          logit("e", "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(plmParams->pSCNLPar[plmParams->numSCNLPar].comp, str, 
                TRACE_CHAN_LEN);
        if ( (str = k_str()) == NULL)
        {
          logit("e", "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(plmParams->pSCNLPar[plmParams->numSCNLPar].net, str,
                TRACE_NET_LEN);
        if ( (str = k_str()) == NULL)
        {
          logit("e", "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(plmParams->pSCNLPar[plmParams->numSCNLPar].loc, str,
                TRACE_LOC_LEN);
        plmParams->pSCNLPar[plmParams->numSCNLPar].magCorr = k_val();
        plmParams->pSCNLPar[plmParams->numSCNLPar].fTaper[0] = k_val();
        plmParams->pSCNLPar[plmParams->numSCNLPar].fTaper[1] = k_val();
        plmParams->pSCNLPar[plmParams->numSCNLPar].fTaper[2] = k_val();
        plmParams->pSCNLPar[plmParams->numSCNLPar].fTaper[3] = k_val();
        plmParams->pSCNLPar[plmParams->numSCNLPar].clipLimit = k_val();
        plmParams->numSCNLPar++;
      }
        
#ifdef EWDB
      /* Earthworm Database access: optional */
      else if (k_its("EWDBaccess") )
      {
        if ( (str = k_str()) )
        {
          if ( (plmParams->pDBaccess = 
                (DBACCESS *)calloc(1, sizeof(DBACCESS))) == NULL)
          {
            logit("e", "ReadConfig: out of memory for DBACCESS\n");
            return -1;
          }
          if ( (plmParams->pDBaccess->user = strdup(str)) == NULL)
          {
            logit("e", "ReadConfig: out of memory for DBA user\n");
            return -1;
          }
          if ( (str = k_str()) )
          {
            if ( (plmParams->pDBaccess->pwd = strdup(str)) == NULL)
            {
              logit("e", "ReadConfig: out of memory for DBA password\n");
              return -1;
            }
            if (  (str = k_str()) )
            {
              if ( (plmParams->pDBaccess->service = strdup(str)) == NULL)
              {
                logit("e", "ReadConfig: out of memory for DBA service\n");
                return -1;
              }
            }
            else
            {
              logit("e", 
                      "ReadConfig: \"EWDBaccess\" missing arguments; 3 required\n");
              err = -1;
            }
          }
          else
          {
            logit("e", 
                    "ReadConfig: \"EWDBaccess\" missing arguments; 3 required\n");
            err = -1;
          }
        }
        else
        {
          logit("e", 
                  "ReadConfig: \"EWDBaccess\" missing arguments; 3 required\n");
          err = -1;
        }
      }
#endif      

      /* Earthworm transport stuff: optional */
      else if (k_its("HeartBeatInterval") )
      {
        plmParams->HeartBeatInterval = k_int();
      }
      else if (k_its("RingInName") )
      {
        if ((str = k_str ()) != NULL)
        {
          if( ( pEW->RingInKey = GetKey(str) ) == -1 ) 
          {
            logit("e", "ReadConfig:  Invalid input ring name <%s>", str);
            err = -1;
          }
          plmParams->fEWTP = 1;
        }
        else
        {
          logit("e", "ReadConfig: \"RingInName\" missing argument\n");
          err = -1;
        }
      }

      else if (k_its("RingOutName") )
      {
        if ((str = k_str ()) != NULL)
        {
          if( ( pEW->RingOutKey = GetKey(str) ) == -1 ) 
          {
            logit("e", "ReadConfig:  Invalid output ring name <%s>", str);
            err = -1;
          }
        }
        else
        {
          logit("e", "ReadConfig: \"RingOutName\" missing argument\n");
          err = -1;
        }
      }

      else if (k_its ("MyModId")) 
      {
        if ((str = k_str ()) != NULL)
        {
          if ( GetModId( str, &pEW->hrtLogo.mod ) != 0 ) 
          {
            logit("e", "ReadConfig: Invalid module name <%s>\n", str );
            return( -1 );
          }
          if ( GetLocalInst( &pEW->hrtLogo.instid ) != 0 ) 
          {
            logit("e", "ReadConfig: error getting local installation id\n" );
            return( -1 );
          }
          if ( GetType( "TYPE_HEARTBEAT", &pEW->hrtLogo.type ) != 0 ) 
          {
            logit("e", 
          "ReadConfig: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
            return( -1 );
          }
          pEW->errLogo.mod = pEW->hrtLogo.mod;
          pEW->errLogo.instid = pEW->hrtLogo.instid;
          if ( GetType( "TYPE_ERROR", &pEW->errLogo.type ) != 0 ) 
          {
            logit("e", "ReadConfig: Invalid message type <TYPE_ERROR>\n" );
            return( -1 );
          }

          pEW->magLogo.mod =  pEW->hrtLogo.mod;
          pEW->magLogo.instid =  pEW->hrtLogo.instid;
          if ( GetType( "TYPE_MAGNITUDE", &pEW->magLogo.type ) != 0)
          {
            logit("e", "ReadConfig: Invalid msg type <TYPE_MAGNITUDE>\n" );
            return( -1 );
          }
          pEW->noMagLogo.mod =  pEW->hrtLogo.mod;
          pEW->noMagLogo.instid =  pEW->hrtLogo.instid;
          if ( GetType( "TYPE_NOMAGNITUDE", &pEW->noMagLogo.type ) != 0)
          {  /* TYPE_NOMAGNITUDE not found; use TYPE_ERROR instead */
            logit("e",
                "ReadConfig Warning: Msg type TYPE_NOMAGNITUDE not found; using TYPE_ERROR\n" );
            pEW->noMagLogo.type = pEW->errLogo.type;
          }
        }
      }
      else if (k_its("getEventsFrom") )
      {
        MSG_LOGO *tLogo = NULL;
        tLogo = (MSG_LOGO *)realloc( pEW->GetLogo, (pEW->nGetLogo+1) * 
                                     sizeof(MSG_LOGO));
        if( tLogo == NULL )
        {
          logit("e", "ReadConfig: getEventsFrom: error reallocing"
                " %d bytes; exiting!\n",
                (pEW->nGetLogo+1)*sizeof(MSG_LOGO) );
          return( -1 );
        }
        pEW->GetLogo = tLogo;
        if ((str = k_str ()) != NULL)
        {
          if ( GetInst( str, &(pEW->GetLogo[pEW->nGetLogo].instid) ) != 0 )
          {
            logit("e", "ReadConfig: getEventsFrom: invalid installation `%s'\n",
                  str );
            return( -1 );
          }
        }
        if( ( str = k_str() ) != NULL )
        {
          if ( GetModId( str, &(pEW->GetLogo[pEW->nGetLogo].mod) ) != 0 )
          {
            logit("e", "ReadConfig: getEventsFrom: invalid module id `%s'\n",
                  str );
            return( -1 );
          }
        }
        if ( (str = k_str()) == NULL )
        {
           k_err(); /* clear the error */
           /*
           ** Old-style line without TYPE_XXXXX, assumes TYPE_HYP2000ARC
           */
           if ( GetType( "TYPE_HYP2000ARC", &(pEW->GetLogo[pEW->nGetLogo].type) ) != 0 )
           {
             logit("e", "ReadConfig: getEventsFrom: invalid msgtype `%s'\n", 
                   "TYPE_HYP2000ARC");
             return( -1 );
           }
        }
        else
        {
           if ( GetType( str, &(pEW->GetLogo[pEW->nGetLogo].type) ) != 0 )
           {
             logit("e", "ReadConfig: getEventsFrom: invalid msgtype `%s'\n", 
                   str);
             return( -1 );
           }
        }
        pEW->nGetLogo++;
      }
        
      /* Optional: wave_server timeout */
      else if (k_its("wsTimeout") )
      {
        plmParams->wsTimeout = k_int();
      }
      
      /* Optional: log switch */
      else if (k_its ("LogFile"))
      {
        *logSwitch = k_int();
      }

      /* Optional: debug command */
      else if (k_its( "Debug") )
      {
        plmParams->debug |= k_int();
      }
      
      /* Optional: SkipStationsNotInArc */
      else if (k_its( "SkipStationsNotInArc") )
      {
	plmParams->SkipStationsNotInArc = 1;
      }

      /* Optional: MinWeightPercent */
      else if (k_its( "MinWeightPercent") )
      {
        plmParams->MinWeightPercent = k_val();
        if (plmParams->MinWeightPercent < 0.0  ||  plmParams->MinWeightPercent > 99.0 ) {
          logit("e", "ReadConfig: MinWeightPercent: %lf is not in range [0..99]\n", 
                plmParams->MinWeightPercent);
          err = 1;
        }
      }

      /* Optional: MLQpar1 */
      else if (k_its( "MLQpar1") )
      {
        plmParams->MLQpar1 = k_val();
        if (plmParams->MLQpar1 < 0.0) {
          logit("e", "ReadConfig: MLQpar1: %lf has to be greater than zero. To disable the computation of the quality of magnitude do not declare MLQpar1.\n", 
                plmParams->MLQpar1);
          err = 1;
        }
      }

      else if( t_com()      ) processor = "t_com";

      /* Unknown command */ 
      else 
      {
        logit("e", "ReadConfig: <%s> Unknown command in <%s>.\n", 
                 com, configfile);
        continue;
      }

      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        logit("e", 
                 "localmag: Bad <%s> command in <%s>\n\t%s\n",
              processor, configfile, k_com());
        return -1;
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
    logit("e", "localmag: ERROR, no ");
    if (!init[0])  logit("e", "<staLoc> ");
    if (!init[1])  logit("e", "<maxSta> ");
    if (!init[2])  logit("e", "<maxDist> ");
    if (!init[3])  logit("e", "<maxTrace> ");
    if (!init[4])  logit("e", "<SgSpeed> ");
    if (!init[5])  logit("e", "<logA0> ");
    
    logit("e", "command(s) in <%s>; exitting!\n", configfile);
    return -1;
  }
  return err;
}




/*
 * InitConfig: initialize the PARAMS structure. These initial values are
 * tested against in ReadConfig to see if command-line settings have
 * already been made. The command-line must be parsed before the config file
 * is read, so localmag can figure out which config file to read.
 */
static void InitConfig( LMPARAMS *plmParams )
{
  plmParams->maxDist = 0.0;        /* required param, so no default */
  plmParams->waitTime = 0.0;
  plmParams->waitNow = 0;	/* 0 wait till waitTime after Origin Time, 1 = wait waitTime from now */
  plmParams->SgSpeed = 0.0;        /* required param, so no default */
  plmParams->searchStartPhase = LM_SSP_S;
  plmParams->peakSearchStart = 1.0;
  plmParams->peakSearchEnd = 45.0;
  plmParams->slideLength = 0.8;
  plmParams->traceStart = 5.0;
  plmParams->traceEnd = 60.0;  
  plmParams->z2pThresh = 3.0;
  plmParams->pDBaccess = NULL;
  plmParams->pAdd = NULL;
  plmParams->pDel = NULL;
  plmParams->pSCNLPar = NULL;
  plmParams->pWA = NULL;
  plmParams->pWSV = NULL;      /* assign default "servers" file later */
  plmParams->HeartBeatInterval = 0;
  plmParams->debug = 0;
  plmParams->fEWTP = 0;
  plmParams->fDist = LM_UNDEF;
  plmParams->fGetAmpFromSource = LM_UNDEF;
  plmParams->fMeanCompMags = FALSE;
  plmParams->fWAsource = LM_UNDEF;
  plmParams->eventSource = LM_UNDEF;
  plmParams->maxSCNLPar = 0;
  plmParams->maxSta = 0;
  plmParams->nLtab = 0;
  plmParams->numSCNLPar = 0;
  plmParams->outputFormat = LM_UNDEF;
  plmParams->respSource = LM_UNDEF;
  plmParams->saveTrace = LM_UNDEF;
  plmParams->staLoc = LM_UNDEF;
  plmParams->traceSource = LM_UNDEF;
  plmParams->wsTimeout = 5000;  
  plmParams->eventID = NULL;
  plmParams->outputNameFormat = NULL;
  plmParams->respDir = NULL;
  plmParams->respNameFormat = NULL;
  plmParams->sacInDir = NULL;
  plmParams->sacOutDir = NULL;
  plmParams->saveNameFormat = NULL;
  plmParams->sourceNameFormat = NULL;
  plmParams->staLocFile = NULL;
  plmParams->outputFile = NULL;
  plmParams->pEW->nGetLogo = 0;
  plmParams->pEW->GetLogo = (MSG_LOGO *)NULL;
  plmParams->pEW->RingInKey = 0l;
  plmParams->pEW->RingOutKey = 0l;
#ifdef UW
  plmParams->UWpickfile = NULL;
#endif
  plmParams->ChannelNumberMap[0]=0; /* map 1 2 3 channel cars to N E and Z respecitvely */
  
  return;
}

/*
 * ParseCommand: parse the command-line options and arguments.
 */
static void ParseCommand( LMPARAMS *plmParams, int argc, char **argv, 
                          char **commandFile, int *logSwitch)
{
  int iarg = 1;
  
  while (iarg < argc && argv[iarg][0] == '-')
  {
    switch(argv[iarg][1])
    {
    case 'e':   /* Event source */
      switch(argv[iarg][2])
      {
      case 'h':   /* Hypoinverse archive file */
        plmParams->eventSource = LM_ES_ARCH;
        break;
      case 's':   /* SAC file */
        plmParams->eventSource = LM_ES_SAC;
        break;
#ifdef EWDB
      case 'e':   /* earthworm database */
        plmParams->eventSource = LM_ES_EWDB;
        break;
#endif
#ifdef UW
      case 'w':   /* UW-format pick file */
        plmParams->eventSource = LM_ES_UW;
        if (++iarg >= argc)
          usage(argv[0]);
        if (argv[iarg][0] == '-')
        {
          fprintf(stderr, 
                  "localmag: pickfile name missing after \"-ew\" flag\n");
          usage(argv[0]);
        }
        if ( (plmParams->UWpickfile = strdup( argv[iarg])) == NULL)
        {
          fprintf(stderr, "localmag: out of memory for pickfile name\n");
          usage(argv[0]);          
        }

        /* Make trace source default to UW;   *
         * cannot be overriden by config file */
        if (plmParams->traceSource == LM_UNDEF)
          plmParams->traceSource = LM_TS_UW;

        break;
#endif
      case ' ':
        fprintf(stderr, "localmag: missing event source after \"-e\"\n");
        usage(argv[0]);
        break;        
      default:
        fprintf(stderr, "localmag: unknown event source <%c>\n", 
                argv[iarg][2]);
        usage(argv[0]);        
      }
      break;
    case 't':   /* Trace Source */
      switch(argv[iarg][2])
      {
      case 's':   /* SAC file */
        plmParams->traceSource = LM_TS_SAC;
        if (argv[iarg][3] == 'W')
          plmParams->fWAsource = 1;

        if (++iarg >= argc)
          usage(argv[0]);
        if (argv[iarg][0] == '-')
        {
          fprintf(stderr, 
                  "localmag: SAC directory name missing after \"-ts\" flag\n");
          usage(argv[0]);
        }
        if ( (plmParams->sacInDir = strdup( argv[iarg])) == NULL)
        {
          fprintf(stderr, "localmag: out of memory for SAC directory\n");
          usage(argv[0]);
        }
        if (++iarg >= argc)
          usage(argv[0]);
        if (argv[iarg][0] == '-')
        {
          fprintf(stderr, 
                  "localmag: SAC filename format missing after \"-ts\" flag\n");
          usage(argv[0]);
        }
        if ( (plmParams->sourceNameFormat = strdup( argv[iarg])) == NULL)
        {
          fprintf(stderr, "localmag: out of memory for SAC filename format\n");
          usage(argv[0]);
        }
        break;
#ifdef EWDB
      case 'e':
        plmParams->traceSource = LM_TS_EWDB;
        plmParams->fWAsource = 1;  /* EWDB doesn't hold raw traces */
        break;
#endif
#ifdef UW
      case 'v':
        plmParams->traceSource = LM_TS_WS;
        break;
      case 'w':
        plmParams->traceSource = LM_TS_UW;
        if (argv[iarg][3] == 'W')
          plmParams->fWAsource = 1;
        break;
#endif
      case ' ':
        fprintf(stderr, "localmag: missing trace source after \"-t\"\n");
        usage(argv[0]);
        break;        
      default:
        fprintf(stderr, "localmag: unknown trace source <%c>\n", 
                argv[iarg][2]);
        usage(argv[0]);
      }
      break;
    case 's':   /* Save Traces */
      switch(argv[iarg][2])
      {
      case 'n':   /* Don't save traces */
        plmParams->saveTrace = LM_ST_NO;
        break;
      case 's':   /* SAC file */
        plmParams->saveTrace = LM_ST_SAC;
        if (++iarg >= argc)
          usage(argv[0]);
        if (argv[iarg][0] == '-')
        {
          fprintf(stderr, 
                  "localmag: SAC directory name missing after \"-ss\" flag\n");
          usage(argv[0]);
        }
        if ( (plmParams->sacOutDir = strdup( argv[iarg])) == NULL)
        {
          fprintf(stderr, "localmag: out of memory for SAC directory\n");
          usage(argv[0]);
        }
        if (++iarg >= argc)
          usage(argv[0]);
        if (argv[iarg][0] == '-')
        {
          fprintf(stderr, 
                  "localmag: SAC dir-name format missing after \"-ss\" flag\n");
          usage(argv[0]);
        }
        if ( (plmParams->saveDirFormat = strdup( argv[iarg])) == NULL)
        {
          fprintf(stderr, "localmag: out of memory for save dir name format\n");
          usage(argv[0]);
        }
        if (++iarg >= argc)
          usage(argv[0]);
        if (argv[iarg][0] == '-')
        {
          fprintf(stderr, 
                  "localmag: SAC filename format missing after \"-ss\" flag\n");
          usage(argv[0]);
        }
        if ( (plmParams->saveNameFormat = strdup( argv[iarg])) == NULL)
        {
          fprintf(stderr, "localmag: out of memory for SAC filename format\n");
          usage(argv[0]);
        }
        break;
#ifdef UW
      case 'w':  /* UW-format data files */
        plmParams->saveTrace = LM_ST_UW;
        if (++iarg >= argc)
          usage(argv[0]);
        if (argv[iarg][0] == '-')
        {
          fprintf(stderr, 
                  "localmag: UW filename format missing after \"-ss\" flag\n");
          usage(argv[0]);
        }
        if ( (plmParams->saveNameFormat = strdup( argv[iarg])) == NULL)
        {
          fprintf(stderr, "localmag: out of memory for UW filename format\n");
          usage(argv[0]);
        }
        break;
#endif
      case ' ':
        fprintf(stderr, "localmag: missing save trace flag after \"-s\"\n");
        usage(argv[0]);
        break;        
      default:
        fprintf(stderr, "localmag: unknown save trace flag <%c>\n", 
                argv[iarg][2]);
        usage(argv[0]);        
      }
      break;
    case 'l':  /* log switch value */
      *logSwitch = atoi(&argv[iarg][2]);
      break;
    case 'a':  /* Get amp direct from source */
      plmParams->fGetAmpFromSource = 1;
      break;
    default:
      fprintf(stderr, "localmag: unknow command flag <%s>\n", argv[iarg]);
      usage(argv[0]);
    }
    iarg++;
  }

  if (iarg < argc)
    *commandFile = argv[iarg];
  else
    usage(argv[0]);

  return;
}


static void usage( char *argv0 )
{
  fprintf(stderr, "localmag: version %s\n", LOCALMAG_VERSION);
#ifdef UW
  fprintf(stderr, "Usage: %s [-e[h | s | e | w <pickfile>] ]\n", argv0);
  fprintf(stderr, "\t[-t[s[W] <sac dir> <sac name format> | e | v | w[W] ] ]\n");
  fprintf(stderr, "\t[-s[n | s <base dir> <dir format> <file format> | w]\n");
#else
  fprintf(stderr, "Usage: %s [-e[h | s | e ] ]\n", argv0);
  fprintf(stderr, "\t[-t[s[W] <sac dir> <sac name format> | e] ]\n");
  fprintf(stderr, "\t[-s[n | s <base dir> <dir format> <file format>]\n");
#endif  
  fprintf(stderr, "\t[-a] [-l[0 | 1 | 2] config-file\n");

  exit (-1);
  return;  /* not really */
}
