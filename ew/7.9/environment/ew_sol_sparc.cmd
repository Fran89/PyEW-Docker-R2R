# Create an Earthworm environment on Solaris!
# This file should be sourced by any shell wanting 
# to run or build EARTHWORM system.

# Set environment variables describing your Earthworm directory/version
setenv EW_HOME          /home/earthworm
setenv EW_VERSION       earthworm_7.9
setenv SYS_NAME         `hostname`

# Set environment variables used by earthworm modules at run-time
# Path names must end with the slash "/"
setenv EW_INSTALLATION  INST_UNKNOWN
setenv EW_PARAMS        ${EW_HOME}/run_working/params/
setenv EW_LOG           ${EW_HOME}/run_working/log/
setenv EW_DATA_DIR      ${EW_HOME}/run_working/data/


# Tack the earthworm bin directory in front of the current path
set path=( ${EW_HOME}/${EW_VERSION}/bin $path )

# Set up library path for dynamically loaded libraries:
setenv OPENWINHOME   /usr/openwin
setenv COMPILER_DIR  /opt/SUNWspro
if ("${?LD_LIBRARY_PATH}" == 0) then
	setenv LD_LIBRARY_PATH "${OPENWINHOME}/lib:${COMPILER_DIR}/lib:/usr/lib"
else
	setenv LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:${OPENWINHOME}/lib:${COMPILER_DIR}/lib:/usr/lib"
endif

# set the type of compile
#export EWBITS=32
export EWBITS=64

# Set environment variables for compiling earthworm modules
setenv GLOBALFLAGS "-m${EWBITS} -D_SPARC -D_SOLARIS -I${EW_HOME}/${EW_VERSION}/include"
setenv PLATFORM "SOLARIS"
setenv LINK_LIBS "-lm"
setenv CFLAGS "$GLOBALFLAGS"
setenv CPPFLAGS "$GLOBALFLAGS"
setenv FFLAGS "-m$EWBITS"

# Create an alias for making executables
alias emake   'make -f makefile.sol'
alias make_ew 'make_ew_solaris'

# Number of available file descriptors
limit descriptors 256
