#
#                    Status Manager Configuration File
#                             (statmgr.d)
#
#   This file controls the notifications of earthworm error conditions.
#   The status manager can send pager messages to a pageit system, and
#   it can also send email messages to a list of recipients.
#   Earthquake notifications are not handled by the status manager.
#   In this file, comment lines are preceded by #.
#
MyModuleId  MOD_STATMGR

#   <RingName> specifies the name of the transport ring to check for
#   heartbeat and error messages.  Ring names are listed in file
#   earthworm.h.  Example ->  RingName HYPO_RING
#
RingName    WAVE_RING    # was HYPO_RING, but we only use WAVE_RING

#   <GetStatusFrom> lists the installations & modules whose heartbeats
#   and error messages statmgr should grab from transport ring:
#
#              Installation     Module           Message Types
GetStatusFrom   INST_UNKNOWN   MOD_WILDCARD   # heartbeats & errors

#   <LogFile> sets the switch for writing a log file to disk.
#             Set to 1 to write a file to disk.
#             Set to 0 for no log file.
#             Set to 2 for module log file but no logging to stderr/stdout
#
LogFile   1

#   <heartBeatPageit> is the time in seconds between heartbeats
#   sent to the pageit system.  The pageit system will report an error
#   if heartbeats are not received from the status manager at regular
#   intervals.
#
heartbeatPageit  60

#   <pagegroup> is the pager group name.
#   The pageit program maps this name to a list of pager recipients.
#   This line is required.
#
pagegroup  larva_test

#   Specify the name of a computer to use as a mail server.
#   This system must be alive for mail to be sent out.
#   This parameter is used by Windows NT only.
#
MailServer  YourMailComputer

#   Any number (or none) of email recipients may be specified below.
#   These lines are optional.
#
#   Syntax
#     mail  <emailAddress1>
#     mail  <emailAddress2>
#             ...
#     mail  <emailAddressN>
#
mail  YourName@YourWebAddress.com
#

#
# Mail program to use, e.g /usr/ucb/Mail (not required)
# If given, it must be a full pathname to a mail program
MailProgram /bin/mail

#
# Subject line for the email messages. (not required)
#
Subject "This is an earthworm status message"

#
# Message Prefix - useful for paging systems, etc.
#    this parameter is optional
#
MsgPrefix "(("

#
# Message Suffix - useful for paging systems, etc.
#    this parameter is optional
#
MsgSuffix "))"

#   Now list the descriptor files which control error reporting
#   for earthworm modules.  One descriptor file is needed
#   for each earthworm module.  If a module is not listed here,
#   no errors will be reported for the module.  The file name of a
#   module may be commented out, if it is temporarily not to be used.
#   To comment out a line, insert # at the beginning of the line.
#
Descriptor  statmgr.desc
Descriptor  srusb2ew.desc
Descriptor  wave_serverV.desc
Descriptor  startstop.desc
