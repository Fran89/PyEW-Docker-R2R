#
# nq2ring configuration file
#
# This code receives MiniSEED records as files, converts them into
# Earthworm trace buf messages, and stuffs them into a wave ring.
#
#
ModuleId	  MOD_NQ2RING        # module id for this import,
InRingName      HYPO_RING        # shared memory ring for input
OutRingName     WAVE_RING        # shared memory ring for output

 HeartBeatInt   10      # Heartbeat interval in seconds
                        # this should match the nq2ring.desc heartbeat!

 LogFile         1      # If 0, don't write logfile;; if 1, do
                        # if 2, log to module log but not stderr/stdout

# Directory where we should look for the NetQuakes miniSEED files
NQFilesInDir    /home/luetgert/getfiles/netquakes

# Directory where the successfully processed files are put
NQFilesOutDir /home/luetgert/getfiles/netquakes/save

# Directory where the problem files are put
NQFilesErrorDir /home/luetgert/getfiles/netquakes/trouble

# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
# Debug

# Stating "Granulate" will cause all tracebufs to be written beginning at
# integer seconds. Although this will result in the loss of the fractional
# second beginning and end of the mSEED data, it is necessary if tracebufs are
# ever to be re-written to DB based waveservers like Winston.
Granulate

# Delay added to loop that throws tracebufs on the ring.
# Optional. Default = 200 (msec)
Delay  50
