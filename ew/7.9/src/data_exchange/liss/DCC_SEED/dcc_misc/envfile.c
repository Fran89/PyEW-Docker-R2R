/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: envfile.c 44 2000-03-13 23:49:34Z lombard $
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

_SUB void envfile(char *envvar, char *fname, char *outname)
{

  char sbuf[256],*genv,ic;
  int tlen;

  genv = getenv(envvar);
  if (genv==NULL) 
    bombout(EXIT_ABORT,"Environment variable %s not set",envvar);

  strcpy(sbuf,genv);
  tlen = strlen(sbuf);
  if (tlen<=0) 
    ic = '\0';
  else
    ic = sbuf[tlen-1];

  if (ic!='\0')
    strcat(sbuf,"/");

  strcat(sbuf,fname);

  strcpy(outname,sbuf);

}

_SUB void catfile(char *envvar, char *fname, char *outname)
{

  char sbuf[256],ic;
  int tlen;

  strcpy(sbuf,envvar);
  tlen = strlen(sbuf);
  if (tlen<=0) 
    ic = '\0';
  else
    ic = sbuf[tlen-1];

  if (ic!='\0')
    strcat(sbuf,"/");

  strcat(sbuf,fname);

  strcpy(outname,sbuf);

}
