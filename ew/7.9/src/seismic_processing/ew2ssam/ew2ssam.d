#
#                     Configuration File for ew2ssam
#
MyModId        MOD_EW2SSAM
InRing           WAVE_RING       # Transport ring to find waveform data on,
OutRing          HYPO_RING       # Transport ring to write output to,
HeartBeatInterval     15         # Heartbeat interval, in seconds,
LogFile            1             # 1 -> Keep log, 0 -> no log file
Debug			   3			 # OPTIONAL,  0-no debug
                                 #            1-basic debug
                                 #            2-super debug
                                 #            3-super-duper debug


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
#   o IncludeSCN (required)
#   o ExcludeSCN (optional)
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

#IncludeSCN HJG VHZ NC
IncludeSCN KGM VHZ NC

#
# MaxSCN - max number of SCNs to track
#
MaxSCN		50

#
# SsamPeriod - time period, in seconds, for SSAM computation 
#
#SsamPeriod	2.56
SsamPeriod	60
#SsamPeriod	600
 

#  Ssam taper width - defines the width of the Hanning taper.  
#  Data is tapered before being sent to the fft function.
#  The taper is applied simultaneously from both ends of the data.
#  For SsamTaper equal to 0.0 no taper is applied.  
#  For SsamTaper equal to 0.5 a full Hanning Taper is applied.
SsamTaper       0.0
