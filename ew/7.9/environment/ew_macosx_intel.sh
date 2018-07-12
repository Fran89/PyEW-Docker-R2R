# Create an Earthworm environment on MacOSX?!
# This file should be sourced by a bourne shell wanting 
# to run or build an EARTHWORM system under Linux.

# Set environment variables describing your Earthworm directory/version

# Use value from elsewhere if defined (eg from .bashrc)
export EW_HOME="${EW_INSTALL_HOME:-/opt/earthworm}"
export EW_VERSION="${EW_INSTALL_VERSION:-earthworm_7.9}"
# Or set your own value directly
#export EW_HOME=/Users/earthworm
#export EW_VERSION=earthworm_7.9

# this next env var is required if you run statmgr:
export SYS_NAME=`hostname`

# Set environment variables used by earthworm modules at run-time
# Path names must end with the slash "/"
export EW_INSTALLATION=INST_UNKNOWN
export EW_PARAMS=${EW_HOME}/run/params/
export EW_LOG=${EW_HOME}/run/logs/
export EW_DATA_DIR=${EW_HOME}/run/data/

# Tack the earthworm bin directory in front of the current path
export PATH=${EW_HOME}/$EW_VERSION/bin:$PATH

# set EWBITS=64 to build for 64-bit target (note that with EWBITS=64
#  size of 'long' type changes from 4 bytes to 8 bytes)
export EWBITS=32

# warning flags for compiler:
export WARNFLAGS="-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unknown-pragmas -Wno-pragmas -Werror=format"
#  extra flags for enabling more warnings during code development:
#export WARNFLAGS="-Wall -Wextra -Wcast-align -Wpointer-arith -Wbad-function-cast -Winline -Wundef -Wnested-externs -Wshadow -Wfloat-equal -Wno-unused-parameter -Werror=format"

# Set environment variables for compiling earthworm modules
export GLOBALFLAGS="-m${EWBITS} -D_DARWIN_C_SOURCE -D_MACOSX -D_INTEL -D_USE_PTHREADS -D_USE_SCHED -I${EW_HOME}/${EW_VERSION}/include -D_GFORTRAN ${WARNFLAGS}"
export PLATFORM="LINUX" #Uses the same make options as Linux, so used here

# this is needed to get the mac linker to work with some modules.
export LDFLAGS=-Wl,-search_paths_first


# for intel Mac OS X I only found gfortran to work on 10.4.X with the hyp2000 modules, get it from http://hpc.sourceforge.net
export FC=gfortran
export FFLAGS=-m${EWBITS}
export CC=gcc
# the flag below is not needed with gfortran
#export FLFLAGS=-lSystemStubs
#
# Note I (Paul Friberg) got this hyp2000 and hyp2000_mgr to work using this version 
# $ gfortran -v
#Using built-in specs.
#Target: i386-apple-darwin8.10.1
#Configured with: /tmp/gfortran-20071231/ibin/../gcc/configure --prefix=/usr/local/gfortran --enable-languages=c,fortran --with-gmp=/tmp/gfortran-20071231/gfortran_libs --enable-bootstrap
#Thread model: posix
#gcc version 4.3.0 20071231 (experimental) [trunk revision 131236] (GCC) 


export CFLAGS=$GLOBALFLAGS
export KEEP_STATE=""
