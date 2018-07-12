/* data_utils.c utilities for working with SDAS data
 *
 * This file contain standalone utilities - no setup is required before
 * using any of the functions in this file
 *
 * S. Flower, May 2002 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#ifdef _SOLARIS
#include <dirent.h>
#include <sys/wait.h>
#include <sys/uio.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "geolib.h"
#include "sdas.h"

/***************************************************************************
 * change_missing_data_value
 *
 * Description: change the value used to represent missing data in
 *              a data array
 *
 * Input parameters: old_val - the old value used for missing data
 *                   new_val - the new value
 *                   data - the data array
 *                   n_points - the size of the array
 * Output parameters: data is modified
 * Returns: none
 *
 * Comments:
 *
 ***************************************************************************/
void change_missing_data_value (long old_val, long new_val,
                                long *data, int n_points)
                                
{

  int count;
  long *data_ptr;
  
  
  for (count=0, data_ptr=data; count<n_points; count++, data_ptr++)
  {
    if (*data_ptr == old_val) *data_ptr = new_val;
  }

}

/***************************************************************************
 * swap_byte_order
 *
 * Description: swap byte order (big-endian <-> liitle-endian) in
 *              a data (long) array
 *
 * Input parameters: data - the data array
 *                   n_points - the size of the array
 * Output parameters: data is modified
 * Returns: none
 *
 * Comments:
 *
 ***************************************************************************/
void swap_byte_order (long *data, int n_points)
                                
{

  int count;
  long *data_ptr;
  char store [2], *ptr;
  
  
  for (count=0, data_ptr=data; count<n_points; count++, data_ptr++)
  {
    ptr = (char *) data_ptr;
    store [0] = *ptr;
    store [1] = *(ptr +1);
    *ptr = *(ptr +3);
    *(ptr +1) = *(ptr +2);
    *(ptr +2) = store [1];
    *(ptr +3) = store [0];
  }

}

/***************************************************************************
 * fill_with_missing_data
 *
 * Description: fill an array with missing data flags
 *
 * Input parameters: miss_val - the missing data value
 *                   data - the data array
 *                   n_points - the size of the array
 * Output parameters: data is modified
 * Returns: none
 *
 * Comments:
 *
 ***************************************************************************/
void fill_with_missing_data (long miss_val, long *data, int n_points)
                                
{

  int count;
  
  
  for (count=0; count<n_points; count++) *(data + count) = miss_val;

}

/***************************************************************************
 * make_sdas_data_filename
 *
 * Description: make the name for an SDAS data file
 *
 * Input parameters: dir - the directory for the file
 *                   channel - the channel
 *                   year, month, day - the date
 *                   hour - the time
 * Output parameters: filename - the filename
 * Returns: none
 *
 * Comments:
 *
 **************************************************************************/
void make_sdas_data_filename (char *dir, int channel, int year, int month,
				   			  int day, int hour, char *filename)

{

  strcpy (filename, "");
  if (dir)
  {
    if (*dir)
    {
      strcpy (filename, dir);
      terminate_path (filename, GEO_TRUE);
    }
  }
  sprintf (&filename[strlen(filename)], "chan_%04d/%04d_%02d_%02d_%02d.dat",
           channel, year, month, day, hour);

}

/***************************************************************************
 * make_sdas_event_filename
 *
 * Description: make the name for an SDAS event file
 *
 * Input parameters: dir - the directory for the file
 *                   year, month, day - the date
 * Output parameters: filename - the filename
 * Returns: none
 *
 * Comments:
 *
 **************************************************************************/
void make_sdas_event_filename (char *dir, int year, int month,
				   	 		   int day, char *filename)

{

  strcpy (filename, dir);
  terminate_path (filename, GEO_TRUE);
  sprintf (&filename [strlen (filename)], "%04d_%02d_%02d.event",
           year, month, day);

}

/***************************************************************************
 * make_sdas_stats_filename
 *
 * Description: make the name for an SDAS data statistics file
 *
 * Input parameters: dir - the directory for the file
 *                   channel - the channel
 *                   year, month, day - the date
 *                   hour - the time for an hourly file, or -1 for a daily file
 * Output parameters: filename - the filename
 * Returns: none
 *
 * Comments:
 *
 **************************************************************************/
void make_sdas_stats_filename (char *dir, int channel, int year, int month,
				   	 		   int day, int hour, char *filename)

{

  strcpy (filename, "");
  if (dir)
  {
    if (*dir)
    {
      strcpy (filename, dir);
      terminate_path (filename, GEO_TRUE);
    }
  }
  if (hour >= 0)
    sprintf (&filename[strlen(filename)], "chan_%04d/%04d_%02d_%02d_%02d.stats",
             channel, year, month, day, hour);
  else
    sprintf (&filename[strlen(filename)], "chan_%04d/%04d_%02d_%02d.stats",
             channel, year, month, day);

}

/**************************************************************************
 * calc_n_samples
 *
 * Description: given a sample rate and duration, calculate the length
 *              of the array needed to hold data
 *
 * Input parameters: sr - in samples / hour
 *                   duration - in milli-seconds
 * Output parameters: none
 * Returns: the array length NOT THE SIZE IN BYTES (multiply by sizeof(long)
 *          for the size in bytes)
 *
 * Comments:
 *
 **************************************************************************/
int calc_n_samples (int sr, long duration)

{


  /* the following old code used GMP and had a memory leak in the
   * GMP division function - it has been replaced with conversion
   * to doubles for the arithmetic calculation */
/*  static int first = GEO_TRUE; */
/*  static MP_INT sample_rate, result1, result2; */
    

  /* initialise */
/*  if (first) */
/*  { */
/*    mpz_init (&sample_rate); */
/*    mpz_init (&result1); */
/*    mpz_init (&result2); */
/*    first = GEO_FALSE; */
/*  } */

  /* perform the calculation */
/*  mpz_set_si (&sample_rate, sr); */
/*  mpz_mul_ui (&result1, &sample_rate, (unsigned long) duration); */
/*  mpz_mdiv_ui (&result2, &result1, 3600000ul); */
/*  return mpz_get_si (&result2); */

  return (int) (((double) sr * (double) duration) / 3600000.0);

}
