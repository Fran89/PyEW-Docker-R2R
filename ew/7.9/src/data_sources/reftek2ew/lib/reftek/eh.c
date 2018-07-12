#pragma ident "$Id: eh.c 2399 2006-08-04 16:05:50Z paulf $"
/*======================================================================
 *
 *  Decode a EH record
 *
 	Revised  :
		12Jul06	 (rs) add routine to get rate from eh for reftek2ew
		14Sep05  (pld) add support for all defined triggers
		03Sep05  (pld) change sample rates to float
					(pld) handle Steim2 data format
*====================================================================*/
#include "private.h"

/* Offsets to the various pieces */

#define EVTN_OFF  16  /* event number           */
#define STRM_OFF  18  /* stream id              */
#define FRMT_OFF  23  /* data format descriptor */
#define SINT_OFF  88  /* sample rate            */
#define TTYP_OFF  92  /* trigger type           */
#define TRON_OFF  96  /* trigger time           */
#define TOFS_OFF 112  /* time of first sample   */

BOOL reftek_eh(struct reftek_eh *dest, UINT8 *src)
{
REAL32 srate;
UINT16 urate, yr, da, hr, mn, sc, ms;

/* Load the common header */

    reftek_com(src, &dest->exp, &dest->unit, &dest->seqno, &dest->tstamp);

/* Load the record specific parts */

    dest->evtno  = (UINT16) utilBcdToUint32(src + EVTN_OFF, 4, 0); 
    dest->stream = (UINT16) utilBcdToUint32(src + STRM_OFF, 2, 0);

    switch (utilBcdToUint32(src + FRMT_OFF, 2, 0)) {
      case 16:
        dest->format = REFTEK_F16;
        break;

      case 32:
        dest->format = REFTEK_F32;
        break;

      case 120:
        dest->format = REFTEK_FC0;
        break;

      case 122:
        dest->format = REFTEK_FC2;
        break;

      default:
        errno = EINVAL;
        return FALSE;
    }

    sscanf((char *) (src + SINT_OFF), "%4hu", &urate);
    if (urate == 0)
    sscanf((char *) (src + SINT_OFF), "%4f", &srate);
    else
	    srate = (REAL32)urate;
    dest->sint = (REAL32) 1.0 /  srate;

    if (memcmp(src + TTYP_OFF, "CON", 3) == 0) {
        dest->trgtype = REFTEK_TRGCON;
    } else if (memcmp(src + TTYP_OFF, "CRS", 3) == 0) {
        dest->trgtype = REFTEK_TRGCRS;
    } else if (memcmp(src + TTYP_OFF, "EVT", 3) == 0) {
        dest->trgtype = REFTEK_TRGEVT;
    } else if (memcmp(src + TTYP_OFF, "EXT", 3) == 0) {
        dest->trgtype = REFTEK_TRGEXT;
    } else if (memcmp(src + TTYP_OFF, "LVL", 3) == 0) {
        dest->trgtype = REFTEK_TRGLVL;
    } else if (memcmp(src + TTYP_OFF, "RAD", 3) == 0) {
        dest->trgtype = REFTEK_TRGRAD;
    } else if (memcmp(src + TTYP_OFF, "TIM", 3) == 0) {
        dest->trgtype = REFTEK_TRGTIM;
    } else if (memcmp(src + TTYP_OFF, "TML", 3) == 0) {
        dest->trgtype = REFTEK_TRGTML;
    } else if (memcmp(src + TTYP_OFF, "VOT", 3) == 0) {
        dest->trgtype = REFTEK_TRGVOT;
    } else if (memcmp(src + TTYP_OFF, "CMD", 3) == 0) {
        dest->trgtype = REFTEK_TRGCMD;
    } else if (memcmp(src + TTYP_OFF, "CAL", 3) == 0) {
        dest->trgtype = REFTEK_TRGCAL;
    } else {
        errno = EINVAL;
        return FALSE;
    }

    sscanf((char *) (src + TRON_OFF), "%4hd%3hd%2hd%2hd%2hd%3hd",
        &yr, &da, &hr, &mn, &sc, &ms
    );
    dest->on = util_ydhmsmtod(yr, da, hr, mn, sc, ms);

    sscanf((char *) (src + TOFS_OFF), "%4hd%3hd%2hd%2hd%2hd%3hd",
        &yr, &da, &hr, &mn, &sc, &ms
    );
    dest->tofs = util_ydhmsmtod(yr, da, hr, mn, sc, ms);

    return TRUE;

}


/****************************************************************************
	Purpose:	Get rate from header packet
   Returns:	Nothing
   Revised:
		06Jul12	---- (rs) provide means to extract rate from eh
===========================================================================*/
REAL32 reftek_eh_rate(UINT8 *src)
	{
	UINT16	urate;
	REAL32	srate;
 
	sscanf((char *) (src + SINT_OFF), "%4hu", &urate);
	if (urate == 0)
	sscanf((char *) (src + SINT_OFF), "%4f", &srate);
	else
	   srate = (REAL32)urate;
	   
	return srate;

	}

/* Revision History
 *
 * $Log$
 * Revision 1.2  2006/08/04 16:05:50  paulf
 * 1.7 reftek2ew changes from Roberta Stavely of Reftek Inc
 *
 * Revision 2.0  2005/10/07 21:30:46  pdavidson
 * Finish Steim2 support.
 * Bug fixes in 0.1 sps, aux data (stream 9) support.
 * Handle all trigger types in EH/ET decoding.
 * Promote archive API, modified programs to v2.0.
 * DOES NOT INCLUDE modifications to RTP log or client protocol.
 *
 * Revision 1.3  2005/09/03 21:52:31  pdavidson
 *
 * Minimal modifications to support Steim2 recording format, 0.1 sps sample
 * rate and FD packets.
 *
 * Revision 1.2  2002/01/18 17:55:56  nobody
 * replaced WORD, BYTE, LONG, etc macros with size specific equivalents
 * changed interpretation of unit ID from BCD to binary
 *
 * Revision 1.1.1.1  2000/06/22 19:13:09  nobody
 * Import existing sources into CVS
 *
 */
