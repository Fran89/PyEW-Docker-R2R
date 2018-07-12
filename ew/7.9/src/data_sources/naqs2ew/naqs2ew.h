/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqs2ew.h 2043 2005-11-23 22:44:29Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2005/11/23 22:44:29  dietz
 *     Added new command <RepackageNmx> to control how Nmx packets are converted
 *     into EW msgs (either as 1-second EW msgs, or 1 EW msg per Nmx packet).
 *     Default is 1-second EW msgs.
 *
 *     Revision 1.4  2004/04/14 20:07:09  dietz
 *     modifications to support location code
 *
 *     Revision 1.3  2002/07/09 18:10:33  dietz
 *     logit changes
 *
 *     Revision 1.2  2002/03/15 23:10:09  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/06/20 22:35:07  dietz
 *     Initial revision
 *
 */

/*   naqs2ew.h    */
 
#ifndef _NAQS2EW_H
#define _NAQS2EW_H

#include "naqschassis.h"

#define STATION_LEN  (TRACE2_STA_LEN)-1  /* max string-length of station code  */
#define CHAN_LEN     (TRACE2_CHAN_LEN)-1 /* max string-length of channel code  */
#define NETWORK_LEN  (TRACE2_NET_LEN)-1  /* max string-length of network code  */
#define LOC_LEN      (TRACE2_LOC_LEN)-1  /* max string-length of location code */

#define SCN_NOTAVAILABLE   0    /* SCN not served by current NaqsServer   */
#define SCN_AVAILABLE      1    /* SCN is serverd by current NaqsServer   */

#define NAQS2EW_1MSGPERPKT 1    /* create 1 EW message per NMX packet     */
#define NAQS2EW_1SECMSG    2    /* create 1-second EW messages            */

#define ABS(X) (((X) >= 0) ? (X) : -(X))
 
/* Structure for tracking requested channels
 *******************************************/
typedef struct _SCN_INFO {
   char               sta[STATION_LEN+1];
   char               chan[CHAN_LEN+1];
   char               net[NETWORK_LEN+1];
   char               loc[LOC_LEN+1];
   int                pinno;
   int                delay;
   int                format;
   int                sendbuffer;
   int                flag;
   int                lastrate;   /* sample rate of last data rcvd */
   double             texp;       /* expected time of next packet  */
   NMX_CHANNEL_INFO   info;
   NMX_DECOMP_DATA    carryover;  /* data not-yet shipped (<1sec)  */
} SCN_INFO;

/* Function Prototypes 
 *********************/
/* channels.c */
int       SelectChannels( NMX_CHANNEL_INFO *chinf, int nch, 
                          SCN_INFO *req, int nreq ); 
SCN_INFO *FindChannel( SCN_INFO *list, int nlist, int chankey );

#endif
