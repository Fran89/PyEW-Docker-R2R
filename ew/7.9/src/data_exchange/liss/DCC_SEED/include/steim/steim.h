/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: steim.h 2870 2007-03-28 16:54:42Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/03/28 16:54:42  paulf
 *     MACOSX change
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

/* Modified to include peek_buffer_overflowed flag. Pete Lombard, 
 * USGS contractor, 2/17/2000 */

    /*
     * File:    steim.h
     *          header file for steim compression/decompression at levels 1,2,3
     */

    /*
             Copyright (C) 1994 reserved by Joseph M. Steim
                                            Quanterra, Inc.
                                            325 Ayer Road
                                            Harvard, MA 01451, USA
                                            TEL: 508-772-4774
                                            FAX: 508-772-4645
                                            e-mail: steim@geophysics.harvard.edu

            This program is free software; you can redistribute it and/or modify
            it under the terms of the GNU General Public License as published by
            the Free Software Foundation; either version 2 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU General Public License for more details.

            If you do not have a copy of the GNU General Public License,
            write to the: Free Software Foundation, Inc.
                          675 Mass Ave, Cambridge, MA 02139, USA.

            Note particularly the following Terms excerpted from the GNU General Public License:

            You may modify your copy or copies of the Program or any portion
            of it, thus forming a work based on the Program, and copy and
            distribute such modifications or work under the terms of Section 1
            above, provided that you also meet all of these conditions:

               a) You must cause the modified files to carry prominent notices
               stating that you changed the files and the date of any change.

               b) You must cause any work that you distribute or publish, that in
               whole or in part contains or is derived from the Program or any
               part thereof, to be licensed as a whole at no charge to all third
               parties under the terms of this License.

               c) If the modified program normally reads commands interactively
               when run, you must cause it, when started running for such
               interactive use in the most ordinary way, to print or display an
               announcement including an appropriate copyright notice and a
               notice that there is no warranty (or else, saying that you provide
               a warranty) and that users may redistribute the program under
               these conditions, and telling the user how to view a copy of this
               License.  (Exception: if the Program itself is interactive but
               does not normally print such an announcement, your work based on
               the Program is not required to print an announcement.)

    Edit History:
            Ed  date      Changes
            --  --------  --------------------------------------------------
             3  94/02/26  split from main to create library function header file.
             4  94/03/12  adaptivity_control changed.
     */

#include <stdio.h>

#ifndef _MACOSX
#include <malloc.h>
#endif

#ifndef OSK
#include <string.h>
#include <stdlib.h>
#else
#include <strings.h>
#endif


#define LONG        long
#define SHORT       short
#define CHAR        char
#define WORD        unsigned short
#define FLOAT       double
#define BOOLEAN     int
#define UCHAR       unsigned char
#define REGISTER


#ifdef sun
#ifndef __STDC__
#define  const            static
#endif
#define  labs             abs
#endif

#ifdef OSK
#define  const            static
#define  labs             abs
#define  void
#undef   REGISTER
#define  REGISTER         register
#endif

#ifndef  TRUE
#define  TRUE             1
#define  FALSE            0
#endif


  /*
   * *****************************************************************
   * the following group of definitions is needed for compression.   *
   * the conditional "STATISTICS", and code it controls may be       *
   * omitted from "production" compression code, in which histograms *
   * of what differences fit in how many bits are not needed.        *
   * *****************************************************************
   */

#define STATISTICS  1



  /*
   * to simplify buffer wrap handling, PEEKELEMS should be set to at least the largest
   * number of samples (N) to be written to the buffer + 143, the maximum number of samples
   * in a level 3 frame. this is really only a convenience for the peek buffer management
   * routines in this module. as long as the user does not overflow the peek buffer, and keeps
   * it sufficiently full that no samples are "padded" by compress_frame, the peek buffer
   * can be filled any way you like.
   */

#define    PEEKELEMS           150 + 2016    /* Maximum number of short  */
                                             /*   samples in earthworm   */
                                             /*   TRACE_BUF message      */
#define    WORDS_PER_FRAME     16
#define    MAXSAMPPERWORD      9
#define    ICONSTCODE          0xE0000000
#define    ICONSTMASK          (~ICONSTCODE)
#define    ICONSTSIGN          (((LONG)ICONSTMASK+1) >> 1)
#define    NOTDIFFERENCE       0xC8000000
#define    SECONDDIFF          0x80000000
#define    REPLACEMENTMASK     0x70000000


  typedef union {
      CHAR                  b[4] ;
      SHORT                 s[2] ;
      LONG                  l ;
    } COMPWORD ;


  typedef LONG              peekbuffertype[PEEKELEMS];

  typedef COMPWORD          compressed_frame[WORDS_PER_FRAME];

  typedef compressed_frame  *cfp;



    typedef struct {
        SHORT                 next_in ;
        SHORT                 peek_total, next_out ;
        LONG                  frames ;
        LONG                  last_1, last_2, final ;
        peekbuffertype        *peeks ;
#ifdef STATISTICS
        LONG                  squeezed_flags ;
        LONG                  fits[33] ;
#endif
        compressed_frame      framebuf ;
        SHORT                 peek_threshold ;
        SHORT                 peek_buffer_overflow ; /* Added: PNL 2/17/2000 */
      } compression_continuity ;


    typedef compression_continuity *ccptype ;


    typedef struct {
        ccptype               ccp ;
        SHORT                 bestdiff ;
        SHORT                 difference ;
        SHORT                 trials ;
        SHORT                 firstframe ;
        SHORT                 framespertrial ;
        SHORT                 framesperpackage ;
        SHORT                 level ;
        SHORT                 flip ;
      } adaptivity_control ;

    typedef adaptivity_control *adptype ;

  /*
   * ****************************************************************
   * the following group of definitions is needed for decompression *
   * ****************************************************************
   */

#define     UNPACKSIZE          150
#define     UNPACKFINAL         (UNPACKSIZE-2)

  /*
   * the following status codes are returned by "decompress_frame".
   * EDF_OK implies fully correct decompression, including check of
   *        integration constants.
   * EDF_INTEGRESYNC implies a non-fatal advisory, such as successful resynchronization
   *        at the integration constant check following a descrepancy,
   *        such as when one or more records are missing.
   * others imply an internally corrupt frame. this may arise also from
   *        decompression at the wrong level.
   * the status code masked with EDF_FATAL returned by decompress frame may be checked
   *        to test for failure to decompress.
   * the status code masked with EDF_NONFATAL returned by decompress frame may be checked
   *        to test for advisory status, such as integration constant resync.
   */
#define     EDF_OK              0x00
#define     EDF_INTEGRESYNC     0x01
#define     EDF_INTEGFAIL       0x02
#define     EDF_SECSUBCODE      0x04
#define     EDF_TWOBLOCK        0x08
#define     EDF_OVERRUN         0x10
#define     EDF_REPLACEMENT     0x20
#define     EDF_LASTERROR       0x40
#define     EDF_COUNTERROR      0x80
#define     EDF_FATAL           (EDF_INTEGFAIL|EDF_SECSUBCODE|EDF_TWOBLOCK|EDF_OVERRUN|EDF_REPLACEMENT)
#define     EDF_NONFATAL        (EDF_INTEGRESYNC|EDF_LASTERROR|EDF_COUNTERROR)


  typedef  LONG         unpacktype[UNPACKSIZE] ;

  typedef  SHORT        *sptype ;

  typedef struct {
      LONG      samp_1 ;
      LONG      samp_2 ;
      SHORT     numdecomp ;
    } decompression_continuity ;

  typedef   decompression_continuity *dcptype ;


  /*
   * the following structures allow build complete "records", which comprise a GENERICHDRL-byte
   * header, followed by up to (MAXTOTALFRAMESPERRECORD-1) 64-byte compressed "frames".
   * a pointer to such a "generic_data_record" may be cast to point to SEED data records,
   * or NSN data records, or whatever, while allowing all the compression library functions
   * not to care what kind of header will be installed. you should set GENERICHDRL
   * to the size of headers on the types of actual data records you're using, bearing in mind that
   * "frames" must begin at least on a "long" boundary.
   */

#define MAXTOTALFRAMESPERRECORD         64
#ifndef GENERICHDRL
#define GENERICHDRL                     64
#endif

  typedef struct {
      CHAR                              head[GENERICHDRL] ;
      compressed_frame                  frames[MAXTOTALFRAMESPERRECORD-1] ;
    } generic_data_record ;


  typedef struct {
      adptype                           adp ;
      generic_data_record               *gdr ;
      LONG                              nsamples ;
    } generic_data_record_control ;

  typedef generic_data_record_control   *gdptype ;


    /*  -------------------- END OF STEIM.H ------------------------ */
