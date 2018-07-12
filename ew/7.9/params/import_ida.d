# import_ida parameter file

#  Basic Earthworm setup:

MyModuleId         MOD_IMPORT_IDA # module id for this process
OutRing            WAVE_RING      # output ring
HeartBeatInterval  30             # seconds between heartbeats


# Pin No.  funtionality has been removed
# 2005/04/20   DK   Per AB
# PinnoFile    - NO LONGER SUPPORTED
# DefaultPinno - NO LONGER SUPPORTED
#

# Define the policy to follow when reconnecting after circuit timeouts.
# There are two choices: MINIMAL_GAP, or MINIMAL_DELAY.  If the former
# is selected then the program will request the flow start at the end of
# whatever has been received so far.  If the latter is selected then the
# program will request the flow start with the most recent packet available
# on the server.

Policy             MINIMAL_DELAY

# If the policy is MINIMAL_DELAY but the data server is a hub running
# with MINIMAL_GAP policy (as is usually the case for IDA hubs) then
# you can run into a situation where late data can be delivered.  This
# is because the data are "current" as far as the data source is
# concerned but the timestamps therein may be too late to be of interest
# as far as the data consumer is concerned.  This can result in
# wasted bandwidth.  To avoid this, set the MaximumLag parameter
# to be the maximum interval (in seconds) between current host time (GMT)
# and the data time stamps which are acceptable.  If the data exceed this
# threshold then import_ida will break the current data feed and re-issue
# a new request for data starting at the current time.  This parameter is
# ignored if the policy is MIMINAL_GAP.

MaximumLag        3600

# The network code for Project IDA stations is II, and that is the default
# code which gets loaded into the packet headers.  If you want to override
# this with some other code, enter it here.

Network            II

# List the server(s) and stream(s) that are desired.
# The station channel specifications should be of the form
# sta1:chn1,chn2,...,chnN+sta2:chn1,chn2,...
#
# You can give either the fully qualified host name, or its IP address
# in the usual decmial dot notation (eg, 132.239.4.71)
#
#          Hostname              Station/channel list

server idahub.ucsd.edu           nna:bhz,bhn,bhe+pfo:bhz,bhn,bhe+sur:bhz,bhn,bhe
server kdak-gw.giseis.alaska.edu kdak:bhz,bhn,bhe
server hukapele.ptwc.noaa.gov    kip:bhz,bhn,bhe
