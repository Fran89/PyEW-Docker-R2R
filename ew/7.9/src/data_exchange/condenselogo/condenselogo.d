# condenselogo configuration file
#   Reads messages from a list of logos from one transport ring 
#   and writes them to another ring using its own installation and
#   moduleid in the logo.  Does not alter the contents of the 
#   message in any way.

#  Basic Earthworm setup:
#
MyModuleId    MOD_K2ALL      # module id for this instance of condenselogo 
InRing        K2_RING        # shared memory ring for input
OutRing       WAVE_RING      # shared memory ring for output
HeartbeatInt  30             # seconds between heartbeats
LogFile       1              # 0 log to stderr/stdout only 
                             # 1 log to stderr/stdout and to disk file
                             # 2 log to disk file only

# List the message logos to grab from InRing, WILDCARDs permitted.
# Multiple "GetLogo" commands are allowed, with no hardcoded limit.
#         Installation    Module      Message Types
#-----------------------------------------------------------------
GetLogo  INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF2
# or specific installations:
GetLogo  INST_PRSN      MOD_WILDCARD  TYPE_TRACEBUF2


# Optional commands:
#-------------------
MaxMessageSize      4096     # length (bytes) of largest msg to be processed
                             # from InRing.  Default=MAX_BYTES_PER_EQ

UseOriginalInstid   0        # If 0, replace both inst_id and module_id (default).
                             # If non-zero, use original installation_id in
                             #   outgoing logo (replace only the module_id).

#UseNewInstid INST_BLAH     # rewrite installation ID if necessary!
                             
# new PTWC features (OPTIONAL)

# get specific stations if a TRACEBUF2 message
#Sta   LGNH.--.CN.--
#Sta   PAPH.--.CN.--
#Sta   FDF.--.G.--
#Sta   HDC.--.G.--

# or get all from a given network:
#Net CU
