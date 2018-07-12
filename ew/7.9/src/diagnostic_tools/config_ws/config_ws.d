#
# config_ws.d
#	

# General Info
MyModuleId        MOD_CONFIG_WS    # Module id for this instance of config_ws
RingName          WAVE_RING        # Shared memory ring for input/output
HeartBeatInterval 30               # Seconds between heartbeats, if 0, no heartbeats
                                   #  will be sent.
LogSwitch         1                # If 0, turn off disk log file
                                   # If 1, log to disk file and stderr/stdout
                                   # If 2, log only disk file
# Message Settings
MsgMaxBytes       4096             # Size of biggest message to be sent. Required
TraceType         SCNL             # The input trace stream type:
                                   #  SCNL - For SCNL data.
                                   #  SCN  - Fir SCN data.
                                   #  Default is SCNL.
# Queue Settings
InputQueueLen     1000             # The Desired Size of the Input Queue in Messages
                                   #  Default is 1000
# Table Settings
TableSize         2000             # Maximum number of stations in the station table
                                   #  Default is 2000
StationFile       stations.bin     # File to save the station information to on shutdown.
                                   #  Required
# Tank settings
DaysToSave        3                # Number of days of trace each tank file should save. Required
IndexSize         10000            # The desired index size for each tank, default is 10000
MessageSize       -2               # The desired message size for each tank
                                   #  If -1, the message size is the max message seen for
                                   #    each SCNL.
                                   #  If -2, config will calcuate the max message size based
                                   #    on the max message size seen, and the difference 
                                   #    between the max and min sizes.
                                   #  Default is -1
EnforceLimit      1                # WaveserverV has an inherent maximum tank size of 
                                   #  1 gigabyte, if this flag is set, config_ws will
                                   #  not allow any tank to be larger than 1 gigabyte 
                                   #  Default is 0  
# Output Settings
OutPath           c:\hydra\ws_internal\run_ws\newtankfiles
                                   # Full path to write the generated files too.  Required
FileWriteInt      600              # Time in seconds between writes of generated files to disk
                                   #   Default is 300 seconds   
SortBy            SCNL S           # Controls how the program sorts the tanks
                                   #    SCNL - sorts the tanks by the SCNL where:
                                   #      S = SNLC
                                   #      C = CSNL
                                   #      N = NSLC
                                   #      L = LSNC
                                   #    Logo - sorts the tank by the Logo 
                                   #       and then by SNLC.
                                   # Default is SCNL S
ForceInstIdToWild 0                # Force the Installation ID to INST_WILDCARD, default is 0
                                   #  Note: If the InstID in the logo is not matched in the
                                   #    earthworm_global.d the string "inst_unlisted" will be written
                                   #    for the InstID.
ForceModIdToWild  1                # Force the Module ID to MOD_WILDCARD, default is 0
                                   #  Note: If the ModID in the logo is not matched in the
                                   #    earthworm.d file the string "mod_unlisted" will be written
                                   #    for the ModID.
                                   #    If the InstID in the logo is not the same as the local one
                                   #    the string "Mod_NotLocal" will be written for the ModID
# Hard Drive settings:
# Note: If no hard drives are specified,
# config_ws will create one file
# Max # of hard drive definitions is 100
#  Name is used to construct the tankfile name
#  Path is the full path to the tanks on that waveserver
#  Size is the size of the waveserver in megabytes.
#
#         Name  Tank Path       Size
Harddrive E     E:\hydra\tanks\ 67000
Harddrive F     F:\hydra\tanks\ 67000
Harddrive G     G:\hydra\tanks\ 67000