#
#  coda_aav's Configuration File
#
MyModId       MOD_CODA_AAV     # This instance of pick_ew
StaFile       "pick_ew.sta"    # File containing station name/pin# info
InRing           WAVE_RING     # Transport ring to find waveform data on,
OutRing          PICK_RING     # Transport ring to write output to,
HeartbeatInt            30     # Heartbeat interval, in seconds,
Debug                    0     # If 1, print debugging message

# Specify which messages to look at with Getlogo commands.
#   GetLogo <installation_id> <module_id> <message_type>
# The message_type must be either TYPE_TRACEBUF or TYPE_TRACEBUF2.
# Use as many GetLogo commands as you need.
# If no GetLogo commands are given, pick_ew will look at all
# TYPE_TRACEBUF and TYPE_TRACEBUF2 messages in InRing.
#-----------------------------------------------------------------
GetLogo  INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF2
