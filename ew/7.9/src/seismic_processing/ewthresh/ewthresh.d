 MyModuleId     MOD_EWTHRESH     # module id for this program
 InRing         TEST_RING           # transport ring to use for input
 OutRing        HYPO_RING           # transport ring to use for output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile; if 1, do
                                    # if 2, write to module log but not to 
                                    # stderr/stdout 
 MaxMsgSize     4096          # maximum size (bytes) for input/output msgs


 # Up to 1000 Threshold or ConvertedThreshold commands:
 # Threshold has 5 arguments, the SCNL and threshold value to compare against
 # threshold value is a real value (tracebuf2x scaling factor will be used if
 # present in the tracebuf2 header).
 # 
 Threshold 0202 BHZ QT TT 749.0     

 #
 # or Optional converted threshold where 6 values are SCNL then threshold
 # and conversion factor multiplier from counts to units of thresh
 # this can be used where input tracebuf2's are not of the tracebuf2x flavor
 #ConvertedThreshold STA HNZ NN LL 749.0 0.938

 DebugLevel 0	# set to 1 to see thresh messages on stderr, 
		# set to 2 to see every packets absolute converted level


# Optional:
# NOTE only ONE of these ThreshVotes lines can be specified:
# ThreshVotes NumTrig TimeSpanSeconds PreEventSeconds NumSCNLs SCNL1 SCNL2 ...
# Generate a triglist message whenever NumTrig alerts occur within TimeSpanSecconds
#    Time of message is PreEventSeconds before first trigger; listed SCNLs are included
#
# ThreshVotes 10 300 30 2 SSSS CCC NN LL ssss ccc nn ll

# Optional: Number of seconds to wait before sending another message for the same
# SCNL (skipped messages will be logged)
# If followed by SameNetSta, will compare against message from any station
# with the same Net and Sta (as opposed to the full SCNL)

# WaitBeforeNextThresh    60    SameNetSta

# Optional: Number of seconds to wait after startup before sending any messages.

# WaitBeforeMessages  60
