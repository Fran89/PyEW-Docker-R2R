#include "geolib.h"

/************************************************************************
 * days_in_month
 *
 * Description: Return the number of days in the given month
 *
 * Input parameters: month - the required month (1 - 12)
 *                   year - the year (0 - ??)
 * Output parameters: none
 * Returns: the number of days of -ve number for parameter error
 *
 * Comments:
 *
 ************************************************************************/
int days_in_month (int month, int year)

{

  /* array holding number of days / month */
  static int months [13] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0};

  /* check parameters */
  if (year < 0 || month < 1 || month > 12) return GEO_ERROR;

  /* correct februarys months entry */
  if (is_leap_year (year)) months [1] = 29;
  else months [1] = 28;

  /* return the number of days */
  return (months [month -1]);

}

