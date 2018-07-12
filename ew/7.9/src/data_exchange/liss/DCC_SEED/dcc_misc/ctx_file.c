/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ctx_file.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

/* POSIX - Phase 1 - 29-May-92/SH */

#include <dcc_std.h>

_SUB FILE *open_ctx_file()
{

  char cfile[200];

#ifdef VMS
  sprintf(cfile,"sys$login:ctx_%x.tmp",getpid());
#else
  sprintf(cfile,"/tmp/ctx_%x",(int) getppid()); /* Get parent id */
#endif

  return(fopen(cfile,"w"));

}
