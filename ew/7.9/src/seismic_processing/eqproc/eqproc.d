# This is Eqproc's Parameter File

# Basic Earthworm setup:
#-----------------------
 MyModuleId   MOD_EQPROC    # Module id for this instance of eqproc
 RingName     PICK_RING     # Ring to get input from
 HeartbeatInt 30            # seconds between heartbeats to statmgr
 LogFile      1             # 0 = turn off disk log file; 
                            # 1 = turn on disk log
                            # 2 = write disk log but not to stderr/stdout

# List the message logos to grab from transport ring
#               Installation       Module          Message Types
#-----------------------------------------------------------------
 GetPicksFrom   INST_WILDCARD    MOD_WILDCARD    # pick2k & coda2k
 GetAssocFrom   ${EW_INST_ID}    MOD_BINDER      # quake2k & link2k
  
# Send output to the following command (uncomment one):
#------------------------------------------------------
 PipeTo "eqbuf eqbuf.d"     # buffer events for downstream modules
#PipeTo "eqcoda eqcoda.d"   # do coda weighting & extrapolation
#PipeTo "log_everything"    # end chain here for debugging

# Load station list
#------------------
 maxsite     3500
 site_file   calsta.hinv

# Load crustal model
# Refer to file containing "lay" commands, or list them here
#-----------------------------------------------------------
#@ncal_model.d           # file containing "lay" commands
 lay   0.0     4.0
 lay   3.5     5.9
 lay   15.0    6.85
 lay   25.0    7.85

# Set pick/quake FIFO lengths (must be >= binder's fifo lengths)
#---------------------------------------------------------------
 pick_fifo_length  1000  # optional: default = 1000
 quake_fifo_length  100  # optional: default = 100

# Control how/when events are reported
#-------------------------------------
 ReportS      0     # 0 = do not send S-phases to next process
                    # non-zero = do send S-phases to next process
 rpt_dwell 60.0     # seconds to wait after last update before 
                    #   reporting event to downstream modules
                    #   (optional: default = 30)
 rpt_check  5.0     # interval (sec) at which to check all hypocenters   
                    #   to see if it's time to report an event
                    #   (optional: default = 0.3 * rpt_dwell)

# Control debugging info to log
#------------------------------
 rpt_grab   4.0     # tolerance (sec) for noting waif picks for
                    #   in log file. (optional: default = 4.0)
#print              # uncomment for extra debug logging
#graph              # uncomment for extra debug logging

