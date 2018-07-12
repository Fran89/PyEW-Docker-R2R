#
# sendfilemt.d  -  Configuration file for the sendfilemt program
#
# LogFile: Sets the switch for writing to a log file in the EW_LOG
#          directory.
#          If 0, don't write log file to disk.
#          If 1, write log file to disk.
#          If 2, write log file to disk but not to console window.
LogFile 2

# This is the module id of this instance of sendfilemt.  It must be
# entered in the earthworm.h file.
MyModuleId  MOD_SENDFILEMT

# sendfilemt will send heartbeats and error messages to statmgr
# via an Earthworm transport ring.  Also, sendfilemt can be restarted
# via this ring.  The ring must be entered in earthworm.h and
# startstop.d.
RingName  HYPO_RING

# This is the number of seconds between heartbeats sent to statmgr
# via the Earthworm transport ring.
HeartBeatInterval 30

# sendfilemt will wait SendPause milliseconds before trying to open
# a file for shipping. This gives file writers of unknown behaviour
# some time to finish writing and close the file.  In Windows, set
# SendPause to zero, because files can be opened by only one program
# at a time.  In Solaris, set SendPause > 0.
SendPause  200

# If the file is not sent successfully in TimeOut seconds, the
# connection will be dropped and then retried after RetryInterval
# seconds.
TimeOut  15

# After sending a file, sendfilemt waits AckTimeOut seconds for
# an "ACK" to be returned by getfileII. If ACK is not received
# sendfilemt assumes that getfileII did not get the whole file,
# so sendfilemt will try again to send the file.
AckTimeOut  30

# If an error occurs while sending a file, try to send again after
# this many seconds.
RetryInterval  60

# Enter Server command lines to establish connections to multiple
# instances of getfileII.  Any number of Server lines may be entered.
# For each Server line, sendfilemt will create another thread.
#
# Tag        = Tag to identify file recipient (12 chars max)
# ServerIP   = IP address of computer running getfileII
# ServerPort = Well-known port of getfileII program
# OutDir     = Full path of directory with files to send to getfileII.

#           Tag         ServerIP  Port        OutDir
#           ---         --------  ----        ------
# Windows
 Server  getfilea    130.118.43.36 3456  C:\earthworm\run-will\sendfilea
 Server  getfileb    130.118.43.36 3457  C:\earthworm\run-will\sendfileb

# Solaris
#Server  getfilea    130.118.43.36 3456  /home/earthworm/run-will/sendfilea
#Server  getfileb    130.118.43.36 3457  /home/earthworm/run-will/sendfileb
