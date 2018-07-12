#
# samtac2ew configuration file
#
# This code receives serial data stream (SDS) packets from the SAMTAC via
# a TCP/IP port, converts them into Earthworm trace buf messages, and
# stuffs them into a wave ring.
#

TcpAddr         192.168.1.5           # IP address of SAMTAC interface
TcpPort         5001            # TCP port number of SAMTAC interface

DeviceID		21				# SAMTAC DeviceID(serial), must be defined here for SCNL map

ModuleId        MOD_SAMTAC2EW        # module id for this import
RingName        WAVE_RING       # transport ring to use for input/output

LogFile         1               # If 0, don't output to logfile; if 1, do
                                # if 2, log to module log but not stderr/stdout

HeartbeatInt    30              # Heartbeat interval in seconds

# Optional commands:

SOH_int 300                     # State of Health interval, default 300 (sec)

CommTimeout   5000              # If unable to communicate with SAMTAC for X
                                # milliseconds, retry socket or terminate
                                # default: 5000 (milliseconds)

Debug 2                         # debug level: 0 or commented out for no debug
                                # logging; 1 - 4 for increasing verbosity
                                # default: no debug output

@SCNL_MAP.d