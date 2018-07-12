/* $Id: stdtypes.h 6803 2016-09-09 06:06:39Z et $ */
/* --------------------------------------------------------------------
  
  Standard explicit data type to enhance portability.

  R. Banfill and D. Chavez

----------------------------------------------------------------------- */
#if !defined _STANDARD_TYPES_DEFINED_
#define _STANDARD_TYPES_DEFINED_

#if defined (X86_16BIT)

/* -------------------------------------------------------------------- */
/* Standard types on x86 hardware with 16 bit compilers, i.e., Microsoft
   Visual C++ 1.52 or Borland C++.  Also RTOS on 68k processors */

 /* Void type */
typedef void VOID;

 /* Characters */
typedef char CHAR;

 /* Boolean values (4 bytes) */
typedef unsigned long BOOL;

 /* Signed and unsigned 8 bit integers */
typedef signed char INT8;
typedef unsigned char UINT8;

 /* 16 bit integer values */
typedef signed short INT16;
typedef unsigned short UINT16;

 /* 32 bit integer values */
typedef signed long INT32;
typedef unsigned long UINT32;

 /* 32 bit IEEE 754 Real */
typedef float REAL32;

 /* 64 bit IEEE 754 Real */
typedef double REAL64;

 /* 80 bit IEEE 754 Real */
typedef long double REAL80;

#elif defined (X86_WIN32)

/* -------------------------------------------------------------------- */
/* Standard types on x86 hardware with Win32 compilers, i.e., Microsoft
   Visual C++ 5.00 */

#include <windows.h>

 /* Void type */
 /* VOID is defined by the Win32 API in windows.h */

 /* Characters */
typedef char CHAR;

 /* Boolean values */
 /* BOOL is defined as unsigned 32 bits by the Win32 API in windows.h */

#pragma warning(disable:4142) // Benign redefinition of type

 /* Signed and unsigned 8 bit integers */
typedef signed __int8 INT8;
typedef unsigned __int8 UINT8;

 /* 16 bit integer values */
typedef signed __int16 INT16;
typedef unsigned __int16 UINT16;

 /* 32 bit integer values */
#if _MSC_VER < 1200
typedef signed __int32 INT32;
typedef unsigned __int32 UINT32;
#endif

 /* 64 bit integers */
typedef signed __int64 INT64;
typedef unsigned __int64 UINT64;

 /* 32 bit IEEE 754 Real */
typedef float REAL32;

 /* 64 bit IEEE 754 Real */
typedef double REAL64;

 /* 80 bit IEEE 754 Real */
typedef long double REAL80;


#elif defined(__x86_64__) || defined(X86_UNIX64)

/* -------------------------------------------------------------------- */
/* 64-bit UNIX -- assume that 'inttypes.h' is available and use it      */
/* -------------------------------------------------------------------- */

#include <inttypes.h>

/* Void type */
#if defined VOID
#   undef VOID
#endif
typedef void VOID;

 /* Characters */
#if defined CHAR
#   undef CHAR
#endif
typedef char CHAR;

 /* Signed and unsigned 8 bit integers */
typedef int8_t INT8;
typedef uint8_t UINT8;

 /* 16 bit integer values */
typedef int16_t INT16;
typedef uint16_t UINT16;

 /* 32 bit integer values */
typedef int32_t INT32;
typedef uint32_t UINT32;

 /* 64 bit integers */
typedef int64_t INT64;
typedef uint64_t UINT64;

 /* 32 bit IEEE 754 Real */
typedef float REAL32;

 /* 64 bit IEEE 754 Real */
typedef double REAL64;

 /* 80 bit IEEE 754 Real */
typedef long double REAL80;

 /* Boolean values */
 /* We make it 32 bits because NT (currently) has BOOL that big and some
  * code exists which looks for offsets using sizeof(BOOL) and that causes
  * problems for data portability.  This will keep things happy until such
  * time as NT changes the size of BOOL.
  */
#if defined BOOL
#   undef BOOL
#endif
typedef UINT32 BOOL;


#elif defined(X86_UNIX32) || defined(SPARC_UNIX32)

/* -------------------------------------------------------------------- */
/* Standard 32 bit UNIX types (both sparc and x86 hardware) */

/* It turns out to be a real bitch to come up with names for the
 * various size specific names that work for all flavors of Unix.
 * Life was great with Solaris 2.6, as it provides <inttypes.h>, but
 * not everybody else does (yet).   So, rather than define things in
 * like INT8 in terms of something like int8_t, which we know will work
 * instead I just do it in terms of things that I know work on the
 * machines I'm currently using.  Not very pretty, and things will
 * likely break in the future.  When they do, see if you've got
 * <inttypes.h> and use it.
 */

/* Void type */
#if defined VOID
#   undef VOID
#endif
typedef void VOID;

 /* Characters */
#if defined CHAR
#   undef CHAR
#endif
typedef char CHAR;

 /* Signed and unsigned 8 bit integers */
typedef signed char INT8;
typedef unsigned char UINT8;

 /* 16 bit integer values */
typedef signed short INT16;
typedef unsigned short UINT16;

 /* 32 bit integer values */
typedef signed int INT32;
typedef unsigned int UINT32;

 /* 64 bit integers */
typedef long long INT64;
typedef unsigned long long UINT64;

 /* 32 bit IEEE 754 Real */
typedef float REAL32;

 /* 64 bit IEEE 754 Real */
typedef double REAL64;

 /* 80 bit IEEE 754 Real */
typedef long double REAL80;

 /* Boolean values */
 /* We make it 32 bits because NT (currently) has BOOL that big and some
  * code exists which looks for offsets using sizeof(BOOL) and that causes
  * problems for data portability.  This will keep things happy until such
  * time as NT changes the size of BOOL.
  */
#if defined BOOL
#   undef BOOL
#endif
typedef UINT32 BOOL;

#else

#    error "can't determine platform!"

#endif

/* Boolean constants -------------------------------------------------- */

#ifndef TRUE
#    define TRUE  ((BOOL) 1)
#endif

#ifndef FALSE
#    define FALSE ((BOOL) 0)
#endif

#endif
