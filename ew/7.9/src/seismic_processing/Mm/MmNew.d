#
#                     Mm's Configuration File
#
MyModId         MOD_MMNEW              # This instance of mm
InRing          WAVE_RING_LP           # Input waveform ring (input)
HypoRing        HYPO_RING              # Hypocenter ring (input)
StaFile         "pick_wcatwc_lp.sta"   # File containing stations to be processed
StaDataFile     "station.dat"          # File with information about stations
HeartbeatInt    15                     # Heartbeat interval, in seconds
#
#    Add file created in ATPlayer if used in conjunction with player (optional)
#    Comment this line if using mm in real-time mode
#ATPLineupFileLP "\DoNotCopyToEB\ATP-LP.sta"   # Station config file
#
MinutesInBuff          180     # Time (minutes) to allocate for each trace 
FileLengthLP            20     # Length of LP file in minutes (should be evenly
                               #  divisible into 60 (i.e., 1,2,3,4,5,6,10,...)
FileSuffix            ".L"     # Data file suffix start (see disk_wcatwc)
SigNoise                4.     # Signal:Noise to exceed for Mm computations
                               #  to proceed
AutoStart                1     # 1->Mm processing start automatically from 
                               # HYPO_RING messages; 0->manual start only
MagThreshForAuto       5.5     # Minimum magnitude to start Mm processing
NumStnForAuto           12     # Minimum number of stations in soln for Mm Auto
#
# Temp Stuff
#
DummyFile    "./dummyX.dat"                     # WC/ATWC EarlyBird dummy file
MwFile       "./mm/mw.dat"   # Mw results file
#
# Other Stuff
#
ResponseFile  "calibs"          # Broadband response file for Mm
RegionFile    "./Regions/Regions"     # Mm path correction file
QuakeFile     "./oldquakeX.dat"              # List of located quakes
DataDirectory "./datafile"   # Path where data are saved
ArchiveDir    "./event_archive"  # Path where archived data is saved
#
Debug                    1     # If 1, print debugging message
