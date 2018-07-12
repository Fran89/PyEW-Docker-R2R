
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: cksum.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:20:06  lucky
 *     Initial revision
 *
 *
 */


#ifndef lint
static char rcsid[] = "$Header$";
#endif 

#include <sys/types.h>

/*
 * Compute checksum to characterize a character string
 */
unsigned int
cksum(s)
	char	*s;
{
	unsigned int	sum;
	int	c;

	sum = 0;
	while ((c = *s++) != 0) {
		if (sum&01)
			sum = (sum>>1) + 0x8000;
		else
			sum >>= 1;
		sum += c;
		sum &= 0xFFFF;
	}
	return sum;
}
