/* FILE: pargpskd.h             Copyright (c), Symmetric Research, 2003-2007

This include file is useful only for people wanting to recompile the
PARGPS library and kernel mode device driver.  MOST USERS DO NOT
NEED TO INCLUDE IT.

The constants and structures in this file are those shared between
the pargps.c user library and pargpskd.c kernel mode device driver,
and those used by the driver installation/removal utility programs.


Change History:

12/05/07 Added Garmin GPS 18 LVC support
09/15/06 Added FLUSH_SERIAL IOCTL to support error recovery
06/20/05 Added UserIrp to Linux IRP definition to support 2.6 kernels

*/


// Define OS dependent device strings and functions

#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )
#define REG_PARM_DRIVERNAME      "DriverName"
#define LREG_PARM_DRIVERNAME    L"DriverName"

#define COMOFFSET     0
#define COMFMT        "COM%d"
#define KERNEL_COMFMT "\\DosDevices\\COM%d"


#elif defined( SROS_LINUX )
#define ERROR_SERVICE_DISABLED          ((long)(-ENOSYS))

#define COMOFFSET     1
#define COMFMT        "/dev/ttyS%d"
#define KERNEL_COMFMT "/dev/ttyS%d"

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

typedef struct _IoctlIrp {

        unsigned long  Command;
        void          *InBuffer;
        unsigned long  InSize;
        void          *OutBuffer;
        unsigned long  OutSize;
        unsigned long  ReturnedBytes;
        int            DataMethod;
        int            ErrorCode;
        void          *UserIrp;

        } IRP, *PIRP;


#else
#pragma message( "COMPILE ERROR: SROS_xxxxx MUST BE DEFINED !! " )

#endif  // SROS_WINXP  SROS_WIN2K  SROS_WINNT  SROS_LINUX



// Defines shared between library and driver

#define PARGPS_INTTYPE_BAD      -1
#define PARGPS_INTTYPE_PLAIN     0
#define PARGPS_INTTYPE_PPS       1
#define PARGPS_INTTYPE_DREADY    2
#define PARGPS_INTTYPE_FAKEPPS   3
#define PARGPS_INTTYPE_MAX       4

#define THREAD_STOPPING       0
#define THREAD_STARTING       1
#define THREAD_RUNNING        2
#define THREAD_DONE           3



// Define environment/registry key string for gps model name
// Complete format is SrParGps0_ModelName

#define REGENVSTR_GPSMODELNAME      "ModelName"




/* IOCTL_ID NUMBERS:

The I/O Control code id numbers are used to indicate which function
the device driver should perform on the kernel side.

The format of these id numbers are OS dependent. DO NOT change them,
they have bit fields which are OS specific.

*/


#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )

/* UNDER WINXP, WIN2K and WINNT:

Device types in the range 0x8000 to 0xFFFF (32768-65535) are for customer use.
IOCTL function codes 0x800 to 0xFFF (2048-4095) are for customer use.

Winioctl.h cannot be included here since it conflicts with ntddk.h on
DEVICE_TYPE.

The bit fields in the IOCTL_ID ( WinNT CTL_CODE ) are assigned in
winioctl.h as follows:

#define CTL_CODE( DeviceType, Function, Method, Access) =

                ( (DeviceType) << 16 ) |
                ( (Access)     << 14 ) |
                ( (Function)   <<  2 ) |
                ( (Method)     <<  0 )

The two decoding macros IOCTL_TYPE and _IOCTL_FUNC return their
respective bit fields from IOCTL_ID.

*/

#if !defined( DEVICE_TYPE )
#include <winioctl.h>
#endif

#define PARGPS_DEVICE_TYPE              0xDD47

#define DA_RD                           FILE_READ_ACCESS
#define DA_WR                           FILE_WRITE_ACCESS
#define DA_RW                          (FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define DM_BF                           METHOD_BUFFERED
#define DM_DI                           METHOD_IN_DIRECT
#define DM_DO                           METHOD_OUT_DIRECT
#define DM_NE                           METHOD_NEITHER

#define IOCTL_PARGPS_ID( id, access, method, size ) CTL_CODE( PARGPS_DEVICE_TYPE, (0x900+id), method, access )
#define IOCTLPARGPSBASE                 0x900

#define IOCTL_PARGPS_TYPE( ioctl_id )    (((ioctl_id)>>16) & 0xFFFF)
#define IOCTL_PARGPS_FUNC( ioctl_id )    (((ioctl_id)>>2)  & 0x0FFF)
#define IOCTL_PARGPS_METH( ioctl_id )    (((ioctl_id)>>0)  & 0x0003)



#elif defined( SROS_LINUX )

#include <sys/ioctl.h>        // included for ioctl macros

#define PARGPS_DEVICE_TYPE              0x21

#define DA_RD                           _IOR
#define DA_WR                           _IOW
#define DA_RW                           _IOWR

#define DM_BF
#define DM_DI
#define DM_DO
#define DM_NE

#define IOCTL_PARGPS_ID( id, access, method, size ) access( PARGPS_DEVICE_TYPE, id, size )
#define IOCTLPARGPSBASE                 0x00

#define IOCTL_PARGPS_TYPE( ioctl_id )   (_IOC_TYPE(ioctl_id))
#define IOCTL_PARGPS_FUNC( ioctl_id )   (_IOC_NR(ioctl_id))
#define IOCTL_PARGPS_METH( ioctl_id )   (DM_BF)


#endif  // SROS_xxxxx

// The I/O Control codes are used to indicate which function the
// hardware device should perform. DA_RD means read value from kernel
// space and copy to user.

#define IOCTL_PARGPS_START            IOCTL_PARGPS_ID( 0x00, DA_WR, DM_BF, IRP )
#define IOCTL_PARGPS_STOP             IOCTL_PARGPS_ID( 0x01, DA_WR, DM_BF, IRP )
#define IOCTL_PARGPS_INIT             IOCTL_PARGPS_ID( 0x02, DA_RW, DM_BF, IRP )
#define IOCTL_PARGPS_RESET            IOCTL_PARGPS_ID( 0x03, DA_WR, DM_BF, IRP )
#define IOCTL_PARGPS_SET_INTR_MODE    IOCTL_PARGPS_ID( 0x04, DA_WR, DM_BF, IRP )
#define IOCTL_PARGPS_SET_SERIAL_PORT  IOCTL_PARGPS_ID( 0x05, DA_WR, DM_BF, IRP )
#define IOCTL_PARGPS_GET_KAREA        IOCTL_PARGPS_ID( 0x06, DA_RD, DM_BF, IRP )
#define IOCTL_PARGPS_GET_INTR_COUNT   IOCTL_PARGPS_ID( 0x07, DA_RD, DM_BF, IRP )
#define IOCTL_PARGPS_GET_COUNTERFREQ  IOCTL_PARGPS_ID( 0x08, DA_RD, DM_BF, IRP )
#define IOCTL_PARGPS_GET_CURRENT_TIME IOCTL_PARGPS_ID( 0x09, DA_RD, DM_BF, IRP )
#define IOCTL_PARGPS_THREAD_STATE     IOCTL_PARGPS_ID( 0x0A, DA_WR, DM_BF, IRP )
#define IOCTL_PARGPS_READ_PPS_DATA    IOCTL_PARGPS_ID( 0x0B, DA_RD, DM_DO, IRP )
#define IOCTL_PARGPS_READ_SERIAL_DATA IOCTL_PARGPS_ID( 0x0C, DA_RD, DM_DO, IRP )
#define IOCTL_PARGPS_FLUSH_SERIAL     IOCTL_PARGPS_ID( 0x0D, DA_WR, DM_BF, IRP )
#define IOCTL_PARGPS_MAXFUNC          IOCTLPARGPSBASE+ 0x0E
