#  
#  export_scnl_actv configuration file
#
#  Exports messages from a given list of station/channel/net/loc codes
#
 MyModuleId     MOD_EXPORT_SCN_ACTV # module id for this program
 RingName       WAVE_RING           # transport ring to use for input/output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
#                                   
 LogFile        1                   # If 0, don't write logfile
#
# Logos of messages to export to client systems.
#------------------------------------------------
# Installation and Module can be wildcards.
# Knows how to decipher these message types:
#  w/  location code: TYPE_TRACEBUF2, TYPE_TRACE2_COMP_UA, 
#                     TYPE_PICK_SCNL, TYPE_CODA_SCNL
#                     TYPE_STRONGMOTIONII
#  w/o location code: TYPE_TRACEBUF, TYPE_TRACE_COMP_UA,
#                     TYPE_PICK2K, TYPE_CODA2K msgs
#
#              Installation       Module           Message Type
 GetMsgLogo    ${EW_INST_ID}       MOD_WILDCARD     TYPE_TRACEBUF2
 
# 
# Receiver (import) host parameters
#---------------------------------------
 ReceiverIpAdr    aaa.bbb.ccc.ddd   # Import host IP Address
 ReceiverPort     16005             # Well-known port number to receive msgs on
 
 RcvAliveText     "RCValive"        # text of client's heartbeat (we get this)
 RcvAliveInt      150               # seconds between client's heartbeats to us.
                                    # 0 => no heartbeat

 MaxMsgSize       4096              # maximum size (bytes) for input/output msgs
 RingSize	  500               # max msgs in outgoing circular msg buffer 

# 
# Sender (export) host parameters
#---------------------------------------
 SendAliveText    "SNDalive"        # string sent to client as heartbeat 
 SendAliveInt     30                # seconds between alive msgs sent to client.
                                    # 0=> no heartbeat
#
# Socket commands (optional)
#---------------------------------------
 SocketTimeout    200000            # Timeout length in milliseconds for socket
                                    # calls this should be atleast the value of
                                    # the RcvAliveInt
 SocketDebug      0                 # Optional: If 1, socket_ew debug statements 
                                    # are logged. If 0, socket_ew debug is turned 
                                    # off

 Verbose          0                 # Optional: set to 0 for quiet

# List of station/channel/network/loc codes to export.
#--------------------------------------------------------
# NOTE: Send_scn and Send_scn_remap commands are now obsolete, and
# have been replaced with Send_scnl and Send_scnl_remap (which
# include the location code field).  If the old commands are used,
# export_scnl sets the location code field to * (wildcard).
#
# Use any combination of Send_scnl (to send data as-is) and
#   Send_scnl_remap (to change the SCNL on the fly) commands.
# Use * as a wildcard for any field. A wildcard in the
#   "map to" fields of Send_scnl_remap means that field will
#   not be renamed.
#
#                  send SCNL     map to SCNL
 Send_scnl       JMP ASZ NC 01                # send this specific channel
 Send_scnl       JPS *   NC *                 # send all components of JPS NC
 Send_scnl_remap JGR VHZ NC --   *   EHZ * *  # change component code only
 Send_scnl_remap CAL *   NC *    ALM *   * *  # send all component of CAL, but
                                              # change the site code to ALM
