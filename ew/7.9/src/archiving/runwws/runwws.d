# This is runwws's Parameter File

# Basic Earthworm setup
# ---------------------
 MyModuleId   MOD_RUN_WWS   # Module id for this instance of runwws
 RingName     HYPO_RING     # Ring for heartbeats
 HeartbeatInt 30            # Seconds between heartbeats to statmgr
 LogFile      1             # 0 = turn off disk log file;
                            # 1 = turn on disk log
                            # 2 = write disk log but not to stderr/stdout

# PipeTo: Send output to a child program started with the following command.
# Do not use the --noinput option, because it causes the child process to
# ignore requests to shut down politely.
#
# To use this PipeTo command with the Winston included with Earthworm:
# Copy src\archiving\Winston1.1\lib to %EW_PARAMS%\wws-lib 
#    (note 'lib' is renamed 'wws-lib')
# Copy src\archiving\Winston1.1\WWS.config to %EW_PARAMS%\wws.d
# Copy src\archiving\Winston1.1\ImportEW.config to %EW_PARAMS%\imp-wws.d
# Copy src\archiving\Winston1.1\Winston.config to %EW_PARAMS%\winston.d
# edit wws.d and imp-wws.d and change references of Winston.config to winston.d
# 
# Go ahead and configure MySQL and Winston as appropriate. These pages may be
# helpful with that, though keep in mind the new names we have for WWS.config, 
# ImportEW.config and Winston.config:
# http://www.avo.alaska.edu/Software/winston/W_Manual_SR.html 
# http://www.avo.alaska.edu/Software/winston/W_Manual_TUT.html 
# http://www.avo.alaska.edu/Software/winston/W_Manual_CF.html
# 
# If for some reason you want to keep the .config names instead you can remove the 
# .d filenames from the end of the PipeTo command.
# -------------------------------------------------------------------------
# PipeTo "java -Xrs -cp wws-lib/winston.jar gov.usgs.winston.in.ew.ImportEW imp-wws.d"
PipeTo "java -cp wws-lib/winston.jar gov.usgs.winston.server.WWS wws.d"


# ChildWaitTime: After asking the WWS java process to exit, runwws waits
# ChildWaitTime milliseconds for the child process to die.  If doesn't
# die in this amount of time, runwws forcibly terminates the child process.
#
# Note that this expects the java program we're asking to quit to quit
# gracefully given the 'q' command.
# ------------------------------------------------------------------------
ChildWaitTime 5000
