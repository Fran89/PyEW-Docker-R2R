/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_select.h 2192 2006-05-25 15:32:13Z paulf $
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
 *     Revision 1.2  2003/06/16 22:04:03  patton
 *     Fixed Microsoft WORD typedef issue
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

struct	sel3_stat {
	struct sel3_stat *next;
	char            *network;
	char		*station;
	unsigned int	addnot : 1;
	unsigned int	class  : 1;
};

struct sel3_chan {
	struct sel3_chan *next;
	char		*location;
	char		*channel;
	unsigned int	addnot : 1;
};

struct sel3_date {
	struct sel3_date *next;
	UDCC_LONG		sjday,ejday;
	STDTIME		sdate,edate;
	unsigned int	addnot : 1;
};

extern STDTIME _ztime;

char *filename;

struct sel3_chain {
	struct sel3_chain *next;
	struct sel3_stat *root_stat,*tail_stat;
	struct sel3_chan *root_chan,*tail_chan;
	struct sel3_date *root_date,*tail_date;	
	ITEMLIST	*taglist;
};

struct sel3_root {
  struct sel3_chain *root_chain,*tail_chain;
  UDCC_LONG jdmax,jdmin;
};


#include <seed/select_proto.h>
