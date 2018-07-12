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


LISSaddr   anmo.iu.liss.org      # IP address of the LISS (server)
                                 # If you have DNS configured, you may also
                                 # give the domain name of the LISS.
LISSport    4000                 # Port number for LISS; often this is 4000

lenSEED      512                 # The size of SEED records to read from LISS
                                 # OPTIONAL; defaults to 512 bytes.

SocketTimeout  30                # Socket timeout in seconds; not more than
                                 # HeartBeatInterval

TraceLength    100               # Number of trace data samples to put in one
                                 # TRACE_BUF packet. Range is 10 - 1000

MaxSCNs        3                 # Number of SCNs to accept; must be at least
                                 # the number of AcceptSCNL commands.
# List each sta/comp/net/location that you expect from LISS in an 
# "AcceptSCNL" command. Location code (LC) is a 2-digit code to further
# identify a seismological data path. LC (and ONLY LC) may be given as `*' to
# specify a wildcard. If the LISS has more than one LC for a give SCN,
# using the wildcard will generate errors in log.
# Do NOT specify the same SCN with two different LC values, as this will
# cause great confusion within earthworm!!!!
# If liss2ew sees an SCNL which is not listed here, it will be ignored.
# On each line after the SCNL, list a pinnumber to use for this SCNL.

#          site comp net LC pinno
#          ---- ---- --- -- -----
AcceptSCNL  ANMO BHZ  IU 00 1234
AcceptSCNL  ANMO BH1  IU 00 1235
AcceptSCNL  ANMO BH2  IU 00 1236
