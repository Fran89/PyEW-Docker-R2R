#
# nq2pgm configuration file
#
# This code receives MiniSEED records as files, calculates peak ground 
# motions and spectral amplitudes.
# Results are sent to transport ring as strong motion messages and
# an XML file is written.
#
#
ModuleId        MOD_NQ2PGM       # module id for this import,
RingName        HYPO_RING        # shared memory ring for input and output

HeartBeatInt   10      # Heartbeat interval in seconds
                       # this should match the nq2pgm.desc heartbeat!

LogFile         1      # If 0, don't write logfile;; if 1, do
                       # if 2, log to module log but not stderr/stdout

# Directory where we should put the strong motion XML files
XMLDir    /home/luetgert/sendfiles/nqxmls/

# Directory where we should look for the NetQuakes miniSEED files
NQFilesInDir    /home/luetgert/getfiles/netquakes/nq2pgm

# StationList /home/earthworm/run/params/netquake.db1 # The file with all the station metadata.
StationList /home/earthworm/working-jim/Contrib/Menlo/src/netquakes/nq2pgm/netquake.db1 # The file with all the station metadata.
StationList /home/earthworm/run-jim/params/calsta.db1 # The file with all the station metadata.


# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
  Debug
