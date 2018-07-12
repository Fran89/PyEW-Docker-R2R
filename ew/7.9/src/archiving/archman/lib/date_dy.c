#include "geolib.h"

/************************************************************************
 * date_to_day_of_year
 *
 * Description: Return the numeric day in the year for the given date
 *
 * Input parameters: day - the day (1 - 31)
 *                   mon - the month (1 - 12)
 *                   year - the year
 * Output parameters: none
 * Returns: The day of the year (1 -> 366) or -ve number on error
 *
 * Comments:
 *
 ************************************************************************/
int date_to_day_of_year (int day, int month, int year)

{

  /* check parameters */
  if (year < 0 || month < 1 || month > 12 || day < 1) return GEO_ERROR;
  if (day > days_in_month (month, year)) return GEO_ERROR;

  /* perform the conversion */
  for (month--; month; month--)
  {
    day = day + days_in_month (month, year);
  }
  return day;

}

