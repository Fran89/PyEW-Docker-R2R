#
# ew2seisvole's configuration file.
# This module was derived from cubic_msg. It interfaces to Alan Jones'
# Seisvole display program: it leaves files containing cubic messages in
# the directory specified below. The idea is that Seisvole is hopefully
# configured to scan this directory (every 10 seconds), and read and
# delete any files it finds there.  Seisvole builds its own internal
# cataloge of events from that.  Lucky Vidmar, Feb 1999.
#
MyModuleId      MOD_EW2SEISVOLE # module id for this instance of cubic_msg
RingName        HYPO_RING       # trasport ring for I/0
LogSwitch       1               # 0 to turn off logging to disk; 1 to turn it on
                                # to 2 log to module log but not to stderr/stdout

# List the message logos to grab from transport ring
#               Installation    Module          Message Type
GetSumFrom      INST_WILDCARD   MOD_WILDCARD      TYPE_H71SUM2K

# Read the magnitude threshold
#
MagThreshold    1.0

#
# Into what directory (full path) should the cubic files be written.
# Make sure that the name contains the appropriate trailing slash,
# i.e. / on solaris and \ on NT.
#
#
#TargetDirectory c:\cubic\
TargetDirectory /tmp/cubic/

