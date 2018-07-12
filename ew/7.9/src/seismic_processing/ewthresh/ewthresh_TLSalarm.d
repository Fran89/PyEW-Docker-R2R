 MyModuleId     MOD_EWTHRESH     # module id for this program
 InRing         WAVE_RING           # transport ring to use for input
 OutRing        TRIG_RING           # transport ring to use for output
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
# Threshold APE HHZ GE -- 749.0     

 #
 # or Optional converted threshold where 6 values are SCNL then threshold
 # and conversion factor multiplier from counts to units of thresh
 # this can be used where input tracebuf2's are not of the tracebuf2x flavor
# TMO07: conversion factor cnt -> m/s = 1/1.5e9  --> 0.6667 is conversion factor for cnt -> nm/s

# conversion factors
#QN1  (z/n/e):  8.20662024    8.21348010    8.20480011 
#QN2  (z/n/e):  8.20582045    8.20596028    8.20622006 
#QN3  (z/n/e):  8.19636057    8.21044011    8.20372009 
#QN4  (z/n/e):  8.20066020    8.19612012    8.19130037 

ConvertedThreshold QC1 HHZ QC 01 200000.0 5.9605     % this corresponds to a threshold of ~ 200.000 nm/s
ConvertedThreshold QN1 HHZ QC 01 200000.0 8.2066     % this corresponds to a threshold of ~ 200.000 nm/s
ConvertedThreshold QN2 HHZ QC 01 200000.0 8.2058     % this corresponds to a threshold of ~ 200.000 nm/s
ConvertedThreshold QN3 HHZ QC 01 200000.0 8.1964     % this corresponds to a threshold of ~ 200.000 nm/s
ConvertedThreshold QN4 HHZ QC 01 200000.0 8.2007     % this corresponds to a threshold of ~ 200.000 nm/s

#ConvertedThreshold TMO07 HHZ FI 10 20000.0 0.6667     % this corresponds to a threshold of ~ 20.000 nm/s
ConvertedThreshold TMO07 HHZ KB 10 20000.0 0.6667     % this corresponds to a threshold of ~ 20.000 nm/s

 DebugLevel 2	# set to 1 to see thresh messages on stderr, 
		# set to 2 to see every packets absolute converted level

# or Optional converted threshold where 6 values are SCNL then threshold
# and conversion factor multiplier from counts to units of thresh
# this can be used where input tracebuf2's are not of the tracebuf2x flavor

# Optional:
# NOTE only ONE of these ThreshVotes lines can be specified:
# Generate a triglist message whenever NumTrig alerts occur within TimeSpanSecconds
#    Time of message is PreEventSeconds before first trigger; listed SCNLs are included
#
# ThreshVotes NumTrig TimeSpanSeconds PreEventSeconds NumSCNLs SCNL1 SCNL2 ...
ThreshVotes     3          1.5               0.1          4     QN1 HHZ QC 01 QN2 HHZ QC 01  QN3 HHZ QC 01  QN4 HHZ QC 01

# define traffic-light threshold exeedances for coincident trigger (yellow/red)
Threshold_yellow 200000
Threshold_red    400000

# define associated alarm sounds to play if threshold exeedances (yellow/red) are observed
Alarmsound_yellow	alarm_yellow.wav
Alarmsound_red   	alarm_red.wav


