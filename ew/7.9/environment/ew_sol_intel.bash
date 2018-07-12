# Create an Earthworm environment on Solaris using Bash!
# This file should be sourced by a bourne shell wanting 
# to run or build an EARTHWORM system under Linux.

# Use value from elsewhere if defined (eg from .bashrc)
export EW_HOME="${EW_INSTALL_HOME:-/opt/earthworm}"
export EW_VERSION="${EW_INSTALL_VERSION:-earthworm_7.9}"
EW_RUN_DIR="${EW_RUN_DIR:-$EW_HOME/run_working}"
# Or set your own value directly
#export EW_HOME=/opt/earthworm
#export EW_VERSION=earthworm_7.9
#export EW_RUN_DIR=$EW_HOME/run_working

export SYS_NAME=`hostname`

# Set environment variables used by earthworm modules at run-time
# Path names must end with the slash "/"
export EW_INSTALLATION=INST_UNKNOWN
export EW_PARAMS="$EW_RUN_DIR/params/"
export EW_LOG="$EW_RUN_DIR/log/"
export EW_DATA_DIR="$EW_RUN_DIR/data/"


# Tack the earthworm bin directory in front of the current path
export PATH="$EW_HOME/$EW_VERSION/bin:$PATH"


# type of compile
#export EWBITS=32
export EWBITS=64
export GLOBALFLAGS="-m${EWBITS} -D_INTEL -D_SOLARIS -I${EW_HOME}/${EW_VERSION}/include"
export PLATFORM="SOLARIS"

export KEEP_STATE=""

# Set initial defaults
export CFLAGS=$GLOBALFLAGS
export CPPFLAGS=$GLOBALFLAGS
# be explicit about which compiler to use
export CC=`which cc`
# new and needed in places for floor() math library when chron3.o is linked
export LINK_LIBS="-lm"

# eventually replace with gfortran when we move away from Forte compilers
# Auto-detect fortran compiler and flags
if which f77 1> /dev/null 2>&1
then
    export FC=`which f77`
fi

# Alternatively, you can hard-code values here:
#export FC='...'
export FFLAGS=-m${EWBITS}
