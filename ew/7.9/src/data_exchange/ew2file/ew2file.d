
#   ew2file_generic configuration file
#
#
 MyModuleId     MOD_EW2FILE         # module id for this program
 InRing         SM_RING             # transport ring to use for input
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile
                                    # if 2, write to module log but not to
                                    # stderr/stdout .
#
# Logos of messages to export to client systems
# Each message of a given logo will be written to a file with the specified
# File Suffix at the end of the file name folloing `.'.
# Do NOT include a `.' in the suffix.
# Use as many GetMsgLogo commands as you want.
#              Installation       Module       Message Type        File Suffix
 GetMsgLogo    ${EW_INST_ID}      MOD_REDI2EW   TYPE_STRONGMOTIONII     smUCB
 
 MaxMsgSize        512         # maximum size (bytes) for input msgs
 QueueSize         1000        # number of msgs to buffer

 TimeBasedFilename 1           # set non-zero if you want a time string in the
                               # form of "yyyymmdd-hhmmss." at the beginning of
                               # each output file. The timestring is generated from
                               # msg contents, currently possible for only msgtypes:
                               # TYPE_PICK2K, TYPE_PICK_SCNL, TYPE_HYP2000ARC

# some new file naming options
 NoKeeperInFilename 0          # do not use keeper number in filename, OPTIONAL defaults to off if not in .d file
 QIDinFilename 0               # use quake ID number in filename, OPTIONAL defaults to off if not in .d file
				# this is valid only for HYP2000ARC and MAGNITUDE messages

# OneFilePerMessage 1  		# an optional mode of ew2file to store all messages to a single file instead of
				# the normal one file per message. This is useful for debugging purposes, or for gathering picks
       
# Optional heartbeat file commands. 
#----------------------------------
# Default HeartFileInt is 0 => send no heartbeat files.
# Default HeartFileMsg is "Alive".
# Default HeartFileSuffix is "hrt"
 HeartFileInt     30           # heartbeat file interval; optional
 HeartFileMsg     Alive        # string for heartbeat file; optional  
 HeartFileSuffix  hrtUCB       # suffix for heartbeat files; do not include `.'

# SerialKeeper: File name where the serial number of the output files is 
# stored. This should be a unique file name, in the params directory.
# If the file does not exist, ew2file will attempt to create it.
SerialKeeper  ew2file_keeper

# TempDir: Directory in which files are created. This directory must be in the
# same filesystem as all the SendDir directories. Exactly one TempDir is required.
TempDir      /home/earthworm/run/ew2file/temp  

# SendDir: Directory to which output files will be moved after they have 
# been created. 
#  Use as many SendDir commands as you need.
# Be sure the final line is terminated with a newline.
SendDir      /home/earthworm/run/ew2file/Menlo
SendDir	     /home/earthworm/run/ew2file/CGS
SendDir	     /home/earthworm/run/ew2file/Pas	     
