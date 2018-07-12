
#
#                  Evanstrig's Configuration File
#
MyModuleId    MOD_EVANSTRIG # module id for this program,
InRing        WAVE_RING     # Transport ring to find waveform data on
OutRing       PICK_RING     # Transport ring to write all output to
LogFile       1             # 1 means write to module; 0 means do not
                            # 2 means write to module log but not to stderr/stdout

HeartbeatInt  30    # Heartbeat interval (seconds)
                    # if 0, no heartbeats will be sent.

Log_triggers 1      # If 1, log to screen only
                    # If 2, log to log file only
                    # If 3, log to screen and log file

Debug_enabled  0    # If 1, print debug messages

# List the message logos to grab from input transport ring
#              Installation       Module       Message Type
GetWavesFrom   INST_WILDCARD   MOD_WILDCARD   # TYPE_TRACEBUF2 msg

# Define the maximum gap (in # of sample intervals) between
# consecutive tracbuf messages to interpolate thru without a restart.  
# The number of samples that will be interpolated is MaxGap-1.
MaxGap 3
                          
#  Magic numbers from John Evans for configuring the trigger:
@evanstrig_params.d      # file with integer & floating-point parameters

# Optional command for changing the maximum number of channels 
# which can be triggered on.  Default is 32. 
# MaxSCNL 20

# List the SCNL's to trigger on:
#           site  comp net loc
TriggerOn    MPR  VHZ  NC  --
TriggerOn    MCM  VHZ  NC  --
TriggerOn    MCV  VHZ  NC  --
TriggerOn    MDC  VHZ  NC  --
TriggerOn    MTU  VHZ  NC  --
TriggerOn    SLK  VHZ  NN  --
TriggerOn    MMP  VHZ  NC  --
TriggerOn    MLM  VHZ  NC  --
TriggerOn    MGP  VHZ  NC  --
TriggerOn    MLK  VHZ  NC  --
TriggerOn    MMT  VHZ  NC  --
TriggerOn    MRS  VHZ  NN  --
TriggerOn    MDP  VHZ  NC  --
TriggerOn    MDR  VHZ  NC  --
TriggerOn    MEM  VHZ  NC  --
