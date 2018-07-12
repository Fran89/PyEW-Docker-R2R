#
# snwclient.d  -  Configuration file for the snwclient program
#
# ServerIP      = IP address of computer running SNWCollectionAgent
# ServerPort    = Well known port of SNWCollectionAgent program
# TimeOut       = Send/receive will time out after TimeOut seconds.
# RetryInterval = If an error occurs, retry after this many seconds
# OutDir        = Path of directory containing files to send to getfile
# LogFile       = If 0, don't log to screen or disk.
#                 If 1, log to screen only. 
#                 If 2, log to disk only. 
#                 If 3, log to screen and disk.   
# Verbose       = If 0, don't log routine messages.
#                 If 1, log each transmission. 
#                 If 2, log each transmission and more. 
# TimeZone      = Timezone of computer system clock (ignored in Solaris)
# LogFileName   = Full path name of log files. The current date will
#                 be appended to log file names.
#
-ServerIP      130.118.43.38      # campbell
-ServerPort    6666
-TimeOut       15
-RetryInterval 60
-LogFile       3   
-TimeZone      GMT
-Verbose       1

# Solaris
# -------
-OutDir        /home/earthworm/outdir
-LogFileName   /home/earthworm/run/log/snwclient.log

# Windows
# -------
#-OutDir        c:\earthworm\outdir
#-LogFileName   c:\earthworm\run\log\snwclient.log

