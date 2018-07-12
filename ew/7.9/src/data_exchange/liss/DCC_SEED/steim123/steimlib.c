/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: steimlib.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:49:34  lombard
 *     Initial revision
 *
 *
 *
 */

/*  File:
  steimlib.c

  library of compression and decompression functions for steim level 1,2,3 compression.

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
   3  94/02/28  first public release. JPL, ASL
   4  94/03/04  converted C version to library.
                will gracefully exit "compress_frame" if
                  all "blocks" have been reserved, as in filling
                  an empty frame. this was actually in ed 3.
                "compress_frame" will now work with < 2 samples.
                some header includes re-arranged. (creeping C-junkie at work).
                some flags defined as TRUE/FALSE for better readability.
                "init_compression" and "init_adaptivity" now allocate
                  and return pointers to their appropriate structures.
                general source clean-up to ANSI compile with no warnings.
                "decompress_frame" will now return a defined numeric
                  status code that may be passed to "dferrorfatal",
                  a procedure that will print an appropriate message and
                  return TRUE if the frame could not be decompressed.
                "peek_write" will now return the number of samples
                  written, which is 1, if successful.
   5  94/03/12  init_adaptivity, compress_adaptively, and
                  clear_generic_compression changed to fix lack of
                  constant insertion in records with frames reserved
                  for blockettes.
                  bug fixed when peek buffer underflows and differences do
                            not fit in the first scan length tried.
 */


  /*
   * *********************************************************************************
   * this has so far been successfully compiled on Microsoft Quick-C, Sun 4.1.1B C,  *
   * Sun ANSI C 2.0.1 (SPARCCompiler-C) under 4.1.3_U1, and OS9/68000 C v3.2         *
   * *********************************************************************************
   */

  /*
   *    recommended cc lines:
   *        sun 4.1.1B          cc -O2 -dalign steimlib.c
   * sun ANSI C 4.1.3U          acc -O4 -Xc -dalign steimlib.c
   *        os9 (68020)         cc -DOSK -k=2w -O=2 -M=10k -t=/r0 steimlib.c
   *        os9 (68000)         cc -DOSK -k=0w -O=2 -M=10k -t=/r0 steimlib.c
   */

#include <steim/steim.h>


typedef struct
{
  SHORT            samps ;
  CHAR             postshift ;
  LONG             mask ;
  LONG             hibit ;
  LONG             neg ;
} decompbittype ;



#ifdef __STDC__
void endianflip_frame (cfp framep)
#else
     void endianflip_frame (framep)
     cfp framep;
#endif
     /*
     * procedure for in-place byte-endian reversal of a 64-byte compressed frame.
     */
{
  SHORT   i ;
  COMPWORD   c ;
  COMPWORD  *p ;

  for ( i = 0; i < WORDS_PER_FRAME; i++ )
  {
    p = &((*framep)[i]) ;
    c = *p ;
    p->b[0] = c.b[3] ;
    p->b[1] = c.b[2] ;
    p->b[2] = c.b[1] ;
    p->b[3] = c.b[0] ;
  }
}



#ifdef __STDC__
LONG endianflip (LONG e)
#else
     LONG endianflip (e)
     LONG e;
#endif
     /*
     * function for byte-endian reversal of a 32-bit word.
     */
{
  COMPWORD c, p ;

  p.l = e ;
  c.b[0] = p.b[3] ;
  c.b[1] = p.b[2] ;
  c.b[2] = p.b[1] ;
  c.b[3] = p.b[0] ;
  return (c.l) ;
}



#ifdef __STDC__
SHORT swapb (SHORT e)
#else
     SHORT swapb (e)
     SHORT e;
#endif
     /*
     * function for byte-endian reversal of a 16-bit word.
     */
{
  COMPWORD c, p ;

  p.s[0] = e ;
  c.b[0] = p.b[1] ;
  c.b[1] = p.b[0] ;
  return (c.s[0]) ;
}



#ifdef __STDC__
SHORT compress_frame (ccptype ccp,  SHORT difference,  SHORT level,  SHORT reserve,  SHORT flip)
#else
     SHORT compress_frame ( ccp,  difference,  level,  reserve,  flip)
     ccptype ccp;
     SHORT difference;
     SHORT level;
     SHORT reserve;
     SHORT flip;
#endif
{
  /*
   * passed:
   *        ccp            :  pointer to the compression continuity structure for this channel.
   *        difference     :  the desired differencing operation, 1 or 2
   *        level          :  the desired compression coding level, 1, 2, or 3
   *        reserve        :  the number of non-data 32-bit "blocks" to reserve at the beginning of
   *                            the frame, used for embedded storage of integration constants.
   *                            typically, "reserve" will be 2 for level 1/2 and 3 for level 3,
   *                            which will install the first (level 1/2, difference=1) or the
   *                            first and second (level 3, difference 1 or 2) samples in the record
   *                            and reserve space for later installation of the last sample of a group
   *                            of frames, such as in a 4096-byte (63 frame+header) data record.
   *                            reserve=15 will generate a totally empty frame, in the appropriate level.
   *        flip           :  0 will generate a frame in the native endian-ness of the computer,
   *                          1 will reverse the endian-ness.
   *        *(ccp->peeks)  :  the compression look-ahead (peek) buffer containing data to be compressed.
   *                            the data will begin at ccp->peeks^[ccp->next_out], and continue for
   *                            ccp->peek_total samples. it is advisable to guarantee that the peek
   *                            buffer always contain at least 143 samples (maximum in level 3) to
   *                            avoid generating compressed frames padded with non-data, which will
   *                            greatly reduce compression efficiency.
   * returns:
   *        ccp->framebuf  :  will contain the 64-byte compressed frame in the selected endian-ness.
   *        *ccp           :  the compression continuity strucure will be updated, particularly,
   *                            ccp->peek_total will be decremented by the amount of samples removed
   *                            from the peek buffer, and will be negative by the number of 32-bit
   *                            non-data blocks padded at the end of the frame if the peek buffer did
   *                            not contain a sufficient number of samples to fill the frame. if this
   *                            underflow occurs, and the caller has more data to write, the
   *                            clear_compression should be called to reset buffer pointers before
   *                            new data is placed in the peek buffer.
   *
   *        function value :  will return the number of samples from the peek buffer actually compressed
   *                            if successful. if differences are too large to compress in the selected
   *                            mode, a frame containing as many samples as possible that fit will be
   *                            built, and the number that fit (>= 0) will be returned.
   *                            if in subsequent calls data are still too large, will return -1,
   *                            at which point the peek-buffer next-out will start at the
   *                            group of samples that will not compress. the caller can clip the data,
   *                            and try again.
   */

#define      B7X4       (2 << 2)
#define      B9X3       (3 << 3)
#define      B6X5       1
#define      B5X6       0
#define      B5X12      0
#define      B3X20      1
#define      B3X10      3
#define      B2X15      2
#define      B1X30      1
#define      HUGE       2147483647
#define      LARGE      1073741823
#define      BIG         536870911
#define      GRAND        16777215
#define      FAT           8388607
#define      F8X3       0x01000000
#define      F6X4       0x02000000
#define      F4X6       0x03000000
#define      F3X8       0x04000000
#define      F2X12      0x05000000
#define      F1X24      0x06000000


  enum code_type  { c9x3, c7x4, c6x5, c5x6, c4x8, c3x10, c5x12, c2x15, c2x16, c1x30, c1x32,
                    c8x3, c6x4, c4x6, c3x8, c2x12, c1x24, c3x20 } ;

  typedef struct
  {
    SHORT            next ;
    SHORT            scan ;
    enum code_type   cc ;
    LONG             bc ;
    LONG             cbits ;
    CHAR             shift ;
    LONG             mask ;
    LONG             disc ;
    SHORT            fit ;
  } compseqtype ;


  const compseqtype  compseq [27] =
  {  { 1,      4,     c4x8,    1,            0,        8,          255,         127,      8},
     { 2,      2,    c2x16,    2,            0,       16,        65535,       32767,     16},
     { -1,     1,    c1x32,    3,            0,       32,           -1,        HUGE,     32},
     { 4,      7,     c7x4,    3,         B7X4,        4,           15,           7,      4},
     { 5,      6,     c6x5,    3,         B6X5,        5,           31,          15,      5},
     { 6,      5,     c5x6,    3,         B5X6,        6,           63,          31,      6},
     { 7,      4,     c4x8,    1,            0,        8,          255,         127,      8},
     { 8,      3,    c3x10,    2,        B3X10,       10,         1023,         511,     10},
     { 9,      2,    c2x15,    2,        B2X15,       15,        32767,       16383,     15},
     { -1,     1,    c1x30,    2,        B1X30,       30,        LARGE,         BIG,     30},
     { 11,     9,     c9x3,    3,         B9X3,        3,            7,           3,      3},
     { 12,     7,     c7x4,    3,         B7X4,        4,           15,           7,      4},
     { 13,     6,     c6x5,    3,         B6X5,        5,           31,          15,      5},
     { 14,     5,     c5x6,    3,         B5X6,        6,           63,          31,      6},
     { 15,     4,     c4x8,    1,            0,        8,          255,         127,      8},
     { 16,     3,    c3x10,    2,        B3X10,       10,         1023,         511,     10},
     { 17,     5,    c5x12,    2,        B5X12,       12,         4095,        2047,     12},
     { 18,     2,    c2x15,    2,        B2X15,       15,        32767,       16383,     15},
     { 19,     2,    c2x16,    0,            0,       16,        65535,       32767,     16},
     { 20,     3,    c3x20,    2,        B3X20,       20,      1048575,      524287,     20},
     { -1,     1,    c1x30,    2,        B1X30,       30,        LARGE,         BIG,     30},
     { 22,     8,     c8x3,    0,         F8X3,        3,            7,           3,      3},
     { 23,     6,     c6x4,    0,         F6X4,        4,           15,           7,      4},
     { 24,     4,     c4x6,    0,         F4X6,        6,           63,          31,      6},
     { 25,     3,     c3x8,    0,         F3X8,        8,          255,         127,      8},
     { 26,     2,    c2x12,    0,        F2X12,       12,         4095,        2047,     12},
     { -1,     1,    c1x24,    0,        F1X24,       24,        GRAND,         FAT,     24 } };
  
  typedef struct
  {
    LONG              fword ;
    LONG              repcode ;
  } flagreptype ;

  const flagreptype   commonflags [3] =
  { {0x15555555, 0x50000000},
    { 0x2AAAAAAA, 0x60000000},
    { 0x3FFFFFFF, 0x70000000 } };

  REGISTER SHORT    block ;
  LONG              flag_word ;
  LONG              block_code ;
  REGISTER SHORT    i ;
  LONG              diffs [MAXSAMPPERWORD] ;
  LONG              s [MAXSAMPPERWORD+2] ;
  SHORT             total_in_peek_buffer_at_entry ;
  LONG              flag_replacement ;
  SHORT             ctabx ;
  BOOLEAN           done ;
  SHORT             clipped_total ;
  SHORT             pop, empty ;
  LONG              diffbit ;
  LONG              samp_1, samp_2 ;
  REGISTER SHORT    hiscan ;
  SHORT             ctabhi, ctablo, ctabw, ctabfit ;
  SHORT             number_of_samples ;
  compseqtype       *sp ;
  SHORT             t_scan  ;
  REGISTER LONG     accum ;
  REGISTER LONG     t_mask  ;
  LONG              t_disc  ;

  flag_word = 0 ;
  total_in_peek_buffer_at_entry = ccp->peek_total ;
  number_of_samples = 0 ;
  /*
   * it not assumed that there are any valid samples are in the peek buffer at entry.
   * if there are not sufficient data, these values may be meaningless. the appropriate
   * "not data" code will be installed in their place if that is so.
       */
  samp_1 = (*(ccp->peeks)) [ccp->next_out] ;
  samp_2 = (*(ccp->peeks)) [(ccp->next_out+1) % PEEKELEMS] ;
  /*
   * for each 32-bit "block", determine how many differences will fit, and fit them.
   * start placing compressed differences within the frame at the "block" indicated by the
   * "reserve" parameter. at some user-selected interval, the frame will contain the
   * integration constants and final sample to be used in reconstruction.
   * level 1 and 2 will usually reserve 2 blocks, for the first and last sample, while
   * level 3 may reserve 3 blocks to include the first, second, and optionally last sample.
   * don't, however, reserve more blocks than in the record!
   */
  if (reserve >= WORDS_PER_FRAME)
    reserve = (WORDS_PER_FRAME-1) ;
  block = 1 ;
  while (block < (reserve+1))
  {
    if (level < 3)
    {
      /*
       * in levels 1 and 2, no data is always identified by block code 0. the position for
       * the last sample is reserved, and temporarily filled with 0.
       */
      block_code = 0 ;
      switch (block)
      {
      case 1 :
        if (ccp->peek_total >= 1)
          ccp->framebuf [ block ].l = samp_1 ;
        else
          ccp->framebuf [ block ].l = 0 ;
        break;
      default:
        ccp->framebuf [ block ].l = 0;
        break;
      }
    }
    else
    {
      /*
       * in level 3, an integration constant is identified with a unique code in the
       * 3 block lead-in bits, allowing up to 29 bits for constant storage. if block 3
       * is reserved, it is tagged temporarily as not data, on the assumption that
       * the space will be used to store the last sample in a group of frames,
       * and given the appropriate code at that time.
       */
      block_code = 3 ;
      switch (block)
      {
      case 1 :
        if (ccp->peek_total >= 1)
          ccp->framebuf [ block ].l = (samp_1 & ICONSTMASK) | ICONSTCODE ;
        else
          ccp->framebuf [ block ].l = NOTDIFFERENCE ;
        break;
      case 2 :
        if (ccp->peek_total >= 2)
          ccp->framebuf [ block ].l = (samp_2 & ICONSTMASK) | ICONSTCODE ;
        else
          ccp->framebuf [ block ].l = NOTDIFFERENCE ;
        break;
      default:
        ccp->framebuf [ block ].l = NOTDIFFERENCE ;
        break;
      }
    }
    flag_word = (flag_word << 2) + block_code ;
    block = block + 1 ;
  }
  /*
       * select the region of the table to begin scanning, and the limits
       * beyond which scanning must stop. assume in level 1, that for the
       * first block, 4 differences will fit, and in levels 2,3, 5 differences.
       * the roaming table pointer will have a good estimate after the first
       * block of how many differences to try.
       */
  switch (level)
  {
  case 3 :
    ctablo = 10 ;
    ctabhi = 20 ;
    ctabx = 13 ;
    break ;
  case 2 :
    ctablo = 3 ;
    ctabhi = 9 ;
    ctabx = 5 ;
    break ;
  case 1 :
    ctablo = 0 ;
    ctabhi = 2 ;
    ctabx = 0 ;
    break ;
  }
  /*
       * for each block, scan the appropriate number of differences based
       * on the roaming table pointer, and update the roaming table pointer.
       */
  while (block < WORDS_PER_FRAME)
  {
    s[0] = ccp->last_2 ;
    s[1] = ccp->last_1 ;
    /*
     * when scanning ahead for a new sample group, if there is known to be no samples in
     * the peek buffer, then the buffer is already empty. empty will contain the "diffs"
     * index at and beyond which no actual data are represented. therefore, if there are
     * no data when starting the scan, "diffs" contain no data starting at the beginning.
     * if there are data in the peek buffer, "empty" is initialized to -1, which is not
     * a valid "diffs" index.
     */
    if (ccp->peek_total <= 0)
      empty = 0;
    else
      empty = -1;
    /*
     * now start the scan loop for this block. "hiscan" is the number of differences in
     * the local difference buffer, which is increased as required.
     * "ctabfit" is > 0 if a table entry has been found in which all differences fit;
     * when starting the scan for this block, we don't know what fits.
     * "ctabw" is the table pointer at entry, which if it necessary to scan beyond, means
     * that once a fit is found, there is not point in looking backwards to see if any
     * more will fit.
     */
    hiscan = 0 ;
    done = FALSE ;
    ctabfit = -1 ;
    ctabw = ctabx ;
    do {
      sp = (compseqtype *)&compseq[ctabx] ;
      t_scan = sp->scan ;
      t_disc = sp->disc ;
      /*
       * now scan ahead for at least the number of differences that can be stored in
       * a 32-bit block based on the current table pointer.
       * this code is somewhat redundant for performance's sake, because the
       * inner loop is actually executed typically several times per sample.
       * the required processing could all be accomodated by the "else" clause
       * of "if peek_total >= scan".
       * if there is known to be enough data in the "peek" buffer to fill the scan,
       * don't bother to check whether the buffer has hit empty.
       */
      if (ccp->peek_total >= t_scan)
        if (difference == 1)
          while (hiscan < t_scan)
          {
            s[hiscan+2] = (*(ccp->peeks))[(ccp->next_out + hiscan) % PEEKELEMS] ;
            diffs [hiscan] = s[hiscan+2] - s[hiscan+1] ;
            hiscan = hiscan + 1 ;
          }
        else
          while (hiscan < t_scan)
          {
            s[hiscan+2] = (*(ccp->peeks))[(ccp->next_out + hiscan) % PEEKELEMS] ;
            diffs [hiscan] = s[hiscan+2] - (s[hiscan+1] << 1) + s[hiscan] ;
            hiscan = hiscan + 1 ;
          }
      else
        while (hiscan < t_scan)
        {
          pop = (ccp->next_out + hiscan) % PEEKELEMS ;
          /*
           * if the end of the peek buffer is reached, all "diffs" at and beyond this
           * index are not really data.
           */
          if ((pop == ccp->next_in) && (ccp->peek_total < PEEKELEMS) && (empty < 0))
            empty = hiscan ;
          /*
           * if the next_out index passed the next_in index, the buffer has run
           * out of data, and it will be necessary to force selection of a "block_code"
           * that will allow representing the real data without trailing garbage.
           * as non-data are pulled from the peek buffer, peek_total will be negative
           * by the amount required to pad the frame.
           */
          if (empty < 0)
          {
            s[hiscan+2] = (*(ccp->peeks))[pop] ;
            if (difference == 1)
              diffs [hiscan] = s[hiscan+2] - s[hiscan+1] ;
            else
              diffs [hiscan] = s[hiscan+2] - (s[hiscan+1] << 1) + s[hiscan] ;
          }
          else
          {
            s [hiscan+2] = 0 ;
            diffs [hiscan] = 0 ;
          }
          hiscan = hiscan + 1 ;
        }
      if (empty < 0)
        /*
         * in this case, the difference buffer contains all actual data. scan each
         * difference to see if it fits in the size indicated by the current table
         * pointer.
         */
        for (i = 0; i < t_scan; i++)
          if (labs(diffs[i]) > t_disc)
          {
            /*
             * at least one difference in the scan does not fit. if there has been
             * no table entry found yet for which the data will fit, go to the
             * next larger storage size. otherwise, take the one that fits, and
             * quit scanning. this may be the case if the table is probed toward
             * the beginning, looking for more efficient fits.
             */
            if (ctabfit < 0)
              ctabx = sp->next ;
            else
            {
              ctabx = ctabfit ;
              done = TRUE ;
            }
            break ;
          }
          else
            if (i == (t_scan - 1))
            {
              /*
               * here, all differences within the scan fit. if the table is scanning
               * toward larger storage sizes, then smaller ones cannot possibly fit,
               * so the scan is done.
               */
              if (ctabx > ctabw)
                done = TRUE ;
              else
                                /*
                                 * here, note that at least the current table entry is a fit,
                                 * but if the start of the table has not been reached, try scanning
                                 * toward smaller storage sizes. if the smaller storage size won't
                                 * work, the size selected by "ctabfit" will be used (see above).
                                 */
                if (ctabx > ctablo)
                {
                  ctabfit = ctabx ;
                  ctabx = ctabx - 1 ;
                }
                else
                  done = TRUE ;
              break ;
            }
            else ;  /* note the null statement */
      else
        /*
         * here there are some non-differences within the scan length.
         */
        if (empty > 0)
        {
          /*
           * reduce the scan length until at least the current block is completely
           * occupied by actual data. set the last forward-direction index ctabw to the
           * current ctabx so that increased scan length will not be tried again,
           * and set the reverse index, ctabfit, to indicate that nothing has yet been
           * found to fit.
           */
          ctabw = ctabx;
          ctabfit = -1;
          while ((ctabx >= 0) && ((compseq[ctabx].scan-1) >= empty))
            ctabx = compseq[ctabx].next ;
          /*
           * there are now actual data within the scan length, so try again to find a
           * fit.
           */
          empty = -1;
        }
        else
        {
          /*
           * in this case, the buffer contains no actual data, so select use of
           * an entire block. the block will later be filled with the appropriate
           * non-data code for decompression
           */
          ctabx = ctabhi ;
          break ;
        }
      /*
       * continue scanning this block until a fit is found or the end of the table (maximum storage size)
       * is hit and the differences still don't fit.
       */
    } while ((ctabx >=0) && (done == FALSE)) ;

    /*
     * the differences will not fit correctly in any available storage unit.
     * cease scanning here, and return the number of samples actually compressed to the caller.
     * this will leave the data structures and status intact for resuming compression with
     * (possibly clipped) data.
     */
    if (ctabx < 0)
      break ;
    /*
     * check that a two-block scan has not been found starting in the last block of a frame.
     * this is not allowed. in this case, move to the next larger storage unit.
     * note that modifying the roaming table pointer is ok here, because if it is the last block
     * in the frame, the table pointer will not be used again.
     */
    if ((block == (WORDS_PER_FRAME-1)) && ((compseq[ctabx].cc == c5x12) || (compseq[ctabx].cc == c3x20)))
      ctabx = compseq[ctabx].next ;
    /*
     * using the selected storage unit, pack the differences into the current block and
     * update various counters and indices
     */
    sp = (compseqtype *)&compseq[ctabx] ;
    t_scan = sp->scan ;
    t_mask  = sp->mask ;
    ccp->last_2 = s[t_scan] ;
    ccp->last_1 = s[t_scan+1] ;
    ccp->peek_total = ccp->peek_total - t_scan ;
    ccp->next_out = (ccp->next_out + t_scan) % PEEKELEMS ;
    /*
     * when the peek buffer has run out of data, indicate in the
     * compression flag bits that the related words are not data.
     * for levels 1,2, this means block code=0;
     * for level 3, the block code=3, and the block itself must contain leading bits 11001.
     */
    if (ccp->peek_total < 0)
      if (level < 3)
      {
        block_code = 0 ;
        ccp->framebuf [ block ].l = 0 ;
      }
      else
      {
        block_code = 3 ;
        ccp->framebuf [ block ].l = NOTDIFFERENCE ;
      }
    else
    {
      /*
       * these storage optimizations speed things up considerably on some systems, even
       * though the compiler may be optimizing.
       * note that "i" is redefined within the current scope.
       */
      register SHORT i;
      register CHAR  t_shift;
      CHAR           t_halfshift ;

      ccp->final = ccp->last_1 ;
      block_code = sp->bc ;
#ifdef STATISTICS
      ccp->fits[sp->fit] = ccp->fits[sp->fit] + t_scan ;
#endif
      /*
       * except for c5x12, c3x20 (level 3), all compression types are handled by
       * the "default" clause.
       * the c5x12,c3x20 cases are special, because the data spans two "blocks", or 8 bytes.
       * note that these special cases "fall through" to the default case after processing
       * the first of two "blocks".
       */
      i = 0;
      accum = sp->cbits ;
      switch (sp->cc)
      {
      case c5x12 :
      case c3x20 :
        t_halfshift = (CHAR) (sp->shift >> 1) ;
        accum = 0 ;
        while (i < (t_scan >> 1))
          accum = (accum << sp->shift) | (t_mask & diffs [i++]) ;
        accum = (accum << t_halfshift) | ((t_mask & diffs [i]) >> t_halfshift) ;
        ccp->framebuf [ block ].l = accum ;
        flag_word = (flag_word << 2) + block_code ;
        block = block + 1 ;
        accum = sp->cbits ;
        accum = (accum << t_halfshift) | ((t_mask >> t_halfshift) & diffs [i++]) ;
      default:
        t_shift = sp->shift;
        while (i < t_scan)
          accum = (accum << t_shift) | (t_mask & diffs [i++]) ;
        ccp->framebuf [ block ].l = accum ;
        break ;
      }
    }
    flag_word = (flag_word << 2) + block_code ;
    block = block + 1 ;
  }
  /*
       * if the buffer ran into data too large to compress (ctabx < 0), fill the remainder of the frame
       * with "not data" and return. this will leave data structures intact for resumption of
       * compression with (possibly clipped) data.
       */
  while (block < WORDS_PER_FRAME)
  {
    if (level < 3)
    {
      block_code = 0 ;
      ccp->framebuf [ block ].l = 0 ;
    }
    else
    {
      block_code = 3 ;
      ccp->framebuf [ block ].l = NOTDIFFERENCE ;
    }
    flag_word = (flag_word << 2) + block_code ;
    block = block + 1 ;
  }
  /*
       * encode the difference type (1 or 2) in the most significant bit of the flag word.
       * this is done in all levels, although field-deployed code will currently not generate
       * second differences, and always therefore have the difference bit == 0.
       */
  if (difference == 1)
    diffbit = 0 ;
  else
    diffbit = SECONDDIFF ;
  /*
   * do not add the number of padded samples (after the peek buffer runs dry) to the
   * count of compressed samples in the frame.
       */
  clipped_total = ccp->peek_total ;
  if (clipped_total < 0)
    clipped_total = 0 ;
  number_of_samples = total_in_peek_buffer_at_entry - clipped_total ;
  ccp->framebuf [0].l = flag_word | diffbit ;
  ccp->frames = ccp->frames + 1 ;
  /*
       * endian-flip the frame at this point if required.
       */
  if (flip != 0)
    endianflip_frame ((cfp) ccp->framebuf) ;
  /*
   * check for the special condition that the caller has requested ALL the usable blocks (15) in the
   * frame to be reserved (as in filling an empty frame).
       */
  if (reserve >= (WORDS_PER_FRAME-1))
    return (0);
  /*
   * quit here if the buffer ran into data too large to compress
   */
  if (ctabx < 0)
  {
    if (number_of_samples == 0)
      return (-1) ;
    else
      return (number_of_samples) ;
  }
  
  /*
   * if compression is not level 3, or there are not enough samples available to be able
   * to use the possibly-liberated space in the event of flag-word replacment, the frame is done.
       */
  if ((level != 3) || (ccp->peek_total < 8))
    return (number_of_samples) ;
  /*
   * scan the replacement table to find a matching flag word and its replacement. if no match
   * found, compression is finished.
       */
  i = 0 ;
  while (i < 3)
    if (flag_word == commonflags[i].fword)
      break ;
    else
      i = i + 1 ;
  if (i >= 3)
    return (number_of_samples) ;
  flag_replacement = commonflags[i].repcode ;
  /*
       * a squeezable compression flag word has been found, but
       * at this point, it's still not definite that the replacement can be made: only if the
       * data fits in the available 24 bits. now try to compress again.
       * the following code is a concise, but slower version of the main scanning code above.
       */
  s[0] = ccp->last_2 ;
  s[1] = ccp->last_1 ;
  for (i = 0; i < 8; i++)
  {
    s[i+2] = (*(ccp->peeks))[(ccp->next_out + i) % PEEKELEMS] ;
    if (difference == 1)
      diffs [i] = s[i+2] - s[i+1] ;
    else
      diffs [i] = s[i+2] - (s[i+1] << 1) + s[i] ;
  }
  /*
       * start scanning in the portion of the table used only for flag-word squeezing
       */
  ctabx = 21 ;
  done = FALSE ;
  while ((ctabx >= 0) && (done == FALSE))
  {
    sp = (compseqtype *)&compseq[ctabx] ;
    for (i = 0; i < sp->scan; i++)
      if (labs(diffs[i]) > sp->disc)   /* if a sample in the group does not fit in the allowed size*/
      {
        ctabx = sp->next ;     /* go on to the next, larger size */
        break ;
      }
      else
        if (i == (sp->scan - 1))
        {
          done = TRUE ;      /* this must be the compression code that works */
          break ;
        }
  }
  /*
       * if the compression table was exhausted, the data did not fit in 24 bits, and so flag replacement
       * is not possible.
       */
  if (ctabx < 0)
    return (number_of_samples) ;
  /*
   * some samples are definitely squeezable into the flag word, so go ahead and compress them.
   * the number of samples in the record will be incremented accordingly.
   */
#ifdef STATISTICS
  ccp->squeezed_flags = ccp->squeezed_flags + 1 ;
#endif
  sp = (compseqtype *)&compseq[ctabx] ;
  ccp->last_2 = s[sp->scan] ;
  ccp->last_1 = s[sp->scan+1] ;
  ccp->final = ccp->last_1 ;
  ccp->peek_total = ccp->peek_total - sp->scan ;
  number_of_samples = number_of_samples + sp->scan ;
  ccp->next_out = (ccp->next_out + sp->scan) % PEEKELEMS ;
#ifdef STATISTICS
  ccp->fits[sp->fit] = ccp->fits[sp->fit] + sp->scan ;
#endif
  accum = 0 ;
  for (i = 0; i < sp->scan; i++)
    accum = (accum << sp->shift) | (sp->mask & diffs [i]) ;
  flag_word = flag_replacement | sp->cbits | (accum & GRAND) ;
  /*
       * replace the appropriately-byte-ordered flag word
       */
  if (flip != 0)
    ccp->framebuf [0].l = endianflip (flag_word | diffbit) ;
  else
    ccp->framebuf [0].l = flag_word | diffbit ;
  return (number_of_samples);
}




#ifdef __STDC__
LONG extend29 (LONG c)
#else
     LONG extend29 (c)
     LONG c;
#endif
     /*
      * function for sign-extending a 29-bit embedded integration constant
      * found in a level 3 frame
      */
{
  c = c & ICONSTMASK ;
  if ((c & ICONSTSIGN) != 0)
    c = c - (ICONSTMASK+1) ;
  return (c) ;
}


#ifdef __STDC__
LONG unpack (decompbittype *p,  LONG wunpk[],  SHORT start,  SHORT stop,  LONG accum)
#else
     LONG unpack (p,  wunpk,  start,  stop,  accum)
     decompbittype *p;
     LONG wunpk[];
     SHORT start;
     SHORT stop;
     LONG accum;
#endif
     /*
     * handles unpacking the bitfields in a 32-bit "block".
     */
{
  register SHORT k ;
  register LONG  work ;

  for ( k = start; k >= stop; k-- )
  {
    work = accum & p->mask ;
    if ((work & p->hibit) != 0)
      work = work - p->neg ;
    wunpk[k] = work ;
    accum = accum >> p->postshift ;
  } ;
  return (accum) ;
}



#ifdef __STDC__
SHORT decompress_frame (cfp framep,  unpacktype unpacked,  sptype final,  SHORT lastframesize,  SHORT level,
                        SHORT recursion,  SHORT flip,  SHORT ignore, sptype statcode)
#else
     SHORT decompress_frame (framep, unpacked, final, lastframesize, level, recursion, flip, ignore, statcode)
     cfp        framep;
     unpacktype unpacked;
     sptype     final;
     SHORT      lastframesize;
     SHORT      level;
     SHORT      recursion;
     SHORT      flip;
     SHORT      ignore;
     sptype     statcode;
#endif
     /*
     * passed:
     *        framep    :       pointer to the 64-byte frame to be decompressed.
     *        unpacked  :       pointer to an array large enough to hold the maximum number of samples
     *                            possible in level 3 in one frame + 2 history samples + 1 decoded
     *                            embedded integration constant for the "last sample" in a record == 146.
     *        final     :       a pointer to a 16-bit integer, see below.
     *        level     :       compression level of the frame, 1,2, or 3. the differencing operation
     *                            used to build the frame is internally encoded and will be automatically
     *                            determined.
     *   lastframesize  :       should be 0 for the very first frame to be decompressed,
     *                            and must be maintained by the caller for each frame to be decompressed.
     *                            it is used to update the previous history. see example usage
     *                            of the procedure.
     *       recursion  :       should always be 0 supplied by the caller.
     *            flip  :       0 to used the native endian-ness of the computer.
     *                          1 to reverse endian-ness.
     *          ignore  :       > 0 will ignore interpretation of any embedded integration constants at
     *                            and above the value, i.e. 1 will ignore all, 2 will ignore the second,
     *                            and so on. this is useful in the case where multiple smaller groups of
     *                            frames (such as, say, 9) each with embedded constants are batched togther
     *                            to create larger, say 4096-byte, records, and the integration constants
     *                            not at the beginning of the 4096-byte records may not contain valid
     *                            data, particularly the "final" sample.
     *       *statcode  :       pointer to a 16-bit integer into which will be returned a status of the
     *                              decompression.
     * returns:
     *     in unpacked  :       the 32-bit decompressed data will start at the third element
     *                            (unpacked[2]) of unpacked, with the first two elements containing
     *                            2 previous history samples, which may be used by the caller.
     *          *final  :       if non-zero is the index of "unpacked" that contains an embedded
     *                            integration constant for the "last sample" in a record. this may be
     *                            saved by the caller to be used at the end of a group of frames to
     *                            check decompression.
     *       *statcode  :       see above.
     *   function value :       will return the number of samples decompressed, or -1 if there was a
     *                            detectable error.
     */
{
  typedef struct
  {
    LONG              fword ;
    LONG              repcode ;
  } flagreptype ;

  const flagreptype   commonflags [3] =
  { {0x15555555, 0x50000000},
    { 0x2AAAAAAA, 0x60000000},
    { 0x3FFFFFFF, 0x70000000 } };

  const decompbittype decomptab [12] =
  { {5 ,  12 ,        4095 ,        2048 ,        4096},
    { 1 ,   0 ,  1073741823 ,   536870912 ,  1073741824},
    { 2 ,  15 ,       32767 ,       16384 ,       32768},
    { 3 ,  10 ,        1023 ,         512 ,        1024},
    { 5 ,   6 ,          63 ,          32 ,          64},
    { 6 ,   5 ,          31 ,          16 ,          32},
    { 7 ,   4 ,          15 ,           8 ,          16},
    { 9 ,   3 ,           7 ,           4 ,           8},
    { 3 ,  20 ,     1048575 ,      524288 ,     1048576},
    { 4 ,   8 ,         255 ,         128 ,         256},
    { 2 ,  16 ,       65535 ,       32768 ,       65536},
    { 1 ,   0 ,          -1 ,           0 ,           0 } };

  const decompbittype squeezeddecomptab [8] =
  { {0 ,   0 ,           0 ,           0 ,           0},
    { 8 ,   3 ,           7 ,           4 ,           8},
    { 6 ,   4 ,          15 ,           8 ,          16},
    { 4 ,   6 ,          63 ,          32 ,          64},
    { 3 ,   8 ,         255 ,         128 ,         256},
    { 2 ,  12 ,        4095 ,        2048 ,        4096},
    { 1 ,  24 ,    16777215 ,     8388608 ,    16777216},
    { 0 ,   0 ,           0 ,           0 ,           0 } };


  LONG              flag_word ;
  REGISTER SHORT    i, j ;
  REGISTER SHORT    fp ;
  SHORT             flags[WORDS_PER_FRAME] ;
  SHORT             blockindex ;
  SHORT             subcode, twoblocksubcode ;
  REGISTER LONG     accum ;
  LONG              wunpk[MAXSAMPPERWORD] ;
  LONG              squeezed_flagword ;
  SHORT             difference ;
  LONG              block, nextblock ;
  LONG              samp_1, samp_2 ;
  SHORT             embeddedcount, integrate ;
  SHORT             blockcode ;
  decompbittype     *dcp ;
  compressed_frame  eframe ;
  SHORT             scode ;
  SHORT             halfshift ;
  SHORT             middle ;
  LONG              halfmask ;


  /*
   * before any processing, endian-flip the entire frame if necessary. because "decompress_frame"
   * is recursive, this should be done only once by the top level, and because it's generally not
   * good practice to modify the caller's data, make a local copy of the flipped frame, and replace
   * the pointer to point to it.
   */
  if (flip != 0)
  {
    memcpy ((char *)eframe, (char *)framep, (int)sizeof(compressed_frame)) ;
    framep = (compressed_frame*)eframe ;
    endianflip_frame (framep) ;
  }

  /*
   * indicate to the caller that the "final" sample to be returned to the caller at the end of
   * the "unpacked" array has not been found. the caller will save this value for checking
   * decompression at the end of a group of frames.
   * also zero the return status, assuming outcome will be ok.
   */
  *final = 0 ;
  *statcode = EDF_OK ;
  /*
   * this is the number of embedded integration constants found in the frame.
   */
  embeddedcount = 0 ;
  /*
   * first update the history of the "unpacked" storage using the results of decompression
   * of the previous frame. if there was no previous frame, initialize history to 0 to avoid
   * using otherwise undefined history contents. leave the history unchanged if this a recursive
   * call to re-sync the data stream, because the history has been properly initialized in this case.
   */
  if (recursion == 0)
  {
    if (lastframesize > 0)
    {
      unpacked[0] = unpacked[lastframesize] ;
      unpacked[1] = unpacked[lastframesize+1] ;
    }
    else
    {
      unpacked[1] = 0 ;
      unpacked[0] = 0 ;
    }
  }
  
  /*
   * initialize the decompressed sample counter, and get the bit-map flag word from the
   * beginning of the frame.
   */
  fp = 0 ;
  flag_word = (*framep)[0].l ;
  /*
   * in level 3, the flag-word most-significant bit contains the differencing type (1 or 2).
   * nobody says that this cannot be used for the differencing code in level 1 or 2, so always
   * decode it. in field-deployed level 1 and 2, this bit is always 0, indicating first differences.
   */
  if ((flag_word & SECONDDIFF) != 0)
    difference = 2 ;
  else
    difference = 1 ;
  /*
   * check before disassembling the flag word whether it a "squeezed" flag (only in level 3).
   */
  i = 0 ;
  while (i < 3)
    if ((flag_word & REPLACEMENTMASK) == commonflags[i].repcode)
      break ;
    else
      i = i + 1 ;
  /*
   * if the flag replacement word was found in the table, translate first to the original
   * flag word, and save the squeezed flag word for decompression at the end of the frame.
   */
  if (i < 3)
  {
    squeezed_flagword = flag_word ;
    flag_word = commonflags[i].fword ;
  }
  else
    squeezed_flagword = 0 ;
  /*
   * now decompose the flag word into 15 2-bit map nibbles, corresponding to each following "block"
   */
  for ( i = (WORDS_PER_FRAME-1); i >= 1; i--)
  {
    flags [i] = (SHORT)(flag_word & 3) ;
    flag_word = flag_word >> 2 ;
  }
  /*
   * and start decompressing each "block" according to its map nibble
   */
  blockindex = 1 ;
  while (blockindex < WORDS_PER_FRAME)
  {
    integrate = 0 ;
    blockcode = flags[blockindex] ;
    block = (*framep)[blockindex].l ;
    if (((level < 3) && (blockcode == 0)) || ((level == 3) && (block == NOTDIFFERENCE)))
    {
      /*
       * not compressed data. in level 1, this can mean either the integration constants
       * are stored in this block, or the block is empty.
       * in level 3, not-a-difference is explicitly represented as a unique code.
       * assume that this is a data-bearing frame, and that some differences will follow.
       */
      if ((level < 3) && (ignore < blockindex))
        switch (blockindex)
        {
        case 1 :
          samp_1 = block ;
          embeddedcount = 1 ;
          break ;
        case 2 :
          unpacked[UNPACKFINAL] = block ;
          *final = UNPACKFINAL ;
          break ;
        }
    }
    else
      if ((level == 3) && (blockcode == 3) && ((block & ICONSTCODE) == ICONSTCODE))
      {
        /*
         * this is a unique code for an integration constant in level 3.
         * if block=1, this is the first uncompressed 29-bit sample in the frame (samp_1)
         * if block=2, this is the second uncompressed 29-bit sample in the frame (samp_2)
         * if block=3, this is the last uncompressed 29-bit sample in the record, which is
         * inserted after a merge of multiple frames, and used as a decompression check.
         */
        if (ignore < blockindex)
          switch (blockindex)
          {
          case 1 :
            samp_1 = extend29 (block) ;
            embeddedcount = 1 ;
            break ;
          case 2 :
            samp_2 = extend29 (block) ;
            embeddedcount = 2 ;
            break ;
          case 3 :
            unpacked[UNPACKFINAL] = extend29 (block) ;
            *final = UNPACKFINAL ;
            break ;
          }
      }
      else
      {
        if (blockcode == 1)
          /*
           * block code == 1 is always 4 8-bit differences in all levels
           */
          subcode = 9 ;
        else
          if (((level == 1) && (blockcode == 2)) || ((level == 3) && (blockcode == 0)))
            /*
             * 2 16-bit differences are available in levels 1 and 3 under different block codes
             */
            subcode = 10 ;
          else
            if (((level == 1) && (blockcode == 3)))
              /*
               * single 32-bit differences are available only in level 1
               */
              subcode = 11 ;
            else
              /*
               * in level 2, 3: extract an index into the decompression table from the map nibble
               * and the additional bits inside the "block". all 8 possible such codes are used in
               * level 3, but only 6 are used in level 2. this code will interpret all 8 states,
               * and therefore fail on corrupted level 2 data.
               * all cases except those that span two "blocks" are handled by the
               * clause following "if subcode > 0".
               * special handling is needed for the c5x12, c3x20 cases.
               */
              subcode = (SHORT)(((blockcode - 2) << 2) + ((block >> 30) & 3)) ;
        /*
         * using the selected decompression table pointer, decode the differences
         * and integrate.
         */
        dcp = (decompbittype *)&decomptab[subcode] ;
        accum = block ;
        if (subcode > 0)
        {
          unpack (dcp, wunpk, dcp->samps-1, 0, accum) ;
          integrate = dcp->samps ;
        }
        else
          /*
           * this handles the 5-difference/12-bit
           * and 3-difference/20-bit cases in level 3 that span two "blocks".
           */
          if (blockindex < (WORDS_PER_FRAME-1))
          {
            blockindex = blockindex + 1 ;
            nextblock = (*framep)[blockindex].l ;
            twoblocksubcode = (SHORT)((nextblock >> 30) & 3) ;
            switch (twoblocksubcode)
            {
            case 0 :
            case 1 :
              dcp = (decompbittype *)&decomptab [8*twoblocksubcode] ;
              halfshift = (SHORT)(dcp->postshift >> 1) ;
              middle = (SHORT)(dcp->samps >> 1) ;
              halfmask = (dcp->mask >> halfshift) ;
              wunpk[middle] = accum & halfmask ;
              accum = accum >> halfshift ;
              unpack (dcp, wunpk, middle-1, 0, accum) ;
              accum = nextblock ;
              accum = unpack (dcp, wunpk, dcp->samps-1, middle+1, accum) ;
              accum = (accum & halfmask) | (wunpk[middle] << halfshift) ;
              unpack (dcp, wunpk, middle, middle, accum) ;
              integrate = dcp->samps ;
              break ;
            default:
              *statcode |= EDF_SECSUBCODE ;
              break ;
            }
          }
          else
            *statcode |= EDF_TWOBLOCK ;
      }
    /*
     * integrates (once or twice) the unpacked bit fields to reconstruct the original data.
     * if the buffer is overrun, the data are garbage anyway, because this can't happen if
     * the original frame was intact. this just prevents clobbering somebody else's data.
     */
    if ((fp + integrate) > UNPACKFINAL)
      *statcode |= EDF_OVERRUN ;
    else
    {
      if (integrate > 0)
      {
        if (difference == 1)
        {
          for (j = 0; j < integrate; j++)
          {
            unpacked[fp+2] = wunpk[j] + unpacked[fp+1] ;
            fp = fp + 1 ;
          }
        }
        else
        {
          for (j = 0; j < integrate; j++)
          {
            unpacked[fp+2] = wunpk[j] + (unpacked[fp+1] << 1) - unpacked[fp] ;
            fp = fp + 1 ;
          }
        }
      }
    }
    blockindex = blockindex + 1 ;
  }
  /*
   * all "blocks" are decompressed. now check if the flag word was "squeezed", indicating additional
   * samples to decompress
   */
  if (squeezed_flagword != 0)
  {
    subcode = (SHORT)((squeezed_flagword >> 24) & 7) ;
    dcp = (decompbittype *)&squeezeddecomptab [subcode] ;
    if (dcp->samps > 0)
      if ((fp + dcp->samps) > UNPACKFINAL)
        *statcode |= EDF_OVERRUN ;
      else
      {
        accum = (squeezed_flagword & 16777215) ;
        unpack (dcp, wunpk, dcp->samps-1, 0, accum) ;
        for (j = 0; j < dcp->samps; j++)
        {
          if (difference == 1)
            unpacked[fp+2] = wunpk[j] + unpacked[fp+1] ;
          else
            unpacked[fp+2] = wunpk[j] + (unpacked[fp+1] << 1) - unpacked[fp] ;
          fp = fp + 1 ;
        }
      }
    else
      *statcode |= EDF_REPLACEMENT ;
  } ;
  /*
   * all samples have been decompressed. if this is the very first frame to be decompressed,
   * the result will probably not be right, because the initial conditions (unpacked[0],[1])
   * were not known. in this case, recursively repeat the decompression after some linear
   * algebra to define the initial conditions, based on the decompression just completed. note
   * that if there are really errors in the data, repeating this recursively will not improve
   * after the second try.
   */
  if (((embeddedcount >= 1) && (unpacked[2] != samp_1)) ||
      ((embeddedcount >= 2) && (unpacked[3] != samp_2)))
  {
    if (recursion == 0)
    {
      if (fp > 0)
      {
        /*
         * report errors only after the first frame. errors on the first frame are expected.
         */
        if (lastframesize > 0)
          *statcode |= EDF_INTEGRESYNC ;
        if (difference == 1)
          unpacked[1] = unpacked[1] + samp_1 - unpacked[2] ;
        else
        {
          if ((fp < 2) && (embeddedcount < 2))
          {
            /*
             * an insufficient number of samples are present to actually reconstruct
             * the data. this may happen if a record with one sample is written. in this
             * case, set unpacked[3]=samp_2=0, which will guarantee reconstruction of the
             * first sample.
             */
            unpacked[3] = 0 ;
            samp_2 = 0 ;
          }
          unpacked[0] = -3*unpacked[2]+3*samp_1+2*unpacked[3]-2*samp_2+unpacked[0] ;
          unpacked[1] = unpacked[3]-samp_2-2*unpacked[2]+2*samp_1+unpacked[1] ;
        }
        fp = decompress_frame (framep, unpacked, final, 0, level, recursion+1, 0, ignore, &scode) ;
      }
      else ;      /* no samples decompressed. note the null statement */
    }
    else
    {
      /*
       * if decompression failed, and this is the second attempt, return -1 to the caller
       * to indicate that data were present, but wrong.
       */
      fp = -1 ;
      *statcode |= EDF_INTEGFAIL ;
    }
  }
  /*
   * return the number of samples actually decompressed
   */
  return (fp) ;
}




#ifdef __STDC__
BOOLEAN dferrorfatal (SHORT statcode, FILE *print)
#else
     BOOLEAN dferrorfatal (statcode, print)
     SHORT   statcode;
     FILE    *print;
#endif
     /*
      * used to print an error string describing the associcated decompression errors.
      * passed:
      *     statcode    : the status code returned by decompress frame.
      *         print   : file pointer to which you want status messages to be printed.
      *                     NULL otherwise.
      * returns:
      *     TRUE        : if a "fatal" decompression error was encountered on this frame,
      *                     i.e. the frame could not be decompressed without errors.
      *                   TRUE is not returned if decompression encountered a failure at
      *                     the integration constant check (EDF_INTEGRESYNC), which may
      *                     happen if a record is simply missing. will also not return fatal
      *                     if the sample count is wrong, or the last integration-constant
      *                     check produces an error. somebody may have just made a mistake.
      *                     of course, if you consider these errors fatal, check for them
      *                     explicitly in "statcode" in your user program.
      */
{
  if (print != NULL)
  {
    /*if ((statcode & (EDF_FATAL|EDF_NONFATAL)) != 0)
      fprintf (print,"\n") ;*/
    if (statcode & EDF_SECSUBCODE)
      fprintf (print,"decompress frame: illegal secondary subcode spanning two blocks.\n") ;
    if (statcode & EDF_TWOBLOCK)
      fprintf (print,"decompress frame: illegal two-block code at end of frame.\n") ;
    if (statcode & EDF_OVERRUN)
      fprintf (print,"decompress frame: unpacked buffer overrun.\n") ;
    if (statcode & EDF_REPLACEMENT)
      fprintf (print,"decompress frame: illegal flag-word replacement subcode.\n") ;
    if (statcode & EDF_INTEGRESYNC)
      fprintf (print,"decompress frame: expansion error at integration constant check.\n") ;
    if (statcode & EDF_INTEGFAIL)
      fprintf (print,"decompress frame: expansion failed. frame internally damaged.\n") ;
#if 0
    if (statcode & EDF_LASTERROR)
      fprintf (print,"decompress record: last sample does not agree with decompression.\n") ;
    if (statcode & EDF_COUNTERROR)
      fprintf (print,"decompress record: sample count disagreement.\n") ;
#endif
  }
  return ((statcode & EDF_FATAL) != 0) ;
}




/*
   * *****************************************************************************
   * the following are pseudo-object-oriented functions to deal with compression *
   * *****************************************************************************
   */

#ifdef __STDC__
void fix (ccptype ccp)
#else
     void fix (ccp)
     ccptype ccp;
#endif
     /*
     * this procedure is used to clip differences which cannot be compressed in level 2 or 3.
     * only first differences are checked here, because if a first difference cannot be made to
     * fit, it is highly unlikely that a second difference will fit. more definitive action
     * could be taken, or the data could simply be clipped at, say, +/- 27 bits, which is
     * absolutely guaranteed to fit under any circumstances. note that the method here may not
     * be the smartest way to handle too-large data, since this may generate a series of
     * 1-sample frames, because only one sample is "fixed" at a time. clipping at +/- 27 bits,
     * (28-bit two's complement, almost) is probably the more straight-forward method), although
     * in principle the method here allows representing data larger than 27 bits, as long
     * as differences are sufficiently small.
     */
{
  LONG  diff ;
  LONG  s0, s1 ;

  s1 = (*(ccp->peeks)) [ccp->next_out] ;
  s0 = ccp->last_1 ;
  diff = s1 - s0 ;
  if (diff < 0)
    if (diff < -536870911)
      diff = -536870911 ;
    else ;
  else
    if (diff > 536870911)
      diff = 536870911 ;
  s1 = diff + s0 ;
  (*(ccp->peeks)) [ccp->next_out] = s1 ;
}



#ifdef __STDC__
void clear_compression (ccptype ccp,  SHORT level)
#else
     void clear_compression ( ccp,  level)
     ccptype ccp;
     SHORT level;
#endif
     /*
     * will reset buffer indices and install the minimum number of samples required to call
     * "compress_frame" without the chance of generating a "padded" frame, according to the
     * selected compression level. note that the compression level can be changed by calling
     * this routine with the appropriate level.
     */
{
  SHORT  i ;

#ifdef STATISTICS
  for (i = 0; i < 33; i++)
    ccp->fits [i] = 0 ;
  ccp->squeezed_flags = 0 ;
#endif
  ccp->next_in = 0 ;
  ccp->peek_total = 0 ;
  ccp->next_out = 0 ;
  ccp->last_1 = 0 ;
  ccp->last_2 = 0 ;
  ccp->frames = 0 ;
  switch (level)
  {
  case 1 :
    ccp->peek_threshold = 15*4 + 2 ;
    break;
  case 2 :
    ccp->peek_threshold = 15*7 + 2 ;
    break;
  case 3 :
    ccp->peek_threshold = 15*9 + 8 + 2 ;
    break;
  }
}



#ifdef __STDC__
ccptype init_compression (SHORT level)
#else
     ccptype init_compression (level)
     SHORT level;
#endif
     /*
     * will allocate a "compression_continuity" structure, and then
     * will allocate a "peek" buffer, and install a pointer to it in the "compression_continuity".
     * because this actually allocates memory, this function should be called
     * only at program initialization. (you can use your favorite memory allocator).
     * returns a NULL pointer if no memory is available for either allocated structure.
     */
{
  ccptype ccp ;

  if ((ccp = (ccptype)malloc(sizeof(compression_continuity))) == NULL )
    return (NULL);
  if ((ccp->peeks = (peekbuffertype*)malloc(sizeof(peekbuffertype))) == NULL )
  {
    free (ccp);
    return (NULL);
  }
  clear_compression (ccp, level) ;
  return (ccp) ;
}


#ifdef __STDC__
LONG final_sample (ccptype ccp)
#else
     LONG final_sample (ccp)
     ccptype ccp;
#endif
{
  return (ccp->final) ;
}



#ifdef __STDC__
SHORT peek_threshold_avail (ccptype ccp)
#else
     SHORT peek_threshold_avail (ccp)
     ccptype ccp;
#endif
     /*
     * returns >= 0 if at least enough samples are in the associated "peek" buffer to guarantee
     * a full compressed frame. if < 0, indicates how many more are needed.
     */
{
  return (ccp->peek_total - ccp->peek_threshold) ;
}



#ifdef __STDC__
SHORT peek_contents (ccptype ccp)
#else
     SHORT peek_contents (ccp)
     ccptype ccp;
#endif
     /*
     * returns the current number of samples in the "peek" buffer. maximum value is PEEKELEMS.
     * minimum value may be negative, and if so, indicates samples were padded in the compressed_frame
     * that produced the condition (the absolute value is the number of padded 32-bit "blocks".
     * when the peek buffer contents goes negative, buffer pointers should be reset with a
     * call to "clear_compression" before new data is read into the peek buffer and compression
     * resumed.
     */
{
  return (ccp->peek_total) ;
}


#ifdef __STDC__
LONG frames (ccptype ccp)
#else
     LONG frames (ccp)
     ccptype ccp;
#endif
{
  return (ccp->frames) ;
}



#ifdef __STDC__
SHORT blocks_padded (ccptype ccp)
#else
     SHORT blocks_padded (ccp)
     ccptype ccp;
#endif
     /*
     * returns the number of non-data 32-bit "blocks" padded at the last frame to compress,
     * which happens when the "peek" buffer runs out of data.
     */
{
  if (ccp->peek_total < 0)
    return (abs(ccp->peek_total)) ;
  else
    return (0) ;
}



#ifdef __STDC__
SHORT peek_write (ccptype ccp,  LONG samples[], SHORT numwrite)
#else
     SHORT peek_write (ccp,  samples,  numwrite)
     ccptype     ccp;
     LONG        samples[];
     SHORT       numwrite;
#endif
     /*
     * will write 32-bit samples to the peek buffer if there is room. normally this
     * would be called only if you know there is room.
     */
{
  SHORT     written = {0} ;

  while ((ccp->peek_total < PEEKELEMS) && (written < numwrite))
  {
    (*(ccp->peeks))[ccp->next_in] = *(samples++) ;
    ccp->next_in = (ccp->next_in + 1) % PEEKELEMS ;
    ccp->peek_total++ ;
    written++ ;
  }
  return (written) ;
}


#ifdef __STDC__
void insert_constant (cfp framep,  LONG sample,  SHORT level,  SHORT flip,  SHORT blockindex, BOOLEAN force)
#else
     void insert_constant (framep,  sample,  level,  flip,  blockindex, force)
     cfp      framep;
     LONG     sample;
     SHORT    level;
     SHORT    flip;
     SHORT    blockindex;
     BOOLEAN  force;
#endif
     /*
    * this procedure will insert an integration constant into a previously-compressed frame
    * at the requested block. this will not check that the block has previously reserved, although
    * it will check if at least the block contents are either 0 or a non-data code, according
    * to the level. typically, "blockindex" will be 3 for level 3 data and 2 for level 2 data.
    */
{
  /*
   * first flip the byte order to the native order.
   */
  if (flip != 0)
    (*framep)[blockindex].l = endianflip ((*framep)[blockindex].l) ;
  if (level < 3)
    if (force || ((*framep)[blockindex].l == 0))
      (*framep)[blockindex].l = sample ;
    else ;
  else
    /*
     * in level 3, an integration constant is identified with a unique code in the
     * 3 block lead-in bits, allowing up to 29 bits for constant storage.
     */
    if (force || ((*framep)[blockindex].l == NOTDIFFERENCE))
      (*framep)[blockindex].l = (sample & ICONSTMASK) | ICONSTCODE ;
  /*
   * finaly restore byte order to the desired target.
   */
  if (flip != 0)
    (*framep)[blockindex].l = endianflip ((*framep)[blockindex].l) ;
}


/*
   * *****************************************************************************
   * the following are pseudo-object-oriented functions to deal with compressing *
   * a group of frames at levels 1,2,3 while adjusting the differencing for      *
   * maximum compression, and inserting integration constants at regular         *
   * intervals.                                                                  *
   * *****************************************************************************
   */

#ifdef __STDC__
adptype init_adaptivity (SHORT diff, SHORT fpt, SHORT fpp, SHORT level, SHORT flip)
#else
     adptype init_adaptivity (diff,  fpt,  fpp,  level,  flip)
     SHORT diff;
     SHORT fpt;
     SHORT fpp;
     SHORT level;
     SHORT flip;
#endif
     /*
     * used to initialize the structure for adaptive differencing and integration constant insertion.
     *
     * passed:
     *          diff    :       the difference level if compression level is 1 or 2
     *          fpp     :       "frames per package", where a "package" is a group of frames comprising
     *                            a larger record. this is typically 63 in a 4096-byte SEED record,
     *                            with 64 bytes reserved for a header. every "fpp" frames, integration
     *                            constants will be inserted in the compressed frame.
     *          fpt     :       "frames per trial" is the number of frames, starting at the first
     *                            frame in a package, over which the effect of first or second
     *                            differencing will be measured. the remainder of the package will
     *                            be compressed with the differencing most effective over the first
     *                            "fpt" frames in the package. setting "fpt" == "fpp" produces adaptive
     *                            differencing each frame (which is overkill)
     *          level   :       the compression level
     *          flip    :       0 for native byte order.
     *                          1 for endian-flipped byte order
     * returns:
     *   function value :       pointer to "adaptivity_control" struc, initialized.
     *                          a "compression_cotinuity" and "adaptivity_control" were successfully
     *                              allocated if the function returns a non-NULL pointer.
     */
{
  adptype adp;

  if ((adp = (adptype)malloc(sizeof(adaptivity_control))) == NULL )
    return (NULL) ;
  /*
   * allocate a compression continuity structure
   */
  if ((adp->ccp = (ccptype)init_compression (level)) == NULL )
  {
    free (adp) ;
    return (NULL) ;
  }
  adp->bestdiff = 0 ;
  adp->trials = 0 ;
  adp->firstframe = 0 ;
  adp->difference = diff ;
  adp->framespertrial = fpt ;
  adp->framesperpackage = fpp ;
  adp->level = level ;
  adp->flip = flip ;
  return (adp) ;
}



#ifdef __STDC__
SHORT compress_adaptively (adptype adp)
#else
     SHORT compress_adaptively (adp)
     adptype adp;
#endif
     /*
     * this will compress a frame considered to be part of a "package" (see the description of
     * "init_adaptivity" above) that will be compressed at the requested level, with integration
     * constants inserted at appropriate intervals.
     *
     *  passed:
     *          adp     :       pointer to "adaptivity_control" struc, initialized as above.
     *
     *  funtion value returns the number of samples actually compressed, as compress_frame does.
     *
     */
{
  SHORT      reserve, used, used1 ;
  compression_continuity saved [2] ;
  /*
   * this will insert the appropriate number of integration constants in the first frame
   * in a "package" of frames, according to the compression level
   */
  if ((adp->ccp->frames % adp->framesperpackage) == adp->firstframe)
    if (adp->level < 3)
      reserve = 2 ;
    else
      reserve = 3 ;
  else
    reserve = 0 ;
  if (adp->level < 3)
    /*
     * straight non-adaptive compression in level 1,2 using first differences.
     */
    used = compress_frame (adp->ccp, adp->difference, adp->level, reserve, adp->flip) ;
  else
  {
    /*
     * in level 3, a subset of frames will be used to determine the best differencing.
     * the best average difference level will be used for the following frames. level 3
     * could be fully adaptive, only at the expense of CPU time, although overall compression
     * improvement will probably be negligible.
     */
    if ((adp->ccp->frames % adp->framesperpackage) == adp->firstframe)
    {
      adp->bestdiff = 0 ;                             /* reset the best difference accumulator */
      adp->trials = adp->framespertrial ;             /* preset counter to how many frames to accumulate */
    }
    if (adp->trials)
    {
      /*
       * within the first "framespertrial" frames in the "package", use both
       * first and second differences, and use the best result. accumulate
       * the difference that does the best job, so that this can be used
       * for the remainder of the "package"
       */
      saved[0] = *(adp->ccp) ;
      used1 = compress_frame (adp->ccp, 1, 3, reserve, adp->flip) ;
      saved[1] = *(adp->ccp) ;
      *(adp->ccp) = saved[0] ;
      used = compress_frame (adp->ccp, 2, 3, reserve, adp->flip) ;
      if (used <= used1)
      {
        *(adp->ccp) = saved[1] ;
        used = used1 ;
      }
      else
        adp->bestdiff = adp->bestdiff + 1 ;
      if (!(--adp->trials))
        adp->difference = ((2*adp->bestdiff >= adp->framespertrial) ? 2 : 1) ;
    }
    else
      used = compress_frame (adp->ccp, adp->difference, 3, reserve, adp->flip) ;
  } ;
  return (used) ;
}


/*
   * *******************************************************************************
   * the following are pseudo-object-oriented functions to deal with decompressing *
   * generic data records including                                                *
   * embedded integration constants that will be automatically parsed.             *
   * *******************************************************************************
   */

#ifdef __STDC__
void clear_generic_decompression (dcptype dcp)
#else
     void clear_generic_decompression (dcp)
     dcptype dcp;
#endif
{
  /*
   * this procedure is used to clear decompression history (such as after an unrecoverable expansion error)
   */
  dcp->numdecomp = 0 ;
  dcp->samp_1 = 0 ;
  dcp->samp_2 = 0 ;
}




#ifdef __STDC__
dcptype init_generic_decompression ()
#else
     dcptype init_generic_decompression ()
#endif
{
  /*
   *  init_generic_decompression:   allocate and initialize a decompression_continuity structure, to be
   *                                  used with "decompress_generic_record".
     *
     *  passed          :  nothing
     *  returns:
     *  function value  :  a pointer to a decompression_continuity structure, or NULL if memory is unavailable.
     */

  dcptype dcp ;

  if ((dcp = (dcptype)malloc(sizeof(decompression_continuity))) == NULL )
    return (NULL) ;
  clear_generic_decompression (dcp) ;
  return (dcp) ;
}





#ifdef __STDC__
LONG decompress_generic_record (generic_data_record *gdr, LONG udata[], sptype statcode, dcptype dcp,
                                SHORT firstframe, LONG headertotal, SHORT level, SHORT flip, SHORT dataframes)
#else
     LONG decompress_generic_record (gdr, udata, statcode, dcp, firstframe, headertotal, level, flip, dataframes)
     generic_data_record *gdr;
     LONG                udata[];
     sptype              statcode;
     dcptype             dcp;
     SHORT               firstframe;
     LONG                headertotal;
     SHORT               level;
     SHORT               flip;
     SHORT               dataframes;
#endif
{
  /*
   * decompress_generic_record:    will decompress an entire data record into a linear array of LONG's.
   *
   * passed:
   *        gdr :   pointer to a generic data record, starting with the header.
   *      udata :   pointer to an array that will hold the decompressed data from the entire record.
   *                    it is the caller responsibility to see that this is big enough.
   *        dcp :   pointer to a "decompression_continuity" structure for this channel, which has
   *                    previously been allocated and initialized by "init_decompression".
   * firstframe :   the frame at which data begins, normally 0.
   * headertotal:   the number of samples said to be in the record according to a stored value in a header.
   *                if this value < 0, no check will be performed.
   *      level :   the compression level 1/2/3.
   *       flip :   >0 to endian-flip the data before processing.
   * dataframes :   the number of data frames, e.g. 63 for a 4096-byte record with 64-byte header.
       *
       * returns:
       *      udata :   decompressed data.
       *   statcode :   cumulative status of decompression of all frames in the record. any irregularity
       *                    will appear as one of the EDF_... error code bits, which may be printed
       *                    using the "dferrorfatal" function.
       *        dcp :   various internal elements will be updated.
       * function value:    the number of samples in the record, or a negative value if unsuccessful.
       *
       */

  compressed_frame          *framep ;
  SHORT                     i, framecount ;
  SHORT                     thisdecompress ;
  unpacktype                samples ;
  SHORT                     final, dstat ;
  LONG                      total_in_record ;
  SHORT                     ignore, havefinal ;
  LONG                      finalsample ;

      /*
       * restore the decompression continuity
       */
  if (dcp->numdecomp > 0)
  {
    samples[dcp->numdecomp] = dcp->samp_1 ;
    samples[dcp->numdecomp+1] = dcp->samp_2 ;
  }
  /*
       * zero a few items for decompressing this record
       */
  total_in_record = 0 ;
  havefinal = 0 ;
  ignore = 0 ;
  *statcode = 0 ;
  /*
   * calculate number of frames to process, and starting frame pointer.
       */
  framecount = dataframes - firstframe ;
  framep = (cfp)&(gdr->frames[firstframe][0]) ;
  /*
   * decompress each frame
   */
  while (framecount--)
  {
    /*
     * decompress the frame into the temporary array "samples", and update frame pointer.
          */
    if ((thisdecompress = decompress_frame(framep++, samples, &final, dcp->numdecomp, level, 0, flip, ignore, &dstat)) > 0)
      dcp->numdecomp = thisdecompress ;
    /*
     * accumulate the decompression status and
     * bail out silently if the data cannot be decompressed, allowing the caller to do what he wants.
          */
    *statcode |= dstat ;
    if (dferrorfatal(*statcode, NULL))
      return (-1) ;
    /*
     * for all frames past the first data frame, ignore any embedded constants
     */
    ignore = 2 ;
    if (level > 2)
      ignore = 3 ;
    /*
     * look for an embedded end-of-record constant, and save it for later.
     * this will happen only on the first data frame.
     */
    havefinal = havefinal + final ;
    if (final > 0)
      finalsample = samples[final] ;
    /*
     * the decompressed data are available here in the "samples" array, starting at "samples[2]".
     * copy them into the caller's linear buffer containing data for the entire record.
     */
    for (i = 1; i <= thisdecompress; i++)
      udata[total_in_record++] = samples[i+1] ;
  }
  /*
       * at the last frame of the record, check the end-of-record integration constant
       */
  if (havefinal != 0)
    if (dcp->numdecomp != 0)
      if (finalsample != samples[dcp->numdecomp+1])
        *statcode |= EDF_LASTERROR ;
  if ((headertotal >=0) && (headertotal != total_in_record))
    *statcode |= EDF_COUNTERROR ;
  /*
   * update the continuity structure for the next record
   */
  if (dcp->numdecomp > 0)
  {
    dcp->samp_1 = samples[dcp->numdecomp] ;
    dcp->samp_2 = samples[dcp->numdecomp+1] ;
  }
  /*
       * return the total number of samples decompressed
       */
  return (total_in_record) ;
}



#ifdef __STDC__
SHORT compress_generic_record (gdptype gdp,  SHORT firstframe)
#else
     SHORT compress_generic_record (gdp, firstframe)
     gdptype gdp;
     SHORT   firstframe;
#endif
{
  /*
   * compress_generic_record:
     *
     *          function to compress a single frame of an entire data record. continuity
     *          is maintained in the *gdp structure.
     *
     * passed:
     *          gdp     :   the adaptive generic data record control structure pointer
     *     firstframe   :   the frame at which data begins (where the last sample will be inserted),
     *                      typeically 1.
     *
     * returns:             the number of full data frames in the record, excluding the header.
     *                      the record is full when this number is == gdp->adp->framesperpackage,
     *                      which is initialized by the caller. this is typically the total number
     *                      of frames in a record - 1, e.g. 63 for a 4096-byte record.
     */
  cfp     framep;
  SHORT   used;
  SHORT   blockindex;
  /*
     * if a record is already built, return the information to the caller. note that if
     * the current frame is sitting at the end of the record, don't try to go beyond.
     */
  if (gdp->adp->ccp->frames >= gdp->adp->framesperpackage)
    return ((SHORT)gdp->adp->ccp->frames) ;
  /*
   * compress the next frame, and move the results into the record from the compression
   * continuity frame buffer when done
   */
  framep = (cfp)&(gdp->gdr->frames[gdp->adp->ccp->frames][0]) ;
  used = compress_adaptively (gdp->adp) ;
  memcpy ((char *)framep, (char *)gdp->adp->ccp->framebuf, (int)sizeof(compressed_frame)) ;
  /*
     * totalize the number of samples in the entire record, or repair differences that were
     * too large to fit.
     */
  if (used > 0)
    gdp->nsamples = gdp->nsamples + used ;
  else
    fix (gdp->adp->ccp) ;
  /*
   * install the last sample in the record if the last frame has just been compressed.
     */
  if (gdp->adp->ccp->frames >= gdp->adp->framesperpackage)
  {
    /*
     * calculate at which "block" the last sample will appear, depending on the level
     */
    if (gdp->adp->level < 3)
      blockindex = 2 ;
    else
      blockindex = 3 ;

    insert_constant ((cfp)&(gdp->gdr->frames[firstframe][0]), final_sample(gdp->adp->ccp),
                     gdp->adp->level,  gdp->adp->flip, blockindex, FALSE) ;
  }
  /*
     * return the number of filled data frames
     */
  return ((SHORT)gdp->adp->ccp->frames) ;
}



#ifdef __STDC__
LONG generic_record_samples (gdptype gdp)
#else
     LONG generic_record_samples (gdp)
     gdptype gdp;
#endif
{
  return (gdp->nsamples) ;
}




#ifdef __STDC__
void clear_generic_compression (gdptype gdp, SHORT framesreserved)
#else
     void clear_generic_compression (gdp, framesreserved)
     gdptype gdp ;
     SHORT   framesreserved ;
#endif
{
  /*
   *      framesreserved  :   the number of frames at which to begin inserted data, normally 0, unless
   *                          event/cal...etc blockettes are to be inserted. if this == the number of
   *                          data frames in the record (e.g. 63 for a 4096-byte record), a record with
   *                          no data frames will be generated by compress_generic_record.
       */

  gdp->nsamples = 0 ;
  gdp->adp->ccp->frames = framesreserved ;
  gdp->adp->firstframe = framesreserved ;
}



#ifdef __STDC__
gdptype init_generic_compression (SHORT diff, SHORT fpt, SHORT fpp, SHORT level, SHORT flip, generic_data_record *gdr)
#else
     gdptype init_generic_compression (diff,  fpt,  fpp,  level,  flip,  gdr)
     SHORT diff;
     SHORT fpt;
     SHORT fpp;
     SHORT level;
     SHORT flip;
     generic_data_record *gdr;
#endif
{
  /*
   * allocates and initializes a control structure for compressing a "generic data record" at the
   * requested level.
       *
       * passed:
       *        diff    :   the default differencing to be used, 1 or 2. this is ignored in level 3,
       *                    which adaptively determines the best differencing.
       *        fpt     :   frames per trial. controls the number of frames in a package over which
       *                    differencing adaptivity is performed, typically, say, 8 with fpp=63.
       *        fpp     :   frames per package. the number of data frames (not including header)
       *                    that comprises a full data record, typically 63 for a 4096-byte record
       *                    having one 64-bye header.
       *        level   :   the compression level, 1/2/3.
       *        flip    :   if you want the output data reversed from the native byte order of the
       *                    computer on which you're running.
       *         *gdr   :   the already-allocated data record into which the compressed frames will go.
       *
       * returns:           a pointer to a "generic_data_record_control" structure, partially
       *                    initialized. before use, the caller should use "clear_generic_compression"
       *                    to establish the location of the first data frame and zero the number of
       *                    samples in the complete record.
       */

  gdptype   gdp;

  if ((gdp = (gdptype)malloc(sizeof(*gdp))) == NULL )
    return (NULL) ;

  if ((gdp->adp = init_adaptivity (diff,  fpt,  fpp,  level,  flip)) == NULL )
  {
    free (gdp) ;
    return (NULL) ;
  }
  gdp->gdr = gdr ;
  return (gdp) ;
}


