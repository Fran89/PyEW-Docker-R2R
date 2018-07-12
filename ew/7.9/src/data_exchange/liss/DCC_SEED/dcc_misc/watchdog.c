/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: watchdog.c 44 2000-03-13 23:49:34Z lombard $
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

#include <unistd.h>

#include <stdio.h>

#include <time.h>

#include <string.h>

FILE *fop=NULL;
  
_SUB void watchdog(char *pidfile)
{

  time_t tim = time(NULL);

  if (fop==NULL) {
    char actpid[200];

    sprintf(actpid,"/tmp/%s.pid",pidfile);

    fop = fopen(actpid,"w");
    if (fop==NULL) 
      bombout(EXIT_ABORT,"Cannot write pid file %s",actpid);

  }

  fprintf(fop,"%d %ld",(int) getpid(),(long) tim);
  fflush(fop);

  rewind(fop);

  return;
}
