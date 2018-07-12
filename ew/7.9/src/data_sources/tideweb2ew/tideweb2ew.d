#
# tideweb2ew configuration file
#
# This code parses a web page maintained by a tide guage, converts the data into
# Earthworm trace buf messages, and stuffs them into a wave ring.  
# It also produces daily reports of its readings
#
#
ModuleId	    MOD_TIDEWEB2EW   # module id for this import,
InRingName      HYPO_RING        # shared memory ring for input
OutRingName     WAVE_RING        # shared memory ring for output

HeartBeatInt   10      # Heartbeat interval in seconds
                       # this should match the tideweb2ew.desc heartbeat!

LogFile         1      # If 0, don't write logfile;; if 1, do
                       # if 2, log to module log but not stderr/stdout

# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
# Debug

# Stating "Granulate" will cause all tracebufs to be written beginning at
# integer seconds. Although this will result in the loss of the fractional
# second beginning and end of the mSEED data, it is necessary if tracebufs are
# ever to be re-written to DB based waveservers like Winston.
Granulate

#----

# IP address of tide guage webpage
GaugeIpAddr   136.145.29.158

# Net.Station associated with gauge
GaugeNetSta   NN.SSSSS

# Guage polling interval (seconds)
GaugePollSecs 30

# Path to location where daily log files will be writte; each file's path will
# be this value w/ YYYYMMDD.data appended to it
BaseFilePath   /path/to/client/module/output/files/fname_prefix

HeaderLine1     "Fajardo,_PR 9753216 PR Continuous 18.3336 -65.6311 20150503"
HeaderLine2     "NGWLS m UTC 1 min Water_Depth WCATWC Unfiltered"

# Optional: code used for ring packets & daily reports
# If not present, assumed to be PWL
PrimaryCode     PWL2

# Optional: code used for ring packets
# If not present, assumed to be BWL unless PrimaryCode was specified
SecondaryCode   BWL2

# PWL_SCALE    -1    # Optional multiplier for PWL values; default is 1
# BWL_SCALE    -1    # Optional multiplier for BWL values; default is 1
# PWL_CHAN    ABC    # Optional Channel code for PWL TRACEBUF2s; default is UTZ
# BWL_CHAN    ABC    # Optional Channel code for BWL TRACEBUF2s; default is UTZ
# PWL_LOC      99    # Optional Location code for PWL TRACEBUF2s; default is 00
# BWL_LOC      98    # Optional Location code for BWL TRACEBUF2s; default is 01
