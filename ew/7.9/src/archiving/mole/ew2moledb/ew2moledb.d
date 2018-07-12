#
#   ew2moledb generic configuration file for HYPO_RING
#
#
 MyModuleId     MOD_EW2MOLEDB       # module id for this program

# Multiple declaration of InRing. The first one is used for reporting
# heartbeat and status messages. Be careful with messages duplicated
# in different rings.
# InRing         WAVE_RING           # transport ring to use for input
 InRing         PICK_RING           # transport ring to use for input
 InRing         HYPO_RING           # transport ring to use for input
 InRing         ALARM_RING          # transport ring to use for input

 HeartBeatInt   30                  # EW internal heartbeat interval (sec)
 LogFile        1                   # If 0, don't write logfile
                                    # if 2, write to module log but not to
                                    # stderr/stdout .

# Unique name that indentifies the EW instance is usually declared as variable
EWInstanceName    ${EWMOLE_INSTANCENAME}
#
# Mole DB connection parameters
DBHostname        ${MOLEDBHOSTNAME}
DBName            ${MOLEDBNAME}
DBUsername        ${MOLEDBUSER}
DBPassword        ${MOLEDBPASS}
# DBPort           0

# Seconds to wait after a DB error
WaitSecAfterDBError 5

# Flag to debug MySql instructions
DebugMySql 0

# Logos of messages to export to client systems
# Each message of a given logo will be written to a file with the specified
# File Suffix at the end of the file name folloing `.'.
# Do NOT include a `.' in the suffix.
# Use as many GetMsgLogo commands as you want.

#              Installation           Module       Message Type

# Message types usually read from WAVE_RING
 # GetMsgLogo    ${EW_INSTALLATION}   MOD_WILDCARD   TYPE_TRACEBUF2

# Message types usually read from PICK_RING
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_PICK_SCNL
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_CODA_SCNL
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_QUAKE2K
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_LINK

# Message types usually read from HYPO_RING
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_HYP2000ARC
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_STRONGMOTIONII
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_MAGNITUDE

# Earlybird Message types
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_PICKTWC
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_HYPOTWC
 GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_ALARM

# Other message types
# WARNING: if you use copystatus, the messages TYPE_HEARTBEAT and TYPE_ERROR
# are duplicated on different rings. If you want to archive them, read only
# from the destination ring of the copystatus.
 # GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_HEARTBEAT
 # GetMsgLogo    ${EW_INSTALLATION}    MOD_WILDCARD   TYPE_ERROR
 

 MaxMsgSize        56700        # maximum size (bytes) for input msgs
                                # MAX_BYTES_PER_EQ from earthworm_defs.h
 QueueSize         5000         # number of msgs to buffer. Useful in case of temporary
				# database connection failures, ew2moledb buffers the
                                # loading operations and postpone their executions until
                                # the connection was reestablished.

#   mail commands are optional. ew2moledb is also able to sent messages
#   TYPE_PICK_SCNL, TYPE_CODA_SCNL, TYPE_HYP2000ARC and TYPE_MAGNITUDE,
#   to the email recipients specified below.
#
#   Syntax
#     mail  <emailAddress1>
#     mail  <emailAddress2>
#             ...
#     mail  <emailAddressN>
#

#   Specify the name of a computer to use as a mail server.
#   This system must be alive for mail to be sent out.
#   This parameter is used by Windows NT only.
MailServer  mail.server.domain

# Specify the "From" line for the email messages. (not required)
# If commented out, the email "From" field will be filled out
# with environment variables:  %USERNAME%@%COMPUTERNAME%.
# This parameter is used by Windows NT only.
# mailFrom username@thishost
mailFrom "account@server.domain"

# Mail program to use, e.g /usr/ucb/Mail (not required)
# If given, it must be a full pathname to a mail program
#
# MailProgram /usr/ucb/Mail
# MailProgram "/usr/ucb/Mail -r account@server.domain"

