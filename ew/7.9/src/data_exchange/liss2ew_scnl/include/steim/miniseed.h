/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: miniseed.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:43  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:56:01  mark
 *     Initial checkin
 *
 *     Revision 1.1  2000/03/05 21:49:14  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:43  lombard
 *     Initial revision
 *
 *
 *
 */


    /*
     * File:    miniseed.h
     *          header file for SEED data record headers, including blockettes 1000/1001
     */

    /*                                      by Joseph M. Steim
                                            Quanterra, Inc.
                                            325 Ayer Road
                                            Harvard, MA 01451, USA
                                            TEL: 508-772-4774
                                            FAX: 508-772-4645
                                            e-mail: steim@geophysics.harvard.edu

            This program is free software; you can redistribute it and/or modify
            it with the sole restriction that:

               You must cause any work that you distribute or publish, that in
               whole or in part contains or is derived from the Program or any
               part thereof, to be licensed as a whole at no charge to all third
               parties.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Edit History:
            Ed  date      Changes
            --  --------  --------------------------------------------------
             3  94/02/26  split from main to create library function header file.
     */


#define MAXSEEDFRAMES           MAXTOTALFRAMESPERRECORD
#define MAXSEEDSAMPLES          ((MAXSEEDFRAMES-1)*143)


  typedef struct {
      SHORT     blockette_type ;    /*1000 always*/
      SHORT     next_offset ;       /*offset to next blockette*/
      CHAR      encoding_format ;   /*10 = Steim1, 11 = Steim2*/
      CHAR      word_order ;        /*1 always*/
      CHAR      rec_length ;        /*8=256, 9=512, 12=4096*/
      CHAR      dob_reserved ;      /*0 always*/
    } data_only_blockette ;

  typedef struct {
      SHORT     blockette_type ;    /*1001 always*/
      SHORT     next_offset ;       /*offset to next blockette*/
      CHAR      qual ;              /*-1 to 5 quality indicator*/
      CHAR      usec99 ;            /*0 - 99 microseconds*/
      CHAR      state_of_health ;   /*copy of soh*/
      CHAR      frame_count ;       /* number of 64 byte data frames */
    } data_extension_blockette ;

  typedef struct {
      SHORT     yr ;
      SHORT     jday ;
      CHAR      hr ;
      CHAR      minute ;
      CHAR      seconds ;
      CHAR      unused ;
      SHORT     tenth_millisec ;
    } SEED_time_struc ;

  /*
   * it is very important that the compiler handle non-aligned arrays correctly.
   * the size of the following structure must be 64 bytes, or there is a problem
   */

  typedef struct {
      CHAR                      sequence [6] ;                      /* record number */
      CHAR                      seed_record_type ;                  /* D for data record */
      CHAR                      continuation_record ;               /* space normally */
      CHAR                      station_ID_call_letters [5] ;       /* last char must be space */
      CHAR                      location_id [2] ;                   /* non aligned! */
      CHAR                      channel_ID [3] ;                    /* non aligned! */
      CHAR                      seednet [2] ;                       /* space filled */
      SEED_time_struc           starting_time ;
      SHORT                     samples_in_record ;
      SHORT                     sample_rate_factor ;
      SHORT                     sample_rate_multiplier ;            /* always 1 */
      CHAR                      activity_flags ;                    /* ?I?LEBTC */
      CHAR                      IO_flags ;                          /* ??L????? */
      CHAR                      data_quality_flags ;                /* ???G???? */
      CHAR                      number_of_following_blockettes ;    /* normally 1 */
      LONG                      tenth_msec_correction ;             /* always 0 */
      SHORT                     first_data_byte ;                   /* 48 or 64 - data starts in frame 1 */
      SHORT                     first_blockette_byte ;              /* 0 or 48 */
      data_only_blockette       DOB ;
      data_extension_blockette  DEB;
    } SEED_fixed_data_record_header ;


  typedef struct {
      SEED_fixed_data_record_header     Head ;
      compressed_frame                  Frames[MAXSEEDFRAMES-1] ;
    } SEED_data_record ;
