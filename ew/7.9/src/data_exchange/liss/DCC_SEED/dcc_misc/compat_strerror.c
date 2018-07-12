/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: compat_strerror.c 44 2000-03-13 23:49:34Z lombard $
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

#ifndef HAVE_STRERROR

/* Compatibility routines */

extern char *sys_errlist[];
extern int sys_nerr;

static char rets[100];

char *strerror(int errnum)
{

  if (errnum<0 || errnum>sys_nerr) {
    sprintf(rets,"Error out of range (value=%d)",errnum);
    return(rets);
  }

  return(sys_errlist[errnum]);

}

#endif

    
