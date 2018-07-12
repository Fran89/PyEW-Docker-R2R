# ewhttpd configuration file


# Earthworm stuff
 MyModuleId     MOD_EWHTTPD          # Module ID
 HeartbeatInt   30                   # Heartbeat interval in seconds
 LogFile        1                    # If 0. don't write logfile; if 1. do
                                     # if 2. log but not stderr/stdout
 Debug          0



###############################
# Settings for the Web Server #
###############################

# List of server options
# See http://code.google.com/p/mongoose/wiki/MongooseManual for details
#WebServerOpt	Option Name	Option Value
 WebServerOpt	listening_ports	8080	# IP and port to use
 WebServerOpt	document_root	"web"			# Website root folder
 WebServerOpt	error_log_file	web/webserver.log	# Log file for errors
 WebServerOpt   enable_directory_listing no		# Allow listing files
 

# List of users. If there are no users, authentication is disabled
User		Username	Password
User		ew		ew
 
 

####################################
# Settings for the SQLite DataBase #
####################################

 DataBaseFile	ewhttpd.sqlite	# File used to store messages
 MsgLifeTime    30		# Time before a msg is deleted, in days
 
 
##############################################
# Settings for collecting data from EW rings #
##############################################

# Input rings. Maximum 10. The first ring will be used for heartbeats
 InRing         PICK_RING
 InRing         HYPO_RING
 
# Message logos to retrieve and store in the database
 GetLogo	INST_WILDCARD	MOD_WILDCARD	TYPE_HYP2000ARC
 GetLogo	INST_WILDCARD	MOD_WILDCARD	TYPE_PICK_SCNL
 GetLogo	INST_WILDCARD	MOD_WILDCARD	TYPE_CARLSTATRIG_SCNL
 GetLogo	INST_WILDCARD	MOD_WILDCARD	TYPE_TRIGLIST_SCNL
 

 
 
 
                         
                         
