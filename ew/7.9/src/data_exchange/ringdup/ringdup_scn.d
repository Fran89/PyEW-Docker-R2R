
#  ringdup_scn configuration file
#
#
 MyModuleId     MOD_RINGDUP_SCN     # module id for this program
 InRing         WAVE_RING           # transport ring to use for input
 OutRing        EXPPT_RING          # transport ring to use for output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile; if 1, do
                                    # if 2, write to module log but not to 
                                    # stderr/stdout

 UseOriginalLogo 1                  # If 0, use this module's instid/modid when
                                    #   writing all msgs to OutRing
                                    # If non-zero, apply a message's original logo
                                    #   when duplicating it to OutRing (default)
#
# Logos of messages to export to client systems
#              Installation       Module       Message Type
 GetMsgLogo    INST_WILDCARD   MOD_WILDCARD    TYPE_TRACEBUF2
 
 MaxMsgSize     4096          # maximum size (bytes) for input/output msgs
 RingSize	 100          # max# messages in outgoing circular msg buffer 


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
 Block_scnl      BOZ LHZ US *                 # block this specific channel
 Send_scnl       JMP ASZ NC 01                # send this specific channel
 Send_scnl       JPS *   NC *                 # send all components of JPS NC
 Send_scnl_remap JGR VHZ NC --   *   EHZ * *  # change component code only
 Send_scnl_remap CAL *   NC *    ALM *   * *  # send all component of CAL, but
                                              # change the site code to ALM

