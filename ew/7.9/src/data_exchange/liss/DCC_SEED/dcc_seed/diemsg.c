/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: diemsg.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_seed.h>

_SUB BOOL diemsg(char *file, int line, char *date)
{

	fprintf(stderr,"Routine aborted %s (%d) %s\n",file,line,date);

	return(FALSE);

}
