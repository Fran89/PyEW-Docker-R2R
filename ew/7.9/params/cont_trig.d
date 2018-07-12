#
#	IMPORTANT NOTE ABOUT RUNNING CONT_TRIG
#
# Cont_Trig issues continuous trigger messages for all the stations in
# the station file. This allows, in combination with a trig2disk,
# to save continuous waveform for a specific station list in
# fixed length files.
#
# In order for times to be compared between machines, all machines should
# be synchronized within a few seconds or better. Xntp (available with
# Solaris2.6 and also public domain) is a good choice for time
# synchronization. 

# Because Cont_trig uses both system and data time to perform the
# trigger logic, the system time must be set to UTC. If this is
# not done, no triggers will occur.
# Under NT, TZ must be set to GMT in ew_nt.cmd in order to run under
# GMT time.
#

#
# Cont_Trig's Parameter File
#

#
#  Basic Earthworm Setup
#
MyModuleId	MOD_CONT_TRIG	# Module id for this instance of Cont_Trig -
				#
Debug		2		# Write out debug messages? (0 = No,
				#   1 = Minimal, 4 = Chatterbox )
RingName   	CONT_RING	# Name of ring to which triggers will be
				#   written - REQUIRED.
HeartBeatInterval	30	# Heartbeat Interval (seconds).

#
# Cont_Trig Specific Setup
#
StationFile	"pnsn_trig.sta"	# Name of file containing station information -
				#   REQUIRED.
Latency		15		# Number of seconds that the Network clock
				#   is behind wall clock REQUIRED.
				#   This is the time to wait before sending a trigger message for T time
TriggerDur   60			# Number of seconds for the trigger duration REQUIRED
				#   This is the length of the trigger, and the interval at wich messages are sent
				#   As so, it represents the length of data file produced
CompAsWild                      # Flag (no value) to list component names in
                                #   trigger messages as "*" (wildcard).
OriginName	"toto"		# CodeName of this continuous trigger (9 characters max) OPTIONAL

# Load the next valid trigger sequence number
@cont_id.d            # this name is hard-coded; do not change
