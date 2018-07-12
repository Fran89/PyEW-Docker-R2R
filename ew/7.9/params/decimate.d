#
#                     Configuration File for decimate
#
 MyModId         MOD_DECIMATE
 InRing             WAVE_RING    # Transport ring to find waveform data on,
 OutRing            WAVE_RING    # Transport ring to write output to,
                                 # InRing and OutRing may be the same.
 HeartBeatInterval  30           # Heartbeat interval, in seconds,
 LogFile            1            # 1 -> Keep log file, 0 -> no log file
                                 # 2 -> log to file but not to stderr/stdout
#Debug		 		 # Write out debug messages (optional)

#Quiet				 # Turn off gap and overlap messages to stderr (optional)


# Specify the decimation rates as one or more integer values greater than 1, 
# all on one "DecimationRates" line, enclosed in quotes.
# These will be used in successive stages of decimation. The overall
# decimation rate is the product of all the stage decomation rates.
# The filters will be more efficient if several small decimation stages
# are used instead of one large one, and if largest rates are given first.
# Currently a maximum of 10 stages is enforced.
#--------------------------------------------------------------------------
 DecimationRates    "5 2"     # Decimation rates of each stage, in quotes!

 MinTraceBuf         10       # Minimum number of samples in output TRACE_BUF.

 MaxGap              1.5      # Maximum gap, in sample periods, allowed
                              # between trace data points.
                              # When exceeded, channel is restarted.

#TestMode                     # If you want Decimate to compute and log 
                              # its filter coefficients and then
                              # exit, specify "TestMode".

# Specify logo(s) of the messages to grab from the InRing.
# Up to 10 GetWavesFrom commands may be used.
# Must specify installation ID, module ID and message type.
# Installation and module may be wildcarded.
# The only valid messages types are TYPE_TRACEBUF and TYPE_TRACEBUF2
# Output message type will be the same as the input message type. 
#--------------------------------------------------------------------------
 GetWavesFrom    INST_WILDCARD MOD_WILDCARD TYPE_TRACEBUF2 

# List SCNL codes of trace messages to decimate and their output SCNL codes.
# If old GetSCN command is used, location code is set to "--".
# Wildcard characters are not allowed here.
#--------------------------------------------------------------------------
#         Input-SCNL      Output-SCNL
 GetSCNL  ABC EHZ UW --   ABC EWZ UW 01



