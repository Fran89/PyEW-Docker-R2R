/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: bombout.c 3363 2008-09-26 22:25:00Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2008/09/26 22:25:00  kress
 *     Fix numerous compile warnings and some tab-related fortran errors for linux compile
 *
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <dcc_config.h>

void bombout(int error, char *formstr, int p1, int p2, int p3, int p4, 
	     int p5, int p6, int p7, int p8)
{
  fflush(stdout);
  fflush(stderr);
  fprintf(stderr,"\nFatal error bombout:\n");
  fprintf(stderr,formstr,p1,p2,p3,p4,p5,p6,p7,p8);
  fprintf(stderr,"\n");
  fprintf(stderr,"%s-%s\n",DCCPACKAGE,DCCVERSION);
  exit(error);
}

void _bombout(int error, char *formstr, int p1, int p2, int p3, int p4, 
	      int p5, int p6, int p7, int p8)
{
  fflush(stdout);
  fflush(stderr);
  fprintf(stderr,"\nFatal error bombout:\n");
  fprintf(stderr,formstr,p1,p2,p3,p4,p5,p6,p7,p8);
  fprintf(stderr,"\n");
  fprintf(stderr,"%s-%s\n",DCCPACKAGE,DCCVERSION);
  exit(error);
}

void _bomb2(int error, char *formstr, int p1, int p2, int p3, int p4, 
	    int p5, int p6, int p7, int p8)
{
  fflush(stdout);
  fflush(stderr);
  fprintf(stderr,formstr,p1,p2,p3,p4,p5,p6,p7,p8);
  fprintf(stderr,"\n");
  exit(error);
}

void _bombdesc(char *file, char *date, int line)
{
  fflush(stdout);
  fflush(stderr);
  fprintf(stderr,"\n%s-%s Fatal error bombout(File=%s Date=%s Line=%d)\n",
	  DCCPACKAGE,DCCVERSION,file,date,line);

}
