#include "geolib.h"

/****************************************************************************
 * check_number
 *
 * Description: check that a character string contains only valid numbers
 *
 * Input parameters: string - the string to check
 *		     negative - flag to allow negative values
 *		     real - flag to allow real (non-integer) numbers
 * Output parameters: none
 * Returns: TRUE for number OK, FALSE otherwise
 *
 * Comments:
 *
 ***************************************************************************/
int check_number (char *string, int negative, int real)

{

  int count, dot_count = 0, trailing_space = GEO_FALSE;

  /* trim leading white space */
  while (*string == ' ' || *string == '\t') string ++;

  /* check the number */
  for (count=0; *string; count ++)
  {
    switch (*string ++)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (trailing_space) return GEO_FALSE;
      break;
    case '-':
      if (! negative) return GEO_FALSE;
    case '+':
      if (count || trailing_space) return GEO_FALSE;
      break;
    case '.':
      if (trailing_space) return GEO_FALSE;
      if (! real || dot_count ++) return GEO_FALSE;
      break;
    case ' ':
    case '\t':
      trailing_space = GEO_TRUE;
      break;
    default:
      return GEO_FALSE;
    }
  }

  return GEO_TRUE;

}
