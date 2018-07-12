FILE: readme.txt                Copyright (c), Symmetric Research, 2007-2010

The run_16chan directory is to help configure SrPar2Ew so it can
acquire data from 16 channels on one machine.  It requires two PAR8CH,
one PARGPS, and a PC with two parallel ports and one serial port.  The
params directory contains a consistent set of earthworm .d and .desc
files appropriate for a basic version of this configuration using
Symmetric Research equipment.  General users or those interested in
more complex configurations are advised to start from the Memphis Test
suite instead.  It can be found at:
folkworm.ceri.memphis.edu/ew-dist/v7.2/earthworm_test.memphis.tar.gz

In this configuration, all the modules use INST_UNKNOWN as their
institution and run on the same machine using the loopback device
127.0.0.1 as their IP address.  The main earthworm startstop module is
used to start and stop earthworm and control the other modules while
statmgr is used to track the status of all the modules.  The
SrPar2Ew module acquires data and the wave_serverV module saves
it on disk.  Wave_viewer is used to display the data.  It is run
stand-alone in a separate command prompt window and is not controlled
by startstop.  You may substitute Swarm for viewing the data instead
of using wave_viewer if you prefer.

The following steps describe how to run EW7.4 with 2 PAR8CH + 1 PARGPS.
Linux users should replace references to c:\earthworm with
/usr/local/earthworm:

1. Install the official version of Earthworm v7.4
   A. Download the "Windows Binary (zip)" from
      folkworm.ceri.memphis.edu/ew-dist/index.html
      the file is named v7.4-win-bin.zip.  Linux users should get
      the Linux tar file instead.
   B. Create the directory c:\earthworm and unzip v7.4 into this
      directory.

2. Install the latest version of the PARxCH with GPS software.
   A. The files containing this software are named pxgpsw2k.zip for the 
      Windows version or pxgpslnx.tar for the Linux version.
      You can download them free from our website www.symres.com.
   B. Install it.  The install just copies the software to the
      \sr\parxch and \sr\pargps directories.  You must then manually
      install the device drivers using the indriver utilities in the
      respective driver subdirectories.
   C. Open a command prompt, change to \sr\pargps\driver and run
      indriver SrParGps0 to install the PARGPS driver.  Linux users
      should also specify a com port.  For example:
      indriver SrParGps0 com1
   D. Change to \sr\parxch\driver.  Since you have two PAR8CH you must
      install two copies of the driver.  To do the install you will need
      to know the port address and irq for each parallel port.  
      Under Windows, you can find this information in the Device Manager 
      by looking under the Ports class and viewing the properties of the
      parallel or printer ports you will be using.  While there, be sure
      to enable interrupts for the primary parallel port (the one on the
      motherboard).  
      Under Linux, typing "cat /proc/ioports | grep parport" without the 
      quotes will show you the addresses of all the parallel ports in your 
      system.  For add-in parallel port cards, you can also use the command 
      lspci -vvv to see what resources (eg addresses) are used by your PCI 
      card.  The parallel port address will vary depending on which PCI slot 
      your card is in and on how much PCI functionality is on your motherboard.
      Remember when using the Linux indriver to install the 2nd PARxCH driver, 
      you must explicitly include a major number since the first driver will be
      using the default of 123.  For example:
         indriver PAR8CH 0x378 7
         indriver PAR8CH 0xF400 17 125

3. Connect up the PARxCH and PARGPS hardware.
   A. Make the appropriate physical connections for both PAR8CH.  They
      should each have power, a parallel port cable hooked up to their
      respective DB25, and digital inputs #0 and #3 (DB15 pins 1 and 4)
      connected to the corresponding pins on the PARGPS DB15 connector.  
      Digital input #3 carries the PPS signal and digital input #0 
      contains the related toggle signal which is used to synchronize
      the start of the two boards.  The PARGPS should of course also have 
      power, a serial port cable to a COM port, and an antenna hooked up.
   B. We also recommend that you duplicate the signal from one sensor
      so you can send an identical signal to a channel on BOTH PAR8CH A/D 
      boards.  This will provide an extra check so you can tell if the 
      two boards ever lose synch for any reason.

4. Test the PARxCH and PARGPS software and hardware installation
   A. Run PARxCH diag to verify both PAR8CH, and then run PARGPS diag to
      verify that the primary PAR8CH (the one on the motherboard port with
      interrupts) is properly connected to the PARGPS.  
   B. You may also want to run the scope or simp program on each PAR8CH
      separately to acquire a little data, followed by the dat2asc program
      in \sr\parxch\convert\ascii.  Then you can inspect the ASCII
      data in any text editor.  For both PAR8CH, you should see channel 9
      (digital inputs) alternate between 0 and 1 with the PPS toggle and
      channel 11 (the OBC or On Board Count) should be non-zero once every
      second.  For the primary PAR8CH, you should also see channel 10 (PPS
      Mark) be non-zero once a second.

5. Update srpar2ew in the earthworm directory from step 1 to the latest
   as follows:
   A. Go to the c:\earthworm\v7.4\src\data_sources\srpar2ew directory
   B. Delete the contents of this directory
   C. Replace them with the files in \sr\parxch\extras\earthworm
   D. Copy run_v7.4\srpar2ew.exe to c:\earthworm\v7.4\bin\.

6. Set up your run_symres directory
   A. Go to c:\earthworm\v7.4 and create the run_symres directory
   B. Go to c:\earthworm\v7.4\run_symres and create directories data, logs,
      and params
   C. Fill the params directory with the files in run_16chan\params
   D. Review the srpar2ew.d file and edit the AtodModelName,
      PortMode and GpsSerialPort to be appropriate for your equipment.
      The standard keyword AtodDriverName identifies the primary PAR8CH 
      which should be on the motherboard port with interrupts enabled.
      The new keywords AtodDriverNameN, AtodModelNameN, and PortModeN
      identify the secondary PAR8CH which can be on the add-in card port.

7. Start earthworm
   A. Open a command prompt or xterm and go to c:\earthworm\v7.4\run_symres\params
   B. Run ew_nt.cmd to set the earthworm environment parameters for
      Windows or ew_linux.bash for Linux.
   C. Type startstop.  This should start 
      statmgr to monitor errors
      srpar2ew to acquire data
      wave_serverV to store the acquired data

8. View the data
   A. Open another command prompt and go to c:\earthworm\v7.4\run_symres\params 
   B. Set the earthworm environment parameters by running ew_nt.cmd or ew_linux.bash
   C. Type wave_viewer wave_viewer.d
      This should run wave_viewer which asks wave_serverV for the
      acquired data and displays it on the screen.
   D. Wave_viewer is Windows only, so Linux users will need to use
      a program like Swarm to view the data instead.  Swarm is available
      from www.avo.alaska.edu/Software.  It uses Java and can be run under
      Windows too.

9. Check the logs to see if there are any problems, 
   A. Review the contents of the log files in C:\earthworm\v7.4\run_symres\log

10. Once you have verified that EW and PARxCH work together, you can
   modify station names, IP addresses, etc.  You can also add additional
   earthworm modules for picking, locating, etc.  If you have questions
   at this stage, the best place to get answers is the Earthworm Users
   Group.  This is a mailing list run out New Mexico Tech for users of
   the earthworm system.  The people are very friendly and give good
   advice based on many years of experience with running earthworm in
   real-world situations.

Questions? Issues? 
Subscribe to the Earthworm Google Groups List.
http://groups.google.com/group/earthworm_forum?hl=en
