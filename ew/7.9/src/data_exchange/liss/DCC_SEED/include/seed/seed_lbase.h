/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_lbase.h 23 2000-03-05 21:49:40Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/05 21:49:40  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:09  lombard
 *     Initial revision
 *
 *
 *
 */

/* Structure/linked list to contain network/station/location/channel spans */

struct lbase_sdb {
	STDTIME	start;
	STDTIME	end;
	int	interesting;
	int	update;
	struct	lbase_sdb *next;
};

#include <seed/loadbase_proto.h>
