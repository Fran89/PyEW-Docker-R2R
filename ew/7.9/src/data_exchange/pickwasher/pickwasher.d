
#   pickwasher configuration file
#
#
 MyModuleId     MOD_PICKWASHER	# module id for this program
 InRing         PICK_RING           # transport ring to use for input
 OutRing        ASSOC_RING          # transport ring to use for output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile
						# If 1, write to module log and stderr/stdout
                                    # If 2, write to module log but not stderr/stdout
 MaxMsgSize     128     	     	# maximum size (bytes) for input/output msgs
 RingSize       50          		# number of msgs to buffer
