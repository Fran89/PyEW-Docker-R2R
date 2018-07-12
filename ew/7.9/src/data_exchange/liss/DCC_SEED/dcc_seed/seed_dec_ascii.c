/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_dec_ascii.c 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:06:20  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>
#include <dcc_seed.h>
#include <seed_data.h>
#include <dcc_misc.h>

_SUB int SH_Decode_ASCII(long data[], SEED_DATA *inrec,int swdata)
{

  register int lp,ct,ds;
  register DCC_LONG *oarray;
  register UDCC_BYTE *bytarr;
  register int numsam;

  numsam = SH_Number_Samples(inrec);
  
  bytarr= (UDCC_BYTE *) inrec;
  ds = SH_Start_Data(inrec);
  numsam = SH_Number_Samples(inrec);

  if (ds==0) return(-1);
  if (numsam==0) return(0);

  oarray=(DCC_LONG *) data;
  ct=0;
  
  for (lp=0; lp<numsam; lp++) {

    *oarray++ = bytarr[ds+lp];
    
    ct++;
  }
  return(ct);
}



