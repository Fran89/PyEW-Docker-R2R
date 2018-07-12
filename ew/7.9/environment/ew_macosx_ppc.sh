# Create an Earthworm environment on MacOSX?!
# This file should be sourced by a bourne shell wanting 
# to run or build an EARTHWORM system under Linux.

# Set environment variables describing your Earthworm directory/version
export EW_HOME=/Users/earthworm
export EW_VERSION=earthworm_7.9
export SYS_NAME=`hostname`

# Set environment variables used by earthworm modules at run-time
# Path names must end with the slash "/"
export EW_INSTALLATION=INST_UNKNOWN
export EW_PARAMS=${EW_HOME}/run/params/
export EW_LOG=${EW_HOME}/run/logs/
export EW_DATA_DIR=${EW_HOME}/run/data/

# Tack the earthworm bin directory in front of the current path
export PATH=${EW_HOME}/$EW_VERSION/bin:$PATH


# Set environment variables for compiling earthworm modules
export GLOBALFLAGS="-D_MACOSX -D_SPARC -D_USE_PTHREADS -D_USE_SCHED -I${EW_HOME}/${EW_VERSION}/include"

export FC=g77

# max os x needs this for g77 linking
export FLFLAGS=export FLFLAGS=-lSystemStubs

export CFLAGS=$GLOBALFLAGS
export KEEP_STATE=""
