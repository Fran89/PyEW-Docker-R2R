#
#                  DiskMgr's Configuration File
#
MyModuleId    MOD_DISKMGR   # module id for this program,
RingName        HYPO_RING   # transport ring to write to,
# CheckInterval        30   # Optional. Default is 15 seconds.
LogFile                 0   # If 0, don't write logfile at all, if 1, do
                            # if 2, write module log but not to stderr/stdout
Min_kbytes           5000   # Yell when free disk space is lower than
                            # this many kbytes
DefDir             /spool   # Optional command.  If specified, check disk
                            # availability from this directory.
