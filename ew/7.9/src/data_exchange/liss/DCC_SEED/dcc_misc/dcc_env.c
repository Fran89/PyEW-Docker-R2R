/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_env.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_misc.h>

_SUB char *dcc_env(char *inenv)
{

  char *getval;

  getval = getenv(inenv);

  if (getval==NULL)
    bombout(EXIT_ABORT,"Required DCC Environment Variable %s not set!",
	    inenv);

  return(getval);
}
