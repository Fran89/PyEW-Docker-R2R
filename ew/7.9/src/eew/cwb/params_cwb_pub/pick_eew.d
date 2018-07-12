
#
#                     Pick_ew's Configuration File
#
MyModId        MOD_PICK_EEW     # This instance of pick_ew

StaFile       "pick_eew.sta"    # Cofigured file for auto-picking


InRing           WAVE_RING    # Transport ring to find waveform data on,
OutRing          PICK_RING     # Transport ring to write output to,
HeartbeatInt            30     # Heartbeat interval, in seconds,
RestartLength          100     # Number of samples to process for restart
MaxGap                  15     # Maximum gap to interpolate
Debug                    0     # If 1, print debugging message


StorePicks 0				   # If 1, store picks in a file
Ignore_weight 5				   # Ignore picks with weight #num, If -1, disable this function.				   


EEWFile		sta_hisn_Z		   # Cofigured file of station location and gain factor


# Specify which messages to look at with Getlogo commands.
#   GetLogo <installation_id> <module_id> <message_type>
# The message_type must be either TYPE_TRACEBUF or TYPE_TRACEBUF2.
# Use as many GetLogo commands as you need.
# If no GetLogo commands are given, pick_ew will look at all
# TYPE_TRACEBUF and TYPE_TRACEBUF2 messages in InRing.
#-----------------------------------------------------------------
GetLogo  INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF2
