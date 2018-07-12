

#                 Startstop Configuration File for Windows NT
#
#    <nRing> is the number of transport rings to create.
#    <Ring> specifies the name of a ring followed by it's size
#    in kilobytes, eg        Ring    WAVE_RING 1024
#    The maximum size of a ring depends on your operating system, 
#    amount of physical RAM and configured virtual (paging) memory 
#    available. A good place to start is 1024 kilobytes. 
#    Ring names are listed in file earthworm.h.
#
  nRing               4
  Ring   EEW_RING   1024
  Ring   WAVE_RING  4064
  Ring   PICK_RING  1024
  Ring   HYPO_RING  1024
#
 MyModuleId       MOD_STARTSTOP  # Module Id for this program
 HeartbeatInt     15             # Heartbeat interval in seconds
 MyPriorityClass  Normal         # For startstop
 LogFile           1             # 1=write a log file to disk, 0=don't, 
                         # 2=write to module log but not stderr/stdout
 KillDelay        30             # number of seconds to wait on shutdown
                                 #  for a child process to self-terminate
                                 #  before killing it
 # statmgrDelay     2        # Uncomment to specify the number of seconds
                       # to wait after starting statmgr 
                       # default is 1 second
# maxStatusLineLen 80            # Uncomment to set the maximum value of a 
                                 # status line length. 80 is the default
# PipeName      EarthwormStartstopPipe # name of "named pipe" for 
                                 # startstopconsole to access. you only
                                 # need to use this if you're running 
                                 # multiple startstops on one Windows box

#
#    PriorityClass values:
#       Idle            4
#       Normal          9 forground, 7 background
#       High            13
#       RealTime        24
#
#    ThreadPriority values:
#       Lowest          PriorityClass - 2
#       BelowNormal     PriorityClass - 1
#       Normal          PriorityClass
#       AboveNormal     PriorityClass + 1
#       Highest         PriorityClass + 2
#       TimeCritical    31 if PriorityClass is RealTime; 15 otherwise
#       Idle            16 if PriorityClass is RealTime; 1 otherwise
#
#    Display can be either NewConsole, NoNewConsole, or MinimizedConsole.
#
#    If the command string required to start a process contains
#    embedded blanks, it must be enclosed in double-quotes.
#    Processes may be disabled by commenting them out.
#    To comment out a line, preceed the line by #.
#

 Process          "tankplayer tankplayer.d"
 PriorityClass     Normal
 ThreadPriority    Normal
 Display           MinimizedConsole


 Process          "pick_eew_v1 pick_eew.d" 
 PriorityClass     Normal
 ThreadPriority    Normal
 Display           NewConsole
 
 Process          "tcpd_v1   tcpd.d"
 PriorityClass     Normal
 ThreadPriority    Normal
 Display           NewConsole
 
 Process          "dcsn_xml_v1  dcsn_xml.d"
 PriorityClass     Normal
 ThreadPriority    Normal
 Display           MinimizedConsole

 Process          "wave_serverV  wave_serverV.d"
 PriorityClass     Normal
 ThreadPriority    Normal
 Display           MinimizedConsole 
 
 Process          "wave_viewer  wave_viewer.d"
 PriorityClass     Normal
 ThreadPriority    Normal
 Display           MinimizedConsole 

