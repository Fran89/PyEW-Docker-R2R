
# Configuration file for eqfilter
#
# pick up arc messages (TYPE_HYP2000ARC), if at least 1 authoritative
# regiona is defined, extract the lat and lon of the location and the
# installation ID of the author of the  message. Then, using the
# authoritative region polygons, determine whether the location is
# within this installation's authoritative region. If it is not, then
# do not write the message to the OutRing, otherwise the message will
# not get written. Exclude regions are also permitted.
#
# Next, perform all the other optional tests.  If any configured tests
#   fail, do not write message to OutRing.
#
# Except for the authreg test, WildCards are permitted.  If an event
#   logo does not have an instid contained in the list of a particular
#   parameter, then the list is checked for a WildCard.  If there is
#   a WildCard entry, the test is performed.  If WildCard is not
#   configured and the logo instid is not on the list, the message
#   will NOT be written to OutRing (but will be logged if Debut is set).
#
# All tests (including authreg) are optional.  Lack of any configured
#   tests is not tested for (we assume a reasonable level of skill).
#
#


LogFile 1     # 0 means don't create a disc log file. 1=> do.
              # 2 means write to module log but not to stderr/stdout
MyModuleId MOD_EQFILTER
InRing     HYPO_RING
OutRing    HYPO_RING_CLEAN
HeartBeatInt 30

#
# Optional Debug switch - if this is uncommented
# lots of debug messages will be produced 
#
Debug

# What message logos to listen to. Can be more than one, but no more
# than five.
# The type is hard coded to TYPE_HYP2000ARC

GetEventsFrom INST_WILDCARD MOD_WILDCARD

#
# AllowUndefInst - OPTIONAL
#  Applies only to authreg test.
#  If this is uncommented, the TYPE_HYP2000ARC messages with installation
#  ID not listed in any InclRegion command below will be written to the 
#  OutRing.  To prevent writing of messages with "unknown" installation
#  IDs, make sure that this options is NOT commented out.
#
AllowUndefInst

#
# List authoritative regions: at least one InclRegion keyword is required.
#  It associates an earthworm installation ID with a polygon (max 20 sides)
#  inside of which that installation is authoritative.
#    o An installation can have more than one authoritative region.
#    o A region can belong to more than one installation.
#
# The points of a polygon should be listed as if lines of the polygon
# were being drawn without lifting up the pencil, until the polygon
# is closed. Therefore, the first and the last point should be
# the same, and each two the points in between should be the ends
# of one side of the polygon.
#
# ExclRegion is optional. It allows for "holes" to be cut inside of the 
# authoritative regions defined by InclRegion option.
#



#
# Region polygons below are from CNSS:
#   http://quake.geo.berkeley.edu/anss/anss-detail.html#polygons 
#

#
#Keyword      InstID     NumSides    Lat, Lon  Lat, Lon  ...
#

#
# Alaska Regional Network - AK
#

#
# AVO Volcanos
#
InclRegion   INST_AVO       5    62.50 -148.00   60.00 -150.00   57.50 -157.00   58.75 -157.00   62.50 -152.50   62.50 -148.00
InclRegion   INST_AVO       4    51.25 -175.50   52.50 -175.50   52.50 -178.00   51.25 -178.00   51.25 -175.50


#
# Southern California Seismic Network - CI
#

#
# HVO Network - HV
#
InclRegion   INST_HVO       8    18.83 -155.00   19.50 -154.75   20.00 -154.92   20.25 -155.25   20.33 -155.92   20.00 -156.50   19.17 -156.17   18.75 -155.67   18.83 -155.00

#
# Montana Seismic Network - MB
#
InclRegion   INST_BUTTE     8    45.17 -110.00   45.17 -111.33   44.50 -111.33   44.50 -113.00   47.95 -116.00   48.50 -115.00   48.50 -113.00   47.00 -110.00   45.17 -110.00

#
# Northern California Seismic Networks - NC
#
InclRegion    INST_MENLO     9   36.68 -117.79   37.75 -118.25   37.75 -119.50   39.50 -120.75   42.00 -121.41   42.00 -122.70   43.02 -125.00   40.00 -125.50   34.69 -121.37   36.68 -117.79

#
# New Madrid Seismic Network - NM
#
InclRegion    INST_MEMPHIS   8   37.63  -86.68   36.63  -88.62   34.03  -88.42   34.00  -93.50   38.00  -93.50   40.00  -91.50   40.00  -85.00   38.80  -85.00   37.63  -86.68

#
# Nevada Seismic Network - NN
#
InclRegion    INST_UNR      11   35.30 -114.60   35.80 -116.40   36.68 -117.79   37.75 -118.25   37.75 -119.50   39.50 -120.75   40.50 -120.50   41.50 -119.20   41.30 -118.30   40.00 -116.00   37.50 -114.30   35.30 -114.60

#
# SouthEast US Seismic Network - SE
#

#
# Puerto Rico Seismic Network - PR
#

#
# Utah Seismograph Network - UU
#
InclRegion    INST_UTAH      4   36.75 -108.75   36.75 -114.25   42.50 -114.25   42.50 -108.75   36.75 -108.75

#
# Pacific Northwest Seismic Network - UW
#
InclRegion    INST_UW       11   43.02 -125.00   42.00 -122.70   42.00 -121.00   45.90 -117.80   48.20 -117.80   48.95 -122.00   48.95 -122.80   48.20 -123.00   48.50 -125.00   44.50 -124.60   43.00 -125.00   43.02 -125.00

#
# Yellowstone WY Seismic Network - WY
#
InclRegion    INST_UTAH      4   44.00 -109.75   44.00 -111.33   45.17 -111.33   45.17 -109.75   44.00 -109.75

# for all subsequent tests INST_WILDCARD is legal but will be superceded
# by additional Keyword entries with non-wildcard InstID.
 
#Depth test
# is hypo between MinDepth and MaxDepth km
#Keyword    InstID         MinDepth  MaxDepth
#DepthTest   INST_WILDCARD   0.0       25.0
 
#number phase (high weight) test
# are there at least NPhase phases with phase weight greater than 0.1
#Keyword    InstID         NPhase
#nphTest  INST_WILDCARD   5

#total number phase (any weight) test
# are there at least NPhase phases with phase weight greater than 0.0
#Keyword    InstID         NPhaseTotal
nphtotalTest  INST_WILDCARD   5
 
#gap test
# is the gap < MaxGap  degrees
#Keyword    InstID         MaxGap
#GapTest     INST_MEMPHIS   270.0
 
#dmin test
# is distance to nearest station at no greater than MaxDmin km
#Keyword    InstID         MaxDmin
#DminTest    INST_MEMPHIS   50.0
 
#rms test
# is the rms less than MaxRMS seconds
#Keyword    InstID         MaxRMS
#RMSTest     INST_MEMPHIS   2.5
 
#e0 test
# is the largest principal error less than MaxE0 km
#Keyword    InstID         MaxE0
#MaxE0Test      INST_MEMPHIS   50.0
 
#erh test
# is the horizontal error less than MaxERH km
#Keyword    InstID         MaxERH
#MaxERHTest     INST_MEMPHIS   50.0
 
#erz test
# is the vertical error less than MaxERZ km
#Keyword    InstID         MaxERZ
#MaxERZTest     INST_MEMPHIS   25.0
 
#Mag test
# is the magnitude greater than MinMag
#Keyword    InstID         MinMag
MinMagTest     INST_MEMPHIS   -9.9
MinMagTest     INST_AVO        3.0
MinMagTest     INST_HVO        3.0
MinMagTest     INST_BUTTE      0.0
MinMagTest     INST_MENLO      2.5
MinMagTest     INST_UNR        2.5
MinMagTest     INST_UTAH       2.5
MinMagTest     INST_UW         2.5
MinMagTest     INST_WILDCARD   4.0

# a crude teleseism filter (3 args to pass in)
#             min num first motions to run test, min gap to run test, max percent same polarity 
# if all tests pass, that more than some percent all have the same first motion, the event fails
#  probably a teleseism OR a blast
#PolarityTest    7    180   70.0
 
#Ncoda test
# given an event with Magnitude Mag, are there at least MinC coda picks
#
# note here that since a test is defined for INST_MEMPHIS, unless there
#   is a wildcard test also defined, only INST_MEMPHIS events have a
#   chance of passing.  This is so because once a test is configured,
#   only configured instid's will pass the test.
#Keyword    InstID         MinC  Mag
NcodaTest   INST_MEMPHIS   4     2.0
NcodaTest   INST_MEMPHIS   9     3.0
NcodaTest   INST_WILDCARD  6     2.0
