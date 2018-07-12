 MyModuleId     MOD_EWACCEL     # module id for this program
 InRing         TEST_RING           # transport ring to use for input
 OutRing        HYPO_RING           # transport ring to use for output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile; if 1, do
                                    # if 2, write to module log but not to 
                                    # stderr/stdout 
 MaxMsgSize     4096          # maximum size (bytes) for input/output msgs

#
# Floor definitions
#   Each specifies the mass, SCNL & conversion factor for that Floor
#
#   There must be exactly 1 ground floor
 GroundFloor 100 5405 Hde NP BS 1.5

#   There can be an arbitrary number of non-ground floors, but each must have
#   a distinct identifier (first arg to Floor command)
 Floor 1 100 5405 Hde NP BS 1.5

BaseShearThreshold 2.5	# force threshold to trigger alert

