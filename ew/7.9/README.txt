
OVERVIEW

Earthworm is a free open source waveform and automatic Earthquake processing software package written primarily in the C language.

Originally developed by the United States Geological Survey (USGS), Earthworm now has modules contributed by users all over the world.
 
Earthworm has been an active open source project since 1993. In the last number of years, the USGS has funded continued releases and maintenance by ISTI. University of Memphis and ISTI host the Earthworm documentation and distribution.

Digitizers from different hardware manufacturers export their data into shared memory rings

Various open source processing modules can operate in conjunction on shared memory data

Earthworm provides a common protocol for regional networks to share waveforms, picks, etc.

If you have at least 4 stations, Earthworm can attempt to associate phase arrivals and compute earthquake locations.

DOCUMENTATION

The Earthworm documentation can be found within this distribution at 

doc/WEB_DOC/index.html

The same documentation can be found
online at 
http://folkworm.ceri.memphis.edu/ew-doc/
and
http://isti2.com/ew/

GET EARTHWORM

If a software distribution is available for your operating system and hardware architecture, the easiest thing is to download the Earthworm software from
http://folkworm.ceri.memphis.edu/ew-dist/
or
http://www.isti2.com/ew/distribution/

If your operating system isn't listed, you may try and compile the binary executables for Earthworm yourself. (See COMPILING below.)

GETTING STARTED

Earthworm Minimum Setup Overview:
- Create parameter and log directories
- Acquire or decide on an installation id
- Set environment variables (run script)
- Add all modules you want to use to the startstop configuration file
- Make sure all modules you want to use have a module ID in the earthworm.d config file
- Add all modules you want to monitor to the statmgr configuration
- Configure modules


The easiest way to get started with Earthworm is to run an existing Earthworm configuration, see how it works, and then modify it for your own needs. Start with the Memphis Test Suite, you can get it from:
http://www.isti2.com/ew/distribution/earthworm_test.memphis.tar.gz
or
http://www.isti2.com/ew/distribution/earthworm_test.memphis.zip
or possibly from
http://folkworm.ceri.memphis.edu/ew-dist/v7.5/earthworm_test.memphis.tar.gz

Read the README within the test suite, and try running the test suite. Note that a minor difference in the depth value (ie: 11.74 vs. 11.71) are acceptable.

Once you're done with that, and you've gotten the correct test results, you can request a new Installation ID from Mitch. (See instructions at the top of earthworm_global.d for how to do this. earthworm_global.d lives in the environment directory of the distribution, but needs to be placed in your parameters directory to become active) The new Installation ID will need to go in your .desc files as sell as some .d files. You can modify the files in the Test Suite params directory to replace INST_MEMPHIS with your installation ID, or you can get a fresh set of parameters from the distribution's 'params' directory. 

It's cleaner to only copy over parameters for the programs that you need to your 'params' directory from the distribution's 'params' directory. 

Two key programs are the core of Earthworm. Startstop and statmgr. Startstop contains the command line to run each module you'd like to use, and specifies the module name and the corresponding .d configuration file. Statmgr monitors the health of all running earthworm programs, as configured in it's own .d file. It uses the .desc file for each module to configure how it runs. 

See the module documentation in ewdoc/WEB_DOC/index.html for more on configuring startstop, statmgr or other modules

As you've seen from the test suite, just type 'starstop' to run Earthworm. (To have Earthworm automatically start, you can make a unix startup script for /etc/init.d or you can use Starstop Service on Windows.)

If Earthworm doesn't run properly, check the logs directory if the error output of startstop isn't helpful. If you can't figure it out, join and then email the Google Groups Earthworm list
http://groups.google.com/group/earthworm_forum

If you want to learn more about Earthworm and the documentation isn't enough, or if you want to learn how to program your own Earthworm modules, ISTI runs an Earthworm class approximately yearly. Contact ISTI to see about the current schedule, availability and cost for the course. http://www.isti.com/contact-info

COMPILING

See the src/README for a some additional information.

If you want to compile the Earthworm binaries yourself, you must run or source the ew_* file ("environment file") appropriate to your platform. It's located in the environment directory; you may copy it to your params directory.

ew_cygwin.bash   ew_macosx_intel.sh   ew_nt.cmd   ew_sol_sparc.bash ew_linux.bash   ew_macosx_ppc.sh    ew_sol_intel.cmd  ew_sol_sparc.cmd
Note that ew_nt is for all Microsoft Windows operating systems.

Before you run or source your ew_* file, you must make sure all the directories actually exist that are specified by EW_HOME EW_PARAMS and EW_LOG. Also, you may need to specify the compiler you've got installed, especially if you're using Windows. The directory path must match what you've got. If you don't have a Fortran compiler you won't be able to compile Hypoinverse used to locate earthquakes, but you should be able to compile most other modules. You may need to comment hypoinverse out of the makefiles.

To compile, after you've run or sourced your environment file, run the following command, depending on your platform.
nmake nt --- For Windows
make unix  --- this is for the many flavors of Linux or MacOSX or Solaris
