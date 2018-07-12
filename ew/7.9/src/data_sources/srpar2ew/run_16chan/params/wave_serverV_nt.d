#
#         Wave ServerV Configuration File 
#
#	Note:  All directories defined in this configuration file must already
#		exit or WaveServerV will die.
#
MyModuleId    MOD_WAVESERVERV # wave_server's module id
RingName      WAVE_RING        # name of transport ring to get data from
LogFile       1                # 1=write log file to disk; 0=don't
                               # 2=write to module log but not stderr/stdout
HeartBeatInt  15               # seconds between heartbeats to statmgr
ServerIPAdr   127.0.0.1        # address of machine running wave_server: Using loopback device
ServerPort    16030            # port for receiving requests & sending waves
GapThresh     1.5              # threshhold for gap declaration 
                               # (in sampling periods)

#

SocketTimeout 11000  # Timeout length in MILLISECONDS for socket calls
                     # This is for calls sending responses back to the
                     # client. Values should be a few seconds, certainly
                     # less than one minute.


ClientTimeout  60000 # Optional.  Not recommended feature but it does work.
                     # Timeout in MILLISECONDS for response from client. 
                     # Threads that have not heard anything from their client 
                     # in this period will exit.
                     # Comment out or set to -1 if you don't want to 
                     # kill threads of silent clients.


# Each tank file has an associated in-memory index.  On re-start, the
# index image on disk must be updated to match the tank.  The more out
# of date the on-disk index is, the longer it takes to rebuild.  Rebuild
# times can be from milliseconds to minutes per tank, depending how large
# the tank is and how old the index is.
# Set IndexUpdate to the length in time in seconds between
# updates to disk.  The larger the update interval, the longer
# a crash recovery will take.  The smaller the update interval
# the more disk I/O that is required for wave_server to operate,
# and thus the slower it will operate, once it has reached I/O
# saturation. 

IndexUpdate   10		       
			       

# Similar to an Index, each tank has TANK structure that depicts the tank.
# The tank structure is maintained in memory, and periodically written to
# disk.  The TANK structure tracks the status of the tank.  Any data written
# to the tank since the last time the TANK structure was written to disk
# is effectively lost.  TankStructUpdate is the interval in seconds that the
# Tank Structure file on disk is updated.  The higher the interval, the more
# the tank data is that is potentially lost in a crash, the lower the interval
# the more the disk I/O that is required for wave_server to operate.

TankStructUpdate 1

# The file where TANK structures are stored

TankStructFile  c:\earthworm\v7.4\run_symres\data\p1000-1.str

# I open many files, one tracedata file for each SCNL channel to serve
# At 500 bytes/second, 1 channel requires 41.2 megabytes per day.
# NOTE: Record size must be multiple of 4 bytes or wave_serverV will crash 
# with data misalignment. 
# Also, record size must not be greater than MAX_TRACEBUF_SIZ, currently 4096,
# (defined in tracebuf.h)
#
#           SCNL      Record       Logo                  File Size   Index Size       File Name	    New       
#          names       size  (TYPE_TRACEBUF2 only)       (megabytes) (max breaks)     (full path)      Tank      

Tank    CH00 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch00.tnk
Tank    CH01 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch01.tnk
Tank    CH02 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch02.tnk
Tank    CH03 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch03.tnk
Tank    CH04 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch04.tnk
Tank    CH05 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch05.tnk
Tank    CH06 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch06.tnk
Tank    CH07 SHZ SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch07.tnk
Tank    DGTL DIG SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\dgtl.tnk
Tank    MARK GPS SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\mark.tnk
Tank    CNTR GPS SR P1 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\cntr.tnk

Tank    CH08 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch08.tnk
Tank    CH09 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch09.tnk
Tank    CH10 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch10.tnk
Tank    CH11 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch11.tnk
Tank    CH12 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch12.tnk
Tank    CH13 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch13.tnk
Tank    CH14 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch14.tnk
Tank    CH15 SHZ SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ch15.tnk
Tank    DGTL DIG SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\dig2.tnk
Tank    MARK GPS SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\mrk2.tnk
Tank    CNTR GPS SR P2 480   INST_UNKNOWN    MOD_WILDCARD       1         10000         c:\earthworm\v7.4\run_symres\data\ctr2.tnk


# Advanced Options
# YES = 1, NO = 0, NO = (default)

#RedundantTankStructFiles  Set to 1 to use redundant tank struct files. (Recommended)
RedundantTankStructFiles 1

#RedundantIndexFiles  Set to 1 to use redundant tank index files. (Recommended)
RedundantIndexFiles      1


# Must be set if RedundantTankStructFiles = 1
TankStructFile2  c:\earthworm\v7.4\run_symres\data\p1000-2.str


#InputQueueLen:  The number of messages to buffer.  Messages are buffered
#in a queue.  They are added to the queue when they are pulled off of an
#earthworm message ring, they are removed from the queue when the main
#thread is ready to process them.  Depending on the CPU and disk speed
#of the machine you are using, this number should be about twice the
#number of tanks you are trying to serve.  Slower machines may need
#larger queues.
InputQueueLen 30


###################################
#           Other Optional Commands


#MaxMsgSize: Optional command to tell wave_server about TRACEBUF2 messages
# that could be larger than any going to tanks for this server. This
# may happen if you have two wave_servers and TRACEBUF2 sources that
# produce different size messages, e.g., ref2ew messages are 1064 bytes.
MaxMsgSize 1064

# Debug - optional value of the debug flag. Higher debug 
#    levels include all debug messages from the lower levels,
#    plus more. WARNING: Debug files can get VERY VERY LARGE.
#
#   While the scope of each Debug level may vary, following
#    values are accepted:
#
# Debug 1 ==>  Basic user level: will periodically log 
#    message queue watermarks and print the server thread status 
#    table. 
#
# Debug 2 ==> Advanced user level: Everything from lower debug  
#    levels plus additional information which could be used
#    to troubleshoot installation problems.
#
# Debug 3 ==> Advanced programmer level: Everything from lower debug  
#    levels plus additional information about the execution flow,
#    and other low-level debugging information.
#
#
#  NOTE: This command is optional. The absence of Debug means that 
#        only error conditions will be logged and reported.
#
#Debug 1

#SocketDebug Set to 1 to get SOCKET_ew debug statements
SocketDebug 0 

#PleaseContinue  Set to 1 to have wave_server continue, even if
#  there are errors during initialization
# PleaseContinue 1

#ReCreateBadTanks Set to 1 to have bad tanks re-created from scratch.
#ReCreateBadTanks 1

#SecondsBetweenQueueErrorReports   Minimum period of time between error
#  reports to statmgr due to the internal message queue being lapped,
#  and thus messages being lost.  Default is 60 seconds
#SecondsBetweenQueueErrorReports 30

#MaxServerThreads  Maximum of server threads to deploy to handle client
#  requests.  Default is 10.
#MaxServerThreads 10

#QueueReportInterval  The minimum number of seconds between
#  reports on internal queue high and low water marks.  The default is 30.
#QueueReportInterval 5

#AbortOnSingleTankFailure  Set to 0 to have wave_server continue even
#if there is a fatal error on a tank during normal processing.
#if this flag is not set to 0, wave_server will die if any type of 
#IO error occurs on any tank.  If set to 1 wave_server will not exit
#unless there is a server wide error.
#AbortOnSingleTankFailure 1



#TruncateTanksTo1GBAndContinue  Uncomment entry to have wave_server truncate
# any tanks that are >1GB down to 1GB in size.  1GB is the maximum save tank
# size in wave_serverV.  This will NOT affect EXISTING TANKS, only new ones
# listed in the config file.
#TruncateTanksTo1GBAndContinue 
