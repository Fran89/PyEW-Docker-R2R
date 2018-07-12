#
#
#                     rayloc_ew Configuration File
#
MyModId        MOD_RAYLOC_EW   # This instance of rayloc_ew
StaFile        "/home/ew/model/glass_station_list.txt"   # File containing station name/pin# info
WorkDir        "/home/ew/model"         # Directory where model and tmp files are stored
InRing           HYPO_RING     # Transport ring to find TYPE_LOC_GLOBAL messages,
OutRing          PICK_RING     # Transport ring to write TYPE_RAYLOC messages
HeartbeatInt            30     # Heartbeat interval, in seconds,
Debug                    1     # If 1, print debugging message
#
#
# Parameters/flags used to control processing of rayloc 
# Default values are provided in comments and used if any
# of the parameters in to configuration is undefined
#
hold_params           0    # Hold latitude, longitude, depth, and origin time flag (def = 0)
fix_depth             1    # Fix depth (def = 1)
use_PKP               1    # Use PKP phases (def = 1)
use_depth_ph          1    # Use depth phases (def 1)
use_S_ph              1    # Use S phases (def = 1)
pick_weight_interval  0    # Pick weight residual interval flag (T/F) (def = 1)
Rmin                  0    # Lower bound of residual interval in which weights can be non-zero (def = 0)
Rmax                  0    # Upper bound of residual interval in which weights can be non-zero (def = 0)
#
D1                    0    # Pick weight distance interval flag(s) (T/F) (def = 0)
Dmin1                  0    # Lower bound of distance interval(s) in which weights are zero (def = 0)
Dmax1                180    # Upper bound of distance interval(s) in which weights are zero (def = 180)
#
D2                    0   # Pick weight distance interval flag(s) (T/F) (def = 0)
Dmin2                 0   # Lower bound of distance interval(s) in which weights are zero (def = 0)
Dmax2               180   # Upper bound of distance interval(s) in which weights are zero (def = 180)
#
D3                    0   # Pick weight distance interval flag(s) (T/F) (def = 0)
Dmin3                 0   # Lower bound of distance interval(s) in which weights are zero (def = 0)
Dmax3               180   # Upper bound of distance interval(s) in which weights are zero (def = 180)
#
D4                    0   # Pick weight distance interval flag(s) (T/F) (def = 0)
Dmin4                 0   # Lower bound of distance interval(s) in which weights are zero (def = 0)
Dmax4               180   # Upper bound of distance interval(s) in which weights are zero (def = 180)
#
D5                    0   # Pick weight distance interval flag(s) (T/F) (def = 0)
Dmin5                 0   # Lower bound of distance interval(s) in which weights are zero (def = 0)
Dmax5               180   # Upper bound of distance interval(s) in which weights are zero (def = 180)
#
