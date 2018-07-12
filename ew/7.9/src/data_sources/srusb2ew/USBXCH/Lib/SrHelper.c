// FILE: SrHelper.c
// COPYRIGHT: (c), Symmetric Research, 2009
//
// These are the SR user library helper functions.  They are mostly
// wrappers around some utility functions that differ between various OS's.
//
//

#include <stdio.h>       // fileno function, etc
#include <time.h>        // time, gmtime, mktime function, etc
#include <sys/stat.h>    // fstat function  (VC++ can use / in path)


// OS dependent includes ...

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )
#include <windows.h>     // windef.h -> winnt.h -> defines GENERIC_READ etc
#include <io.h>          // access function
#include <conio.h>       // kbhit, getch functions
#include <direct.h>      // mkdir function
#define OsAccess _access
#define OsFileno _fileno


#elif defined( SROS_LINUX )
#include <unistd.h>      // access function and STDIN_FILENO
#include <fcntl.h>       // O_xxx macros
#include <string.h>      // for strncopy, strncmp, strcmp
#include <ctype.h>       // for toupper
#include <sys/time.h>    // for gettimeofday
#define OsAccess access
#define OsFileno fileno


#elif defined( SROS_WIN9X )
#include <windows.h>     // windef.h -> winnt.h -> defines GENERIC_READ etc
#include <stdlib.h>      // sleep function now from windows.h
#include <io.h>          // access function
#include <conio.h>       // kbhit, getch functions
#include <direct.h>      // mkdir function
#define OsAccess _access
#define OsFileno _fileno


#elif defined( SROS_MSDOS )
#include <io.h>          // access function
#include <conio.h>       // kbhit, getch functions
#include <direct.h>      // mkdir function
#include <sys\timeb.h>   // ftime function
#define OsAccess _access
#define OsFileno _fileno


#endif  // SROS_xxxxx


#if !defined( SRLOCAL )
#define SRLOCAL static
#endif


#include "SrDefines.h"
#include "SrHelper.h"



// Prototypes for OS dependent helper functions

SRLOCAL int  OsSetNonBlock( int value );
SRLOCAL int  OsGetKey( int wait );
SRLOCAL void OsFlushPrint( void );
SRLOCAL int  OsGetPcTime( double *Time,
                          int *Year, int *Month, int *Day, 
                          int *Hour, int *Minute, int *Second,
                          long *Microsecond );
SRLOCAL int  OsSetPcTime( double Time,
                          int Year, int Month, int Day, 
                          int Hour, int Minute, int Second,
                          long Microsecond, int FromYMD );
SRLOCAL int  OsSleep( int ms, int hardwait );
SRLOCAL int  OsMkDir( char *FileName );




//------------------------------------------------------------------------------
// ROUTINE: SrHelperRev
// PURPOSE: Allows the user to determine the library rev they are working with.
//
// Short rev history:
//
//  SRHELPER_REV  101  ( 10/15/2003 )  < Changed SrFileExist return, added Rev fn
//  SRHELPER_REV  102  ( 02/25/2006 )  < SrFileLength to take FILE*, remove SrCommit
//  SRHELPER_REV  103  ( 03/20/2006 )  < Added SrFileReadable, SrFileModTime
//          
//------------------------------------------------------------------------------

#define SRHELPER_REV      103

FUNCTYPE( int )  SrHelperRev( int* Rev ) {
        
        *Rev = SRHELPER_REV;
        return( *Rev );
}


//------------------------------------------------------------------------------
// ROUTINE: SrKeyboardNonBlock
// PURPOSE: Ensures non-blocking mode is enabled, allowing SrGetKeyCheck
//          to return even if a key has not been pressed.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrKeyboardNonBlock( void ) {
        return( OsSetNonBlock( 1 ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrGetKeyCheck
// PURPOSE: Check for an available keypress character.  If one is there, it is
//          read and returned.  Otherwise, EOF is returned.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrGetKeyCheck( void ) {
        return( OsGetKey( 0 ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrGetKeyWait
// PURPOSE: Wait until a keypress character is ready and then return it.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrGetKeyWait( void ) {
        return( OsGetKey( 1 ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrFlushPrint
// PURPOSE: Ensure the last printf string is displayed, even if it didn't end
//          with a new line (\n) character.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrFlushPrint( void ) {
        OsFlushPrint();
}

//------------------------------------------------------------------------------
// ROUTINE: SrSleep
// PURPOSE: Put the calling program to sleep for the specified number of
//          milliseconds.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrSleep( int ms ) {
        return( OsSleep( ms, 0 ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrWait
// PURPOSE: Wait for the specified number of milliseconds usually by putting
//          the calling program to sleep.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrWait( int ms ) {
        return( OsSleep( ms, 1 ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrMkDir
// PURPOSE: Make a new directory with the specified name and full permissions 
//          (if applicable).
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrMkDir( char *DirectoryName ) {
	return( OsMkDir( DirectoryName ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrFileExist
// PURPOSE: Test to see if the specified file exists.
//          The return value changed as of 10/15/2003.  
//          Now it is 1 when the file exists and 0 otherwise.
//------------------------------------------------------------------------------

#if !defined F_OK
#define F_OK 0x00
#endif

FUNCTYPE( int )  SrFileExist( const char *FileName ) {
        if ( OsAccess( FileName, F_OK ) == 0 )
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrFileReadable
// PURPOSE: Test to see if the specified file is readable.
//          It returns 1 when the file can be read and 0 otherwise.
//------------------------------------------------------------------------------

#if !defined R_OK
#define R_OK 0x04
#endif

FUNCTYPE( int )  SrFileReadable( const char *FileName ) {
        if ( OsAccess( FileName, R_OK ) == 0 )
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrFileLength
// PURPOSE: Returns the length of the specified file in bytes.
//------------------------------------------------------------------------------
FUNCTYPE( long ) SrFileLength( FILE *fp ) {

        struct stat buffer;

        if ( fstat( OsFileno( fp ), &buffer ) == -1 ) {
                printf( "Could not get file stats\n" );
                return( -1 );
                }

        return( buffer.st_size );
}

//------------------------------------------------------------------------------
// ROUTINE: SrFileModTime
// PURPOSE: Returns the last modification time of the specified file.
//------------------------------------------------------------------------------
FUNCTYPE( long ) SrFileModTime( FILE *fp ) {

        struct stat buffer;

        if ( fstat( OsFileno( fp ), &buffer ) == -1 ) {
                printf( "Could not get file stats\n" );
                return( -1 );
                }

        return( (long)buffer.st_mtime );
}

//------------------------------------------------------------------------------
// ROUTINE: SrStringCopy
// PURPOSE: This function copies up to MaxChar-1 characters from SrcStr to 
//          DestStr, ensuring that the last character in the destination string
//          is a null.  It returns the total number of characters in the
//          destination string, including the terminating null.
//          *** Be sure MaxChar is <= the dimension of DestStr. ***
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrStringCopy( char *DestStr, const char *SrcStr, int MaxChar ) {

        int i;

        // Copy characters from SrcStr to DestStr, stopping after
        // MaxChar-1 characters or a null at the end of the source
        // string is reached.

        i = 1;
        while ( (i < MaxChar) && (*DestStr++ = *SrcStr++) )
                i++;


        // If the copying stopped because of hitting the character
        // limit, fill the last character with null (if the copy stopped
        // because of reaching the end of the source string, the last
        // character is already null).

        if ( i == MaxChar )
                *DestStr = '\0';

        return( i );
}

//------------------------------------------------------------------------------
// ROUTINE: SrStringCmp
// PURPOSE: This function does a case insensitive compare between TrialStr and
//          TargetStr for up to MaxChar characters.  It returns 1 if Trial > 
//          Target, -1 if Trial < Target, and 0 if the strings are equal.
//          MaxChar is the dimension of the smallest string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrStringCmp( char *TrialStr, const char *TargetStr, int MaxChar ) {

        int i, cha, chb, na, nb, nlimit;

        // Get string lengths (including terminating null)

        i  = 0;
        na = strlen( TrialStr )  + 1;
        nb = strlen( TargetStr ) + 1;

        // Limit characters to check to the smallest length

        nlimit = MaxChar;
        if (nlimit > na )
                nlimit = na;
        if (nlimit > nb )
                nlimit = nb;

        // Process all characters up to the limit
        
        while ( i < nlimit ) {

                // Convert current character from each string to upper case

                cha = toupper( TrialStr[i] );
                chb = toupper( TargetStr[i] );


                // Compare them

                if ( cha > chb )        // Trial > Target
                        return( 1 );

                else if ( cha < chb )   // Trial < Target
                        return( -1 );

                else                    // equal so far
                        i++;            // go to next char
                }


        // Strings are equal

        return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrGetLine
// PURPOSE: This function reads characters from stdin until a newline ('\n')
//          character is entered.  It copies up to MaxChar-1 of these
//          characters into DestStr and throws away the rest.  It also
//          throws away the newline and ensures the last character in the
//          destination string is a null.  It returns the total number of
//          characters in the destination string, including the terminating null.
//          *** Be sure MaxChar is <= the dimension of DestStr. ***
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrGetLine( char *DestStr, int MaxChar ) {

        int c, i;

        // Error check

        if (!DestStr)
                return( 0 );

        
        // Begin reading characters looking for a newline

        i = 0;
        c = EOF;
        
        while ( c != '\n' ) {

                // Read until a key is pressed

                c = EOF;
                while ( c == EOF )
                        c = getchar();

                // If there is room, save character 

                if ( i < MaxChar-1 ) {
                        DestStr[i] = c;
                        i++;

                        // If newline, decrement i so we overwrite it

                        if (c == '\n')
                                i--;
                        }
                }

        DestStr[i] = '\0';

        return( i+1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrGetPcTime
// PURPOSE: Return the current PC system time as YMD HMS and as seconds since
//          1970.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrGetPcTime( double *Time,
                             int *Year, int *Month, int *Day, 
                             int *Hour, int *Minute, int *Second,
                             long *Microsecond ) {
        
        return( OsGetPcTime( Time, Year, Month, Day, 
                             Hour, Minute, Second, Microsecond ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrSetPcTime
// PURPOSE: Set the current PC system time.
//
//          This function returns 1 for success, 0 for failure.
//          FromYMD is 1 when the time to set is taken from the Year, Month, etc
//          values and and 0 when it is taken from the Time in seconds since
//          1970.  The underlying Windows function uses YMD, while the underlying
//          Linux function uses Time.  But both work with either setting.
//
//          NOTE: You MUST have the proper permissions to successfully set
//                the PC time.  Typically this means being an Administrator
//                or Power User under Windows and being root or running
//                "set uid" root under Linux.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrSetPcTime( double Time,
                             int Year, int Month, int Day, 
                             int Hour, int Minute, int Second,
                             long Microsecond, int FromYMD ) {
        
        return( OsSetPcTime( Time, Year, Month, Day, 
                             Hour, Minute, Second, Microsecond, FromYMD ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrSecTimeSplit
// PURPOSE: Convert time represented as seconds since 1970 into YMD HMS.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrSecTimeSplit( double Time,
                                int *Year, int *Month, int *Day,
                                int *Hour, int *Minute, int *Second, 
                                long *Microsecond ) {
        time_t    ttTime;
        struct tm tmTime;
        double    usec;


        // Split time into pieces (for UTC)

        ttTime = (time_t)Time;
        tmTime = *gmtime( &ttTime );
        usec   = Time - (double)ttTime + 0.0000005;
        
        // Save pieces
        
        if (Year)         *Year        = tmTime.tm_year + 1900;
        if (Month)        *Month       = tmTime.tm_mon + 1;
        if (Day)          *Day         = tmTime.tm_mday;
        if (Hour)         *Hour        = tmTime.tm_hour;
        if (Minute)       *Minute      = tmTime.tm_min;
        if (Second)       *Second      = tmTime.tm_sec;
        if (Microsecond)  *Microsecond = (long)(usec * SR_USPERSEC);

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrSecTimeCombine
// PURPOSE: Convert time represented as YMD HMS into seconds since 1970.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrSecTimeCombine( double *Time,
                                  int Year, int Month, int Day,
                                  int Hour, int Minute, int Second, 
                                  long MicroSecond ) {

        time_t    ttTime;
        struct tm tmTime;
        long      lTime;

        // Set default YMD values when 0's are passed in

        if (Year == 0)   Year  = 1970;
        if (Month == 0)  Month = 1;
        if (Day == 0)    Day   = 1;


        // Fill pieces
        
        tmTime.tm_year  = Year - 1900;
        tmTime.tm_mon   = Month - 1;
        tmTime.tm_mday  = Day;
        tmTime.tm_hour  = Hour;
        tmTime.tm_min   = Minute;
        tmTime.tm_sec   = Second;
        tmTime.tm_wday  = 0;
        tmTime.tm_yday  = 0;
        tmTime.tm_isdst = -1;


        // Combine time from GMT pieces to epoch seconds using local correction,
        // then use the zone offset to remove the local correction

        ttTime = mktime( &tmTime );
        if (ttTime == (time_t)-1)
                return( 0 );
        lTime = (long)ttTime; // WCT - beware 64 bit time
        ttTime -= SrCalcTimeZoneCorr( lTime );
        if (Time)  *Time  = (double)ttTime + (double)MicroSecond / (double)SR_USPERSEC;
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrCalcTimeZoneCorr
// PURPOSE: Calculate the correction between the local time zone and UTC time.
//
//          This function does two conversions between time stored as a structure
//          and time as seconds since the epoch (1970-01-01).  They should cancel
//          out, but the first is done using GMT time and the second is done using
//          local time.  The difference gives us the amount of the local correction.
//          Subtract this value from a local time to get a GMT time.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrCalcTimeZoneCorr( long StartTime ) {
        
        time_t    TargetTime, NewTime;
        struct tm tm;
        int       ZoneCorrection;

        
        // Convert epoch seconds to time structure for gmt
        
        TargetTime = (time_t)StartTime;
        tm = *gmtime( &TargetTime );

        
        // Convert time structure for local to epoch seconds (force DST unknown)
        
        tm.tm_isdst = -1;
        NewTime = mktime( &tm );


        // Difference is local correction

        ZoneCorrection = (int)(NewTime - TargetTime); // WCT - beware 64 bit time
        return( ZoneCorrection );
}


//******************* SR64BIT LARGE INTEGER HELPER FUNCTIONS ******************

// Functions for subtracting two SR64BIT values and for adding to a
// SR64BIT value.  The computation varies depending on what type of
// data is stored in the SR64BIT variable.

//------------------------------------------------------------------------------
// ROUTINE: SrLargeIntDiff
// PURPOSE: Subtract 2 64 bit numbers C = A - B.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrLargeIntDiff( SR64BIT A, SR64BIT B, SR64BIT *C, double *C2 ) {

        SR64BIT Result;
        double  Answer;


        // Now subtract standard large ints

        Result.QuadPart = (A.QuadPart - B.QuadPart);
        Answer          = (double)(Result.QuadPart);



        // Fill the users results variables

        if (C)   C->QuadPart = Result.QuadPart;
        if (C2) *C2          = Answer;

        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrLargeIntAdd
// PURPOSE: Add a long to a 64 bit numbers C = A + long.
//------------------------------------------------------------------------------
FUNCTYPE( SR64BIT ) SrLargeIntAdd( SR64BIT A, long More ) {

        SR64BIT C;

        C.QuadPart = A.QuadPart + More;

        return( C );
}

//------------------------------------------------------------------------------
// ROUTINE: SrLargeIntDiffType
// PURPOSE: Subtract 2 64 bit numbers C = A - B, but compensate for an older
//          style of representing a number in 64 bits.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrLargeIntDiffType( SR64BIT A, SR64BIT B, int Type,
                                       SR64BIT *C, double *C2 ) {

        long    Seconds, Usec;
        SR64BIT Result;
        double  Answer;


        // If Linux timeval style large ints, convert into standard large ints

        if ( Type == SR_COUNTER_HIGH32LOW32 ) {

                Seconds     = A.u.HighPart;
                Usec        = A.u.LowPart;
                A.QuadPart  = Seconds;
                A.QuadPart *= SR_USPERSEC;
                A.QuadPart += Usec;

                Seconds     = B.u.HighPart;
                Usec        = B.u.LowPart;
                B.QuadPart  = Seconds;
                B.QuadPart *= SR_USPERSEC;
                B.QuadPart += Usec;

                }

        // Now subtract standard large ints

        Result.QuadPart = (A.QuadPart - B.QuadPart);
        Answer          = (double)(Result.QuadPart);



        // Fill the users results variables

        if (C)   C->QuadPart = Result.QuadPart;
        if (C2) *C2          = Answer;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrLargeIntAddType
// PURPOSE: Add a long to a 64 bit numbers C = A + long, but compensate for an
//          older style of representing a number in 64 bits.
//------------------------------------------------------------------------------
FUNCTYPE( SR64BIT ) SrLargeIntAddType( SR64BIT A, long More, int Type ) {

        SR64BIT C;

        // Windows style large ints

        if (Type == SR_COUNTER_INT64) {
                C.QuadPart = A.QuadPart + More;
                }

        else { // Type == SR_COUNTER_HIGH32LOW32
                C.u.HighPart = A.u.HighPart;         // seconds part

                if ( More >= 0 ) {
                        while ( More > SR_USPERSEC ) {
                                C.u.HighPart++;
                                More -= SR_USPERSEC;
                                }
                        C.u.LowPart  = A.u.LowPart + More;   // usec part
                        while ( C.u.LowPart >= SR_USPERSEC) {
                                C.u.HighPart++;
                                C.u.LowPart -= SR_USPERSEC;
                                }
                        }
                else { // ( More < 0 )
                        while ( More < -SR_USPERSEC ) {
                                C.u.HighPart--;
                                More += SR_USPERSEC;
                                }
                        while ( (long)A.u.LowPart < -More ) {
                                C.u.HighPart--;
                                A.u.LowPart += SR_USPERSEC;
                                }
                        C.u.LowPart  = A.u.LowPart + More;   // usec part
                        }
                }

        return( C );
}








//------------------------------------------------------------------------------
// OS DEPENDENT HELPER FUNCTIONS:
//
// These functions help hide OS dependencies from the rest of
// the library by providing a common interface.  The functions
// are repeated once for each supported OS, but the SROS_xxxxx
// defines select only one set to be compiled in to the code.
// 
// Supported OS's include
//
//     SROS_WINXP  Windows XP
//     SROS_WIN2K  Windows 2000
//     SROS_WIN9X  Windows 95, 98, and ME
//     SROS_MSDOS  MS Dos
//     SROS_LINUX  Linux 2.6.18-1.2798.fc6 kernel (Fedora Core 6)
//
//
//------------------------------------------------------------------------------


#if defined( SROS_WINXP ) || defined( SROS_WIN2K )

//------------------------------------------------------------------------------
// ROUTINE: OsSetNonBlock (Windows version)
// PURPOSE: Sets non-blocking mode which allows testing for a keypress.
//------------------------------------------------------------------------------
SRLOCAL int OsSetNonBlock( int value ) {

        // Win2K/XP is always in non-blocking mode (value=1)
        //   value = 0 blocks reads until a character is ready
        //   value = 1 allow reads to return immediately
        //   returns -1 for failure, 0 for success

        if (value == 1)
                return( 0 );
        else
                return( -1 );
}

//------------------------------------------------------------------------------
// ROUTINE: OsGetKey (Windows version)
// PURPOSE: Test for a keypress and return it if one is ready.  The wait
//          argument indicates what to do if a keypress is not immediately
//          available.  If wait is true, then wait until one is ready.
//          Otherwise, just return right away.
//------------------------------------------------------------------------------
SRLOCAL int OsGetKey( int wait ) {

        int c;

        // Get a ready keypress or wait until one is ready if requested.

        c = EOF;
        if ( _kbhit() || wait ) {
                c = _getch();
                if (c == 0xD) printf("\n");  // add newline if char was ENTER
                if (c == 0x00 || c == 0xE0 ) // deal with extended key (fn,->)
                        c = _getch();         // c has scan code
                }
        
        return( c );
}

//------------------------------------------------------------------------------
// ROUTINE: OsFlushPrint (Windows version)
// PURPOSE: Ensures that printf statements that don't end with a carriage return
//          (ie \n) are output immediately rather then buffered for later output.
//------------------------------------------------------------------------------
SRLOCAL void OsFlushPrint( void ) {
        
        // Win2K/XP automatically flushes printf.
}

//------------------------------------------------------------------------------
// ROUTINE: OsGetPcTime (Windows version)
// PURPOSE: Get the PC system time and return it as time specified as seconds
//          since 1970 and as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int OsGetPcTime( double *Time,
                         int *Year, int *Month, int *Day, 
                         int *Hour, int *Minute, int *Second,
                         long *Microsecond ) {
        
        SYSTEMTIME Systim;
        long       Usec;


        // Get current system time

        GetSystemTime( &Systim );
        Usec = ((long)Systim.wMilliseconds) * SR_USPERMS;

        
        // Return time as YMD HMS for UTC (not local)

        if (Time) {
                SrSecTimeCombine( Time, 
                                  Systim.wYear,
                                  Systim.wMonth,
                                  Systim.wDay,
                                  Systim.wHour,
                                  Systim.wMinute,
                                  Systim.wSecond,
                                  Usec );
                }
        
        if (Year)         *Year        = Systim.wYear;
        if (Month)        *Month       = Systim.wMonth;
        if (Day)          *Day         = Systim.wDay;
        if (Hour)         *Hour        = Systim.wHour;
        if (Minute)       *Minute      = Systim.wMinute;
        if (Second)       *Second      = Systim.wSecond;
        if (Microsecond)  *Microsecond = Usec;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: OsSetPcTime (Windows version)
// PURPOSE: Set the PC system time from the supplied time specified as seconds
//          since 1970 or as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int OsSetPcTime( double Time,
                         int Year, int Month, int Day, 
                         int Hour, int Minute, int Second,
                         long Microsecond, int FromYMD ) {

        SYSTEMTIME Systim;
        int        Yr, Mo, Dy, Hh, Mm, Ss;
        long       Usec;


        // Prepare time in SYSTEMTIME format Windows prefers

        if (FromYMD == 1) {
                Systim.wYear         = Year;
                Systim.wMonth        = Month;
                Systim.wDay          = Day;
                Systim.wHour         = Hour;
                Systim.wMinute       = Minute;
                Systim.wSecond       = Second;
                Systim.wMilliseconds = (int)(Microsecond / SR_USPERMS);
                }
        else {
                SrSecTimeSplit( Time, &Yr, &Mo, &Dy, &Hh, &Mm, &Ss, &Usec );
                Systim.wYear         = Yr;
                Systim.wMonth        = Mo;
                Systim.wDay          = Dy;
                Systim.wHour         = Hh;
                Systim.wMinute       = Mm;
                Systim.wSecond       = Ss;
                Systim.wMilliseconds = (int)(Usec / SR_USPERMS);
                }
        
        Systim.wDayOfWeek    = 7; // this invalid number is ignored

        if (SetSystemTime( &Systim ) == 0)
                return( 0 );
        else 
                return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: OsSleep (Windows version)
// PURPOSE: Sleeps for the specified number of milliseconds.  This usually 
//          allows other processes access to the CPU.  The DOS version just
//          returns immediately unless the hardwait parameter is true.  In which
//          case it busy waits instead.  The return value is 0 for success and
//          -1 for error.
//------------------------------------------------------------------------------
SRLOCAL int OsSleep( int ms, int hardwait ) {

        // Sleep for specifed number of milliseconds.
        

        if ( ms <= 0 )
                return( 0 );
        
        Sleep( ms );
        return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: OsMkDir (Windows version)
// PURPOSE: Make a new directory with the specified name.
//------------------------------------------------------------------------------
SRLOCAL int OsMkDir( char *DirectoryName ) {
	return( _mkdir( DirectoryName ) );
}


#elif defined( SROS_LINUX )

//------------------------------------------------------------------------------
// ROUTINE: OsSetNonBlock (Linux version)
// PURPOSE: Sets non-blocking mode which allows testing for a keypress.
//------------------------------------------------------------------------------
SRLOCAL int OsSetNonBlock( int value ) {

        // Set non-blocking mode for keyboard reads
        //   value = 0 blocks reads until a character is ready
        //   value = 1 allow reads to return immediately
        //   returns -1 for failure, 0 for success

        int oldflags, result;

        oldflags = fcntl( STDIN_FILENO, F_GETFL, 0 );

        if (oldflags == -1)
                return( -1 );

        if (value != 0)
                oldflags |= O_NONBLOCK;
        else
                oldflags &= ~O_NONBLOCK;

       result = fcntl( STDIN_FILENO, F_SETFL, oldflags );

       return( result );
}

//------------------------------------------------------------------------------
// ROUTINE: OsGetKey (Linux version)
// PURPOSE: Test for a keypress and return it if one is ready.  The wait
//          argument indicates what to do if a keypress is not immediately
//          available.  If wait is true, then wait until one is ready.
//          Otherwise, just return right away.
//------------------------------------------------------------------------------
SRLOCAL int OsGetKey( int wait ) {

        int c, c2;

        // Get a ready keypress or wait until one is ready if requested.

        // Notes:
        //
        // This version assumes that non-blocking reads have been
        // enabled.  Otherwise, the first getchar will always wait until
        // a character is ready.
        //
        // Getchar requires an ENTER after the character before it will
        // start processing the character.


        c = getchar();
        while ( (c == EOF) && wait )
                c = getchar();


        // If found a character, flush any remaining characters (eg ENTER)
        
        c2 = c;
        while ( c2 != EOF )
                c2 = getchar();
            
        return( c );
}

//------------------------------------------------------------------------------
// ROUTINE: OsFlushPrint (Linux version)
// PURPOSE: Ensures that printf statements that don't end with a carriage return
//          (ie \n) are output immediately rather then buffered for later output.
//------------------------------------------------------------------------------
SRLOCAL void OsFlushPrint( void ) {
        
        fflush( STDIN_FILENO );
}

//------------------------------------------------------------------------------
// ROUTINE: OsGetPcTime (Linux version)
// PURPOSE: Get the PC system time and return it as time specified as seconds
//          since 1970 and as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int OsGetPcTime( double *Time,
                         int *Year, int *Month, int *Day, 
                         int *Hour, int *Minute, int *Second,
                         long *Microsecond ) {

        struct timeval tv;
        int            Yr, Mo, Dy, Hh, Mm, Ss;
        long           Usec;
        double         SetTime, IntTime, FracTime;


        // Prepare time in timeval format Linux prefers

        gettimeofday( &tv, NULL );

        IntTime  = (double)tv.tv_sec;
        FracTime = (double)tv.tv_usec / (double)SR_USPERSEC;

        SetTime = IntTime + FracTime;
        
        SrSecTimeSplit( SetTime, &Yr, &Mo, &Dy, &Hh, &Mm, &Ss, &Usec );


        if (Time)         *Time        = SetTime;
        if (Year)         *Year        = Yr;
        if (Month)        *Month       = Mo;
        if (Day)          *Day         = Dy;
        if (Hour)         *Hour        = Hh;
        if (Minute)       *Minute      = Mm;
        if (Second)       *Second      = Ss;
        if (Microsecond)  *Microsecond = Usec;
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: OsSetPcTime (Linux version)
// PURPOSE: Set the PC system time from the supplied time specified as seconds
//          since 1970 or as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int OsSetPcTime( double Time,
                         int Year, int Month, int Day, 
                         int Hour, int Minute, int Second,
                         long Microsecond, int FromYMD ) {

        struct timeval tv;
        double         SetTime, IntTime, FracTime;


        // Prepare time in timeval format Linux prefers

        if (FromYMD == 1)
                SrSecTimeCombine( &SetTime, Year, Month, Day, 
                                  Hour, Minute, Second, Microsecond );
        else
                SetTime = Time;

        IntTime = (long)SetTime;
        FracTime = SetTime - IntTime;

        tv.tv_sec = (long)IntTime;
        tv.tv_usec = (long)(FracTime*SR_USPERSEC);
        
        if (settimeofday( &tv, NULL ) == -1)
                return( 0 );
        else
                return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: OsSleep (Linux version)
// PURPOSE: Sleeps for the specified number of milliseconds.  This usually 
//          allows other processes access to the CPU.  The DOS version just
//          returns immediately unless the hardwait parameter is true.  In which
//          case it busy waits instead.  The return value is 0 for success and
//          -1 for error.
//------------------------------------------------------------------------------
SRLOCAL int OsSleep( int ms, int hardwait ) {

        // Sleep for specifed number of milliseconds.
        // Returns 0 for success, -1 for error.

        struct timespec twait, tleft;
        
        if ( ms <= 0 )
                return( 0 );

        
        // Linux nanosleep takes structure with seconds and nanoseconds.
        
        tleft.tv_sec  = tleft.tv_nsec = 0;
        twait.tv_sec  = (long)(ms / 1000);
        twait.tv_nsec = (ms - (twait.tv_sec*1000)) * 1000000;
        
        return( nanosleep( &twait, &tleft ) );
}

//------------------------------------------------------------------------------
// ROUTINE: OsMkDir (Linux version)
// PURPOSE: Make a new directory with the specified name and full permissions.
//------------------------------------------------------------------------------
SRLOCAL int OsMkDir( char *DirectoryName ) {
        
        int Permissions;

        Permissions = (S_IRWXU | S_IRWXG | S_IRWXO);  // Read,Write,Execute
                                                      // for User,Group,Other

	return( mkdir( DirectoryName, Permissions ) );
}


#elif defined( SROS_WIN9X ) || defined( SROS_MSDOS )

//------------------------------------------------------------------------------
// ROUTINE: OsSetNonBlock (DOS version)
// PURPOSE: Sets non-blocking mode which allows testing for a keypress.
//------------------------------------------------------------------------------
SRLOCAL int OsSetNonBlock( int value ) {

        // Win9x/MsDOS are always in non-blocking mode (value=1)
        //   value = 0 blocks reads until a character is ready
        //   value = 1 allow reads to return immediately
        //   returns -1 for failure, 0 for success

        if (value == 1)
                return( 0 );
        else
                return( -1 );
}

//------------------------------------------------------------------------------
// ROUTINE: OsGetKey (DOS version)
// PURPOSE: Test for a keypress and return it if one is ready.  The wait
//          argument indicates what to do if a keypress is not immediately
//          available.  If wait is true, then wait until one is ready.
//          Otherwise, just return right away.
//------------------------------------------------------------------------------
SRLOCAL int OsGetKey( int wait ) {

        int c;

        // Get a ready keypress or wait until one is ready if requested.

        c = EOF;
        if ( _kbhit() || wait ) {
                c = _getch();
                if (c == 0xD) printf("\n");  // add newline if char was ENTER
                if (c == 0x00 || c == 0xE0 ) // deal with extended key (fn,->)
                        c = _getch();        // c has scan code
                }
        
        return( c );
}

//------------------------------------------------------------------------------
// ROUTINE: OsFlushPrint (DOS version)
// PURPOSE: Ensures that printf statements that don't end with a carriage return
//          (ie \n) are output immediately rather then buffered for later output.
//------------------------------------------------------------------------------
SRLOCAL void OsFlushPrint( void ) {
        
        // Win9x/MsDOS automatically flushes printf.
}

//------------------------------------------------------------------------------
// ROUTINE: OsGetPcTime (DOS version)
// PURPOSE: Get the PC system time and return it as time specified as seconds
//          since 1970 and as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int OsGetPcTime( double *Time,
                         int *Year, int *Month, int *Day, 
                         int *Hour, int *Minute, int *Second,
                         long *Microsecond ) {
        
        time_t    ttNow;
        struct tm tmNow;
        double    dtime;
        long      ltime;

        // Return time as YMD HMS for UTC (not local)

        time( &ttNow );
        tmNow   = *gmtime( &ttNow );
        dtime   = (double)ttNow;
        ltime   = (long)ttNow;

        if (Time)         *Time        = dtime;
        if (Year)         *Year        = tmNow.tm_year + 1900;
        if (Month)        *Month       = tmNow.tm_mon + 1;
        if (Day)          *Day         = tmNow.tm_mday;
        if (Hour)         *Hour        = tmNow.tm_hour;
        if (Minute)       *Minute      = tmNow.tm_min;
        if (Second)       *Second      = tmNow.tm_sec;
        if (Microsecond)  *Microsecond = (long)((dtime - ltime) * SR_USPERMS);

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: OsSetPcTime (DOS version)
// PURPOSE: Set the PC system time from the supplied time specified as seconds
//          since 1970 or as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int OsSetPcTime( double Time,
                         int Year, int Month, int Day, 
                         int Hour, int Minute, int Second,
                         long Microsecond, int FromYMD ) {

        // NOT SUPPORTED
        printf( "Can not set PC time for DOS or Windows95/98\n" );

        return( 0 );
}


#if defined ( SROS_WIN9X )
//------------------------------------------------------------------------------
// ROUTINE: OsSleep (Windows95/98 version)
// PURPOSE: Sleeps for the specified number of milliseconds.  This usually 
//          allows other processes access to the CPU.  The DOS version just
//          returns immediately unless the hardwait parameter is true.  In which
//          case it busy waits instead.  The return value is 0 for success and
//          -1 for error.
//------------------------------------------------------------------------------
SRLOCAL int OsSleep( int ms, int hardwait ) {

        // Sleep for specifed number of milliseconds.
        // Returns 0 for success, -1 for error.
        
        if ( ms <= 0 )
                return( 0 );

        Sleep( ms );

        return( 0 );
}
#elif defined( SROS_MSDOS )
//------------------------------------------------------------------------------
// ROUTINE: OsSleep (DOS version)
// PURPOSE: Sleeps for the specified number of milliseconds.  This usually 
//          allows other processes access to the CPU.  The DOS version just
//          returns immediately unless the hardwait parameter is true.  In which
//          case it busy waits instead.  The return value is 0 for success and
//          -1 for error.
//------------------------------------------------------------------------------
SRLOCAL int OsSleep( int ms, int hardwait ) {

        struct _timeb timebuffer;
        unsigned short endms;
        time_t endsec;

        // Sleep for specifed number of milliseconds.
        // Returns 0 for success, -1 for error.
        
        if ( ms <= 0 )
                return( 0 );

        // No sleep function is available for MSDOS, so just
        // return.  Unless the user has requested that they
        // really want to wait, even if it means spinning in
        // a loop wasting CPU cycles.
        
        if ( hardwait ) {
                
                _ftime( &timebuffer );
                endsec = timebuffer.time;
                endms  = timebuffer.millitm + ms;
                
                while (endms > 1000) {
                        endms  -= 1000;
                        endsec += 1;
                        }
                
                while (timebuffer.time < endsec)
                        _ftime( &timebuffer );
                
                while ( (timebuffer.time == endsec) &&
                        (timebuffer.millitm < endms) )
                        _ftime( &timebuffer );
                }

                
//                ms *= 1000; // Convert ms to us
//                while ( ms-- )
//                        _asm {  mov  dx,0x379  // Read from status port
//                                in   al,dx     // takes about 1 microsecond
//                                }

        return( 0 );
}



#endif // SROS_WIN9X or SROS_MSDOS for sleep function difference

//------------------------------------------------------------------------------
// ROUTINE: OsMkDir (DOS version)
// PURPOSE: Make a new directory with the specified name.
//------------------------------------------------------------------------------
SRLOCAL int OsMkDir( char *DirectoryName ) {
	return( _mkdir( DirectoryName ) );
}



#endif // SROS_xxxxx   End of OS dependent helper functions

