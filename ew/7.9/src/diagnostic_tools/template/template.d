
# This is template's parameter file

#  Basic Earthworm setup:
#
MyModuleId         MOD_REPORT  # module id for this instance of template 
RingName           HYPO_RING   # shared memory ring for input/output
LogFile            1           # 0 to turn off disk log file; 1 to turn it on
                               # to log to module log but not stderr/stdout
HeartBeatInterval  15          # seconds between heartbeats

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_WILDCARD  TYPE_HYP2000ARC TYPE_H71SUM2K



