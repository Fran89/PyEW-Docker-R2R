#
#                     Configuration File for ewintegrate
#
 MyModId            MOD_DRIFT
 InRing             DISPLACEMENT_RING  # Transport ring to find waveform data on,
 OutRing            DRIFT_RING # Transport ring to write output to,
                              # InRing and OutRing may be the same.
 HeartBeatInterval  30        # Heartbeat interval, in seconds,
 LogFile            1         # 1 -> Keep log file, 0 -> no log file
                              # 2 -> log to file but not to stderr/stdout
# Debug		 		           # Write out debug messages (optional)

 MaxGap              1.5      # Maximum gap, in sample periods, allowed
                              # between trace data points.
                              # When exceeded, channel is restarted.

# TestMode                     # If you want Debias to compute and log 
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
#          Input-SCNL1      Input-SCNL2      Output-SCNL    ConversionFactor
 GetSCNL   5405 HdE NP GN   5405 HdE NP FN   5405 HrE NP GN 1.16
 GetSCNL   5405 HdE NP FN   5405 HdE NP EN   5405 HrE NP FN 1.15
 GetSCNL   5405 HdE NP EN   5405 HdE NP DN   5405 HrE NP EN 1.14
 GetSCNL   5405 HdE NP DN   5405 HdE NP CN   5405 HrE NP DN 1.13
 GetSCNL   5405 HdE NP CN   5405 HdE NP BN   5405 HrE NP CN 1.12
 GetSCNL   5405 HdE NP BN   5405 HdE NP AN   5405 HrE NP BN 1.11
 GetSCNL   5405 HdE NP AN   5405 HdE NP 9N   5405 HrE NP AN 1.10
 GetSCNL   5405 HdE NP 9N   5405 HdE NP 8N   5405 HrE NP 9N 1.09
 GetSCNL   5405 HdE NP 8N   5405 HdE NP 7N   5405 HrE NP 8N 1.08
 GetSCNL   5405 HdE NP 7N   5405 HdE NP 6N   5405 HrE NP 7N 1.07
 GetSCNL   5405 HdE NP 6N   5405 HdE NP 5N   5405 HrE NP 6N 1.06
 GetSCNL   5405 HdE NP 5N   5405 HdE NP 4N   5405 HrE NP 5N 1.05
 GetSCNL   5405 HdE NP 4N   5405 HdE NP 3N   5405 HrE NP 4N 1.04
 GetSCNL   5405 HdE NP 3N   5405 HdE NP 2N   5405 HrE NP 3N 1.03
 GetSCNL   5405 HdE NP 2N   5405 HdE NP 1N   5405 HrE NP 2N 1.02
 GetSCNL   5405 HdE NP GS   5405 HdE NP FS   5405 HrE NP GS 2.16
 GetSCNL   5405 HdE NP FS   5405 HdE NP ES   5405 HrE NP FS 2.15
 GetSCNL   5405 HdE NP ES   5405 HdE NP DS   5405 HrE NP ES 2.14
 GetSCNL   5405 HdE NP DS   5405 HdE NP CS   5405 HrE NP DS 2.13
 GetSCNL   5405 HdE NP CS   5405 HdE NP BS   5405 HrE NP CS 2.12
 GetSCNL   5405 HdE NP BS   5405 HdE NP AS   5405 HrE NP BS 2.11
 GetSCNL   5405 HdE NP AS   5405 HdE NP 9S   5405 HrE NP AS 2.10
 GetSCNL   5405 HdE NP 9S   5405 HdE NP 8S   5405 HrE NP 9S 2.09
 GetSCNL   5405 HdE NP 8S   5405 HdE NP 7S   5405 HrE NP 8S 2.08
 GetSCNL   5405 HdE NP 7S   5405 HdE NP 6S   5405 HrE NP 7S 2.07
 GetSCNL   5405 HdE NP 6S   5405 HdE NP 5S   5405 HrE NP 6S 2.06
 GetSCNL   5405 HdE NP 5S   5405 HdE NP 4S   5405 HrE NP 5S 2.05
 GetSCNL   5405 HdE NP 4S   5405 HdE NP 3S   5405 HrE NP 4S 2.04
 GetSCNL   5405 HdZ NP XS   5405 HdZ NP YS   5405 HrZ NP XS 2.00
 GetSCNL   5405 HdN NP GE   5405 HdN NP FE   5405 HrN NP GE 3.16
 GetSCNL   5405 HdN NP FE   5405 HdN NP EE   5405 HrN NP FE 3.15
 GetSCNL   5405 HdN NP EE   5405 HdN NP DE   5405 HrN NP EE 3.14
 GetSCNL   5405 HdN NP DE   5405 HdN NP CE   5405 HrN NP DE 3.13
 GetSCNL   5405 HdN NP CE   5405 HdN NP BE   5405 HrN NP CE 3.12
 GetSCNL   5405 HdN NP BE   5405 HdN NP AE   5405 HrN NP BE 3.11
 GetSCNL   5405 HdN NP AE   5405 HdN NP 9E   5405 HrN NP AE 3.10
 GetSCNL   5405 HdN NP 9E   5405 HdN NP 8E   5405 HrN NP 9E 3.09
 GetSCNL   5405 HdN NP 8E   5405 HdN NP 7E   5405 HrN NP 8E 3.08
 GetSCNL   5405 HdN NP 7E   5405 HdN NP 6E   5405 HrN NP 7E 3.07
 GetSCNL   5405 HdN NP 6E   5405 HdN NP 5E   5405 HrN NP 6E 3.06
 GetSCNL   5405 HdN NP 5E   5405 HdN NP 4E   5405 HrN NP 5E 3.05
 GetSCNL   5405 HdN NP 4E   5405 HdN NP 3E   5405 HrN NP 4E 3.04
 GetSCNL   5405 HdN NP YE   5405 HdN NP XE   5405 HrN NP XE 3.00
 GetSCNL   5405 HdN NP XE   5405 HdN NP 1E   5405 HrN NP 1E 3.01
 GetSCNL   5405 HdE NP XW   5405 HdE NP 1W   5405 HrN NP 1W 4.01
 GetSCNL   5405 HdN NP GW   5405 HdN NP FW   5405 HrN NP GW 4.16
 GetSCNL   5405 HdN NP FW   5405 HdN NP EW   5405 HrN NP FW 4.15
 GetSCNL   5405 HdN NP EW   5405 HdN NP DW   5405 HrN NP EW 4.14
 GetSCNL   5405 HdN NP DW   5405 HdN NP CW   5405 HrN NP DW 4.13
 GetSCNL   5405 HdN NP CW   5405 HdN NP BW   5405 HrN NP CW 4.12
 GetSCNL   5405 HdN NP BW   5405 HdN NP AW   5405 HrN NP BW 4.11
 GetSCNL   5405 HdN NP AW   5405 HdN NP 9W   5405 HrN NP AW 4.10
 GetSCNL   5405 HdN NP 9W   5405 HdN NP 8W   5405 HrN NP 9W 4.09
 GetSCNL   5405 HdN NP 8W   5405 HdN NP 7W   5405 HrN NP 8W 4.08
 GetSCNL   5405 HdN NP 7W   5405 HdN NP 6W   5405 HrN NP 7W 4.07
 GetSCNL   5405 HdN NP 6W   5405 HdN NP 5W   5405 HrN NP 6W 4.06
 GetSCNL   5405 HdN NP 5W   5405 HdN NP 4W   5405 HrN NP 5W 4.05
 GetSCNL   5405 HdN NP 4W   5405 HdN NP 3W   5405 HrN NP 4W 4.04

