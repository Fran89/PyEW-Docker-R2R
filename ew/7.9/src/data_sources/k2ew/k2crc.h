/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2crc.h 90 2000-05-04 23:48:43Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/05/04 23:48:05  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2crc.h:  Macro for computing K2 CRCs used in Serial Data Stream */
/*            packets; uses 'g_k2crc_table[]' from "k2crctbl.c" */
/*  */
/*    1/6/99 -- [ET] */
/*  */


/*  K2_CRC:  Macro called iteratively with each byte of data frame */
/*     wcrc - 16-bit unsigned integer variable for ongoing CRC value (should */
/*            be initialized to zero before first use of macro on data frame) */
/*     bt   - byte value of data frame to be entered in CRC calculation */

#define K2_CRC(wcrc,bt) \
       (wcrc=(unsigned short)((unsigned short)((wcrc)<<(unsigned char)8)^ \
       (g_k2crc_table[(unsigned char)(((wcrc)>>(unsigned char)8)^ \
       (unsigned char)(bt))])))


extern unsigned short g_k2crc_table[];         /* reference to global CRC table */

