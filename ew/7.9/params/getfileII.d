#
#  getfileII.d  -  Configuration file for the getfileII program
#
# ServerIP      = IP address of this computer's host adapter
# ServerPort    = Well-known port used by this program
# TimeOut       = Send/receive will time out after TimeOut seconds.
# LogFile       = If 0, don't log to screen or disk.
#                 If 1, log to screen only.
#                 If 2, log to disk only.
#                 If 3, log to screen and disk.
# TimeZone      = Timezone of computer system clock (ignored in Solaris)
# LogFileName   = Full path name of log files. The current date will
#                 be appended to log file names.
# TempDir       = Full path name of directory to contain temporary files.
#                 This directory must be on the same partition as the
#                 client directories.
# Client        = IP address of trusted client, followed by the path of
#                 directory to contain files sent by this client.
#                 Connections from all other IP addresses are rejected.
#                 From 1 to 100 of these lines are allowed.
#                 The client directories must be on the same disk
#                 partition as is TempDir.
# Be sure the final line is terminated with a newline.
#
# set up to run on aramis and get stuff from athos
-ServerIP    128.32.149.237   # Athos
-ServerPort  3456
-TimeOut     15
-LogFile     3
-TimeZone    GMT
-TempDir     /home/redi/run/get_evt_athos/temp
-LogFileName /home/redi/run/logs/get_evt_athos.log
-Client    128.32.149.217   /home/redi/run/get_evt_athos/get



