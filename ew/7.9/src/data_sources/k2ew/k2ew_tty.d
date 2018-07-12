#
# k2ew configuration file
#
# This code receives serial data stream (SDS) packets from the K2 via
# a Unix TTY port, converts them into Earthworm trace buf messages, and
# stuffs them into a wave ring.
#

TtyName        /dev/sts/ttyc29  # TTY port for K2
Speed           19200           # K2 baud rate

ModuleId        MOD_K2EW        # module id for this import
RingName        WAVE_RING       # transport ring to use for input/output

LogFile         1               # If 0, don't output to logfile; if 1, do
                                # if 2, log to module log but not stderr/stdout

HeartbeatInt    30              # Heartbeat interval in seconds

Network         NC              # Network name for EW msg headers

ForceBlockMode  0		# should be set to 1 for K2's with modems
				# so that the k2ew program can get control at startup

LCFlag 1			# Indicator for action on missing
				# ChannelNames and LocationNames
				# 1: use the default location code and
				#    k2-configured channel code if none
				#    found in k2ew configuration
				# 2: Exit with error if any data channel
				#    is not assigned in ChannelName and 
				#    LocationName commands.

# Optional commands:

BasePinno     1001              # Earthworm pin numbers are assigned by
                                # adding this base number to the stream
                                # number (0 - 5) of each packet
                                # default: 0

# StationID SEAB		# If specified, assign a different station
				# code to the TRACEBUF packets than what
				# is configured into the K2

# ChannelNames	HHE,HHN,HHZ	# If present, assign different channel names
				# to the TRACEBUF packets that the names
				# configured in the K2. The list can be
				# delimited by commas or spaces. See the
				# documentation for details about channel
				# and serial stream numbering conventions!
                                # List only STREAMed channels here, which
                                # may be fewer than the K2 acquires.

# LocationNames 01,01,01	# If present, assign location codes to
				# the TRACEBUF packets emitted by k2ew.
				# Otherwise, default location code
				# will be assigned; currently "--"
                                # List only STREAMed channels here, which
                                # may be fewer than the K2 acquires.

# HeaderFile "..\k2hdr\k2hdr.SEA" # If specified, k2ew will write the K2 binary
                                # header to this file.  The file is created at
                                # program startup and once per day thereafter.
                                # default: none

# RestartFile   "restart.SEA"   # If you want to attempt a K2 restart, give
                                # a name for the restart file. It MUST be
                                # a unique name for this K2 or gross confusion
                                # will result.
                                # default: none

MaxRestartAge    60             # Maximum age of the restart file, in seconds
                                # If the restart file is older than this
                                # when k2ew restarts, a normal start will
                                # be used
                                # Should be less than WaitTime (below)
                                # default: 0 (seconds)

CommTimeout   5000              # If unable to communicate with K2 for X
                                # milliseconds, then terminate
                                # default: 5000 (milliseconds)

WaitTime  60                    # Maximum number of seconds to wait for a
                                # packet to be resent from the k2
                                # default: 60 (seconds)

# DontQuit                        # Optional command; if present, k2ew will
                                # not quit on timeouts. K2ew will quit on
                                # errors in any event.

# MaxBlkResends 4		# Sets the maximum number of times a resend
				# request may be sent to the K2 asking for a
				# given packet. 

# MaxReqPending 6		# Sets the maximum for the total number of
				# data blocks upon which resend requests may
				# be pending to the K2 at any given time. 

# ResumeReqVal 2		# Sets the number of pending "waiting" blocks
				# that must be received before new resend
				# requests are resumed after reaching the
				# 'MaxReqPending' limit.

# WaitResendVal	120		# Sets the maximum time that k2ew will wait
				# for packets to be resent from the K2 to N
				# seconds.

# RestartComm			# This command is pertinent only when the
				# 'DontQuit' command is present. In this case,
				# when 'k2ew' detects a timeout condition (and
				# 'RestartComm' is set), the communications link
				# to the K2 will be closed and reopened before
				# 'k2ew' resumes reading data.   

StatusInterval   30             # Interval in minutes between requests for
                                # K2 status (if 0, then no status requests)
                                # default: 30 (minutes)

OnBattery                       # Report when K2 loses external power and is
                                # running on battery power.
                                # default: do not report

LowBattAlarm 110                # In tenths of volts, threshold for low
                                # battery alarm (if -1, then no alarm)
                                # default: -1

MinDiskKB  500  -1              # Low disk alarm threshold in Kilobytes, for
                                # disk A and disk B (if -1, then no alarm)
                                # default: -1 -1

ExtStatus                       # Request extended status with normal status
                                # Comment this out for no extended status
                                # default: no extended status

LowTempAlarm  150               # Low temperature alarm threshold, in tenths
                                # degree, C; default: -1000

HighTempAlarm 350               # High temperature alarm threshold, in tenths
                                # degree, C; default: 1000

GPSLockAlarm  24.0              # If GPS doesn't lock for this many hours,
                                # report to statmgr.  Optional parameter.
                                # default: Don't report to statmgr.

#InjectInfo                      # a new v2.33 directive to inject K2INFO messages into
                                # the RING...these are picked up by TNW                     
                                # default is to have this turned OFF if not present in the .d file
				#
				# NOTE in order to use this SeisNetWatch feature, you
				# need to add the message type TYPE_K2INFO_PACKET to
				# the earthworm.d. The only program which can take advantage
				# of this new message type is k2ewagent available from www.isti.com
				# This new agent monitors the parameters found in the K2 status messages

Debug 0                         # debug level: 0 or commented out for no debug
                                # logging; 1 - 4 for increasing verbosity
                                # default: no debug output
