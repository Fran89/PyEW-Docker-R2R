/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: select_proto.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:32  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:45  mark
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

