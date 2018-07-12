#include "geolib.h"

/**********************************************************************
 * check_clock
 *
 * Description: Check the given clock
 *
 * Input parameters: sec, min, hour - time
 *                   day, month, year - date
 * Output parameters: none
 * Returns: TRUE for clock OK, FALSE otherwise
 *
 * Comments:
 *
 **********************************************************************/
int check_clock (int sec, int min, int hou, int da, int mon, int yea)

{

  int max_day;

  /* check the given clock */
  max_day = days_in_month (mon, yea);
  if (max_day == 0) return GEO_FALSE;
  if (da < 1 || da > max_day) return GEO_FALSE;
  if (hou < 0 || hou > 23) return GEO_FALSE;
  if (min < 0 || min > 59) return GEO_FALSE;
  if (sec < 0 || sec > 59) return GEO_FALSE;

  return GEO_TRUE;

}

