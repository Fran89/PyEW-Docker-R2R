/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: makeTrace.h 1147 2002-11-25 22:55:49Z alex $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/11/25 22:55:49  alex
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

//    adsend.h

#define TIMEUPDATE    600	// Update PC clock every X times through
#define MAX_CHAN	10000
typedef struct
{
   char sta[6];
   char comp[4];
   char net[3];
   int  pin;
}
SCN;
