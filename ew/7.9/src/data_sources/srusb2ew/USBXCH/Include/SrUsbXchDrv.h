// FILE: SrUsbXchDrv.h
// COPYRIGHT: (c), Symmetric Research, 2009-2011
// 
// Those users wanting access to the defines and prototypes needed when
// calling the USBxCH library functions should include SrUsbXch.h and NOT
// this file SrUsbXchDrv.h.
// 
// This include file is useful only for people wanting to recompile the
// USBxCH library and kernel mode device driver.  MOST USERS DO NOT
// NEED TO INCLUDE IT.  
// 
// The constants and structures in this file are those shared between
// the UsbXch.c user library and SrUsbXch.c kernel mode device driver,
// and those used by the driver installation/removal utility programs.
// 
// 
// Change History:
// 
// 2011/08/09  WCT  Minor changes for POSIX compliance
// 2010/10/01  WCT  Added Cypress VID/PID defines
// 2010/04/19  WCT  Moved USB_DEVICE_DESCRIPTOR defines into driver source
// 2009/10/26  WCT  Include LINUX support
// 2009/08/05  WCT  Initial version
// 


#ifndef __SRUSBXCHDRV_H__
#define __SRUSBXCHDRV_H__ (1)


#if defined( SROS_WIN2K ) || defined( SROS_WINXP )
#include <basetsd.h>
#elif defined( SROS_LINUX )
#endif

//FIX - Think about disabling EP4 and EP8 so 2 & 6 can go quad.

// These pipe numbers must match what is really
// in the USBxCH USB chip

#define PIPE_EP1_OUT      0
#define PIPE_EP1_IN       1
#define PIPE_EP2_OUT      2
#define PIPE_EP4_OUT      3
#define PIPE_EP6_IN       4
#define PIPE_EP8_IN       5
#define PIPE_EP0          6

#define PIPE_MIN          PIPE_EP1_OUT
#define PIPE_MAX          PIPE_EP8_IN


#define USBXCH_PIPE_TYPE_NONE           -1
#define USBXCH_PIPE_TYPE_CMD             0
#define USBXCH_PIPE_TYPE_ANALOG          1
#define USBXCH_PIPE_TYPE_SERIAL          2
#define USBXCH_PIPE_TYPE_MAX             3


// Defines from Cypress EZUSB

#define ANCHOR_LOAD_INTERNAL    0xA0      // Implemented w/o firmware
#define CPUCS_REG_FX2           0xE600      // location, Bit 0 controls 8051 reset
#define CPUCS_REG_EZUSB         0x7F92      // EZ-USB Control and Status Register


// USB Vendor and Product IDs that identify the SR USBxCH devices

#define SRUSBXCH_VID            0x15D3
#define SRUSBXCH_PID_4CH        0x5504
#define SRUSBXCH_PID_8CH        0x5508

#define SRUSBXCH_CYPRESS_VID    0X04B4
#define SRUSBXCH_CYPRESS_PID    0X6473


// SRUSBXCH Symbolic Link Name (USBXCH_DEFAULT_DRIVER_NAME in usbxch.h should match)

#define SRUSBXCH_DRIVER_NAME      "SrUsbXch"
#define SRUSBXCH_DRIVER_NAME_L   L"SrUsbXch"
#define SRUSBXCH_DRIVER_NAME_D    "SrUsbXch%d"


// Each driver can manage 10 devices (0-9), under Linux they have names of
// the following form while under Windows their names are long GUIDs

#define MAX_DEVICES    10

#define SRUSBXCH_DEVICE_NAME      "SrUsbXchDev"
#define SRUSBXCH_DEVICE_NAME_D    "SrUsbXchDev%d"




// Define environment/registry key string for model name
// Complete format is UsbXch_ModelName

#define REGENVSTR_MODELNAME      "ModelName"




// Define shared USB request structures

typedef struct _SR_STANDARD_REQUEST_DATA {
	unsigned char  Direction;
	unsigned char  Recipient;
	unsigned char  Request;
	unsigned short Value;
	unsigned short Index;
	unsigned long  Length;
	} SR_STANDARD_REQUEST_DATA;

typedef struct _SR_VENDOR_REQUEST_DATA {
	unsigned char  Request;
	unsigned short Value;
	unsigned short Index;
	unsigned long  Length;
	} SR_VENDOR_REQUEST_DATA;




// Define OS dependent device strings and functions

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )
#define REG_PARM_DRIVERNAME      "DriverName"
#define LREG_PARM_DRIVERNAME    L"DriverName"

#include "initguid.h"

// SR Instrumentation Class Guid. {AD1787FF-EF97-45D4-A4A8-620D8A00AC56}

DEFINE_GUID(GUID_SRINSTRUMENTATION_CLASS, 
            0xAD1787FF, 0xEF97, 0x45D4, 0xA4, 0xA8, 0x62, 0x0D, 0x8A, 0x00, 0xAC, 0x56);


// SR USBXCH Interface Guid. {C5B7F228-CAFF-42d5-A472-6B9EDA7982EC}

DEFINE_GUID(GUID_SRUSBXCH_INTERFACE, 
	    0xc5b7f228, 0xcaff, 0x42d5, 0xa4, 0x72, 0x6b, 0x9e, 0xda, 0x79, 0x82, 0xec);



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

#endif  // SROS_WINXP  SROS_WIN2K  SROS_LINUX
   



// IOCTL_ID NUMBERS:
// 
// The I/O Control code id numbers are used to indicate which function
// the device driver should perform on the kernel side.
// 
// The format of these id numbers are OS dependent. DO NOT change them,
// they have bit fields which are OS specific.
// 



#if defined( SROS_WINXP ) || defined( SROS_WIN2K )

// UNDER WIN2K/XP:
// 
// Device types in the range 0x8000 to 0xFFFF (32768-65535) are for customer use.
// IOCTL function codes 0x800 to 0xFFF (2048-4095) are for customer use.
// 
// Winioctl.h is only included here, if ntddk.h has not already been
// included elsewhere.  This is because either file defines the needed
// ioctl information, but they conflict over DEVICE_TYPE.
// 
// The bit fields in the IOCTL_ID ( Windown CTL_CODE ) are assigned in
// winioctl.h as follows:
// 
// #define CTL_CODE( DeviceType, Function, Method, Access) =
// 
//                 ( (DeviceType) << 16 ) |
//                 ( (Access)     << 14 ) |
//                 ( (Function)   <<  2 ) |
//                 ( (Method)     <<  0 )
// 
// The two decoding macros IOCTL_TYPE and _IOCTL_FUNC return their
// respective bit fields from IOCTL_ID.
// 


#if !defined( DEVICE_TYPE )
#include <winioctl.h>
#endif


// The following value is arbitrarily chosen from the space defined by
// Microsoft as being "for non-Microsoft use"

#define USBXCH_DEVICE_TYPE                  41683  // 0xA2D3

#define DA_RD                               FILE_READ_ACCESS
#define DA_WR                               FILE_WRITE_ACCESS
#define DA_RW                               (FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define DM_BF                               METHOD_BUFFERED
#define DM_DI                               METHOD_IN_DIRECT
#define DM_DO                               METHOD_OUT_DIRECT
#define DM_NE                               METHOD_NEITHER

#define IOCTLBASE                            0x800
#define IOCTL_ID( id, access, method, size)  CTL_CODE( USBXCH_DEVICE_TYPE, (IOCTLBASE+id), method, access)

#define IOCTL_TYPE( ioctl_id)                (((ioctl_id)>>16) & 0xFFFF)
#define IOCTL_FUNC( ioctl_id)                (((ioctl_id)>>2)  & 0x0FFF)
#define IOCTL_METH( ioctl_id)                (((ioctl_id)>>0)  & 0x0003)




#elif defined( SROS_LINUX )

#include <sys/ioctl.h>          // included for ioctl macros

#define USBXCH_DEVICE_TYPE                   0x4B

#define DA_RD                                _IOR
#define DA_WR                                _IOW
#define DA_RW                                _IOWR

#define DM_BF
#define DM_DI
#define DM_DO
#define DM_NE

#define IOCTL_ID( id, access, method, size)  access( USBXCH_DEVICE_TYPE, id, size)
#define IOCTLBASE                            0x00

#define IOCTL_TYPE( ioctl_id)                (_IOC_TYPE(ioctl_id))
#define IOCTL_FUNC( ioctl_id)                (_IOC_NR(ioctl_id))
#define IOCTL_METH( ioctl_id)                (DM_BF)


#endif  // SROS_xxxxx


#define IOCTL_USBXCH_GET_DEVICE_DESCRIPTOR        IOCTL_ID( 0x01, DA_RD, DM_BF, IRP )
#define IOCTL_USBXCH_GET_CONFIG_DESCRIPTOR_SIZE   IOCTL_ID( 0x02, DA_RD, DM_BF, IRP )
#define IOCTL_USBXCH_GET_CONFIG_DESCRIPTOR        IOCTL_ID( 0x03, DA_RD, DM_BF, IRP )
#define IOCTL_USBXCH_VENDOR_REQUEST               IOCTL_ID( 0x04, DA_RW, DM_DI, IRP )
#define IOCTL_USBXCH_BULK_READ_CMD                IOCTL_ID( 0x05, DA_WR, DM_DO, IRP )
#define IOCTL_USBXCH_BULK_WRITE_CMD               IOCTL_ID( 0x06, DA_RD, DM_DI, IRP )
#define IOCTL_USBXCH_RESET_PIPE                   IOCTL_ID( 0x07, DA_WR, DM_BF, IRP )
#define IOCTL_USBXCH_ABORT_PIPE                   IOCTL_ID( 0x08, DA_WR, DM_BF, IRP )
#define IOCTL_USBXCH_RESET_PORT                   IOCTL_ID( 0x09, DA_WR, DM_BF, IRP )
#define IOCTL_USBXCH_CYCLE_PORT                   IOCTL_ID( 0x0A, DA_WR, DM_BF, IRP )
#define IOCTL_USBXCH_BULK_READ                    IOCTL_ID( 0x0B, DA_WR, DM_DO, IRP )
#define IOCTL_USBXCH_BULK_WRITE                   IOCTL_ID( 0x0C, DA_RD, DM_DI, IRP )
#define IOCTL_USBXCH_STANDARD_REQUEST             IOCTL_ID( 0x0D, DA_RW, DM_DI, IRP )
#define IOCTL_MAXFUNC                             IOCTLBASE+0x0E

//#define IOCTL_USBXCH_GET_INTERFACE_INFORMATION    IOCTL_ID( 0x04, DA_RD, DM_BF, IRP )
//#define IOCTL_USBXCH_GET_PIPE_INFORMATION         IOCTL_ID( 0x05, DA_RD, DM_BF, IRP )
//#define IOCTL_USBXCH_GET_LAST_USB_ERROR           IOCTL_ID( 0x0A, DA_RD, DM_BF, IRP )
//#define IOCTL_USBXCH_GET_USB_STATUS               IOCTL_ID( 0x0B, DA_RW, DM_BF, IRP )
//#define IOCTL_USBXCH_RESET_DEVICE                 IOCTL_ID( 0x0E, DA_RW, DM_BF, IRP )
//#define IOCTL_USBXCH_SET_INTERFACE                IOCTL_ID( 0x0F, DA_WR, DM_BF, IRP )
//#define IOCTL_USBXCH_CONTROL_CMD                  IOCTL_ID( 0x10, DA_RW, DM_BF, IRP )
//#define IOCTL_USBXCH_GET_FRAME                    IOCTL_ID( 0x11, DA_RD, DM_BF, IRP )


#endif // __SRUSBXCHDRV_H__
