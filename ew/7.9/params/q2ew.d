#
# q2ew configuration file
#
# This code receives MiniSEED records from COMSERV, converts them into
# Earthworm trace buf messages, and stuffs them into a wave ring.
#
#
 ModuleId	MOD_Q2EW	# module id for this import,
 RingName	WAVE_RING	# transport ring to use for input/output,

 HeartbeatInt	10		# Heartbeat interval in seconds
				# this should match the q2ew.desc heartbeat!

 LogFile	1		# If 0, don't write logfile;; if 1, do
                                # if 2, log to module log but not stderr/stdout

 LOG2LogFile 	1		# <Optional> write LOG chans to log file
				# this is useful for debugging timing or
				# data logger problems.	


TimeoutNoSend	0		# If no data for X seconds from COMSERV for
				# ALL stations,  then kill process (HB and
				# main thread). 
				# NOTE: this timeout does not take effect until
				# a trace data packet comes from comserv. That
				# is, LOG channels do not count! 
				# 
				# Iff TimeoutNoSend == 0, then this data
				# check is DISABLED and q2ew will only die
				# if EW or COMSERV dies.

# For ATWC, set this next config param to CSQ_LAST
 ComservSeqBuf	CSQ_LAST	# This is a comserv sequence control value
				# that can have the following settings:
				# CSQ_NEXT  = Get newer than already received
				# CSQ_LAST  = Get first available data (latest)
				# CSQ_FIRST = Get all data in DAQ
				# This effects what data are returned.

# These are optional for those EW users who want to have a pin number
# instead of a SCNL. The keyword here is SCNL2pinmap. Note that SCNL2pinmap replaces
 SCN2pinmap since q2ew now supports location code. No duplicate checking
# occurs

# Example
#		S	C    	N	L	pin
SCNL2pinmap 	Q003	HHZ  	AT	--	1
#SCNL2pinmap 	Q003	HHN  	AT	00	2
#SCNL2pinmap 	Q003	HHE  	AT	11	3
