# sm_file2ew.d
#
# This is a generic config file for the strongmotion conversion
# programs:  sm_csmip2ew
#            sm_nsmp2ew
#            sm_redi2ew
#            sm_terra2ew
#            sm_tremor2ew
#
# Most commands in this file are required by all of the sm_file2ew
# programs.  Converter-specific commands are listed at the end.
#
# Each sm_file2ew program performs these tasks:
# Picks up files from a specified directory, converts the contents 
# to TYPE_STRONGMOTION message(s) and places into a message ring. 
# Option to save (to subdir ./save) or delete the file afterwards.
# If it has trouble converting as file, it saves it to subdir ./trouble. 
# Maintains its own local heartbeat and also monitors the peer's
# heartbeat via a file. Complains if the peer's expected heartbeat
# interval is exceeded; announces resumption of peer's heartbeat.

# Basic Module information
#-------------------------
MyModuleId        MOD_TERRA2EW     # module id 
RingName          PICK_RING	   # shared memory ring for output
HeartBeatInterval 30               # seconds between heartbeats to statmgr

LogFile           1                # 0 log to stderr/stdout only; 
                                   # 1 log to stderr/stdout and disk;
                                   # 2 log to disk module log only.

Debug             1                # 1=> debug output. 0=> no debug output

# Data file manipulation
#-----------------------
GetFromDir       c:\sm_files       # look for files in this directory
CheckPeriod      1                 # sleep this many seconds between looks
OpenTries        5                 # How many times we'll try to open a file 
OpenWait         200               # Milliseconds to wait between open tries

SaveDataFiles    1                 # 0 = remove files after processing
                                   # non-zero = move files to save subdir
                                   #            after processing    

LogOutgoingMsg   1                 # If non-zero, write contents of each
                                   #   outgoing msg to the daily log.
 


# Peer (remote partner) heartbeat manipulation
#---------------------------------------------
PeerHeartBeatFile terra1  HEARTBT.TXT  600 
                                   # PeerHeartBeatFile takes 3 arguments:
                                   # 1st: Name of remote system that is 
                                   #   sending the heartbeat files.
                                   # 2nd: Name of the heartbeat file. 
                                   # 3rd: maximum #seconds between heartbeat 
                                   #   files. If no new PeerHeartBeatFile arrives
                                   #   in this many seconds, an error message will
                                   #   be sent.  An "unerror message" will be
                                   #   sent after next heartbeat file arrives
                                   #   If 0, expect no heartbeat files.
                                   # Some remote systems may have multiple 
                                   # heartbeat files; list each one in a
                                   # seperate PeerHeartBeatFile command
                                   # (up to 5 allowed).

PageOnLostPeer smtechnician        # Optional command: Name of group to page 
                  	           #   if PeerHeartBeatFile is late. This allows
                                   #   pages to be sent to groups other than that
                                   #   listed in statmgr.d. Up to 5 PageOnLostPeer
                                   #   commands can be used.
                                   # Must run telafeeder on same system to 
                                   #   actually get the pages sent.

LogHeartBeatFile 1                 # If non-zero, write contents of each
                                   #   heartbeat file to the daily log.


# Commands specific to sm_redi2ew (comment out for other modules)
#----------------------------------------------------------------
#QuakeAuthor 014024003    # REQUIRED command for sm_redi2ew.
                          # Author string for the module that originally
                          # produced the event as a TYPE_HYP2000ARC msg,
                          # (usually MOD_EQPROC).
                          # The string should be the msg logo represented
                          # as 9 characters, 3 chars each for:
                          #   msgtype moduleid installationid
                          # Required so that the strongmotion data can
                          # be associated with the correct DBMS eventid.

# Commands specific to sm_terra2ew (comment out for other modules)
#-----------------------------------------------------------------
#NumComPort  10           # REQUIRED command for sm_terra2ew. 
                          # Specifies the number of COM ports that
                          # should be listed in each heartbeat file.
                          # sm_terra2ew will complain if the heartbeat
                          # file contains any other number.

# Commands specific to sm_tremor2ew (comment out for other modules)
#-----------------------------------------------------------------
#NumTremorSites 13        # OPTIONAL command for sm_tremor2ew.
                          # Specifies the number of Tremor sites that
                          # should be listed as "live" in each
                          # heartbeat file.  sm_tremor2ew will complain
                          # if the heartbeat contains any other number.
 

# Commands specific to other strongmotion converters
#---------------------------------------------------

MaxChannel 100      # OPTIONAL cmd for sm_csmip2ew, sm_nsmp2ew,
                    #                  sm_terra2ew, sm_tremor2ew
                    # Set the maximum number of ChannelName
                    # commands to read below (default=100).

# ChannelName: REQUIRED cmd for sm_csmip2ew, sm_nsmp2ew,
#                               sm_terra2ew, sm_tremor2ew
# The entries below assign an IRIS-style SCNL name to each active channel
# of each instrument we may hear from. Each instrument has a 'box' identifier
# which is sent in its message (could be a name or a serial number).
# Use "" to enter a NULL string for any of the SCNL fields. 
# Non-NULL strings do not need to be enclosed in quotes.
#
# NOTE: legal channel numbers (as of Oct 99) are 0-5
#
#            box  chan   S     C   N    L
ChannelName  WIG   0     WIG   1   PG   ""
ChannelName  WIG   1     WIG   2   PG   ""
ChannelName  WIG   2     WIG   3   PG   ""
ChannelName  P01   0     P01  ADZ  NC   ""
ChannelName  P01   1     P01  ADN  NC   ""
ChannelName  P01   2     P01  ADE  NC   ""
ChannelName  P13   0     P13  ADZ  NC   ""
ChannelName  P13   1     P13  ADN  NC   ""
ChannelName  P13   2     P13  ADE  NC   ""
