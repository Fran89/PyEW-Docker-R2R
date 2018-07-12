// FILE: SrDefines.h
// COPYRIGHT: (c), Symmetric Research, 2009
//
// SR defines use in most SR C include files and function libraries.
//
// These defines help shield the rest of the code from variations between
// different OS's.
//



#ifndef __SRDEFINES_H__
#define __SRDEFINES_H__ (1)


// FUNCTION TYPE DEFINES:
//
// Most SR application software links in static copies of the helper and
// product libraries it uses.  For static linking, function and global
// variable declarations are easy and standard across operating systems.
// The following defines default to static linking if not otherwise
// specified.
//
// Supporting Windows dynamic DLL libraries introduces complications.  The
// following ifdef defines allow for one product function library source to
// service either statically linked or DLL applications.
//
// Linux shared object libraries are also supported but do not require any
// compile time specificatons beyond static.
//


#if defined( SROS_MSDOS )  // DOS requires simple declarations
#define FUNCTYPE( type )        type
#define VARTYPE( type )         type

#elif defined( _WIN_DLLEXPORT )
#define FUNCTYPE( type )        __declspec( dllexport ) type __stdcall
#define VARTYPE( type )         __declspec( dllexport ) type

#elif defined( _WIN_DLLIMPORT )
#define FUNCTYPE( type )        __declspec( dllimport ) type __stdcall
#define VARTYPE( type )         __declspec( dllimport ) type

#else           // default to simple declarations for static linking
#define FUNCTYPE( type )        type
#define VARTYPE( type )         type

#endif



// DEVICE HANDLE AND MISCELLANEOUS OS DEPENDENT DEFINES:
//
// The various OS's use different data types to keep track of the current
// kernel mode device driver which is used when communicating with product
// hardware.  These defines helps hide this difference from the rest of the
// code.  The allowed SROS_xxxxx options are:
//
// SROS_WIN2K  Windows 2000
// SROS_WINXP  Windows XP
// SROS_LINUX  Linux kernel 2.6.25 = Fedora 9
//


#if defined( SROS_WINXP ) || defined( SROS_WIN2K )
#define DEVHANDLE void *

#elif defined( SROS_LINUX )
#define DEVHANDLE int

#else
#pragma message( "COMPILE ERROR: SROS_xxxxx MUST BE DEFINED !! " )

#endif  // SROS_WINXP SROS_WIN2K  SROS_LINUX

#define BAD_DEVHANDLE ((DEVHANDLE)(-1))



// 64 BIT SUPPORT:
//
// Support for 64-bit values varies from one operating system to
// another.  To compensate, a SR64BIT structure is defined.
//


#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )
typedef union _SR64BIT {
    struct {
        unsigned long LowPart;
        long          HighPart;
    } u;
    __int64 QuadPart; // this is a LONGLONG
} SR64BIT;


#elif defined( SROS_WIN95 ) || defined( SROS_MSDOS )
typedef union _SR64BIT {
    struct {
        unsigned long LowPart;
        long          HighPart;
    } u;
//    LONGLONG QuadPart;
} SR64BIT;


#elif defined( SROS_LINUX )
typedef union _SR64BIT {
    struct {
        unsigned long LowPart;
        long          HighPart;
    } u;
    long long int QuadPart;
} SR64BIT;

#endif  // SROS_xxxxx

//FIX - these are currently needed in the largeint type functions in SrHelper.c,
//      should they be defined here or in srdat ?
//      or should the largeint type helper functions be moved to SrUsbXch.c ?

#define SR_COUNTER_UNKNOWN          -1
#define SR_COUNTER_INT64             0       // Standard 64 bit int
#define SR_COUNTER_HIGH32LOW32       1       // Linux timeval structure
#define SR_COUNTER_OTHER             2
#define SR_COUNTER_MAX               3





// Slash and Enter defines

#if defined( SROS_LINUX )
#define SR_DIRSLASH '/'
#define SR_QENTER "q+Enter"
#define SR_ENTERKEY 0x0A
#else
#define SR_DIRSLASH '\\'
#define SR_QENTER "q"
#define SR_ENTERKEY 0x0D
#endif


// General time defines

#define SR_SECPERMIN       60  // seconds / minute
#define SR_SECPERHR      3600  // 60 sec/min * 60 min/hr = 3600 sec/hr
#define SR_SECPERDAY    86400L // 24 hr/day * 60 min/hr * 60 sec/min = 86400 sec/day
#define SR_NSUPERSEC 10000000L // 100ns units / second
#define SR_USPERSEC   1000000L // microseconds / second
#define SR_USPERMS       1000L // microseconds / millisecond
#define SR_MSPERSEC      1000L // milliseconds / second





#endif // __SRDEFINES_H__
