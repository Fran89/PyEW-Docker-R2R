/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: wildmatch.c 44 2000-03-13 23:49:34Z lombard $
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

#define MAXNAMLEN 1025

/*
 * match -- match wildcard pattern to string
 *    allows multiple '*'s and works without backtracking
 *    upper and lower case considered equivalent
 *    written by Jack Rouse
 *    working without backtracking is cute, but is this usually going
 *       to be the most efficient method?
 */
_SUB BOOL WildMatch(char *pattern, char *target)
{
	int link[MAXNAMLEN];		/* list of matches to try in pattern */
	register int first, last;	/* first and last items in list */
	register int here, next;	/* current and next list items */
	char lowch;			/* current target character */

	/* start out trying to match at first position */
	first = last = 0;
	link[0] = -1;

	/* go through the target */
	for (; *target; ++target)
	{
		/* get lowercase target character */
		lowch = tolower(*target);

		/* go through all positions this round and build next round */
		last = -1;
		for (here = first; here >= 0; here = next)
		{
			next = link[here];
			switch (pattern[here])
			{
			case '*':
				/* try match at here+1 this round */
				/*!!!check needed only if "**" allowed? */
				if (next != here + 1)
				{
					link[here + 1] = next;
					next = here + 1;
				}
				/* retry match at here next round */
				break;
			default:
				if (tolower(pattern[here]) != lowch)
					continue;
				/* matched, fall through */
			case '?':
				/* try match at here+1 next round */
				++here;
				break;
			}
			/* try match at here value next round */
			if (last < 0)
				first = here;
			else
				link[last] = here;
			last = here;
		}
		/* if no positions left, match failed */
		if (last == -1) return(0);
		/* terminate list */
		link[last] = -1;
	}

	/* at end of target, skip empty matches */
	while (pattern[last] == '*')
		++last;

	return(pattern[last] == '\0');
}
