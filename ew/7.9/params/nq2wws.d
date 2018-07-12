#
#

#   nq2wws configuration file
#
#Background:  
# Import/Export are the long-distance message transfer
# modules. They operate as one-on-one pairs. There are two variants of
# export: export_generic, which ships messages with specified logos, and
# export_scn which ships only trace data, but with specified SCN names.
#
# Import/Export send heartbeats to each other, as well as into their
# local earthworm systems. If the heartbeat from the distant partner is
# not received whithin the expected time period (RcvAliveInt) the link is
# terminated, and an attempt to reconnect is initiated. If things go
# seriously wrong, the heartbeat into the local earthworm system in
# stopped. The expectation is that "restartMe" has been set in the .desc
# file, and we'll be killed and restarted.
#
# All socket operations are performed with a timeout. This is noramlly
# defaulted, but can be set in this file (SocketTimeout).
#
# Export maintains a circular FIFO buffer of messages to be shipped. The
# size of this buffer (RingSize) controls the maximum latency of the
# data.
#
#
#Configuration notes:
#
# "restartMe" should be stated in our desc file.
#
# The period of our local heartbeat (HeartBeatInt) must be safely smaller
# (faster) than our advertised period in our .desc file (tsec:).
# Otherwise we'll get continually restarted for no good reason.
# Note that tsec:0 implies no heartbeats expected, and so we'll never get
# restarted.
#
# The rate at which we send heartbeats to our distant partner should be
# considerably faster than the rate at which our partner expects them.
# Otherwise, a heartbeat delayed in transmission will cause our partner to
# conclude that the link is broken, and cause them to break the link and
# reinitialize. Which will cause us to do the same.
#
# For export, the ServerIPAdr is  the address of the port to be used in
# the exporting machine.  This is to specify a network card case the
# exporting machine has several network cards.
#
# If SocketTimeout is specified, it should be at least as large as the
# expected period of heartbeats from our distant partner.

#
 MyModuleId     MOD_NQ2WWS          # module id for this program
 RingName       HYPO_RING           # transport ring to use for input/output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
                                    #   Should be >= RcvAliveInt

 LogFile        1      # If 0, don't write logfile
                       #    1, write to logfile and stdout
                       #    2, write to module log but not stderr/stdout
#Verbose               # If uncommented, VERY LARGE logfiles will be 
                       #   generated with info about queue status of 
                       #   each msg, socket alive msgs sent & received.
#
# Logos of messages to export to client systems
#              Installation       Module       Message Type
 GetMsgLogo    ${EW_INST_ID}     MOD_WILDCARD     TYPE_PICK2K
 GetMsgLogo    ${EW_INST_ID}     MOD_WILDCARD     TYPE_CODA2K
 
# ServerIPAdr and ServerPort specify the local IP address and port number
# on which this module will be ready to accept a client connection.
#
 ServerIPAdr  aaa.bbb.ccc.dd   # Export messages via this ethernet card   
 ServerPort     16005          # Well-known port number to export msgs on

 MaxMsgSize       128          # maximum size (bytes) for input/output msgs
 RingSize         100          # number of msgs to buffer for export 

 SendAliveText   "alive"       # string sent to client as heartbeat 
 SendAliveInt      30          # seconds between alive msgs sent to client.
                               #  0=> no heartbeat
 RcvAliveText    "alive"       # text of client's heartbeat (we get this)
 RcvAliveInt       150         # seconds between client's heartbeats to us.
                               #  0 => no heartbeat
#
# Socket commands
#
# The commands below are optional. 
# SocketTimeout defaults to RcvAliveInt + 3 
#  If set to -1, all socket calls will block (no timeout).
#  SocketTimeout has no effect in export, unless it is set to -1,
#  because there is no code in export to handle socket timeouts.
#  If set to -1, the program may run slightly more efficiently because
#  timeout checking code will not execute.
# SocketTimeout 200000  # Timeout length in milliseconds for socket calls

# SocketDebug defaults to 0
 SocketDebug    1       # if 1, socket_ew debug statements are logged
                       # if 0, socket_ew debug is turned off

# Directory where we should look for the NetQuakes miniSEED files
NQFilesInDir    /home/luetgert/getfiles/netquakes

# Directory where the successfully processed files are put
NQFilesOutDir /home/luetgert/getfiles/netquakes/save

# Directory where the problem files are put
NQFilesErrorDir /home/luetgert/getfiles/netquakes/trouble

# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
# Debug

