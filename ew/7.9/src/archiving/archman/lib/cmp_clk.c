#include "geolib.h"

/**************************************************************************
 * compare_clocks
 *
 * Description: Compare two given clock times
 *
 * Input parameters: sec_1, min_1, hou_1 - first time
 *                   da_1, mon_1, yea_1 - first date
 *                   sec_2, min_2, hou_1 - second time
 *                   da_2, mon_2, yea_2 - second date;
 * Output parameters: none
 * Returns: -1 for clock_1 earlier than clock_2
 *           0 for clock_1 equals clock_2
 *          +1 for clock_1 later than clock_2
 *
 * Comments:
 *
 ************************************************************************/
#ifdef __STDC__
int compare_clocks (int sec_1, int min_1, int hou_1,
                    int da_1, int mon_1, int yea_1,
                    int sec_2, int min_2, int hou_2,
                    int da_2, int mon_2, int yea_2)
#else
int compare_clocks (sec_1, min_1, hou_1, da_1, mon_1, yea_1,
                    sec_2, min_2, hou_2, da_2, mon_2, yea_2)
int sec_1, min_1, hou_1, da_1, mon_1, yea_1;
int sec_2, min_2, hou_2, da_2, mon_2, yea_2;
#endif

{

  /* compare the years */
  if (yea_1 > yea_2) return 1;
  if (yea_1 < yea_2) return -1;

  /* compare the months */
  if (mon_1 > mon_2) return 1;
  if (mon_1 < mon_2) return -1;

  /* compare the days */
  if (da_1 > da_2) return 1;
  if (da_1 < da_2) return -1;

  /* compare the hours */
  if (hou_1 > hou_2) return 1;
  if (hou_1 < hou_2) return -1;

  /* compare the minutes */
  if (min_1 > min_2) return 1;
  if (min_1 < min_2) return -1;

  /* compare the seconds */
  if (sec_1 > sec_2) return 1;
  if (sec_1 < sec_2) return -1;

  /* if you get here the times are the same */
  return 0;

}

