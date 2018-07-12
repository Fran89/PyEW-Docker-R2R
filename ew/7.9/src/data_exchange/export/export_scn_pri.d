#  export_scn_pri configuration file
#
#  Exports messages from a given list of station/comp/net/loc codes
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
 MyModuleId     MOD_EXPORT_SCN      # module id for this program
 RingName       WAVE_RING           # transport ring to use for input/output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
#                                     Should be >= RcvAliveInt 
 LogFile        1                   # If 0, don't write logfile; if 1, do.
                                    # if 2, write to module log but not to 
                                    # stderr/stdout

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
 GetMsgLogo    INST_WILDCARD      MOD_WILDCARD     TYPE_TRACEBUF2
 
# ServerIPAdr and ServerPort specify the local IP address and port number
# on which this module will be ready to accept a client connection.
#
 ServerIPAdr  aaa.bbb.cc.ddd   # Export messages via this ethernet card 
 ServerPort     16005          # Well-known port number to export msgs on

 MaxMsgSize       900          # maximum size (bytes) for input/output msgs
 RingSize	  100	       # max# messages in outgoing circular msg buffer 

 SendAliveText   "alive"       # string sent to client as heartbeat 
 SendAliveInt      30          # seconds between alive msgs sent to client.
                               #  0=> no heartbeat
 RcvAliveText    "alive"       # text of client's heartbeat (we get this)
 RcvAliveInt       150         # seconds between client's heartbeats to us.
                               #  0 => no heartbeat
 MaxLatency         60         # Optional command; Packets with delay time
                               #  (i.e., time between present and packet
                               #  start time) greater than this amount will be
                               #  filtered out.  Time is in minutes.  0 -> time
                               #  is not checked.  System time must be accurate.
                               #  Only works on TYPE_TRACEBUF messages.

# Socket commands (optional)
#---------------------------
# SocketTimeout defaults to RcvAliveInt + 3
#  If set to -1, all socket calls will block (no timeout).
#  SocketTimeout has no effect in export, unless it is set to -1,
#  because there is no code in export to handle socket timeouts.
#  If set to -1, the program may run slightly more efficiently because
#  timeout checking code will not execute.
#
#SocketTimeout 200000  # Timeout length in milliseconds for socket calls
 
#SocketDebug   0       # if 1, socket_ew debug statements are logged
                       # if 0, socket_ew debug is turned off (default)
 

# List of station/channel/network/loc codes (and optional priorities) to export
#------------------------------------------------------------------------------
# NOTE: Send_scn and Send_scn_remap commands are now obsolete, and
# have been replaced with Send_scnl and Send_scnl_remap (which
# include the location code field).  If the old commands are used,
# export_scn_pri sets the location code field to * (wildcard).
#
# Export_scn_pri adds an optional parameter for the priority as
# the last element of the Send_scnl and Send_scnl_remap commands. 
# Valid priority values are 1 (highest) through 9 (lowest).
# If any other value is found, or if the value is missing, 
# then the default (9) will be used. 
#
Send_scnl JMP VFZ NC 01 1   # export waveforms from this station at priority 1 (highest)
Send_scnl JMP VFN NC 01 1   # export waveforms from this station at priority 1
Send_scnl JMP VFE NC 01 5   # export waveforms from this station at priority 5
Send_scnl JMP ASZ NC 01 3   # export waveforms from this station at priority 3
Send_scnl JMP ASN NC 03     # export waveforms from this station at default priority (9) 
