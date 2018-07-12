#
#                     Configuration File for pick_recorder
#
MyModId        MOD_PICK_RECORDER
InRing           PICK_RING       # Transport ring to find PICK data on,
HeartBeatInterval     15         # Heartbeat interval, in seconds,
LogFile                1         # 1 -> Keep log, 0 -> no log file
                                 # 2 to log to module log but not to stderr/stdout
#Debug		 0		 # OPTIONAL,  0-no debug
                                 #            1-basic debug
                                 #            2-super debug


#
# Specify logos of the messages to grab from the InRing.
# TYPE_PICK2K and TYPE_PICK_SCNL are assumed, therefore only module ID and 
# installation ID need to be specified. Specify as many as necessary.
#
GetPicksFrom    INST_WILDCARD MOD_WILDCARD  # TYPE_PICK2K (assumed)


#
# TargetDir - directory where picks will be written
#
TargetDir ./pick_dir
