#
# export_scnl_ack configuration file
#
# Export_scnl_ack expects to receive an acknowledgment packet for every
# packet that is writes to socket. It keeps a SendQueue of messages it has
# sent and their status. The SocketSend thread will not re-use a slot in
# its SendQueue until the SocketRecv thread has marked that slot as ACKed.
#
# Import/Export send heartbeats to each other, as well as into their
# local earthworm systems. If the socket heartbeat from the distant partner
# is not received whithin the expected time period (RcvAliveInt) the link is
# terminated, and an attempt to reconnect is initiated. If things go
# seriously wrong, the heartbeat into the local earthworm system in
# stopped. The expectation is that "restartMe" has been set in the .desc
# file, and we'll be killed and restarted.
#
# Socket heartbeats are not put in either the message stacking queue
# or in the SendQueue, so they do not take up precious space when
# the connection between import/export is down.
#
# All socket operations are performed with a timeout. This is noramlly
# defaulted, but can be set in this file (SocketTimeout).
#
# Export maintains a circular FIFO buffer of messages to be shipped. 
# Let's call this the MessageStacking queue. The depth of this queue 
# (RingSize) controls the maximum latency of the data.
#
#
# Configuration notes:
#
# "restartMe" should be uncommented in my .desc file.
#
# The period of our local heartbeat (HeartBeatInt) must be safely smaller
# (faster) than our advertised period in our .desc file (tsec:).
# Otherwise we'll get continually restarted for no good reason.
# Note that tsec:0 implies no heartbeats expected, and so we'll never get
# restarted.
#
# The rate at which we send socket heartbeats to our distant partner should
# be considerably faster than the rate at which our partner expects them.
# Otherwise, a heartbeat delayed in transmission will cause our partner to
# conclude that the link is broken, and cause them to break the link and
# reinitialize. Which will cause us to do the same.
#
# For export, the ServerIPAdr is the address of the port to be used in
# the exporting machine.  This is to specify a network card case the
# exporting machine has several network cards.  
# Using ServerIPAdr 0.0.0.0 will allow export to answer connections
# on any valid address of the machine.
#
# If SocketTimeout is specified, it should be at least as large as the
# expected period of heartbeats from our distant partner.

# Basic Earthworm setup
#----------------------
 MyModuleId     MOD_EXPORT_SCNL_ACK # module id for this program
 RingName       PICK_RING           # transport ring to use for input/output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
                                    #   Should be >= RcvAliveInt
# Logging Control
#----------------
 LogFile        1      # If 0, don't write logfile
                       #    1, write to logfile and stdout
                       #    2, write to module log but not stderr/stdout
#Verbose               # If uncommented, VERY LARGE logfiles will be 
                       #   generated with info about queue status of 
                       #   each msg, socket alive msgs sent & received.

# Logos of messages to export to client systems.
#------------------------------------------------
# Installation and Module can be wildcards.
# Knows how to decipher these message types:
#  w/  location code: TYPE_TRACEBUF2, TYPE_TRACE2_COMP_UA, 
#                     TYPE_PICK_SCNL, TYPE_CODA_SCNL,
#                     TYPE_STRONGMOTIONII
#  w/o location code: TYPE_TRACEBUF, TYPE_TRACE_COMP_UA,
#                     TYPE_PICK2K, TYPE_CODA2K msgs
#
#              Installation       Module       Message Type
#--------------------------------------------------------------
 GetMsgLogo    ${EW_INST_ID}    MOD_WILDCARD     TYPE_TRACEBUF2

# Set up Message Queues 
# Note: socket "alive" messages are not put in either queue
#---------------------------------------------------------- 
 MaxMsgSize             4096   # maximum size (bytes) for input/output msgs
 RingSize                200   # number of msgs in MessageStacking queue.
                               #  (msgs waiting to be sent to socket)
 SendQueueLength         100   # Optional command: #msgs in SendQueue
                               #  (msgs sent, acknowledgments pending)
                               #  Valid range: 1-254 (default 100) 
                               #  shipped on export restarts.
#MaxLatency               60   # Optional command: Packets with delay time
                               #  (i.e., time between present and packet
                               #  start time) greater than this amount will be
                               #  filtered out. Time is in minutes.  0 -> time
                               #  is not checked. System time must be accurate.
                               #  Only works on waveform messages.
 
# Set up server/client communications
#------------------------------------
 ServerIPAdr     0.0.0.0       # Export messages via this ethernet card
                               # 0.0.0.0 is useful because the export listens
                               # on all available IP addresses, from loopback
                               # (127.0.0.1) to your actual external IP address.
                               # 0.0.0.0 may be sufficient, or you may want to
                               # be more specific about what IP address you
                               # export at.
 ServerPort            16005   # Well-known port number to export msgs on

 SendAliveText      ExpAlive   # string sent to client as heartbeat
 SendAliveInt             30   # seconds between alive msgs sent to client.
                               #  0 => no heartbeat
 RcvAliveText       ImpAlive   # text of client's heartbeat (we get this)
 RcvAliveInt              60   # seconds between client's heartbeats to us.
                               #  0 => no heartbeat

# Optional Socket commands:
# SocketTimeout defaults to RcvAliveInt + 3 
#  If set to -1, all socket calls will block (no timeout).
#  SocketTimeout has no effect in export, unless it is set to -1,
#  because there is no code in export to handle socket timeouts.
#  If set to -1, the program may run slightly more efficiently because
#  timeout checking code will not execute.
#----------------------------------------------------------------------
#SocketTimeout 200000  # Timeout length in milliseconds for socket calls

 SocketDebug   0       # if 1, socket_ew debug statements are logged
                       # if 0, socket_ew debug is turned off (default)

# List of station/channel/network/loc codes to export.
#--------------------------------------------------------
# NOTE: Send_scn and Send_scn_remap commands are now obsolete, and 
# have been replaced with Send_scnl and Send_scnl_remap (which 
# include the location code field).  If the old commands are used, 
# export_scn sets the location code field to * (wildcard).
#
# Use any combination of Send_scnl (to send data as-is) and 
#   Send_scnl_remap (to change the SCNL on the fly) commands.
# Use * as a wildcard for any field. A wildcard in the
#   "map to" fields of Send_scnl_remap means that field will
#   not be renamed.
#--------------------------------------------------------
#                  send SCNL     map to SCNL
 Send_scnl       JMP ASZ NC 01                # send this specific channel
 Send_scnl       JPS *   NC *                 # send all components of JPS NC
 Send_scnl_remap JGR VHZ NC --   *   EHZ * *  # change component code only
 Send_scnl_remap CAL *   NC *    ALM *   * *  # send all component of CAL, but
                                              # change the site code to ALM

