/*! \file
 *
 * \brief Computing a 32 bit CRC.
 *
 * $Id: nmxp_crc32.h 3898 2010-03-25 13:25:46Z quintiliani $
 *
 */

#ifndef NMXP_CRC32_H
#define NMXP_CRC32_H 1

#include <stdint.h>

#define POLYNOMIAL (uint32_t)0xedb88320

/*! \brief Computes a 32 bit crc of the data in the buffer,
 * and returns the crc.  the polynomial used is 0xedb88320. */
uint32_t crc32(uint32_t crc32val, const char *buf, uint32_t len);

#endif /* CRC32_H */


