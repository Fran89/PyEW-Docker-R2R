
#
# This is menlo_report's parameter file

#  Basic Earthworm setup:
#
MyModuleId         MOD_REPORT     # module id for this instance of report 
RingName           HYPO_RING      # ring to get input from
LogFile            1              # 0 to completely turn off disk log file
                                  # 2 to log to module log but not stderr/stdout
HeartBeatInterval  30             # seconds between heartbeats

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_EQPROC     # hyp2000arc & h71sum2k

# Set suffixes for output filenames
# NOTE: If a suffix is "none", no file will   
#       be reported for that type of message
ArcSuffix     ".arc"        # suffix for archive (hinvarc message) files
SumSuffix     "none"        # suffix for summary (h71sum message) files
TmpRemoteFile "report.tmp"  # name for remote file while it's being copied

# Create name for local file using the event origin time and
#   - last 2 digits of the event id and ArcSuffix/SumSuffix
#     this is the default
#     (i.e.  201011040930_57.sum)
#   - all digits of the event id, plus id-version,  and ArcSuffix/SumSuffix
#     (i.e.  201011040932_0000097658_2.arc)
#     to create long file names declare parameter EnableLongFileName
#     This parameter is useful only when KeepLocalCopy is also declared
# EnableLongFileName
# KeepLocalCopy

# Local directory to write temporary files in
LocalDir      c:\earthworm\run\log  

# Info about remote machine to write files on
RemoteHost    bruno                      # copy eq files to this machine 
RemoteDir     /we/earthworm/v2.3/rapid   # put them in this directory
RemoteUser    whoever                    # as this user
RemotePasswd  whatever                   # with this password; 
                                         #  Not used by Solaris or NT version
                                         #  (originally required for OS/2 version)
