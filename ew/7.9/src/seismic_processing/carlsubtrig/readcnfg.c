
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readcnfg.c 5965 2013-09-23 15:36:11Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2009/08/28 17:48:36  paulf
 *     added TrigIdFilename option for pointing to trig_id.d
 *
 *     Revision 1.7  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.6  2002/06/05 14:52:46  patton
 *     Made logit changes.
 *
 *     Revision 1.5  2000/11/01 22:35:05  dietz
 *     csuNet->eventID is now read with k_long instead of k_val.
 *
 *     Revision 1.4  2000/08/08 18:33:24  lucky
 *     Lint cleanup
 *
 *     Revision 1.3  2000/06/10 16:25:22  lombard
 *     clean up return conditions
 *
 *     Revision 1.2  2000/06/10 04:15:47  lombard
 *     Fixed bug that caused crash on zero-length lines in config files.
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * readcnfg.c: Read the CarlSubTrig parameters from a file.
 *              1) Set members of the NETWORK and CTPARAM structures.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
/*                                                                      */
/*      Inputs:         Pointer to a string(input filename)             */
/*                      Pointer to the CarlSubTrig Network structure    */
/*                                                                      */
/*      Outputs:        Updated CarlSubTrig structures (above)  */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlsubtrig.h on       */
/*                        failure                                       */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* strcpy                                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* GetKey                                       */
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
/*   k_str, k_val                               */

#define MAXBUF  4       /* found in kom.c ( should move to kom.h ??? )  */
#define NUMCTPARAMS     25      /* Number of parameters in CSUPARAM that*/
/*   can be set from the config file.   */
#define NUMCTREQ        12      /* Number of parameters in CSUPARAM that*/
/*   MUST be set from the config file.  */

/*******                                                        *********/
/*      CarlSubTrig Includes                                            */
/*******                                                        *********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Functions referenced in this source file                        */
/*******                                                        *********/

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
int ReadConfig( char* filename, NETWORK* csuNet )
{
  char  filesOpen[MAXBUF][MAXFILENAMELEN];      /* Names of open files. */
  char  params[NUMCTPARAMS];    /* Flag for each parameter that is set. */
  char* paramNames[NUMCTPARAMS];        /* Name of each parameter.      */
  int   index;                  /* Loop control variable.               */
  int   numOpen;                /* Number of open files.                */
  char  staCode[TRACE2_STA_LEN];
  char  compCode[TRACE2_CHAN_LEN];
  char  netCode[TRACE2_NET_LEN];
  char  locCode[TRACE2_LOC_LEN];

  /*    Initialize the parameter names                                  */
  paramNames[0] = "MyModuleId";
  paramNames[1] = "RingNameIn";
  paramNames[2] = "RingNameOut";
  paramNames[3] = "HeartBeatInterval";
  paramNames[4] = "GetEventsFrom";
  paramNames[5] = "StationFile";
  paramNames[6] = "Latency";
  paramNames[7] = "NetTriggerDur";
  paramNames[8] = "SubnetContrib";
  paramNames[9] = "PreEventTime";
  paramNames[10] = "MaxDuration";
  paramNames[11] = "DefStationDur";
  paramNames[12] = "next_id";
  paramNames[13] = "Debug";
  paramNames[14] = "Subnet";
  paramNames[15] = "Channel";
  paramNames[16] = "ListSubnets";
  paramNames[17] = "AllSubnets";
  paramNames[18] = "CompAsWild";
  paramNames[19] = "MaxTrigMsgLen";
  paramNames[20] = "TrigIdFilename";
  paramNames[21] = "CoincidentStaTriggers";
  paramNames[22] = "IgnoreCoincident";
  paramNames[23] = "EarlyWarning";   /* RSL: For early warning parameter */
  paramNames[24] = "PlaybackMode";   /* paulf for using data time, not system */

  csuNet->coincident_stas=0;
  csuNet->ignore_coincident=3;
  csuNet->early_warning = 0; /* RSL: Default setting does not send early warning */
  csuNet->useDataTime =0;
  
  /*    Initialize the parameter set flags                              */
  for ( index = 0; index < NUMCTPARAMS; index++ )
  {
    params[index] = 0;
  }
  /* default to the old trig_id.d */
  csuNet->csuParam.trigIdFilename = NULL;

  /* The subnet commands are in the main config file                    */
  strcpy( csuNet->csuParam.subFile, filename );

  /*    Open the main configuration file                                */
  if ( 0 == ( numOpen = k_open( filename ) ) )
  {
    logit( "e", "CarlSubTrig: Error opening configuration file '%s'.\n",
             filename );
    return (ERR_CONFIG_OPEN);
  }
  else
  {
    /*  Remember the name of the current file                           */
    strcpy( filesOpen[0], filename );

    /*  Continue processing as long as there are open files             */
    while ( numOpen > 0 )
    {
      /*        Continue reading from the current open file             */
      while ( k_rd( ) )
      {
        char*   token;  /* Next token read from the configuration file. */
        char*   value;  /* Value associated with the token.             */
        int     result; /* Return code from function calls.             */

        /*      Retrieve the first token from the current line          */
        token = k_str( );

        if ( ! token )
          /*    Ignore blank lines                                      */
          continue;

        if ( '#' == token[0] )
          /*    Ignore comment lines                                    */
          continue;
        
        if ( '@' == token[0] )
        {
          /*    Open a nested configuration file                        */
          result = k_open( &(token[1]) );
          if ( result != numOpen + 1 )
          {
            logit( "e",
                     "CarlSubTrig: Error opening nested configuration file '%s'.\n",
                     &(token[1]) );
            numOpen = 0;
            return (ERR_CONFIG_OPEN);
          }
          else
          {
            /*  Remember the name of the current file                   */
            strcpy( filesOpen[numOpen], &(token[1]) );

            numOpen++;
            continue;
          }
        }

        /*      Look for a configuration parameter - required first     */
        if ( k_its( paramNames[0] ) )
        {
          /*    Read the MyModuleId value                               */
          if ( value = k_str( ) )
          {
            strcpy( csuNet->csuParam.myModName, value );
            params[0] = 1;
          }
        }
        else if ( k_its( paramNames[1] ) )
        {
          /*    Read the RingNameIn value                               */
          if ( value = k_str( ) )
          {
            strcpy( csuNet->csuParam.ringIn, value );
            if ( -1 == ( csuNet->csuParam.ringInKey = 
                         GetKey( csuNet->csuParam.ringIn ) ) )
            {
              logit( "e", "CarlSubTrig: Error finding key for ring '%s'.\n",
                       csuNet->csuParam.ringIn );
              numOpen = 0;
              return (ERR_CONFIG_READ);
            }
            else
              params[1] = 1;
          }
        }
        else if ( k_its( paramNames[2] ) )
        {
          /*    Read the RingNameOut value                              */
          if ( value = k_str( ) )
          {
            strcpy( csuNet->csuParam.ringOut, value );
            if ( -1 == ( csuNet->csuParam.ringOutKey = 
                         GetKey( csuNet->csuParam.ringOut ) ) )
            {
              logit( "e", "CarlSubTrig: Error finding key for ring '%s'.\n",
                       csuNet->csuParam.ringOut );
              numOpen = 0;
              return (ERR_CONFIG_READ);
            }
            else
              params[2] = 1;
          }
        }
        else if ( k_its( paramNames[3] ) )
        {
          /*    Read the HeartBeatInterval value                        */
          csuNet->csuParam.heartbeatInt = k_int( );
          params[3] = 1;
        }
        else if ( k_its( paramNames[4] ) )
        {
          /*    Read the GetEventsFrom values                           */
          if ( value = k_str( ) )
          {
            strcpy( csuNet->csuParam.readInstName, value );
            if ( value = k_str( ) )
            {
              strcpy( csuNet->csuParam.readModName, value );
              params[4] = 1;
            }
          }
        }
        else if ( k_its( paramNames[5] ) )
        {
          /*    Read the StationFile value                              */
          if ( value = k_str( ) )
          {
            strcpy( csuNet->csuParam.staFile, value );
            params[5] = 1;
          }
        }
        else if ( k_its( paramNames[6] ) )
        {
          /*    Read the Latency value                                  */
          csuNet->latency =  k_long( );
          params[6] = 1;

          /* During the latency period, we can receive at most one      */
          /*   trigger message per second for each station. Assuming    */
          /*   half of these are ON and half of these are OFF messages, */
          /*   we need latency / 2 slots for pending triggers, and a    */
          /*   minimum of 4 to give some operating room.                */
          csuNet->nSlots = csuNet->latency / 2;
          if ( csuNet->nSlots < 4 ) csuNet->nSlots = 4;
        }
        else if ( k_its( paramNames[7] ) )
        {
          /*    Read the Base Network Trigger Duration                  */
          csuNet->NetTrigDur = k_long( );
          params[7] = 1;
        }
        else if ( k_its( paramNames[8] ) )
        {
          /*    Read the Subnet Contribution value                      */
          csuNet->subnetContrib = k_long( );
          params[8] = 1;
        }
        else if ( k_its( paramNames[9] ) )
        {
          /*    Read the PreEventTime value                             */
          csuNet->PreEventTime = k_long( );
          params[9] = 1;
        }
        else if ( k_its( paramNames[10] ) )
        {
          /*    Read the Maximum Duration                               */
          csuNet->MaxDur = k_long( );
          params[10] = 1;
        }
        else if ( k_its( paramNames[11] ) )
        {
          /*    Read the Default Station Duration                       */
          csuNet->DefaultStationDur = k_long( );
          params[11] = 1;
        }
        else if ( k_its( paramNames[12] ) )
        {
          /*    Read the next event ID number                           */
          csuNet->eventID = k_long( );
          params[12] = 1;
        }
        else if ( k_its( paramNames[13] ) )
        {
          /*    Read the Debug value                                    */
          csuNet->csuParam.debug = k_int( );
          params[13] = 1;
        }
        else if ( k_its( paramNames[14] ) )
          /* Ignore Subnet commands here; see readsub.c                 */
          continue;

        else if ( k_its( paramNames[15] ) )
        {
          if ( value = k_str( ) )
          {
            if ( sscanf( value, "%[^.].%[^.].%[^.].%s", staCode, compCode, 
                         netCode, locCode ) == 4 )
            {
              if ( csuNet->channels = 
                   (CHANNEL *) realloc(csuNet->channels, 
                                       ( csuNet->nChan + 1 ) * sizeof( CHANNEL ) ) )
              {
                strncpy( csuNet->channels[csuNet->nChan].staCode, staCode, 
			 TRACE2_STA_LEN );
                strncpy( csuNet->channels[csuNet->nChan].compCode, compCode, 
			 TRACE2_CHAN_LEN );
                strncpy( csuNet->channels[csuNet->nChan].netCode, netCode, 
			 TRACE2_NET_LEN );
                strncpy( csuNet->channels[csuNet->nChan].locCode, locCode, 
			 TRACE2_LOC_LEN );
                csuNet->nChan++;
              } 
              else 
              {
                logit( "e", "carlsubtrig: Error allocating channel memory." );
                return (ERR_MALLOC);
              }
            }
            else 
            {       /*  Encountered a parsing problem           */
              logit( "e", "ReadConfig: Failed to parse channel: '%s'.\n", value );
              return (ERR_CONFIG_READ);
            }
            params[15] = 1;
          }
        }
        
        else if ( k_its( paramNames[16] ) )
        {
          /* Read the list-subnets value */
          csuNet->listSubnets = k_val( );
          params[16] = 1;
        }
        
        else if ( k_its( paramNames[17] ) )
        {
          /* Read the numSubAll value */
          csuNet->numSubAll = k_val( );
          params[17] = 1;
        }

        else if ( k_its( paramNames[18] ) )
        {
          /* Set the component as wildcard flag */
          csuNet->compAsWild = 1;
          params[18] = 1;
        }
        else if ( k_its( paramNames[19] ) )
        {
          /* Set maximum length of a triglist message */
          csuNet->trigMsgBufLen = k_long( );
          if( csuNet->trigMsgBufLen > MAX_BYTES_PER_EQ ) 
            csuNet->trigMsgBufLen = MAX_BYTES_PER_EQ;
          params[19] = 1;
        }
        else if ( k_its( paramNames[20] ) )
        {
          csuNet->csuParam.trigIdFilename = strdup(k_str( ));
          params[20] = 1;
          /* okay, now here is where it gets hairy, we need to open this file and parse the next_id token inside it */
          result = k_open( csuNet->csuParam.trigIdFilename );
          if ( result != numOpen + 1 )
          {
            logit( "e",
                     "CarlSubTrig: Error opening trig_id file '%s'.\n",
                     csuNet->csuParam.trigIdFilename );
            numOpen = 0;
            return (ERR_CONFIG_OPEN);
          }
          else
          {
            /*  Remember the name of the current file                   */
            strcpy( filesOpen[numOpen], csuNet->csuParam.trigIdFilename);

            numOpen++;
            continue;
          }
        }
        else if ( k_its( paramNames[21] ) )  /* number of coincident stations to throw out a subnet trigger */
        {
          /* EXPERIMENTAL */
	    csuNet->coincident_stas=k_int();
        }
        else if ( k_its( paramNames[22] ) )  /* if more than this subnets trigger, ignore coincident check */
        {
          /* EXPERIMENTAL */
	    csuNet->ignore_coincident=k_int();
        }
        else if ( k_its( paramNames[23] ) )  /* if 1, launch early warning */
        {
          /* EXPERIMENTAL */
	    csuNet->early_warning=k_int();
        }
        else if ( k_its( paramNames[24] ) )  /* if 1, set to playback mode*/
        {
          /* EXPERIMENTAL */
	    csuNet->useDataTime=k_int();
        }
        /*      Unknown parameter found                                 */
        else
        {
          fprintf( stderr, "CarlSubTrig: Unknown parameter '%s' in '%s'.\n",
                   token, filesOpen[numOpen - 1] );
        }

        /*      Check for processing errors                             */
        if ( k_err( ) )
        {
          fprintf( stderr, "CarlSubTrig: Error processing '%s' in '%s'.\n", token,
                   filesOpen[numOpen - 1] );
          numOpen = 0;
          return (ERR_CONFIG_READ);
          break;
        }
        
      }

      /*      File read ok, close and finish reading other files      */
      numOpen = k_close( );

      /*        End of processing for current file                      */
    }
  }
  
  return ( 0 );
}
