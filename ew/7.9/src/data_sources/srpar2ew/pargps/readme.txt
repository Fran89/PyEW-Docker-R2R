FILE: readme.txt                 Copyright (c), Symmetric Research, 2007-2009

This directory contains a few files from the SR PARxCH with PARGPS
software that are required for recompiling and running the
SrPar2Ew module.  Of course, you won't be able to acquire any
data unless you have the actual SR PARxCH 24 bit A/D and PARGPS timing
hardware.  

The files from this directory that you need to compile are:

  File                  Description
  ----                  -----------
  lib\pargps.c          PARGPS library functions for accessing hardware
  include\pargps.h      Include file with library function protos
  include\pargpskd.h    Include file for items shared by library + driver

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
troubleshooting section of the /usr/local/sr/pargps/driver/readme.txt file 
in the complete SR software for more details.

The files from the driver subdirectory that you need to install and run are:

Windows         Linux           Description
-------         -----           -----------
indriver.exe    indriver        Driver install utility
rmdriver.exe    rmdriver        Driver remove utility
showdriver.exe  showdriver      Installed driver info utility
setmodel.exe    setmodel        Utility to change default GPS model
                setmodelbase    Linux template file for setmodel
                clrmodel        Linux utility to clear model environment variable
                clrmodelbase    Linux template file for clrmodel
srgps0.sys      SrParGps0.ko    Driver 0
srgps1.sys      SrParGps1.ko    Driver 1
srgps2.sys      SrParGps2.ko    Driver 2
srgps.sys       SrParGps.ko     Driver 3

See the readme.txt in the ..\parxch directory for additional info.
