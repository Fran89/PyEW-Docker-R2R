#
#                     Pick_ew's Configuration File
#
MyModId        MOD_PICK_EEW     # This instance of pick_ew
StaFile "C:\DAS\params\pick_eew.sta" # File containing station name/pin# info
InRing           WAVE_RING     # Transport ring to find waveform data on,
OutRing          PICK_RING     # Transport ring to write output to,
HeartbeatInt            60     # Heartbeat interval, in seconds,
RestartLength        30000     # Number of samples to process for restart
MaxGap                1000     # Maximum gap to interpolate
Debug                    0     # If 1, print debugging message