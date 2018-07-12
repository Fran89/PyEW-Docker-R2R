#
#                     Ms's Configuration File
#
MyModId             MOD_MS     # This instance of develo
InRing        WAVE_RING_LP     # Input waveform ring (input)
PRing            PICK_RING     # P-pick ring (output)
HypoRing         HYPO_RING     # Hypocenter ring (input)
StaFile        "pick_wcatwc_lp.sta" # File containing stations to be processed
StaDataFile    "station.dat"   # File with information about stations
ResponseFile        "calibs"   # Broadband response file
HeartbeatInt            15     # Heartbeat interval, in seconds
DataDirectory "./datafile"      # Path where data are saved
ArchiveDir    "./event_archive"
#    Add file created in ATPlayer if used in conjunction with player (optional)
#    Comment this line if using Ms in real-time mode
#ATPLineupFileLP "\DoNotCopyToEB\ATP-LP.sta"   # Station config file
#
#    Ms can bandpass filter data prior to display. 
#    The following values are used as filter coefficients.
#    The default values (28S, 14S) relate to the
#    response coefficients given within magcomp.  If other values are
#    used, the filter response for those values must be computed and used 
#    instead of the standard response.
#
LowCutFilter     0.0357143     # In hz, low frequency bandpass filter cut (28s)
HighCutFilter    0.0714286     # In hz, high frequency bandpass filter cut (14s)
#
MinutesInBuff          180     # Time (minutes) to allocate for each trace 
                               #  (MinutesInBuff should be at least 90 minutes
                               #  to ensure real-time processing does not have
                               #  to get data off disk)
FileLengthLP            20     # Length of LP file in minutes (should be evenly
                               #  divisible into 60 (i.e., 1,2,3,4,5,6,10,...)
FileSuffix            ".L"     # Data file suffix start (see disk_wcatwc)
SigNoise                6.     # Signal:Noise to exceed for MS computations
                               #  to proceed
AutoStart                1     # 1->Ms processing start automatically from 
                               # HYPO_RING messages; 0->manual start only
MagThreshForAuto       5.5     # Minimum magnitude to start Ms processing
NumStnForAuto           12     # Minimum number of stations in soln for Ms Auto
#
# Temp Stuff
#
DummyFile  "./dummyX.dat"                               # WC/ATWC EarlyBird dummy file
LPRTFile   "./earthworm\atwc\src\locate\ms.dat"         # Ms data File
QuakeFile  "./oldquakeX.dat"                            # List of located quakes
#
Debug                    1     # If 1, print debugging message
