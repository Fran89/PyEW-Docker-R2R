
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: configure.c 1001 2002-06-19 16:50:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2002/06/19 16:50:23  lucky
 *     Fixed a missing comma in logit_init call
 *
 *     Revision 1.2  2002/05/16 16:49:27  patton
 *     Made logit changes.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * Configure the Fir module:
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
/*                      Pointer to the Fir World structure              */
/*                                                                      */
/*      Outputs:        Updated Fir parameter structures(above)         */
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
/*      Fir Includes                                                    */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: Configure                                             */
int Configure(WORLD *pFir, char **argv)
{

  /* Set initial values of WORLD structure */
  InitializeParameters( pFir );

  /* Initialize name of log-file & open it 
	 ***************************************/
  logit_init (argv[1], 0, MAXMESSAGELEN, 1);

  /* Read config file and configure the Fir filter */
  if (ReadConfig(argv[1], pFir) != EW_SUCCESS)
  {
    fprintf (stderr, "fir: failed reading config file <%s>\n", argv[1]);
    return EW_FAILURE;
  }

  logit ("" , "Read command file <%s>\n", argv[1]);

  /* Look up important info from earthworm.h tables
	 ************************************************/
  if (ReadEWH(pFir) != EW_SUCCESS)
  {
    fprintf (stderr, "fir: Call to ReadEWH failed \n" );
    return EW_FAILURE;
  }

  /* Reinitialize logit to desired logging level 
	 *******************************************/
  logit_init (argv[1], 0, MAXMESSAGELEN, pFir->firParam.logSwitch);
  
  /* Get our process ID
	 **********************/
  if ((pFir->MyPid = getpid ()) == -1)
  {
    logit ("e", "fir: Call to getpid failed. Exitting.\n");
    return (EW_FAILURE);
  }

  /* Set up and log the filter structure */
  if ( SetFilter( pFir ) != EW_SUCCESS )
  {
    logit("e", "fir: call to SetFilter failed, exitting.\n");
    return EW_FAILURE;
  }
  
  /* Initialize the filter buffers for each station */
  InitSta( pFir );
  
  return EW_SUCCESS;
}
