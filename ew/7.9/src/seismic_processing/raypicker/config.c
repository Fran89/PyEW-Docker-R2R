
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c 4475 2011-08-04 15:17:09Z kevin $
 *
 *
 */

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* earthworm/hydra includes */
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <watchdog_client.h>

/* raypicker includes */
#include "raypicker.h"
#include "config.h"
#include "returncodes.h"

#define ncommand 6          /* Number of REQUIRED commands in the config file */

 /***********************************************************************
  *                 raypicker_default_params()                          *
  * Sets defaults for some configuration parameters                     *
  * Returns EW_SUCCESS upon completion      .                           *
  ***********************************************************************/
int raypicker_default_params(RParams *params)
{

    params->HeartBeatInterval = 60;       /* I'm alive every 60 seconds              */
    params->LogFile = 1;                  /* log to disk and stderr/stdout           */
    params->Debug = 0;                    /* No debug messages                       */
    params->PickIDFile[0] = '\0';         /* No filename                             */
    params->MaxSampleRate = 100.0;        /* Maximum sampling rate of 100 Hz         */
    params->MaxTriggerSecs = 600.0;       /* Maximum length of trigger of 10 minutes */
    params->MaxGapNoTriggerSecs = 10800.; /* Max gap when no trigger of 3 hour       */
    params->MaxGapInTriggerSecs = 3600.;  /* Max gap when a trigger exists of 1 hour */
    params->QueueSize = 500;              /* Max no of messages in queue             */
    params->MaxPreFilters = 20;           /* Max no of prefilters allowed            */
	params->SCNLFile[0] = '\0';           /* No filename                             */
	params->tUpdateInterval = 43200;      /* 12 hours                                */

    return EW_SUCCESS;
}

 /***********************************************************************
  *                       raypicker_config()                            *
  * Processes command file using kom.c functions.                       *
  * Returns EW_FAILURE if any errors are encountered.                   *
  ***********************************************************************/
int raypicker_config(char *config_file, RParams *params)
{
    char     init[ncommand];     /* Flags, one for each command */
    int      nmiss;              /* Number of commands that were missed */
    int      nfiles;
    int      i;
   
    /* Set to zero one init flag for each required command
     *****************************************************/
    for (i = 0; i < ncommand; i++)
      init[i] = 0;

   /* Open the main configuration file
    **********************************/
    nfiles = k_open(config_file);
    if (nfiles == 0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker_config: Error opening configuration file <%s>\n", config_file);
      return EW_FAILURE;
    }

	/* Process all nested configuration files
    ****************************************/
    while (nfiles > 0)                    /* While there are config files open */
    {
      while (k_rd())                      /* Read next line from active file  */
      {
          int  success;
          char *com;
          char *str;
    
          com = k_str();                  /* Get the first token from line */

          if (!com) continue;             /* Ignore blank lines */
          if (com[0] == '#') continue;    /* Ignore comments */

          /* Open another configuration file
           *********************************/
          if (com[0] == '@')
          {
              success = nfiles + 1;
              nfiles  = k_open(&com[1]);
              if (nfiles != success)
              {
                  reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Error opening command file <%s>.\n",
                        &com[1]);
                  return EW_FAILURE;
              }
              continue;
          }

          /* Read configuration parameters
           *******************************/
 /*0*/	  else if (k_its("MyModId"))
          {
              if ((str = k_str()) != NULL)
              {
                  if (GetModId(str, &params->MyModId) == -1)
                  {
                      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid MyModId <%s>.\n", str);
                      return EW_FAILURE;
                  }
              }
              init[0] = 1;
          }
 /*1*/	  else if (k_its("InRing")) 
          {
              if ((str = k_str()) != NULL)
              {
                  if ((params->InKey = GetKey(str)) == -1)
                  {
                      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid InRing name <%s>. Exiting.\n", str);
                      return EW_FAILURE;
                  }
              }
              init[1] = 1;
          }
 /*2*/    else if (k_its ("OutRing")) 
          {
              if ((str = k_str()) != NULL)
              {
                   if ((params->OutKey = GetKey(str)) == -1)
                   {
                       reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid OutRing name <%s>. Exiting.\n", str);
                       return EW_FAILURE;
                   }
              }
              init[2] = 1;
          }
 /*3*/	  else if (k_its("HeartBeatInterval")) 
          {
              params->HeartBeatInterval = k_long ();
              init[3] = 1;
          }
          else if (k_its("LogFile"))
              params->LogFile = k_int();
          else if (k_its("Debug"))
              params->Debug = k_int();
 /*4*/    else if (k_its("PickIDFile"))
          {
              if ((str = k_str()) != NULL)
              {
                   strcpy(params->PickIDFile, str);
                   init[4] = 1;
              }
          }
          else if (k_its("MaxSampleRate"))
              params->MaxSampleRate = k_val();
          else if (k_its("MaxTriggerSecs"))
              params->MaxTriggerSecs = k_val();
          else if (k_its("MaxGapNoTriggerSecs"))
              params->MaxGapNoTriggerSecs = k_val();
          else if (k_its("MaxGapInTriggerSecs"))
              params->MaxGapInTriggerSecs = k_val();
          else if (k_its("QueueSize"))
              params->QueueSize = k_int();
          else if (k_its("MaxPreFilters"))
              params->MaxPreFilters = k_int();
		  else if (k_its("SCNLUpdateInterval"))
			  params->tUpdateInterval = k_int();
 /*4*/    else if (k_its("SCNLFile"))
          {
              if ((str = k_str()) != NULL)
              {
                   strcpy(params->SCNLFile, str);
                   init[5] = 1;
              }
          }

          /* An unknown parameter was encountered
           **************************************/
          else
          {
              reportError(WD_WARNING_ERROR, NORESULT, "raypicker: <%s> unknown parameter in <%s>\n",
                  com, config_file);
              continue;
          }

          /* See if there were any errors processing the command
           *****************************************************/
          if (k_err())
          {
              reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Bad <%s> command in <%s>.\n", com,
                  config_file);
              return EW_FAILURE;
          }
      } /* while k_rd() */
      nfiles = k_close();
    }

    /* After all files are closed, check flags for missed commands
     *************************************************************/
    nmiss = 0;
    for (i = 0; i < ncommand; i++)
      if (!init[i])
          nmiss++;

    if (nmiss > 0)
    {
      if (!init[0]) reportError(WD_FATAL_ERROR, GENFATERR, 
                            "raypicker: ERROR, no <MyModId> command(s) in <%s>. Exiting.\n", config_file);
      if (!init[1]) reportError(WD_FATAL_ERROR, GENFATERR, 
                            "raypicker: ERROR, no <InRing> command(s) in <%s>. Exiting.\n", config_file);
      if (!init[2]) reportError(WD_FATAL_ERROR, GENFATERR, 
                            "raypicker: ERROR, no <OutRing> command(s) in <%s>. Exiting.\n", config_file);
      if (!init[3]) reportError(WD_FATAL_ERROR, GENFATERR, 
                            "raypicker: ERROR, no <HeartBeatInterval> command(s) in <%s>. Exiting.\n", config_file);
      if (!init[4]) reportError(WD_FATAL_ERROR, GENFATERR, 
                            "raypicker: ERROR, no <PickIDFile> command(s) in <%s>. Exiting.\n", config_file);
      if (!init[5]) reportError(WD_FATAL_ERROR, GENFATERR, 
                            "raypicker: ERROR, no <SCNLFile> command(s) in <%s>. Exiting.\n", config_file);
      return EW_FAILURE;
    }

    return EW_SUCCESS;
}

 /***********************************************************************
  *                           CheckgConfig()                            *
  *                                                                     *
  *  Ensure that the configuration parameters have acceptable values    *
  ***********************************************************************/
int CheckConfig(RParams parameters, int numSCNL)
{
    if (strlen(parameters.PickIDFile) >= PICK_FILELEN || strlen(parameters.PickIDFile) == 0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid PickIDFile name: %s\n", parameters.PickIDFile);
      return EW_FAILURE;
    }

    if (parameters.MaxSampleRate <= 0.0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid MaxSampleRate: %lf\n", parameters.MaxSampleRate);
      return EW_FAILURE;
    }

    if (parameters.MaxTriggerSecs <= 0.0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid MaxTriggerSecs: %lf\n", parameters.MaxTriggerSecs);
      return EW_FAILURE;
    }

    if (parameters.MaxGapNoTriggerSecs < 0.0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid MaxGapNoTriggerSecs: %lf\n", parameters.MaxGapNoTriggerSecs);
      return EW_FAILURE;
    }

    if (parameters.MaxGapInTriggerSecs < 0.0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid MaxGapInTriggerSecs: %lf\n", parameters.MaxGapInTriggerSecs);
      return EW_FAILURE;
    }

    if (parameters.QueueSize <= 0.0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid QueueSize: %d\n", parameters.QueueSize);
      return EW_FAILURE;
    }

    if (parameters.MaxPreFilters <= 0.0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Invalid MaxPreFilters: %d\n", parameters.MaxPreFilters);
      return EW_FAILURE;
    }

    if (parameters.HeartBeatInterval < 15)
    {
      parameters.HeartBeatInterval = 15;
      reportError(WD_WARNING_ERROR, NORESULT, "raypicker: Resetting HeartBeatInterval to 15 secs");
    }

    if (numSCNL <= 0)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: No SNCLs found. Nothing to do, so packing up my bags and going home.");
      reportError(WD_FATAL_ERROR, GENFATERR, " Please reconfigure me! /n");
      return EW_FAILURE;
    }

    return EW_SUCCESS;
}
 
/************************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  * Log the configuration parameters                                    *
  ***********************************************************************/
void LogConfig(RParams params, RaypickerSCNL *scnl, int num_SCNL)
{
    int i;

    logit("", "\n");
    logit("", "MyModId:             %6u\n",   params.MyModId);
    logit("", "InKey:               %6d\n",   params.InKey);
    logit("", "OutKey:              %6d\n",   params.OutKey);
    logit("", "HeartBeatInterval:   %6d\n",   params.HeartBeatInterval);   
    logit("", "LogFile:             %6d\n",   params.LogFile);
    logit("", "Debug:               %6d\n",   params.Debug);
    logit("", "PickIDFile:          %s\n",    params.PickIDFile);
    logit("", "MaxSampleRate:       %6f\n",   params.MaxSampleRate);
    logit("", "MaxTriggerSecs:      %6f\n",   params.MaxTriggerSecs);
    logit("", "MaxGapNoTriggerSecs: %6f\n",   params.MaxGapNoTriggerSecs);
    logit("", "MaxGapInTriggerSecs: %6f\n",   params.MaxGapInTriggerSecs);
    logit("", "QueueSize:           %6u\n",   params.QueueSize);
	logit("", "MaxPreFilters:       %6d\n\n", params.MaxPreFilters);
    logit("", "SCNLFile:            %s\n",    params.SCNLFile);
	logit("", "SCNL count:          %d\n",    num_SCNL);

    for (i = 0; i < num_SCNL; i++)
	{
      logit("", "SCNL: %s %s %s %s\n", scnl[i].sta, 
                 scnl[i].chan,scnl[i].net, scnl[i].loc);
	}
}
 
