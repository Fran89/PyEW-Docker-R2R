#include "geolib.h"

/*****************************************************************
 * day_diff
 *
 * Description: Find the difference (in days) between two values
 *
 * Input parameters: day_1, month_1, year_1 - first date
 *                   day_2, month_2, year_2 - second date
 * Output parameters: none
 * Returns: The difference (time_2 - time_1) in minutes
 *
 * Comments:
 *
 *****************************************************************/
int day_diff (int day_1, int month_1, int year_1,
			  int day_2, int month_2, int year_2)

{

  int operation = 0, dm, accumulator;


  accumulator = 0;

  /* is date_1 <, > or == date_2 */
  if (year_1 < year_2) operation = 1;
  if (year_1 > year_2) operation = -1;
  if (! operation)
  {
    if (month_1 < month_2) operation = 1;
    if (month_1 > month_2) operation = -1;
  }
  if (! operation)
  {
    if (day_1 < day_2) operation = 1;
    if (day_1 > day_2) operation = -1;
  }

  /* sort out the dates */
  switch (operation)
  {
  case 0:	/* dates are the same */
    break;

  case 1:	/* date_1 < date_2 */
    dm = days_in_month (month_1, year_1);
    while (day_1 != day_2 || month_1 != month_2 || year_1 != year_2)
    {
      day_1 = day_1 + 1;
      if (day_1 > dm)
      {
        day_1 = 1;
        month_1 ++;
        if (month_1 > 12)
        {
          month_1 = 1;
          year_1 ++;
        }
        dm = days_in_month (month_1, year_1);
      }
      accumulator += 1;
    }
    break;

  case -1:	/* date_1 > date_2 */
    while (day_1 != day_2 || month_1 != month_2 || year_1 != year_2)
    {
      day_1 = day_1 - 1;
      if (day_1 < 1)
      {
        month_1 --;
        if (month_1 < 1)
        {
          month_1 = 12;
          year_1 --;
        }
		day_1 = days_in_month (month_1, year_1);
      }
      accumulator -= 1;
    }
    break;
  }
  
  return accumulator;
  
}

/*****************************************************************
 * minute_diff
 *
 * Description: Find the difference (in minutes) between two values
 *
 * Input parameters: sec_1, min_1, hour_1, day_1, month_1, year_1 - first time
 *                   sec_2, min_2, hour_2, day_2, month_2, year_2 - second time
 * Output parameters: none
 * Returns: The difference (time_2 - time_1) in minutes
 *
 * Comments:
 *
 *****************************************************************/
long minute_diff (int sec_1, int min_1, int hour_1, int day_1, int month_1,
				  int year_1, int sec_2, int min_2, int hour_2, int day_2,
				  int month_2, int year_2)

{

  int operation = 0, dm;
  long accumulator;


  /* sort out the seconds */
  min_1 = min_1 + ((sec_1 >= 30) ? 1 : 0);
  min_2 = min_2 + ((sec_2 >= 30) ? 1 : 0);

  /* sort out the minutes */
  accumulator = min_2 - min_1;

  /* sort out the hours */
  accumulator += (hour_2 - hour_1) * 60;

  /* is date_1 <, > or == date_2 */
  if (year_1 < year_2) operation = 1;
  if (year_1 > year_2) operation = -1;
  if (! operation)
  {
    if (month_1 < month_2) operation = 1;
    if (month_1 > month_2) operation = -1;
  }
  if (! operation)
  {
    if (day_1 < day_2) operation = 1;
    if (day_1 > day_2) operation = -1;
  }

  /* sort out the dates */
  switch (operation)
  {
  case 0:	/* dates are the same */
    break;

  case 1:	/* date_1 < date_2 */
    dm = days_in_month (month_1, year_1);
    while (day_1 != day_2 || month_1 != month_2 || year_1 != year_2)
    {
      day_1 = day_1 + 1;
      if (day_1 > dm)
      {
        day_1 = 1;
        month_1 ++;
        if (month_1 > 12)
        {
          month_1 = 1;
          year_1 ++;
        }
        dm = days_in_month (month_1, year_1);
      }
      accumulator += 1440L;
    }
    break;

  case -1:	/* date_1 > date_2 */
    while (day_1 != day_2 || month_1 != month_2 || year_1 != year_2)
    {
      day_1 = day_1 - 1;
      if (day_1 < 1)
      {
        month_1 --;
        if (month_1 < 1)
        {
          month_1 = 12;
          year_1 --;
        }
		day_1 = days_in_month (month_1, year_1);
      }
      accumulator -= 1440L;
    }
    break;
  }
  
  return accumulator;
  
}

/*****************************************************************
 * second_diff
 *
 * Description: Find the difference (in seconds) between two values
 *
 * Input parameters: sec_1, min_1, hour_1, day_1, month_1, year_1 - first time
 *                   sec_2, min_2, hour_2, day_2, month_2, year_2 - second time
 * Output parameters: none
 * Returns: The difference (time_2 - time_1) in secondss
 *
 * Comments:
 *
 *****************************************************************/
long second_diff (int sec_1, int min_1, int hour_1, int day_1, int month_1,
				  int year_1, int sec_2, int min_2, int hour_2, int day_2,
				  int month_2, int year_2)

{

  int operation = 0, dm;
  long accumulator;
  

  /* sort out the seconds */
  accumulator = sec_2 - sec_1;

  /* sort out the minutes */
  accumulator += (min_2 - min_1) * 60;

  /* sort out the hours */
  accumulator += (hour_2 - hour_1) * 3600;

  /* is date_1 <, > or == date_2 */
  if (year_1 < year_2) operation = 1;
  if (year_1 > year_2) operation = -1;
  if (! operation)
  {
    if (month_1 < month_2) operation = 1;
    if (month_1 > month_2) operation = -1;
  }
  if (! operation)
  {
    if (day_1 < day_2) operation = 1;
    if (day_1 > day_2) operation = -1;
  }

  /* sort out the dates */
  switch (operation)
  {
  case 0:	/* dates are the same */
    break;

  case 1:	/* date_1 < date_2 */
    dm = days_in_month (month_1, year_1);
    while (day_1 != day_2 || month_1 != month_2 || year_1 != year_2)
    {
      day_1 = day_1 + 1;
      if (day_1 > dm)
      {
        day_1 = 1;
        month_1 ++;
        if (month_1 > 12)
        {
          month_1 = 1;
          year_1 ++;
        }
        dm = days_in_month (month_1, year_1);
      }
      accumulator += 86400L;
    }
    break;

  case -1:	/* date_1 > date_2 */
    while (day_1 != day_2 || month_1 != month_2 || year_1 != year_2)
    {
      day_1 = day_1 - 1;
      if (day_1 < 1)
      {
        month_1 --;
        if (month_1 < 1)
        {
          month_1 = 12;
          year_1 --;
        }
  		day_1 = days_in_month (month_1, year_1);
      }
      accumulator -= 86400L;
    }
    break;
  }
  
  return accumulator;
  
}



