
#   ringdup_generic configuration file
#
#
 MyModuleId     MOD_RINGDUP_GENERIC  # module id for this program
 InRing         WAVE_RING           # transport ring to use for input
 OutRing        EXPPT_RING          # transport ring to use for output
 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile
                                    # if 2, write to module log but not stderr/stdout

 UseOriginalLogo 1                  # If 0, use this module's instid/modid when
                                    #   writing all msgs to OutRing
                                    # If non-zero, apply a message's original logo
                                    #   when duplicating it to OutRing (default)

#
# Logos of messages to export to client systems
#              Installation       Module       Message Type
 GetMsgLogo    INST_WILDCARD   MOD_WILDCARD     TYPE_H71SUM2K
 
 MaxMsgSize       128          # maximum size (bytes) for input/output msgs
 RingSize          10          # number of msgs to buffer
