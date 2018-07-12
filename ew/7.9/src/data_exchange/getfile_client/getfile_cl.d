#
#  getfile_cl.d  -  Configuration file for the getfile client program
#
# ServerIP      = IP address of the sendfile_srv computer's host adapter
# ServerPort    = Well-known port used by this program
# ClientIP      = IP address of this computer's host adapter
# TimeOut       = Send/receive will time out after TimeOut seconds.
# RetryInterval = If an error occurs, retry after this many seconds
# ConnectInterval = Reconnect after this many seconds to see if there are more files
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
# InDir         = Full path name of directory to contain received files.
#                 This directory must be on the same partition as is TempDir.
# Be sure the final line is terminated with a newline.
#
# 
-ServerIP    130.118.43.31   # qt1
-ServerPort  3456
-ClientIP    130.118.43.31   # qt1
-TimeOut     15
-RetryInterval 60
-ConnectInterval 300
-LogFile     3 
-TimeZone    GMT
-TempDir     /home/earthworm/working-jim/Contrib/Menlo/src/getfile_client/temp
-InDir       /home/earthworm/working-jim/Contrib/Menlo/src/getfile_client/indir
-LogFileName /home/earthworm/working-jim/Contrib/Menlo/src/getfile_client/getfile_cl.log



