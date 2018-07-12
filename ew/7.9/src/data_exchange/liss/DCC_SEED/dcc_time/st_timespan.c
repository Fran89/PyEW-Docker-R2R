/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_timespan.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>

/*  Spanning cases

==============================================================================
NIL		AAAAAAAAAAAAAAAAAA
					BBBBBBBBBBBBBBBBBBBBBBB
EMPTY
==============================================================================
ACB		AAAAAAAAAAAAAAAAAAAA
		      BBBBBBBBBB
B		      ----------
==============================================================================
BCA		      AAAAAAAAAAA
		BBBBBBBBBBBBBBBBB
A		      -----------
==============================================================================
ALB		AAAAAAAAAAAAAAAA
			BBBBBBBBBBBBBBBBBBB
SB-EA		        --------
==============================================================================
BLA		        AAAAAAAAAAAAAAAA
SA-EB		BBBBBBBBBBBBBBB
                        -------
==============================================================================


Also..... (big bug discovered 27-Mar-95) that

           AAAAAAAABBBBBBBB  (where startb and enda are the same) 
         is NIL!!!

*/


_SUB int ST_TimeSpan(STDTIME starta,STDTIME enda,
		     STDTIME startb,STDTIME endb,
		     STDTIME *retstart,STDTIME *retend)
{

  STDTIME copy;
  int ord=0;


  /* Make a's start time first */

  if (ST_TimeComp(starta,startb)>0) {
    copy = startb;
    startb = starta;
    starta = copy;
    copy = endb;
    endb = enda;
    enda = copy;
    ord = 1;
  }

  if (ST_TimeComp(startb,enda)>=0) {   /* must >= not > */
    if (retstart) *retstart = ST_Zero();
    if (retend) *retend = ST_Zero();
    return(ST_SPAN_NIL);	/* No intersection */
  }

  if (ST_TimeComp(enda,endb)>0) {
    if (retstart) *retstart = startb;
    if (retend) *retend = endb;
    return(ord?ST_SPAN_BCA:ST_SPAN_ACB);
  }

  if (retstart) *retstart = startb;
  if (retend) *retend = enda;
	
  return(ord?ST_SPAN_BLA:ST_SPAN_ALB);

}
