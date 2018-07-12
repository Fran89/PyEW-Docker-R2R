/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readline.c 44 2000-03-13 23:49:34Z lombard $
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

_SUB BOOL ReadLine(FILE *ifile,char *buffer,int leng)
{
	int a;

	if (feof(ifile)) return(FALSE);

	if (fgets(buffer,leng,ifile)==NULL) return(FALSE);
	a = strlen(buffer);
	if (a>0&&buffer[a-1]=='\n') buffer[a-1]='\0';

	return(TRUE);
}
