# this modules listens for TYPE_HYP2000ARC messages only and compares them
#------------------------------------------------------------------
 MyModuleId      MOD_HYPASSOC
 InRing		 TEST_RING    # shared memory ring for input
 OutRing 	 TOTO_RING    # shared memory ring for output
 HeartBeatInt    30

 LogFile         1            # 0 => don't create a disk log file.
                              # 1 => write a disk log file.
                              # 2 => log to disk file but not stderr/stdout

 Debug                        # Debug info will be logged if uncommented

# What message logos to listen to. Can be more than one.
# The type is hard coded to TYPE_HYP2000ARC
#-------------------------------------------------------
 GetEventsFrom   ${EW_INST_ID}  MOD_WILDCARD

# Output control
#---------------

 OutDir       /home/ew/run_prod   # base directory for output data files
 
 DelayTime 30             #  maximum time difference between two detections
                          #  that we could consider as a single event

 WaitTime 10              #  maximum time we wait to be sure of having received
                          #  any detection before sending the message
                          #  on the output ring

#CheckLocCode             # Optional: check also for location code match when looking
			  # for new phases
			  # Default : don't check

# Optional queue commands
#------------------------
#QueueSize 5              # Optional: # trigger msgs to hold in queue; 
                          # default is 10

 QueueFile hypAssoc.que   # Optional: queue dumpfile name, for saving state of queue


# THE END
