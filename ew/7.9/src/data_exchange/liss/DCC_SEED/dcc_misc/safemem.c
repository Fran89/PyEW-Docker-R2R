/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: safemem.c 44 2000-03-13 23:49:34Z lombard $
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

_SUB void *SafeAlloc(int siz)
{

  void *newptr;

  newptr = malloc(siz);
  if (newptr==NULL) 
    bombout(EXIT_INSFMEM,
	    "Could not SafeAlloc %d bytes",siz);

  return(newptr);
}

_SUB char *SafeAllocString(char *str)
{

  char *nstr;

  nstr = (void *) SafeAlloc(strlen(str)+1);
  strcpy(nstr,str);

  return(nstr);
}

_SUB void SafeFree(void *inptr)
{

  free(inptr);

}

_SUB void exit_nomem()
{

  exit(EXIT_INSFMEM);

}
