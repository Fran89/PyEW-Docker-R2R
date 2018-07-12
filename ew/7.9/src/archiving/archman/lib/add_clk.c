#include "geolib.h"

/***********************************************************************
 * add_clock
 *
 * Description: Add the given number of seconds to the given clock
 *
 * Input variables: sec, min, hou - the time
 *                  da, mon, yea - the date
 *		    increment - the number of seconds to add
 * Output variables: none
 * Returns: TRUE for write OK, FALSE otherwise
 *
 * Comments:
 *
 *************************************************************************/
int add_clock (int *sec, int *min, int *hou, int *da, int *mon, int *yea,
	       long increment)

{

  long sec_u;
  int min_u, hou_u, da_u, mon_u, yea_u;

  sec_u = (long) *sec;
  min_u = *min;
  hou_u = *hou;
  da_u = *da;
  mon_u = *mon;
  yea_u = *yea;

  /* check the given clock */
  if (! check_clock ((int) sec_u, min_u, hou_u, da_u, mon_u, yea_u))
    return GEO_FALSE;

  /* check the increment */
  if (increment > 0L)
  {
    /* add the given value (+ve) to the clock */
    sec_u = sec_u + increment;
    while (sec_u > 59L)
    {
      sec_u = sec_u - 60L;
      min_u ++;
      if (min_u > 59)
      {
	min_u = 0;
	hou_u ++;
	if (hou_u > 23)
	{
	  hou_u = 0;
	  da_u ++;
	  if (da_u > days_in_month (mon_u, yea_u))
	  {
	    da_u = 1;
	    mon_u ++;
	    if (mon_u > 12)
	    {
	      mon_u = 1;
	      yea_u ++;
	    }
	  }
	}
      }
    }
  }
  else
  {
    /* add the given value (-ve) to the clock */
    sec_u = sec_u + increment;
    while (sec_u < 0L)
    {
      sec_u = sec_u + 60L;
      min_u --;
      if (min_u < 0)
      {
	min_u = 59;
	hou_u --;
	if (hou_u < 0)
	{
	  hou_u = 23;
	  da_u --;
	  if (da_u < 1)
	  {
	    mon_u --;
	    if (mon_u < 1)
	    {
	      mon_u = 12;
	      yea_u --;
	      if (yea_u < 0) return GEO_FALSE;
	    }
	    da_u = days_in_month (mon_u, yea_u);
	  }
	}
      }
    }
  }

  /* return the updated clock */
  *sec = (int) sec_u;
  *min = min_u;
  *hou = hou_u;
  *da = da_u;
  *mon = mon_u;
  *yea = yea_u;

  return GEO_TRUE;

}

/***********************************************************************
 * add_milli
 *
 * Description: Add the given number of milli-seconds to the given clock
 *
 * Input variables: milli, sec, min, hou - the time
 *                  da, mon, yea - the date
 *		    increment - the number of mS to add
 * Output variables: none
 * Returns: TRUE for write OK, FALSE otherwise
 *
 * Comments:
 *
 *************************************************************************/
int add_milli (int *milli, int *sec, int *min, int *hou, int *da, int *mon, int *yea,
               long increment)

{

  long sec_inc, milli_inc;


  sec_inc = increment / 1000;
  milli_inc = increment % 1000;

  *milli += milli_inc;
  while (*milli > 999)
  {
    *milli -= 1000;
    sec_inc ++;
  }
  while (*milli < 0)
  {
    *milli += 1000;
    sec_inc --;
  }
  return add_clock (sec, min, hou, da, mon, yea, sec_inc);

}
