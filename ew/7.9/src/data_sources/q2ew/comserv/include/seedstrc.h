/* DP Seed data record definitions
   Copyright 1994 Quanterra, Inc.
   Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 19 Mar 94 WHO Split off from dpstruc.h
    1 30 May 94 WHO Missing semicolons identified by DSN added.
    2 12 Dec 94 WHO Updated to SEED version 2.3B.
    3  2 Dec 96 WHO Add blk_min and block_record.
    4 17 Oct 97 WHO Add VER_SEEDSTRC.
    5 26 Nov 97 WHO Change deb_reserved to deb_flags, add DEB_NEWTIME.
*/

/* Make sure dpstruc.h is included, this forces including pascal.h */
#ifndef cs_dpstruc_h
#include "dpstruc.h"
#endif

/* Flag this file as included */
#define cs_seedstrc_h
#define VER_SEEDSTRC 5
 
#define SEED_ACTIVITY_FLAG_CAL_IN_PROGRESS     0x1
#define SEED_ACTIVITY_FLAG_APPARENT_TIME_GAP   0x2
#define SEED_ACTIVITY_FLAG_BEGIN_EVENT         0x4
#define SEED_ACTIVITY_FLAG_END_EVENT           0x8
#define SEED_ACTIVITY_FLAG_LEAP_FORW           0x10
#define SEED_ACTIVITY_FLAG_LEAP_REV            0x20
#define SEED_ACTIVITY_FLAG_EVENT_IN_PROGRESS   0x40
#define SEED_QUALITY_FLAG_MISSING_DATA         0x10
#define SEED_QUALITY_FLAG_QUESTIONABLE_TIMETAG 0x80
#define SEED_IO_CLOCK_LOCKED                   0x20

#define DEB_NEWTIME 1 /* Mshear time stamping */

/* Data only blockette structure for data records */
typedef struct
  begin
    short blockette_type ; /* 1000 always */
    short next_offset ;    /* offset to next blockette */
    byte encoding_format ; /* 10 = Steim1, 11 = Steim2, 19 = Steim3, 0 = none */
    byte word_order ;      /* 1 always */
    byte rec_length ;      /* 9=512 always */
    byte dob_reserved ;    /* 0 always */
  end data_only_blockette ;

/* data extension blockette structure for data records */
typedef struct
  begin
    short blockette_type ; /* 1001 always */
    short next_offset ;    /* offset to next blockette */
    byte qual ;            /* 0 to 100% quality indicator */
    byte usec99 ;          /* 0 to 99 microseconds */
    byte deb_flags ;       /* DEB_XXXXX */
    byte frame_count ;     /* number of 64 byte data frames */
  end data_extension_blockette ;

/* time structure used in seed records */
typedef struct
  begin
    short yr ;             /* year */
    short jday ;           /* julian day */
    byte hr ;              /* hour */
    byte minute ;          /* minutes */
    byte seconds ;         /* seconds */
    byte unused ;          /* zero */
    short tenth_millisec ; /* tenth of a millisecond */
  end seed_time_struc ;

/* this 48 byte structure is used for data records and blockettes */
typedef struct
  begin
    char sequence[6] ;                    /* record number */
    char seed_record_type ;
    char continuation_record ;
    char station_ID_call_letters[5] ;
    location_type location_id ;           /* non aligned! */
    seed_name_type channel_id ;           /* non aligned! */
    seed_net_type seednet ;               /* seed netword ID */
    seed_time_struc  starting_time ;
    short samples_in_record ;
    short sample_rate_factor ;
    short sample_rate_multiplier ;        /* always 1 */
    byte activity_flags ;                 /* ?I?LEBTC */
    byte IO_flags ;                       /* ??L????? */
    byte data_quality_flags ;             /* Q??M???? */
    byte number_of_following_blockettes ; /* normally 0 */
    long tenth_msec_correction ;          /* temporarily 0 */
    short first_data_byte ;               /* 0, 48, or 56 or multiple of 64 */
    short first_blockette_byte ;          /* 0 or 48 */
  end seed_record_header ;

/* Data records also include the data_only_blockette and data_extension_blockette */
typedef struct
  begin
    seed_record_header header ;
    data_only_blockette dob ;
    data_extension_blockette deb ;
  end seed_fixed_data_record_header ;

/* 512 byte data record consists of */
typedef struct
  begin
    seed_fixed_data_record_header head ;
    compressed_frame_array frames ;
  end seed_data_record ;

/* Blockettes have many possible blockettes, but only two blockettes will be contained
   in a blockette record returned from comserv. The first blockette will
   always be a data only blockette (1000) followed by a detection, calibration,
   or timing blockette. Message records contain one blockette (1000) followed by
   strings starting at offset 56.
*/

/* Murdock-Hutt event detections use this blockette */
typedef struct
  begin
    short blockette_type ;              /* 201 for MH */
    short next_blockette ;              /* record offset of next */
    float signal_amplitude ;
    float signal_period ;               /* not used for threshold */
    float background_estimate ;         /* limit crossed for threshold */
    byte event_detection_flags ;        /* 0/1 for MH */
    byte reserved_byte ;                /* 0 */
    seed_time_struc signal_onset_time ;
    byte snr[6] ;                       /* only first 5 used */
    byte lookback_value ;               /* 0, 1, or 2 */
    byte pick_algorithm ;               /* 0 or 1 */
    /* Extensions */
    char s_detname[24] ;                /* space filled on right */
  end murdock_detect ;

/* Threshold detections */
typedef struct
  begin
    short blockette_type ;              /* 200 for Threshold */
    short next_blockette ;              /* record offset of next */
    float signal_amplitude ;
    float signal_period ;               /* not used for threshold */
    float background_estimate ;         /* limit crossed for threshold */
    byte event_detection_flags ;        /* 4 for threshold */
    byte reserved_byte ;                /* 0 */
    seed_time_struc signal_onset_time ;
    /* Extensions */
    char s_detname[24] ;                /* space filled on right */
    end threshold_detect ;

/* Clock timing */
typedef struct
  begin
    short blockette_type ;              /* 500 */
    short next_blockette ;              /* record offset of next */
    float vco_correction ;              /* 0 to 100.0% of full range */
    seed_time_struc time_of_exception ;
    byte usec99 ;                       /* 0-99 usecs extension to above */
    byte reception_quality ;            /* 0 to 100% clock quality */
    long exception_count ;              /* seconds/consecutive */
    char exception_type[16] ;           /* Type of exception */
    char clock_model[32] ;              /* Manufacture & Type of clock */
    char clock_status[128] ;            /* Operating status */
  end timing ;

/* Calibration blockettes are made up of pieces */
typedef struct
  begin
    short blockette_type ;             /* 300, 310, 320, or 395 */
    short next_blockette ;             /* record offset of next */
    seed_time_struc calibration_time ; /* start or stop */
  end cal1 ;
    
typedef struct
  begin
    float calibration_amplitude ;              /* volts or amps, depending on LCQ */
    seed_name_type calibration_input_channel ; /* monitor channel, if any */
    byte cal2_res ;                            /* zero */
    float ref_amp ;                            /* Reference amplitude */
    char coupling[12] ;                        /* Method of connection to sensor */
    char rolloff[12] ;                         /* Filtering of signal */
  end cal2 ;

typedef struct
  begin
    cal1 fixed ;
    byte number_of_steps ;      /* 1 */
    byte calibration_flags ;    /* bit 0 = +, bit 2 = automatic */
    long calibration_duration ; /* 0.0001 seconds / count */
    long interval_duration ;    /* 0 */
    cal2 step2 ;
  end step_calibration ;
    
typedef struct
  begin
    cal1 fixed ;
    byte res ;                  /* 0 */
    byte calibration_flags ;    /* bit 2 = automatic, bit 4 set */
    long calibration_duration ; /* 0.0001 seconds / count */
    float sine_period ;         /* in seconds */
    cal2 sine2 ;
  end sine_calibration ;
    
typedef struct
  begin
    cal1 fixed ;
    byte res ;                  /* 0 */
    byte calibration_flags ;    /* bit 2 = automatic, bit 4 = ? */
    long calibration_duration ; /* 0.0001 seconds / count*/
    cal2 random2 ;
    char noise_type[8] ;        /* frequency characteristics of noise */
  end random_calibration ;
    
typedef struct
  begin
    cal1 fixed ;
    short res ;                 /* 0 */
  end abort_calibration ;

typedef struct
  begin
    short blockette_type ;      /* any number */
    short next_blockette ;      /* record offset of next */
    short blockette_lth ;       /* byte count of blockette */
    short data_offset ;         /* offset from start of blockette to start of data */
    long record_num ;           /* record number */
    byte word_order ;           /* 1 = correct, 0 = wrong */
    byte data_flags ;           /* fragmentation not supported */
      /* number of header fields is one byte, but that would make odd
         length structure, not too convenient */
  end opaque_hdr ;              /* Minimum contents of opaque blockette */

typedef struct
  begin
    seed_record_header hdr ;    /* Record header */
    opaque_hdr bmin ;           /* minimum blockette */
    byte blk_data[450] ;        /* Rest of blockette, or more blockettes */
  end block_record ;            /* Basic format of a blockette record */
