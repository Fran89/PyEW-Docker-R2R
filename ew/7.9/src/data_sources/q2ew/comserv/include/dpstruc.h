/* DP Common data structure definitions
   Copyright 1994-1996 Quanterra, Inc.
   Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 17 Mar 94 WHO Derived from dpstruc.pas
    1  7 Apr 94 WHO Ultra status added to linkstat_rec.
    2 13 Apr 94 WHO Client_info added.
    3 16 Apr 94 WHO Seed version and station description added to linkstat.
    4 30 May 94 WHO comm detector mask and ultra revision added to ultra_rec.
                    definition for parameter for CSCM_COMM_EVENT changed
                    to structure.
    5  9 Jun 94 WHO Arrays defined as [] replaced with [VARIABLE] to avoid
                    warnings. (DSN)
    6  9 Aug 94 WHO Cosmetic change to SOH flags (DSN). Add last_good & 
                    last_bad to linkstat_rec.
    7 11 Aug 94 WHO More SOH changes to satisfy the anal C language.
    8 12 Dec 94 WHO cl_spec_type moved to quanstrc.h, since it is no longer
                    passed to users.
    9 20 Jun 95 WHO linkadj_com changed to more accurately reflect purposes.
                    link_record updated as well. Linkstat_rec enhanced.
                    linkset_com added.
   10  5 Jun 96 WHO Start of conversion to run on OS9.
   11 22 Jul 96 WHO Update list of clocks.
   12 17 Oct 97 WHO Add VER_DPSTRUC.
*/

/* Make sure pascal.h is included */
#ifndef cs_pascal_h
#include "pascal.h"
#endif

/* Flag this file as included */
#define cs_dpstruc_h
#define VER_DPSTRUC 12
  
/* Boolean constants */
#define FALSE 0
#define TRUE 1 /* Must be 1 for compatibility with Pascal */

/* Indication of variable length array in use. */
#define VARIABLE 1

/* Quanterra SOH flags */
#define SOH_INACCURATE 0x80            /* inaccurate time tagging, in SOH */
#define SOH_GAP 0x40                   /* time gap detected, in SOH */
#define SOH_EVENT_IN_PROGRESS 0x20     /* record contains event data */
#define SOH_BEGINNING_OF_EVENT 0x10    /* record is first of event sequence */
#define SOH_CAL_IN_PROGRESS 0x08       /* record contains calibration data */
#define SOH_EXTRANEOUS_TIMEMARKS 0x04  /* too many time marks received during this record */
#define SOH_EXTERNAL_TIMEMARK_TAG 0x02 /* record is time-tagged at a mark */
#define SOH_RECEPTION_GOOD 0x01        /* time reception is adequate */

/* Compressed record sizes */
#define TOTAL_FRAMES_PER_RECORD 8    /* Maximum, may be shorter */
#define WORDS_PER_FRAME 16           /* there are 4 bytes * 16 = 64 bytes per frame */
#define MAXSAMPPERWORD 9             /* this is maximum using level 3 */

/* Calibration structure limits */
#define MAXCAL 8       /* maximum number of calibrators */
#define MAXCAL_FILT 4  /* maximum number of calibrator filters */

/* Comm Event count and maximum length */
#define CE_MAX 32      /* maximum number of comm events */
#define COMMLENGTH 11  /* number of characters in a comm event name*/
#define COMMENT_STRING_LENGTH 132

/* Clock models */
#define CL_UNKNOWN 0
#define CL_OS9 1         /* OS9 internal */
#define CL_KIN_GOES 2    /* Kinemetrics Goes */
#define CL_KIN_OMEGA 3   /* Kinemetrics Omega */
#define CL_KIN_DCF 4     /* Kinemetrics DCF */
#define CL_MEIN_UA31S 5  /* Meinburg UA31S */
#define CL_GPS1_QTS 6    /* GPS1 QTS format */
#define CL_GPS1_GOES 7   /* GPS1 Goes emulation */
#define CL_GPS1_UA31S 8  /* GPS1 UA31S emulation */
#define CL_GPS2_QTS2 9   /* GPS2 QTS2 format */
#define CL_K2_GPS 10     /* Altus K2 with GPS */

/* Calibrator waveform types */
#define SINE 0
#define STEP 1
#define RAND 2   /* Red noise */
#define WRAND 3  /* White noise */

/* Sine wave frequencies */
#define Hz_DC 0
#define Hz25_000 1
#define Hz10_000 2
#define Hz8_0000 3
#define Hz6_6667 4
#define Hz5_0000 5
#define Hz4_0000 6
#define Hz3_3333 7
#define Hz2_5000 8
#define Hz2_0000 9
#define Hz1_6667 10
#define Hz1_2500 11
#define Hz1_0000 12
#define Hz0_8000 13
#define Hz0_6667 14
#define Hz0_5000 15
#define Hz0_2000 16
#define Hz0_1000 17
#define Hz0_0667 18
#define Hz0_0500 19
#define Hz0_0400 20
#define Hz0_0333 21
#define Hz0_0250 22
#define Hz0_0200 23
#define Hz0_0167 24
#define Hz0_0125 25
#define Hz0_0100 26
#define Hz0_0050 27
#define Hz0_0020 28
#define Hz0_0010 29
#define Hz0_0005 30

/* Predefined event detector types */
#define MURDOCK_HUTT 0
#define THRESHOLD 1

/* Clock correction types */
#define NO_CLOCK_CORRECTION 0
#define MISSING_TIMEMARK 1
#define EXPECTED_TIMEMARK 2
#define VALID_TIMEMARK 3
#define UNEXPECTED_TIMEMARK 4
#define DAILY_TIME_CORRECTION_REPORT 5

/* Difference in seconds between 1970 and 1984 */
#define SECCOR 441763200

/* Alias some names. Make sure that logical not is used on boolean
   values rather than bitwise not. Non-O/S specific values should
   never be declared as "int" or "unsigned int" since this may 16
   bits on some systems, and 32 bits on others, use short or long
   instead.
*/
#define boolean unsigned char
#define byte unsigned char

/* instead of pascal 0..31 sets, bitmaps are used */
typedef long psuedo_set ;

/* likewise, channel maps are represented by bitmaps */
typedef long chan_map ;

typedef char *pchar ;
typedef void *pvoid ;

/* types defined as "stringxx" are pascal format strings. The first byte is
   the dynamic length, followed by up to xx characters */
typedef char string15[16] ;
typedef char string23[24] ;
typedef char string59[60] ;
typedef char string79[80] ;

/* Common fields within SEED headers and other structures */
typedef char seed_name_type[3] ;
typedef char seed_net_type[2] ;
typedef char location_type[2] ;

/* 2 byte and 4 byte unions */
typedef union
  begin
    unsigned char b[2] ;
    signed char sb[2] ;
    short s ;
  end compword ;
typedef union
  begin
    unsigned char b[4] ;
    signed char sb[4] ;
    short s[2] ;
    long l ;
    float f ;
  end complong ;

/* Structure returned for CSIM_LINK */
typedef struct
  begin
    byte window_size ;  /* window size, max pending is window_size -1 */
    byte total_prio ;   /* total number of priority levels */
    byte msg_prio ;     /* message record priority */
    byte det_prio ;     /* detection packets priority */
    byte time_prio ;    /* timing packets priority */
    byte cal_prio ;     /* calibration packets priority */
    byte link_format ;  /* link format (see CSF_xxx above */
    boolean rcecho ;    /* True if command confirmations expected */
    short resendtime ;  /* resend packets timeout in seconds */
    short synctime ;    /* sync packet time in seconds */
    short resendpkts ;  /* packets in resend blocks */
    short netdelay ;    /* network restart delay in seconds */
    short nettime ;     /* network connect timeout in seconds */
    short netmax ;      /* unacked network packets before considered timeout */
    short groupsize ;   /* group packet count */
    short grouptime ;   /* group timeout in seconds */
   end link_record ;

/* Duration information for each type of waveform */
typedef struct
  begin
    long min_dur ; /* minimum duration */
    long max_dur ; /* maximum duration */
    long inc_dur ; /* duration increment */
  end tdurations ;

/* Description of one calibrator returned from CSIM_CAL */
typedef struct
  begin
    boolean coupling_option ;       /* supports capacitor/resistor coupling */
    boolean polarity_option ;       /* supports plus and minus step */
    short board ;                   /* which board this goes with */
    short min_settle ;              /* minimum settling time in seconds */
    short max_settle ;              /* maximum settling time in seconds */
    short inc_settle ;              /* supports settling time in inc seconds */
    short min_mass_dur ;            /* minimum duration in ms */
    short max_mass_dur ;            /* maximum duration in ms, 0=none */
    short inc_mass_dur ;            /* duration increment */
    short def_mass_dur ;            /* default duration in ms */
    short min_filter ;              /* minimum filter number */
    short max_filter ;              /* maximum filter number */
    short min_amp ;                 /* minimum amplitude in db */
    short max_amp ;                 /* maximum amplitude in db */
    short amp_step ;                /* amplitude step in db */
    short monitor ;                 /* channel that monitors the calibrator */
    short rand_min_period ;         /* minimum random period */
    short rand_max_period ;         /* maximum random period */
    short default_step_filt ;       /* default step filter */
    short default_rand_filt ;       /* default rand filter */
    short ct_sp2 ;                  /* always zero */
    tdurations durations[WRAND+1] ; /* Durations for each waveform */
    chan_map map ;                  /* channels this calibrator calibrates */
    psuedo_set waveforms ;          /* supported waveforms */
    psuedo_set sine_freqs ;         /* supported sine frequencies */
    psuedo_set default_sine_filt[MAXCAL_FILT] ;
    string23 name ;                 /* name of the calibrator board */
    string59 filtf ;                /*filter description string*/
   end eachcal ;

/* Header returned from CSIM_CAL */
typedef struct
  begin
    short number ;          /* actual number of active calibrators */
    boolean mass_ok ;       /* non-zero if mass recentering ok */
    byte ct_sp1 ;           /* always zero */
    eachcal acal[MAXCAL] ;  /* the calibrators */
  end cal_record ;

/* Structure returned from CSIM_DIGI */
typedef struct
  begin
    string23 name ;            /* digitizer name */
    string23 version ;         /* program version number */
    string79 clockmsg;         /* available clock messages */
    boolean prefilter_ok ;     /* server supports prefiltering */
    boolean detector_load_ok ; /* server supports loadable detectors */
    boolean setmap_ok ;        /* server will process a set map command */
    boolean clockstring_ok ;   /* server will return clockstring */
    boolean int_ext_ok ;       /* two mark sources */
    boolean send_message_ok ;  /* can send digitizer messages */
    boolean message_chan_ok ;  /* messages can be sent to channels */
    boolean set_osc_ok ;       /* can set the master oscillator */
    boolean set_clock_ok ;     /* can set the clock */
    byte wait_for_data ;       /* number of seconds to wait for data */
    byte dt_sp1 ;              /* always zero */
    byte dt_sp2 ;              /* always zero */
  end digi_record ;

/* Structure returned from CSIM_CHAN, each follows immediately after the next.
   The list of channels is terminated by a NUL ('\0') character as the first
   character in the SEED name. For the Available and Enabled masks, the bits are :
      bit 0 = continous on this comm link
      bit 1 = event on this comm link
      bit 2 = continous on tape
      bit 3 = event on tape
      bit 4 = continous on disk
      bit 5 = event on disk
*/
typedef struct
  begin
    seed_name_type seedname ; /* channel_id, 3 characters */
    byte stream ;             /* Internal DA stream identification */
    location_type seedloc ;   /* location, right padded with spaces */
    byte physical ;           /* physical channel number, 0 if not QSRVD */
    byte available ;          /* can be enabled */
    byte enabled ;            /* currently enabled */
    byte det_count ;          /* number of detectors */
    byte c_prio ;             /* comlink continuous priority */
    byte e_prio ;             /* comlink event priority */
    short rate ;              /* sampling rate, + is samp/sec, - is sec/samp */
  end chan_record ;

/* Structure to return all requested channels */
typedef struct
  begin
    short chancount ;             /* Number of active channels */
    chan_record chans[VARIABLE] ; /* Array of channels */
  end chan_struc ;

/* Structure returned from CSIM_ULTRA call */
typedef struct
  begin
    short vcovalue ;                        /* current vco value */
    boolean pllon ;                         /* true if PLL controlling PLL */
    boolean umass_ok ;                      /* copy of cal.mass_ok */
    long comm_mask ;                        /* Current comm detector mask */
    byte ultra_rev ;                        /* Ultra revision */
    char commnames[CE_MAX*(COMMLENGTH+1)] ; /* Comm event names */
  end ultra_rec ;

/* Structure returned for the CSIM_LINKSTAT call */
typedef struct
  begin
    boolean ultraon ;      /* Ultra Shear mode */
    boolean linkrecv ;     /* Link packet received */
    boolean ultrarecv ;    /* Ultra packet received */
    boolean suspended ;    /* Suspended */
    long total_packets ;   /* total number of packets */
    long sync_packets ;    /* number of sync packets */
    long seq_errors ;      /* number of sequence errors */
    long check_errors ;    /* number of checksum errors */
    long io_errors ;       /* number of I/O errors */
    long lastio_error ;    /* last io error number */
    long blocked_packets ; /* number of blocked packets */
    long seconds_inop ;    /* seconds in operation */
    double last_good ;     /* Time of last good packet received */
    double last_bad ;      /* Time of last bad packet received */
    char seedformat[4] ;   /* Seed format (such as V2.3) */
    char seedext ;         /* Seed extension level */
    byte data_format ;     /* Data format */
    string59 description ; /* Station description */
    short lsr_sp1 ;        /* For longword alignment */
    long pollusecs ;       /* server polling delay */
    long reconcnt ;        /* how many errors to do reconfigure */
    long net_idle_to ;     /* no packets received timeout */
    long net_conn_dly ;    /* network connection polling delay */
    long grpsize ;         /* ack packet grouping */
    long grptime ;         /* ack pakcet grouping timeout */
  end linkstat_rec ;

/* Structure returned from the CSIM_COMSTAT call */
typedef struct
  begin
    long command_tag ;       /* command tag */
    long completion_status ; /* completion status */
    long moreinfo ;          /* this is really variable length */
  end comstat_rec ;

/* Structure returned from the CSCM_CLIENTS call */
typedef struct
  begin
    int client_memid ;       /* Client's shared memory ID */
    int client_pid ;         /* Client's process ID */
    complong client_name ;   /* Client's name */
    double last_service ;    /* Time of last service */
    long timeout ;           /* Timeout value */
    short block_count ;      /* Number of packets blocked by this client */
    boolean blocking ;       /* True if blocking used */
    boolean active ;         /* True if client active */
    boolean reserved ;       /* Reserved client */
  end one_client ;
  
typedef struct
  begin
    short client_count ;           /* Number of clients */
    one_client clients[VARIABLE] ; /* One for each client */
  end client_info ;
    
/* CSCM_SHELL command structure */
typedef struct
  begin
    string79 shell_parameter ; /* parameter string for DA shell */
    boolean log_local ;        /* set to TRUE to log results on DA */
    boolean log_host ;         /* set to TRUE to log results in host msg log */
  end shell_com ;

/* CSCM_VCO command structure */
typedef short vco_com ;

/* CSCM_LINKADJ command structure */
typedef struct
  begin
    short window_size ; /* window size, 2 to 64 */
    byte set_msg ;      /* message record priority */
    byte set_det ;      /* detection packet priority */
    byte set_time ;     /* timing packet priority */
    byte set_calp ;     /* calibration packet priority */
    short resendtime ;  /* resend packets timeout in seconds */
    short synctime ;    /* sync packet time in seconds */
    short resendpkts ;  /* packets in resend blocks */
    short netdelay ;    /* network restart delay in seconds */
    short nettime ;     /* network connect timeout in seconds */
    short netmax ;      /* unacked network packets before being considered timeout */
    short groupsize ;   /* group packet count */
    short grouptime ;   /* group timeout in seconds */
    short lasp1 ;
    short lasp2 ;
  end linkadj_com ;

/* CSCM_LINKSET command structure */
typedef struct
  begin
    long pollusecs ;       /* server polling delay */
    long reconcnt ;        /* how many errors to do reconfigure */
    long net_idle_to ;     /* no packets received timeout */
    long net_conn_dly ;    /* network connection polling delay */
    long grpsize ;         /* ack packet grouping */
    long grptime ;         /* ack pakcet grouping timeout */
   end linkset_com ;
  
/* CSCM_MASS_RECENTER command structure */
typedef struct
  begin
    short board ;    /* calibrator board number */
    short duration ; /* duration in milliseconds */
  end recenter_com ;

/* CSCM_CAL_START command structure */
typedef struct
  begin
    byte calcmd ;        /* such as SINE or STEP */
    byte sfrq ;          /* sine frequency */
    boolean plus ;       /* set to TRUE for positive step */
    boolean capacitor ;  /* resistive = FALSE, capacitive = TRUE */
    byte autoflag ;      /* non zero for automatic calibration */
    byte ext_sp1 ;       /* set to zero */
    short calnum ;       /* calibrator board number */
    long duration ;      /* in seconds, 0=infinate */
    short amp ;          /* amplitude, in DB */
    short rmult ;        /* random multiplier */
    short map ;          /* channel map local to this board */
    short settle ;       /* relay settling time, in seconds */
    short filt ;         /* filter to use 1..MAXCAL_FILT */
    short ext_sp2 ;      /* set to zero */
  end cal_start_com ;

/* CSCM_CAL_ABORT command structure */
typedef short board ;

/* Detector blocks for CSCM_DET_ENABLE command */
typedef struct
  begin
    short detector_id ; /* Detector ID to enable */
    boolean enable ;    /* set to TRUE to enable detector */
    byte de_sp1 ;       /* Set to zero */
  end det_en_entry ;

/* CSCM_DET_ENABLE command structure */
typedef struct
  begin
    short count ;                /* number of valid entries */
    det_en_entry detectors[20] ; /* up to 20 detector entries */
  end det_enable_com ;

/* Substructure used for CSCM_DET_CHANGE command and as part of CSCM_DET_REQUEST response.
   NOTE: The SUN ANSI C compiler incorrectly calculates the size of this structure
   as 44 bytes, when it is really 42 bytes. Therefor when this structure is
   referenced in another structure, use and array of 42 bytes instead
*/
typedef struct
  begin
    long filhi, fillo ;     /* threshold limits */
    long iwin ;             /* window length in samples & threshold hysterisis */
    long n_hits ;           /* #P-T >= th2 for detection & threshold min. dur. */
    long xth1, xth2, xth3, xthx ;
    long def_tc ;
    long wait_blk ;         /* samples to wait before new detection */
    short val_avg ;
  end shortdetload ;

/* CSCM_DET_CHANGE command structure */
typedef struct
  begin
    short id ;          /* detector id */
    boolean enab ;      /* set to TRUE to enable */
    byte dct_sp ;       /* set to zero */
    byte ucon[42] ;     /* parameters for detector, should be shortdetload */
  end det_change_com ;

/* Structure for changing one recording channel */
typedef struct
  begin
    seed_name_type seedname ; /* channel id */
    byte mask ;               /* enable mask */
    location_type seedloc ;   /* location */
    byte c_prio ;             /* comlink continous priority */
    byte e_prio ;             /* comlink event priority */
    short rec_sp1 ;           /* set to zero */
  end rec_one ;

/* CSCM_REC_ENABLE command structure */
typedef struct
  begin
    short count ;        /* number of changes */
    rec_one changes[8] ; /* channels to change */
  end rec_enable_com ;

/* CSCM_COMM_EVENT command structure */
typedef struct
  begin
    psuedo_set remote_map ;  /* 1 = on, 0 = off, per bit */
    psuedo_set remote_mask ; /* 1 = change this bit per remote_map, 0 = ignore bit */
  end comm_event_com ;

/* Detector description used as the result of the CSCM_DET_REQUEST command */
typedef struct
  begin
    boolean enabled ;     /* detector running */
    boolean remote ;      /* is a remote detector */
    byte dettype ;        /* standard detector types */
    byte dd_sp1 ;         /* always zero */
    byte cons[42] ;       /* detector parameters, should be shortdetload */
    short id ;            /* detector ID */
    string23 name ;       /* detector name */
    string15 params[12] ; /* params[0] is detector type name, 1..11 are parameter names */
  end det_descr ;

/* CSCM_DET_REQUEST response record */
typedef struct
  begin
    short count ;        /* number of detectors described */
    short dat_sp1 ;      /* always zero */
    det_descr dets[20] ; /* up to 20 detector descriptions */
  end det_request_rec ;

/* CSCM_DOWNLOAD command structure */
typedef struct
  begin
    string59 dasource ;  /* source file name */
    string23 dpmodname ; /* DP data module name for OS9 host */
   end download_com ;

/* CSCM_UPLOAD command structure */
typedef struct
  begin
    string59 dadest ;      /* destination file name */
    string23 dpmodname ;   /* DP data module name for OS9 host */
    int dpshmid ;          /*   or shared memory segment for Unix host */
    unsigned short fsize ; /* file size in bytes */
  end upload_com ;

/* Download status (command output) */
typedef struct
  begin
    int dpshmid ;               /* Shared memory segment for Unix host */
    unsigned short fsize ;      /* file size in bytes */
    unsigned short byte_count ; /* Bytes transferred so far */
  end download_result ;

/* Upload status (command output) */
typedef struct
  begin
    unsigned short bytecount ;  /* bytes transferred so far */
    short retries ;             /* packets resent */
  end upload_result ;

typedef byte tupbuf[65000] ; /* up/download buffer (data module/shared memory */
 
/* Definitions for a compressed frame */
typedef complong compressed_frame[WORDS_PER_FRAME] ;

/* Compressed frame array cannot be translated into C correctly, since the starting
   index should be 1 (0 is the header frame) */
typedef compressed_frame compressed_frame_array[TOTAL_FRAMES_PER_RECORD-1] ;

