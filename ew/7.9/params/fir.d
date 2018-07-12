
#
#                     Configuration File for Fir
#
MyModId          MOD_FIR
InRing           WAVE_RING       # Transport ring to find waveform data on,
OutRing          WAVE_RING       # Transport ring to write output to,
                                 # InRing and OutRing may be the same.
HeartBeatInterval     30         # Heartbeat interval, in seconds,
LogFile            1             # 1 -> Keep log, 0 -> no log file
                                 # 2 -> write to module log but not to stderr or
                                 #   stdout
#Debug		 		 # Write out debug messages (optional)

MaxGap              1.5         # Maximum gap, in sample periods, allowed
                                # between trace data points.
                                # When exceeded, channel is restarted.

# Specify the filter frequency response characteristics with two or more
# commands fo the form: Band <f_low> <f_high> <level> <deviation>
#
# where:
#   f_low = low frequency limit of a band
#   f_high = high frequency limit of a band
#   level = desired level of the band: 1 for pass band, 0 for stop band
#   deviation = amount of ripple allowed from desired level
# 
# All frequencies must be scaled to the range 0.0 - 1.0, where 1.0
# corresponds to the Nyquist frequency. (Nyquist frequency is half the sample
# rate and is the maximum frequency content that can be in a time series.)
# Note that the sampling rate is not known at configuration time, but will be
# gotten from the trace data packets as they're received at run-time.
# One band must start at 0.0 frequency; one band must end at 1.0 frequency. Any
# number of additional bands may be specified (up to a total of ten bands.)
Band 0.0 0.3 1 0.005
Band 0.4 1.0 0 0.005

# If you want Fir to compute and log its filter coefficients and then
# exit, specify "TestMode".
TestMode

# optional setting to allow a queue buffer size larger than 100 messages, defaults to 100
QueueSize 2000
# optional setting to allow a shorter sleep between ring pollings, defaults to 500
SleepMilliSeconds 50

#
# Specify logos of the messages to grab from the InRing.
# TYPE_TRACEBUF2 is assumed, therefore only module ID and 
# installation ID need to be specified
#
GetWavesFrom    INST_WILDCARD MOD_WILDCARD  # TYPE_TRACEBUF2 (assumed)

#
# List SCNL codes of trace messages to fir and their output SCN codes
# Wildcard characters are not allowed here.
#
#         IN-SCNL        OUT-SCNL
GetSCNL TEST EHZ UW --  TEST EWZ UW 01


