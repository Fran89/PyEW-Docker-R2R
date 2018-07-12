
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readcnfg.c 6229 2015-01-23 16:45:37Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2008/02/21 21:28:46  paulf
 *     patch to STAtime from David Wilson of HVO
 *
 *     Revision 1.6  2007/03/12 20:47:06  paulf
 *     fixed size of params array bug
 *
 *     Revision 1.5  2006/10/20 15:44:37  paulf
 *     udpated with changes from Utah, STAtime now configurable
 *
 *     Revision 1.4  2005/04/12 22:43:07  dietz
 *     Added optional command "GetWavesFrom <instid> <module_id>"
 *
 *     Revision 1.3  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.2  2002/05/16 14:56:15  patton
 *     Made logit changes
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * readcnfg.c: Read the CarlStaTrig parameters from a file.
 *              1) Set members of the CSTPARAM and WORLD structures.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
/*                                                                      */
/*      Inputs:         Pointer to a string(input filename)             */
/*                      Pointer to the CarlStaTrig World structure      */
/*                                                                      */
/*      Outputs:        Updated CarlStaTrig parameter structures(above) */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <stdlib.h>     /* realloc                                      */
#include <string.h>     /* strcpy                                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* GetKey                                       */
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
                        /*   k_str, k_val                               */

#define MAXBUF  4       /* found in kom.c ( should move to kom.h ??? )  */
#define NUMCTPARAMS     14      /* Number of parameters that can be set */
                                /*   from the config file.              */
#define NUMCTREQ        8       /* Number of parameters that MUST be    */
                                /*   set from the config file.          */

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ReadConfig                                            */
int ReadConfig( char* filename, WORLD* cstWorld)
{
  char  filesOpen[MAXBUF][MAXFILENAMELEN];      /* Names of open files. */
  char  params[NUMCTPARAMS];    /* Flag for each parameter that is set. */
  char* paramNames[NUMCTPARAMS];        /* Name of each parameter.      */
  int   index;                  /* Loop control variable.               */
  int   numOpen;                /* Number of open files.                */
  int   retVal = 0;             /* Return value for this function.      */

  /*    Initialize the parameter names                                  */
  paramNames[0] = "MyModuleId";
  paramNames[1] = "RingNameIn";
  paramNames[2] = "RingNameOut";
  paramNames[3] = "HeartBeatInterval";
  paramNames[4] = "StationFile";
  paramNames[5] = "StartUp";
  paramNames[6] = "Ratio";
  paramNames[7] = "Quiet";
  paramNames[8] = "LTAtime";
  paramNames[9] = "MaxGap";
  paramNames[10] = "Decimation";
  paramNames[11] = "Debug";
  paramNames[12] = "GetWavesFrom";
  paramNames[13] = "STAtime";

  /*    Initialize the parameter set flags                              */
  for ( index = 0; index < NUMCTPARAMS; index++ )
  {
    params[index] = 0;
  }

  /*    Open the main configuration file                                */
  if ( 0 == ( numOpen = k_open( filename ) ) )
  {
    logit( "e", "CarlStaTrig: Error opening configuration file '%s'.\n",
        filename );
    retVal = ERR_CONFIG_OPEN;
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
                "CarlStaTrig: Error opening nested configuration file '%s'.\n",
                &(token[1]) );
            numOpen = 0;
            retVal = ERR_CONFIG_OPEN;
            break;
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
          if ( (value = k_str( )) != NULL )
          {
            strcpy( cstWorld->cstParam.myModName, value );
            params[0] = 1;
          }
        }
        else if ( k_its( paramNames[1] ) )
        {
          /*    Read the RingNameIn value                               */
          if ( (value = k_str( )) != NULL )
          {
            strcpy( cstWorld->cstParam.ringIn, value );
            if ( -1 == ( cstWorld->cstParam.ringInKey = 
                         GetKey( cstWorld->cstParam.ringIn ) ) )
            {
              logit( "e", "CarlStaTrig: Error finding key for ring '%s'.\n",
                        cstWorld->cstParam.ringIn );
              numOpen = 0;
              retVal = ERR_CONFIG_READ;
              break;
            }
            else
              params[1] = 1;
          }
        }
        else if ( k_its( paramNames[2] ) )
        {
          /*    Read the RingNameOut value                              */
          if ( (value = k_str( )) != NULL )
          {
            strcpy( cstWorld->cstParam.ringOut, value );
            if ( -1 == ( cstWorld->cstParam.ringOutKey = 
                         GetKey( cstWorld->cstParam.ringOut ) ) )
            {
              logit( "e", "CarlStaTrig: Error finding key for ring '%s'.\n",
                        cstWorld->cstParam.ringOut );
              numOpen = 0;
              retVal = ERR_CONFIG_READ;
              break;
            }
            else
              params[2] = 1;
          }
        }
        else if ( k_its( paramNames[3] ) )
        {
          /*    Read the HeartBeatInterval value                        */
          cstWorld->cstParam.heartbeatInt = k_int( );
          params[3] = 1;
        }
        else if ( k_its( paramNames[4] ) )
        {
          /*    Read the StationFile value                              */
          if ( (value = k_str( )) != NULL )
          {
            strcpy( cstWorld->cstParam.staFile, value );
            params[4] = 1;
          }
        }
        else if ( k_its( paramNames[5] ) )
        {
          /*    Read the StartUp value                          */
          cstWorld->startUp = k_int( );
          params[5] = 1;
        }
        else if ( k_its( paramNames[6] ) )
        {
          /*    Read the Ratio value                    */
          cstWorld->Ratio = k_val( );
          params[6] = 1;
        }
        else if ( k_its( paramNames[7] ) )
        {
          /*    Read the Quiet value                                    */
          cstWorld->Quiet = k_val( );
          params[7] = 1;
        }
        /*      Continue looking for optional configuration parameters  */
        else if ( k_its( paramNames[8] ) )
        {
          /*   Read the LTAtime value */
          cstWorld->LTAtime = k_int();
          params[8] = 1;
        }
        else if ( k_its( paramNames[9] ) )
        {
          /*    Read the MaxGap value                                   */
          cstWorld->maxGap = k_long( );
          params[9] = 1;
        }
        else if ( k_its( paramNames[10] ) )
        {
          /*    Read the Decimation value                               */
          cstWorld->decimation = k_int( );
          params[10] = 1;
        }
        else if ( k_its( paramNames[11] ) )
        {
          /*    Read the Debug value                                    */
          cstWorld->cstParam.debug = k_int( );
          params[11] = 1;
        }
/*opt*/ else if ( k_its( paramNames[13] ) )
        {
          cstWorld->STAtime = k_int();
          params[13] = 1;
        }
/*opt*/ else if ( k_its( paramNames[12] ) )
        {
          /*    Read the GetWavesFrom instid  modid                     */
          char     *str   = NULL;
          MSG_LOGO *tlogo = NULL;
          int       nlogo = cstWorld->cstParam.nGetLogo;
          tlogo = (MSG_LOGO *)realloc( cstWorld->cstParam.GetLogo, 
                                      (nlogo+1)*sizeof(MSG_LOGO) );
          if( tlogo == NULL )
          {
            logit( "e", "CarlStaTrig: %s: error reallocing %d bytes.\n",
                   paramNames[12], (nlogo+1)*sizeof(MSG_LOGO) );
            numOpen = 0;
            retVal = ERR_CONFIG_READ;
            break;
          }
          cstWorld->cstParam.GetLogo = tlogo;

          if( (str=k_str()) != NULL )       /* read instid */
          {
            if( GetInst( str, &(cstWorld->cstParam.GetLogo[nlogo].instid) ) != 0 )
            {
              logit( "e", "CarlStaTrig: Invalid installation name <%s>"
                     " in <%s> cmd!\n", str, paramNames[12] );
              numOpen = 0;
              retVal = ERR_CONFIG_READ;
              break;
            }
            if( (str=k_str()) != NULL )    /* read module id */
            {
              if( GetModId( str, &(cstWorld->cstParam.GetLogo[nlogo].mod) ) != 0 )
              {
                logit( "e", "CarlStaTrig: Invalid module name <%s>"
                       " in <%s> cmd!\n", str, paramNames[12] );
                numOpen = 0;
                retVal = ERR_CONFIG_READ;
                break;
              }
              cstWorld->cstParam.nGetLogo++;
              params[12] = 1;
            } /* end if modid */
          } /* end if instid */
        } /* end GetWavesFrom cmd */

        /*      Unknown parameter found                                 */
        else
        {
          logit( "e", "CarlStaTrig: Unknown parameter '%s' in '%s'.\n",
                token, filesOpen[numOpen - 1] );
        }

        /*      Check for processing errors                             */
        if ( k_err( ) )
        {
          logit( "e", "CarlStaTrig: Error processing '%s' in '%s'.\n", 
                   token, filesOpen[numOpen - 1] );
          numOpen = 0;
          retVal = ERR_CONFIG_READ;
          break;
        }

        /*      End of processing for current line                      */
      }

      /*        Check for read errors                                   */
      if ( k_err( ) )
      {
        logit( "e", "CarlStaTrig: Error reading from '%s'.\n",
                filesOpen[numOpen - 1] );
        numOpen = 0;
        retVal = ERR_CONFIG_READ;
      }
      else
      {
        /*      File read ok, close and finish reading other files      */
        numOpen = k_close( );
      }

      /*        End of processing for current file                      */
    }

    if ( ! CT_FAILED( retVal ) )
    {
      /*        Check for required parameters that were not set         */
      for ( index = 0; index < NUMCTREQ; index++ )
      {
        if ( ! params[index] )
        {
          logit( "e", "CarlStaTrig: Required parameter '%s' not found.\n",
                paramNames[index] );
          retVal = ERR_CONFIG_READ;
        }
      }

      if ( ! CT_FAILED( retVal ) )
      {
        /*      Notify user if any default values are used              */
        for ( index = NUMCTREQ; index < NUMCTPARAMS; index++ )
        {
          if ( ! params[index] )
            logit( "e", "CarlStaTrig: Using default value for '%s'.\n",
                paramNames[index] );
        }
      }
    }
  }

  return ( retVal );
}
