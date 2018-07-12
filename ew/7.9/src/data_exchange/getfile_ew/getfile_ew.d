#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: getfile_ew.d 1122 2002-11-03 19:03:23Z lombard $
#
#    Revision history:
#     $Log$
#     Revision 1.2  2002/11/03 19:01:26  lombard
#     Added RCS header
#
#
#
#
#  getfile_ew.d  -  Configuration file for the getfile program
#
# REQUIRED CONFIGURATION PARAMETERS
#
#
# ServerIP      = IP address of this computer's host adapter
# ServerPort    = Well-known port used by this program
# TimeOut       = Send/receive will time out after TimeOut seconds.
# TempDir       = Full path name of directory to contain temporary files.
#                 This directory must be on the same partition as the
#                 client directories.
# Client        = IP address of trusted client, followed by the path of
#                 directory to contain files sent by this client.
#                 Connections from all other IP addresses are rejected.
#                 The client directories must be on the same disk
#                 partition as is TempDir.
#
ServerIP    136.177.31.72
ServerPort  3457
TimeOut     15
TempDir     /tmp/indir
Client      136.177.31.78   /tmp/indir/rocky
Client      130.118.43.38   /tmp/indir/campbell



#
# OPTIONAL PARAMETERS -- MUST SPECIFY ONE OF THE TWO SUBGROUPS
#

#
# Optional Standalone Setup
#

# LogFile       = If 0, don't log to screen or disk.
#                 If 1, log to screen only.
#                 If 2, log to disk only.
#                 If 3, log to screen and disk.
# LogFileDir    = Full path name to the directory where log files
#                 will be stored. This directory must exist.
LogFile       3
LogFileDir    /tmp/indir/log/


#
# Optional Earthworm Setup
#   If present, this module will attach to the OutRing and
#   beat its heart as prescribed. Otherwise, the module
#   works as a standalone program
#
#MyModuleId           MOD_ARC2TRIG    # module id for this instance of arc2trig
#OutRingName          HYPO_RING       # shared memory ring for output
#EwLogSwitch          1               # 0 to turn off disk log file; 1 to turn it on
                                      # 2 to log to module log but not to stderr/stdout
#HeartBeatInterval    30              # seconds between heartbeats

