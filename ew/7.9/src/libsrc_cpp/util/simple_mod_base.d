#
#
#
#
# This file contains the parameters which are either required
# or appropriate for a module as a result of deriving from
# the SimpleModuleBase class (in simple_mod_base.h/cpp).
#
# Just copy this file into the .d for the module using the
# derivative class and add additional values as needed.
#

# ModuleName
#
# Purpose:  Identifies this module.
#
# Required: Yes
#
# May Repeat: No
#
# Formats:
#          ModuleName <module>
#
# Examples:
#          ModuleName  MOD_MYMODULE 
#
# Comment:  Value from earthworm.d, earthworm_global.d
#

ModuleName  MOD_NOMODULE


# CmdRingName
#
# Purpose:  Identifies the name of the command ring, where
#           heartbeats and errors are sent, and which is
#           checked for the shutdown flag.
#           If InRingName (below) is omitted, incoming
#           messages will be received from this ring.
#
# Required: Yes
#
# May Repeat: No
#
# Formats:
#          CmdRingName <ring_name>
#
# Examples:
#          CmdRingName  WAVE_RING 
#
# Comment:  Value from earthworm.d
#

CmdRingName  WAVE_RING


# InRingName
#
# Purpose:  Identifies the name of a ring where incoming
#           messages will be picked up.
#           If omitted, messages will be picked up off of
#           CmdRingName.
#
# Required: No
#
# Default: (see Purpose)
#
# May Repeat: No
#
# Formats:
#          InRingName <ring_name>
#
# Examples:
#          InRingName  PICK_RING 
#
# Comment:  Value from earthworm.d
#

InRingName  PICK_RING


# OutRingName
#
# Purpose:  Identifies the name of a ring where produced
#           messages dropped for transmission.
#           If omitted messages will be dropped on InRingName
#           (If InRingName is also omitted, dropped onto
#           CmdRingName).
#
# Required: No
#
# Default: (see Purpose)
#
# May Repeat: No
#
# Formats:
#          OutRingName <ring_name>
#
# Examples:
#          OutRingName  HYPO_RING 
#
# Comment:  Value from earthworm.d
#

OutRingName  HYPO_RING


# HeartbeatSecs
#
# Purpose:  Indicates the minimum number of seconds between
#           heartbeat messages.
#
# Required: No
#
# Default:  10 seconds
#
# May Repeat: No
#
# Formats:
#          HeartbeatSecs <hb_interval>
#
# Examples:
#          HeartbeatSecs  5 
#
# Comment: Since the main thread sends the heartbeats,
#          there is a dependency on the value of
#          MainLoopSleepMS to allow the messages to be
#          sent at the appropriate frequency.
#

# HeartbeatSecs 500



# MainLoopSleepMS
#
# Purpose:  Directs the number of milliseconds that the main
#           thread (as opposed to the stacker or handler threads)
#           will sleep between loop passes.
#
# Required: No
#
# Default:  1000 ms
#
# May Repeat: No
#
# Formats:
#          MainLoopSleepMS <sleep_ms>
#
# Examples:
#          MainLoopSleepMS  2500 
#
# CAUTION: The main thread looks for shutdown flag, checks
#          the status of the worker threads, sends
#          heartbeats and possibly performs other tasks
#          (implemented in a derivative class).
#          Therefore, do not make this value too large.
#

# MainLoopSleepMS 500


# StackerSleepMS
#
# Purpose:  Directs the number of milliseconds that the stacker
#           thread will sleep between loop passes -- if there
#           is no message to copy from the input ring to the
#           internal queue.
#
# Required: No
#
# Default:  500 ms
#
# May Repeat: No
#
# Formats:
#          StackerSleepMS <sleep_ms>
#
# Examples:
#          StackerSleepMS  250 
#
# Comment: it no message is found to be stacked, the entire
#          stacker loop is performed very quickly.  So for all
#          intents, this sleep time is essentially how long
#          the stacker thread will wait until checking for
#          another message.
#          This also means that the stacker thread has
#          relatively little cpu impact until a message
#          arrives.
#
# CAUTION: The stacker thread must set status indicators
#          at least every 9 - 10 seconds or the main thread
#          will determine that it has died and terminate
#          the program.
#

# StackerSleepMS 250


# HandlerSleepMS
#
# Purpose:  Directs the number of milliseconds that the handler
#           thread will sleep between loop passes -- if there
#           is no message waiting in the queue.
#
# Required: No
#
# Default:  400 ms
#
# May Repeat: No
#
# Formats:
#          HandlerSleepMS <sleep_ms>
#
# Examples:
#          HandlerSleepMS  250 
#
# Comment: it no message is found to be processed, the entire
#          handler loop is performed very quickly.  So for all
#          intents, this sleep time is essentially how long
#          the handler thread will wait until checking for
#          another message.
#          This also means that the handler thread has
#          relatively little cpu impact until a message
#          arrives.
#          Probably want to have this time be similar to,
#          or slightly lower than StackerSleepMS. 
#
# CAUTION: The handler thread must set status indicators
#          at least every 9 - 10 seconds or the main thread
#          will determine that it has died and terminate
#          the program.
#

# HandlerSleepMS 250


# InputLogo
#
# Purpose:  Identifies the logos to take off the InputRing
#           for processing.
#
# Required: Yes
#
# May Repeat: Yes
#
# Formats:
#          InputLogo <installation> <module> <type>
#
# Examples:
#          InputLogo  INST_USNSN  MOD_MYMODULE  TYPE_PICK2K
#
# Comment:  Values from earthworm.d, earthworm_global.d
#           Wildcard is asterisk (*)
#

InputLogo  INST_USNSN  MOD_MYMODULE  TYPE_SOMETYPE


# LoggingLevel
#
# Purpose:  Identifies this level of logging to use in the module.
#
# Required: No
#
# Default: 'Details'
#
# May Repeat: No
#
# Formats:
#          LoggingLevel <level>
#
# Examples:
#          LoggingLevel  3 
#
# Comment:
#
#   Values domain:
#     0 - No logging
#     1 - Errors only
#     2 - Status (some informative messages) 
#     3 - Tracking (more messages)
#     4 - Details (many messages)
#     5 - Debug (all available messages)
#

LoggingLevel  5


# QueueSize
#
# Purpose:  Identifies the number of messages and the maximum size
#           of a message to hold in the internal circular queue.
#
# Required: Yes
#
# May Repeat: No
#
# Formats:
#          QueueSize <max_count> <max_size>
#
# Examples:
#          QueueSize 100 240
#
# Comment: Message size is in bytes.
#

QueueSize  200  120

# ==============================================================================
# ADD PARAMETERS FOR DERIVATIVE CLASSES BELOW.
# ==============================================================================


 