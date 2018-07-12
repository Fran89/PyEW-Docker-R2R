#
#                     Configuration file for scn2scnl
#
MyModuleId     MOD_SCN2SCNL  # Module id for this instance of scn2scnl,
InRing         WAVE_RING     # Where to find input messages,
OutRing        WAVE_RING2    # Where to put output messages,
                             #  InRing and OutRing may be the same ring.
HeartbeatInt   15            # Heartbeat interval in secs
                             #   if 0, send no heartbeats

# List SCN's that are to be converted and the SCNL to which they are to be
# renamed. Format is:
# SCN  sta chan net   sta chan net loc
# You can list SCN's explicitly or with wildcards. The only supported wildcard
# for station and network names is "*"; no other characters should be included
# in a station or network name if the wildcard is used.
# For channel names, you can use "*" or two letters followed by `?', as in 
# "HH?". The `?' wildcard is supported only as the third character of the
# channel name.
#
# The output SCNL must be given explicitly if the input SCNl is given
# explicitly.
# If a wildcard is used in the input SCN, you can use wildcards in the output 
# SCNL except in the location code. In the output SCNL, the wildcard means
# "use whatever was in the input S, C or N".
# 
# The explicit SCN's will be sorted with scn2scnl starts up so that a binary
# search can be used to quickly find a match.
# Incoming packets will be matched against this explicit list first. 
# Any packets that don't match an explicit SCN will then be matched against
# the wildcarded SCNs in the order they appear in this file.
# Any packets that do not match the wildcard list WILL NOT BE CONVERTED.
# To ensure that all packets are converted, you should end the SCN list
# with "SCN * * *  * * * --". This will assign the location code "--" to
# any packets not otherwise matched.
SCN  * * *  * * * --