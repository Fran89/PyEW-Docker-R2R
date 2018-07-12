/*
 * getsubnet.c: Get the subnet and network trigger length
 *              1) Count the stations triggered in each subnet
 *              2) Determine subnet and network trigger status.
 *              3) Compute trigger duration
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: getSubnet                                             */
/*                                                                      */
/*      Inputs:         pointer to the Network structure                */
/*                                                                      */
/*      Outputs:        triggered subnets are flagged                   */
/*                                                                      */
/*      Returns:        current network trigger duration                */
/*                      0 if network is not triggered                   */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

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
int DoubleCmp( const void *s1, const void *s2 )
{
    double *d1,*d2;
    d1 = (double *) s1;
    d2 = (double *) s2;
    if (*d1 < *d2) return -1;
    if (*d1 > *d2) return 1;
    return 0;
}


/*      Function: GetSubnet                                             */
long GetSubnet( NETWORK *csuNet )
{
  STATION *station = csuNet->stations;
  SUBNET *subnet = csuNet->subnets;
  int active = 0;               /* Network trigger status               */
                                /* 0: not triggered                   	*/
                                /* 1: network triggered                 */
  int subStasTriggered, numSub;
  int isub, jsta;               /* array counters                       */
  double ontimes[500];
    
  numSub = 0;
  for ( isub = 0; isub < csuNet->nSub; isub++ )
  {
    subStasTriggered = 0;

    for ( jsta = 0; jsta < subnet[isub].nStas; jsta++ )
    {
		/* Changed by Eugene Lublinsky, 3/31/Y2K */
		/* triggerable flag added */
		if ( subnet[isub].triggerable[jsta] && (station[subnet[isub].stations[jsta]].countDown > 0) )
                {
                        if (csuNet->coincident_stas>0) 
                        {
                             ontimes[subStasTriggered] = station[subnet[isub].stations[jsta]].onTime;
                        }
			subStasTriggered++;     /* station is triggerable for that subnet and is currently triggered */
                }
    }
    if (csuNet->coincident_stas>0 && subStasTriggered>= subnet[isub].minToTrigger) 
    {
          double last_time;
          int coincident_count=0;
          /* sort the times */
          qsort(ontimes,  subStasTriggered, sizeof(double),  DoubleCmp);
 	  /* do a second pass to see if any have coincident time triggers that match exactly!!! (no tolerance since
	     carlstatrig only measures times to the second. 
          */
	  last_time = ontimes[0];
          for ( jsta = 1; jsta < subStasTriggered; jsta++ )
          {
                if (last_time == ontimes[jsta])
                {
                    coincident_count++;
                    if (coincident_count==1) coincident_count++; /* increase it by 1 for first one to indicate 2 stations match */
                    if (coincident_count == csuNet->coincident_stas)
                    {
                        if ( csuNet->csuParam.debug > 2 )
                          logit("et", "DEBUG: %d coincident triggers at time %f for subnet %d which had %d total triggers\n", 
				coincident_count, last_time, isub+1, subStasTriggered);
                        break;	 /* break out since we found N (or more coincident triggers) - possible GLITCH */
                    }
                }
                else
                {
                    coincident_count=0; 	/* reset the counter */
                    last_time = ontimes[jsta];
                }
          }
          if (coincident_count == csuNet->coincident_stas)
          {
                 subStasTriggered -= coincident_count;  /* subtract these coincident triggers from the count */
          }
    }


    if ( subStasTriggered >= subnet[isub].minToTrigger )
    {   /*  This subnet is active                               */
      subnet[isub].Triggered = 1;       /* reset when network triggers off */
      /* logit("et", "DEBUG: subnet %d triggered which had %d triggers\n", isub+1, subStasTriggered); */
      active = 1;               /* so the network is active             */
      numSub++;
    }
  }
  
  if ( active )
  {
    if ( numSub > csuNet->numSub ) csuNet->numSub = numSub;
    return ( csuNet->NetTrigDur + numSub * csuNet->subnetContrib );
  }
  else
    return ( 0 );
}
