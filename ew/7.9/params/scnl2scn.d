#
#                     Configuration file for scnl2scn
#
MyModuleId     MOD_SCNL2SCN  # Module id for this instance of scnl2scn,
InRing         WAVE_RING     # Where to find input messages,
OutRing        WAVE_RING2    # Where to put output messages,
                             #  InRing and OutRing may be the same ring.
HeartbeatInt   15            # Heartbeat interval in secs
                             #   if 0, send no heartbeats
Debug			     # Optional command, no parameters
			     # logs every SCNL is sees!   
# List SCNL's that are to be converted and the SCN to which they are to be
# renamed. Format is:
# SCNL  sta chan net loc   sta chan net
# You can list SCNL's explicitly or with wildcards. The only supported wildcard
# for station and network names is "*"; no other characters should be included
# in a station, network or location name if the wildcard is used.
# For channel names, you can use "*" or two letters followed by `?', as in 
# "HH?". The `?' wildcard is supported only as the third character of the
# channel name.
#
# The output SCN must be given explicitly if the input SCNL is given
# explicitly.
# If a wildcard is used in the input SCNL, you can use wildcards in the output.
# In the output SCN, the wildcard means "use whatever was in the 
# input S, C or N".
# 
# The explicit SCNL's will be sorted with scnl2scn starts up so that a binary
# search can be used to quickly find a match.
# Incoming packets will be matched against this explicit list first. 
# Any packets that don't match an explicit SCNL will then be matched against
# the wildcarded SCNLs in the order they appear in this file.
# Any packets that do not match the wildcard list WILL NOT BE CONVERTED.
# To ensure that all packets are converted, you could end the SCNL list
# with "SCNL * * * *  * * *". This will simply delete the location code from
# any packets not otherwise matched.
# Be careful with this, however. It may cause multiple SCNLs to be converted
# into a single SCN, producing utter garbage!
SCNL  * * * *  * * *
