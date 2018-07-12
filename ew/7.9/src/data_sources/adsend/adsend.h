/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: adsend.h 1410 2004-04-20 16:26:32Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/04/20 16:26:32  kohler
 *     Modified adsend to produce TYPE_TRACEBUF2 messages, which contain location
 *     codes in the headers.  The configuration file, adsend.d, must contain
 *     location codes in the channel specifications.  Also, cleaned up some error
 *     reporting.  WMK 4/20/04
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

//    adsend.h

#define TIMEUPDATE    600	// Update PC clock every X times through

typedef struct
{
   char sta[6];
   char comp[4];
   char net[3];
   char loc[3];
   int  pin;
}
SCNL;
