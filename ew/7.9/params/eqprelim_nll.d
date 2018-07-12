
#
# This is eqprelim_nll's parameter file

#  Basic Earthworm setup:
#
MyModuleId   MOD_EQPRELIM_NLL  # module id for this instance of eqprelim
RingName     PICK_RING     # ring to get input from
LogFile      1             # 0 to turn off disk log file; 1 to turn on disk log
                           # 2 to write to module log but not to stderr/stdout
HeartbeatInt 30            # Seconds between heartbeats

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetPicksFrom   INST_WILDCARD    MOD_WILDCARD    # pick2k  & coda2k
GetAssocFrom   ${EW_INST_ID}     MOD_BINDER_EW   # quake2k & link

LocalCode      W         # single char to identify local picks with
ReportS        0         # if 0, do not send S-phases to next process
NumPickNotify  25        # send preliminary event when it has this
                         # many picks associated with it

# Send output to the following command:
PipeTo "eqbuf eqbuf_nll.d"
# PipeTo "eqcoda eqcoda_nll.d"
# PipeTo "log_everything"

# Load station list
#
maxsite 1800
site_file "calsta.hinv"

# Load Central California crustal model
#
@ncal_model.d
