#
# import_ack configuration file
#
# This is the companion receiver for export_ack and export_scnl_ack. 
#
# It initiates a connect request to the export specified in this file. 
# It will accept any legal message from the export it connects to 
# and put it into the ring it's attached to. The logo of the message 
# will the same as it was in the originating system.
# It will return an acknowledgment packet for every packet it receives.
# If the heartbeat string it receives doesn't match, it will put it 
# the message into the ring as a TYPE_HEARTBEAT message, but it will
# not drop the connection.
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
# Configuration notes:
#
# "restartMe" should be uncomment in our .desc file.
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
# If SocketTimeout is specified, it should be at least as large as the
# expected period of heartbeats from our distant partner.
#

# Basic Earthworm setup
#----------------------
 MyModuleId     MOD_IMPORT_ACK  # module id for this import,
 RingName       PICK_RING       # transport ring to use for input/output,
 HeartBeatInt   90              # Heartbeat interval in seconds
                                #    should be >= RcvAliveInt
 LogFile        1               # If 0, don't write log file 
                                #    1, do write log file
                                #    2, write module log but not stderr/stdout

 MaxMsgSize     65000      	# maximum size (bytes) for input/output msgs

# Socket communication info
#--------------------------
 SendAliveText  ImpAlive	# heartbeat text to foreign server machine
 SendAliveInt   30      	# seconds between alive msgs sent to server
				#   0 = no alive messages to sending machine
 
# Description of export partner
#------------------------------
 ServerIPAdr	aaa.bbb.c.ddd	# Ip address of the machine we're to listen to
 ServerPort	16005		# IP port number used by sender
 RcvAliveText   ExpAlive        # export's heartbeat text
 RcvAliveInt	60		# export's heart beat interval in seconds
				#   0 = no heartbeat expected

# Optional commands
#------------------
#SocketTimeout  60000   # Timeout for Socket_ew calls in milliseconds
                        #   Should be at least the RcvAliveInt
	       	        #   Will default to RcvAliveInt
#HeartbeatDebug 0       # non-zero to get heartbeat debug messages.
#SocketDebug    1       # non-zero to get socket debug messagese.

#LogoRewrite    1       # rewrite the logo as my own instead of the imported ModID and InstID
