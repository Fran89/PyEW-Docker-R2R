
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: evanstrig.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/21 22:32:23  dietz
 *     added location code; inputs TYPE_TRACEBUF2, outputs TYPE_LPTRIG_SCNL
 *
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */

/******************************************************************
 *                         File evanstrig.h                       *
 ******************************************************************/
#include <trace_buf.h>
#include "mteltrg.h"

/* Definition of station list and working space
   ********************************************/
#define MAX_SCNL   32   /* default maximum number of SCN's to process */

typedef struct {
   char        sta[TRACE2_STA_LEN];       /* Station name                 */
   char        chan[TRACE2_CHAN_LEN];     /* Component code               */
   char        net[TRACE2_NET_LEN];       /* Network code                 */
   char        loc[TRACE2_LOC_LEN];       /* Location code                */
   int         pin;              /* pinno associated with this SCNL       */
   char        first;            /* flag for 1st time this SCNL is seen   */
   char        okrate;           /* does this SCNL have required dig-rate?*/
   int32_t     enddata;          /* Last data value of previous message   */
   double      endtime;          /* Stop time of previous message         */
   TEL_CHANNEL qch;              /* updated trigger params for this SCN   */
} STATION;

