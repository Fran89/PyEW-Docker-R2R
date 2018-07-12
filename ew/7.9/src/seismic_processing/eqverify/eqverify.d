
#
# This is eqverify's parameter file
#
MyModuleId  MOD_EQPROC  # module id to label logfile  with.
                        # Note: eqverify is part of a mega-module which is
                        # ultimately started by the program eqproc.  All
                        # child processes of this mega-module need to use the
                        # same module id (thus use eqproc's module id).
LogFile       1         # 0=no log; 1=log errors
                        # 2=write to moduel log but not to stderr/stdout
# send output to the following command:
 PipeTo "log_everything"
# PipeTo "hypo_mgr hypo_mgr.d ncal.hyp"

# Required commands to turn glitch tests on (>0) or off (=0)
test_slopevsmag  1
test_freefitrms  1
test_codawt      1
test_pgroup      1

# Optional commands that change eqverify test defaults
# force_report  18 2.0    # report events with more than 18 picks
#                         # and mag>=2.0 regardless of event weight
# define_glitch  4 0.03   # a glitch is at least 4 picks within
                          # 0.03 seconds
# EventThreshold     3    # event weight at which event is "noise"
# MaxDeltaT         30.   # consider arrival times within this many
#                         # seconds of the 1st arriving pick
# coda_clip        820    # level at which a coda-avg-absolute-value
#                         # is clipped (see notes in eqcoda.command)
