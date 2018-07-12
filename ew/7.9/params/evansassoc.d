
#
#           evansassoc's Configuration File
#
MyModuleId       MOD_EVANSASSOC    # Module id for this program,
InRing           PICK_RING         # Transport ring to find triggers on
OutRing          HYPO_RING         # Transport ring to write output to
HeartbeatInt     30                # Heartbeat interval (s); 0=no heartbeat
LogFile          1                 # 1=write log file on disk; 0=don't
                                   # 2=write to module log but not to stderr/stdout

# List the message logos to grab from transport ring
#                Installation       Module       Message Type
GetTriggersFrom  INST_WILDCARD   MOD_WILDCARD   # TYPE_LPTRIG_SCNL

# LP-trigger association parameters:
TriggerTimeLimit  17    # seconds; Set to (array aperture)/(8 or 10 km/s).
CriticalMu        10    # number of triggers within TriggerTimeLimit.
CriticalNu        10    # ..."... Set equal to CriticalMu.

# Parameters for saving seismograms:
MinEventTime     90     # total record length (s) of "normal" event
MaxEventTime    105     # total record length (s) of "big" event
PreEventTime     15     # pre-event portion (s) of event record 

EventIDfile  nextlp.id  # name of file containing next valid eventid
                        # this file should live in EW_PARAMS directory
            
# List any other channels (time code, non-seismic, etc) that should be 
# included in every event message. Wildcards (*) may be used in any field.
# List one SCNL per line, as many as you need:
#           sta chan net loc
AddChannel  MSL  VHZ  NC  01
AddChannel  MDR   *   NC  --

