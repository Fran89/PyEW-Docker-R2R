#ifndef __Q3302EW_PLATFORM_H__
#define __Q3302EW_PLATFORM_H__

#include <sys/types.h>

#ifdef _linux
#include <stdint.h>
#endif

#ifdef WIN32
#define int8 __int8
#define int16 __int16
#define int32 __int32
#define int64 __int64

#define uint8 unsigned __int8
#define uint16 unsigned __int16
#define uint32 unsigned __int32
#define uint64 unsigned __int64
#else

#define int8 int8_t
#define int16 int16_t
#define int32 int32_t
#define int64 int64_t

#define uint8 unsigned int8
#define uint16 unsigned int16
#define uint32 unsigned int32
#define uint64 uint64_t
#endif

#endif
