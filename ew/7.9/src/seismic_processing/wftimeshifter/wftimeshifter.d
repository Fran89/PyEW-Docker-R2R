# wftimeshifter configuration file

# Basic Earthworm setup:
#-----------------------
 MyModuleId    MOD_WFTIMESHIFTER # module id for this instance of wftimefilter 
 InRing        SCNL_RING         # shared memory ring for input
 OutRing       WAVE_RING         # shared memory ring for output
 HeartbeatInt  30                # seconds between heartbeats to statmgr
 LogFile       1                 # 0 log to stderr/stdout only 
                                 # 1 log to stderr/stdout and to disk file
                                 # 2 log to disk file only

# List the message logos to grab from InRing, WILDCARDs permitted.
# Multiple "GetLogo" commands are allowed, with no hardcoded limit.
# The four valid message types are:
# TYPE_TRACEBUF, TYPE_TRACEBUF2, TYPE_TRACE_COMP_UA, TYPE_TRACE2_COMP_UA
#         Installation   Module        Message Types
#-----------------------------------------------------------------
 GetLogo  INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF2

 MaxMessageSize    4096  # length (bytes) of largest msg to be processed

# List the SCNLs to grab from InRing, WILDCARDs permitted.
# Multiple "Offset_scnl" commands are allowed, with no hardcoded limit.
# If wildcards are present, the list will be searched top to bottom with
# the first match being applied. If no wildcards are present, the list
# will be sorted and a more efficient binary search used.
#         
#-----------------------------------------------------------------
Offset_scnl ADAG EHZ AV -- -0.270
Offset_scnl KOKL EHZ AV -- -0.300
Offset_scnl KOKV  *  AV -- -0.300
Offset_scnl PS4A EHZ AV -- -0.270
Offset_scnl * * * * 0.0
