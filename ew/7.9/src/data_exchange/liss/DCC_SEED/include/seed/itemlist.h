/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: itemlist.h 23 2000-03-05 21:49:40Z lombard $
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

#ifndef ITEMLIST_DEF

#define ITEMLIST struct _itemlist

struct _itemlist {
	struct	_itemlist	*next;
	char	*item;
};

#define ITEMLIST_DEF
#endif
