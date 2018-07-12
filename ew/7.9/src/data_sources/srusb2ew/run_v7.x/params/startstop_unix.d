#
#       Startstop (Linux Version) Configuration File
#
#    <nRing> is the number of transport rings to create.
#    <Ring> specifies the name of a ring followed by it's size
#    in kilobytes, eg        Ring    WAVE_RING 1024
#    The maximum size of a ring depends on your operating system,
#    amount of physical RAM and configured virtual (paging) memory
#    available. A good place to start is 1024 kilobytes.
#    Ring names are listed in file earthworm.h.
#

 nRing               1          # was 3, but we're only testing the wave_ring
 Ring   WAVE_RING 1024

# Ring   PICK_RING 1024
# Ring   HYPO_RING 1024
#
 MyModuleId    MOD_STARTSTOP  # Module Id for this program
 HeartbeatInt  50             # Heartbeat interval in seconds
 MyClassName   TS             # For this program
 MyPriority     0             # For this program
 LogFile        1             # 1=write a log file to disk, 0=don't
 KillDelay      30            # seconds to wait before killing modules on
                              #  shutdown
# statmgrDelay		2     # Uncomment to specify the number of seconds
					# to wait after starting statmgr 
					# default is 1 second

#
#    Class must be RT, RR or FIFO
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
 Process          "srusb2ew srusb2ew.d"
 Class/Priority    TS 0
#
 Process          "wave_serverV wave_serverV_ux.d"
 Class/Priority    TS 0
#

