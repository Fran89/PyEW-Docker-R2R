#pragma ident "$Id: sh.c 1331 2004-03-16 23:32:39Z kohler $"
/*======================================================================
 *
 *  Decode a SH packet
 *
 *====================================================================*/
#include "private.h"

BOOL reftek_sh(struct reftek_sh *dest, UINT8 *src)
{
    reftek_com(src, &dest->exp, &dest->unit, &dest->seqno, &dest->tstamp);
    return TRUE;
}

/* Revision History
 *
 * $Log$
 * Revision 1.1  2004/03/16 23:17:19  kohler
 * Initial revision
 *
 * Revision 1.2  2002/01/18 17:55:57  nobody
 * replaced WORD, BYTE, LONG, etc macros with size specific equivalents
 * changed interpretation of unit ID from BCD to binary
 *
 * Revision 1.1.1.1  2000/06/22 19:13:09  nobody
 * Import existing sources into CVS
 *
 */
