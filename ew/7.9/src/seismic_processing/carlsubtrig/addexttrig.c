
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: addexttrig.c 1458 2004-05-11 17:49:07Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.2  2000/08/08 18:33:24  lucky
 *     Lint cleanup
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * addexttrig.c: Flag extra station triggers to be added to the triglist
 *		 message based on the ListSubnets flag in the configuration
 *               file.  The following 'listMe' values are possible:
 *    
 *          listMe   station status  
 *          ------   -------------------------------------------- 
 *             = 0   untriggered station in untriggered subnet(s)
 *             = 1   triggered station in untriggered subnet(s)
 *       even >= 2   untriggered station in triggered subnet(s) 
 *        odd >= 3   triggered station in triggered subnet(s)
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: AddExtTrig						*/
/*									*/
/*	Inputs:	pointer to Network structure				*/
/*									*/
/*	Outputs: modified station trigger saveOnTimes			*/
/*									*/
/*	Returns:	void						*/

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

/*	Function: AddExtTrig						*/
void AddExtTrig( NETWORK* csuNet )
{
  int isub, jsta;
  int thisSubTrig;
  STATION* station = csuNet->stations;
  SUBNET* subnet = csuNet->subnets;
  
  /* Loop through all the subnets					*/
  for (isub = 0; isub < csuNet->nSub; isub++ )
  {
    /* Flag all stations in triggered subnets */
    thisSubTrig = 0;
    if ( subnet[isub].Triggered  &&  csuNet->listSubnets ) 
    {
      thisSubTrig = 1;
    }
    /* Be really conservative; Flag all stations in       */
    /* any subnet that had at least one station triggered */
    else if ( csuNet->listSubnets == 3 )
    { 
      for ( jsta = 0; jsta < subnet[isub].nStas; jsta++ )
      {
	if ( (station[subnet[isub].stations[jsta]].listMe % 2) == 1 )
	{  /* This station triggered; mark all the stations in this subnet */
	  thisSubTrig = 1;
	  goto SetTrig;
	}
      }
    }
    
  SetTrig:
    if ( thisSubTrig )
    {
      for ( jsta = 0; jsta < subnet[isub].nStas; jsta++ )
      {
	/*if ( station[subnet[isub].stations[jsta]].listMe == 0 )*/
	/*  station[subnet[isub].stations[jsta]].listMe = 2; */
	  station[subnet[isub].stations[jsta]].listMe += 2;	
      }
    }
  }

/* Get rid of triggered stations that weren't in triggered subnets */
  if( csuNet->listSubnets == 1 )
  {
     for( jsta = 0; jsta < csuNet->nSta; jsta++ )
     {
       if( station[jsta].listMe < 2 ) station[jsta].listMe = 0;
     }
  }

  return;
}
