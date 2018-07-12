#
#                     Theta's Configuration File
#
MyModId          MOD_THETA          # This instance of Theta
InRing           WAVE_RING          # Input waveform ring (input)
HypoRing         HYPO_RING          # Hypocenter ring (input)
HeartbeatInt            20          # Heartbeat interval, in seconds
Debug                    1          # If 1, print debugging message
#
StaFile        "pick_wcatwc.sta"    # File containing stations to be processed
StaDataFile    "station.dat"        # File with information about stations
ResponseFile   "calibs"             # Broadband response file for Theta
#
#    Add file created in ATPlayer if used in conjunction with player (optional)
#    Comment this line if using in real-time mode
#ATPLineupFileBB "\DoNotCopyToEB\ATP-BB.sta" # Station config file
#
MagThreshForAuto       5.5          # Events will be processed if average M greater than this
NumStnForAuto           12          # Minimum number of stations in soln for Theta Auto
AutoStart                1          # 1->Theta processing start automatically from 
                                    #  HYPO_RING messages; 0->manual start only
MinDelta               30.          # Station will be considered if delta greater than this
MaxDelta               90.          # Station will be considered if delta lass than this
WindowLength            70          # Number of seconds following P used for energy calculation
FiltLo                 0.1          # Low cut filter for energy calculation
FiltHi                 2.0          # High cut filter for energy calculation
#
MinutesInBuff           20          # Time (minutes) to allocate for each trace 
FileSize                 2          # Number of minutes per data file (see disk_wcatwc)
FileSuffix             ".S"         # Data file suffix start (see disk_wcatwc)
DataDirectory   "\datafile"         # Root directory for disk file reads
ArchiveDir      "\\192.168.6.39\EventArchives"  # Path where archived data is saved
AlarmFile   "\DoNotCopyToEB\PageAlarm.txt" # File to send pager messages
#
# Temp Stuff
#
DiskPFile    "\DoNotCopyToEB\earthworm\atwc\src\locate\PTimeLoc.dat"  # P-time file from LOCATE
DummyFile    "\DoNotCopyToEB\dummyX.dat"                              # WC/ATWC EarlyBird dummy file
ThetaFile    "\DoNotCopyToEB\earthworm\atwc\src\locate\Theta.dat"     # Theta output File
LocFilePath  "\DoNotCopyToEB\LocFiles\"
QuakeFile    "\DoNotCopyToEB\oldquakeX.dat"                           # List of located quakes
