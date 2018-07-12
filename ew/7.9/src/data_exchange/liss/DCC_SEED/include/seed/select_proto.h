/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: select_proto.h 23 2000-03-05 21:49:40Z lombard $
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

/* Processing file sel_mem.c */

	VOID SelectInit(struct sel3_root *root)
;
 VOID SelectList(struct sel3_root *root)
;
	VOID SelectLoadup(struct sel3_root *root,char *infilename)
;
 BOOL SelectStatInteresting(struct sel3_root *root,
				char *network,char *station,
				char *location,char *channel,
				ITEMLIST **taglist)
;
 BOOL SelectChainInteresting(struct sel3_chain *root,
				 char *network,char *station,
				 char *location,char *channel,
				 ITEMLIST **taglist)
;
 BOOL SelectDateInteresting(struct sel3_root *root,
				STDTIME btime,STDTIME etime,
				char *network,char *station,
				char *location,char *channel,
				ITEMLIST **taglist)
;

/* Found 6 functions */

