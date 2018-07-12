                     
#
#                     Loc_wcatwc's Configuration File
#
MyModId        MOD_LOC_WCATWC  # This instance of Loc_wcatwc
StaFile        "pick_wcatwc.sta" # File containing stations picked and sent here
StaDataFile    "station.dat"   # File with information about stations
ResponseFile        "calibs"   # Broadband response file
InRing           PICK_RING     # Transport ring to find P-picks
OutRing          HYPO_RING     # Transport ring to write quake locations
AlarmRing       ALARM_RING     # Transport ring to write alarm messages to
HeartbeatInt            30     # Heartbeat interval, in seconds
#
#    Add file created in ATPlayer if used in conjunction with player (optional)
#    Comment this line if using loc_wcatwc in real-time mode
#ATPLineupFileBB "\DoNotCopyToEB\ATP-BB.sta"   # Station config file
RedoLineupFile           1     # For player - 1->reset lineup file (regional locator=0)
#
NumNearStn              16     # Number of closest stations to check (max in .h file)
NumPBuffs               20     # Number of P-time Repositories (max in .h file)
MaxTimeBetweenPicks      2.    # Maximum allowable time between P-picks placed in the same buffer (minutes)
MinPs                    5     # Minimum number of P-times used in location (5 min)
MaxDistance              5.    # Max distance (degrees) allowed for buffer sort
#
# BVals
#
BValFile        "Data/EarlyBird/Bvalues/bvals.dat"    # File containing Richter B-value data for mb.
                               #  Must have 2500 entries.
# The following boundaries define the area over which locations in this 
# routine are trusted.
SouthernLat            -90.    # The whole world  
NorthernLat             90.   
WesternLon            -180.   
EasternLon             180.   
# Define the minimum magnitude for which to email quake output in page_alarm
# (NOTE: Dependent on location and this value, some smaller quakes may be
#  issued)
MinMagToSend            6.5
# The following files are used to connect information from loc_wcatwc to
# the programs ANALYZE and LOCATE.
#
# Temp Stuff
#
OldQuakes  "oldquakeX.dat"                            # List of located quakes
AutoLoc    "autolocX.dat"  # Screen update trigger file
DummyFile  "dummyX.dat"                               # WC/ATWC EarlyBird dummy file
MapFile    "location.dat"   # WC/ATWC EarthVu Map Key File
RTPFile    "prtimeX.dat"    # WC/ATWC LOCATE real-time P File
QLogFile   "quake.log"                                # Ongoing log of all quakes located
MwFile     "mw.dat"             # Mws from mm
PFilePath  "LocFiles/"                                # P-data files from loc_wcatwc
ThetaFile  "Theta.dat"      # Theta output File

#
# Cities
#
DepthFile   "Data/EarlyBird/Bvalues/eqdepth.dat"      # File with avg & max depth data
CityFileWUC "Data/Cities/cities.dat"      # Pacific cities file
CityFileWLC "Data/Cities/citiesLC.dat"    # Pacific cities file-lc
CityFileEUC "Data/Cities/cities-ec.dat"   # Atlantic cities file
CityFileELC "Data/Cities/cities-eclc.dat" # Atlantic cities file-lc
NameFile    "Data/Cities/names.fer"        # FE Region name file
NameFileLC  "Data/Cities/nameslc.fer"      # FE Region name file-lower case
IndexFile   "Data/Cities/llindx.fer"       # FE Region index file
LatFile     "Data/Cities/lattiers.fer"     # FE Region lat file
# below are for windows only
FERegionPath  "Data/voice/feregion/"   # Path to FE Region names
DistancePath  "Data/voice/distance/"   # Path to Distances
DirectionPath "Data/voice/dir/"        # Path to Directions
CitiesPath    "Data/voice/city/"       # Path to Cities
#
Iasp91File    "Data/EarlyBird/Iasp91/iasp91"                      # iasp91 file
Iasp91TblFile "Data/EarlyBird/Iasp91/iasp91.tbl"                  # iasp91 file
#
Debug                    1     # If 1, print debugging message
