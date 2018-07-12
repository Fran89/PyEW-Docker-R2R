#
# sendfileII.d  -  Configuration file for the sendfileII program
#
# ServerIP      = IP address of computer running getfileII
# ServerPort    = Well known port of getfileII program
# TimeOut       = Send will time out after TimeOut seconds.
# AckTimeOut    = Wait AckTimeOut seconds for "ACK" from getfileII.
#                 If ACK is not received, sendfileII assumes that getfileII
#                 did not get the whole file, so sendfileII will try again.
# RetryInterval = If an error occurs, retry after this many seconds
# OutDir        = Path of directory containing files to send to getfileII
# SendPause     = Wait SendPause milliseconds before trying to open
#                 a file for shipping. This gives file writers of unknown
#                 behavior some time to finish writing & close the file.
#                 Optional: default is SendPause = 0.
# LogFile       = If 0, don't log to screen or disk.
#                 If 1, log to screen only. 
#                 If 2, log to disk only. 
#                 If 3, log to screen and disk.   
# TimeZone      = Timezone of computer system clock (ignored in Solaris)
# LogFileName   = Full path name of log files. The current date will
#                 be appended to log file names.
#
# configured to send data to sage2 at MP
#
-ServerIP      130.118.43.39
-ServerPort    3456
-SendPause     200
-TimeOut       15
-AckTimeOut    30
-RetryInterval 60
-LogFile       3   
-TimeZone      GMT

# Solaris
# -------
-OutDir        /home/redi/run/sendfileII_dir
-LogFileName   /home/redi/run/logs/sendfileII.log

# Windows
# -------
#-OutDir        c:\earthworm\outdir
#-LogFileName   c:\earthworm\run\log\sendfileII.log

