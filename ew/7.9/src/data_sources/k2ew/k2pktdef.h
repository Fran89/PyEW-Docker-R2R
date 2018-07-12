/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2pktdef.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.2  2003/05/15 00:42:45  lombard
 *     Changed to version 2.31: fixed handling of channel map.
 *
 *     Revision 1.1  2000/05/04 23:48:24  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2pktdef.h:  Kinemetrics K2 block mode packets definitions and types */
/*  */
/*  12/30/98 -- [ET]  Extracted from 'qtpkt.h' sample file */
/*  06/05/2005 - paulf - added in comm_status struct */
/*  */

#ifndef K2PKTDEF_H                /* compile file only once */
#define K2PKTDEF_H 1              /* indicate file compiled */

#include "nkwhdrs.h"              /* Kinemetrics header definitions */

/* ..... defines for packets  ................ */

#define PKTESC  '\\'     /* $5c, 92d */
#define PKTFRAME 0xC0    /*     192d */
                         /* send one before packet_header
                            send one after data checksum
                                 (mcu sends two after ) */

#define ESC_EQUIVALENT  0xDC
#define END_EQUIVALENT  0xDD

#define PACKET_MAX_SIZE  3003      /* 2992 + 8 + 3, same as QT */
#define PACKET_MAX_DATALEN 2992

#define PACKET_HDR_SIZE 8

#define K2_HEAD_SIZE  2040
#define MW_HEAD_SIZE  2736

#define STATUS_SIZE 24

/* ..... see end of file for packet code types */

/*---------------------------------------------------*/
/* PACKET HEADER Structure                           */
/*---------------------------------------------------*/

struct PACKET_HDR
{
  unsigned char typeCode;       /* packet type, see table */
  unsigned char seqNo;          /* $00..$ff command ID#, response will echo it */
  unsigned short source;        /* $0000..$ffff, command task or instrument # */
                                /* response will echo it */
  unsigned short destination;   /* command = $0000 */
                                /* response will echo source number
                                   in destination */
  unsigned short dataLength;    /* # bytes in data area, without extra
                                     bytes due to escape sequences */
};


/* #define STN_ID_LENGTH 5     */      /* from 'kwhdrs.h' [ET] */
/* #define CHANNEL_ID_LENGTH 5 */      /* from 'kwhdrs.h' [ET] */

/*---------------------------------------------------*/
/* STATUS INFO Structure   -- mcu k2defs.h           */
/*---------------------------------------------------*/
#define SF_FAULT 1
#define SF_OK 0
#define SF_FREE 0x03FF
#define SF_NOT_READY 0x0400
#define SF_KB 0x0800
#define SF_MB 0x1000
#define SF_GB 0x2000

struct STATUS_INFO {
   uint32_t systemTime;          /* time in seconds since Jan 1, 1980 0-3 */
   unsigned short serialNumber;  /*                                   4-5 */
   unsigned short events;        /*                                   6-7 */
   unsigned short recordingErrors; /*                                 8-9 */
   unsigned short triggerStatus; /* bit 0 = ACQ STATUS (ON=1, OFF=0)  10-11
                                    bit 1 = STREAM TRIG (1), DETRIG (0)
                                    bit 2 = KEYBRD TRIG (1), DETRIG (0)
                                    bit 3 = ALARM TRIG (1), DETRIG (0)
                                    bit 4 = EXTERNAL TRIG (1), DETRIG (0)
                                  */
   unsigned char batteryStatus;  /* volts * 10; + means charging      12 */
   unsigned char hardwareStatus; /* one of: SF_FAULT ("FAULT")        13
                                            SF_OK ("OK")
                                  */
   unsigned short driveStatus[2]; /* for both drives.                 14-17
                                     Bits 0-9 is space remaining
                                     Bits 10-15 are flags :
                                     Bit 10 Set: SF_NOT_READY = Drive not ready
                                     Bit 11 Set: SF_KB = space in kilobytes
                                     Bit 12 Set: SF_MB = space in megabytes
                                     Bit 13 Set: SF_GB = space in gigabytes
                                   */

   char stnID[STN_ID_LENGTH];    /* null terminated                  18-22 */
   unsigned char clockSource;    /* 0 = RTC from cold start          23 = 24
                                    1 = keyboard
                                    2 = Sync w/ ext. ref. pulse
                                    3 = Internal GPS
                                  */
};

/* Added: PNL */
/*---------------------------------------------------*/
/* K2 EXTENDED STATUS INFO Structure                 */
/*---------------------------------------------------*/

#define FAULT_SYSTEM            0x0001
#define FAULT_FLASH             0x0002
#define FAULT_RAM               0x0004
#define FAULT_PCMCIA            0x0008
#define FAULT_DSP               0x0010
#define FAULT_PARMBLK           0x0020
#define FAULT_FLASH_MAINTENANCE 0x0040
#define EXT_SIZE  12
#define EXT2_SIZE 40

struct EXT_STATUS_INFO
{
  short temperature;                 /* degrees C x 10              0-1 */
  uint32_t restartTime;              /* time of last restart        2-5 */
  unsigned short fault;              /* Fault flags                 6-7
                       FAULT_SYSTEM    BIT0  bad parameters
                       FAULT_FLASH     BIT1  flash error (old MCU only)
                       FAULT_RAM       BIT2  ram error (failed r/w test)
                       FAULT_PCMCIA    BIT3  bad or missing PCMCIA board
                       FAULT_DSP       BIT4  failed to load or start DSP
                       FAULT_PARMBLK   BIT5  parameter block crc error
                       FAULT_FLASH_MAINTENANCE BIT6  flash maintenance required
                                     */
  uint32_t lastRAMError;     /* from SOH task                     8-11 = 12*/
}
;

/* Revised extended status structure, per Altus Block Mode Communications *
 * Manual, Kinemetrics Doc 302218 Rev E. Added May 2003, Pete Lombard     */
struct EXT2_STATUS_INFO
{
    short temperature;                 /* degrees C x 10              0-1 */
    uint32_t restartTime;              /* time of last restart        2-5 */
    unsigned short fault;              /* Fault flags                 6-7
					  FAULT_SYSTEM    BIT0  bad parameters
					  FAULT_FLASH     BIT1  flash error (old MCU only)
					  FAULT_RAM       BIT2  ram error (failed r/w test)
					  FAULT_PCMCIA    BIT3  bad or missing PCMCIA board
					  FAULT_DSP       BIT4  failed to load or start DSP
					  FAULT_PARMBLK   BIT5  parameter block crc error
					  FAULT_FLASH_MAINTENANCE BIT6  flash maintenance required
				       */
    uint32_t lastRAMError;       /* from SOH task                     8-11 = 12*/
    unsigned char clockSource;   /* same code as 
				  * TIME_RO_PARAMS.clockSource       12 */
    unsigned char gpsStatus;     /* GPS Lock status                  13 */
    short gpsLatitude;           /* latitude * 100, degrees North    14 - 15 */
    short gpsLongitude;          /* longitude * 100, degrees East    16 - 17 */
    short gpsAltitude;           /* altitude in meters               18 - 19 */
    uint32_t gpsLastLockTime;    /* Last GPS lock time               20 - 23 */
    unsigned char pad[16];       /* Pad for future expansion         24 - 39 */
}
;
/*---------------------------------------------------*/
/* Communications  info. from  ISTI		     */
/*---------------------------------------------------*/
struct COMM_INFO 
{

	time_t program_start_time;
	time_t comm_info_time;			/* the time at which this is logged */
	time_t last_info_time;			/* the prior one */
	/* all of the total_ values are since program_start_time */
	uint32_t total_pkt_received;
	uint32_t total_pkt_sent;
	uint32_t total_pkt_output_seqerr;
	time_t last_output_seqerr_time;
	uint32_t total_pkt_retries;
	float total_error_rating;
	uint32_t total_pkt_missing_err;
	uint32_t total_pkt_skipped;
	uint32_t total_pkt_request_resend_failure;
	uint32_t total_comm_retries;
	
	/* since the last summary */
	uint32_t last_pkt_received;
	uint32_t last_pkt_sent;
	uint32_t last_pkt_retries;
	float last_error_rating;
	
};

  
/*---------------------------------------------------*/
/* EVENT SUMMARY  from k2defs.h  mcu                 */
/*---------------------------------------------------*/

#define ABS_MAX_CHANNELS 18          /* Mt. Whitney */
#define EVT_SUM_SIZE  608            /* K2 = 416, MW = 608 */

struct EVT_SUMMARY
{
   int nchan;                        /*  0- 1 # channels in file */
   int a2dBits;                      /*  2- 3 resolution of A/D in bits */
   unsigned int flags;               /*  4- 5 bit 0 = FT flag */
   unsigned int sps;                 /*  6- 7 sampling rate */
   uint32_t nscans;                  /*  8-11 # of scans */
   uint32_t triggerTime;             /* 12-15 */
   unsigned int triggerTimeMsec;     /* 16-17 */
   uint32_t startTime;               /* 18-21 */
   unsigned int startTimeMsec;       /* 22-23 */
   uint32_t triggerBitMap;           /* 24-27 , channel(s) triggered */
   uint32_t channelBitMap;           /* 28-31 = 32 bytes, map of channels in file */
   struct {
      int32_t maxPeak;               /*  0- 3 */
      uint32_t maxPeakOffset;        /*  4- 7 */
      int32_t minPeak;               /*  8-11 */
      uint32_t minPeakOffset;        /* 12-15 */
      float sensitivity;             /* 16-19 */
      float fullScale;               /* 20-23 */
      unsigned short gain;           /* 24-25 */
      char id[CHANNEL_ID_LENGTH];    /* 26-30 , 4 chars + NULL */
      char bytepad;                  /* 31 */
   } channel[ABS_MAX_CHANNELS];      /*   32 * 18 = 576 + 32 = 608 */
};


/*---------------------------------------------------*/
/* DIRENTRY from k2defs.h mcu                        */
/*---------------------------------------------------*/

#define ATTRIB_READONLY 0x01
#define ATTRIB_HIDDEN   0x02
#define ATTRIB_SYSTEM   0x04
#define ATTRIB_SUBDIR   0x10
#define ATTRIB_ARCHIEVE 0x20

/* dir entry structure for PKR_DIR packets */

#define K2_DIRENTRY_SIZE 22     /* size in bytes */

struct K2_DIRENTRY              /* 22 bytes exactly */
{
   uint32_t time;         /* creation time in seconds since Jan 1, 1980 */
   uint32_t size;         /* file size, in bytes */
   char ext[4];           /* extension, padded with spaces, NULL terminated */
   char name[9];          /* file name, padded with spaces, NULL terminated */
   unsigned char attribute;     /* file attribute:
                                   0011 0111
                                     ||  ||\_ReadOnly
                                     ||  |\_Hidden
                                     ||  \_System
                                     |\_Sub-directory
                                     \_Archieve
                                 */

};


/*---------------------------------------------------*/
/* ALARM  from k2defs.h mcu                          */
/*---------------------------------------------------*/
struct K2_ALARM
{
   uint32_t channelBitMap;      /* each bit represents a channel (bit 0=ch1)
                                   it means the channel alarm is triggered */
};


/*---------------------------------------------------*/
/* JMA  from mcu code for ETNA_SI                    */
/*      total size = 32 bytes                        */
/*      divide (*10) values by 10.0 before display   */
/*---------------------------------------------------*/
struct JMA
{
  unsigned int Ijma;     /* JMA 60 second Intnesity Scale 0-7.0 (*10) */
  unsigned int Ijma10;   /* JMA 10 second Intnesity Scale 0-7.0 (*10) */
  unsigned int SIvl;     /* SI value */
  unsigned int Acch;     /* Vector sum or horiz accel: Acch1 & Acch2 (*10) */
  unsigned int Accv;     /* Vertical Accel (*10) */
  unsigned int res1;     /* user selectable RES1 */
  unsigned int res2;     /* user selectable RES2 */
  unsigned int resC;     /* code definition of RES1, RES2, default: 1714 */
  unsigned int stat;     /* status */
  unsigned int resv;     /* reserved */
  uint32_t time;         /* seconds since 1/1/80 */
  unsigned int msec;     /* milliseconds  0..999 */
  unsigned int Acch1;    /* Horizontal 1 Accel (*10) */
  unsigned int Acch2;    /* Horizontal 2 Accel (*10) */
  unsigned int Accxyz;   /* Vector sum of accle: Acch1, Accv, Acch2 (*10) */
};


#define JMAPKT_SIZE 33   /* 1 + 32 */

struct JMAPKT
{
  unsigned char  code;   /* 0= current values,
                            1= peak values,
                            2= peak & clear */
  struct JMA  jma;
};

#define STRBLKHDR_SIZE 16

#define MAXSTREAMS 6     /* max # channels per DSP board */

                         /* Stream data block header & temp CRC area */
struct StrDataHdr
{
  unsigned short SerialNum;
  unsigned char StreamNum;    /* Logical channel #, i.e.
                                 chan 1-2-3-4 selected, # = 0..3,
                                 chan 3 selected, # = 0 */
  unsigned char Status;       /* bit 0..2 = data type:
                                            0= 24-bit uncompressed
                                            1= compressed */
  uint32_t DataSeq;           /* data block sequence - scan number */
  uint32_t Seconds;           /* seconds since 1/1/1980 */
  unsigned short Msecs;       /* milliseconds, 0..999 */
  unsigned char Reserved[2];  /* two bytes 0x0000 */
  uint32_t CRC;               /* temporary place for data block CRC,
                                  is sent after data bytes
                                  0x00, 0x00, crchigh, crclow */
};

struct StrSendCmd
{
  uint32_t DataSeq;           /* retry: # from bad block
                                 normal: # generated by host */
  unsigned char stream;       /* retry: # from bad block
                                 normal: stream # wanted  */
  unsigned char cmd;          /* 0 = mode 3 normal request
                                 1 = mode 1,2,3 retry */
};

struct StrRetryLogEntry
{
  unsigned char active;       /* 1 = waiting for retry to come in
                                 0 = got retry  */
  uint32_t DataSeq;           /* DataSeqNum of failed block */
  unsigned char stream;       /* stream # of failed block */
  unsigned int BufPtr;        /* buffer pointer to start of data */
};


/* ..... PACKET CODE TYPES TABLE  ....................... */

#define PKC_MSG               0x00
#define PKC_GETPARMS          0x01
#define PKC_SETPARMS          0x02
#define PKC_GETSTATUS         0x03
#define PKC_GETDIR            0x04
#define PKC_GETFILE           0x05
#define PKC_DELETEFILE        0x06
/* #define PKC_UNDELETEFILE      0x07  not used */
#define PKC_PING              0x08
#define PKC_SETALARM          0x09
#define PKC_ACQSTART          0x0A
#define PKC_ACQSTOP           0x0B
#define PKC_OPENDIR           0x0C
#define PKC_CLOSEDIR          0x0D
#define PKC_OPENFILE          0x0E
#define PKC_CLOSEFILE         0x0F
/* #define PKC_PACK              0x10  not used */
/* #define PKC_FORMAT            0x11  not used */
#define PKC_RMDIR             0x12
/* #define PKC_MKDIR             0x13  not used */
#define PKC_CHDIR             0x14
#define PKC_FT                0x15
#define PKC_GETDIRNAME        0x16
#define PKC_RESETALARM        0x17
#define PKC_TRIGGER           0x18
#define PKC_DETRIGGER         0x19
#define PKC_TIME              0x1A
#define PKC_GETEVENTSUMMARY   0x1B
#define PKC_PASSWORD          0x1C

#define PKC_SI                0x1D   /* ETNA-SI only */
#define PKC_EXTSTATUS         0x1E   /* Added: PNL */
#define PKC_STRCTRL           0x20   /* streams only */
#define PKC_STRSEND           0x21


#define PKR_ACK               0x80
#define PKR_PARMS             0x81
#define PKR_STATUS            0x83
#define PKR_DIR               0x84
#define PKR_FILE              0x85
#define PKR_EXTSTATUS         0x86   /* Added: PNL */
#define PKR_PING              0x88

#define PKR_STRNAK            0x89   /* streams only */
#define PKR_STRDATA           0x8A
#define PKR_STRRQDATA         0x8B

#define PKR_DIRNAME           0x96
#define PKR_EVENT_SUMMARY     0x9B

#define PKR_SI                0x9D   /* ETNA-SI only */


#endif

