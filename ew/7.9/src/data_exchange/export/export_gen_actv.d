#   
#   export_gen_actv configuration file
#
#
 MyModuleId     MOD_EXPORT_GEN_ACTV # module id for this program
 RingName       PICK_RING           # transport ring to use for input/output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
#      
 LogFile        1                   # If 0, don't write logfile
#
# Logos of messages to export to client systems
#              Installation       Module       Message Type
 GetMsgLogo    ${EW_INST_ID}   MOD_WILDCARD     TYPE_PICK2K
 GetMsgLogo    ${EW_INST_ID}   MOD_WILDCARD     TYPE_CODA2K
 
#
 ReceiverIpAdr  128.110.129.19  # Import host IP Address
 ReceiverPort     16005         # Well-known port number to receive msgs on

 RcvAliveText     "RCValive"    # text of client's heartbeat (we get this)
 RcvAliveInt      150           # seconds between client's heartbeats to us.
                                # 0 => no heartbeat

 MaxMsgSize       4096          # maximum size (bytes) for input/output msgs
 RingSize         500           # number of msgs to buffer for export 

# 
# Sender (export) host parameters
#
 SendAliveText    "SNDalive"    # string sent to client as heartbeat 
 SendAliveInt     30            # seconds between alive msgs sent to client.
                                # 0=> no heartbeat
#
# Socket commands
#
 SocketTimeout    200000        # Timeout length in milliseconds for socket 
                                # calls this should be atleast the value of 
                                # the RcvAliveInt
 SocketDebug      0             # Optional: If 1, socket_ew debug statements 
                                # are logged. If 0, socket_ew debug is turned 
                                # off
#
# verbosity
#
 Verbose          0             # Optional: set to 0 for quiet

