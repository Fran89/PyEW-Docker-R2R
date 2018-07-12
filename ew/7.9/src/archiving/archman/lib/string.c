/* string - string functions that are not available on the host compiler */

#include <ctype.h>
#include <string.h>

#include "geolib.h"

/*************************************************************************
 * strupr
 *
 * Description: convert string to uppercase
 *
 * Input parameters: string - the string to convert
 * Output parmeters: none
 * Returns: string
 *
 * Comments:
 *
 *************************************************************************/
char *strupr (char *string)

{

  char *copy, byte;


  for (copy = string, byte = *copy; byte; byte = *copy)
  {
    if (byte >= 'a' && byte <= 'z') *copy = byte - 32;
    copy ++;
  }
  return string;

}

/*************************************************************************
 * strlwr
 *
 * Description: convert string to lowercase
 *
 * Input parameters: string - the string to convert
 * Output parmeters: none
 * Return: string
 *
 * Comments:
 *
 ************************************************************************/
char *strlwr (char *string)

{

  char *copy, byte;


  for (copy = string, byte = *copy; byte; byte = *copy)
  {
    if (byte >= 'A' && byte <= 'Z') *copy = byte + 32;
    copy ++;
  }
  return string;

}

/*************************************************************************
 * stricmp2
 *
 * Description: case independant strcmp
 *
 * Input parameters: string1, string2 - the strings to compare
 * Output parameters: none
 * Returns: -1 if string1 < string2
 *		     0 if string1 == string2
 *			+1 if string1 > string2
 *
 * Comments:
 *
 ********************************************************************/
int stricmp2 (char *string1, char *string2)

{

  char test1, test2;
  

  while (*string1) 
  {
    if (*string1 >= 'a' && *string1 <= 'z') test1 = *string1 - 32;
    else test1 = *string1;
    if (*string2 >= 'a' && *string2 <= 'z') test2 = *string2 - 32;
    else test2 = *string2;
    if (test1 < test2) return -1;
    if (test1 > test2) return 1;
    string1 ++;
    string2 ++;
  }

  if (*string2) return -1;
  return 0;

}

/**************************************************************************
 * stristr
 *
 * Description: case independant strstr ()
 *
 * Input parameters: string - the string the search in
 *		     sub - the substring to search with
 * Output parameters: none
 * Returns: The address of the substring within the string
 *	    OR NULL if not found
 *
 * Comments:
 *
 *************************************************************************/
char *stristr (char *string, char *sub)

{

  int string_len, sub_len, found, count, count2;
  char *string_ptr, *sub_ptr, char_1, char_2;

  string_len = strlen (string);
  sub_len = strlen (sub);

  if (sub_len > string_len) return (char *) 0;

  for (count=0; count<=string_len - sub_len; count++)
  {
    string_ptr = string ++;
    sub_ptr = sub;
    found = GEO_TRUE;
    for (count2=0; count2<sub_len && found; count2++)
    {
      char_1 = *string_ptr ++;
      char_2 = *sub_ptr ++;
      if (char_1 != char_2)
      {
        if (char_1 >= 'a' && char_1 <= 'z') char_1 = char_1 - 32;
        if (char_2 >= 'a' && char_2 <= 'z') char_2 = char_2 - 32;
		if (char_1 != char_2) found = GEO_FALSE;
      }
    }
    if (found) return string -1;
  }

  return (char *) 0;

}

/**************************************************************************
 * strlist
 *
 * Description: find a string from within a token separated list
 *		of strings
 *
 * Input parameters: string - the string the search in
 *		     sub - the substring to search with
 *		     delim - the token delimiter
 * Output parameters: none
 * Returns: TRUE if sub-string found, FALSE otherwise
 *
 * Comments: eg. to find "one" in "three!one!two" with '!' delimiter
 *		 returns FALSE
 *		 to find "a" in "!A!B!C" returns FALSE
 *	     etc.
 *
 *************************************************************************/
int strlist (char *string, char *sub, int delim)

{

  int string_len;
  char *string_ptr, *ptr, del_string [2];

  /* make a local copy of the string */
  string_len = strlen (string);
  string_ptr = malloc (string_len +1);
  if (! string_ptr) return GEO_FALSE;
  strcpy (string_ptr, string);

  /* work through the string */
  del_string [0] = delim;
  del_string [1] = '\0';
  for (ptr = strtok (string_ptr, del_string); ptr; ptr = strtok ((char *) 0, del_string))
  {
    if (! strcmp (ptr, sub))
    {
      free (string_ptr);
      return GEO_TRUE;
    }
  }

  /* if you get here then the string was not found */
  free (string_ptr);
  return GEO_FALSE;

}

/**************************************************************************
 * strilist
 *
 * Description: find a string from within a token separated list
 *		of strings (case independant)
 *
 * Input parameters: string - the string the search in
 *		     sub - the substring to search with
 *		     delim - the token delimiter
 * Output parameters: none
 * Returns: TRUE if sub-string found, FALSE otherwise
 *
 * Comments: eg. to find "one" in "three!one!two" with '!' delimiter
 *		 returns TRUE
 *		 to find "a" in "!A!B!C" returns TRUE
 *	     etc.
 *
 *************************************************************************/
int strilist (char *string, char *sub, int delim)

{

  int string_len;
  char *string_ptr, *ptr, del_string [2];

  /* make a local copy of the string */
  string_len = strlen (string);
  string_ptr = malloc (string_len +1);
  if (! string_ptr) return GEO_FALSE;
  strcpy (string_ptr, string);

  /* work through the string */
  del_string [0] = delim;
  del_string [1] = '\0';
  for (ptr = strtok (string_ptr, del_string); ptr; ptr = strtok ((char *) 0, del_string))
  {
    if (! stricmp2 (ptr, sub))
    {
      free (string_ptr);
      return GEO_TRUE;
    }
  }

  /* if you get here then the string was not found */
  free (string_ptr);
  return GEO_FALSE;

}

#ifndef WIN32
/*************************************************************************
 * strnicmp
 *
 * Description: case independant string compare with maximum length
 *
 * Input parameters: string1, 2 - the strings to conmpare
 *		     maxlen - the maximum length to compare
 * Output parmeters: none
 * Returns: -1 for string1 <  string 2,
 *	     0 for string1 == string2,
 *           1 for string1 >  string 2,
 *
 * Comments:
 *
 *************************************************************************/
int strnicmp (char *string1, char *string2, int maxlen)

{

  char test1, test2;

  while (*string1)
  {
    if (maxlen <= 0) return 0;
    if (*string1 >= 'a' && *string1 <= 'z') test1 = *string1 - 32;
    else test1 = *string1;
    if (*string2 >= 'a' && *string2 <= 'z') test2 = *string2 - 32;
    else test2 = *string2;
    if (test1 < test2) return -1;
    if (test1 > test2) return 1;
    string1 ++;
    string2 ++;
    maxlen --;
  }

  if (*string2) return -1;
  return 0;

}

#endif
/*****************************************************************************
 * trim_leading_whitespace
 *
 * Description: trim leading whitespace from a string
 *
 * Input parameters: string - the string to trim
 * Output parameters: none
 * Returns: pointer to the new start of the string
 *
 * Comments: string is not modified in any way
 *
 ****************************************************************************/
char *trim_leading_whitespace (char *string)

{

  while (*string && strchr (" \t", *string)) string ++;
  return string;

}

/*****************************************************************************
 * trim_trailing_whitespace
 *
 * Description: trim trailing whitespace from a string
 *
 * Input parameters: string - the string to trim
 * Output parameters: string is modified by inserting NULLs
 * Returns: the string
 *
 * Comments: string is modified
 *
 ****************************************************************************/
char *trim_trailing_whitespace (char *string)

{

  char *ptr;
  
  
  for (ptr = string + strlen (string) -1; ptr >= string; ptr --)
  {
    if (strchr (" \t", *ptr)) *ptr = '\0';
    else break;
  }
  return string;

}

/*********************************************************************
 * strtok2
 *
 * Description: version of strtok that returns for every separator
 *              character found - back to back seps return with an
 *              empty string
 *
 * Input parameters: string - the string to parse OR NULL to continue
 *                            with the previus string
 *                   seps - list of separators
 * Output parameters: string is modified as in strtok()
 * Returns: the next string segment OR NULL when string is complete
 *********************************************************************/
char *strtok2 (char *string, char *seps)

{

  char *ret_val, *sep_ptr;

  static int last_char_is_sep = GEO_FALSE;
  static char *string_ptr = "";
  
  
  /* have we got a new string to parse */
  if (string)
  {
    last_char_is_sep = GEO_FALSE;
    string_ptr = string;
  }

  /* was the last char a separator at the end of a string - if so we need
   * to return an empty sub-string as the last thing from this string */
  if (last_char_is_sep)
  {
    last_char_is_sep = GEO_FALSE;
    return "";
  }

  /* record where we are in the string, so we can return this to the caller */
  ret_val = string_ptr;

  /* find the next separator character */
  while (*string_ptr)
  {
    for (sep_ptr=seps; *sep_ptr; sep_ptr++)
    {
      if (*sep_ptr == *string_ptr)
      {
        /* found a separator - insert a null and move on the the next char */
        *string_ptr = '\0';
        string_ptr ++;
        
        /* if the next char is the end of the string, flag that we need
         * to return an empty string next time */
        if (! *string_ptr) last_char_is_sep = GEO_TRUE;
        return ret_val;
      }
    }
    string_ptr ++;
  }
  
  /* if you get here then the string was exhausted */
  if (*ret_val) return ret_val;
  return 0;

}

/*****************************************************************************
 * make_ordinal_number
 *
 * Description: convert an ordinal number ot its string equivalent
 *
 * Input parameters: number - the value to convert
 * Output parameters: none
 * Returns: a static string representing the ordinal value
 *
 * Comments:
 *
 ****************************************************************************/
char *make_ordinal_number (int number)

{

  int unit, tens;

  static char string [20];


  /* find the units and tens values from the number */
  unit = number % 10;
  tens = (number % 100) / 10;

  /* fake the unit for the teens, which are always 'th' */
  if (tens == 1) unit = 9;

  /* calculate the string */
  switch (unit)
  {
  case 1:  sprintf (string, "%dst", number); break;
  case 2:  sprintf (string, "%dnd", number); break;
  case 3:  sprintf (string, "%drd", number); break;
  default: sprintf (string, "%dth", number); break;
  }

  return string;

}
