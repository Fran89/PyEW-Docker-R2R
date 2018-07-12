
#
# This is menlo_report's parameter file

#  Basic Earthworm setup:
#
MyModuleId         WEB_REPORT     # module id for this instance of report 
RingName           HYPO_RING      # ring to get input from
LogFile            1              # 0 to completely turn off disk log file
                                  # 2 to log to module log but not stderr/stdout
HeartBeatInterval  30             # seconds between heartbeats

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_EQPROC     # hyp2000arc & h71sum2k

# Webserver directory to write  files in
WebDir      c:\earthworm

# Webserver directory to write  files in
WebDir      c:\earthworm

WebFileName index.html

TotalLocations 20
