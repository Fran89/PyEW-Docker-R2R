
#
#                     Pick_ew's Configuration File
#
MyModId        MOD_PICK_EW     # This instance of pick_ew
StaFile       "pick_ew.sta"    # File containing station name/pin# info
InRing           WAVE_RING     # Transport ring to find waveform data on,
OutRing          PICK_RING     # Transport ring to write output to,
HeartbeatInt            30     # Heartbeat interval, in seconds,
RestartLength          100     # Number of samples to process for restart
MaxGap                  15     # Maximum gap to interpolate
Debug                    0     # If 1, print debugging message


#NoCoda  1  # if this OPTIONAL flag is turned on, then No Coda values are computed from any channel
	    # this has the advantage of allowing pick_ew to pick MORE phases, like S 
	    # it has the disadvantage that you will not get any Coda magnitudes


#NoCodaHorizontal  1 # do not compute coda values on horizontal components (only those ending in Z)
		     # has no effect if NoCoda is also set on

# PickIndexDir  dir_name  # OPTIONAL direcive to put the pick index files in a separate directory 
			  # otherwise defaults to $EW_PARAMS directory (which can clutter things up)

# Specify which messages to look at with Getlogo commands.
#   GetLogo <installation_id> <module_id> <message_type>
# The message_type must be either TYPE_TRACEBUF or TYPE_TRACEBUF2.
# Use as many GetLogo commands as you need.
# If no GetLogo commands are given, pick_ew will look at all
# TYPE_TRACEBUF and TYPE_TRACEBUF2 messages in InRing.
#-----------------------------------------------------------------
GetLogo  INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF2
