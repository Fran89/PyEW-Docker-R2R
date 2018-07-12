/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ntohl.c 502 2001-04-11 20:48:24Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2001/04/11 20:48:24  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: ntohl.c,v 1.4 1996/12/12 03:19:56 tholo Exp $";
#endif /* LIBC_SCCS and not lint */

/* ack, more machine dependencies 
   withers forced usage of local endian.h */
#include <rpc_nt/types.h>
#include <rpc_nt/endian.h>

#undef ntohl

u_int32_t
ntohl(x)
	u_int32_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *)&x;
	return (u_int32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#else
	return x;
#endif
}