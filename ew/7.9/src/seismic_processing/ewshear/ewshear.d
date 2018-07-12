#
#         ewshear Configuration File
#
 MyModuleId     MOD_EWSHEAR     # module id for this program
 InRing         TEST_RING           # transport ring to use for input
 OutRing        HYPO_RING           # transport ring to use for output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)


 LogFile        1                   # If 0, don't write logfile
 									# if 1, do (DEFAULT)
                                    # if 2, write to module log but not to 
                                    # stderr/stdout 
 MaxMsgSize     4096          # maximum size (bytes) for input/output msgs

#
# Floor definitions -- exactly one of each kind
#   Each specifies the SCNL for that Floor's sensor
#
 GroundFloor 5405 HNE NP ES
 TopFloor    5405 HNE NP GS

#
# Vertical distance between floors
#
 Height 100

#
# Length of sampling window (seconds)
#
 WindowSize 5
 
#
# Optional override of threshold for alert
#
# PctShearThreshold 25	# % change in velocity to trigger alert; default is 20

#
# Epsilon used in computing cross-correlation
# Default is 0
# Epsilon 

# Taper source data: Type Fraction
#		Type is either BARTLETT, HANNING, PARZAN, or BMHARRIS
#		Fraction is how much of each end of data is tapered
 Taper	HANNING 0.1
