// FILE: SrHelper.h
// COPYRIGHT: (c), Symmetric Research, 2009
//
// SR helper functions C user library include file.
//
// These helper functions a used to shield the rest of the code from
// variations between different OS's.
//



#include "SrDefines.h" // for function type, devhandle, 64 bit, + other defines


#ifndef __SRHELPER_H__
#define __SRHELPER_H__ (1)



// GENERIC HELPER FUNCTIONS:
//
// These functions perform a few generic keyboard and file IO tasks and
// are used in various SR application programs to help smooth over OS
// differences.
//


FUNCTYPE( int )  SrHelperRev( int* Rev );
FUNCTYPE( int  ) SrKeyboardNonBlock( void );
FUNCTYPE( int  ) SrGetKeyCheck( void );
FUNCTYPE( int  ) SrGetKeyWait(  void );
FUNCTYPE( void ) SrFlushPrint( void );
FUNCTYPE( int  ) SrSleep( int ms );
FUNCTYPE( int  ) SrWait( int ms );
FUNCTYPE( int )  SrMkDir( char *DirectoryName );
FUNCTYPE( int  ) SrFileExist( const char *filename );
FUNCTYPE( int  ) SrFileReadable( const char *filename );
FUNCTYPE( long ) SrFileLength( FILE *fp );
FUNCTYPE( long ) SrFileModTime( FILE *fp );
FUNCTYPE( int  ) SrStringCopy( char *DestStr, const char *SrcStr, int MaxChar );
FUNCTYPE( int  ) SrStringCmp( char *TrialStr, const char *TargetStr, int MaxChar );
FUNCTYPE( int )  SrGetLine( char *DestStr, int MaxChar );
FUNCTYPE( int  ) SrGetPcTime( double *Time,
                              int *Year, int *Month, int *Day, 
                              int *Hour, int *Minute, int *Second,
                              long *Microsecond );
FUNCTYPE( int  ) SrSetPcTime( double Time,
                              int Year, int Month, int Day, 
                              int Hour, int Minute, int Second,
                              long Microsecond, int FromYMD );
FUNCTYPE( int  ) SrSecTimeSplit( double Time,
                                 int *Year, int *Month, int *Day,
                                 int *Hour, int *Minute, int *Second, 
                                 long *Microsecond );
FUNCTYPE( int  ) SrSecTimeCombine( double *Time,
                                   int Year, int Month, int Day,
                                   int Hour, int Minute, int Second, 
                                   long MicroSecond );
FUNCTYPE( int  ) SrCalcTimeZoneCorr( long StartTime );

FUNCTYPE( int     ) SrLargeIntDiff( SR64BIT A, SR64BIT B, SR64BIT *C, double *C2 );
FUNCTYPE( SR64BIT ) SrLargeIntAdd( SR64BIT A, long More );
FUNCTYPE( int     ) SrLargeIntDiffType( SR64BIT A, SR64BIT B, int Type, SR64BIT *C, double *C2 );
FUNCTYPE( SR64BIT ) SrLargeIntAddType( SR64BIT A, long More, int Type );

#endif // __SRHELPER_H__
