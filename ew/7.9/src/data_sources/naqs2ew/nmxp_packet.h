/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nmxp_packet.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.16  2009/07/23 16:41:44  dietz
 *     Added support for TimeServer PLL status=5 (GPS in duty cycle mode and off)
 *
 *     Revision 1.15  2009/07/22 21:53:03  dietz
 *     Added support for Libra GPS status = 13 (GPS in duty-cycle mode, off)
 *
 *     Revision 1.14  2009/07/22 16:41:40  dietz
 *     Added processing of TimeServer PLL/GPS bundles for checking time quality
 *     of Janus and Taurus. Added option to produce TYPE_SNW messages to be
 *     shipped to SeisNetWatch for monitoring.
 *
 *     Revision 1.13  2009/06/05 21:41:58  dietz
 *     Corrected NMX model definitions
 *
 *     Revision 1.12  2007/10/02 17:27:44  dietz
 *     Added three new Nanometrics instrument types
 *
 *     Revision 1.11  2006/01/04 17:54:33  dietz
 *     Fixed bug in unpack_tsdata_bundle() to treat compression_bits==00 as
 *     meaning "no data in the data set" (previous versions had incorrectly
 *     handled it as meaning "no compression" and may have added extra bogus
 *     samples to output).
 *
 *     Revision 1.10  2005/07/25 19:02:58  dietz
 *     Added functions to unpack additional SOH packets used by ISTI's
 *     nmxagent program. Changes made by ISTI.
 *
 *     Revision 1.9  2003/02/14 00:17:01  dietz
 *     changed port field of NXMPSERIALHDR structure from short to char
 *     because it's really only a 1 byte number in the header.
 *
 *     Revision 1.8  2003/02/11 01:07:00  dietz
 *     Added functions to read transparent serial packets
 *
 *     Revision 1.7  2003/02/10 22:13:55  dietz
 *     Added definitions for transparent serial packets
 *
 *     Revision 1.6  2002/11/08 21:34:09  dietz
 *     Added support for Libra time quality bundles
 *
 *     Revision 1.5  2002/11/04 18:32:47  dietz
 *     Added support for extended seismic data headers in the
 *     compressed data packet (needed for Trident data).
 *
 *     Revision 1.3  2002/10/15 23:53:26  dietz
 *     *** empty log message ***
 *
 *     Revision 1.2  2002/03/15 23:10:09  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/06/20 22:35:07  dietz
 *     Initial revision
 *
 *
 *
 */

/*
 * nmxp_packet.h
 *
 * Lynn Dietz  27 March 2001
 *
 * Written to specifications described in: 
 *
 * "Lynx Manual: Appendix B - Libra Data Format" (last revised 22 Mar 2000).
 *
 * Definitions and prototypes to be used by client programs of NaqsServer
 * for reading raw "incoming packets" - those created in the field by 
 * various types of Nanometrics hardware.  
 *
 * When they first come in from the field, all incoming packets consist 
 * of a synchronization pattern (2-byte), oldest packet available for a 
 * data stream, time stamp bundle, n other bundles and a CRC (2-byte).  
 * NaqsServer uses the sync pattern and CRC to verify that the packet 
 * has not been corrupted. When forwarding packets to client programs,
 * NaqsServer strips off the sync pattern and CRC. So, NaqsServer client 
 * programs will see NMXP packets that contain: 
 *    
 *     4 bytes   oldest packet available for this stream
 *    17 bytes   Packet header bundle
 *  17*n bytes   n bundles where n is odd
 *
 * All data in these packets are represented in little endian (Intel)
 * byte order!
 */

#ifndef _NMXP_PACKET_H
#define _NMXP_PACKET_H

#include <platform.h>

#define NMXP_BYTES_PER_BUNDLE        17

/* Packet Type (1 byte, bit 5 = 1 for retransmitted packet)
 **********************************************************/
#define NMXP_COMPRESSDATA_PKT         1
#define NMXP_STATUS_PKT               2
#define NMXP_TIMESYNCH_PKT            4
#define NMXP_LOGMESSAGE_PKT           5
#define NMXP_TRANSPARENTSERIAL_PKT    6
#define NMXP_FILLER_PKT               9

/* Bundle types (1 byte) - the first byte of each bundle.
 * The exception is the compressed data bundle, where the first byte 
 * contains the compression bits for that bundle, so its value will vary.
 ************************************************************************/
#define NMXP_EXTENDEDHDR_BUNDLE       0
#define NMXP_VCXOCALIB_BUNDLE         7 
#define NMXP_NULL_BUNDLE              9
#define NMXP_MINMAX1_BUNDLE          10
#define NMXP_MINMAX2_BUNDLE          11
#define NMXP_INSTLOG_BUNDLE          12
#define NMXP_GPSLOCATION_BUNDLE      13
#define NMXP_GPSERROR_BUNDLE         14
#define NMXP_GPSSTATUS_BUNDLE        15
#define NMXP_D1THRESHTRIG_BUNDLE     20
#define NMXP_D2THRESHTRIG_BUNDLE     21
#define NMXP_D1STALTATRIG_BUNDLE     22
#define NMXP_D2STALTATRIG_BUNDLE     23
#define NMXP_EVENT_BUNDLE            24
#define NMXP_RM3SOH_BUNDLE           27
#define NMXP_RM3RXSTATUS_BUNDLE      29
#define NMXP_FASTEXTSOH_BUNDLE       32   
#define NMXP_SLOWEXTSOH_BUNDLE       33   
#define NMXP_SLOWINTSOH_BUNDLE       34
#define NMXP_GPSTIMEQUAL_BUNDLE      39
#define NMXP_GPSSATINFO_BUNDLE       40
#define NMXP_SERIALPORTMAP_BUNDLE    41
#define NMXP_TELEMPKTREADERR_BUNDLE  42
#define NMXP_SERIALPORTERR_BUNDLE    43
#define NMXP_RXSLOTSTATE_BUNDLE      44
#define NMXP_TXSLOTERR_BUNDLE        45
#define NMXP_RXSLOTERR_BUNDLE        47
#define NMXP_LIBRAINSTSOH_BUNDLE     48
#define NMXP_LIBRAENVSOH_BUNDLE      49
#define NMXP_TRANSMITTER_BUNDLE      50
#define NMXP_RECEIVER_BUNDLE         51
#define NMXP_BURST_BUNDLE            52
#define NMXP_EPOCH_BUNDLE            53
#define NMXP_LIBRAGPSTIMEQUAL_BUNDLE 54
#define NMXP_LIBRASYSTIMEQUAL_BUNDLE 55
#define NMXP_LIBRAOPSTATE_BUNDLE     56
#define NMXP_SERIALDATABYTES_BUNDLE  57
#define NMXP_TELEMPKTSENDSOH_BUNDLE  58
#define NMXP_AUTHENTICATION_BUNDLE   59
#define NMXP_TIMESRV_INSTSOH_BUNDLE  60
#define NMXP_TIMESRV_PLLSOH_BUNDLE   61
#define NMXP_TIMESRV_GPSSOH_BUNDLE   62
#define NMXP_NMXBUS_MASTERSOH_BUNDLE 63
#define NMXP_NMXBUS_REQSOH_BUNDLE    64
#define NMXP_NMXBUS_RXSOH_BUNDLE     65
#define NMXP_NMXBUS_TXSOH_BUNDLE     66
#define NMXP_NMXBUS_DEVLIST_BUNDLE   67
#define NMXP_TRIDENT_PLLSTAT_BUNDLE  68

/* HRD PLL status definitions
 ****************************/
#define HRD_PLL_FINELOCK             1
#define HRD_PLL_COARSELOCK           2
#define HRD_PLL_FREE_GPSOFF          3
#define HRD_PLL_FREE_GPSON           4

/* HRD GPS status definitions
 ****************************/
#define HRD_GPS_3DNAV                0
#define HRD_GPS_2DNAV                1
#define HRD_GPS_TRACKING             2
#define HRD_GPS_SEARCHING            3
#define HRD_GPS_OFF                  4
#define HRD_GPS_ERROR                5    /* actually, 5 or 6 */

/* Libra PLL status definitions
 ******************************/
#define LIBRA_PLL_FINELOCK           1
#define LIBRA_PLL_COARSELOCK         2
#define LIBRA_PLL_NOLOCK             3

/* Libra GPS status definitions
 ******************************/
#define LIBRA_GPS_NAV                0
#define LIBRA_GPS_NOTIME             1
#define LIBRA_GPS_NEEDINIT           2
#define LIBRA_GPS_HIGHPDOP           3
#define LIBRA_GPS_TRACK0             8
#define LIBRA_GPS_TRACK1             9
#define LIBRA_GPS_TRACK2            10
#define LIBRA_GPS_TRACK3            11
#define LIBRA_GPS_DUTYCYCLEOFF      13

/* TimeServer PLL status definitions
 ***********************************/
#define TIMESRV_PLL_INITIALIZING     0
#define TIMESRV_PLL_NOTIME           1
#define TIMESRV_PLL_RAWTIME          2
#define TIMESRV_PLL_APPROXTIME       3
#define TIMESRV_PLL_MEASURINGFREQ    4
#define TIMESRV_PLL_GPSDUTYCYCLEOFF  5
#define TIMESRV_PLL_NOLOCK           7
#define TIMESRV_PLL_COARSELOCK       8
#define TIMESRV_PLL_FINELOCK         9
#define TIMESRV_PLL_SUPERFINELOCK   10

/* TimeServer GPS status definitions
 ***********************************/
#define TIMESRV_GPS_BADGEOMETRY      2
#define TIMESRV_GPS_ACQUIRINGSAT     3
#define TIMESRV_GPS_POSITIONHOLD     4
#define TIMESRV_GPS_PROPAGATEMODE    5
#define TIMESRV_GPS_2DFIX            6
#define TIMESRV_GPS_3DFIX            7

/* Instrument ID information
 ***************************/
#define NMXP_INSTNAME_LEN  4
typedef struct _NMXPINSTRUMENT
{
   short model;                   /* intrument type -  5 bits        */
   short sn;                      /* serial number  - 11 bits        */
   char  code[NMXP_INSTNAME_LEN]; /* string denoting instrument type */  
} NMXPINSTRUMENT;

/* Instrument Model Type Definitions
 ***********************************/
#define NMX_HRD          0
#define NMX_ORION        1
#define NMX_RM3          2
#define NMX_RM4          3
#define NMX_LYNX         4
#define NMX_CYGNUS       5
#define NMX_EUROPA       6
#define NMX_CARINA       7
#define NMX_TIMESERVER   8
#define NMX_TRIDENT      9
#define NMX_JANUS       10
#define NMX_DEPRECATED  11
#define NMX_APOLLO      12
#define NMX_TRIDENT305  13
#define NMX_CARINA105   14
#define NMX_CYGNUS205   15
#define NMX_TAURUS      16

/* Definitions useful for decoding Compressed data bundles.
 * The format of a timeseries data bundle is as follows:
 *   1 byte:   compression bits (2 bits for each data set)
 *   4 bytes:  Compressed data set 1 
 *   4 bytes:  Compressed data set 2 
 *   4 bytes:  Compressed data set 3 
 *   4 bytes:  Compressed data set 3    
 **********************************************************/
#define NMXP_DATASET_PER_BUNDLE      4
#define NMXP_MAX_SAMPLE_PER_BUNDLE  16
#define NMXP_NO_DATA     0  /* data set holds no data values       */
#define NMXP_1BYTE_DIFF  1  /* data set holds 4 1-byte differences */
#define NMXP_2BYTE_DIFF  2  /* data set holds 2 2-byte differences */
#define NMXP_4BYTE_DIFF  3  /* data set holds 1 4-byte difference  */

/* Compressed Data packet: header bundle
 ***************************************/
typedef struct _NMXPDATAHDR 
{
  char  pkttype;       /* 1 byte  packet type = 1 (bit 5=ReTx bit) */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  short subsec;        /* 2 byte  10,000ths of second              */
  short instrumentid;  /* 2 byte  instrumentid:  5 bit model type, */
                       /*                       11 bit serial#     */
  int32_t  seqnum;     /* 4 byte  sequence number                  */
  char  isamprate;     /* 1 byte  5 bits for sample rate index     */
  char  chan;          /*         3 bits for channel number        */
  int32_t  firstsample; /* 3 byte  first sample value              */
} NMXPDATAHDR;

/* Compressed Data packet: extended header bundle
 ************************************************/
typedef struct _NMXPDATAHDRX 
{
  int32_t  firstsample; /* 4 byte  first sample value              */
  char  calibstat[3];  /* 1 byte  bit0 chan 1 calibration status   */
                       /*         bit1 chan 2 calibration status   */
                       /*         bit2 chan 3 calibration status   */
                       /*         bit3-7 unused                    */
} NMXPDATAHDRX;


/* Compressed Data packet: data bundle
 *************************************/
typedef struct _NMXPDATAVAL
{
  int  ndata;
  int32_t data[NMXP_MAX_SAMPLE_PER_BUNDLE];
} NMXPDATAVAL;

/* Transparent Serial Packet: header bundle
 ******************************************/
typedef struct _NMXPSERIALHDR
{
  char  pkttype;       /* 1 byte  packet type = 6 (bit 5=ReTx bit) */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  short subsec;        /* 2 byte  10,000ths of second              */
  short instrumentid;  /* 2 byte  instrumentid:  5 bit model type, */
                       /*                       11 bit serial#     */
  int32_t  seqnum;     /* 4 byte  sequence number                  */
  short nbyte;         /* 2 byte  number of bytes of payload data  */
  char  port;          /* 1 byte  serial port number               */
                       /* 1 byte  spare                            */
} NMXPSERIALHDR;

/* State-of-Health packet: header bundle
 ***************************************/
typedef struct _NMXPSOHHDR 
{
  char  pkttype;       /* 1 byte  packet type = 2 (bit 5=ReTx bit) */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  short subsec;        /* 2 byte  10,000ths of second, always 0    */
  short instrumentid;  /* 4 byte  instrumentid:  5 bit model type, */
                       /*                       11 bit serial#     */
  int32_t  seqnum;     /* 4 byte  sequence number                  */
                       /* 4 byte  reserved                         */
} NMXPSOHHDR;

/* State-of-Health packet: VCXO Calibration Bundle
 *************************************************/
typedef struct _NMXPVCXOCALIB 
{
  char  bundletype;    /* 1 byte  bundle type = 7                   */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01    */
  short VCXOvalue;     /* 2 byte  VCXO value                        */
  short tdiffcount;    /* 2 byte  time difference at lock in counts */
  float tdiffusec;     /*         divided by 3.84 to get microsec   */
  short terrcount;     /* 2 byte  time error in counts              */
  float terrusec;      /*         divided by 3.84 to get microsec   */
  short freqerr;       /* 2 byte  frequency error in counts/sec or  */
                       /*         counts/16sec (how to tell which?) */
  short crystaltemp;   /* 2 byte  crystal temperature               */
  char  PLLstatus;     /* 1 byte  PLL status:                       */   
                       /*          1 = fine locked                  */
                       /*          2 = coarse locking               */
                       /*          3 = temp. ref, gps off           */
                       /*          4 = temp. ref, gps on            */
  char  GPSstatus;     /* 1 byte  GPS status:                       */
                       /*          0 = 3D navigation                */
                       /*          1 = 2D navigation                */
                       /*          2 = tracking 1 satellite or more */
                       /*          3 = searching for satellites     */
                       /*          4 = gps off                      */
                       /*          5 = gps error                    */
} NMXPVCXOCALIB;

/* State-of-Health packet: HRD GPS Location bundle
 *************************************************/
typedef struct _NMXPGPSLOC 
{
  char  bundletype;    /* 1 byte  bundle type = 13                  */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01    */
  float lat;           /* 4 byte  latitude (decimal degrees)        */
  float lon;           /* 4 byte  longitude (decimal degrees)       */
  float elev;          /* 4 byte  elevation (meters)                */
} NMXPGPSLOC;

/* State-of-Health packet: HRD fast/slow external SOH bundle 
 ***********************************************************/
typedef struct _NMXPEXTSOH 
{
  char  bundletype;    /* 1 byte  bundle type = 32 or 33            */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01    */
  float soh1;          /* 4 byte  calibrated SOH1 in volts or units */
  float soh2;          /* 4 byte  calibrated SOH2 in volts or units */
  float soh3;          /* 4 byte  calibrated SOH3 in volts or units */
} NMXPEXTSOH;

/* State-of-Health packet: HRD slow internal SOH bundle
 ******************************************************/
typedef struct _NMXPHRDSOH 
{
  char  bundletype;    /* 1 byte  bundle type = 34                 */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  float voltage;       /* 4 byte  battery voltage measured at PSU in volts */
  float VCXOtemp;      /* 4 byte  VCXO temperature in degrees Celsius      */
  float radioSNR;      /* 4 byte  radio SNR in xxxx (not implemented???)   */
} NMXPHRDSOH;

/* State-of-Health packet: GPS Time Quality bundle
 *************************************************/
typedef struct _NMXPGPSTIMEQUAL 
{
  char  bundletype;    /* 1 byte  bundle type = 39                  */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01    */
  short ontime;        /* 2 byte  GPS on time                       */
  short offtime;       /* 2 byte  GPS off time during last cycle    */
  short tlock;         /* 2 byte  GPS time to lock in seconds       */
  short tdiffcount;    /* 2 byte  time difference at lock in counts */
  float tdiffusec;     /*         divided by 3.84 to get microsec   */
  short VCXOoffset;    /* 2 byte  VCXO offset                       */
  float DACoffset;     /*         divided by 16 to get DAC offset   */
  char  offnote;       /* 1 byte  Reason GPS turned off:            */   
                       /*          0 = PLL finished correcting time error */
                       /*          1 = GPS on time expired          */
  char  mode;          /* 1 byte  Final GPS mode:                   */
                       /*          0 = 3D navigation                */
                       /*          1 = 2D navigation                */
                       /*          2 = tracking 1 satellite or more */
                       /*          3 = searching for satellites     */
} NMXPGPSTIMEQUAL;

/* State-of-Health packet: Receiver Slot Error Bundle 
 *******************************************************/
typedef struct _NMXPRCVSLOTERRBUNDLE 
{
  char  bundletype;    /* 1 byte  bundle type = 47                 */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  int32_t  IPaddr;     /* 4 byte  IP Address                       */
  int32_t  badPkt;     /* 4 byte  no. of bad pkts since start of TDMA */
  int32_t  goodPkt;    /* 4 byte  no. of good pkts since start of TDMA */
} NMXPRCVSLOTERRBUNDLE;

/* State-of-Health packet: Libra Instrument SOH bundle
 *****************************************************/
typedef struct _NMXPLIBRASOH 
{
  char  bundletype;    /* 1 byte  bundle type = 48                 */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  short freqerr;       /* 2 byte  ten MHz frequency error          */
  float SSPBtemp;      /* 2 byte  SSPB temperature                 */
  float WWtemp;        /* 2 byte  WW (controller) temperature      */
  float TXtemp;        /* 2 byte  TX (modem) temperature           */
  float voltage;       /* 2 byte  battery voltage                  */
                       /* 2 byte  unused                           */
} NMXPLIBRASOH;

/* State-of-Health packet: Burst Bundle 
 *******************************************************/
typedef struct _NMXPBURSTBUNDLE 
{
  char  bundletype;    /* 1 byte  bundle type = 52                 */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  int32_t IPaddr;      /* 4 byte  IP Address                       */
  char state;          /* 1 byte  slot (bits 0-1) and burst state (bits 2-3) */
  int32_t goodBurst;   /* 3 byte  no. of good burst since start of TDMA */
  int32_t badBurst;    /* 3 byte  no. of bad burst since start of TDMA */
  char spare;	       /* 1 byte spare */
} NMXPBURSTBUNDLE;

/* State-of-Health packet: Libra GPS Time Quality Bundle
 *******************************************************/
typedef struct _NMXPLIBRAGPSTIMEQUAL 
{
  char  bundletype;    /* 1 byte  bundle type = 54                 */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  short GPSstatus;     /* 2 byte  GPS status                       */
  short nSat;          /* 2 byte  number of usable satellites      */
  float PDOP;          /* 4 byte  PDOP value                       */
  float TDOP;          /* 4 byte  TDOP value                       */
} NMXPLIBRAGPSTIMEQUAL;

/* State-of-Health packet: Libra System Time Quality Bundle
 **********************************************************/
typedef struct _NMXPLIBRASYSTIMEQUAL 
{
  char  bundletype;    /* 1 byte  bundle type = 55                 */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  int32_t  tquality;   /* 4 byte  system time quality              */
  short PLLstatus;     /* 2 byte  PLL mode                         */
  short tdiff;         /* 2 byte  system time - GPS time (nanosec) */
  short tvelocity;     /* 2 byte  time velocity                    */
  float compensation;  /* 2 byte  current compensation             */
} NMXPLIBRASYSTIMEQUAL;

/* State-of-Health packet: TimeServer Time PLL bundle
 **********************************************************/
typedef struct _NMXPTIMESERVERTIMEPLL
{
  char  bundletype;    /* 1 byte  bundle type = 61                */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01  */
  int32_t  subsec;     /* 3 byte  subsec time in fast counts      */
  char  status;        /* bit0-3 of 1-byte status:                */
                       /*             0=initializing              */
                       /*             1=no time                   */
                       /*             2=raw time                  */
                       /*             3=approximate time          */
                       /*             4=measuring frequency       */
                       /*           5-6=reserved                  */
                       /*             7=no lock                   */
                       /*             8=coarse lock               */
                       /*             9=fine lock                 */
                       /*            10=superfine lock            */
                       /*         11-15=reserved                  */
  char  tquality;      /* bit4-7 of 1-byte status:                */ 
                       /*             0= < 100 ns                 */
                       /*             1= < 200 ns                 */
                       /*             2= < 500 ns                 */
                       /*             3= <   1 microsec           */
                       /*             4= <   2 microsec           */
                       /*             5= <   5 microsec           */
                       /*             6= <  10 microsec           */
                       /*             7= <  20 microsec           */
                       /*             8= <  50 microsec           */
                       /*             9= < 100 microsec           */
                       /*            10= <   1 ms                 */
                       /*            11= <  10 ms                 */
                       /*            12= < 100 ms                 */
                       /*            13= <   1 s                  */
                       /*            14= <  10 s                  */
                       /*            15= >  10 s                  */
  int32_t  timeError;  /* 4 byte  time error in fast counts       */
  char  freqError;     /* 1 byte  measured freq error (0.1ppm) * 0.96 to get Hz */
  int32_t  timeSinceLock; /* 3 byte  time since GPS lock loss     */
} NMXPTIMESERVERTIMEPLL;

/* State-of-Health packet: TimeServer M12 GPS Soh bundle
 **********************************************************/
typedef struct _NMXPTIMESERVERGPSSOH
{
  char  bundletype;    /* 1 byte  bundle type = 62                */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01  */
  char  GPSengineOn;   /* bit 0 of 1-byte status:                 */
                       /*            0=not powered                */
                       /*            1=powered                    */
  char  GPSantenna;    /* bit 1-2 of 1-byte status:               */
                       /*            0=OK                         */
                       /*            1=overcurrent                */
                       /*            2=not connected              */
                       /*            3=undefined                  */
  char  GPStoofewsat;  /* bit 3 of 1-byte status:                 */
                       /*            0=false (constellation OK)   */
                       /*            1=true (too few satellites)  */
  char  GPSautosurvey; /* bit 4 of 1-byte status:                 */
                       /*            0=false                      */
                       /*            1=true                       */
  char  GPSstatus;     /* bit 5-7 of 1-byte status:               */
                       /*            0=undefined                  */
                       /*            1=undefined                  */
                       /*            2=bad geometry               */
                       /*            3=acquiring satellites       */
                       /*            4=position hold              */
                       /*            5=propogate mode             */
                       /*            6=2D fix                     */
                       /*            7=3D fix                     */
  char  numvisSats;    /* 1 byte  visible sats 			  */
  char  numtrackSats;  /* 1 byte  tracked sats                    */
  char  UTCoffset;     /* 1 byte  diff between UTC and GPS        */
  short clockBias;     /* 2 byte  clock bias nano secs            */
  short freqBias;      /* 2 byte  frequency bias Hz               */
  float recvTemp;      /* 2 byte  receiver temp in deg C   	  */
  float antVolts;      /* 2 byte  measured antenna voltage        */
} NMXPTIMESERVERGPSSOH;

/* State-of-Health packet: Trident PLL Status Soh bundle
 **********************************************************/
typedef struct _NMXPTRIDENTPLLSTATUSSOH
{
  char  bundletype;    /* 1 byte  bundle type = 68                 */
  int32_t  sec;        /* 4 byte  whole seconds since 1970/01/01   */
  short currentState;  /* 2 byte  Current state, 0-6               */
  short DACcounts;     /* 2 byte  DAC counts                       */
  float terr;          /* 4 byte  time error  micro secs           */
  float temp;          /* 4 byte  temp deg celsius                 */
} NMXPTRIDENTPLLSTATUSSOH;

/* Functions for reading various types of bundles:
 *************************************************/
/* InstrumentID variable */
int  unpack_instid( short instid, NMXPINSTRUMENT *inst );
/* Compressed timeseries bundles */
int  unpack_tsheader_bundle( char *pbundle, NMXPDATAHDR *header );
int  unpack_tsxheader_bundle( char *pbundle, NMXPDATAHDRX *xheader );
int  unpack_tsdata_bundle( char *pbundle, int32_t data1, NMXPDATAVAL *out );
/* Transparent serial packet bundles */
int  unpack_serialheader_bundle( char *pbundle, NMXPSERIALHDR *header ); 
/* SOH bundles */
int  unpack_sohheader_bundle( char *pbundle, NMXPSOHHDR *header );
int  unpack_vcxocalib_bundle( char *pbundle, NMXPVCXOCALIB *dat );
int  unpack_gpslocation_bundle( char *pbundle, NMXPGPSLOC *dat );
int  unpack_externalsoh_bundle( char *pbundle, NMXPEXTSOH *dat );
int  unpack_slowintsoh_bundle( char *pbundle, NMXPHRDSOH *dat );
int  unpack_gpstimequal_bundle( char *pbundle, NMXPGPSTIMEQUAL *dat );
int  unpack_librasoh_bundle( char *pbundle, NMXPLIBRASOH *dat );
int  unpack_libragpstimequal_bundle( char *pbundle, NMXPLIBRAGPSTIMEQUAL *dat );
int  unpack_librasystimequal_bundle( char *pbundle, NMXPLIBRASYSTIMEQUAL *dat );
int  unpack_tridentpll_bundle( char *pbundle, NMXPTRIDENTPLLSTATUSSOH *dat );
int  unpack_timeservergps_bundle( char *pbundle, NMXPTIMESERVERGPSSOH *dat );
int  unpack_timeserverpll_bundle( char *pbundle, NMXPTIMESERVERTIMEPLL *dat );
int  unpack_burst_bundle( char *pbundle, NMXPBURSTBUNDLE *dat );
int  unpack_rcvsloterr_bundle( char *pbundle, NMXPRCVSLOTERRBUNDLE *dat );


#endif
