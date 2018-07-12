#
# This is scream2ew's parameter file
#
# Please check that line endings are set appropriately for your platform.
#

#  Basic Earthworm setup:
#
MyModuleId         MOD_SCREAM2EW  # module id for this instance of scream2ew 
RingName           WAVE_RING      # shared memory ring for input/output
LogFile            1              # 0 to turn off disk log file; if 1, do log.
                                  # if 2, log to module log but not stderr/stdout
Verbose            1              # 1=> log every packet. 0=> don't
HeartBeatInterval  30             # seconds between heartbeats
# SleepInterval      100          # Ms to sleep during each loop (Ignored, provided for compatability)
# Verbose	     1		  # Enable verbosity (Ignored, provided for compatability)
#
#
#
PortNumber         1567
Server             192.168.42.46  # If specified name or IP-number this module
                                  # will run in TCP-mode and requests data from
                                  # a screem server on the given PortNumber.
                                  # If unspecified this module will run in
                                  # UDP-mode and listen to all broadcast
                                  # messages on the specified PortNumber.
#
#
#

#
# These lines specify the mapping between the GCF system ID and stream ID and the earthworm
# network, station, location, channel and pin number.
#
# For example:
#  ChanInfo "D850 EKA12 AB CDE 00 BHZ 1"
#
# This line would map Scream packets with:
#  GCF system ID: D850
#  GCF stream ID: EKA12
#
# To:
#  Network:       AB
#  Station:       CDE
#  Location:      00
#  Channel:       BHZ
#  Pin number:    1
#
# If you do not wish to specify any location, use the sequence '--'.
#
ChanInfo "D850 EKA12 AB CDE 00 BHZ 1"
ChanInfo "D850 EKA13 AB CDE 01 BHZ 2"
