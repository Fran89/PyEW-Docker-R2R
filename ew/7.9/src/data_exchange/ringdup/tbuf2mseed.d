
# The tbuf2mseed command file is very similar to the ringdup_scn command file, as
# it's basically the same program, just compiled to write mseed out instead of tbuf2s
#
#  tbuf2mseed configuration file
#
#
 MyModuleId     MOD_TBUF2MSEED      # module id for this program
 InRing         WAVE_RING           # transport ring to use for input
 OutRing        MSEED_RING          # transport ring to use for output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile; if 1, do
                                    # if 2, write to module log but not to 
                                    # stderr/stdout
 Debug     0                        # set to 1 to see mseed packet creations
#
# Logos of messages to export to client systems
#              Installation       Module       Message Type
 GetMsgLogo    INST_WILDCARD   MOD_WILDCARD    TYPE_TRACEBUF2
 
 MaxMsgSize     4096          # maximum size (bytes) for input/output msgs
 RingSize	100000          # max# messages in outgoing circular msg buffer 

#
# Optional parameters
#
#FlushLatency 300             # flush any buffered data streams idle for this many seconds
#Int32Encoding STEIM2         # specify miniSEED encoding for 32-bit integers, default STEIM2
                              #   possible values are STEIM2, STEIM1 or INT32 (no compression)

# List of station/channel/network/loc codes to duplicate.
#--------------------------------------------------------
# NOTE: Send_scn and Send_scn_remap commands are now obsolete, and
# have been replaced with Send_scnl and Send_scnl_remap (which
# include the location code field).  If the old commands are used,
# ringdup_scn sets the location code field to * (wildcard).
#
# Use any combination of Send_scnl (to send data as-is) and
#   Send_scnl_remap (to change the SCNL on the fly) commands.
# Use * as a wildcard for any field. A wildcard in the
#   "map to" fields of Send_scnl_remap means that field will
#   not be renamed.
# Use the Block_scnl command (works with both wildcards
#   and not, but only makes sense to use with wildcards) to block
#   any specific channels that you don't want sent.
#   Note, the Block commands must precede any wildcard commands for 
#   the blocking to occur.
#
#                  send SCNL     map to SCNL
# Block_scnl      BOZ LHZ US *                 # block this specific channel
# Send_scnl       JMP ASZ NC 01                # send this specific channel
# Send_scnl       JPS *   NC *                 # send all components of JPS NC
# Send_scnl_remap JGR VHZ NC --   *   EHZ * *  # change component code only
# Send_scnl_remap CAL *   NC *    ALM *   * *  # send all component of CAL, but
                                               # change the site code to ALM
Send_scnl * *  NM *
