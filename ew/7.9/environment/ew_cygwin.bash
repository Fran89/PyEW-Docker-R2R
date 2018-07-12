# Create an Earthworm environment on Linux!
# This file should be sourced by a bourne shell wanting 
# to run or build an EARTHWORM system under Linux.

echo
echo "NOTE: Anything done with earthworm under cygwin is VERY experimental"
echo

# Set environment variables describing your Earthworm directory/version
export EW_HOME=/cygdrive/c/earthworm
export EW_VERSION=earthworm_7.9
export SYS_NAME=`hostname`

# Set environment variables used by earthworm modules at run-time
# Path names must end with the slash "/"
export EW_INSTALLATION=INST_UNKNOWN
export EW_RUN_DIR=${EW_HOME}/run_working
export EW_PARAMS=${EW_RUN_DIR}/params/
export EW_LOG=${EW_RUN_DIR}/log/
export EW_DATA_DIR=${EW_RUN_DIR}/data/

export PATH=${EW_HOME}/${EW_VERSION}/bin\:$PATH


# Set environment variables for compiling earthworm modules
# we're keeping the LINUX and related defines here, because having them helps more than
# hurts cygwin.  We do add a -D_CYGWIN for cygwin specific nuances

export GLOBALFLAGS="-D_CYGWIN -Dlinux -D__i386 -D_LINUX -D_INTEL -D_USE_SCHED  -D_USE_PTHREADS -D_USE_TERMIOS -I${EW_HOME}/${EW_VERSION}/include"

export CFLAGS=$GLOBALFLAGS
export KEEP_STATE=""
