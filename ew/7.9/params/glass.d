
# Directory section  - allows you to select which directory glass will
#  look in for particutlar files.
# DLLs   - directory where glass will look for the Module DLLs specified below
#  This should not be needed if you put the bin directory in your "PATH"
#$ DLLs j:\src\usgs\home\earthworm\v6.2\bin\
$ DLLs e:\ew\v7.0beta\bin\

# DATA   - directory where glass will look for DATA files
$ DATA glass_data\

# Module load section (load all modules before initializing parameters)
mod {DLLs}Network.dll
mod {DLLs}Flinn.dll
mod {DLLs}Earth.dll
mod {DLLs}Glint.dll
mod {DLLs}Glass.dll
mod {DLLs}Glock.dll
mod {DLLs}Solve.dll
mod {DLLs}ManQuake.dll
mod {DLLs}Grok.dll
mod {DLLs}Catalog.dll
mod {DLLs}Earthworm.dll
mod {DLLs}Summary.dll
mod {DLLs}Residuals.dll
mod {DLLs}Refocus.dll
#mod {DLLs}FocusGUI.dll
#mod {DLLs}FocusGUI_DP.dll
mod {DLLs}Publisher.dll


# enables nexus traffic monitor
DISPLAY

# Means: messages go to all modules
BROADCAST

# Processed by nexus.dll
Initialize

DISPATCH
# Turns on glass status window(glass.dll)
GlassMonitor

# associator param file (glass.dll)
GlassParams S:File={DATA}glass_assoc.d

# Hypo station list (network.dll)
HypoLoad S:File={DATA}glass.hinv

# Init file for earth module (tt code) (earth.dll)
TTInit S:File=tt_tables.d

# map base (grok.dll)
World S:Base={DATA}World.bmp

# glass pub params
PubAlgParams S:File={DATA}glass_pub_params.d

# Reporting params (glass.dll)
ReportInterval I:Interval=20

# Earthworm params (earthworm.dll)
RingIn S:Name=PICK_RING
RingOut S:Name=ASSOC_RING
HeartBeat I:Interval=30
ModuleId S:Module=MOD_GLASS_EW
LogFile

# NoFlushInput  - uncomment this line to prevent glass from flushing
#                 the earthworm input RING.  - not recommended 
                                               for normal operation
#NoFlushInput


# LOG_PICKS  -  Log picks to the glass_pick_log.txt file in EW_LOG dir.
LOG_PICKS

# LOG_ORIGINS - Log origins to the glass_origin_log.txt file in EW_LOG dir.
LOG_ORIGINS

# SHOW_PICKS -  Display new picks in the Map window (grok.dll)
SHOW_PICKS

# TERMINATE - uncomment this line to have glass terminate immediately after 
#             processing the config file.
# TERMINATE


# Flinn Engdahl Regions
FlinnEngdahlLoad S:Dir={DATA}

#  DebugLevel commands
#  CatalogDebugLevel | EarthwormDebugLevel | GlassDebugLevel | GlintDebugLevel | LocatorDebugLevel | PublisherDebugLevel 
#    Level:  0-9
#           MAJOR_ERROR 0
#           MAJOR_WARNING 1
#           MAJOR_INFO 2
#           MINOR_ERROR 3
#           MINOR_WARNING 4
#           MINOR_INFO 5
#           FUNCTION_TRACE 9
#     
#    OTF : OutputToFile 
#    OTD : OutputToDebugger 
#    OTE : OutputToError 
#    OTS : OutputTimeStamp
#    OSM : OutputStatusMessage
#
#  Format:
#  <MODULE>DebugLevel I:Level={0-9} I:OTF={0|1} I:OTD={0|1} I:OTE={0|1} I:OTS={0|1} I:OSM={0|1}
#
#  Default values:  {OTF,OTD,OTE,OTS,OSM}
#  {1,1,1,1,1},  // MAJOR ERROR
#  {1,1,1,1,1},  // MAJOR WARNING
#  {1,1,0,0,0},  // MAJOR INFO
#  {1,1,1,1,1},  // MINOR ERROR
#  {1,1,1,0,0},  // MINOR WARNING
#  {0,0,0,0,0},  // MINOR INFO
#  {0,0,0,0,0}   // FUNCTION TRACE

#
#  Example:
#  EarthwormDebugLevel  I:Level=2 I:OTF=1 I:OTD=0 I:OTE=0 I:OTS=0 I:OSM=0
EarthwormDebugLevel  I:Level=4 I:OTF=1 I:OTD=1 I:OTE=1 I:OTS=1 I:OSM=1
GlassDebugLevel  I:Level=5 I:OTF=1 I:OTD=1 I:OTE=1 I:OTS=1 I:OSM=1

