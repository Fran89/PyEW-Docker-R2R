#
# import_gen_pasv configuration file
#
# The intent is that specific import_xxx modules can be cloned from this.
# This program includes a site-specific routine "import_filter" which
# determines what to do with the received messages. The default 
# "import_filter" routine assumes that the message originated from
# an  "export_gen_pasv" with a default "export_filter" routine: That routine
# attaches the logo of the originating message as the first nine characters
# (3 groups of 3) of the outgoing message, and exports it.
# The local "import_filter" removes this logo from the message, decodes it,
# and broadcasts the message onto its ring under this logo.

 MyModuleId     MOD_INPUT            # module id for this import,
 RingName       INPUT_RING           # transport ring to use for input/output,
 HeartBeatInt   30                   # internal heartbeat interval in seconds
#
 LogFile        1                    # If 0, don't write logfile at all,

 MaxMsgSize     65000      	         # max size (bytes) for input/output msgs

# Note: Messages are assumed to terminate with two newline '\n\n'  
#       characters. The '\n\n' are both removed, and replaced by a zero byte.
#       Someday this should be an option defined in this file...
# Note: If an oversized message is received, it will be discarded, the 
#       socket will be closed, log an error, and it will connect and try
#	again. This might flood the log file if all incoming messages are
#	too long. Maybe it should terminate instead...

 MyAliveString      "alive"          # heartbeat text to foreign server machine
 MyAliveInt         120              # seconds between alive msgs sent to server
                                     # 0 => no alive messages to sending machine
 
 ReceiverIpAdr      127.0.0.1        # Import data through this ethernet card
 ReceiverPort       16010            # IP port to set up for connections

### Sender description follows:

 SenderIpAdr        0.0.0.0          # The IP address of the export host
 SenderNetMask      0.0.0.0          # OPTIONAL network mask setting to open to any sender IP on a network or a specific IP
                                     # 0.0.0.0 means match ANY incoming IP
                                     # 255.255.255.0 means match ANY incoming IP on the same network component of SenderIpAdr

 SenderHeartRate    60	             # Sender's heart beat interval in seconds
				                             # 0 => no heartbeat expected
 SenderHeartText    "alive"       # Sender's heartbeat text
 SocketTimeout      60000            # Timeout for Socket_ew calls in mili secs
                                     # Should be atleast the SenderHeartRate
 HeartbeatDebug     0                # Optional: 1 = to get heartbeat debug msgs.
 SocketDebug        0                # Optional: 1 = to get socket debug msgs.

