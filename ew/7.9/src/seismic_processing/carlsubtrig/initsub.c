/*
 * initsub.c: Initialize a subnet structures.
 *              1) Initialize members of the SUBNET structures.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: InitializeSubnet					*/
/*									*/
/*	Inputs:		Pointer to the CarlSubTrig subnet array		*/
/*			number of subnet structures in array		*/
/*									*/
/*	Outputs:	Updated subnet array (above)			*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in carlsubtrig.h on 	*/
/*			  failure					*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: InitializeSubnet					*/
void InitializeSubnet( SUBNET* subnet, int nSubs )
{
  int   i, j;
  
  for ( i = 0; i < nSubs; i++ )
  {
    subnet[i].nStas = 0;
    subnet[i].minToTrigger = 0;
    
    for ( j = 0; j < NSUBSTA; j++ )
	{
      subnet[i].stations[j] = -1;
	  subnet[i].triggerable[j] = 0;
	}
  }
}
