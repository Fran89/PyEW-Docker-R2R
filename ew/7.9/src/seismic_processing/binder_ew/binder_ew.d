# This is binder's parameter file!
#---------------------------------
 MyModuleId   MOD_BINDER_EW # Module id for this instance of binder

 RingName     PICK_RING     # Public transport ring for I/O

 BufferRing   BINDER_RING   # Private ring for buffering incoming picks.
                            #  This ring's name must be defined in earthworm.d,
                            #  but must not be listed in startstop's config file
                            #  (default=BINDER_RING).

 BufferRingKB 256           # Size of private ring in kilobytes (default=256).

 HeartbeatInt 30            # Seconds between heartbeats

 LogFile      1             # 0 = turn off logging
                            # 1 = log to disk and stderr/stdout 
                            # 2 = log to disk but not to stderr/stdout

# List the message logos to grab from transport ring
#              Installation       Module      # Message Type
#-------------------------------------------------------------
 GetPicksFrom  INST_WILDCARD    MOD_WILDCARD  # TYPE_PICK_SCNL

# Set level of output for writing log file
#-----------------------------------------
#log_stack            # comment out to turn of grid-stack logging
 hypcode    4         # Sets a value to specify how much information should be 
                      # included in binder's log file after each event 
                      # relocation. Possible values 0-7.
                        	
# Load station list
#------------------
 maxsite 1800
 site_file "calsta.hinv"

# Load crustal model 
# Refer to file containing "lay" commands, or list them here
#-----------------------------------------------------------
#@ncal_model.d        # contents of ncal_model.d follow:
 lay   0.0     4.0
 lay   3.5     5.9
 lay   15.0    6.85
 lay   25.0    7.85

# Set FIFO lengths
#-----------------
 pick_fifo_length  1000   # optional: default=1000
 quake_fifo_length  100   # optional: default=100

# Define association grid, set stacking parameters.
#--------------------------------------------------
 dspace     4.0
 grdlat    32.5    43.0
 grdlon  -125.0  -114.5
 grdz       0.0    20.0
 rstack   100.0
 tstack     0.65          # truncates to nearest 10th sec
 stack     50
 thresh    18
 focus     30
 grid_wt 0  4
 grid_wt 1  3
 grid_wt 2  2
 grid_wt 3  1

# optional setting to ignore the location code in same station decision making, otherwise the Station/Network/Location codes are used to
# decide if a pick on a given channel is at the same station as another channel at that station
#
# ignore_loc_code_in_same_station_decision 1     # set to 1 to turn on, or 0 to turn off, if not set, it is off by default 
                                                 # yes, its a horribly long name, but so be it...you know what it means!!

# new optional feature to prevent event collision during stacking (nucleation) of events:
#nearest_quake_dist 10.0 2.0	# Optional, kill any events that are created within 10km and 2sec origin from existing events
				# this feature allows tuning of stacking to prevent multiple events happening 
				# that can compete for waif picks

# stack_horizontals       	# Optional: allow horizontal components to play in role in stacking 
				# (a VERY bad idea!, but needed for limited networks).

# Set parameters for associating picks with active hypocenters
#-------------------------------------------------------------


#no_S_on_Z 		# Optional setting to not associate S phases on Z components
			# off by default

#no_P_on_Horiz		# Optional setting to not assocaite P phases on Horizontal components
			# off by default

#s_to_p_amp_ratio 2.0	# Optional: only associate S phase if earlier P phase on same SNL and S phase amplitude is greater than N x P amp
			# this is a new experimental option

#ChannelNumberMap NEZ   # means map the number to the chan code 1=N, 2=E, 3=Z 
			# this is for horizontal component identification

#ChannelNumberMapByNet NEZ CI  # means map the number to the chan code 1=N, 2=E, 3=Z  for the CI network code
			# you may have as many of these as you wish

# grid_debug 1		# turn on some verbosity related to stacking procedure and which chans get deselected
# bind_debug 1		# turn on some verbosity related to binding procedure and which chans get deselected

# new optional feature, ChannelPriority ranks band and sensor code chans from same site (which pick to use)
# channel code followed by priority, higher priority is better
# ChannelPriority  HH  30
# ChannelPriority  HN  20
# ChannelPriority  BH  10

 rAvg_Factor 7.0
 taper   0.0     1.0
 taper   50.0    1.0
 taper   200.0   2.5
 taper   400.0   4.5
# bfh: 11/5/94: "variable taper" proportional to Origin Time Uncertainty:
# Set both to 0.0 to get Carl's original distance taper only.
 taper_OT 2.0    1.0
 t_dif   -3.0  120.


# Set parameters for locating events
#-----------------------------------
 wt 0 1.0
 wt 1 0.5
 wt 2 0.25
 wt 3 0.125
 ph P  1.0
 ph Pn 0.5
 ph Pg 0.5
 r_max  400.0
 zrange 1.0 30.0
 MaxStep 25.0  5.0
 MinXYZstep 0.1
 MaxIter 4
 MaxDeltaRms 1.0001
 locate_eq  25   2
 locate_eq  50   4
 locate_eq  75   8
 locate_eq 100  16

# Set parameters for assessing groups of picks
# with Bill Ellsworth's resampling technique:
#----------------------------------------------
 assess_pk  8  12
# If you are not using Bill Ellsworth's technique, 
# you may set to 0 0 to disable
# assess_pk  0 0
 maxtrial   500
 maxwt      3
 v_halfspace  5.0
 residual_cut  5.0  10.0
#log_accepted

# Load in the next valid quake sequence number
#---------------------------------------------
 EventIdFile quake_id.d

# THE END
