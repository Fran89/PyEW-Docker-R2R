# This is arc2trig's parameter file

#  Basic Earthworm setup:
#
MyModuleId        MOD_ARC2TRIG   # module id for this instance of arc2trig 
InRingName           HYPO_RING   # shared memory ring for input
OutRingName          HYPO_RING   # shared memory ring for output
LogFile              1           # 0 to turn off disk log file; 1 to turn it on
                                 # 2 to log to module log but not to stderr/stdout
HeartBeatInterval    30          # seconds between heartbeats

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_WILDCARD    # hyp2000arc - no choice.

# Set up output directory and prefix for trigger files.
# Daily files will be written with a suffix of .trg_yyyymmdd
# Set either to "none" or "NONE" to stop writing disk files
OutputDir  /home/big_files/  # directory to write trigger files in
BaseName   arc2trig          # prefix of trigger file name
                             # suffix will be .trg_yyyymmdd

# Optional parameters:
# PrePickTime 15.0	# seconds of pre-p-pick data to save (default=15.)
# PostCodaTime 10.0
	# seconds of post-coda cutoff data to save (default=10.).
# MinDuration  60.0 # save this much regardless of above settings!

# Optional parameter to over ride above PrePickTime and PostCodaTime (requires MinDuration)
# UseOriginTime	1	# use the origin time for the start of every trace
