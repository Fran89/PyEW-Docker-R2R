#
# This is RingtocoaxII's parameter file
#
# This program gets earthworm messages from a transport ring and broadcasts
# them to ethernet.
#
#	*** WARNING ***
#	This module does connection-less broadcasts.
# 	In normal usage, it broadcasts onto an isolated, dedicated class b
#	subnet. There must be no routing to or from that subnet.
#	The broadcast is restricted to that subnet as stated below.
#	Note that the companion module, coaxtoring, has strange
#	system-specific addressing requirements

# General Info
MyModuleId        MOD_RINGTOCOAX   # Module id for this instance of ringtocoax
RingName          WAVE_RING        # Shared memory ring for input/output
HeartBeatInterval 30               # Seconds between heartbeats
LogSwitch         1                # If 0, turn off disk log file
                                   # If 1, log to disk file and stderr/stdout
                                   # If 2, log only disk file
Debug             1                # If 1, print Adaptive PacketsPerBurst info
                                   # If 2, also print message logos on screen
                                   # If 0, don't print debug info.
                                   # NOTE: Logging logos / Adaptive info to disk can 
                                   # slow down RingtocoaxII's performance.  Debug
                                   # should only be used for troubleshooting.
                                   # OPTIONAL, defaults to 0.
# Message Settings
MsgMaxBytes       4096             # Size of biggest message to be sent
CopyStatus        0                # If 0, don't pass heartbeats, errors, or restarts.  
                                   # If 1, pass them.
MaxLogo           256              # Maximum number of logos we can handle
                                   # Set to a number >= 0
# List each logo that you want to broadcast in a BroadcastLogo command.
# If the BroadcastLogo is omitted, all messages in RingName will be shipped.
#               installation   module_id     message_type 
#               ------------   ---------     ------------
BroadcastLogo   INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF

# Network Settings
OutAddress        192.168.14.255   # IP address of net to broadcast to.
                                   # Set the node number to 255
                                   # eg. OutAddress    192.168.4.255
OutPortNumber     10001            # The well-known port number to broadcast on.
                                   # Note: Under Solaris, if the port number is at or below 
                                   #       1024, ringtocoax must run as root.
# Queue Settings
InputQueueLen       1000           # The Desired Size of the Input Queue in Messages,
                                   # Note: Actual size is InputQueueLen-1.
QueueOptimumPercent 10             # Optimum queue level in percent.  Used to determine 
                                   # percent excedence.  OPTIONAL, defaults to 10
ExcedenceConstant   1              # Constant to multiply the percent excedence by to control
                                   # how much the desired excedence rate can change.
                                   # OPTIONAL, defaults to 1.
STAInterval         1.5            # Interval in seconds over which the short term average
                                   # of the queue height is computed. 
                                   # OPTIONAL, defaults to 1.5.
QueueReportInt	  20             # Seconds between queue size reports
                                   # OPTIONAL, defaults to 300 seconds.
# Data Export Rate Settings
MaxPacketsPerBurst  200            # Maximum allowed Number of packets per burst the reciver 
					     # can send
MinPacketsPerBurst  10             # Minmum allowed Number of packets per burst the reciver
           				     # can send. OPTIONAL, defaults to 1.
SqrtCount           2000           # Number of square roots between packets within a burst.
BurstInterval       10             # Sleep this many msec between bursts
ThroughputReportInt 20             # Seconds between throughput reports
                                   # OPTIONAL, defaults to 300 seconds.


