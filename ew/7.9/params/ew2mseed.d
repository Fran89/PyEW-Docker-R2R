# Sample configuration file for ew2mseed
# Version 0.1
# I.Dricker  (i.dricker@isti.com)   10/15/2000


#This defines the root directory where ew2mseed writes MiniSEED files
MseedDir /data/bud

# Verbosity
# 0 means don't create a disc log file.
# 2 => do not log to stderr
# 1 is both log and stderr

LogFile 2
Verbosity 4
LockFile ./ew2mseed_1.lock

# SCNLocSz - List SCNs, location IDs and logical MSEED record size 
#	 to save in MiniSEED format.
# Note that record size must be a power of 2!!!
 
SCNLocSz BOZ BHZ US NONE 4096 STEIM1
 

#
# OPTIONAL - specify StartTime. We are not interested in data before 
# this point. If this are not specified, ew2mseed will get all data 
# available. 
# 
# The format is mandatory and is yyyymnddhhmmss.00
# where
#	 yyyy [4 chars] - year
#	 mn [2 chars] - month
#	 dd [2 chars] - day of month
#	 hh [2 chars] - hour of day
#	 mm [2 chars] - minutes
#	 ss [2 chars] - secs
#	 00 [2 chars] - usecs
# THERE MUST be 17 symbols totally !
#
StartTime 20011119220000.00

# Setup for WaveServers we'll interrogate:
# list of ip addresses and ports of the  WaveServers we're to use
# frame relay..
WaveServer 192.168.12.10 16022

# harry...
#WaveServer 136.177.31.188 16022

# harry2...
# WaveServer 136.177.31.10 16022
  
# We'll wait at most this long for the WaveServers
# to get the time interval we want. Recall that we
# might be asking for trace data containing arrivals
# before the waves got to the sensors.
TravelTimeout  20 

SocketReconnect  3000

#Optional RecordsNumber : number of MSEED records we are extracting
# at once from the WaveServer.
# More records ew2mseed gets from the server, faster is the data retrieval
# is
# Keep in mind that there is a relation
# between the number of records, the size of a records and the allocated
# memory buffer, so  RecordsNumber * Record Size * 4 == memory buffer
# Make sure that you computer has enoguh memory to handle the request
# Default value for RecordsNumber is 1
RecordsNumber 3

#
#	End of configuration
