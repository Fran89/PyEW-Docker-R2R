#
# This is eqprelim's parameter file

# Basic Earthworm setup:
#-----------------------
 MyModuleId   MOD_EQPRELIM  # module id for this instance of eqprelim
 RingName     PICK_RING     # ring to get input from
 HeartbeatInt 30            # seconds between heartbeats to statmgr
 LogFile      1             # 0 = turn off disk log file;
                            # 1 = turn on disk log
                            # 2 = write disk log but not to stderr/stdout

# List the message logos to grab from transport ring
#              Installation     Module          Message Types
#-------------------------------------------------------------------------------
 GetPicksFrom   INST_WILDCARD   MOD_WILDCARD    # TYPE_PICK_SCNL & TYPE_CODA_SCNL
 GetAssocFrom   INST_WILDCARD   MOD_BINDER_EW   # TYPE_QUAKE2K & TYPE_LINK

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
#@ncal_model.d            # file containing "lay" commands
 lay   0.0     4.0
 lay   3.5     5.9
 lay   15.0    6.85
 lay   25.0    7.85


# Control how/when events are reported
#-------------------------------------
 NumPickNotify  25        # send the event downstream when it has this
                          #   many picks associated with it
 ReportS        0         # 0 = do NOT send S-phases to next process
                          # non-zero = send S-phases to next process
 ReportCoda     0         # 0 = do NOT send codas to the next process
                          # non-zero = send codas to the next process
 LocalCode      W         # single char to identify local picks with

# THE END
