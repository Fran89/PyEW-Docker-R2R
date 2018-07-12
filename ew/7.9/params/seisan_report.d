#
# This is seisan_report's parameter file

#  Basic Earthworm setup:
#
MyModuleId         MOD_REPORT     # module id for this instance of report 
RingName           SSG_HYPO_RING  # ring to get input from
HeartBeatInterval  60             # seconds between heartbeats
LogFile            2              # 0 -> stdout, 1 -> stdout + file, 2 -> file
Debug              1              # 0 for no diagnostics

# Delay between detection of event and sfile being written (sec)
# For wavefile to be in sfile this needs to be at least a few minutes
WriteDelay         600

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_WILDCARD      # hyp2000arc

# Directory to write seisan files in
SfileDir  ../../data/sausage_sfiles  

# Directory to find wav files
WavDir ../../data/sausage_triggers

# Agency for hypocentres and magnitudes. Note this can be a 
# maximum of 3 characters!! (Seisan requirement)
Agency BGS

# Switch to control whether alarm messages sent 0=no
AlarmMsg  1

