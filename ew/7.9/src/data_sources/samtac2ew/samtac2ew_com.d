#
# samtac2ew configuration file
#
# This code receives serial data stream (SDS) packets from the SAMTAC-802U via
# a WindowsNT serial port, converts them into Earthworm trace buf messages,
# and stuffs them into a wave ring.
#

ComPort         1               # SAMTAC COM port ID, 1=COM1, 2=COM, etc
Speed           57600           # SAMTAC baud rate

ModuleId        MOD_SAMTAC2EW   # module id for this import
RingName        WAVE_RING       # transport ring to use for input/output

LogFile         1               # If 0, don't output to logfile; if 1, do
                                # if 2, log to module log but not stderr/stdout

HeartbeatInt    30              # Heartbeat interval in seconds

# Optional commands:

SOH_int 300                     # State of Health interval, default 300 (sec)

CommTimeout   5000              # If unable to communicate with K2 for X
                                # milliseconds, retry comm port or terminate
                                # default: 5000 (milliseconds)

Debug 4                         # debug level: 0 or commented out for no debug
                                # logging; 1 - 4 for increasing verbosity
                                # default: 0 (no debug output)

@SCNL_MAP.d