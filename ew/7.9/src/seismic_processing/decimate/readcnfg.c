
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readcnfg.c 4882 2012-07-06 19:54:34Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2004/05/11 18:14:18  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.3  2002/10/25 17:59:44  dietz
 *     Added support for multiple GetWavesFrom commands
 *
 *     Revision 1.2  2002/05/16 15:17:36  patton
 *     Made Logit changes.
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * readcnfg.c: Read the Decimate parameters from a file.
 *              1) Set members of the WORLD structures.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
/*                                                                      */
/*      Inputs:         Pointer to a string(input filename)             */
/*                      Pointer to the Decimate World structure         */
/*                      Pointer to a string(output DecRateStr)          */
/*                          string must be allocated by caller.         */
/*                                                                      */
/*      Outputs:        Updated Decimate parameter structures(above)    */
/*                      Update DecRateStr string                        */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in decimate.h on          */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* strcpy, strlen, strcmp                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
                        /*   k_str, k_val                               */
/*******                                                        *********/
/*      Decimate Includes                                            */
/*******                                                        *********/
#include "decimate.h"

#define NUMREQ          10      /* Number of parameters that MUST be    */
                                /*   set from the config file.          */
#define STATION_INC     10      /* how many more are allocated each     */
                                /*   time we run out                    */

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/
int IsWild( char * );

int IsWild( char *str )
{
   if( strcmp( str, "*" ) == 0 ) return 1;
   return 0;
}

/*      Function: ReadConfig                                            */
int ReadConfig (char *configfile, WORLD* pDcm, char *DecRateStr)
{
  char     		init[NUMREQ];     
  /* init flags, one byte for each required command */
  int      		nmiss;
  /* number of required commands that were missed   */
  char    		*com;
  char    		*str;
  char                  cnull = '\0';         
  int      		nfiles;
  int      		success;
  int      		i;
  STATION               *stations = NULL;  /* Array of STATION structures */
  int                   maxSta = 0;     /* number of STATIONs allocated   */
  int                   nSta = 0;       /* number of STATIONs used        */

  /* Set to zero one init flag for each required command */

  for (i = 0; i < NUMREQ; i++)  init[i] = 0;

  /* Open the main configuration file 
**********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    logit ("e",
             "decimate: Error opening command file <%s>; exiting!\n", 
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
      if (!com) continue;
      if (com[0] == '#') continue;

      /* Open a nested configuration file */
      if (com[0] == '@') 
      {
        success = nfiles + 1;
        nfiles  = k_open (&com[1]);
        if (nfiles != success) 
        {
          logit ("e", 
                   "decimate: Error opening command file <%s>; exiting!\n", &com[1]);
          return EW_FAILURE;
        }
        continue;
      }

      /* Process anything else as a command */
/*0*/ if (k_its ("MyModId")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (pDcm->dcmParam.myModName, str);
          init[0] = 1;
        }
      }

/*1*/ else if (k_its ("InRing")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (pDcm->dcmParam.ringIn, str);
          init[1] = 1;
        }
      }

/*2*/ else if (k_its ("OutRing")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (pDcm->dcmParam.ringOut, str);
          init[2] = 1;
        }
      }

/*3*/ else if (k_its ("HeartBeatInterval")) 
      {
        pDcm->dcmParam.heartbeatInt = k_long ();
        init[3] = 1;
      }

/*4*/ else if (k_its ("LogFile"))
      {
        pDcm->dcmParam.logSwitch = k_int();
        init[4] = 1;
      }

/*5*/ else if (k_its ("DecimationRates")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (DecRateStr, str);
          init[5] = 1;
        }
      }

/*6*/ else if (k_its ("MinTraceBuf"))
      {
        pDcm->dcmParam.minTraceBufLen = k_int();
        init[6] = 1;
      }

/*7*/ else if (k_its ("MaxGap"))
      {
        pDcm->dcmParam.maxGap = k_val();
        init[7] = 1;
      }
          
   /* Enter installation & module types to get */
   /* Added mandatory 3rd argument, msgtype    */
/*8*/ else if(k_its ("GetWavesFrom")) 
      {
        int ilogo = pDcm->dcmParam.nlogo;
        if(ilogo+1 >= MAX_LOGO) 
        {
           logit( "e","decimate: Too many GetWavesFrom commands; "
                  "max=%d; exiting!\n", MAX_LOGO );
           return EW_FAILURE;
        }
        if((str = k_str()) != NULL) 
          strcpy(pDcm->dcmParam.readInstName[ilogo], str);

        if((str = k_str()) != NULL) 
          strcpy(pDcm->dcmParam.readModName[ilogo], str);

        if((str = k_str()) != NULL)
        {
          if( strcmp(str,"TYPE_TRACEBUF" ) != 0 &&
              strcmp(str,"TYPE_TRACEBUF2") != 0    ) 
          {
             logit( "e","decimate: GetWavesFrom: invalid msg type: %s "
                        "(must be TYPE_TRACEBUF or TYPE_TRACEBUF2); "
                        "exiting!\n", str );
             return EW_FAILURE;
          }
          strcpy(pDcm->dcmParam.readTypeName[ilogo], str);
        }
        pDcm->dcmParam.nlogo++;
        init[8] = 1;
      }

   /* Enter SCNLs of traces to process */
/*9*/ else if (k_its("GetSCN")  ||  k_its("GetSCNL") ) 
      {
        int scnl = k_its("GetSCNL");
        if (nSta >= maxSta) 
        {
       /* Need to allocate more */
          maxSta += STATION_INC;
          if ((stations = (STATION *) realloc (stations, 
                             (maxSta * sizeof (STATION)))) == NULL)
          {
            logit ("e", "decimate: realloc for SCNL list failed; exiting!\n");
            return EW_FAILURE;
          }
        }

        str = k_str(); /* read input station code */
        if( !str  ||  strlen(str) >= TRACE2_STA_LEN  ||  IsWild(str) ) {
            if( !str ) str = &cnull;
            logit ("e", "decimate: Invalid input station code: %s "
                        "in GetSCNL cmd; exiting!\n", str);
            return EW_FAILURE;
        }
        strncpy(stations[nSta].inSta, str, TRACE2_STA_LEN) ;

        str = k_str(); /* read input component code */
        if( !str  ||  strlen(str) >= TRACE2_CHAN_LEN  ||  IsWild(str)) {
            if( !str ) str = &cnull;
            logit ("e", "decimate: Invalid input component code: %s "
                        "in GetSCNL cmd; exiting!\n", str);
            return EW_FAILURE;
        }
        strncpy(stations[nSta].inChan, str, TRACE2_CHAN_LEN);

        str = k_str(); /* read input network code */
        if( !str  ||  strlen(str) >= TRACE2_NET_LEN  ||  IsWild(str)) {
            if( !str ) str = &cnull;
            logit ("e", "decimate: Invalid input network code: %s "
                        "in GetSCNL cmd; exiting!\n", str);
            return EW_FAILURE;
        }
        strncpy(stations[nSta].inNet, str, TRACE2_NET_LEN);

        if( scnl ) {  /* read input location code */
          str = k_str();
          if( !str  ||  strlen(str) >= TRACE2_LOC_LEN  ||  IsWild(str)) {
              if( !str ) str = &cnull;
              logit ("e", "decimate: Invalid input location code: %s "
                          "in GetSCNL cmd; exiting!\n", str);
              return EW_FAILURE;
          }
          strncpy(stations[nSta].inLoc, str, TRACE2_LOC_LEN);
        } 
        else {      /* use default blank location code */
          strncpy(stations[nSta].inLoc, LOC_NULL_STRING, TRACE2_LOC_LEN);
        }

        str = k_str(); /* read output station code */
        if( !str  ||  strlen(str) >= TRACE2_STA_LEN  ||  IsWild(str)) {
            if( !str ) str = &cnull;
            logit ("e", "decimate: Invalid output station code: %s "
                        "in GetSCNL cmd; exiting!\n", str);
            return EW_FAILURE;
        }
        strncpy(stations[nSta].outSta, str, TRACE2_STA_LEN);

        str = k_str(); /* read output component code */
        if( !str  ||  strlen(str) >= TRACE2_CHAN_LEN  ||  IsWild(str)) {
            if( !str ) str = &cnull;
            logit ("e", "decimate: Invalid output component code: %s "
                        "in GetSCNL cmd; exiting!\n", str);
            return EW_FAILURE;
        }
        strncpy(stations[nSta].outChan, str, TRACE2_CHAN_LEN);

        str = k_str(); /* read output network code */
        if( !str  ||  strlen(str) >= TRACE2_NET_LEN  ||  IsWild(str)) {
            if( !str ) str = &cnull;
            logit ("e", "decimate: Invalid output network code: %s "
                        "in GetSCNL cmd; exiting!\n", str);
            return EW_FAILURE;
        }
        strncpy(stations[nSta].outNet, str, TRACE2_NET_LEN);

        if( scnl ) {  /* read output location code */
          str = k_str();
          if( !str  ||  strlen(str) >= TRACE2_LOC_LEN  ||  IsWild(str)) {
              if( !str ) str = &cnull;
              logit ("e", "decimate: Invalid output location code: %s "
                          "in GetSCNL cmd; exiting!\n", str );
              return EW_FAILURE;
          }
          strncpy(stations[nSta].outLoc, str, TRACE2_LOC_LEN);
        } 
        else {      /* use default blank location code */
          strncpy(stations[nSta].outLoc, LOC_NULL_STRING, TRACE2_LOC_LEN);
        }

        nSta++;
        init[9] = 1;
      }

      else if (k_its ("TestMode"))  /* Optional */
      {
        pDcm->dcmParam.testMode = 1;
      }

      else if (k_its( "Debug") )    /* Optional */
      {
        pDcm->dcmParam.debug = 1;
      }
      else if (k_its( "Quiet") )    /* Optional */
      {
        pDcm->dcmParam.quiet = 1;
      }
      
   /* Unknown command */ 
      else 
      {
        logit ("e", "decimate: <%s> Unknown command in <%s>.\n", 
                 com, configfile);
        continue;
      }

   /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        logit ("e", 
                 "decimate: Bad <%s> command in <%s>; exiting!\n",
                 com, configfile);
        return EW_FAILURE;
      }

    } /** end while k_rd() **/

    nfiles = k_close ();

  } /** end while nfiles **/

  pDcm->stations = stations;
  pDcm->nSta = nSta;
  
/* After all files are closed, check init flags for missed commands */
  nmiss = 0;
  for(i = 0; i < NUMREQ; i++)  if(!init[i])  nmiss++;

  if (nmiss) 
  {
    logit ("e", "decimate: ERROR, no ");
    if (!init[0])  logit ("e", "<MyModId> "           );
    if (!init[1])  logit ("e", "<InRing> "            );
    if (!init[2])  logit ("e", "<OutRing> "           );
    if (!init[3])  logit ("e", "<HeartBeatInterval> " );
    if (!init[4])  logit ("e", "<LogFIle> "           );
    if (!init[5])  logit ("e", "<DecimationRates> "   );
    if (!init[6])  logit ("e", "<MinTraceBuf> "       );
    if (!init[7])  logit ("e", "<MaxGap> "            );
    if (!init[8])  logit ("e", "<GetWavesFrom> "      );
    if (!init[9])  logit ("e", "<GetSCNL> "           );

    logit ("e", "command(s) in <%s>; exiting!\n", configfile);
    return EW_FAILURE;
  }

/* Make sure that InSCNL and OutSCNL are different */
  for( i=0; i<nSta; i++ )
  {
    if( (strcmp(stations[i].inSta,  stations[i].outSta)  == 0) && 
        (strcmp(stations[i].inChan, stations[i].outChan) == 0) && 
        (strcmp(stations[i].inNet,  stations[i].outNet)  == 0) &&
        (strcmp(stations[i].inLoc,  stations[i].outLoc)  == 0)     )
    {
      logit ("e", "decimate: WARNING: %s.%s.%s.%s will have same "
                  "SCNL after decimation!\n",
                   stations[i].inSta, stations[i].inChan, 
                   stations[i].inNet, stations[i].inLoc );
   /***** Originally coded to exit here.                                *****
    ***** Paul Whitmore requested that this condition should be allowed *****
    ***** We will still print out a warning message, but decimation     *****
    ***** will continue                                                 *****
      return EW_FAILURE;
    **/
    }
  }

  return EW_SUCCESS;
}

