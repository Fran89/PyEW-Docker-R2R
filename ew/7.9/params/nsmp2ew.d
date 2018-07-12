# nsmp2ew.d
#
# Picks up files from a specified directory, converts the contents
# to TYPE_STRONGMOTION message(s) and places into a message ring.
# Option to save (to subdir ./save) or delete the file afterwards.
# If it has trouble converting as file, it saves it to subdir ./trouble.
# Maintains its own local heartbeat and also monitors the peer's
# heartbeat via a file. Complains if the peer's expected heartbeat
# interval is exceeded; announces resumption of peer's heartbeat.

# Basic Module information
#-------------------------
MyModuleId        MOD_NSMP2EW      # module id
RingName          SM_RING          # shared memory ring for output
HeartBeatInterval 30               # seconds between heartbeats to statmgr

LogFile           1                # 0 log to stderr/stdout only; 
                                   # 1 log to stderr/stdout and disk;
                                   # 2 log to disk module log only.

Debug             1                # 1=> debug output. 0=> no debug output

# Data file manipulation
#-----------------------
GetFromDir      /ebird/dietz/smdata/nsmp #for files in this directory
CheckPeriod     1                  # sleep this many seconds between looks
OpenTries       5                  # How many times we'll try to open a file
OpenWait        400                # Milliseconds to wait between open tries
SaveDataFiles   1                  # 0 = delete files after processing
                                   # non-zero = move files to save subdir
                                   #            after processing

LogOutgoingMsg   1                 # If non-zero, write contents of each
                                   #   outgoing msg to the daily log.
 

# Peer (remote partner) heartbeat manipulation
#---------------------------------------------
PeerHeartBeatFile pc1 heartbeat_nms_pc1.txt  5400  
PeerHeartBeatFile pc2 heartbeat_nms_pc2.txt  5400
PeerHeartBeatFile pc3 heartbeat_nms_pc3.txt  5400
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

LogHeartBeatFile 1                 # If non-zero, write contents of each
                                   #   heartbeat file to the daily log.

PageOnLostPeer dietz		   # Optional command: Name of group to page 
                                   #   if PeerHeartBeatFile is late. This allows
                                   #   pages to be sent to groups other than that
                                   #   listed in statmgr.d. Up to 5 PageOnLostPeer
                                   #   commands can be used.
                                   # Must run telafeeder on same system to 
                                   #   actually get the pages sent.
                         

# Commands specific to sm_nsmp2ew
#--------------------------------

MaxChannel    900   # set the maximum number of ChannelName
                    # commands to read below.
                    
XMLType         0   # Format of the XML summary file (optional).
                    # 0 is legacy CGS form (default).
                    # 1 is new EIDS form.

Authority     NC    # Network code of creator of PGMs
                    
# The entries below assign an IRIS-style SCNL name to each active channel
# of each instrument we may hear from. Each instrument has a 'box' identifier
# which is sent in its message (could be a name or a serial number).
# Use "" to enter a NULL string for any of the SCNL fields.
#
# NOTE: legal channel numbers (as of Oct 99) are 0-5
#
#            box  chan   S     C   N    L
#ChannelName  P01   0     P01  ADZ  NC   ""
@NSMP_Chans.db
