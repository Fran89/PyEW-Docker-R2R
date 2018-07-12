# trig2arc:
# I listen to trigger messages (TYPE_TRIGLIST_SCNL), 
# transform them into hypo2000 archive message (TYPE_HYP2000ARC),
# and send them to the output ring.
#------------------------------------------------------------------

# Basic Earthworm setup:
#------------------------
 MyModuleId   MOD_TRIG2ARC
 InRing       TRIG_RING       # shared memory ring for input
 OutRing      OUT_RING	      # shared memory ring for output
 HeartbeatInt 30              # seconds between heartbeats
 LogFile      1               # 0 log to stderr/stdout only 
                              # 1 log to stderr/stdout and to disk file
                              # 2 log to disk file only
 Debug        1

# List the message logos to grab from InRing, WILDCARDs permitted.
# Multiple "GetLogo" commands are allowed, with no hardcoded limit.
#         Installation    Module      Message Types
#-----------------------------------------------------------------
 GetLogo  INST_WILDCARD  MOD_WILDCARD # TYPE_TRIGLIST_SCNL

# Optional commands:
#-------------------
 UseOriginalLogo  0          # 0  apply trig2arc's logo to output message
                             # non-zero means apply the original logo to output
			     # message.
 UseLatitude      14.5268
 UseLongitude     -61.2354
 UseDepth         25

