#include "geolib.h"

/************************************************************************
 * day_of_year_to_date
 *
 * Description: Convert the day of year to a date
 *
 * Input parameters: day_of_year - the day number (1 - 366)
 *                   year - the year (0 - ??)
 * Output parameters: day - the day (1 - 31)
 *                    month - the month (1 - 12)
 * Returns: TRUE for conversion OK, FALSE otherwise
 *
 * Comments:
 *
 ************************************************************************/
int day_of_year_to_date (int day_of_year, int year, int *day, int *month)

{

  int month_days, month_no;


  /* check parameters */
  if (year < 0) return GEO_FALSE;
  if (is_leap_year (year)) month_days = 366;
  else month_days = 365;
  if (day_of_year < 1 || day_of_year > month_days) return GEO_FALSE;

  /* perform the conversion */
  for (month_no = 1; day_of_year; month_no ++)
  {
    month_days = days_in_month (month_no, year);
    if (day_of_year > month_days)
      day_of_year = day_of_year - month_days;
    else
    {
      *day = day_of_year;
      *month = month_no;
      day_of_year = 0;
    }
  }

  return GEO_TRUE;

}

