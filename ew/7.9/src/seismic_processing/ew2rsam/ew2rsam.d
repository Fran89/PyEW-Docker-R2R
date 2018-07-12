
#
#                     Configuration File for ew2rsam
#
MyModId        MOD_EW2RSAM
InRing           WAVE_RING       # Transport ring to find waveform data on,
OutRing          TEST_RING       # Transport ring to write output to,
HeartBeatInterval     15         # Heartbeat interval, in seconds,
LogFile            1             # 1 -> Keep log, 0 -> no log file
#QueueSize         1000          # Size of message queue; default is 500 messages
#Debug			   0			 # OPTIONAL,  0-no debug
                                 #            1-basic debug
                                 #            2-super debug

# ReadTRACEBUF2		# Uncomment to read TRACEBUF2 messages
# WriteTRACEBUF2	# Uncomment to write TRACEBUF2 messages
                    # When writing TRACEBUF2 messages, the period indicator 
                    # is put in the Location field

#
# Specify logos of the messages to grab from the InRing.
# TYPE_TRACEBUF is assumed, therefore only module ID and 
# installation ID need to be specified
#
GetWavesFrom    INST_WILDCARD MOD_WILDCARD  # TYPE_TRACEBUF (assumed)

#
# SCN Selection 
#
#  Use the following two configuration options to narrow down
#  the SCNs of the wave messages of interest (wildcard '*' 
#  characters are allowed for both options):
#
#   o IncludeSCN (required) or IncludeSCNL
#   o ExcludeSCN (optional) or ExcludeSCNL
#
#
#  SCN which matches a line in the IncludeSCN section will be 
#  included, unless it also matches a line in the ExcludeSCN section
#
#  Examples:
#
#   1. Pick up only these SCNs
#
#       IncludeSCN HJG VHZ NC
#       IncludeSCN HJS VHZ NC
#       IncludeSCN KGM VHZ NC
#
#   2. Pick up all SCNs except for KGM VHZ NC
#
#       IncludeSCN * * * 
#       ExcludeSCN KGM VHZ NC
#
#   3. Pick up all SCNs except those with network code NC
#
#       IncludeSCN * * * 
#       ExcludeSCN * * NC

IncludeSCN * * *

# or if TBUF2s which have location codes use IncludeSCNL

IncludeSCNL * * * *

# if you want to exclude NC Location codes from TBUF2's that end in 02
# ExcludeSCNL * * NC 02

#
# MaxSCN - max number of SCNs or SCNLs to track
#
MaxSCN		50

#
# RsamPeriod - time period, in seconds, for RSAM computation 
#
RsamPeriod	2.56
RsamPeriod	60
RsamPeriod	600

