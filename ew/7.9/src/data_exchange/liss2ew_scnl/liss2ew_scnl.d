#
#                     Configuration File for liss2ew
#

MyModId          MOD_LISS2EW
OutRing          WAVE_RING       # Transport ring to write output to,
                                 # InRing and OutRing may be the same.
HeartBeatInterval     30         # Heartbeat interval, in seconds,
LogFile            1             # 1 -> Keep log, 0 -> no log file
                                 # 2 -> write to module log but not stderr/stdout
#Debug		 		 # Write out debug messages (optional)


lenSEED      512                 # The size of SEED records to read from LISS
                                 # OPTIONAL; defaults to 512 bytes.

SocketTimeout  30                # Socket timeout in seconds; not more than
                                 # HeartBeatInterval

TraceLength    100               # Number of trace data samples to put in one
                                 # TRACE_BUF packet. Range is 10 - 1000

# List each Internet source in an "Address" command, followed by the IP number (or
# DNS name) and port number.
# Following each Address command, list one or more sta/comp/net/location expected from
# that source in an "SCNL" command, followed by the station, component, network and
# location codes.  Location code (LC) is a 2-digit code to further
# identify a seismological data path. LC (and ONLY LC) may be given as `*' to
# specify a wildcard. If the LISS has more than one LC for a give SCN,
# using the wildcard will generate errors in the log file.
# If liss2ew sees an SCNL which is not listed here, it will be ignored.

Address     136.177.24.74    4168
SCNL  WMOK BHZ  US  00 
SCNL  WMOK BHN  US  00  
SCNL  WMOK BHE  US  00 
SCNL  WMOK LHZ  US  00 
SCNL  WMOK LHN  US  00 
SCNL  WMOK LHE  US  00 

Address	136.177.24.74    4166
SCNL  AFI  BHE  IU  00
SCNL  AFI  BHN  IU  00
SCNL  AFI  BHZ  IU  00
SCNL  AFI  LHE  IU  00
SCNL  AFI  LHN  IU  00
SCNL  AFI  LHZ  IU  00

