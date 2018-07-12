#
# q3302ew configuration file
#

## The following items are typical Earthworm items

 ModuleId       MOD_Q3302EW     # module id for this module.  Make sure to
				# add this to earthworm.h if it is not already
				# there

 RingName       WAVE_RING       # transport ring to use for input/output,

 HeartbeatInt   60              # Heartbeat interval in seconds
                  
 LogFile        1               # If 0, don't write logfile;; if 1, do
                                # if 2, log to module log but not stderr/stdout

## The following items tell us how to talk to the Q330

IPAddress		192.168.1.166		# The Q330 IP address
BasePort		5330			# The Q330 base port -
						#  5330 is the default base port
DataPort		1			# Which Q330 dataport to use
SerialNumber		0x010000069a412636	# The serial number of the Q330
AuthCode		0x0			# The Q330 auth code

## The following items may help traversing some firewalls

#SourcePortControl	9999	# UDP port to use as a source, when talking to
				# the Q330's control port

#SourcePortData		9998	# UDP port to use as a source when talking to
				# the Q330's data port


## The following items help control the log verbosity

LogLevel	sd, rm, vb, sm 	# Comma seperated list of: 
				# sd - Logs Q330 status on connect
				# cr - Logs command retries
				# rm - Pings and sends a user message
				#      to Q330 on connect/disconnect
				# vb - Logs messages for items like
				#      filter delays
				# sm - Logs 800 series messages
				# pd - Logs all packets sent/received

StatusInterval  240		# time in seconds between status updates


## The following items offer some control over connections

FailedRegistrationsBeforeSleep  5	# How many failed connection attempts
					# before we give it a break for a bit
MinutesToSleepBeforeRetry	3	# How long should that break be?


# optional setting
#RegistrationCyclesLimit	5 	# defaults to 5 if not set, q3302ew exits after 5 
					# failed registration attempts (restart by statmgr)

## Some options to control dutycycle
## comment out to disable


#Dutycycle_MaxConnectTime	10	# We'll disconnect after this many 
					# minutes of being connected
#Dutycycle_BufferLevel		10	# Disconnect when the Q330's buffer is
					# this percentage filled.	
#Dutycycle_SleepTime		30	# Wait this many minutes before 
					# connecting again, when we've stopped
					# for either of the above reasons

## Where should we keep our continuity files?
## These will be named: Q3302EW_cont_[dot_d_filename] and have '.bint' 
## and '.binq' extensions.  
ContinuityFileDirectory	/tmp
