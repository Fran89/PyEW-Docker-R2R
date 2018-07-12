#
#      Configuration File for decompress_UA
#
# CHANGE NOTICE: The "GetWavesFrom" command has been eliminated
# in this version. It is replaced by the "GetLogo" command.

 MyModId   MOD_DECOMPRESS_UA   # This instance of decompress_UA.
 InRing            COMP_RING   # Transport ring to find compressed data on.
 OutRing           WAVE_RING   # Transport ring to write output to.
 HeartBeatInterval 30          # Heartbeat interval, in seconds.
 LogFile           1           # 1=write a log file to disk, 0=don't
                               # 2=write to module log but not to stderr/stdout

 UseOriginalLogo   0           # 0=use decompress_UA's logo on outgoing trace msg
                               # non-zero=use instid&modid of original compressed msg
                               #          on outgoing trace msg

# List specific Logo(s) to process with one or more "GetLogo"
# commands. Wildcards are allowed for installation and module id.
# Message type must be either TYPE_TRACE_COMP_UA or TYPE_TRACE2_COMP_UA.
# If input is TYPE_TRACE_COMP_UA,  output will be TYPE_TRACEBUF.
# If input is TYPE_TRACE2_COMP_UA, output will be TYPE_TRACEBUF2.

 GetLogo  INST_WILDCARD MOD_WILDCARD TYPE_TRACE2_COMP_UA
 


