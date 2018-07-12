#
# servertemplate Configuration file example
#



# ImA - The mode in which to operate.
#
#  Options:
#           StandAlone -- all actions within one process which ends whereafter
#           Client     -- client, calling server
#           Server     -- server, processing for client
#           Module     -- processing messages from ring
#
#  The configuration for each mode will be segregated below
#
ImA Client
      

# *******************************************************************
# *******************************************************************
# BASIC PARAMETERS NEEDED TO PERFORM IN THE CLIENT, SERVER,
# STANDALONE OR MODULE MODES.  SEGREGATED BY MODE.
#
# SEISMOLOGICALLY-RELATED PARAMETERS NOT IN THIS EXAMPLE.  HOWEVER,
# SEE THE COMMENT AT BOTTOM.
# *******************************************************************
# *******************************************************************


# ------------------------ Mode: ALL ------------------------

# Module logging level
# OPTIONAL
#
#     This level will over-ride any value from
#     earthworm.d
#
# values domain:
#
#     0 - No logging
#     1 - Errors only
#     2 - Status (informative messages, new event results) 
#     3 - Tracking (more messages)
#     4 - Details (many messages)
#     5 - Debug (all available messages)
#
# Default is 1
#
LogLevel  5


# DBConnection -- Database connection information
#
# DBConnection   UserName  Password  ServiceName
#
DBConnection   UserName  Password  ServiceName



# ------------------------ Mode: Client ------------------------

# Provider -- Description of server location
# MANDATORY
#
#  May be multiple entries
#
# Provider  IP_Address  Port
#
Provider 136.177.20.31 16075

# ConnectFailDelay -- milliseconds before determining failure to connect
#
ConnectFailDelay 5000

# SendTimeoutMSecs -- milliseconds to attempt send before timeout
#
SendTimeoutMSecs 3000

# RecvTimeoutMSecs -- milliseconds to attempt receive before timeout
# OPTIONAL
#
RecvTimeoutMSecs 3000



# ------------------------ Mode: Server ------------------------

# MyModuleId
# MANDATORY
#
# server's module id
#
# MyModuleId    MOD_MLSERVER

# CmdRingName
# OPTIOAN
#
#    Name of transport ring to check for shutdown flag
#    and to write heartbeats to
#
# CmdRingName   WAVE_RING

# HeartBeatInt
# OPTIONAL
#
#   seconds between heartbeats to CmdRingName
#   if excluded, no heartbeats sent.
#
# HeartBeatInt  15

# ServerAddr -- this server's IP address
#
# ServerAddr 

# ServerPort -- this server's port
#
# ServerPort 16075

# SendTimeoutMSecs -- milliseconds to attempt send before timeout
#
# SendTimeoutMSecs

# RecvTimeoutMSecs -- milliseconds to attempt receive before timeout
#
# RecvTimeoutMSecs

# MaxServerThreads -- maximum client service threads
#
# MaxServerThreads

# DBConnection -- Database connection string
#
# DBConnection 



# ------------------------ Mode: Module ------------------------

# MyModuleId
# MANDATORY
#
# server's module id
#
# MyModuleId    MOD_MLSERVER

# CmdRingName
# MANDATORY
#
#    Name of transport ring to check for shutdown flag
#    and to write heartbeats to
#
# CmdRingName   WAVE_RING

# HeartBeatInt
# OPTIONAL
#
#   seconds between heartbeats to CmdRingName
#   if excluded, no heartbeats sent.
#
# HeartBeatInt  15

# AcceptLogo -- describes the messages to be allowed
# MANDATORY
#
# AcceptLogo  INST_WILDCARD  MOD_WILDCARD  TYPE_TRIGNOTICE

# InputRing -- name of transport ring from which to receive
#              processing requests
#
# InputRing

# OutputRing -- name of transport ring to which to write
#               processing results
#
# OutputRing

# DBConnection -- Database connection string
#
# DBConnection 


# ------------------------ Mode: StandAlone ------------------------

# No specific values needed.



# *******************************************************************
# *******************************************************************
# SEISMOLOGICALLY-RELATED PARAMETERS.
#
# NOTE THAT THE CLIENT MODE ONLY REQUIRES PARAMETERS NEEDED TO
# PREPARE FOR THE REQUEST AND STORE THE RESULT, WHILE THE OTHER MODES
# MAY NEED DIRECTIVES SPECIFICALLY RELATED TO THE PROCESSING.
# *******************************************************************
# *******************************************************************



