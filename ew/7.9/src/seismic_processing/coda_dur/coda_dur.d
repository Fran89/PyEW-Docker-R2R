#
#  coda_dur's Configuration File
#
MyModId       MOD_CODA_DUR     # This instance of pick_ew
HeartbeatInt            30     # Heartbeat interval, in seconds,
Debug                    1     # If 1, print debugging message
StaFile       "pick_ew.sta"    # File containing station name/pin# info

# Info about coda avg absolute values 
#-----------------------------------------------------------------
CodaAAVRing        WAVE_RING                    # where to find CAAVs
GetCodaAAVLogo     INST_WILDCARD  MOD_WILDCARD  # TYPE_CODA_AAV
MaxCodaAAVperSCNL  180                          # default=180

# Info about picks
#-----------------------------------------------------------------
PickRing           PICK_RING                    # where to find picks
GetPickLogo        INST_WILDCARD  MOD_WILDCARD  # TYPE_PICK_SCNL
MaxPickperSCNL     5                            # default=5

# THE END
