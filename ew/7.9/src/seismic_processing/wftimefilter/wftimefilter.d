# wftimefilter configuration file

# Basic Earthworm setup:
#-----------------------
 MyModuleId    MOD_WFTIMEFILTER # module id for this instance of wftimefilter 
 InRing        RAW_RING         # shared memory ring for input
 OutRing       WAVE_RING        # shared memory ring for output
 HeartbeatInt  30               # seconds between heartbeats to statmgr
 LogFile       1                # 0 log to stderr/stdout only 
                                # 1 log to stderr/stdout and to disk file
                                # 2 log to disk file only

# List the message logos to grab from InRing, WILDCARDs permitted.
# Multiple "GetLogo" commands are allowed, with no hardcoded limit.
# The four valid message types are:
# TYPE_TRACEBUF, TYPE_TRACEBUF2, TYPE_TRACE_COMP_UA, TYPE_TRACE2_COMP_UA
#         Installation   Module        Message Types
#-----------------------------------------------------------------
 GetLogo  INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF2

# Waveform Filtering Parameters
#------------------------------
 MaxMessageSize    4096  # length (bytes) of largest msg to be processed

 UseOriginalInstId    1  # 0  apply wftimefilter's installationid to 
                         #    any messages that pass the test. 
                         # non-zero means apply the original instid.

 UseOriginalModId     0  # 0  apply wftimefilter's moduleid to 
                         #    any messages that pass the test 
                         # non-zero means apply the original instid.

 LogGap             0.0  # Minumum gap duration (seconds) for which we'll make
                         # a log entry.

 TimeJumpTolerance  600  # (in seconds) This term is used to catch packets with
                         # dubious timestamps.  If a time gap is detected in an
                         # incoming data stream, the new packet's timestamp is
                         # compared to the system clock.  If it is later than
                         # the current system time by more than TimeJumpTolerance
                         # seconds, wftimefilter assumes the packet timestamp is 
                         # bogus (it's in the future) and ignores the entire packet.
                         # NOTE: if you use this feature with small tolerances,
                         # the system clock must be kept on network time!!!
                         # 
                         # Set to -1 if you never want to compare packet times
                         # to the system clock.
                         # Valid values are -1 or any number >= 0.0


#AllowChangesInSampleRate 1  # Set to 0 to disallow packets with a different 
                             # sample-rate than the rate already observed for 
                             # that channel. (optional; default=1)

# LOGGING And STATUS Commands (all optional)
#-------------------------------------------
#LimitLoggingOfBadChannels 0
                         # Set to 1 to 
                         # suppress log messages from channels that  
                         # exceed the MaxPackets daily log thresholds.
                         # This way, one (or more) bad channel(s) will not generate a
                         # huge logfile. (optional; default=0)

#MaxBadPacketsToLogPerChannelPerDay 3
                         # If LimitLoggingOfBadChannels is enabled, then
                         # this is this number is the MAXIMUM number of
                         # filtered packets that will be logged per channel per day.
                         # (optional; default=3)

#MaxWarningsToLogPerChannelPerDay 3
                         # If LimitLoggingOfBadChannels is enabled, then
                         # this is this number is the MAXIMUM number of
                         # packet warnings that will be logged per channel per day.
                         # Currently, warnings issued for gaps between packets, and
                         # possibly changes in sample rate depending on the value
                         # of AllowChangesInSampleRate. (optional; default=3)

                         
#LogDailySummary 0       # Set to 1 to log a daily summary of the packets processed
                         # and number of errors and warning generated per channel.
                         # (optional; default=0)

#IssueDailyStatusWithSummaryOfBadPackets 0
                         # Set to 1 to have program issue a STATUS ERROR message each day
                         # if bad packets are processed that day. (optional: default=0)

#LogOnlyChannelsWithProblems 0
                         # If LogDailySummary is set to 1, then set this param 
                         # to 1 to cause only channels with problems to be logged
                         # in the daily summary. (optional; default=0)

# THE END
