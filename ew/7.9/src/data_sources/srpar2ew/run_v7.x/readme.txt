FILE: readme.txt                Copyright (c), Symmetric Research, 2007-2010

The run_v7.x directory contains versions of SrPar2Ew compiled
with the Earthworm 7.4 headers for Windows (.exe) and Linux (no
extension).  The params directory contains a consistent set of
earthworm .d and .desc files appropriate for one of the most simple
configurations using Symmetric Research equipment.  General users or
those interested in more complex configurations are advised to start
from the Memphis Test suite instead.  It can be found at:
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

The following steps describe how to run EW7.4 with PARxCH + PARGPS.
Linux users should replace references to c:\earthworm with
/usr/local/earthworm:

1. Install the official version of Earthworm v7.4
   A. From folkworm.ceri.memphis.edu/ew-dist/
      download the "Windows Binary (zip)" v7.4/v7.4-win-bin.zip
      or the "Linux Binary (tgz)" v7.4/v7.4-linux-bin.tar.gz.
      Linux users may prefer getting the source from
      "Source Code (tgz)" v7.4/v7.4-src.tar.gz and recompiling it for
      themselves if their Linux version does not exactly match the
      provided binary.
   B. Create the directory c:\earthworm, unzip v7.4 into this
      directory, and change the names if needed so you have
      c:\earthworm\v7.4 as the earthworm home directory.

2. Install the latest version of the PARxCH with GPS software.
   A. The files containing this software are named pxgpsw2k.zip for the
      Windows version or pxgpslnx.tar for the Linux version.
      You can download them free from our website www.symres.com.
   B. Install it.  Don't forget to run both PARxCH indriver and
      PARGPS indriver.  You may have to run rmdriver first to clear out any
      previous drivers first.
   C. Test it by running both PARxCH and PARGPS diags and PARxCH scope.

3. Update srpar2ew in the earthworm directory from step 1 to the latest
   as follows:
   A. Go to the c:\earthworm\v7.4\src\data_sources\srpar2ew directory
   B. Delete the contents of this directory
   C. Replace them with the files in \sr\parxch\extras\earthworm
   D. Copy run_v7.x\srpar2ew.exe to c:\earthworm\v7.4\bin\.

4. Set up your run_symres directory
   A. Go to c:\earthworm\v7.4 and create the run_symres directory
   B. Go to c:\earthworm\v7.4\run_symres and create directories data, log,
      and params
   C. Fill the params directory with the files in run_v7.x\params
   D. Review the srpar2ew.d file and edit the AtodModelName,
      PortMode and GpsSerialPort to be appropriate for your equipment.

5. Start earthworm
   A. Open a command prompt or xterm and go to c:\earthworm\v7.4\run_symres\params
   B. Run ew_nt.cmd to set the earthworm environment parameters for
      Windows or ew_linux.bash for Linux.
   C. Type startstop.  This should start
      statmgr to monitor errors
      srpar2ew to acquire data
      wave_serverV to store the acquired data

6. View the data
   A. Open another command prompt and go to c:\earthworm\v7.4\run_symres\params
   B. Set the earthworm environment parameters by running ew_nt.cmd or ew_linux.bash
   C. Type wave_viewer wave_viewer.d
      This should run wave_viewer which asks wave_serverV for the
      acquired data and displays it on the screen.
   D. Wave_viewer is Windows only, so Linux users will need to use
      a program like Swarm to view the data instead.  Swarm is available
      from www.avo.alaska.edu/Software.  It uses Java and can be run under
      Windows too.

7. Check the logs to see if there are any problems,
   A. Review the contents of the log files in C:\earthworm\v7.4\run_symres\log

8. Once you have verified that EW and PARxCH work together, you can
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
