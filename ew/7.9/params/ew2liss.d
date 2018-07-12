#
#                     Configuration File for ew2liss
#
MyModId          MOD_EW2LISS
InRing           WAVE_RING       # Transport ring to read trace data from.
HeartBeatInterval     30         # Heartbeat interval, in seconds,
LogFile            1             # 1 -> Keep log, 0 -> no log file
                                 # 2 -> write to module log but not stderr/stdout
#
# Specify logo of the messages to grab from the InRing.
# TYPE_TRACEBUF is assumed, therefore only module ID and 
# installation ID need to be specified
#
GetWavesFrom    INST_WILDCARD MOD_WILDCARD  # TYPE_TRACEBUF (assumed)


LISSaddr   123.45.6.78           # IP address that will be used for serving
                                 # If you have DNS configured, you may also
                                 # give the domain name of the LISS.
LISSport    4000                 # Port number for LISS; often this is 4000

SocketTimeout  30                # Socket timeout in seconds; not more than
                                 # HeartBeatInterval; optional

SeqFile    sea.lsn               # Name of sequence number file.
MaxSCNs        3                 # Number of SCNs to accept; must be at least
                                 # the number of ServeSCN commands.
# List each sta/comp/net that you want to serve with a 
# "ServeSCN" command. Location code (LC) is a 2-digit numerical code that 
# you may use to further identify the data stream. 
# Location code may be omitted (left blank); all other entries must be
# given explicitly (no wildcards).

#          site comp net LC
#          ---- ---- --- --
ServeSCN  SEA   BHZ  UW 00 
ServeSCN  SEA   BHN  UW 00 
ServeSCN  SEA   BHE  UW 00 
