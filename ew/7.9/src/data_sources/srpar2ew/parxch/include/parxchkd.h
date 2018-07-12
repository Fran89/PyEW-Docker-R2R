/* FILE: parxchkd.h                Copyright (c), Symmetric Research, 2003-2007

This include file is useful only for people wanting to recompile the
PARxCH library and kernel mode device driver.  MOST USERS DO NOT
NEED TO INCLUDE IT.

The constants and structures in this file are those shared between
the parxch.c user library and parxchkd.c kernel mode device driver,
and those used by the driver installation/removal utility programs.


Change History:

01/08/07 Added PortAddress and PortIrq registry strings for SrParXch0 support
06/20/05 Added UserIrp to Linux IRP definition to support 2.6 kernels

*/




// Define OS dependent device strings and functions

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )
#define REG_PARM_DRIVERNAME      "DriverName"
#define LREG_PARM_DRIVERNAME    L"DriverName"
#define REG_PARM_PORTADDR        "PortAddress"
#define LREG_PARM_PORTADDR      L"PortAddress"
#define REG_PARM_PORTIRQ         "PortIrq"
#define LREG_PARM_PORTIRQ       L"PortIrq"
#define REG_PARM_PORTFOUND       "PortFound"
#define LREG_PARM_PORTFOUND     L"PortFound"


#elif defined( SROS_WIN9X ) || defined( SROS_MSDOS )
#if defined( SROS_MSDOS )
#define ERROR_CRC                       ((long)23L)
#define ERROR_SERVICE_DISABLED          ((long)1058L)
#define ERROR_SERVICE_ALREADY_RUNNING   ((long)1056L)
#endif

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

        } IRP, *PIRP;

DEVHANDLE Msdos_DrvOpen( char *DriverName );
int Msdos_DrvRead(
                  DEVHANDLE      hDevice,
                  void          *lpBuffer,
                  unsigned long  nNumberOfBytesToRead,
                  unsigned long *lpNumberOfBytesRead
                  );
int Msdos_DrvIoctl(
                    DEVHANDLE      hDevice,
                    unsigned long  dwIoControlCode,
                    void          *lpInBuffer,
                    unsigned long  nInBufferSize,
                    void          *lpOutBuffer,
                    unsigned long  nOutBufferSize,
                    unsigned long *lpBytesReturned
                    );
int Msdos_DrvClose( DEVHANDLE hDevice );
long Msdos_DrvGetLastError( void );


#elif defined( SROS_LINUX )
#define ERROR_CRC                       ((long)(-EIO))
#define ERROR_SERVICE_DISABLED          ((long)(-ENOSYS))
#define ERROR_SERVICE_ALREADY_RUNNING   ((long)(-EEXIST))

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

#endif  // SROS_WINXP  SROS_WIN2K  SROS_WIN9X  SROS_MSDOS  SROS_LINUX



// Define a structure to facilitate passing data between the DLL and
// the device driver.

typedef struct _InitxchParms {

             int XchModel;

             int ParPortIrq;
             int ParPortMode;

             int Df;
             int GainLog;
             int TurboLog;
             int Decimation;
             int ExtraDecimation;

            long ADS1210_CrValue;
             int Error;

        } INITXCHPARMS, *PINITXCHPARMS;


typedef struct _PortValuexch {

        int Port;
        int Value;

        } PORTVALUEXCH, *PPORTVALUEXCH;


// Defines shared between user and kernel portions of the driver.
// A/D health and diag flag values.

#define HEALTH_VOLTAGE_BAD (1 << 0)
#define HEALTH_OVERFLOW    (1 << 1)

#define DIAG_FLAG_VALUE    0x0F
#define DIAG_FLAG_READY    0x10
#define DIAG_FLAG_FULL     0x20


// Define environment/registry key string for model name, address, irq
// Complete format is SrParXch0_ModelName

#define REGENVSTR_MODELNAME      "ModelName"
#define REGENVSTR_PORTADDR       "PortAddress"
#define REGENVSTR_PORTIRQ        "PortIrq"




/* IOCTL_ID NUMBERS:

The I/O Control code id numbers are used to indicate which function
the device driver should perform on the kernel side.

The format of these id numbers are OS dependent. DO NOT change them,
they have bit fields which are OS specific.

*/


#if defined( SROS_WINXP ) || defined( SROS_WIN2K )

/* UNDER WINXP, WIN2K:

Device types in the range 0x8000 to 0xFFFF (32768-65535) are for customer use.
IOCTL function codes 0x800 to 0xFFF (2048-4095) are for customer use.

Winioctl.h is only included here, if ntddk.h has not already been
included elsewhere.  This is because either file defines the needed
ioctl information, but they conflict over DEVICE_TYPE.

The bit fields in the IOCTL_ID ( Win CTL_CODE ) are assigned in
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


#define PARXCH_DEVICE_TYPE                  0xDD4C

#define DA_RD                               FILE_READ_ACCESS
#define DA_WR                               FILE_WRITE_ACCESS
#define DA_RW                               (FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define DM_BF                               METHOD_BUFFERED
#define DM_DI                               METHOD_IN_DIRECT
#define DM_DO                               METHOD_OUT_DIRECT
#define DM_NE                               METHOD_NEITHER

#define IOCTL_ID( id, access, method, size)  CTL_CODE( PARXCH_DEVICE_TYPE, (0x900+id), method, access)
#define IOCTLBASE                            0x900

#define IOCTL_TYPE( ioctl_id)                (((ioctl_id)>>16) & 0xFFFF)
#define IOCTL_FUNC( ioctl_id)                (((ioctl_id)>>2)  & 0x0FFF)
#define IOCTL_METH( ioctl_id)                (((ioctl_id)>>0)  & 0x0003)





#elif defined( SROS_WIN9X ) || defined( SROS_MSDOS )

#define PARXCH_DEVICE_TYPE                   0x3443  // = "4C"

#define DA_RD
#define DA_WR
#define DA_RW

#define DM_BF
#define DM_DI
#define DM_DO
#define DM_NE

#define IOCTL_ID( id, access, method, size)  id
#define IOCTLBASE                            0x00

#define IOCTL_TYPE( ioctl_id)                (PARXCH_DEVICE_TYPE)
#define IOCTL_FUNC( ioctl_id)                (ioctl_id)
#define IOCTL_METH( ioctl_id)                (DM_BF)






#elif defined( SROS_LINUX )

#include <sys/ioctl.h>        // included for ioctl macros

#define PARXCH_DEVICE_TYPE                   0x23

#define DA_RD                                _IOR
#define DA_WR                                _IOW
#define DA_RW                                _IOWR

#define DM_BF
#define DM_DI
#define DM_DO
#define DM_NE

#define IOCTL_ID( id, access, method, size)  access( PARXCH_DEVICE_TYPE, id, size)
#define IOCTLBASE                            0x00

#define IOCTL_TYPE( ioctl_id)                (_IOC_TYPE(ioctl_id))
#define IOCTL_FUNC( ioctl_id)                (_IOC_NR(ioctl_id))
#define IOCTL_METH( ioctl_id)                (DM_BF)


#endif  // SROS_xxxxx



#define IOCTL_PARXCH_INIT                 IOCTL_ID( 0x00, DA_RW, DM_BF, IRP )
#define IOCTL_PARXCH_START                IOCTL_ID( 0x01, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_STOP                 IOCTL_ID( 0x02, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_READY                IOCTL_ID( 0x03, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_OVERFLOW             IOCTL_ID( 0x04, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_USER_LED             IOCTL_ID( 0x05, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIO_READ             IOCTL_ID( 0x06, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_DIO_WRITE            IOCTL_ID( 0x07, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_INTR_ENABLE          IOCTL_ID( 0x08, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_INTR_DISABLE         IOCTL_ID( 0x09, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_VOLTAGE_GOOD         IOCTL_ID( 0x0A, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_CHECK_HEALTH         IOCTL_ID( 0x0B, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_SET_POWER_DOWN  IOCTL_ID( 0x0C, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_BDCTRL_MODE     IOCTL_ID( 0x0D, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_FIFO_RESET      IOCTL_ID( 0x0E, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_FIFO_READ       IOCTL_ID( 0x0F, DA_RD, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_FIFO_WRITE      IOCTL_ID( 0x10, DA_RW, DM_BF, IRP )
#define IOCTL_PARXCH_ARM_DREADY_INTR      IOCTL_ID( 0x11, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_PREPARE_TO_READ      IOCTL_ID( 0x12, DA_RW, DM_BF, IRP )
#define IOCTL_PARXCH_ATTACH_GPS           IOCTL_ID( 0x13, DA_RW, DM_BF, IRP )
#define IOCTL_PARXCH_RELEASE_GPS          IOCTL_ID( 0x14, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_TURN_ON         IOCTL_ID( 0x15, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_TURN_OFF        IOCTL_ID( 0x16, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_POWER_UP        IOCTL_ID( 0x17, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_POWER_DOWN      IOCTL_ID( 0x18, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_PORT_IN         IOCTL_ID( 0x19, DA_RW, DM_BF, IRP )
#define IOCTL_PARXCH_DIAG_PORT_OUT        IOCTL_ID( 0x1A, DA_WR, DM_BF, IRP )
#define IOCTL_PARXCH_GET_INTR             IOCTL_ID( 0x1B, DA_RD, DM_BF, IRP )
#define IOCTL_MAXFUNC                     IOCTLBASE+0x1C
