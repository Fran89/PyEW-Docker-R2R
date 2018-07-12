/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqssoh.h 3677 2009-07-22 16:41:40Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2009/07/22 16:41:40  dietz
 *     Added processing of TimeServer PLL/GPS bundles for checking time quality
 *     of Janus and Taurus. Added option to produce TYPE_SNW messages to be
 *     shipped to SeisNetWatch for monitoring.
 *
 *     Revision 1.5  2009/06/05 22:49:33  dietz
 *     Added network code argument to the "RequestSOH" command and SOH reporting
 *
 *     Revision 1.4  2002/11/08 21:33:17  dietz
 *     Added support for Libra time quality bundles
 *
 *     Revision 1.3  2002/08/06 20:11:28  dietz
 *     Added monitoring for HRD clock (PLL and GPS status)
 *
 *     Revision 1.2  2002/07/09 18:12:10  dietz
 *     logit changes
 *
 *     Revision 1.1  2002/03/15 23:10:09  dietz
 *     Initial revision
 *
 *     Revision 1.1  2001/06/20 22:35:07  dietz
 *     Initial revision
 *
 */

/*   naqs2ew.h    */
 
#ifndef _NAQS2EW_H
#define _NAQS2EW_H

#include "time.h"
#include "naqschassis.h"

#ifndef ABS
#define ABS(X) (((X) >= 0) ? (X) : -(X))
#endif
#ifndef MIN
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) <= (b) ? (b) : (a))
#endif

/* Error numbers 0-49 are reserved for naqschassis 
   Start naqssoh errors at 50 and go up from there
 **************************************************/ 
#define ERR_OUTOFRANGE  50
#define ERR_INRANGE     51
#define ERR_DEADSTATION 52
#define ERR_LIVESTATION 53
#define ERR_CLOCKBAD    54
#define ERR_CLOCKGOOD   55
 
#define NETWORK_LEN      3    /* max string-length of network code      */ 
#define STATION_LEN      7    /* max string-length of station code      */ 
#define CHAN_LEN         9    /* max string-length of channel code      */
#define NAME_LEN        31    /* max string-length of name for logging  */
#define SOH_NOTAVAILABLE 0    /* SOH not served by current NaqsServer   */
#define SOH_AVAILABLE    1    /* SOH is serverd by current NaqsServer   */
#define LABEL_LEN       11    /* max string-length of SOH label         */

/* Define types of SOH information
 *********************************/
#define HRD_VOLT         1    /* from NMXP_SLOWINTSOH_BUNDLE   */
#define HRD_TEMP         2    /* from NMXP_SLOWINTSOH_BUNDLE   */
#define HRD_SESOH1       3    /* from NMXP_SLOWEXTSOH_BUNDLE   */
#define HRD_SESOH2       4    /* from NMXP_SLOWEXTSOH_BUNDLE   */
#define HRD_SESOH3       5    /* from NMXP_SLOWEXTSOH_BUNDLE   */
#define HRD_FESOH1       6    /* from NMXP_FASTEXTSOH_BUNDLE   */
#define HRD_FESOH2       7    /* from NMXP_FASTEXTSOH_BUNDLE   */
#define HRD_FESOH3       8    /* from NMXP_FASTEXTSOH_BUNDLE   */
#define LIBRA_10MHZERR 101    /* from NMXP_LIBRAINSTSOH_BUNDLE */
#define LIBRA_SSPBTEMP 102    /* from NMXP_LIBRAINSTSOH_BUNDLE */
#define LIBRA_WWTEMP   103    /* from NMXP_LIBRAINSTSOH_BUNDLE */
#define LIBRA_TXTEMP   104    /* from NMXP_LIBRAINSTSOH_BUNDLE */
#define LIBRA_VOLT     105    /* from NMXP_LIBRAINSTSOH_BUNDLE */
#define LIBRA_ESOH1    106    /* from NMXP_LIBRAENVSOH_BUNDLE  */
#define LIBRA_ESOH2    107    /* from NMXP_LIBRAENVSOH_BUNDLE  */
#define LIBRA_ESOH3    108    /* from NMXP_LIBRAENVSOH_BUNDLE  */

typedef struct _SOH_ALARM {
   int     sohtype;           /* soh type - see #define list above      */
   char    label[LABEL_LEN];  /* label to add to any error messages     */
   time_t  tnextlog;          /* next time this SOH should be logged    */
   time_t  tnextsnw;          /* next time this SOH should go to SNW    */
   time_t  tbad;              /* time value went out-of-range; 0 if OK  */
   float   min;               /* minimum value allowed                  */
   float   max;               /* maximum value allowed                  */
   char    reported;          /* flag=1 if reported bad, =0 if OK       */
} SOH_ALARM;

typedef struct _GPS_COORDS {
   double  lat;               /* average latitude (decimal degrees)     */
   double  lon;               /* average longitude (decimal degrees)    */
   double  elev;              /* average elevation (meters)             */
   int     navg;              /* number of single-positions in average  */
} GPS_COORDS;

typedef struct _CLOCK_INFO {
   time_t  tlastlock;         /* most recent time when:                 */
                              /*   PLLstatus = fine or coarse lock, and */
                              /*   GPSstatus = 3D or 2D navigation      */
   time_t  tnextSNW;          /* next time to report to SeisNetWatch    */
   int     PLLstatus;         /* most recent PLL status                 */
   int     GPSstatus;         /* most recent GPS status                 */
   char    bad;               /* flag: 0 if clock is good, 1 if bad     */
} CLOCK_INFO;

/* Structure for tracking requested channels
 *******************************************/
typedef struct _SOH_INFO {
   char              net[NETWORK_LEN];
   char              sta[STATION_LEN];
   char              name[NAME_LEN];
   int               delay;
   int               sendbuffer;
   int               flag;
   time_t            tlastsoh;  /* last time we got SOH for this station */
   int               dead;      /* flag: 0 if alive, 1 if reported dead  */
   NMX_CHANNEL_INFO  info;
   GPS_COORDS        gps;
   CLOCK_INFO        digclock;  /* digitizer clock status (Lynx/HRD/TAU) */
   CLOCK_INFO        vsatclock; /* vsat clock status (Cygnus/Lynx/Janus) */
   CLOCK_INFO        tsrvclock; /* timeserver clock status (CygnusNmxbus/Janus) */
   int               nsoh;      /* number of SOH alarms to keep track of */
   SOH_ALARM        *soh;       /* array of SOH alarms */  
} SOH_INFO;

/* Function Prototypes 
 *********************/
/* channels.c */
int       SelectSOHChannels( NMX_CHANNEL_INFO *chinf, int nch, 
                             SOH_INFO *req, int nreq ); 
SOH_INFO *FindSOHChan( SOH_INFO *list, int nlist, int chankey );

#endif
