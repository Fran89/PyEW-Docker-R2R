
# This is template's parameter file

#  Basic Earthworm setup:
#
MyModuleId         MOD_EEW_SVR  # module id for this instance of template 
RingName           WAVE_RING   # shared memory ring for input/output
RingName2          TRIG_RING   # shared memory ring for input/output
LogFile            1           # 0 to turn off disk log file; 1 to turn it on
                               # to log to module log but not stderr/stdout
HeartBeatInterval  15          # seconds between heartbeats

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_WILDCARD    #

