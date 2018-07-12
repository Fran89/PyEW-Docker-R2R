#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "geolib.h"

/* internal mode values for substitute_variables() */
#define LITERAL_TEXT		1
#define NORMAL_VAR			2
#define BRACKETED_VAR		3

/* private forward declarations */
static int do_subst (char *var_name, char *out_string, int out_length);


/*************************************************************************
 * substitute_variables
 *
 * Description: substitute environment variables in the same way as
 *				the shell
 *
 * Input parameters: in_string - string containing text and variables
 *					 remove_double_slash - if a variable ends with '/' and
 *										   the next character in the string
 *										   is '/', remove one of the '/' chars
 * Output parameters: out_string - substituted version
 * Returns: SV_OK - substitution worked OK,
 *		    SV_BAD_VAR - a variable could not be found (var. name
 *						 will be returned in out_string)
 *			SV_GRAMMER - there was a gramatical error
 *
 * Comments: variables may be inserted in strings as follows:
 * 1.) text$VARIABLE/more-text
 * 2.) text$VARIABLE
 * 3.) text$VARIABLE$VARIABLE2
 * 4.) text${VARIABLE}text
 *
 ************************************************************************/
int substitute_variables (char *in_string, int remove_double_slash,
						  char *out_string)

{

  int mode, ret_val, out_length, add_char, length;
  char var_name [50];


  /* initialise ... */
  mode = LITERAL_TEXT;
  ret_val = SV_RUNNING;
  strcpy (var_name, "");
  out_length = 0;

  /* until we are done ... */
  while (ret_val == SV_RUNNING)
  {
    /* test the next character */
    switch (*in_string)
    {
    case '$':
      switch (mode)
	  {
	  case NORMAL_VAR:
		out_length = do_subst (var_name, out_string, out_length);
	    if (out_length < 0) ret_val = SV_BAD_VAR;
		break;
	  case BRACKETED_VAR:
		ret_val = SV_GRAMMER;
		break;
      }
	  mode = NORMAL_VAR;
	  break;

    case '/':
	  add_char = GEO_FALSE;
      switch (mode)
	  {
	  case NORMAL_VAR:
		out_length = do_subst (var_name, out_string, out_length);
	    if (out_length < 0) ret_val = SV_BAD_VAR;
		else if (out_length > 0)
		{
		  if ((*(out_string + out_length -1) != '/') ||
			  (! remove_double_slash)) add_char = GEO_TRUE;
		}
		break;
	  case BRACKETED_VAR:
		ret_val = SV_GRAMMER;
		break;
	  case LITERAL_TEXT:
		if (out_length > 0)
		{
		  if ((*(out_string + out_length -1) != '/') ||
			  (! remove_double_slash)) add_char = GEO_TRUE;
		}
		else add_char = GEO_TRUE;
		break;
      }
	  mode = LITERAL_TEXT;
	  if (add_char)
	  {
		*(out_string + out_length) = '/';
		out_length ++;
	  }
      break;

    case '\0':
      switch (mode)
	  {
	  case NORMAL_VAR:
		out_length = do_subst (var_name, out_string, out_length);
	    if (out_length < 0) ret_val = SV_BAD_VAR;
		else ret_val = SV_OK;
		break;
	  case BRACKETED_VAR:
		ret_val = SV_GRAMMER;
		break;
	  case LITERAL_TEXT:
		ret_val = SV_OK;
		break;
      }
      break;

    case '{':
      switch (mode)
	  {
	  case NORMAL_VAR:
		if (strlen (var_name)) ret_val = SV_GRAMMER;
		else mode = BRACKETED_VAR;
		break;
	  case BRACKETED_VAR:
		ret_val = SV_GRAMMER;
		break;
	  case LITERAL_TEXT:
		*(out_string + out_length) = '{';
		out_length ++;
		break;
      }
      break;

    case '}':
      switch (mode)
	  {
	  case NORMAL_VAR:
	    ret_val = SV_GRAMMER;
		break;
	  case BRACKETED_VAR:
		out_length = do_subst (var_name, out_string, out_length);
	    if (out_length < 0) ret_val = SV_BAD_VAR;
		break;
	  case LITERAL_TEXT:
		*(out_string + out_length) = '}';
		out_length ++;
		break;
      }
	  mode = LITERAL_TEXT;
      break;

    default:
      switch (mode)
	  {
	  case NORMAL_VAR:
	  case BRACKETED_VAR:
		if (isalnum (*in_string) || (*in_string == '_'))
		{
		  length = strlen (var_name);
		  var_name [length ++] = *in_string;
		  var_name [length] = '\0';
		}
		else ret_val = SV_GRAMMER;
		break;
	  case LITERAL_TEXT:
		*(out_string + out_length) = *in_string;
		out_length ++;
		break;
      }
      break;
    }
    
    /* increment the input string */
    in_string ++;
  }

  if (ret_val == SV_OK) *(out_string + out_length) = '\0';
  return ret_val;

}

/************************************************************************
 * do_subst
 *
 * Description: substitute the given variable
 *
 * Input parameters: var_name - the variable
 *					 out_length - current length of the output string
 * Output parameters: out_string - the output string
 * Returns: the length of string if the substitution worked
 *					(in which case var_name is emptied)
 *			-1 if the substitution failed
 *					(in which case out_string has var_name copied to it)
 *
 * Comments:
 *
 ***********************************************************************/
static int do_subst (char *var_name, char *out_string, int out_length)

{

  char *ptr;


  ptr = getenv (var_name);
  if (! ptr)
  {
    strcpy (out_string, var_name);
    return -1;
  }

  strcpy (out_string + out_length, ptr);
  strcpy (var_name, "");
  return out_length + strlen (ptr);

}


