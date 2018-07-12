FILE: readme.txt                 Copyright (c), Symmetric Research, 2010

This directory contains a few files from the SR USBxCH software that
are required for recompiling and running the srusb2ew module.  Of
course, you won't be able to acquire any data unless you have the
actual SR USB4CH 24 bit A/D.

The files from this directory that you need to compile are:

  File                   Description
  ----                   -----------
  Include\SrDefines.h   Include file with contants and typedefs
  Include\SrHelper.h    Include file for OS dependent functions
  Include\SrUsbXch.h    Include file with library function protos
  Include\SrUsbXchDrv.h Include file for items shared by library + driver
  Lib\SrHelper.c        Helper library functions to hide OS variations
  Lib\SrUsbXch.c        USBxCH library functions for accessing hardware
  Lib\SrUsbXch.hw       USBxCH hardware info

In order to run, you must also install the driver.  There are two sets
of files in the driver directory.  One set is used for Windows
(Win2K/XP) and the other is for Linux (Fedora 9, kernel
2.6.25-14.fc9.i686).  These files are all you need to install, remove,
and use the driver.  But you will not be able to re-compile the driver
without the complete SR software.  The latest version of the SR
software is always available free from the download page of our
website: www.symres.com

Linux users will almost certainly need to recompile the driver since
the Linux drivers are specific to the exact kernel rev.  In addition to
the SR software, a kernel source tree is also needed.  Please refer to the
troubleshooting section of the /usr/local/SR/USBXCH/Driver/ReadMe.txt file 
in the complete SR software for more details.

The files from the driver subdirectory that you need to install and run are:

Windows         Linux           Description
-------         -----           -----------
SrUsbXch.inf                    Plug and Play init file
W*.dll                          Windows Driver Framework functions
SrUsbXch.sys    SrUsbXch.ko     SrUsbXch Driver 
                indriver        Driver install utility
                rmdriver        Driver remove utility











