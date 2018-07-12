
# Configuration file for eqfilterII
#
# son of eqfilter
# now reads the following, mutually exclusive message types
#  TYPE_HYP2000ARC
#  TYPE_LOC_GLOBAL
#  TYPE_RAYLOC
#
# pick up messages of specified type, if at least 1 authoritative
# region is defined, extract the lat and lon of the location and the
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
# Note not all tests defined for each message type


LogFile 1     # 0 means don't create a disc log file. 1=> do.
              # 2 means write to module log but not to stderr/stdout
MyModuleId MOD_EQFILTERII
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
# The type is one of TYPE_LOC_GLOBAL, TYPE_HYP2000ARC, or TYPE_RAYLOC

GetEventsFrom INST_WILDCARD MOD_WILDCARD TYPE_LOC_GLOBAL

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

@regions.d

# for all subsequent tests INST_WILDCARD is legal but will be superceded
# by additional Keyword entries with non-wildcard InstID.

#Depth test
# is hypo between MinDepth and MaxDepth km
#Keyword    InstID         MinDepth  MaxDepth
#DepthTest   INST_WILDCARD   0.0       25.0

#number stations test
# are there at least NSta stations used 
#  not used in HYP2000 or global_loc
#Keyword    InstID         NPhase
#nstaTest    INST_WILDCARD   5
 
#number phase (high weight) test
# are there at least NPhase phases with phase weight greater than 0.1
#  HYP2000 only
#Keyword    InstID         NPhase
#nphTest  INST_WILDCARD   5

#total number phase (any weight) test
# are there at least NPhase phases with phase weight greater than 0.0
#  number associated in global_loc, number used in rayloc.
#Keyword    InstID         NPhaseTotal
nphtotalTest  INST_WILDCARD   5

#gap test
# is the gap < MaxGap  degrees
#Keyword    InstID         MaxGap
#GapTest     INST_MEMPHIS   270.0
 
#dmin test
# is distance to nearest station at no greater than MaxGDmin in degrees
# Global Loc and Rayloc only
#Keyword    InstID         MaxGDmin
#GDminTest    INST_MEMPHIS   50.0

#dmin test
# is distance to nearest station at no greater than MaxDmin in km
# HYP2000 only
#Keyword    InstID         MaxDmin
#DminTest    INST_MEMPHIS   50.0
 
#rms test
# is the rms less than MaxRMS seconds
#Keyword    InstID         MaxRMS
#RMSTest     INST_MEMPHIS   2.5
 
#e0 test
# is the largest principal error less than MaxE0 km
#Keyword    InstID         MaxE0
#  not used for global_loc
#MaxE0Test      INST_MEMPHIS   50.0
 
#erh test
# is the horizontal error less than MaxERH km
#Keyword    InstID         MaxERH
#  not used for global_loc
#MaxERHTest     INST_MEMPHIS   50.0
 
#erz test
# is the vertical error less than MaxERZ km
#  not used for global_loc
#Keyword    InstID         MaxERZ
#MaxERZTest     INST_MEMPHIS   25.0

#avh test
# is the equivalent radious of the horizontal error ellipse less than MaxAVH km
#  rayloc only
#Keyword    InstID         MaxAVH
#MaxAVHTest     INST_MEMPHIS   25.0
 
#Mag test
# is the magnitude greater than MinMag
# HYP2000 only
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

#Ncoda test
# given an event with Magnitude Mag, are there at least MinC coda picks
# HYP2000 only
# note here that since a test is defined for INST_MEMPHIS, unless there
#   is a wildcard test also defined, only INST_MEMPHIS events have a
#   chance of passing.  This is so because once a test is configured,
#   only configured instid's will pass the test.
#Keyword    InstID         MinC  Mag
NcodaTest   INST_MEMPHIS   4     2.0
NcodaTest   INST_MEMPHIS   9     3.0
NcodaTest   INST_WILDCARD  6     2.0

