
#
# This is ringtocoax's parameter file
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

MyModName         MOD_RINGTOCOAX  # Module id for this instance of ringtocoax
RingName          WAVE_RING       # Shared memory ring for input/output
LogSwitch         1               # 0 to turn off disk log file, 1 to log to file
                                  # 2 to log to module log but not stderr/stdout
HeartBeatInterval 30              # Seconds between heartbeats
MsgMaxBytes       4096            # Size of biggest message to be sent
OutAddress        192.168.5.255   # IP address of net to broadcast to.
                                  # Set the node number to 255
                                  # eg. OutAddress    192.168.4.255
OutPortNumber     10001           # The well-known port number to broadcast on.
                                  # Note: Under Solaris, if the port number is at or below 1024,
                                  #       ringtocoax must run as root.
MaxLogo           256             # Maximum number of logos we can handle
                                  # Set to a number >= 0
ScrnMsg           1               # If 1, print message logos on screen
                                  # If 0, don't print logos.  If 1, print logos.
CopyStatus        0               # if 0, don't pass heartbeats, errors, or restarts.  If 1, pass them.
BurstInterval     10              # Sleep this many msec between bursts
BurstCount        3               # Number of packets per burst
SqrtCount         2000            # Number of square roots between packets within a burst.
#
# List each logo that you want to broadcast in a BroadcastLogo command.
# If the BroadcastLogo is omitted, all messages in RingName will be shipped.
#               installation   module_id     message_type 
#               ------------   ---------     ------------
BroadcastLogo   INST_WILDCARD  MOD_WILDCARD  TYPE_TRACEBUF
