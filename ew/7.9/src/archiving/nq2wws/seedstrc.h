/* DP Seed data record definitions
   Copyright 1994 Quanterra, Inc.
   Written by Woodrow H. Owens

This is a subset of the original seedstrc.h 
*/


/* Flag this file as included */
#define cs_seedstrc_h
 

#define byte unsigned char
  
/* Boolean constants */
#define FALSE 0
#define TRUE 1 /* Must be 1 for compatibility with Pascal */

/* Compressed record sizes */
#define TOTAL_FRAMES_PER_RECORD 8    /* Maximum, may be shorter */
#define WORDS_PER_FRAME 16           /* there are 4 bytes * 16 = 64 bytes per frame */

typedef union
  {
    unsigned char b[4] ;
    signed char sb[4] ;
    short s[2] ;
    long l ;
    float f ;
  } complong ;

/* Definitions for a compressed frame */
typedef complong compressed_frame[WORDS_PER_FRAME] ;

/* Compressed frame array cannot be translated into C correctly, since the starting
   index should be 1 (0 is the header frame) */
typedef compressed_frame compressed_frame_array[TOTAL_FRAMES_PER_RECORD-1] ;



/* Data only blockette structure for data records */
typedef struct
  {
    short blockette_type ; /* 1000 always */
    short next_offset ;    /* offset to next blockette */
    byte encoding_format ; /* 10 = Steim1, 11 = Steim2, 19 = Steim3, 0 = none */
    byte word_order ;      /* 1 always */
    byte rec_length ;      /* 9=512 always */
    byte dob_reserved ;    /* 0 always */
  } data_only_blockette ;

/* data extension blockette structure for data records */
typedef struct
  {
    short blockette_type ; /* 1001 always */
    short next_offset ;    /* offset to next blockette */
    byte qual ;            /* 0 to 100% quality indicator */
    byte usec99 ;          /* 0 to 99 microseconds */
    byte deb_flags ;       /* DEB_XXXXX */
    byte frame_count ;     /* number of 64 byte data frames */
  } data_extension_blockette ;

/* time structure used in seed records */
typedef struct
  {
    short yr ;             /* year */
    short jday ;           /* julian day */
    byte hr ;              /* hour */
    byte minute ;          /* minutes */
    byte seconds ;         /* seconds */
    byte unused ;          /* zero */
    short tenth_millisec ; /* tenth of a millisecond */
  } seed_time_struc ;

/* Common fields within SEED headers and other structures */
typedef char seed_name_type[3] ;
typedef char seed_net_type[2] ;
typedef char location_type[2] ;

/* this 48 byte structure is used for data records and blockettes */
typedef struct
  {
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
  } seed_record_header ;

/* Data records also include the data_only_blockette and data_extension_blockette */
typedef struct
  {
    seed_record_header header ;
    data_only_blockette dob ;
    data_extension_blockette deb ;
  } seed_fixed_data_record_header ;

/* 512 byte data record consists of */
typedef struct
  {
    seed_fixed_data_record_header head ;
    compressed_frame_array frames ;
  } seed_data_record ;

/* Blockettes have many possible blockettes, but only two blockettes will be contained
   in a blockette record returned from comserv. The first blockette will
   always be a data only blockette (1000) followed by a detection, calibration,
   or timing blockette. Message records contain one blockette (1000) followed by
   strings starting at offset 56.
*/


typedef struct
  {
    short blockette_type ;      /* any number */
    short next_blockette ;      /* record offset of next */
    short blockette_lth ;       /* byte count of blockette */
    short data_offset ;         /* offset from start of blockette to start of data */
    long record_num ;           /* record number */
    byte word_order ;           /* 1 = correct, 0 = wrong */
    byte data_flags ;           /* fragmentation not supported */
      /* number of header fields is one byte, but that would make odd
         length structure, not too convenient */
  } opaque_hdr ;              /* Minimum contents of opaque blockette */

typedef struct
  {
    seed_record_header hdr ;    /* Record header */
    opaque_hdr bmin ;           /* minimum blockette */
    byte blk_data[450] ;        /* Rest of blockette, or more blockettes */
  } block_record ;            /* Basic format of a blockette record */
