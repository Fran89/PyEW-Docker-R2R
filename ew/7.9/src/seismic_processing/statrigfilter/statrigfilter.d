# statrigfilter configuration file

# Basic Earthworm setup:
#------------------------
 MyModuleId   MOD_STATRIGFILTER # module id for this instance of statrigfilter 
 InRing       PICK_RING         # shared memory ring for input
 OutRing      FILTERPICK_RING   # shared memory ring for output
 HeartbeatInt 30                # seconds between heartbeats
 LogFile      1                 # 0 log to stderr/stdout only 
                                # 1 log to stderr/stdout and to disk file
                                # 2 log to disk file only
 Debug        1

# List the message logos to grab from InRing, WILDCARDs permitted.
# Multiple "GetLogo" commands are allowed, with no hardcoded limit.
#         Installation    Module      Message Types
#-----------------------------------------------------------------
 GetLogo  INST_WILDCARD  MOD_WILDCARD # TYPE_CARLSTATRIG & TYPE_CARLSTATRIG_SCNL

# Trigger Filtering Parameters
#-----------------------------
 TriggerHistory       5      # Keep track of this many triggers which have
                             # made it thru the filter for each station.

 TimeTolerance        2.0    # If trigger times are within this many seconds of
                             # each other, they are "duplicates."

 OlderTrigAllowed     1      # 0=reject any non-duplicate trigger whose 
                             #   timestamp is earlier than the youngest 
                             #   passed trigger for this station.
                             # 1=accept a non-duplicate trigger whose timestamp
                             #   is earlier than the youngest passed trigger,
                             #   but place a limit on how old it can be.
                             #   Must also use the "OlderTrigLimit" command.
                             # 2=accept any non-duplicate trigger whose 
                             #   timestamp is earlier than the youngest   
                             #   passed trigger.

 OlderTrigLimit       10     # Required only if OlderTrigAllowed = 1
                             # Accept an trigger whose timestamp is between
                             # TimeTolerance and OlderTrigLimit sec     
                             # earlier than the youngest passed trigger
                             # from this station.

# List of Allowed Components
#---------------------------
# If you want ALL component codes to be eligible to pass thru 
# statrigfilter, comment out all "AllowComponent" commands.
# If you want to limit which component codes pass thru statrigfilter,
# use one "AllowComponent" command for each allowed component code.
#AllowComponent   VDZ
#AllowComponent   VHZ
#AllowComponent   VLZ
#AllowComponent   SHZ
#AllowComponent   SLZ
#AllowComponent   EHZ
#AllowComponent   HHZ
#AllowComponent   BHZ
#AllowComponent   DP1
#AllowComponent   DP2
#AllowComponent   DP3
#AllowComponent   EP1
#AllowComponent   EP2
#AllowComponent   EP3

# Optional commands:
#-------------------
 MaxMessageSize   128        # length (bytes) of largest msg to be processed
                             # from InRing.  Default=MAX_BYTES_PER_EQ

 UseOriginalLogo  1          # 0  apply statrigfilter's logo to any msgs that
                             #    pass the filter test (normal Earthworm behavior)
                             # non-zero means apply the original logo to any
                             #    messages that pass the filter test.  This
                             #    also requires that InRing and OutRing be 
                             #    different to avoid "missed message" or
                             #    "sequence gap" storms in trigger-reading modules.
