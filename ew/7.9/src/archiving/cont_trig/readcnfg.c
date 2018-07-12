
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readcnfg.c,v 1.0  2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: readcnfg.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
 *     Initial revision
 *
 *
 */

/*
 * readcnfg.c: Read the Cont_Trig parameters from a file.
 *              1) Set members of the NETWORK and CTPARAM structures.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
/*                                                                      */
/*      Inputs:         Pointer to a string(input filename)             */
/*                      Pointer to the Cont_Trig Network structure    */
/*                                                                      */
/*      Outputs:        Updated Cont_Trig structures (above)  */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in cont_trig.h on       */
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
#define NUMCTPARAMS     10      /* Number of parameters in CONTPARAM that*/
/*   can be set from the config file.   */
#define NUMCTREQ        12      /* Number of parameters in CONTPARAM that*/
/*   MUST be set from the config file.  */

/*******                                                        *********/
/*      Cont_Trig Includes                                            */
/*******                                                        *********/
#include "cont_trig.h"

/*******                                                        *********/
/*      Functions referenced in this source file                        */
/*******                                                        *********/

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
int ReadConfig( char* filename, NETWORK* contNet )
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
  paramNames[1] = "RingName";
  paramNames[2] = "HeartBeatInterval";
  paramNames[3] = "StationFile";
  paramNames[4] = "Latency";
  paramNames[5] = "TriggerDur";
  paramNames[6] = "next_id";
  paramNames[7] = "Debug";
  paramNames[8] = "CompAsWild";
  paramNames[9] = "OriginName";
  
  /*    Initialize the parameter set flags                              */
  for ( index = 0; index < NUMCTPARAMS; index++ )
  {
    params[index] = 0;
  }

  /*    Open the main configuration file                                */
  if ( 0 == ( numOpen = k_open( filename ) ) )
  {
    logit( "e", "Cont_Trig: Error opening configuration file '%s'.\n",
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
                     "Cont_Trig: Error opening nested configuration file '%s'.\n",
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
            strcpy( contNet->contParam.myModName, value );
            params[0] = 1;
          }
        }
        else if ( k_its( paramNames[1] ) )
        {
          /*    Read the RingName value                              */
          if ( value = k_str( ) )
          {
            strcpy( contNet->contParam.ring, value );
            if ( -1 == ( contNet->contParam.ringKey = 
                         GetKey( contNet->contParam.ring ) ) )
            {
              logit( "e", "Cont_Trig: Error finding key for ring '%s'.\n",
                       contNet->contParam.ring );
              numOpen = 0;
              return (ERR_CONFIG_READ);
            }
            else
              params[1] = 1;
          }
        }
        else if ( k_its( paramNames[2] ) )
        {
          /*    Read the HeartBeatInterval value                        */
          contNet->contParam.heartbeatInt = k_int( );
          params[2] = 1;
        }
        else if ( k_its( paramNames[3] ) )
        {
          /*    Read the StationFile value                              */
          if ( value = k_str( ) )
          {
            strcpy( contNet->contParam.staFile, value );
            params[3] = 1;
          }
        }
        else if ( k_its( paramNames[4] ) )
        {
          /*    Read the Latency value                                  */
          contNet->latency =  k_long( );
          params[4] = 1;

          /* During the latency period, we can receive at most one      */
          /*   trigger message per second for each station. Assuming    */
          /*   half of these are ON and half of these are OFF messages, */
          /*   we need latency / 2 slots for pending triggers, and a    */
          /*   minimum of 4 to give some operating room.                */
          contNet->nSlots = contNet->latency / 2;
          if ( contNet->nSlots < 4 ) contNet->nSlots = 4;
        }
        else if ( k_its( paramNames[5] ) )
        {
          /*    Read the Base Network Trigger Duration                  */
          contNet->TrigDur = k_long( );
          params[5] = 1;
        }
        else if ( k_its( paramNames[6] ) )
        {
          /*    Read the next event ID number                           */
          contNet->eventID = k_long( );
          params[6] = 1;
        }
        else if ( k_its( paramNames[7] ) )
        {
          /*    Read the Debug value                                    */
          contNet->contParam.debug = k_int( );
          params[7] = 1;
        }
        else if ( k_its( paramNames[8] ) )
        {
          /* Set the component as wildcard flag */
          contNet->compAsWild = 1;
          params[8] = 1;
        }
        else if ( k_its( paramNames[9] ) )
        {
          /*    Read the StationFile value                              */
          if ( value = k_str( ) )
          {
            strcpy( contNet->contParam.OriginName, value );
            params[9] = 1;
          }
        }
        
        /*      Unknown parameter found                                 */
        else
        {
          fprintf( stderr, "Cont_Trig: Unknown parameter '%s' in '%s'.\n",
                   token, filesOpen[numOpen - 1] );
        }

        /*      Check for processing errors                             */
        if ( k_err( ) )
        {
          fprintf( stderr, "Cont_Trig: Error processing '%s' in '%s'.\n", token,
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
