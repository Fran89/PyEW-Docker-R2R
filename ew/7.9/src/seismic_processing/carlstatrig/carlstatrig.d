#
# CarlStaTrig's Parameter File
#
#  Basic Earthworm Setup
#-----------------------
MyModuleId      MOD_CARLSTATRIG # Module id for this instance of CarlStaTrig
                                # - REQUIRED
RingNameIn      WAVE_RING       # Name of ring from which trace data will be
                                #   read - REQUIRED.
RingNameOut     PICK_RING       # Name of ring to which triggers will be
                                #   written - REQUIRED.
HeartBeatInterval       0       # Heartbeat Interval (seconds). REQUIRED

# Specify logos of waveforms to look at:  OPTIONAL 
#   GetWavesFrom <installation_id> <module_id> 
# The message_type is hardcoded to TYPE_TRACEBUF2.
# Use as many GetLogo GetWavesFrom as you need. If no GetWavesFrom 
# commands are given, carlstatrig will default to:
# "GetWavesFrom INST_WILDCARD MOD_WILDCARD"
#-----------------------------------------------------------------
GetWavesFrom  INST_WILDCARD  MOD_WILDCARD   # TYPE_TRACEBUF2

# Set debug log message level: OPTIONAL (default: 0)
#   0  log transport errors, changes in datatype, samplerate, failure to flush
#   1  above plus large gaps and overlaps 
#   2  above plus small gaps and overlaps, station trigger changes - 
#      best for beginning installations
#   3  above plus station trigger values and messages 
#   4  above plus "unable to find station..." 
#   5  above plus many details of message handling - very verbose. 
#----------------------------------------------------------------------
Debug 1


# CarlTrig Specific Setup
#------------------------
StationFile     "pnsn_trig.sta" # Name of file containing station information -
                                #   REQUIRED.
MaxGap          1.5             # Maximum gap between trace data points that
                                #   can be interpolated (otherwise restart the
                                #   station). OPTIONAL (default MaxGap = 1)
StartUp         7               # Minimum seconds of trace data needed to
                                #   before using LTAs REQUIRED.
LTAtime         8               # Number of seconds for LTA average
                                # OPTIONAL (default LTAtime = 8)
# STAtime         3             # Number of seconds for short term average
                                # OPTIONAL (integer; default STAtime = 1)

Decimation      1               # Decimation factor used in averages
                                # OPTIONAL (default Decimation = 1)
Ratio           2.3             # Carl Trigger parameter: enumer / edenom
                                # REQUIRED
Quiet           4.0             # Carl Trigger equiet parameter - REQUIRED
