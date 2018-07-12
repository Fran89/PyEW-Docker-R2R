

#
# import_generic configuration file
#
# This is the companion receiver for export_generic and export_scn. It
# initiates a connect request to the export specified in this file. It
# will accept any legal message from the export it connected to, and put 
# it into the ring it's attached to. The logo of the message will the same 
# as it was in the originating system.
# If the heartbeat string it receives doesn't match, it will quit operations.
# If the expected heartbeat is not received within "SenderHeartRate" seconds,
# it will re-initialize the connection.
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

 MyModuleId     MOD_IMPORT_GENERIC   # module id for this import,
 RingName       PICK_RING            # transport ring to use for input/output,
 HeartBeatInt   30                  # Heartbeat interval in seconds
#                                     Should be >= SenderHeartRate
 LogFile        1                    # If 0, don't write logfile; if 1, do
                                     # if 2, write module log but not stderr/stdout

 MaxMsgSize     65000      	# maximum size (bytes) for input/output msgs
# Note: If an oversized message is received, it will be discarded, the 
#       socket will be closed, log an error, and it will connect and try
#	again. 

 MyAliveString  "alive"	# heartbeat text to foreign server machine
 MyAliveInt     30      	# seconds between alive msgs sent to server
				# 0 => no alive messages to sending machine
 
# Sender description follows:
SenderIpAdr	aaa.bbb.c.ddd	# Ip address of the machine we're to listen to
SenderPort	16005		# IP port number used by sender
SenderHeartRate	60		# Sender's heart beat interval in seconds
				# 0 => no heartbeat expected
SenderHeartText    "alive"	# Sender's heartbeat text

# Optional commands:

#SocketTimeout 60000 #Timeout for Socket_ew calls in milliseconds
                     #Should be atleast the SenderHeartRate
	       	     # Will default to SenderHeartRate
#HeartbeatDebug 0    #Change this to 1 to get heartbeat debug messages.
#SocketDebug 1       #Change this to 1 to get socket debug messagese.
