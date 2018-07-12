#
# nq2gif configuration file
#
# This code receives MiniSEED records as files, reads them and
# plots them as GIF images suitable for quick review.
#
#
ModuleId       MOD_NQ2GIF        # module id for this import,
InRingName     HYPO_RING         # shared memory ring for input

HeartBeatInt   10      # Heartbeat interval in seconds
                       # this should match the nq2ring.desc heartbeat!

LogFile         1      # If 0, don't write logfile;; if 1, do
                       # if 2, log to module log but not stderr/stdout

# Directory where we should look for the NetQuakes miniSEED files
NQFilesInDir    /home/luetgert/getfiles/netquakes/nq2gif

# Directory where the successfully processed files are put
NQFilesOutDir /home/luetgert/getfiles/netquakes/nq2gif/save

# Directory where the problem files are put
NQFilesErrorDir /home/luetgert/getfiles/netquakes/nq2gif/trouble

GifDir       /home/luetgert/gifs/netquakes/         # Directory for temporary .gif files.
LocalTarget  /home/luetgert/sendfiles/netquakes/    # Directory for .gif files for output

StationList /home/earthworm/run/params/netquake.db1 # The file with all the station metadata.

# Plot Display Parameters - 
		# The following is designed such that each SCN creates it's own
		# helicorder display; one per panel of data.
		
# 01 SecsPerPlot     Total number of seconds per gif image
# 02 XSize           Overall size of plot in inches Setting these > 100 will imply pixels
# 03 YSize           Ysize/channel.
#                                      
#         01    02   03   04  05 06 07 

 Display  30  1200  200 

# Filter Parameters - [[[ NOT YET IMPLEMENTED ]]]
#
# 01 fLow     Lower filter corner (Hz)
# 02 fHigh    Upper filter corner (Hz)
# 03 fType    Filter type 0 - No Filter
#                         1 - Low Pass ( Uses fHigh)
#                         2 - High Pass ( Uses fLow)
#                         3 - Band Pass ( Uses fLow & fHigh)
#                                      
#         01    02   03   04  05 06 07 

Filter    0.01  20    3 

Logo    smusgs.gif   # Name of logo in GifDir to be plotted on each image
UseDST

# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
  Debug
