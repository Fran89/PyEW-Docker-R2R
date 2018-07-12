/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_config.c 6487 2016-04-18 18:51:32Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.11  2009/08/25 00:10:40  paulf
 *     added extraDelay parameter go gmew
 *
 *     Revision 1.10  2007/05/15 17:32:42  paulf
 *     fixed qsort call (thanks to Ali M. from Utah for the catch
 *
 *     Revision 1.9  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.8  2002/09/25 17:34:56  dhanych
 *     added default value (3.0) for parameter snrThresh to initGM()
 *
 *     Revision 1.7  2002/02/28 17:03:36  lucky
 *     Moved gma.c and gma.h to libsrc and main include
 *
 *     Revision 1.6  2001/07/18 19:41:36  lombard
 *     Changed XMLDir, TempDir and MappingFile in GMPARAMS struct from string
 *     arrays to string pointers. Changed gm_config.c and gm_util.c to support thes
 *     changes. This solved a problem where the GMPARAMS structure was getting
 *     corrupted when a pointer to it was passed into getGMFromTrace().
 *     It's not clear why this was necessary; purify didn't complain.
 *
 *     Revision 1.5  2001/07/18 19:18:25  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2001/06/11 01:27:27  lombard
 *     cleanup
 *
 *     Revision 1.3  2001/06/10 21:27:36  lombard
 *     Changed single transport ring to input and output rings.
 *     Added ability to handle multiple getEventsFrom commands.
 *     Fixed handling of waveservers in config file.
 *
 *     Revision 1.2  2001/04/11 21:12:20  lombard
 *     ../localmag/site.h ../localmag/lm_site.h
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_config.c: routines to configure the gm module.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <kom.h>
#include <tlay.h>
#include <ws_clientII.h>
#include <gma.h>
#include "gm_sac.h"
#include "gm_util.h"
#include "gm_ws.h"
#include "gm_xml.h"
#include "../localmag/lm_site.h"

/* Internal Function Prototypes */
static void initGM(GMPARAMS *);
static int ReadConfig (GMPARAMS *, char * );

#ifdef _WINNT
/* Handle deprecation of strdup in windows */
static char* mystrdup( const char* src ) {
	char* dest = malloc( strlen(src) + 1 );
	if ( dest != NULL )
		strcpy( dest, src );
	return dest;
}
#else
#define mystrdup strdup
#endif

/*
 * Configure: do all the configuration of gmew.
 * This includes initializing the GMPARAMS structure, parsing the command-line
 * arguments, parsing the config file, and checking configuration settings
 * for consistency.
 *    Returns: 0 on success
 *            -1 on failure
 */
int Configure( GMPARAMS *pgmParams, int argc, char **argv, EVENT *pEvt, char *vsn)
{
  int rc = 0, logSwitch = 1;
  int lenT, lenX;
  char *configFile;
  char *str, servDir[GM_MAXTXT];
  
  /* Set initial values, then parse the command-line options */
  initGM( pgmParams );
  if (argc == 2)
    configFile = argv[1];
  else
  {
    fprintf(stderr, "Usage: %s config-file\n", argv[0]);
    fprintf(stderr, "   Version %s\n", vsn );
    exit( -1 );
  }
    
  /* Initialize the Earthworm logit system */
  logit_init(configFile, 0, MAX_BYTES_PER_EQ, logSwitch);

  /* Read the configuration file */
  if ( (rc = ReadConfig( pgmParams, configFile )) < 0)
    return rc;   /* Error; ReadConfig already complained */

  /* Set defaults if they weren't already set */
  if (pgmParams->traceSource == GM_UNDEF )
    pgmParams->traceSource = GM_TS_WS;
  if (pgmParams->saveTrace == GM_UNDEF)
    pgmParams->saveTrace = GM_ST_NO;

  /* Initialize the trace arrays.                                        */
  if ( (rc = initBufs( pgmParams->maxTrace )) < 0)
  {     /* initBufs already complained, so be silent here */
    return rc;
  }

  switch( pgmParams->traceSource)
  {
  case GM_TS_WS:
    if (pgmParams->pWSV == (WS_ACCESS *)NULL)
    {   /* Set the default traceSource to wave_servers listed in *
         * ${EW_PARAMS}/servers                                  */
      if ( (str = getenv("EW_PARAMS")) == NULL)
      {
        logit("e", "Configure: environment variable EW_PARAMS not defined\n");
        return -1;
      }
      if (strlen(str) > GM_MAXTXT - (strlen(DEF_SERVER) + 2))
      {
        logit("e", "Configure: environment variable EW_PARAMS too long;"
              " increase GM_MAXTXT and recompile\n");
        return -1;
      }
      sprintf(servDir, "%s/%s", str, DEF_SERVER);
      if ( (pgmParams->pWSV = 
            (WS_ACCESS *)calloc(1, sizeof(WS_ACCESS))) == 
           (WS_ACCESS *)NULL)
      {
        logit("e", "Configure: out of memory for WS_ACCESS\n");
        return -1;
      }
      if ( (pgmParams->pWSV->serverFile = mystrdup(servDir)) == NULL)
      {
        logit("e", "Configure: out of memory for serverFile\n");
        return -1;
      }
    }
    if (pgmParams->pWSV->pList == (SERVER *)NULL)
    {
      if (readServerFile(pgmParams) < 0)
        return -1;
    }
    if (initWsBuf(pgmParams->maxTrace) < 0)
    {
      logit("e", "Configure: out of memory for trace_buf buffer\n");
      return -1;
    }
    if (pgmParams->wsTimeout == 0)
      pgmParams->wsTimeout = 5000;  /* Default, 5 seconds */
    break;
#ifdef EWDB
  case GM_TS_EWDB:
    if (pgmParams->pDBaccess == (DBACCESS *)NULL)
    {
      logit("e", 
            "Configure: traceSource is EWDB but EWDBaccess not given\n");
      rc = -1;
    }
    break;
#endif
  default:
    logit("e", "Configure: unknown trace source <%d>.\n", 
          pgmParams->traceSource);
    rc = -1;
  }

  switch( pgmParams->respSource )
  {
  case GM_UNDEF:
    logit("e", "Configure: required response source not specified\n");
    rc = -1;
    break;
  case GM_RS_FILE:
    /* Nothing to do for this */
    break;
#ifdef EWDB
  case GM_RS_EWDB:
    if (pgmParams->pDBaccess == NULL)
    {
      logit("e", 
            "Configure: response source is EWDB but EWDBaccess not given\n");
      rc = -1;
    }
    break;
#endif
  default:
    logit("e", "Configure: unknown response source %d\n", 
          pgmParams->respSource);
    rc = -1;
  }

  /* Station Location Source: readConfig requires that it be set,
  	unless we're only responding to alarm messages */
  switch (pgmParams->staLoc)
  {
  case GM_UNDEF:
    break;
  case GM_SL_HYP:
    if ( site_read(pgmParams->staLocFile) < 0)
      rc = -1;
    break;
#ifdef EWDB
  case GM_SL_EWDB:
    if ( pgmParams->pDBaccess == (DBACCESS *)NULL)
    {
      logit("e", "Configure: staLoc is EWDB but EWDBaccess not given\n");
      rc = -1;
    }
    break;
#endif
  default:
    logit("e", "Configure: unknown staLoc <%d>\n", pgmParams->staLoc);
    rc = -1;
  }
  
  if (pgmParams->saveTrace == GM_ST_SAC)
  {  /* Initialize the SAC data array */
    if ( initSACBuf( pgmParams->maxTrace ) < 0)
    {
      logit("e", "Configure: out of memory for SAC data\n");
      return -1;
    }
  }

  lenT = lenX = 0;
  if (pgmParams->TempDir)
    lenT = strlen(pgmParams->TempDir);
  if (pgmParams->XMLDir)
    lenX = strlen(pgmParams->XMLDir);
  if ( lenT == 0 && lenX != 0)
  {
    logit("e", "XMLDir given but TempDir is missing\n");
    rc = -1;
  }

  if (rc == 0)
  {    /* Initialize the station list */
    if ( (pEvt->Sta = (STA *)calloc( pgmParams->maxSta, sizeof(STA))) == 
         (STA *)NULL)
    {
      logit("et", "Configure: out of memory for STA array\n");
      return -1;
    }

    /* Turn on some debugging options */
    if (pgmParams->debug & GM_DBG_WSC)
      (void)setWsClient_ewDebug(1);
  
    if (pgmParams->debug & (GM_DBG_PZG | GM_DBG_TRS | GM_DBG_ARS))
      transferDebug(pgmParams->debug >> 5);

    /* Sort the SCNLPAR array to make searching more efficient */
    if (pgmParams->numSCNLPar > 0)
      qsort(pgmParams->pSCNLPar, pgmParams->numSCNLPar, sizeof(SCNLPAR),
            CompareSCNLPARs);
    
    /* Initialize the XML mapping file */
    if (lenX > 0)
    {
      if (pgmParams->MappingFile && strlen(pgmParams->MappingFile) != 0)
      {
        if (initMappings( pgmParams->MappingFile ) < 0)
        {
          logit("et", "gmew: error initializing mappings; exitting!\n");
          rc = -1;
        }
      }
    }
  }
  
  return rc;
}



#define NUMREQ 9       /* Number of parameters that MUST be    */
/*   set from the config file.          */

/*      Function: ReadConfig                                            */
static int ReadConfig (GMPARAMS *pgmParams, char *configfile )
{
  char     init[NUMREQ];  /* init flags, one byte for each required command */
  int      nmiss;         /* number of required commands that were missed   */
  char     *com;
  char     *str;
  char     *processor;
  int      nfiles, rc;
  int      i;
  int      err = 0;
  double dummy;
  SCNLSEL   *newSel, *pAdd, *pDel;
  char configPath[GM_MAXTXT], *paramsDir;
  GMEW *pEW;
  
  
  pEW = pgmParams->pEW;

  pgmParams->waitTime=0; 	/* don't wait by default since this seems to work for many small networks */
  
  pgmParams->LookAtVersion = vAll;   /* Look at all versions of all events */

  pgmParams->alarmDuration=0; /* default: ignore alarm messages */
  pgmParams->allowDuplicates=0; /* default: ignore dup channel names (including location code diffs) */
  pgmParams->sendActivate=0; /* default: do not send ACTIVATE messages when done with XML files */
  
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
      fprintf(stderr, "gma: Error opening command file <%s>; EW_PARAMS not set\n", 
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
      fprintf(stderr, "gma: Error opening command file <%s> or <%s>\n", 
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
          fprintf(stderr, "gma: Error opening command file <%s>\n", 
                  &com[1]);
          return -1;
        }
        nfiles = rc;
        continue;
      }

      /* Station location source: required to process HYP2000ARC messages */
      else if (k_its("staLoc") )
      {
        if ( (str = k_str()) )
        {
          if (k_its("File") )
          {
            if ( (str = k_str()) )
            {
              if ( (pgmParams->staLocFile = mystrdup(str)) == NULL)
              {
                logit("e", "ReadConfig: out of memory for staLoc\n");
                return -1;
              }
              pgmParams->staLoc = GM_SL_HYP;
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
            pgmParams->staLoc = GM_SL_EWDB;
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
      
      /* MaxSta: required */
      else if (k_its("maxSta") )
      {
        pgmParams->maxSta = k_int();
        init[1] = 1;
      }
      
      /* MaxDist: required to process HYP2000ARC messages */
      else if (k_its("maxDist") )
      {
        pgmParams->maxDist = k_val();
        init[2] = 1;
      }
      
      /* maxTrace: required */
      else if (k_its("maxTrace") )
      {
        pgmParams->maxTrace = (long)k_int();
        init[3] = 1;
      }
      
      /* Trace length (time): optional */
      else if (k_its("traceTimes") )
      {
        pgmParams->traceStart = k_val();
        pgmParams->traceEnd = k_val();
      }
      
      /* Peak Search window parameters: optional */
      else if (k_its("searchWindow") )
      {
        pgmParams->peakSearchStart = k_val();
        pgmParams->peakSearchStartMin = k_val();
        pgmParams->peakSearchEnd = k_val();
        pgmParams->peakSearchEndMin = k_val();
      }
      
      /* SNR threshold */
      else if (k_its("snrThresh") )
        pgmParams->snrThresh = k_val();

      /* waitTime is in seconds to wait before processing an event */
      else if (k_its("extraDelay") )
      {
        pgmParams->waitTime = k_val();
      }
      
      /* LookAtVersion: optional */
      else if (k_its("LookAtVersion") )
      {
        if ( (str = k_str()) )
        {
	    if(strcmp(str, "All") == 0 ) {
		pgmParams->LookAtVersion = vAll;   /* Look at all versions of all events */
		logit("t", "readConfig: looking at all versions of the events.\n");
	    } else if(strcmp(str, "Prelim") == 0 ) {
		pgmParams->LookAtVersion = vPrelim;   /* Look only at version Prelim of all events */
		logit("t", "readConfig: looking only at version Prelim of the events.\n");
	    } else if(strcmp(str, "Rapid") == 0 ) {
		pgmParams->LookAtVersion = vRapid;   /* Look only at version Rapid of all events */
		logit("t", "readConfig: looking only at version Rapid of the events.\n");
	    } else if(strcmp(str, "Final") == 0 ) {
		pgmParams->LookAtVersion = vFinal;   /* Look only at version Final of all events */
		logit("t", "readConfig: looking only at version Final of the events.\n");
	    } else {
		logit("e", "readConfig: value for optional parameter LookAtVersion is not valid. Possible values are the string: All, Prelim, Rapid or Final.\n");
		return -1;
	    }
        }
      }

      /* SCNL Selectors: optional */
      else if (k_its("Add") )
      {
        if ( (str = k_str()) )  /* sta */
        {
          if ( (newSel = (SCNLSEL *)calloc(1, sizeof(SCNLSEL))) == NULL)
          {
            fprintf(stderr, "ReadConfig: out of memory for SCNLSEL\n");
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
               if (pgmParams->pAdd == (SCNLSEL *)NULL)
               {
                 pgmParams->pAdd = newSel;
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
                fprintf(stderr, "ReadConfig: \"Add\" missing 1 of 5 arguments\n");
                err = -1;
                free(newSel);
              }
            }
            else
            {
              fprintf(stderr, "ReadConfig: \"Add\" missing 2 of 5 arguments\n");
              err = -1;
              free(newSel);
            }
          }
          else
          {
            fprintf(stderr, 
                    "ReadConfig: \"Add\" missing 3 of 5 arguments\n");
            err = -1;
            free(newSel);
          }
        }
        else
        {
          fprintf(stderr, "ReadConfig: \"Add\" missing 4 of 5 arguments\n");
          err = -1;
        }
      }

      /* SCN Deleteions: optional */
      else if (k_its("Del") )
      {
        if ( (str = k_str()) )  /* sta */
        {
          if ( (newSel = (SCNLSEL *)calloc(1, sizeof(SCNLSEL))) == NULL)
          {
            fprintf(stderr, "ReadConfig: out of memory for SCNLSEL\n");
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
              	if (pgmParams->pDel == (SCNLSEL *)NULL)
                {
                  pgmParams->pDel = newSel;
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
              fprintf(stderr, "ReadConfig: \"Del\" missing 1 of 5 arguments\n");
              err = -1;
              free(newSel);
            }
          }
            else
            {
              fprintf(stderr, "ReadConfig: \"Del\" missing 2 of 5 arguments\n");
              err = -1;
              free(newSel);
            }
          }
          else
          {
            fprintf(stderr, 
                    "ReadConfig: \"Del\" missing 3 of 5 arguments\n");
            err = -1;
            free(newSel);
          }
        }
        else
        {
          fprintf(stderr, "ReadConfig: \"Del\" missing 4 arguments\n");
          err = -1;
        }
      }

      /* Trace Source: optional */
      else if (k_its( "traceSource" ))
      {
        if (pgmParams->traceSource == GM_UNDEF)
        {                 /* Let command-line take precedence */
          if ( (str = k_str()) )
          {
            if (k_its("waveServer"))
            {
              pgmParams->traceSource = GM_TS_WS;
              if ( (str = k_str()) )
              {
                if ( (pgmParams->pWSV = 
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
                    if ( (pgmParams->pWSV->serverFile = mystrdup(str)) == NULL)
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
                    if (Add2ServerList(str, pgmParams) < 0)
                      err = -1;  /* Add2ServerList already complained */
                    str = k_str();
                    /* We have to catch the kom error here since we are *
                     * intentionally trying to read to the end of the   *
                     * string.                                          */
                    ws_err = k_err();
                    if (ws_err == -17 && pgmParams->pWSV->pList != 
                        (PSERVER) NULL)
                      continue;
                    else if (ws_err < 0)
                    {
                      logit("e", 
                            "ReadConfig: Bad <%s> command in <%s>\n\t%s\n",
                            processor, configfile, k_com());
                      return -1;
                    }
                  }
                }
                /* else default waveServer file is "servers" in $EW_PARAMS dir */
              }
            }
          }
          else
          {
            logit("e", "ReadConfig: Missing traceSource argument\n");
            err = -1;
          }
        } /* else already set from command-line */
      }
      
      /* Response Source: optional */
      else if (k_its("respSource") )
      {
        if ( (str = k_str()) )
        {
          if (k_its("File") )
          {
            if ( (str = k_str()) )
            {
              if ( (pgmParams->respDir = mystrdup(str)) == NULL)
              {
                logit("e", "ReadConfig: out of memory for respDir\n");
                return -1;
              }
              if ( (str = k_str()) )
              {
                if ( (pgmParams->respNameFormat = mystrdup(str)) == NULL)
                {
                  logit("e", 
                        "ReadConfig: out of memory for respNameFormat\n");
                  free(pgmParams->respDir);
                  pgmParams->respDir = NULL;
                  return -1;
                }
                pgmParams->respSource = GM_RS_FILE;
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
#ifdef EWDB
          else if (k_its("EWDB") )
          {
            pgmParams->respSource = GM_RS_EWDB;
          }
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
      
      /* the responses are in meters, from rdseed -pf, convert them to nanometers when read in */
      else if (k_its("ResponseInMeters") )
        setResponseInMeters( 1 );

      /* Save Trace: optional */
      else if (k_its("saveTrace") )
      {
        if (pgmParams->saveTrace == GM_UNDEF)
        {
          if ( (str = k_str()) )
          {
            if (k_its("None") )
              pgmParams->saveTrace = GM_ST_NO;
            else if (k_its("SAC") )
            {
              pgmParams->saveTrace = GM_ST_SAC;
              if ( (str = k_str()) )
              {
                if ( (pgmParams->sacOutDir = mystrdup(str)) == (char *)NULL)
                {
                  logit("e", "readConfig: out of memory for sacOutDir\n");
                  return -1;
                } 
                if ( (str = k_str()) )
                {
                  if ( (pgmParams->saveDirFormat = mystrdup(str)) == 
                       (char *)NULL)
                  {
                    logit("e", "readConfig: out of memory for saveDirFormat\n");
                    return -1;
                  }
                  if ( (str = k_str()) )
                  {
                    if ( (pgmParams->saveNameFormat = mystrdup(str)) == NULL)
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
      

      /* SCNL Parameters: optional */
      else if (k_its("SCNLpar") )
      {
        if (pgmParams->numSCNLPar >= pgmParams->maxSCNLPar)
        {
          pgmParams->maxSCNLPar += 10;
          if ( (pgmParams->pSCNLPar = 
                (SCNLPAR *)realloc(pgmParams->pSCNLPar, sizeof(SCNLPAR) *
                                  pgmParams->maxSCNLPar)) == (SCNLPAR *)NULL)
          {
            fprintf(stderr, "ReadConfig: out of memory for %d SCNLPARs\n",
                    pgmParams->maxSCNLPar);
            return -1;
          }
        }
        if ( (str = k_str()) == NULL)
        {
          fprintf(stderr, "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(pgmParams->pSCNLPar[pgmParams->numSCNLPar].sta, str, 
                TRACE_STA_LEN);
        if ( (str = k_str()) == NULL)
        {
          fprintf(stderr, "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(pgmParams->pSCNLPar[pgmParams->numSCNLPar].comp, str, 
                TRACE_CHAN_LEN);
        if ( (str = k_str()) == NULL)
        {
          fprintf(stderr, "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(pgmParams->pSCNLPar[pgmParams->numSCNLPar].net, str,
                TRACE_NET_LEN);
        if ( (str = k_str()) == NULL)
        {
          fprintf(stderr, "ReadConfig: Bad \"SCNLPar\" command\n");
          return -1;
        }
        strncpy(pgmParams->pSCNLPar[pgmParams->numSCNLPar].loc, str,
                TRACE_LOC_LEN);
        dummy = k_val(); /* Throw away the station magnitude correction */
        pgmParams->pSCNLPar[pgmParams->numSCNLPar].fTaper[0] = k_val();
        pgmParams->pSCNLPar[pgmParams->numSCNLPar].fTaper[1] = k_val();
        pgmParams->pSCNLPar[pgmParams->numSCNLPar].fTaper[2] = k_val();
        pgmParams->pSCNLPar[pgmParams->numSCNLPar].fTaper[3] = k_val();
        pgmParams->pSCNLPar[pgmParams->numSCNLPar].clipLimit = k_val();
        pgmParams->pSCNLPar[pgmParams->numSCNLPar].taperTime = k_val();
        pgmParams->numSCNLPar++;
      }
        
#ifdef EWDB
      /* Earthworm Database access: optional */
      else if (k_its("EWDBaccess") )
      {
        if ( (str = k_str()) )
        {
          if ( (pgmParams->pDBaccess = 
                (DBACCESS *)calloc(1, sizeof(DBACCESS))) == NULL)
          {
            logit("e", "ReadConfig: out of memory for DBACCESS\n");
            return -1;
          }
          if ( (pgmParams->pDBaccess->user = mystrdup(str)) == NULL)
          {
            logit("e", "ReadConfig: out of memory for DBA user\n");
            return -1;
          }
          if ( (str = k_str()) )
          {
            if ( (pgmParams->pDBaccess->pwd = mystrdup(str)) == NULL)
            {
              logit("e", "ReadConfig: out of memory for DBA password\n");
              return -1;
            }
            if (  (str = k_str()) )
            {
              if ( (pgmParams->pDBaccess->service = mystrdup(str)) == NULL)
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

      /* Earthworm transport stuff */
      else if (k_its("HeartBeatInterval") )
      {
        pgmParams->HeartBeatInterval = k_int();
        init[4] = 1;        
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
          init[5] = 1;
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
          init[6] = 1;
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
            logit("e", "ReadConfig: Invalid module name <%s>", str );
            return( -1 );
          }
          if ( GetLocalInst( &pEW->hrtLogo.instid ) != 0 ) 
          {
            logit("e", "ReadConfig: error getting local installation id" );
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
            logit("e", "ReadConfig: Invalid message type <TYPE_ERROR>" );
            return( -1 );
          }

          pEW->gmLogo.mod =  pEW->hrtLogo.mod;
          pEW->gmLogo.instid =  pEW->hrtLogo.instid;
          if ( GetType( "TYPE_STRONGMOTIONII", &pEW->gmLogo.type ) != 0)
          {
            logit("e", "ReadConfig: Invalid msg type <TYPE_STRONGMOTIONII>" );
            return( -1 );
          }

          pEW->ha2kLogo.mod =  pEW->hrtLogo.mod;
          pEW->ha2kLogo.instid =  pEW->hrtLogo.instid;

          pEW->amLogo.mod =  pEW->hrtLogo.mod;
          pEW->amLogo.instid =  pEW->hrtLogo.instid;

          pEW->threshLogo.mod =  pEW->hrtLogo.mod;
          pEW->threshLogo.instid =  pEW->hrtLogo.instid;
        }
        init[7] = 1;
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
        if ( GetType( "TYPE_HYP2000ARC", &(pEW->GetLogo[pEW->nGetLogo].type) ) != 0 )
        {
          logit("e", "ReadConfig: getEventsFrom: invalid msgtype `%s'\n", 
                "TYPE_HYP2000ARC");
          return( -1 );
        }
        pEW->ha2kLogo.type = pEW->GetLogo[pEW->nGetLogo].type;
        pEW->nGetLogo++;
        init[8] = 1;
      }
        
      /* Optional: wave_server timeout */
      else if (k_its("wsTimeout") )
      {
        pgmParams->wsTimeout = k_int();
      }

      /* Optional XML stuff */
      else if (k_its( "XMLDir" ) )
      {
	if ( (str = k_str()) )
        {
          if ( (pgmParams->XMLDir = mystrdup(str)) == NULL)
          {
            logit("e", "ReadConfig: out of memory for XMLDir\n");
            return -1;
          }
        }
        else
        {
          fprintf( stderr, "gmew: Bad <XMLDir> command in <%s>;"
                   " exiting!\n", configfile );
          err = -1;
        }
      }
      else if (k_its( "TempDir" ) )
      {
	if ( (str = k_str()) )
        {
          if ( ( pgmParams->TempDir = mystrdup(str)) == NULL)
          {
            logit("e", "ReadConfig: out of memory for TempDir\n");
            return -1;
          }
        }
        else
        {
          fprintf( stderr, "gmew: Bad <TempDir> command in <%s>;"
                   " exiting!\n", configfile );
          err = -1;
        }
      }
      else if (k_its( "MappingFile" ) )
      {
	if ( (str = k_str()) )
        {
          if ( ( pgmParams->MappingFile = mystrdup(str)) == NULL)
          {
            logit("e", "ReadConfig: out of memory for MappingFile\n");
            return -1;
          }
        }
        else
        {
          fprintf( stderr, "gmew: Bad <MappingFile> command in <%s>;"
                   " exiting!\n", configfile );
          err = -1;
        }
      }
      
      /* Optional: debug command */
      else if (k_its( "Debug") )
      {
        pgmParams->debug |= k_int();
      }
      
      /* Optional: Respond to dup SCNs - note location code was ignored otherwise if multiple at a sta */
      else if (k_its( "allowDuplicates") )
      {
        pgmParams->allowDuplicates = 1;
      }
      /* Optional: send activation requests */
      else if (k_its( "sendActivateLogo") )
      {
      /* get the module ide to use for send activate */
        str = k_str();
    	if ( GetModId( str, &(pEW->amLogo.mod) ) != 0 )
        {
          logit("e", "ReadConfig: sendActivateLogo: invalid module  string %s\n", str);
          return( -1 );
        }
      }
      else if (k_its( "sendActivate") )
      {
        if ( GetType( "TYPE_ACTIVATE_MODULE", &(pEW->amLogo.type) ) != 0 )
        {
          logit("e", "ReadConfig: sendAlarm: invalid msgtype `TYPE_ACTIVATE_MODULE'\n");
          return( -1 );
        }
        pgmParams->sendActivate = 1;
      }
     /* Optional: Respond to thresh alarms */
     else if (k_its( "watchForThreshAlert") )
     {
       MSG_LOGO *tLogo = NULL;
       tLogo = (MSG_LOGO *)realloc( pEW->GetLogo, (pEW->nGetLogo+1) * 
                                    sizeof(MSG_LOGO));
       if( tLogo == NULL )
       {
         logit("e", "ReadConfig: watchForThreshAlert: error reallocing"
               " %d bytes; exiting!\n",
               (pEW->nGetLogo+1)*sizeof(MSG_LOGO) );
         return( -1 );
       }
       pEW->GetLogo = tLogo;
if ( GetInst( "INST_WILDCARD", &(pEW->GetLogo[pEW->nGetLogo].instid) ) != 0 )
{
  logit("e", "ReadConfig: watchForThreshAlert: invalid installation `INST_WILDCARD'\n");
  return( -1 );
}
   if ( GetModId( "MOD_WILDCARD", &(pEW->GetLogo[pEW->nGetLogo].mod) ) != 0 )
       {
         logit("e", "ReadConfig: watchForThreshAlert: invalid module id `MOD_WILDCARD'\n");
         return( -1 );
       }
       if ( GetType( "TYPE_THRESH_ALERT", &(pEW->GetLogo[pEW->nGetLogo].type) ) != 0 )
       {
         logit("e", "ReadConfig: watchForThreshAlert: invalid msgtype `TYPE_THRESH_ALERT'\n");
         return( -1 );
       }
       pEW->threshLogo.type = pEW->GetLogo[pEW->nGetLogo].type;
        str = k_str();
        if ( str ) {
        	pgmParams->threshDuration = atoi( str );
        	if ( pgmParams->threshDuration <= 0 ) {
        		logit("e", "ReadConfig: watchForThreshAlert: invalid duration '%s'; ignoring\n",
        			str );
        		pgmParams->threshDuration = 0;
        	}
        } else {
            k_err();    /* Clear the error of not finding a duration argument */
        	pgmParams->threshDuration = 20;
        }
       pEW->nGetLogo++;
     }
     /* Optional: Respond to activation requests */
      else if (k_its( "watchForAlarm") )
      {
        MSG_LOGO *tLogo = NULL;
        tLogo = (MSG_LOGO *)realloc( pEW->GetLogo, (pEW->nGetLogo+1) * 
                                     sizeof(MSG_LOGO));
        if( tLogo == NULL )
        {
          logit("e", "ReadConfig: watchForAlarm: error reallocing"
                " %d bytes; exiting!\n",
                (pEW->nGetLogo+1)*sizeof(MSG_LOGO) );
          return( -1 );
        }
        pEW->GetLogo = tLogo;
 	    if ( GetInst( "INST_WILDCARD", &(pEW->GetLogo[pEW->nGetLogo].instid) ) != 0 )
	    {
		  logit("e", "ReadConfig: watchForAlarm: invalid installation `INST_WILDCARD'\n");
		  return( -1 );
	    }
    	if ( GetModId( "MOD_WILDCARD", &(pEW->GetLogo[pEW->nGetLogo].mod) ) != 0 )
        {
          logit("e", "ReadConfig: watchForAlarm: invalid module id `MOD_WILDCARD'\n");
          return( -1 );
        }
        if ( GetType( "TYPE_ACTIVATE_MODULE", &(pEW->GetLogo[pEW->nGetLogo].type) ) != 0 )
        {
          logit("e", "ReadConfig: watchForAlarm: invalid msgtype `TYPE_ACTIVATE_MODULE'\n");
          return( -1 );
        }
        pEW->amLogo.type = pEW->GetLogo[pEW->nGetLogo].type;
        str = k_str();
        if ( str ) {
        	pgmParams->alarmDuration = atoi( str );
        	if ( pgmParams->alarmDuration <= 0 ) {
        		logit("e", "ReadConfig: watchForAlarm: invalid duration '%s'; ignoring\n",
        			str );
        		pgmParams->alarmDuration = 0;
        	}
        } else
        	pgmParams->alarmDuration = 60;
        pEW->nGetLogo++;
      }
      
      /* Optional: additional spectral response frequency */
      else if (k_its( "AddSpectraResponseAt") )
      {
      	addSR2list( k_val() );
	  }

      /* Optional: additional spectral response frequency */
      else if (k_its( "PreTriggerSeconds") )
      {
      	pgmParams->preTriggerSeconds = k_int();
	  }

      else if( t_com()      ) processor = "t_com";

      /* Unknown command */ 
      else 
      {
        fprintf(stderr, "ReadConfig: <%s> Unknown command in <%s>.\n", 
                com, configfile);
        continue;
      }

      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        fprintf(stderr, 
                "gma: Bad <%s> command in <%s>\n\t%s\n",
                processor, configfile, k_com());
        return -1;
      }

    } /** while k_rd() **/

    nfiles = k_close ();

  } /** while nfiles **/

  
  if ( pgmParams->alarmDuration ) {
  	/* WatchForAlarm present, so might not need to handle HYP2000ARC messages */
  	if ( !init[0] || !init[2] ) {
  		fprintf( stderr, "Warning: program cannot respond to HYP2000ARC messages:\n" );
  		if ( !init[0] )
			fprintf( stderr, "\tmissing <staLoc> command\n");
  		if ( !init[0] )
			fprintf( stderr, "\tmissing <maxDist> command\n");
		/* Make it look like these have been supplied */
		init[0] = init[2] = 1;
	}
  }
  
  /* After all files are closed, check init flags for missed commands */
  nmiss = 0;
  for (i = 0; i < NUMREQ; i++)  
    if (!init[i]) 
      nmiss++;

  if (nmiss) 
  {
    fprintf(stderr, "gma: ERROR, no ");
    if (!init[0])  fprintf(stderr, "<staLoc> ");
    if (!init[1])  fprintf(stderr, "<maxSta> ");
    if (!init[2])  fprintf(stderr, "<maxDist> ");
    if (!init[3])  fprintf(stderr, "<maxTrace> ");
    if (!init[4])  fprintf(stderr, "<HeartBeatInterval> ");
    if (!init[5])  fprintf(stderr, "<RingInName> ");
    if (!init[6])  fprintf(stderr, "<RingOutName> ");
    if (!init[7])  fprintf(stderr, "<MyModId> ");
    if (!init[8])  fprintf(stderr, "<getEventsFrom> ");
    
    fprintf(stderr, "command(s) in <%s>; exitting!\n", configfile);
    return -1;
  }

  /* Sort the SCNLPAR array to make searching more efficient */
  if (pgmParams->numSCNLPar > 0)
    qsort(pgmParams->pSCNLPar, pgmParams->numSCNLPar, sizeof(SCNLPAR), 
          CompareSCNLPARs);

  /* If no spectra periods defined, use the old hard-coded ones */
  checkSRlist();
  
  return err;
}

static void initGM(GMPARAMS *pgmParams)
{
  pgmParams->peakSearchStart = 0.0;
  pgmParams->peakSearchStartMin = 2.0;
  pgmParams->peakSearchEnd = 0.0;
  pgmParams->peakSearchEndMin = 30.0;
  pgmParams->traceStart = 5.0;
  pgmParams->traceEnd = 60.0;  
  pgmParams->pAdd = NULL;
  pgmParams->pDel = NULL;
  pgmParams->pSCNLPar = NULL;
  pgmParams->pWSV = NULL;      /* assign default "servers" file later */
  pgmParams->HeartBeatInterval = 0;
  pgmParams->debug = 0;
  pgmParams->maxSCNLPar = 0;
  pgmParams->maxSta = 0;
  pgmParams->numSCNLPar = 0;
  pgmParams->snrThresh = 3.0;
  pgmParams->respSource = GM_UNDEF;
  pgmParams->saveTrace = GM_UNDEF;
  pgmParams->staLoc = GM_UNDEF;
  pgmParams->traceSource = GM_UNDEF;
  pgmParams->wsTimeout = 5000;  
  pgmParams->eventID = NULL;
  pgmParams->respDir = NULL;
  pgmParams->respNameFormat = NULL;
  pgmParams->saveNameFormat = NULL;
  pgmParams->staLocFile = NULL;
  pgmParams->TempDir = NULL;
  pgmParams->XMLDir = NULL;
  pgmParams->MappingFile = NULL;
  pgmParams->pEW->nGetLogo = 0;
  pgmParams->pEW->GetLogo = (MSG_LOGO *)NULL;
  pgmParams->pEW->RingInKey = 0l;
  pgmParams->pEW->RingOutKey = 0l;
  pgmParams->pEW->ha2kLogo.type = 0;
  pgmParams->pEW->amLogo.type = 0;
  pgmParams->pEW->threshLogo.type = 0;
  pgmParams->preTriggerSeconds = 10;
  return;
}


