/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: itemlist.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:32  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:44  mark
 *     Initial checkin
 *
 *     Revision 1.1  2000/03/05 21:49:40  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:09  lombard
 *     Initial revision
 *
 *
 *
 */

#ifndef ITEMLIST_DEF

#define ITEMLIST struct _itemlist

struct _itemlist {
	struct	_itemlist	*next;
	char	*item;
};

#define ITEMLIST_DEF
#endif
