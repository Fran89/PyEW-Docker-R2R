#
#            compress_UA's Configuration File
#
# CHANGE NOTICE: The "GetWavesFrom" command has been eliminated 
# in this version. It is replaced by two new commands:
#  "GetLogo" to supply the logos (installation,moduleid,msgtype)
#  "CompressSCNL" to supply the list of channels (sta,comp,net,loc)

 MyModId      MOD_COMPRESS_UA   # This instance of compress_UA
 InRing             WAVE_RING   # Transport ring to find waveform data on,
 OutRing            COMP_RING   # Transport ring to write output to,
 HeartBeatInterval  15          # Heartbeat interval, in seconds,
 LogFile            1           # 1=write a log file to disk, 0=don't
                                # 2=write a module log but not to stderr/stdout

 UseOriginalLogo    0           # 0=use compress_UA's logo on outgoing compressed msgs
                                # non-zero=use instid & modid of original trace msg
                                #          on outgoing compressed msg

# List specific Logo(s) to process with one or more "GetLogo"
# commands. Wildcards are allowed for installation and module id. 
# Message type must be either TYPE_TRACEBUF or TYPE_TRACEBUF2.
# If input is TYPE_TRACEBUF,  output will be TYPE_TRACE_COMP_UA.
# If input is TYPE_TRACEBUF2, output will be TYPE_TRACE2_COMP_UA.

 GetLogo  INST_WILDCARD MOD_WILDCARD TYPE_TRACEBUF2

# Use as many "CompressSCNL" commands as required to define
# the list of channels to process.  If input is TYPE_TRACEBUF,
# the location code field is ignored, but must still be supplied.
# Wildcard character '*' is allowed in any SCNL field.
#
#               sta   cmp net loc
 CompressSCNL   PHL   HHN CI  01 
 CompressSCNL   *     VHZ NC  * 
 CompressSCNL   OMM   *   NN  * 
