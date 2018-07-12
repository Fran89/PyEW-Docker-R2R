/*
 * crc32.h  CRC used by Nanometrcis
 */

#ifndef _CRC32_H
#define _CRC32_H

#include <platform.h>

void crc32_init( void );
void crc32_update( unsigned char *blk_adr, uint32_t blk_len );
uint32_t crc32_value( void );

#endif 
