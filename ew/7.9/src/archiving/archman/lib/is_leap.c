#include "geolib.h"

/************************************************************************
 * is_leap_year
 *
 * Description: Returns TRUE if the given year is a leap year
 *
 * Input parameters: year - the year to test
 * Output parameters: none
 * Returns: TRUE or FALSE or -ve number for year < 0
 *
 * Comments:
 *
 ************************************************************************/
int is_leap_year (int year)

{

  /* check parameters */
  if (year < 0) return GEO_ERROR;

  /* check for leap year */
  if (! (year % 4))
  {
    if (year % 100 == 0 && year % 400 != 0) return GEO_FALSE;
    return GEO_TRUE;
  }
  return GEO_FALSE;

}

