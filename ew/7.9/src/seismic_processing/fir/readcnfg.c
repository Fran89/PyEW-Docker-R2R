
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readcnfg.c 4013 2010-08-18 20:59:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/07/28 22:43:04  lombard
 *     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
 *
 *     Revision 1.2  2002/05/16 16:49:42  patton
 *     Made logit changes.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * readcnfg.c: Read the Fir parameters from a file.
 *              1) Set members of the WORLD structures.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
/*                                                                      */
/*      Inputs:         Pointer to a string(input filename)             */
/*                      Pointer to the Fir World structure         */
/*                                                                      */
/*      Outputs:        Updated Fir parameter structures(above)    */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in fir.h on          */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* strcpy                                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
                        /*   k_str, k_val                               */
/*******                                                        *********/
/*      Fir Includes                                            */
/*******                                                        *********/
#include "fir.h"

#define NUMREQ          9       /* Number of parameters that MUST be    */
                                /*   set from the config file.          */
#define STATION_INC     10      /* how many more are allocated each     */
                                /*   time we run out                    */

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
int ReadConfig (char *configfile, WORLD* pDcm)
{
  /* init flags, one byte for each required command */
  char     		init[NUMREQ];     
  char                  processor[15];
  int      		nmiss;
  char    		*com;
  char    		*str;
  int      		nfiles;
  int      		success;
  int      		i;
  STATION               *stations = NULL;  /* Array of STATION structures */
  int                   maxSta = 0;     /* number of STATIONs allocated   */
  int                   nSta = 0;       /* number of STATIONs used        */


  pDcm->firParam.QueueSize = 100; 		/* the old default value */
  pDcm->firParam.SleepMilliSeconds = 500; 	/* the old default value */
  /* Set to zero one init flag for each required command */

  for (i = 0; i < NUMREQ; i++)
    init[i] = 0;

  /* Open the main configuration file 
**********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    logit ("e",
             "fir: Error opening command file <%s>; exitting!\n", 
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
          logit ("e", 
                   "fir: Error opening command file <%s>; exitting!\n", &com[1]);
          return EW_FAILURE;
        }
        continue;
      }

      /* Process anything else as a command */
      /*0*/ 	if (k_its ("MyModId")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (pDcm->firParam.myModName, str);
          init[0] = 1;
        }
        strcpy(processor, "ReadConfig");
      }
      /*1*/	else if (k_its ("InRing")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (pDcm->firParam.ringIn, str);
          init[1] = 1;
        }
        strcpy(processor, "ReadConfig");
      }
      /*2*/	else if (k_its ("OutRing")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (pDcm->firParam.ringOut, str);
          init[2] = 1;
        }
        strcpy(processor, "ReadConfig");
      }
      /*3*/	else if (k_its ("HeartBeatInterval")) 
      {
        pDcm->firParam.heartbeatInt = k_long ();
        init[3] = 1;
        strcpy(processor, "ReadConfig");
      }

      /*4*/     else if (k_its ("LogFile"))
      {
        pDcm->firParam.logSwitch = k_int();
        init[4] = 1;
        strcpy(processor, "ReadConfig");
      }

      /* 5 */    else if (k_its ("MaxGap"))
      {
        pDcm->firParam.maxGap = k_val();
        init[5] = 1;
        strcpy(processor, "ReadConfig");
      }
          
      /* optional */   else if (k_its ("TestMode"))
        pDcm->firParam.testMode = 1;

      /* optional */   else if (k_its ("QueueSize")) {
		pDcm->firParam.QueueSize = k_int();
	}
      /* optional */   else if (k_its ("SleepMilliSeconds")) {
		pDcm->firParam.SleepMilliSeconds = k_int();
	}
      
      /* Enter installation & module types to get */
      /* 6 */	else if (k_its ("GetWavesFrom")) 
      {
        if ((str = k_str()) != NULL) 
          strcpy(pDcm->firParam.readInstName, str);

        if ((str = k_str()) != NULL) 
          strcpy(pDcm->firParam.readModName, str);

        init[6] = 1;
        strcpy(processor, "ReadConfig");
      }

      /* Enter SCNLs of traces to process */
      /* 7 */	else if (k_its ("GetSCNL")) 
      {
        if (nSta >= maxSta) 
        {
          /* Need to allocate more */
          maxSta += STATION_INC;
          if ((stations = (STATION *) realloc (stations, 
                             (maxSta * sizeof (STATION)))) == NULL)
          {
            logit ("e", "fir: Call to realloc failed; exitting!\n");
            return EW_FAILURE;
          }
        }

        str = k_str ();
        if ( str != NULL) 
          strncpy (stations[nSta].inSta, str, TRACE2_STA_LEN);

        str = k_str ();
        if ( str != NULL) 
          strncpy (stations[nSta].inChan, str, TRACE2_CHAN_LEN);

        str = k_str ();
        if ( str != NULL)
          strncpy (stations[nSta].inNet, str, TRACE2_NET_LEN);

        str = k_str ();
        if ( str != NULL)
          strncpy (stations[nSta].inLoc, str, TRACE2_LOC_LEN);

        str = k_str ();
        if ( str != NULL) 
          strncpy (stations[nSta].outSta, str, TRACE2_STA_LEN);

        str = k_str ();
        if ( str != NULL) 
          strncpy (stations[nSta].outChan, str, TRACE2_CHAN_LEN);

        str = k_str ();
        if ( str != NULL)
          strncpy (stations[nSta].outNet, str, TRACE2_NET_LEN);

        str = k_str ();
        if ( str != NULL)
          strncpy (stations[nSta].outLoc, str, TRACE2_LOC_LEN);

	/* Make sure that InSCNL and OutSCNL are different */
        if ( (strcmp(stations[nSta].inSta, stations[nSta].outSta) == 0) && 
             (strcmp(stations[nSta].inChan, stations[nSta].outChan) == 0) && 
             (strcmp(stations[nSta].inNet, stations[nSta].outNet) == 0) &&
	     (strcmp(stations[nSta].inLoc, stations[nSta].outLoc) == 0))
        {
          /** This we should not do ! **/
          fprintf(stderr, 
                  "fir: WARNING - %s.%s.%s.%s will not be renamed after decimation!\n",
                  stations[nSta].inSta, stations[nSta].inChan, 
		  stations[nSta].inNet, stations[nSta].inLoc);
          /**
        ***** Paul Whitmore requested that this condition should be allowed *****
        ***** We will still print out a warning message, but decimation     *****
        ***** will continue                                                 *****

        return EW_FAILURE;
          **/
        }
	
	/* Also make sure that none of the components are not wildcards */
        if ( (strcmp(stations[nSta].inSta,  "*") == 0) ||
             (strcmp(stations[nSta].inChan, "*") == 0) ||
             (strcmp(stations[nSta].inNet,  "*") == 0) ||
             (strcmp(stations[nSta].inLoc,  "*") == 0) ||
	     (strcmp(stations[nSta].outSta, "*") == 0) ||
             (strcmp(stations[nSta].outChan,"*") == 0) ||
             (strcmp(stations[nSta].outNet, "*") == 0) ||
	     (strcmp(stations[nSta].outLoc, "*") == 0) )
        {

          /** This we cannot do !!!! **/
          logit ("e", 
                   "fir: Wildcards not valid in GetSCNL command; exitting!\n");
          return EW_FAILURE;
        }
        
        nSta++;
        init[7] = 1;
        strcpy(processor, "ReadConfig");
      }

      else if (BandCom( pDcm ) )
      {
        strcpy(processor, "BandCom");
        init[8] = 1;
      }
      
      /* Optional: debug command */
      else if (k_its( "Debug") )
      {
        pDcm->firParam.debug = 1;
      }
      
      /* Unknown command */ 
      else 
      {
        logit ("e", "fir: <%s> Unknown command in <%s>.\n", 
                 com, configfile);
        continue;
      }

      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        logit ("e", 
                 "fir: Bad <%s> command in <%s>; exitting!\n",
                 com, configfile);
        return EW_FAILURE;
      }

    } /** while k_rd() **/

    nfiles = k_close();

  } /** while nfiles **/

  pDcm->stations = stations;
  pDcm->nSta = nSta;
  
  /* After all files are closed, check init flags for missed commands */
  nmiss = 0;
  for (i = 0; i < NUMREQ; i++)  
    if (!init[i]) 
      nmiss++;

  if (nmiss) 
  {
    logit ("e", "fir: ERROR, no ");
    if (!init[0])  logit ("e", "<MyModId> "        );
    if (!init[1])  logit ("e", "<InRing> "          );
    if (!init[2])  logit ("e", "<OutRing> " );
    if (!init[3])  logit ("e", "<HeartBeatInterval> "     );
    if (!init[4])  logit ("e", "<LogFIle> "     );
    if (!init[5])  logit ("e", "<MaxGap> "     );
    if (!init[6])  logit ("e", "<GetWavesFrom> "     );
    if (!init[7])  logit ("e", "<GetSCNL> "     );
    if (!init[8])  logit ("e", "<Band> "     );

    logit ("e", "command(s) in <%s>; exitting!\n", configfile);
    return EW_FAILURE;
  }

  return EW_SUCCESS;
}

