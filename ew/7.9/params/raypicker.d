
#
#                     Configuration File for raypicker
#
MyModId              MOD_RAYPICKER   # Module Name
InRing               WAVE_RING       # Transport ring from which to get waveform data 
OutRing              PICK_RING       # Transport ring to which to write output
HeartBeatInterval    30              # Heartbeat interval, in seconds,
                                     # WARNING: Heartbeat interval is also used to check on thread status;
                                     # No NOT MAKE THIS NUMBER SMALLER THAN THE TIME IT TAKES TO 
                                     # PROCESS A MESSAGE OR THE PROGRAM WILL CONSTANTLY RESTART!!!
                                     # (default min: 15 secs)
PickIDFile           pickfile        # File containing Pick IDs

# List of SCNLs to include
# SCNL               JSA VHZ NC --
#@stalist.picker
SCNLFile             stalist.picker

# Optional Commands
LogFile              1               # no logging; 1 = disk and stderr/stdout; 2 = disk only
Debug                0               # 0-no debug
                                     # 1 - some debugging
                                     # 2 - super debug
MaxSampleRate        201.0           # Maximum sampling rate (default: 101 Hz-
                                     # NB: make this slightly larger than desired, ie.
                                     # 101 Hz for 100 Hz data, to allow for minimal
                                     # tolerance in floating point comparisons in code)
MaxTriggerSecs       600.0           # Maximum length of trigger (default: 10 minutes)
MaxGapNoTriggerSecs    0.0           # Max gap before resetting if no trigger (default: 3 hours)
MaxGapInTriggerSecs    0.0           # Max gap when a trigger exists 
QueueSize             1500           # Max number of messages to queue (default: 500)
MaxPreFilters           20           # Max no of prefilters to allow (need at least 2xnumber of different
                                     #   sampling rates of data expected - one for BB data and one for 
                                     #   data SP; default = 20)
SCNLUpdateInterval   43200           # Time between comparison of SCNLFile and the SCNL list in memory;
                                     #   default is 12 hours (43200 seconds)                                     


