#pragma ident "$Id: stdtypes.h 6114 2014-06-11 17:36:04Z baker $"
/* --------------------------------------------------------------------
 Program  : Any
 Task     : Standard types to enhance portability.
 File     : archive.h
 Purpose  : See task.
 Host     : CC, GCC, Microsoft Visual C++ 5.x, MCC68K 3.1
 Target   : Solaris (Sparc and x86), Linux, DOS, Win32, and RTOS
 Author   : Robert Banfill (r.banfill@reftek.com)
 Company  : Refraction Technology, Inc.
            2626 Lombardy Lane, Suite 105
            Dallas, Texas  75220  USA
            (214) 353-0609 Voice, (214) 353-9659 Fax, info@reftek.com
 Copyright: (c) 1997 Refraction Technology, Inc. - All Rights Reserved.
 Notes    :
 $Revision: 6114 $
 $Logfile : R:/cpu68000/rt422/struct/version.h_v  $
 Revised  :
      17Jan94  ---- (RLB) Initial code.
      18May98  ---- (RLB) Cleaned up and added Win32 stuff.
      18May98  ---- (DEC) Added unix stuff
      23Jun98  ---- (RLB) Made all integer types sign specific

----------------------------------------------------------------------- */

#ifndef _STD_TYPES_
#define _STD_TYPES_

typedef int INT;

#if defined(X86_16BIT) || defined(M68K_32BIT)

/* -------------------------------------------------------------------- */
/* Standard types on x86 hardware with 16 bit compilers, i.e., Microsoft
   Visual C++ 1.52 or Borland C++.  Also RTOS on 68k processors */

 /* Void type */
typedef void VOID;

 /* Characters */
typedef char CHAR;

 /* Boolean values */
typedef unsigned char BOOL;

 /* Signed and unsigned 8 bit integers */
typedef char BYTE;
typedef signed char SBYTE;
typedef unsigned char UBYTE;

 /* 16 bit integer values */
typedef unsigned short WORD;
typedef short SWORD;
typedef unsigned short UWORD;

 /* 32 bit integer values */
typedef signed long LONG;
typedef unsigned long ULONG;
typedef signed long SLONG;

 /* 32 bit IEEE 754 Real */
typedef float FLOAT;

 /* 64 bit IEEE 754 Real */
typedef double DOUBLE;

 /* 80 bit IEEE 754 Real */
typedef double LDOUBLE;

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
 /* BOOL is defined by the Win32 API in windows.h */

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

#elif defined(X86_UNIX32) || defined(SPARC_UNIX32)

/* -------------------------------------------------------------------- */
/* Standard 32 bit UNIX types (both sparc and x86 hardware)

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
typedef void VOID;

 /* Characters */
typedef char CHAR;

 /* Signed and unsigned 8 bit integers */
typedef signed char INT8;
typedef unsigned char UINT8;

 /* 16 bit integer values */
typedef signed short INT16;
typedef unsigned short UINT16;

 /* 32 bit integer values */
typedef signed long INT32;
typedef unsigned long UINT32;

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
typedef INT32 BOOL;

#elif defined(X86_UNIX64)

 /* Void type */
typedef void VOID;

 /* Characters */
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
typedef signed long INT64;
typedef unsigned long UINT64;

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
typedef INT32 BOOL;

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

/* Revision History
 *
 * $Log$
 * Revision 1.6  2008/08/26 00:23:31  rwg
 * fix build on x86_64...?
 *
 * Revision 1.5  2006/08/04 16:05:50  paulf
 * 1.7 reftek2ew changes from Roberta Stavely of Reftek Inc
 *
 * Revision 1.4  2002/11/04 22:42:37  lombard
 * Removed DOS end-of-line chars for use on unix.
 *
 * Revision 1.3  2002/11/04 21:40:53  alex
 * *** empty log message ***
 *
 * Revision 1.1.1.1  2000/06/22 19:13:09  nobody
 * Import existing sources into CVS
 *
 */
