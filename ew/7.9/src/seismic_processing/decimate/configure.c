
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: configure.c 6325 2015-05-01 00:44:09Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2002/10/25 17:59:44  dietz
 *     Added support for multiple GetWavesFrom commands
 *
 *     Revision 1.2  2002/05/16 15:17:18  patton
 *     Made Logit changes.
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * Configure the Decimate module:
 *    read the config file and earthworm*.h parameters
 *    set up logging.
 *    set up the station and filter structures
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: Configure                                             */
/*                                                                      */
/*      Inputs:         Pointer to a string(input filename)             */
/*                      Pointer to the Decimate World structure         */
/*                                                                      */
/*      Outputs:        Updated Decimate parameter structures(above)    */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in decimate.h on          */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  

/*******                                                        *********/
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: Configure                                             */
int Configure(WORLD *pDcm, char **argv, const char *versionStr)
{
  char DecRateStr[MAXFILENAMELEN];   /* string for the decimation rates */

  /* Set initial values of WORLD structure */
  InitializeParameters( pDcm );

  /* Initialize name of log-file & open it  
   ***************************************/
  logit_init (argv[1], 0, MAXMESSAGELEN, 1);

  logit("et", "%s version %s\n", argv[0], versionStr);

  /* Read config file and configure the decimator */
  if (ReadConfig(argv[1], pDcm, DecRateStr) != EW_SUCCESS)
  {
    logit("e", "decimate: failed reading config file <%s>\n", argv[1]);
    return EW_FAILURE;
  }
  logit ("" , "Read command file <%s>\n", argv[1]);

  /* Look up important info from earthworm.h tables
   ************************************************/
  if (ReadEWH(pDcm) != EW_SUCCESS)
  {
    logit("e", "decimate: Call to ReadEWH failed \n" );
    return EW_FAILURE;
  }

  /* Reinitialize logit to the desired logging level 
   ***********************************************/
  logit_init (argv[1], 0, MAXMESSAGELEN, pDcm->dcmParam.logSwitch);
  
  /* Get our process ID
   **********************/
  if ((pDcm->MyPid = getpid ()) == -1)
  {
    logit ("e", "decimate: Call to getpid failed. Exiting.\n");
    return (EW_FAILURE);
  }

  /* Set up and log the decimation stage filters */
  if ( SetDecStages( pDcm, DecRateStr ) != EW_SUCCESS )
  {
    logit("e", "decimate: call to SetDecStage failed, exiting.\n");
    return EW_FAILURE;
  }
  
  /* Set up the filter buffers for each station */
  if ( SetStaFilters( pDcm ) != EW_SUCCESS )
  {
    logit("e", "decimate: call to SetStaFilters failed; exiting.\n");
    return EW_FAILURE;
  }
  
  return EW_SUCCESS;
}
