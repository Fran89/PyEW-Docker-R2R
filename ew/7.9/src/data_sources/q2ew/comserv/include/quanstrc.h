
/* DA-DP Communications structures
   Copyright 1994-1996 Quanterra, Inc.
   Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 18 Mar 94 WHO Derived from dpstruc.pas
    1 30 May 94 WHO ultra_rev and comm_mask added to ultra_type. Typos
                    found by DSN corrected.
    2 25 Sep 94 WHO RECORD_HEADER_3 added.
    3 13 Dec 94 WHO cl_spec_type moved here from dpstruc.h.
    4 17 Jun 95 WHO link_adj_msg removed, link_adj_struc.la now is linkadj_com.
    5 13 Jun 96 WHO seedname and location added to clock_exception and
                    commo_comment.
    6  3 Dec 96 WHO Add BLOCKETTE.
    7 27 Jul 97 WHO Add FLOOD_PKT and FLOOD_CTRL.
    8 17 Oct 97 WHO Add VER_QUANSTRC.
*/

/* Make sure seedstrc.h is included, this forces including pascal.h 
   and dpstruc.h */
#ifndef cs_seedstrc_h
#include "seedstrc.h"
#endif

/* Flag this file as included */
#define cs_quanstrc_h
#define VER_QUANSTRC 8
 
/* Quanterra Record types */
#define EMPTY 0
#define RECORD_HEADER_1 1
#define RECORD_HEADER_2 2
#define CLOCK_CORRECTION 3
#define COMMENTS 4
#define DETECTION_RESULT 5
#define RECORD_HEADER_3 6
#define BLOCKETTE 7
#define FLOOD_PKT 8
#define END_OF_DETECTION 9
#define CALIBRATION 10
#define FLUSH 11
#define LINK_PKT 12
#define SEED_DATA 13
#define ULTRA_PKT 16
#define DET_AVAIL 17
#define CMD_ECHO 18
#define UPMAP 19
#define DOWNLOAD 20

/* Time structures */
typedef struct
  begin
    byte year ;
    byte month ;
    byte day ;
    byte hour ;
    byte min ;
    byte sec ;
  end time_array ;

/* Quanterra data record header */
typedef struct
  begin
    long header_flag ;                  /* flag word for header frame */
    byte frame_type ;                   /* standard frame offset for frame type */
    byte component ;                    /* not valid in QSL */
    byte stream ;                       /* not valid in QSL */
    byte soh ;                          /* state-of-health bits */
    complong station ;                  /* 4 byte station name */
    short millisec ;                    /* time mark millisec binary value */
    short time_mark ;                   /* sample number of time tag, usually 1 */
    long samp_1 ;                       /* 32-bit first sample in record */
    short clock_corr ;                  /* last clock correction offset in milliseconds */
    unsigned short number_of_samples ;  /* number of samples in record */
    signed char rate ;                  /* samp rate: + = samp/sec; - = sec/samp */
    byte blockette_count ;
    time_array time_of_sample ;         /* time of the tagged sample */
    long packet_seq ;
    byte header_revision ;
    byte detection_day ;
    short detection_seq ;
 /* Following fields are valid in QSL format only */
    long clock_drift ;
    seed_name_type seedname ;
    signed char clkq ;
    seed_net_type seednet ;
    location_type location ;
    short ht_sp3 ;
    short microsec ;                    /* time mark microsec binary value */
    byte frame_count ;
    byte hdrsp1 ;
    byte z[5] ;
    byte ht_sp4 ;
  end header_type ;

/* 512 byte Quanterra data frame */
typedef struct
  begin
    header_type h ;
    compressed_frame_array frames ;
  end compressed_record ;

/* Event reporting structure */
typedef struct
  begin
    long jdate ;                        /* seconds since Jan 1, 1984 00:00:00 */
    short millisec ;                    /* 0..999 fractional part of time */
    byte component ;                    /* not valid in QSL */
    byte stream ;                       /* not valid in QSL */
    long motion_pick_lookback_quality ; /* "string" of this gives, eg "CA200999" */
    long peak_amplitude ;               /* & threshold sample value */
    long period_x_100 ;                 /* 100 times detected period */
    long background_amplitude ;         /* & threshold limit exceeded */
  /* Following fields are valid in QSL Format only */
    string23 detname ;                  /* detector name */
    seed_name_type seedname ;           /* seed name */
    byte sedr_sp1 ;                     /* zero */
    location_type location ;            /* seed location */
    short ev_station ;                  /* station name NOT LONGWORD ALIGNED-COMPLONG */
    short ev_station_low ;
    seed_net_type ev_network ;          /* seed network */
  end squeezed_event_detection_report ;

/* time quality. The SUN ANSI C compiler incorrectly calculates the size of
   this structure as 8 bytes instead of 6. Where it is used it is substitued
   with an 6 byte array. */
typedef struct
  begin
    long msec_correction ;             /* last timemark offset in milliseconds */
    byte reception_quality_indicator ; /* from Kinemetrics clock */
    byte time_base_VCO_correction ;    /* time base VCO control byte */
  end time_quality_descriptor ;

/* calibration reporting types */
#define SINE_CAL 0
#define RANDOM_CAL 1
#define STEP_CAL 2
#define ABORT_CAL 3

/* Clock specific information */
typedef struct
  begin
    byte model ;        /* model type from above */
    byte cl_seq ;       /* 1-255 sequence number */
    byte cl_status[6] ; /* clock specific status */
  end cl_spec_type ;

/* Due to flaws in the alignment rules on the SUN ANSI C compiler, the
   Pascal variant record definition cannot be directly translated into
   C structure/unions, they must be separated.
*/

/* Clock exception portion of eventlog_struc. Do to flaws in the SUN ANSI
   C compiler, time_quality_descriptor cannot be used (the compiler thinks
   it takes 8 bytes or requires long alignment) so a byte array is used
   in it's place. */
typedef struct
  begin
    byte frame_type ;
    byte clock_exception ;                 /* type of exception condition */
    time_array time_of_mark ;              /* if clock_exception = EXPECTED, VALID, DAILY, or UNEXPECTED */
    long count_of ;                        /* seconds elapsed or consecutive marks */
    byte correction_quality[6] ;
    short usec_correction ;                /* timemark offset, 0 - 999 microseconds*/
    long clock_drift ;                     /* clock drift in microseconds */
    short vco ;                            /* full VCO value */
    cl_spec_type cl_spec ;                 /* clock specific information */
    short cl_station ;                     /* station name NOT LONG ALIGNED! */
    short cl_station_low ;
    seed_net_type cl_net ;                 /* network */
    location_type cl_location ;            /* location */
    seed_name_type cl_seedname ;           /* seed name */
  end clock_exception ;

/* Detection result portion of eventlog_struc */
typedef struct
  begin
    byte frame_type ;                      /* type of exception condition */
    byte detection_type ;
    time_array time_of_report ;            /* time that detection is reported */
    squeezed_event_detection_report pick ;
  end detection_result ;

/* Calibration result portion of eventlog_struc. Do to flaws in the SUN ANSI
   C compiler, the "calibration_report_struc" had to be merged in here. Code
   will need to be adjusted accordingly. */
typedef struct
  begin
    byte frame_type ;
    byte word_align ;
    short cr_duration ;                /* duration in seconds NOT LONG ALIGNED -LONG*/
    short cr_duration_low ;
    short cr_period ;                  /* period in milliseconds NOT LONG ALIGNED-LONG */
    short cr_period_low ;
    short cr_amplitude ;               /* amplitude in DB */
    time_array cr_time ;               /* time of signal on or abort */
    byte cr_type ;
    byte cr_component ;                /* not valid in QSL */
    byte cr_stream ;                   /* not valid in QSL */
    byte cr_input_comp ;               /* not valid in QSL */
    byte cr_input_strm ;               /* not valid in QSL */
    byte cr_stepnum ;                  /* number of steps? */
    byte cr_flags ;                    /* bit 0 = plus, bit 2 = automatic, bit 4 = p-p */
/* Following are valid in QSL format only */
    byte cr_flags2 ;                   /* bit 0 = cap, bit 1 = white noise */
    short cr_0dB ;                     /* 0 = dB, <> 0 = value for 0dB NOT LONG ALIGNED-FLOAT */
    short cr_0dB_low ;
    seed_name_type cr_seedname ;       /* seed name */
    byte cr_sfrq ;                     /* calibration frequency if sine */
    location_type cr_location ;        /* seed location */
    seed_name_type cr_input_seedname ; /* seed name of cal input */
    byte cr_filt ;                     /* filter number */
    location_type cr_input_location ;  /* location of cal input */
    short cr_station ;                 /* station name NOT LONG ALIGNED-COMPLONG */
    short cr_station_low ;
    seed_net_type cr_network ;         /* network */
  end calibration_result ;

/* The full eventlog_struc as a union */
typedef union
  begin
    clock_exception clk_exc ;
    detection_result det_res ;
    calibration_result cal_res ;
  end eventlog_struc ;
  
#define DEFAULT_WINDOW_SIZE 8  /* sliding comm window default size */
#define LEADIN_ACKNAK 'Z'      /* lead-in char for ascii ack/nak packet */
#define LEADIN_CMD 'X'         /* lead-in char for ascii cmd packet */

/* Command types from DP to DA */
#define ACK_MSG 48
#define SHELL_CMD 50
#define NO_CMD 51
#define START_CAL 54
#define STOP_CAL 55
#define MANUAL_CORRECT_DAC 60
#define AUTO_DAC_CORRECTION 61
#define ACCURATE_DAC_CORRECTION 68
#define MASS_RECENTERING 69
#define COMM_EVENT 70
#define LINK_REQ 71
#define LINK_ADJ 72
#define DOWN_ABT 73
#define FLOOD_CTRL 74
#define ULTRA_REQ 77
#define ULTRA_MASS 78
#define ULTRA_CAL 79
#define ULTRA_STOP 80
#define DET_REQ 81
#define DET_ENABLE 82
#define DET_CHANGE 83
#define REC_ENABLE 84
#define DOWN_REQ 85
#define UPLOAD 86

/* Sequence control types */
#define SEQUENCE_INCREMENT 0
#define SEQUENCE_RESET 1
#define SEQUENCE_SPECIAL 2

/* Misc. maximum lengths */
#define DP_TO_DA_MESSAGE_LENGTH 100  /* size on bytes of DP_to_DA message */
#define MAX_REPLY_BYTE_COUNT 444

/* there is only one kind of comment format */
#define DYNAMIC_LENGTH_STRING = 0

/* comments are pascal strings, not C strings */
typedef char comment_string_type[COMMENT_STRING_LENGTH+1] ;

/* Event records */
typedef struct
  begin
    long header_flag ;
    eventlog_struc header_elog ;
  end commo_event ;

/* Comment records */
typedef struct
  begin
    long header_flag ;
    byte frame_type ;
    byte comment_type ;
    time_array time_of_transmission ;
    comment_string_type ct ;
    byte cc_pad ;
/*
  In QSL Format the following fields are moved up past the end
  of the comment string at transmission, and moved back to their
  proper locations at reception. This is to improve throughput.
  They are not valid in any other format.
*/
    short cc_station ; /* NOT LONG ALIGNED-COMPLONG */
    short cc_station_low ;
    seed_net_type cc_net ;
    location_type cc_location ;
    seed_name_type cc_seedname ;
  end commo_comment ;

/* Reply to DP Record */
typedef struct
  begin
    long header_flag ;
    byte frame_type ;
    boolean first_seg ;
    unsigned short total_bytes ;
    short total_seg ;
    short this_seg ;
    unsigned short byte_offset ;
    unsigned short byte_count ;
    byte bytes[MAX_REPLY_BYTE_COUNT] ;
  end commo_reply ;

/* Link information Record */
typedef struct
  begin
    long header_flag ;
    byte frame_type ;
    boolean rcecho ;
    short seq_modulus ;
    short window_size ;
    byte total_prio ;
    byte msg_prio ;
    byte det_prio ;
    byte time_prio ;
    byte cal_prio ;
    byte link_format ;
    short resendtime ;  /* resend packets timeout in seconds */
    short synctime ;    /* sync packet time in seconds */
    short resendpkts ;  /* packets in resend blocks */
    short netdelay ;    /* network restart delay in seconds */
    short nettime ;     /* network connect timeout in seconds */
    short netmax ;      /* unacked network packets before timeout */
    short groupsize ;   /* group packet count */
    short grouptime ;   /* group timeout in seconds */
   end commo_link ;

typedef byte seg_map_type[128] ;
typedef char string63[64] ;

/* Upload map sent back to DP */
typedef struct
  begin
    long header_flag ;
    byte frame_type ;
    boolean upload_ok ;
    string63 fname ;
    seg_map_type segmap ;
  end commo_upmap ;

/* Record from DA to DP is a union of the above */
typedef union
  begin
    compressed_record cr ;
    commo_event ce ;
    commo_comment cc ;
    commo_reply cy ;
    commo_link cl ;
    commo_upmap cu ;
  end commo_record ;

/* This isn't incorporated into the buffers, since the SUN ANSI C compiler
   calculates the size of this structure as 8 bytes instead of 6
*/
typedef struct
  begin
    short chksum ;   /* chksum not calc over error pkt */
    short crc ;      /* first gen crc, then make checksum NOT LONG ALIGNED-LONG */
    short crc_low ;
  end error_control_packet ;

/* The record is packaged with a sequence number, a control field, the actual data, and error control */
typedef struct
  begin
    short skipped ;          /* START FILLING BUFFER AT "seq", THIS ASSURES THAT
                                THE DATA BUFFER IS LONG ALIGNED */
    byte seq ;
    byte ctrl ;              /* what to do with "Seq" in DP */
    commo_record data_buf ;  /* Compressed seismic data buffer */
    short chksum ;   /* chksum not calc over error pkt */
    short crc ;      /* first gen crc, then make checksum NOT LONG ALIGNED-LONG */
    short crc_low ;
  end DA_to_DP_buffer ;

/* Upload control values */
#define CREATE_UPLOAD 0
#define SEND_UPLOAD 1
#define ABORT_UPLOAD 2
#define UPLOAD_DONE 3
#define MAP_ONLY 4

/* Download control structure */
typedef struct
  begin
    boolean filefound ;
    boolean toobig ;
    unsigned short file_size ;
    string63 file_name ;
    byte contents[65000] ;
  end download_struc ;

/* Messages from DP to DA. While it might be possible to duplicate the
   elegant structure definition in the Pascal version, the C equivalent
   would be a nightmare, therefor things are declared as individual
   structures with duplicated header fields and then merged into a
   group later.
*/
/* The infamous stokely structure */
typedef struct
  begin
    byte param0 ;
    byte dummy2 ;
    short param1 ;
    short param2 ;
    short param3 ;
    short param4 ;
    short param5 ;
    short param6 ;
    short param7 ;
  end stokely ;
  
 typedef struct /* AUTO_DAC_CORRECTION, START_CAL, ACCURATE_DAC_CORRECTION */
  begin
    byte cmd_type ;
    byte dp_seq ;
    stokely cmd_parms ;
  end mm_commands ;

typedef struct /* COMM_EVENT */
  begin
    byte cmd_type ;
    byte dp_seq ;
    short rc_sp1 ;
    long mask ;
  end comm_event_struc ;

typedef struct /* ULTRA_MASS */
  begin
    byte cmd_type ;
    byte dp_seq ;
    short mbrd ;
    short mdur ;
  end ultra_mass_struc ;

typedef struct /* ULTRA_CAL */
  begin
    byte cmd_type ;
    byte dp_seq ;
    short rc_sp2 ;
    cal_start_com xc ;
  end ultra_cal_struc ;
  
typedef struct /* ULTRA_STOP */
  begin
    byte cmd_type ;
    byte dp_seq ;
    short sbrd ;
  end ultra_stop_struc ;

typedef struct /* DET_REQ */
  begin
    byte cmd_type ;
    byte dp_seq ;
    seed_name_type dr_name ;
    byte rc_sp3 ;
    location_type dr_loc ;
  end det_req_struc ;

typedef struct /* DET_ENABLE */
  begin
    byte cmd_type ;
    byte dp_seq ;
    short rc_sp4 ;
    det_enable_com de ;
  end det_enable_struc ;

typedef struct /* DET_CHANGE */
  begin
    byte cmd_type ;
    byte dp_seq ;
    short rc_sp5 ;
    det_change_com dc ;
  end det_change_struc ;

typedef struct /* REC_ENABLE */
  begin
    byte cmd_type ;
    byte dp_seq ;
    short rc_sp6 ;
    rec_enable_com re ;
  end rec_enable_struc ;

typedef struct /* SHELL_CMD */
  begin
    byte cmd_type ;
    byte dp_seq ;
    string79 sc ;
  end shell_cmd_struc ;

typedef struct /* LINK_ADJ */
  begin
    byte cmd_type ;
    byte dp_seq ;
    linkadj_com la ;
  end link_adj_struc ;

typedef struct /* DOWN_REQ */
  begin
    byte cmd_type ;
    byte dp_seq ;
    string63 fname ;
  end down_req_struc ;

typedef struct /* UPLOAD */
  begin
    byte cmd_type ;
    byte dp_seq ;
    boolean return_map ;
    byte upload_control ;
    union
      begin
        struct
          begin /* Send upload */
            unsigned short byte_offset ;
            unsigned short byte_count ;
            short seg_num ;
            byte bytes[DP_TO_DA_MESSAGE_LENGTH - 10] ;
         end up_send ;
       struct
         begin /* Create upload */
           unsigned short file_size ;
           string63 file_name ;
         end up_create ;
     end up_union ;
  end upload_struc ;
  
typedef struct /* FLOOD CONTROL */
  begin
    byte cmd_type ;
    byte dp_seq ;
    boolean flood_on_off ;
  end flood_struc ;

/* All the possible messages as a union */
typedef union
  begin
    byte x[DP_TO_DA_MESSAGE_LENGTH] ;
    mm_commands mmc ;
    comm_event_struc ces ;
    ultra_mass_struc ums ;
    ultra_cal_struc ucs ;
    ultra_stop_struc uss ;
    det_req_struc drs ;
    det_enable_struc des ;
    det_change_struc dcs ;
    rec_enable_struc res ;
    shell_cmd_struc scs ;
    link_adj_struc las ;
    down_req_struc downs ;
    upload_struc us ;
    flood_struc fcs ;
 end DP_to_DA_msg_type ;

/* DP to DA command header structure */
typedef struct
  begin
    short chksum ;   /* chksum not calc over error pkt */
    short crc ;      /* first gen crc, then make checksum NOT LONG ALIGNED-LONG */
    short crc_low ;
    byte cmd ;                     /* Ack, Nak, Cal, Shell indicator */
    byte ack ;                     /* Nr of last packet rcv'd by DP */
  end DP_to_DA_command_header ;

/* DP to DA buffer structure */
typedef struct
  begin  /*for sending DP -> DA cmds*/
    DP_to_DA_command_header c ;     /* minimum required for ack/nak */
    DP_to_DA_msg_type msg ;         /* CAL, SHELL, etc. command params */
  end DP_to_DA_buffer ;

#define DP_TO_DA_LENGTH_ACKNAK sizeof(DP_to_DA_command_header)
#define DP_TO_DA_LENGTH_CMD sizeof (DP_to_DA_buffer)

/* Ultra packet type (full version) */
typedef struct
  begin
    digi_record digi ;
    short vcovalue ; /* current vco value */
    boolean pllon ; /* true if PLL controlling PLL */
    boolean umass_ok ; /* coyp of cal.mass_ok */
    short comcount ; /* number of comm events */
    short comoffset ; /* offset to start of comm names */
    short usedcount ; /* number of used combos */
    short usedoffset ; /* offset to start of used combinations */
    short calcount ; /* copy of cal.number */
    short caloffset ; /* offset to start of calibrator defs */
    byte cpr_levels ; /* comlink priority levels */
    byte ultra_rev ; /* revision level, current = 1 */
    short ut_sp2 ;
    long comm_mask ; /* current comm detector mask */
  end ultra_type ;
