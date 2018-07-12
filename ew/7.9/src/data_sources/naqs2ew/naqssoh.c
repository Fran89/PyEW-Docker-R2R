/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqssoh.c 4513 2011-08-09 18:33:19Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.12  2009/07/23 16:41:44  dietz
 *     Added support for TimeServer PLL status=5 (GPS in duty cycle mode and off)
 *
 *     Revision 1.11  2009/07/23 00:03:38  dietz
 *     Added optional command "SNW_ReplaceStaWithName" to control what info is
 *     written to the TYPE_SNW message.
 *
 *     Revision 1.10  2009/07/22 21:53:03  dietz
 *     Added support for Libra GPS status = 13 (GPS in duty-cycle mode, off)
 *
 *     Revision 1.9  2009/07/22 16:41:40  dietz
 *     Added processing of TimeServer PLL/GPS bundles for checking time quality
 *     of Janus and Taurus. Added option to produce TYPE_SNW messages to be
 *     shipped to SeisNetWatch for monitoring.
 *
 *     Revision 1.8  2009/06/05 22:49:33  dietz
 *     Added network code argument to the "RequestSOH" command and SOH reporting
 *
 *     Revision 1.7  2009/01/09 19:02:15  dietz
 *     Added provision for LibraGPSstatus=32767 (off)
 *
 *     Revision 1.6  2009/01/08 21:26:20  dietz
 *     Added sanity checks to some SOH values to ensure that valid text strings
 *     are written to the log.
 *
 *     Revision 1.5  2002/11/08 21:33:17  dietz
 *     Added support for Libra time quality bundles
 *
 *     Revision 1.4  2002/08/26 21:13:02  dietz
 *     Changed logic on when to report "lost GPS"
 *
 *     Revision 1.3  2002/08/06 20:10:54  dietz
 *     Added monitoring for HRD clock (PLL and GPS status)
 *
 *     Revision 1.2  2002/07/09 18:12:10  dietz
 *     logit changes
 *
 *     Revision 1.1  2002/03/15 23:10:09  dietz
 *     Initial revision
 *
 *
 */

/*
 *   naqssoh.c:  Program to receive SOH packets from a NaqsServer
 *               via socket and issue TYPE_ERROR messages if 
 *               various thresholds are exceeded.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <time_ew.h>
#include <kom.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "naqssoh.h"

/* Functions used in this source file
 ************************************/
int    getsohtype( char *str );
int    checksoh( SOH_INFO *box, int sohtype, float value, long sec );
void   checkstatus( void );
void   checkdigclock( void );
double runavg( double value, double avg, int n );
void   sendSNW( SOH_INFO *box, char *sohlabel, char *value );

/* Globals declared in naqschassis.c
 ***********************************/
extern char          ProgName[]; /* name of program - used in logging          */
extern int           Debug;      /* Debug logging flag (optionally configured) */
extern SHM_INFO      Region;     /* shared memory region to use for output     */
extern unsigned char MyInstId;   /* local installation id                      */
extern unsigned char MyModId;    /* Module Id for this program                 */
 
/* Definitions and Globals specific to this naqsclient
 *****************************************************/
#define USAGELEVEL     3              /* Usage Level for SeisNetWatch           */
#define NOTESIZE     256
static char      Note[NOTESIZE];      /* note for error,SNW messages            */
unsigned char    TypeSNW   = 0;       /* TYPE_SNW for sending to SeisNetWatch   */
static int       MaxReqSOH = 0;       /* working limit on max # scn's in ReqSOH */ 
static int       nReqSOH   = 0;       /* # of scn's found in config file        */
static SOH_INFO *ReqSOH    = NULL;    /* SOHs requested in config file          */
static NMX_CHANNEL_INFO *ChanInf = NULL;  /* channel info structures from NAQS  */
static int               nChan   = 0;     /* actual # channels in ChanInf array */

static int  LogSOHInterval    = -1; /* # minutes between writing SOH to logfile */
static int  LogGPSLocation    = -1; /* flag for logging GPS location from SOH   */
static int  ReportOutOfRangeSec = -1; /* # secs a SOH value must be out-of-range*/
                                      /* before an error is reported to statmgr */
static int  ReportDeadSec     = -1;   /* declare station dead if no SOH data is */
                                      /*   rcvd in this many seconds            */
static int  ReportClockBadSec = -1;   /* declare clock bad if time-since last   */
                                      /*   lock is more than this many seconds  */
static int  SNW_Interval      = -1;   /* # min between outputting TYPE_SNW msgs */
static int  SNW_ReplaceStaWithName = 0; /* flag to use "name" field in TYPE_SNW */
                                        /* message instead of "sta" field       */

/* Match a short character string with the types of
 * SOH fields for ease in reading configuration file
 ***************************************************/
static int nNmxSOH = 16;           /* How many name/type pairs are listed below */
static struct {
   char *name;
   int   type;
} NmxSOH[] = {
  { "HV",   HRD_VOLT       },
  { "HT",   HRD_TEMP       },
  { "HSS1", HRD_SESOH1     },
  { "HSS2", HRD_SESOH2     },
  { "HSS3", HRD_SESOH3     },
  { "HFS1", HRD_FESOH1     },
  { "HFS2", HRD_FESOH2     },
  { "HFS3", HRD_FESOH3     },
  { "LFQE", LIBRA_10MHZERR },
  { "LSST", LIBRA_SSPBTEMP },
  { "LWWT", LIBRA_WWTEMP   },
  { "LTXT", LIBRA_TXTEMP   },
  { "LV",   LIBRA_VOLT     },
  { "LES1", LIBRA_ESOH1    },
  { "LES2", LIBRA_ESOH2    },
  { "LES3", LIBRA_ESOH3    }
};

static int  nHRDPLLtext = 5; 
static char *HRDPLLtext[] = { 
  "undefined",
  "fine locked",
  "coarse locked",
  "free running; gps off",
  "free running; gps on"
};
 
static int   nHRDGPStext = 7;
static char *HRDGPStext[] = { 
  "3D navigation",
  "2D navigation",
  "tracking 1 satellite or more",
  "searching",
  "gps off",
  "gps error",
  "gps error"
};

static int  nLibraPLLtext = 4;
static char *LibraPLLtext[] = { 
  "undefined",
  "fine locked",
  "coarse locked",
  "no lock"
};

static int  nLibraGPStext = 14;
static char *LibraGPStext[] = { 
  "navigating",
  "no time",
  "needs initializing",
  "PDOP too high",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "acquiring satellites (0 tracked)",
  "acquiring satellites (1 tracked)",
  "acquiring satellites (2 tracked)",
  "acquiring satellites (3 tracked)",
  "undefined",
  "duty-cycle off"
};

static int  nTimeSrvPLLtext = 16; 
static char *TimeSrvPLLtext[] = {
  "initializing",
  "no time",
  "raw time",
  "approx time",
  "measuring freq",
  "gps-duty-cycle off",
  "reserved",
  "no lock",
  "coarse lock",
  "fine lock",
  "superfine lock",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved"
};

static int  nTimeSrvQualtext = 16; 
static char *TimeSrvQualtext[] = {
  "< 100 ns",
  "< 200 ns",
  "< 500 ns",
  "< 1 microsec",
  "< 2 microsec",
  "< 5 microsec",
  "< 10 microsec",
  "< 20 microsec",
  "< 50 microsec",
  "< 100 microsec",
  "< 1 ms",
  "< 10 ms",
  "< 100 ms",
  "< 1 s",
  "< 10 s",
  "> 10 s"
};

static int  nTimeSrvGPStext = 8;
static char *TimeSrvGPStext[] = {
  "undefined",
  "undefined",
  "bad geometry",
  "acquiring sats",
  "position hold",
  "propagate mode",
  "2D Fix",
  "3D Fix" 
};

static char *Undefined = {"undefined"};

 
/*******************************************************************************
 * naqsclient_com() read config file commands specific to this client          *
 *   Return 1 if the command was processed, 0 if it wasn't                     *
 *******************************************************************************/
int naqsclient_com( char *configfile )
{
   char  *str;
   size_t size;
   float  tmp1, tmp2;
   int    i;

/* Add an entry to the request-list
 **********************************/
   if(k_its("RequestSOH") ) {
      int badarg = 0;
      int narg   = 0;
      char *argname[] = {"","net","sta","name","delay","sendbuffer","nsoh", 
                            "sohtype", "sohlabel", "sohmin", "sohmax" };
      if( nReqSOH >= MaxReqSOH )
      {
         MaxReqSOH += 10;
         size       = MaxReqSOH * sizeof( SOH_INFO );
         ReqSOH     = (SOH_INFO *) realloc( ReqSOH, size );
         if( ReqSOH == NULL )
         {
            logit( "e",
                   "%s: Error allocating %d bytes"
                   " for requested SOH list; exiting!\n", ProgName, size );
            exit( -1 );
         }
      }
      memset( &ReqSOH[nReqSOH], 0, sizeof(SOH_INFO) );

      str=k_str();
      if( !str || strlen(str)>=(size_t)NETWORK_LEN ) {
         narg = badarg = 1; goto EndRequestSOH;
      }
      strcpy(ReqSOH[nReqSOH].net,str);

      str=k_str();
      if( !str || strlen(str)>=(size_t)STATION_LEN ) {
         narg = badarg = 2; goto EndRequestSOH;
      }
      strcpy(ReqSOH[nReqSOH].sta,str);

      str=k_str();
      if( !str || strlen(str)>=(size_t)NAME_LEN) {
         narg = badarg = 3; goto EndRequestSOH;
      }
      strcpy(ReqSOH[nReqSOH].name,str);

      ReqSOH[nReqSOH].delay = k_int();
      if( ReqSOH[nReqSOH].delay < -1 || ReqSOH[nReqSOH].delay > 300 ) {
         narg = badarg = 4; goto EndRequestSOH;
      }

      ReqSOH[nReqSOH].sendbuffer = k_int();
      if( ReqSOH[nReqSOH].sendbuffer!=0 && ReqSOH[nReqSOH].sendbuffer!=1 ) {
         narg = badarg = 5; goto EndRequestSOH;
      }

      ReqSOH[nReqSOH].nsoh = k_int();
      if( ReqSOH[nReqSOH].nsoh<=0 ) {
         narg = badarg = 6; goto EndRequestSOH;
      }

      ReqSOH[nReqSOH].tlastsoh = time(NULL);
      ReqSOH[nReqSOH].dead     = 0;
      ReqSOH[nReqSOH].digclock.tlastlock = time(NULL);
      ReqSOH[nReqSOH].digclock.tnextSNW  = time(NULL);   
      ReqSOH[nReqSOH].digclock.PLLstatus = 255;  /* force log on initial value */
      ReqSOH[nReqSOH].digclock.GPSstatus = 255;  /* force log on initial value */
      ReqSOH[nReqSOH].digclock.bad       = 0;    /* assume clock is good       */
      ReqSOH[nReqSOH].vsatclock.tlastlock = time(NULL);
      ReqSOH[nReqSOH].vsatclock.tnextSNW  = time(NULL);
      ReqSOH[nReqSOH].vsatclock.PLLstatus = 255; /* force log on initial value */
      ReqSOH[nReqSOH].vsatclock.GPSstatus = 255; /* force log on initial value */
      ReqSOH[nReqSOH].vsatclock.bad       = 0;   /* assume clock is good       */
      ReqSOH[nReqSOH].tsrvclock.tlastlock = time(NULL);
      ReqSOH[nReqSOH].tsrvclock.tnextSNW  = time(NULL);
      ReqSOH[nReqSOH].tsrvclock.PLLstatus = 255; /* force log on initial value */
      ReqSOH[nReqSOH].tsrvclock.GPSstatus = 255; /* force log on initial value */
      ReqSOH[nReqSOH].tsrvclock.bad       = 0;   /* assume clock is good       */

   /* allocate space for the required number of soh alarms */
      size = (size_t)ReqSOH[nReqSOH].nsoh * sizeof(SOH_ALARM);
      ReqSOH[nReqSOH].soh = (SOH_ALARM *) malloc( size );
      if( ReqSOH[nReqSOH].soh == NULL )
      {
         logit( "e",
                "%s: Error allocating %d bytes for requested %d SOH_ALARMS; "
                "exiting!\n", ProgName, size, ReqSOH[nReqSOH].nsoh );
         exit( -1 );
      }
      memset( ReqSOH[nReqSOH].soh, 0, size );

      for( i=0; i<ReqSOH[nReqSOH].nsoh; i++ ) 
      {
         str=k_str();
         if( !str || 
            (ReqSOH[nReqSOH].soh[i].sohtype=getsohtype(str)) == 0 ) {
            badarg = 7;  narg = i*4+badarg;  goto EndRequestSOH;
         }
         str=k_str();
         if( !str || strlen(str)>=(size_t)LABEL_LEN) {
            badarg = 8;  narg = i*4+badarg;  goto EndRequestSOH;
         }
         strcpy(ReqSOH[nReqSOH].soh[i].label,str);
         tmp1 = (float) k_val();
         tmp2 = (float) k_val();
         ReqSOH[nReqSOH].soh[i].min = MIN( tmp1, tmp2 );
         ReqSOH[nReqSOH].soh[i].max = MAX( tmp1, tmp2 );
      }

   EndRequestSOH:
      if( badarg ) {
         logit( "e", "%s: Argument %d (%s) bad in <RequestSOH> "
                "command (too long, missing, or invalid value):\n"
                "   \"%s\"\n", ProgName, narg, argname[badarg], k_com() );
         logit( "e", "%s: exiting!\n", ProgName );
         naqsclient_shutdown();
         exit( -1 );
      }
      nReqSOH++;
      return( 1 );

   } /* end of RequestSOH */

   if(k_its("LogSOHInterval") ) {
      LogSOHInterval = k_int();
      return( 1 );
   }

   if(k_its("LogGPSLocation") ) {
      LogGPSLocation = k_int();
      return( 1 );
   }

   if(k_its("ReportOutOfRange") ) {
      double minutes;
      minutes = k_val();
      if( minutes < 0) {
         logit( "e", "%s: bad value (%.1lf) in <ReportOutOfRange> "
                "command (must be >=0):\n"
                "   \"%s\"\n", ProgName, minutes, k_com() );
         logit( "e", "%s: exiting!\n", ProgName );
         naqsclient_shutdown();
         exit( -1 );
      }
      ReportOutOfRangeSec = (int) (minutes*60.);
      return( 1 );
   }

   if(k_its("ReportDead") ) {
      double minutes;
      minutes = k_val();
      if( minutes <= 0) {
         logit( "e", "%s: bad value (%.1lf) in <ReportDead> "
                "command (must be > 0):\n"
                "   \"%s\"\n", ProgName, minutes, k_com() );
         logit( "e", "%s: exiting!\n", ProgName );
         naqsclient_shutdown();
         exit( -1 );
      }
      ReportDeadSec = (int) (minutes*60.);
      return( 1 );
   }

   if(k_its("ReportClockBad") ) {
      double hours;
      hours = k_val();
      if( hours <= 0) {
         logit( "e", "%s: bad value (%.1lf) in <ReportClockBad> "
                "command (must be > 0):\n"
                "   \"%s\"\n", ProgName, hours, k_com() );
         logit( "e", "%s: exiting!\n", ProgName );
         naqsclient_shutdown();
         exit( -1 );
      }
      ReportClockBadSec = (int) (hours*3600.);
      return( 1 );
   }

   if(k_its("SNW_Interval") ) {
      SNW_Interval = k_int();
      if( SNW_Interval!=0  &&  GetType("TYPE_SNW",&TypeSNW) != 0 )
      {
         logit( "e","%s: Invalid message type <TYPE_SNW>; exiting!\n", ProgName );
         exit( -1 );
      }
      return( 1 );
   }

   if(k_its("SNW_ReplaceStaWithName") ) {
      SNW_ReplaceStaWithName = k_int();
      return( 1 );
   }

   return( 0 );
}


/*******************************************************************************
 * naqsclient_init() initialize client stuff                                   *
 *******************************************************************************/
int naqsclient_init( void )
{
/* Check that all commands were read from config file
 ****************************************************/
   if( nReqSOH == 0 )
   {
     logit("e", "%s: No <RequestSOH> commands in config file!\n",
            ProgName );
     return( -1 );
   }
   if( LogSOHInterval == -1 )
   {
     logit("e", "%s: No <LogSOHInterval> command in config file!\n",
            ProgName );
     return( -1 );
   }
   if( LogGPSLocation == -1 )
   {
     logit("e", "%s: No <LogGPSLocation> command in config file!\n",
            ProgName );
     return( -1 );
   }
   if( ReportOutOfRangeSec == -1 )
   {
     logit("e", "%s: No <ReportOutOfRange> command in config file!\n",
            ProgName );
     return( -1 );
   }
   if( ReportDeadSec == -1 )
   {
     logit("e", "%s: No <ReportDead> command in config file!\n",
            ProgName );
     return( -1 );
   }
   if( ReportClockBadSec == -1 )
   {
     logit("e", "%s: No <ReportClockBad> command in config file!\n",
            ProgName );
     return( -1 );
   }
   if( SNW_Interval == -1 )
   {
     logit("e", "%s: No <SNW_Interval> command in config file!\n",
            ProgName );
     return( -1 );
   }

   return( 0 );
} 


/*******************************************************************************
 * naqsclient_process() handle interesting packets from NaqsServer             *
 *  Returns 1 if packet was processed successfully                             *
 *          0 if packet was NOT processed at all                               *
 *         -1 if there was trouble processing the packet                       *
 *******************************************************************************/
int naqsclient_process( long rctype, NMXHDR *nhd, 
                        char **pbuf, int *pbuflen )
{
   char        *pbundle;   /* working pointer into pbuf          */
   NMXPSOHHDR   hdr;       /* contents of header bundle          */
   SOH_INFO    *box;       /* what piece of hardware this status packet if from */
   char         date[25];  /* string for readable dates */
   int          chankey;   /* key to look up box in requested list */
   int          nbundle;   /* number of bundles in this packet */
   int          ibundle;   /* bundle we're working on */
   char        *sockbuf = *pbuf;  /* working pointer into socket buffer */
   char        *ptext,*ptext2;    /* pointers to text to write to log messages */
   int          rc;
   int          retcode = 1; /* assume successful processing */
   int          nomorebundles = 0;

/* Process Status (SOH) packets 
 ******************************/
   if( rctype == NMXMSG_COMPRESSED_DATA )
   {
      if(Debug) logit("et","%s: got a %d-byte compressed message\n",
                       ProgName, nhd->msglen );

   /* Make sure we have a proper length packet 
    ******************************************/
      if( (nhd->msglen-4)%NMXP_BYTES_PER_BUNDLE != 0  )
      {
         logit("et","%s: Broken packet (%d bytes); valid "
               "NMXP pkts contain 4+%d*n bytes!\n", 
                ProgName, nhd->msglen, NMXP_BYTES_PER_BUNDLE );
         retcode = -1;
         goto done;
      } 
      nbundle = (nhd->msglen-4)/NMXP_BYTES_PER_BUNDLE;

   /* Read Header Bundle & make sure it's a status packet  
    *****************************************************/
      pbundle = sockbuf+4;  /* point to beginning of first bundle */ 
      if( unpack_sohheader_bundle( pbundle, &hdr )!= 0 ) {
         logit("et","%s: error reading status pkt header bundle\n", 
                ProgName);
         retcode = -1;
         goto done;
      }
      if(Debug) logit("et","pkttype:%d inst:%d seqnum:%d\n",
                       hdr.pkttype,hdr.instrumentid,hdr.seqnum );

      if( hdr.pkttype != NMXP_STATUS_PKT ) {
         logit("et","%s: cannot read packet type:%d\n",
                ProgName, (int)hdr.pkttype );
         retcode = -1;
         goto done;
      }

   /* Find this instrument in our requested list 
    ********************************************/
      chankey = (hdr.instrumentid<<16) | (NMX_SUBTYPE_SOH<<8);
      box = FindSOHChan( ReqSOH, nReqSOH, chankey );
      if( box == NULL )
      {
         logit("et","%s: Received unknown chankey:%d "
                   "(not in subscription list)\n", ProgName, chankey );
         retcode = -1;
         goto done;
      }
      if(Debug) logit("e"," Status packet from %s.%s (%s)\n", 
                       box->net, box->sta, box->name );
      box->tlastsoh = time(NULL);  /* set time of last SOH packet */
  
   /* Read status packet bundles of interest
    ****************************************/
      for( ibundle=1; ibundle<nbundle; ibundle++ )
      {
         pbundle += NMXP_BYTES_PER_BUNDLE;  
         if(Debug) logit("e","  bundle: %2d  type: %2d\n", ibundle, pbundle[0] );

         switch( pbundle[0] ) 
         {
        /* HRD internal SOH (voltage, temp, etc) 
         ***************************************/
           case NMXP_SLOWINTSOH_BUNDLE: {  
             NMXPHRDSOH hrd;    
             if( unpack_slowintsoh_bundle( pbundle, &hrd )!= 0 ) {
                logit("et","%s: error reading HRD internal SOH bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %d  volt: %.2f  temp: %.2f  radioSNR: %.2f\n",
                               hrd.sec, hrd.voltage, hrd.VCXOtemp, hrd.radioSNR );
             checksoh( box, HRD_VOLT, hrd.voltage,  hrd.sec );
             checksoh( box, HRD_TEMP, hrd.VCXOtemp, hrd.sec );
             break; }

        /* External state-of-health (HRDfast, HRDslow, LibraEnvironment) 
         ***************************************************************/
           case NMXP_FASTEXTSOH_BUNDLE: { 
             NMXPEXTSOH ext;    
             if( unpack_externalsoh_bundle( pbundle, &ext )!= 0 ) {
                logit("et","%s: error reading fast external SOH bundle\n", 
                       ProgName );
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %d  fast soh1: %.2f  soh2 :%.2f  soh3: %.2f\n",
                                ext.sec, ext.soh1, ext.soh2, ext.soh3 );
             checksoh( box, HRD_FESOH1, ext.soh1, ext.sec );
             checksoh( box, HRD_FESOH2, ext.soh2, ext.sec );
             checksoh( box, HRD_FESOH3, ext.soh3, ext.sec );
             break; }
           case NMXP_SLOWEXTSOH_BUNDLE: {
             NMXPEXTSOH ext;    
             if( unpack_externalsoh_bundle( pbundle, &ext )!= 0 ) {
                logit("et","%s: error reading slow external SOH bundle\n", 
                       ProgName );
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %d  slow soh1: %.2f  soh2 :%.2f  soh3: %.2f\n",
                                ext.sec, ext.soh1, ext.soh2, ext.soh3 );
             checksoh( box, HRD_SESOH1, ext.soh1, ext.sec );
             checksoh( box, HRD_SESOH2, ext.soh2, ext.sec );
             checksoh( box, HRD_SESOH3, ext.soh3, ext.sec );
             break; }
           case NMXP_LIBRAENVSOH_BUNDLE: {
             NMXPEXTSOH ext;    
             if( unpack_externalsoh_bundle( pbundle, &ext )!= 0 ) {
                logit("et","%s: error reading libra environment SOH bundle\n", 
                       ProgName );
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %d  libra soh1: %.2f  soh2 :%.2f  soh3: %.2f\n",
                                ext.sec, ext.soh1, ext.soh2, ext.soh3 );
             checksoh( box, LIBRA_ESOH1, ext.soh1, ext.sec );
             checksoh( box, LIBRA_ESOH2, ext.soh2, ext.sec );
             checksoh( box, LIBRA_ESOH3, ext.soh3, ext.sec );
             break; }

        /* Libra Instrument State-of-health 
         **********************************/
           case NMXP_LIBRAINSTSOH_BUNDLE: {
             NMXPLIBRASOH vsat;    
             if( unpack_librasoh_bundle( pbundle, &vsat )!= 0 ) {
                logit("et","%s: error reading Libra instrument SOH bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %d  freqerr: %d  SSPBtemp: %.2f  WWtemp: %.2f  "
                               "TXtemp: %.2f  volt: %.2f\n",
                               vsat.sec, vsat.freqerr, vsat.SSPBtemp, vsat.WWtemp,
                               vsat.TXtemp, vsat.voltage );
             checksoh( box, LIBRA_10MHZERR, vsat.freqerr,  vsat.sec );
             checksoh( box, LIBRA_SSPBTEMP, vsat.SSPBtemp, vsat.sec );
             checksoh( box, LIBRA_WWTEMP,   vsat.WWtemp,   vsat.sec );
             checksoh( box, LIBRA_TXTEMP,   vsat.TXtemp,   vsat.sec );
             checksoh( box, LIBRA_VOLT,     vsat.voltage,  vsat.sec );
             break; }

        /* GPS time quality 
         ******************/
           case NMXP_GPSTIMEQUAL_BUNDLE: {
             NMXPGPSTIMEQUAL gps;    
             if( unpack_gpstimequal_bundle( pbundle, &gps )!= 0 ) {
                logit("et","%s: error reading GPS time quality bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %d  ontime: %d  offtime:%d  tlock: %d\n"
                                   "    tdiffcount: %d  tdiffusec: %.2f  offnote: %d\n"
                                   "    VCXOoffsec: %d  DACoffset: %.2f  mode: %d\n",
                               gps.sec, gps.ontime, gps.offtime, gps.tlock,
                               gps.tdiffcount, gps.tdiffusec, gps.offnote,
                               gps.VCXOoffset, gps.DACoffset, gps.mode );
             break; }

        /* GPS coordinates 
         *****************/
           case NMXP_GPSLOCATION_BUNDLE: {
             if( LogGPSLocation ) {
                NMXPGPSLOC gps;    
                if( unpack_gpslocation_bundle( pbundle, &gps ) != 0 ) {
                   logit("et","%s: error reading GPS location bundle\n", 
                          ProgName);
                   retcode = -1; 
                   break; 
                }
                box->gps.navg++;
                box->gps.lat  = runavg( (double)gps.lat,  box->gps.lat,  box->gps.navg );
                box->gps.lon  = runavg( (double)gps.lon,  box->gps.lon,  box->gps.navg );
                box->gps.elev = runavg( (double)gps.elev, box->gps.elev, box->gps.navg );
                datestr23( (double)gps.sec, date, 25 );
                logit("e","%s  %ld  %s.%s newGPS: %.6f %.6f %.1f  "
                      "avgGPS: %.6lf %.6lf %.1lf %d\n", 
                      date, gps.sec, box->net, box->sta, gps.lat, gps.lon, gps.elev,
                      box->gps.lat, box->gps.lon, box->gps.elev, box->gps.navg );
             }
             break; }

        /* VCXO calibration bundle - HRD clock status! 
         *********************************************/
           case NMXP_VCXOCALIB_BUNDLE: {
             NMXPVCXOCALIB vcxo;    
             if( unpack_vcxocalib_bundle( pbundle, &vcxo )!= 0 ) {
                logit("et","%s: error reading VCXO calibration bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %d  VCXOval: %hd  tdiffcount:%hd  tdiffusec: %.2f\n"
                                   "    terrcount: %hd  terrusec: %.2f  freqerr: %hd\n"
                                   "    crystaltemp: %hd  PLLstatus: %d  GPSstatus: %d\n",
                               vcxo.sec, vcxo.VCXOvalue, vcxo.tdiffcount, vcxo.tdiffusec,
                               vcxo.terrcount, vcxo.terrusec, vcxo.freqerr,
                               vcxo.crystaltemp, vcxo.PLLstatus, vcxo.GPSstatus );

          /* Log any changes in PLL or GPS status: */
             datestr23( (double)vcxo.sec, date, 25 );
             if( (int)vcxo.PLLstatus != box->digclock.PLLstatus ) {
                if( vcxo.PLLstatus < nHRDPLLtext ) ptext = HRDPLLtext[vcxo.PLLstatus];
                else                               ptext = Undefined;
                logit("e","%s  %ld  %s.%s HRD_PLL: %s (PLLstatus=%d)\n",
                       date, vcxo.sec, box->net, box->sta, ptext, (int)vcxo.PLLstatus );
                box->digclock.PLLstatus = (int)vcxo.PLLstatus;
             }
             if( (int)vcxo.GPSstatus != box->digclock.GPSstatus ) {
                if( vcxo.GPSstatus < nHRDGPStext ) ptext = HRDGPStext[vcxo.GPSstatus];
                else                               ptext = Undefined;
                logit("e","%s  %ld  %s.%s HRD_GPS: %s (GPSstatus=%d)\n",
                       date, vcxo.sec, box->net, box->sta, ptext, (int)vcxo.GPSstatus );
                box->digclock.GPSstatus = (int)vcxo.GPSstatus;
             }

          /* Reset time of last lock if status values warrant it: */
             if(   ( vcxo.PLLstatus==HRD_PLL_FINELOCK || 
                     vcxo.PLLstatus==HRD_PLL_COARSELOCK ) 
                && ( vcxo.GPSstatus==HRD_GPS_3DNAV    || 
                     vcxo.GPSstatus==HRD_GPS_2DNAV      ) )
             {
                box->digclock.tlastlock = vcxo.sec;
             } 
             break; }

        /* Libra GPS Time Quality bundle (produced by Libra or Taurus)
         *************************************************************/
           case NMXP_LIBRAGPSTIMEQUAL_BUNDLE: {
             NMXPLIBRAGPSTIMEQUAL gps;    
             CLOCK_INFO *clock    = &(box->vsatclock);
             int         istaurus = 0;

             if( unpack_libragpstimequal_bundle( pbundle, &gps )!= 0 ) {
                logit("et","%s: error reading Libra GPS time quality bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %ld  GPSstatus: %hd  nSat: %hd  "
                               "PDOP: %.2f  TDOP: %.2f\n",
                               gps.sec, gps.GPSstatus, gps.nSat, 
                               gps.PDOP, gps.TDOP );

          /* Log any changes in GPS status: */
             if( box->info.inst.model == NMX_TAURUS ) {
                clock = &(box->digclock); 
                istaurus = 1;
             }
             datestr23( (double)gps.sec, date, 25 );
             if( (int)gps.GPSstatus != clock->GPSstatus ) {
                if      ( gps.GPSstatus < nLibraGPStext ) ptext = LibraGPStext[gps.GPSstatus];
                else                                      ptext = Undefined;
                logit("e","%s  %ld  %s.%s Libra_GPS: %s (GPSstatus=%hd)  "
                       "nSat=%hd PDOP=%.2f TDOP=%.2f\n",
                       date, gps.sec, box->net, box->sta, ptext, gps.GPSstatus,
                       gps.nSat, gps.PDOP, gps.TDOP );
                clock->GPSstatus = (int)gps.GPSstatus;
             }

          /* For non-Taurus, reset time of last lock if status values warrant it: */
             if( istaurus == 0 
                &&  ( clock->PLLstatus==LIBRA_PLL_FINELOCK  || 
                      clock->PLLstatus==LIBRA_PLL_COARSELOCK ) 
                &&    clock->GPSstatus==LIBRA_GPS_NAV          )
             {
                clock->tlastlock = gps.sec;
             } 
          /* For Taurus, reset time of last lock if status values warrant it: */
             if( istaurus 
                &&  ( clock->PLLstatus==TIMESRV_PLL_SUPERFINELOCK ||
                      clock->PLLstatus==TIMESRV_PLL_FINELOCK      || 
                      clock->PLLstatus==TIMESRV_PLL_COARSELOCK     ) 
                &&    clock->GPSstatus==LIBRA_GPS_NAV                )
             {
                clock->tlastlock = gps.sec;
             } 

             break; }

        /* Libra System Time Quality bundle (produced by Libra only) 
         ***********************************************************/
           case NMXP_LIBRASYSTIMEQUAL_BUNDLE: {
             NMXPLIBRASYSTIMEQUAL sys;    
             if( unpack_librasystimequal_bundle( pbundle, &sys )!= 0 ) {
                logit("et","%s: error reading Libra system time quality bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %ld  tquality: %ld  PLLstatus: %hd\n"
                                   "    tdiff: %hd  tvelocity: %hd  compenstation: %f\n",
                               sys.sec, sys.tquality, sys.PLLstatus, 
                               sys.tdiff, sys.tvelocity, sys.compensation );

          /* Log any changes in PLL status: */
             datestr23( (double)sys.sec, date, 25 );
             if( (int)sys.PLLstatus != box->vsatclock.PLLstatus ) {
                if( sys.PLLstatus < nLibraPLLtext ) ptext = LibraPLLtext[sys.PLLstatus];
                else                                ptext = Undefined;
                logit("e","%s  %ld  %s.%s Libra_PLL: %s (PLLstatus=%hd)  tquality=%ldns\n",
                       date, sys.sec, box->net, box->sta, ptext, sys.PLLstatus, sys.tquality );
                box->vsatclock.PLLstatus = (int)sys.PLLstatus;
             }

          /* Reset time of last lock if status values warrant it: */
             if(  ( box->vsatclock.PLLstatus==LIBRA_PLL_FINELOCK || 
                    box->vsatclock.PLLstatus==LIBRA_PLL_COARSELOCK ) 
                &&  box->vsatclock.GPSstatus==LIBRA_GPS_NAV          )
             {
                box->vsatclock.tlastlock = sys.sec;
             } 
             break; }

        /* TimeServer Time PLL SOH bundle (produced by Cygnus/Janus-TimeServer or Taurus) 
         ********************************************************************************/
           case NMXP_TIMESRV_PLLSOH_BUNDLE: {
             NMXPTIMESERVERTIMEPLL tsrv;
             CLOCK_INFO *clock    = &(box->tsrvclock);
             int         istaurus = 0;

             if( unpack_timeserverpll_bundle( pbundle, &tsrv )!= 0 ) {
                logit("et","%s: error reading TimeServer Time PLL SOH bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) logit("e","    t: %ld  subsec: %ld  PLLstatus: %hd (%s)\n"
                                   "    tquality: %hd (%s)  timeErr: %ld  freqErr: %hd  tsincelock: %ld s\n",
                               tsrv.sec, tsrv.subsec, 
                               (short) tsrv.status, TimeSrvPLLtext[tsrv.status],
                               (short) tsrv.tquality, TimeSrvQualtext[tsrv.tquality],
                               tsrv.timeError, (short)tsrv.freqError, tsrv.timeSinceLock ); 

          /* Log any changes in PLL status: */
             if( box->info.inst.model == NMX_TAURUS ) {
                clock = &(box->digclock); 
                istaurus = 1;
             }
             datestr23( (double)tsrv.sec, date, 25 );
             if( (int)tsrv.status != clock->PLLstatus ) 
             {
                if( tsrv.status   < nTimeSrvPLLtext  ) ptext  = TimeSrvPLLtext[tsrv.status];
                else                                   ptext  = Undefined;
                if( tsrv.tquality < nTimeSrvQualtext ) ptext2 = TimeSrvQualtext[tsrv.tquality];
                else                                   ptext2 = Undefined;
                logit("e","%s  %ld  %s.%s TimeSrv_PLL: %s (PLLstatus=%d)  tquality=%d (%s)"
                          "  tsincelock=%lds\n",
                       date, tsrv.sec, box->net, box->sta, ptext, (int)tsrv.status, 
                       (int)tsrv.tquality, ptext2, tsrv.timeSinceLock );
                clock->PLLstatus = (int)tsrv.status;
             }

          /* For non-Taurus, set time of last lock from this bundle: */
             if( !istaurus ) {
                clock->tlastlock = tsrv.sec - tsrv.timeSinceLock;
                break;
             }

          /* For Taurus, reset time of last lock if status values warrant it: */
             if(  ( clock->PLLstatus==TIMESRV_PLL_SUPERFINELOCK ||
                    clock->PLLstatus==TIMESRV_PLL_FINELOCK      || 
                    clock->PLLstatus==TIMESRV_PLL_COARSELOCK    ) 
                &&  clock->GPSstatus==LIBRA_GPS_NAV               )
             {
                clock->tlastlock = tsrv.sec;
             } 
             break; }

        /* TimeServer GPS SOH bundle (produced by Cygnus/Janus-TimeServer only) 
         **********************************************************************/
           case NMXP_TIMESRV_GPSSOH_BUNDLE: {
             NMXPTIMESERVERGPSSOH tsrv;
             CLOCK_INFO *clock = &(box->tsrvclock);

             if( unpack_timeservergps_bundle( pbundle, &tsrv )!= 0 ) {
                logit("et","%s: error reading TimeServer GPS SOH bundle\n", 
                       ProgName);
                retcode = -1; 
                break; 
             }
             if( Debug ) { logit("e","    t: %ld  GPSengineOn:%d GPSantenna:%d GPStoofewsat:%d"
                                     " GPSautosurvey:%d GPSstatus:%d\n",
                               tsrv.sec, (int)tsrv.GPSengineOn, (int)tsrv.GPSantenna, 
                               (int)tsrv.GPStoofewsat, (int)tsrv.GPSautosurvey, (int)tsrv.GPSstatus );
                            logit("e","    numvisSats:%d numtrackSats:%d UTCoffset:%d clockBias: %d"
                                      " freqBias:%d recvTemp:%.1f antennaVolts:%.1f\n",
                               (int)tsrv.numvisSats, (int)tsrv.numtrackSats, 
                               (int)tsrv.UTCoffset, (int)tsrv.clockBias, (int)tsrv.freqBias,
                               tsrv.recvTemp, tsrv.antVolts ); }

          /* Log any changes in GPS status: */
             datestr23( (double)tsrv.sec, date, 25 );
             if( (int)tsrv.GPSstatus != clock->GPSstatus ) 
             {
                if( tsrv.GPSstatus < nTimeSrvGPStext ) ptext = TimeSrvGPStext[tsrv.GPSstatus];
                else                                   ptext = Undefined;
                logit("e","%s  %ld  %s.%s TimeSrv_GPS: %s (GPSstatus=%d)\n",
                       date, tsrv.sec, box->net, box->sta, ptext, (int)tsrv.GPSstatus ); 
                clock->GPSstatus = (int)tsrv.GPSstatus;
             }

             break; }


           case NMXP_NULL_BUNDLE: /* no more data in this packet */
             nomorebundles = 1;
             break;
 
           default:  /* uninteresting or unknown bundle type */
             break;
         }/*end switch*/

         if( nomorebundles ) break;
      } /* end for every bundle in packet */

   } /* end if compressed data */

/* got a list of all possible channels, continue startup protocol
 *  2) recv "channel list" message
 *  3) send "add channel" message(s)
 ****************************************************************/
   else if( rctype == NMXMSG_CHANNEL_LIST )
   {
      int msglen = 0;
      int nadd   = 0;
      int ir;

      nChan = 0;
      free( ChanInf );

   /* read the channel list message */
      if( nmx_rd_channel_list( nhd, sockbuf, &ChanInf, &nChan ) < 0 )
      {
         logit("e", "%s: trouble reading channel list message\n", ProgName );
         retcode = -1; 
         goto done;
      }

   /* compare the Naqs channel list again the requested channels */

      rc = SelectSOHChannels( ChanInf, nChan, ReqSOH, nReqSOH );

   /* build and send "add channel" message for each available channel */
          
      for( ir=0; ir<nReqSOH; ir++ )
      {
         if( ReqSOH[ir].flag == SOH_NOTAVAILABLE ) continue;
         if( nmx_bld_add_soh( pbuf, pbuflen, &msglen,
                  &(ReqSOH[ir].info.chankey), 1, ReqSOH[ir].delay,
                  ReqSOH[ir].sendbuffer ) != 0 )
         {
            logit("et","%s: trouble building add_soh message\n", ProgName);
            continue;
         }
         nmxsrv_sendto( sockbuf, msglen );
         if(Debug) logit("et","sent add_soh message %d\n", ++nadd );
      }
      
   } /* end if channel list msg */

/* Not a message type we're interested in
 ****************************************/
   else {
      retcode = 0;
   }

done:
   checkstatus();
   checkdigclock();

   return( retcode );

} /* end of naqsclient_process */


/*******************************************************************************
 * naqsclient_shutdown()  frees malloc'd memory, etc                           *
 *******************************************************************************/
void naqsclient_shutdown( void )
{
/* Free all allocated memory
 ***************************/
   free( ReqSOH );
   free( ChanInf );
   return;
}


/*******************************************************************************
 * checksoh()  checks a SOH value against allowable ranges.                    * 
 *  Sends error msg if it's out of range and wasn't before.                    *
 *  Sends "unerror" msg if it's within range, but was previously out-of-range. * 
 *  Logs the value at the proper logging interval.                             *
 *******************************************************************************/
int checksoh( SOH_INFO *box, int sohtype, float value, long sec )
{
   SOH_ALARM *soh = NULL;
   time_t     duration;
   char       date[25];
   char       svalue[32];
   int        i;

/* Are we monitoring this type of SOH information? If not, return now. 
 *********************************************************************/
   for( i=0; i<box->nsoh; i++ ) 
   {
      if( box->soh[i].sohtype == sohtype ) {
         soh = &(box->soh[i]);
         break;
      }
   }
   if( soh == NULL ) return( 0 );   

/* Is the value too low? 
 ***********************/
   if( value < soh->min ) {
      datestr23( (double)sec, date, 25 );
   /* It was OK before...mark it as bad! */
      if( !soh->tbad ) {  
         logit("e","%s  %ld  %s.%s %s: %.2f  First value too low!\n", 
               date, sec, box->net, box->sta, soh->label, value );
         if( SNW_Interval ) {
            sprintf( svalue, "%2.1f", value );
            sendSNW( box, soh->label, svalue );

         }
         soh->tbad = sec;
      }    
   /* See if it's time to complain! */ 
      duration = sec - soh->tbad;
      if( !soh->reported  &&  duration >= ReportOutOfRangeSec ) {
         sprintf( Note, 
                 "%s.%s (%s) %s too low for %.1f min! (value: %.2f  allowed: %.2f - %.2f)",
                  box->net, box->sta, box->name, soh->label, duration/60., 
                  value, soh->min, soh->max ); 
         naqschassis_error( ERR_OUTOFRANGE, Note, (time_t) sec );
         if( SNW_Interval ) {
            sprintf( svalue, "%2.1f", value );
            sendSNW( box, soh->label, svalue );
         }
         soh->reported = 1;
      }    
   } 
/* Is the value too high? 
 ************************/
   else if( value > soh->max ) {
      datestr23( (double)sec, date, 25 );
   /* It was OK before...mark it as bad. */
      if( !soh->tbad ) {  
         logit("e","%s  %ld  %s.%s %s: %.2f  First value too high!\n", 
               date, sec, box->net, box->sta, soh->label, value );
         if( SNW_Interval ) {
            sprintf( svalue, "%2.1f", value );
            sendSNW( box, soh->label, svalue );
         }
         soh->tbad = sec;
      }    
   /* See if it's been bad long enough to complain! */ 
      duration = sec - soh->tbad;
      if( !soh->reported  &&  duration >= ReportOutOfRangeSec ) {
         sprintf( Note, 
                 "%s.%s (%s) %s too high for %.1f min! (value: %.2f  allowed: %.2f - %.2f)",
                  box->net, box->sta, box->name, soh->label, duration/60., 
                  value, soh->min, soh->max ); 
         naqschassis_error( ERR_OUTOFRANGE, Note, (time_t) sec );
         if( SNW_Interval ) {
            sprintf( svalue, "%2.1f", value );
            sendSNW( box, soh->label, svalue );
         }
         soh->reported = 1;
      }    
   } 

/* In range 
 **********/
   else {
 /* it was bad before...tell us it's OK now */
      if( soh->tbad ) {
         datestr23( (double)sec, date, 25 );
         logit("e","%s  %ld  %s.%s %s: %.2f  First value back in acceptable range!\n", 
               date, sec, box->net, box->sta, soh->label, value );
         if( SNW_Interval ) {
            sprintf( svalue, "%2.1f", value );
            sendSNW( box, soh->label, svalue );
         }
         if( soh->reported ) {
           sprintf( Note, 
                   "%s.%s (%s) %s is OK again (value: %.2f  allowed: %.2f - %.2f)",
                    box->net, box->sta, box->name, soh->label, 
                    value, soh->min, soh->max ); 
           naqschassis_error( ERR_INRANGE, Note, (time_t) sec ); 
           soh->reported = 0;
         }        
      }
      soh->tbad = 0;
   }

/* Log value if timer is expired 
 *******************************/
   if( LogSOHInterval  &&  sec >= soh->tnextlog ) {
      datestr23( (double)sec, date, 25 );
      logit("e","%s  %ld  %s.%s %s: %.2f\n", 
            date, sec, box->net, box->sta, soh->label, value );
      soh->tnextlog = sec + LogSOHInterval*60;
   }
   if( SNW_Interval  &&  sec >= soh->tnextsnw ) {
      sprintf( svalue, "%2.1f", value );
      sendSNW( box, soh->label, svalue );
      soh->tnextsnw = sec + SNW_Interval*60;
   }

   return( 0 );
}


/*******************************************************************************
 * getsohtype()  Return the SOH type that is paired in the NmxSOH structure    *
 *  with the given character string.  Return 0 if no match is found.           * 
 *******************************************************************************/
int getsohtype( char *str )
{
   int i;
   for( i=0; i<nNmxSOH; i++ )  {
      if( strcmp( NmxSOH[i].name, str ) == 0 ) return( NmxSOH[i].type );
   }
   return( 0 );
}


/*******************************************************************************
 * checkstatus()  checks the time since last SOH was received for each site.   * 
 *  Sends error msg if it's been longer than acceptable (dead station)         *
 *  Sends "unerror" msg if alive again, but was previously dead.               * 
 *******************************************************************************/
void checkstatus( void )
{
   SOH_INFO *box;
   time_t    now = time(NULL);
   time_t    tsilent;
   int       i;

/* Loop over all stations
 ************************/
   for( i=0; i<nReqSOH; i++ ) 
   {
      box     = &ReqSOH[i];
      tsilent = now - box->tlastsoh;

   /* Silent interval is too long - report as dead
    **********************************************/
      if( tsilent >= ReportDeadSec  &&  !box->dead ) {
         sprintf( Note, 
                 "%s.%s (%s) dead; no SOH data in %.1f minutes.",
                  box->net, box->sta, box->name, (float)tsilent/60.0 ); 
         naqschassis_error( ERR_DEADSTATION, Note, now );
         box->dead = 1;
      }
        
   /* Alive again after being reported dead
    ***************************************/
      else if( tsilent < ReportDeadSec  &&   box->dead ) {
         sprintf( Note, 
                 "%s.%s (%s) alive; SOH received again.",
                  box->net, box->sta, box->name ); 
         naqschassis_error( ERR_LIVESTATION, Note, now );
         box->dead = 0;
      }
   }
   return;
}

/*******************************************************************************
 * checkdigclock()  checks the time since digitizer clock was last locked.     * 
 *  Sends error msg if it's been longer than acceptable.                       *
 *  Sends "unerror" msg if clock locks back up again                           * 
 *******************************************************************************/
void checkdigclock( void )
{
   SOH_INFO   *box;
   CLOCK_INFO *clock;    /* pointer to datalogger's clock SOH */
   time_t      now = time(NULL);
   time_t      tsincelock;
   int         i;

/* Loop over all stations; skip non-digitizer instruments
 ********************************************************/
   for( i=0; i<nReqSOH; i++ ) 
   {
      box = &ReqSOH[i];
      if     ( box->info.inst.model == NMX_HRD    ) clock = &(box->digclock);
      else if( box->info.inst.model == NMX_LYNX   ) clock = &(box->digclock);
      else if( box->info.inst.model == NMX_TAURUS ) clock = &(box->digclock);
      else if( box->info.inst.model == NMX_JANUS  ) clock = &(box->tsrvclock);
      else                                          continue;
      tsincelock = now - clock->tlastlock;

   /* Time since lock too long - report it (once)!
    **********************************************/
      if( tsincelock >= ReportClockBadSec  &&    /* too long since last lock, */
          !clock->bad                      &&    /* hasn't been reported yet, */
          !box->dead  )                          /* and still getting SOH     */
      {
         sprintf( Note, 
                 "%s.%s (%s) no digitizer GPS lock for over %.1f hours (clock free-running).",
                  box->net, box->sta, box->name, (float)tsincelock/3600. ); 
         naqschassis_error( ERR_CLOCKBAD, Note, now );
         clock->bad = 1;
      }
        
   /* Clock resynced again after being reported unlocked
    ****************************************************/
      else if( tsincelock < ReportClockBadSec   &&   clock->bad )       
      {
         sprintf( Note, 
                 "%s.%s (%s) digitizer GPS locked again (clock synced).",
                  box->net, box->sta, box->name ); 
         naqschassis_error( ERR_CLOCKGOOD, Note, clock->tlastlock );
         clock->bad = 0;
      }

   /* Send SeisNetWatch messages if it's time
    *****************************************/
      if( SNW_Interval             &&      /* configured to talk to SeisNetWatch  */
          now >= clock->tnextSNW   &&      /* it's time to talk again             */
          clock->GPSstatus != 255      )   /* GPSstatus has been rx at least once */
      {
         char svalue[32];
         clock->tnextSNW += SNW_Interval*60; 
         if( box->dead ) continue;         /* don't send SNW info for dead station */

         if( box->info.inst.model == NMX_HRD  ||  
             box->info.inst.model == NMX_LYNX   ) {
            sprintf( svalue, "%ld", (long)tsincelock );
            sendSNW( box, "HRD VCXO Seconds Since Last Lock", svalue );
            sprintf( svalue, "%d",  clock->PLLstatus );
            sendSNW( box, "HRD VCXO PLL Status Code",         svalue );
            sprintf( svalue, "%d",  clock->GPSstatus );
            sendSNW( box, "HRD VCXO GPS Status Code",         svalue );
         }
         if( box->info.inst.model == NMX_TAURUS ) {
            sprintf( svalue, "%ld", (long)tsincelock );
            sendSNW( box, "Taurus Seconds Since Last Lock", svalue );
            sprintf( svalue, "%d",  clock->PLLstatus );
            sendSNW( box, "Taurus PLL Status Code",         svalue );
            sprintf( svalue, "%d",  clock->GPSstatus );
            sendSNW( box, "Taurus GPS Status Code",         svalue );
         }
         if( box->info.inst.model == NMX_JANUS ) {
            sprintf( svalue, "%ld", (long)tsincelock );
            sendSNW( box, "TimeServer Seconds Since Last Lock", svalue );
            sprintf( svalue, "%d",  clock->PLLstatus );
            sendSNW( box, "TimeServer PLL Status Code",  svalue );
            sprintf( svalue, "%d",  clock->GPSstatus );
            sendSNW( box, "TimeServer GPS Status Code",  svalue );
         }
      }
   }
   return;
}


/******************************************************
 * runavg() compute the running average               *
 ******************************************************/
double runavg( double value, double avg, int n )
{
  if( n<=0 ) return( 0.0 );
  return( (double)(n-1)/n*avg + (value/n) );
}


/*******************************************************
 * sendSNW() build/send a SeisNetWatch msg to the ring *  
 *******************************************************/
void sendSNW( SOH_INFO *box, char *sohlabel, char *value )
{
   MSG_LOGO logo;
   int      res;
   long     size;
   int      src;
 
   logo.instid = MyInstId;
   logo.mod    = MyModId;
   logo.type   = TypeSNW;

   if( SNW_ReplaceStaWithName )
   {
      src = snprintf( Note, NOTESIZE, 
                     "%s-%s:2:%s=%s;UsageLevel=%d\n",
                      box->net, box->name, sohlabel, value, USAGELEVEL );
   } else {
      src = snprintf( Note, NOTESIZE, 
                     "%s-%s:2:%s=%s;UsageLevel=%d\n",
                      box->net, box->sta,  sohlabel, value, USAGELEVEL );
   }

   if( src < 0 ) {
      logit( "e", "%s: snprintf() buffer too small (%d chars).\n", 
              ProgName, NOTESIZE );
   }
 
   size = strlen( Note );
   res  = tport_putmsg( &Region, &logo, size, Note );
   if( res!= PUT_OK ) {
      logit( "e", "%s: Error sending TYPE_SNW message to ring.\n", 
              ProgName );
   }
   return;                 
}
