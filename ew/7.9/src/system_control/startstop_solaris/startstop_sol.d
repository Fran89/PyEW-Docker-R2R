#
#       Startstop (Solaris Version) Configuration File
#
#    <nRing> is the number of transport rings to create.
#    <Ring> specifies the name of a ring followed by it's size
#    in kilobytes, eg        Ring    WAVE_RING 1024
#    The maximum size of a ring depends on your operating system, 
#    amount of physical RAM and configured virtual (paging) memory 
#    available. A good place to start is 1024 kilobytes. 
#    To have bigger ring sizes, your shared memory max size should
#    be large enough too (see note below)
#
#    Ring names must be listed in file earthworm.d.
#
# Under Solaris 2.6 (and probably other versions as well), the maximum number 
# of shared memory segments is six. This means that on an out-of-the-box machine
# you can only configure six rings. If you try to configure more than that, you
# will see a cryptic message from tport_create about too many open files.  The
# fix to this problem is to add the following lines to the /etc/system
# file, and then reboot the system.
#
# set shmsys:shminfo_shmmax = 4294967295
# set shmsys:shminfo_shmmin = 1
# set shmsys:shminfo_shmmni = 100
# set shmsys:shminfo_shmseg = 20
# set semsys:seminfo_semmns = 200
# set semsys:seminfo_semmni = 70
#
# This allows for 20 rings.

 nRing               3
 Ring   WAVE_RING 1024
 Ring   PICK_RING 1024
 Ring   HYPO_RING 1024
#
 MyModuleId    MOD_STARTSTOP  # Module Id for this program
 HeartbeatInt  50             # Heartbeat interval in seconds
 MyClassName   TS             # For this program
 MyPriority     0             # For this program
 LogFile        1             # 1=write a log file to disk, 0=don't
 KillDelay      30            # seconds to wait before killing modules on
                              #  shutdown
 HardKillDelay  5             # number of seconds to wait on hard shutdown
                              #  for a child to respond to KILL signal
                              #  If missing or 0, no KILL signal will be sent
#  maxStatusLineLen   80     # Uncomment to specify length of lines in status
# statmgrDelay		2     # Uncomment to specify the number of seconds
					# to wait after starting statmgr 
					# default is 1 second

#
#    Class must be RT or TS
#    RT priorities from 0 to 59
#    TS priorities le 0
#
#    If the command string required to start a process contains
#       embedded blanks, it must be enclosed in double-quotes.
#    Processes may be disabled by commenting them out.
#    To comment out a line, preceed the line by #.
#
#
 Process          "statmgr statmgr.d"
 Class/Priority    TS 0
#
 Process          "tankplayer tankplayer.d"
 Class/Priority    TS 0
#
# Process          "coaxtoring coaxtoring.d"
# Class/Priority    TS 0
#
 Process          "tankplayer tankplayer2.d"
 Class/Priority    TS 0
#
 Process          "pick_ew pick_ew.d"
 Class/Priority    TS 0
#
 Process          "binder_ew binder_ew.d"
 Class/Priority    TS 0
#
 Process          "eqproc eqproc.d"
 Class/Priority    TS 0
#
 Process          "diskmgr diskmgr.d"
 Class/Priority    TS 0
#
 Process          "copystatus WAVE_RING HYPO_RING"
# Class/Priority    RT 5
 Class/Priority    TS 0
#
 Process          "copystatus PICK_RING HYPO_RING"
# Class/Priority    RT 5
 Class/Priority    TS 0
#
 Process          "menlo_report menlo_report.d"
 Class/Priority    TS 0

