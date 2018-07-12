# Create an Earthworm environment on Linux!
# This file should be sourced by a bourne shell wanting 
# to run or build an EARTHWORM system under Linux.

# IMPORTANT NOTE, if running on a 64 bit linux system, long's can be 8 bytes and not 4 bytes!
# If this is the case, be sure to compile with the -m32 flag in the GLOBALFLAGS section!!!!

# Set environment variables describing your Earthworm directory/version
export EW_HOME=/usr/local/earthworm
export EW_VERSION=v7.4
export SYS_NAME=`hostname`

# Set environment variables used by earthworm modules at run-time
# Path names must end with the slash "/"
export EW_INSTALLATION=INST_UNKNOWN
export EW_PARAMS=${EW_HOME}/${EW_VERSION}/run_symres/params/
export EW_LOG=${EW_HOME}/${EW_VERSION}/run_symres/log/
export EW_DATA=${EW_HOME}/${EW_VERSION}/run_symres/data/

#
# Database (Oracle) related environment
#

# not used in linux build
#export SCHEMA_DIR=${EW_HOME}/${EW_VERSION}/src/oracle/schema-working
#export APPS_DIR=${EW_HOME}/${EW_VERSION}/src/oracle/apps
#export WEB_DOC_DIR=${EW_HOME}/${EW_VERSION}/WEB_DOC/DBMS/API-DOC

#
# Web server related environment
#
export WEB_DIR=${EW_HOME}/web

# Tack the earthworm bin directory in front of the current path
# Also add Oracle paths to the current path.
#set path=( ${EW_HOME}/${EW_VERSION}/bin /opt/oracle/bin /var/opt/oracle $path )
export PATH=${EW_HOME}/$EW_VERSION/bin\:$PATH

# Set up library path for dynamically loaded libraries:
#export OPENWINHOME   /usr/openwin
#export ORACLE_HOME   /opt/oracle
#export COMPILER_DIR /opt/SUNWspro
#export LD_LIBRARY_PATH="${OPENWINHOME}/lib:${ORACLE_HOME}/lib:${COMPILER_DIR}/lib:/usr/lib"

# Set environment variables for compiling earthworm modules
# add in -m32 if you are running a 64 bit linux architecture! Some EW codes may not work otherwise.
#
export GLOBALFLAGS="-m32 -Dlinux -D__i386 -D_LINUX -D_INTEL -D_USE_SCHED  -D_USE_PTHREADS -D_USE_TERMIOS -I${EW_HOME}/${EW_VERSION}/include"

# this is only needed by hyp2000_mgr!
export FORTRANFLAGS="-m32"
export CFLAGS=$GLOBALFLAGS
export KEEP_STATE=""
