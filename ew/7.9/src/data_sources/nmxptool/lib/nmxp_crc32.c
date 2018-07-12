/*! \file
 *
 * \brief Computing a 32 bit CRC.
 *
 * $Id: nmxp_crc32.c 3898 2010-03-25 13:25:46Z quintiliani $
 *
 *
 */

#include "nmxp_crc32.h"

void crc32_init_table (uint32_t crc32_tab[256]) {
    unsigned int i, j;
    uint32_t h = 1;
    crc32_tab[0] = 0;
    for (i = 128; i; i >>= 1) {
	h = (h >> 1) ^ ((h & 1) ? POLYNOMIAL : 0);
	/* h is now crc_table[i]*/
	for (j = 0; j < 256; j += 2 * i) {
	    crc32_tab[i + j] = crc32_tab[j] ^ h;
	}
    }
}

uint32_t crc32(uint32_t crc32val, const char *s, uint32_t len)
{
	uint32_t i;
	uint32_t crc32_tab[256];

	crc32_init_table(crc32_tab);

	crc32val ^= 0xffffffff;
	for (i = 0;  i < len;  i ++) {
		crc32val = crc32_tab[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
	}

	return crc32val ^ 0xffffffff;
}

