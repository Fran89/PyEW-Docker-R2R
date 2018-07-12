/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_misc.c 44 2000-03-13 23:49:34Z lombard $
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
#include <string.h>
#include <dcc_seed.h>

/*
 *	
 *	Remove spaces from keys
 *	
 */

#define MAXKEYLEN 50
char _searchkey[MAXKEYLEN];

_SUB char *FixKey(char *inkey)
{
	char *oky,*iky;

	if (inkey==NULL) return(NULL);

	for (oky=_searchkey,iky=inkey; *iky!='\0'; iky++) {
		if (isspace(*iky)) continue;
		*oky++ = toupper(*iky);
	}

	*oky++ = '\0';

/*	printf("Fixkey(%s)\n",_searchkey);	*/

	return(_searchkey);
}

_SUB char *OKNull(char *inc)
{
  if (inc!=NULL) return(inc);
  return("");
}

