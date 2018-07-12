# ark2arc.d
#
# Picks up .ark files from a specified directory, places a copy in 
# specified directory that has been converted to a hypoARC message.
#
# Option to save (to subdir ./save) or delete the file afterwards.
# If it has trouble converting a file, it saves it to subdir ./trouble. 
# Maintains its own local heartbeat

# Basic Module information
#-------------------------
MyModuleId        MOD_ARK2ARC      # module id 
RingName          HYPO_RING	   # shared memory ring for output
HeartBeatInterval 30               # seconds between heartbeats to statmgr

LogFile           1                # 0 log to stderr/stdout only; 
                                   # 1 log to stderr/stdout and disk;
                                   # 2 log to disk module log only.

Debug             0                # 1=> debug output. 0=> no debug output

# Data file manipulation
#-----------------------
GetFromDir      /home/pdl2ew/pdl2ark_msgs          # look for files in this directory
SaveToDir       /home/pdl2ew/pdl2arc_msgs          # save files in this directory
DatabasePath    /home/pdl2ew/pdl2ew.db             # path to DB file
CheckPeriod     1                  # sleep this many seconds between looks
OpenTries       5                  # How many times we'll try to open a file 
OpenWait        200                # Milliseconds to wait between open tries
SaveDataFiles   1                  # 0 = remove files after processing
                                   # non-zero = move files to save subdir
                                   #            after processing  
LogOutgoingMsg  0                  # If non-zero, write contents of each 
                                   #   outgoing msg to the daily log.

  

LogHeartBeatFile 1                 # If non-zero, write contents of each
                                   #   heartbeat file to the daily log.
