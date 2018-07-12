#
# gcf2ew configuration file gcf2ew.d
#
# This program pulls GCF packets from an MSS100 connected Guralp Digitizer and converts them to
# Earthworm trace buf messages, and stuffs them into a wave ring.
#
#
 ModuleId	MOD_GCF2EW	# module id for this import,
 RingName	WAVE_RING	# transport ring to use for input/output,

 HeartbeatInt	10		# Heartbeat interval in seconds
				# this should be smaller than the gcf2ew.desc heartbeat!

 LogFile	1		# If 0, don't write logfile;; if 1, do
                                # if 2, log to module log but not stderr/stdout

 SaveSOH2LOG 	1		# <Optional> write State Of Health (SOH) LOG chans to log file
				# this is useful for debugging timing or
				# data logger problems.	

InjectSOH	1		# inject the SOH message as a TYPE_GCFSOH_PACKET type message


TimeoutNoSend	60		# If no data for X seconds from GCF
				# NOTE: this timeout does not take effect until
				# a trace data packet comes from GCF. That
				# is, SOH channels do not count! 
				# 
				# Iff TimeoutNoSend == 0, then this data
				# check is DISABLED and the program will only die if killed
				#
	
HostAddress	mss		# the IP or hostname of the MSS100 unit
PortNumber	3001		# the port number of the MSS100 unit

# The following mapping is used to map a system_id and stream_id to an SCN and as
# a SELECTOR on those channels which should be passed along to Earthworm...channels not
# matching the criteria below are dumped.
# Example
#		Sys	Strm	S	C    	N	
#InfoSCN 	0D0081	1031Z2	ISTI	HHZ  	CI
#InfoSCN 	0D0081	1031N2	ISTI	HHN  	CI
#InfoSCN 	0D0081	1031E2	ISTI	HHE  	CI

#InfoSCN 	0D0594	1031Z2	ISTI	HHZ  	ZZ
#InfoSCN 	0D0594	1031N2	ISTI	HHN  	ZZ
#InfoSCN 	0D0594	1031E2	ISTI	HHE  	ZZ

#		Sys	Strm	S	C    	N	L
InfoSCNL	D0594	1031Z2	ISTI	HHZ  	ZZ	01
InfoSCNL 	D0594	1031N2	ISTI	HHN  	ZZ	01
InfoSCNL 	D0594	1031E2	ISTI	HHE  	ZZ	01
