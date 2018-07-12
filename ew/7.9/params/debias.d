#
#                     Configuration File for debias
#
# debias removes a mean value over a specified time window
#			it DOES NOT perform any other actions on the data.
#
 MyModId            MOD_DEBIAS
 InRing             WAVE_RING # Transport ring to find waveform data on,
 OutRing            HYPO_RING # Transport ring to write output to,
                              # InRing and OutRing may be the same.
 HeartBeatInterval  30        # Heartbeat interval, in seconds,
 LogFile            1         # 1 -> Keep log file, 0 -> no log file
                              # 2 -> log to file but not to stderr/stdout
# Debug		 		           # Write out debug messages (optional)

 MinTraceBuf         10       # Minimum number of samples in output TRACE_BUF.

 MaxGap              1.5      # Maximum gap, in sample periods, allowed
                              # between trace data points.
                              # When exceeded, channel is restarted.

# TestMode                     # If you want Debias to compute and log 
                              # its filter coefficients and then
                              # exit, specify "TestMode".

 AvgWindow   5                # Number of seconds of data to use to compute
                              # the rolling average, which gets removed from
                              # each sample

# GetAllSCNLs                  # Process packets w/ SCNLs not covered by a GetSCNL
                              # command (see below) w/o changing the SCNL

# Specify logo(s) of the messages to grab from the InRing.
# Up to 10 GetWavesFrom commands may be used.
# Must specify installation ID, module ID and message type.
# Installation and module may be wildcarded.
# The only valid messages types are TYPE_TRACEBUF and TYPE_TRACEBUF2
# Output message type will be the same as the input message type. 
#--------------------------------------------------------------------------
 GetWavesFrom    INST_WILDCARD MOD_WILDCARD TYPE_TRACEBUF2 

# List SCNL codes of trace messages to debias and their output SCNL codes.
# If old GetSCN command is used, location code is set to "--".
# Wildcard characters are not allowed here.
#--------------------------------------------------------------------------
#         Input-SCNL      Output-SCNL
 GetSCNL  NP2 BHE ZZ --   NP2 BHX ZZ 01
