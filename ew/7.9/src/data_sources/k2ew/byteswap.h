/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: byteswap.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/04/03 16:01:04  stefan
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/05/04 23:47:27  lombard
 *     Initial revision
 *
 *
 *
 *
 */
/*   byteswap.h:  Defines byte-swap macros to convert between "Intel" and */
/*                "Motorola" byte ordered integers */

/*     1/6/99 -- [ET]  File started */


#ifdef _INTEL
/*  macro returns byte-swapped version of given 16-bit unsigned integer */
#define BYTESWAP_UINT16(var) \
               ((unsigned short)(((unsigned short)(var)>>(unsigned char)8) + \
               ((unsigned short)(var)<<(unsigned char)8)))

/*  macro returns byte-swapped version of given 32-bit unsigned integer */
#define BYTESWAP_UINT32(var) \
                        ((uint32_t)(((uint32_t)(var)>>(unsigned char)24) + \
               (((uint32_t)(var)>>(unsigned char)8)&(uint32_t)0x0000FF00)+ \
               (((uint32_t)(var)<<(unsigned char)8)&(uint32_t)0x00FF0000)+ \
                                      ((uint32_t)(var)<<(unsigned char)24)))


#endif
#ifdef _SPARC
#define BYTESWAP_UINT16(var) (var)
#define BYTESWAP_UINT32(var) (var)
#endif
