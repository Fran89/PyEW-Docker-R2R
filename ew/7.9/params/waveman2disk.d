
#
# Configuration file for waveman2disk:
#

LogFile            1    # 0 means don't create a disc log file. 1=> do.
                        # 2 means write to module log but not stderr/stdout

#
# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
#
Debug

#
# Data input method:
#  triglist - read the file specified by TrigFile option, and 
#             process the TYPE_TRIGLIST2K messages in there
#             NOTE: The last TYPE_TRIGLIST2K message (and thus the 
#                   file) in a triglist file MUST end with a carriage 
#                   return.  Else the last line will not be processed.
#
#  interactive - specify one of more SaveSCN entries in this file
#                 or an external file included with @. Then, the 
#                 user is prompted for start and end time of trace
#                 snippets to be saved
#
InputMethod          triglist

#
# TrigFile - name of the file containing TYPE_TRIGLIST msgs
#  ** required only if InputMethod is triglist.
#
TrigFile             /tmp/trigmsg

#
# SaveSCNL - List SCNs to save in the interactive mode. The user 
#           will specify start and end time 
#  ** required only if InputMethod is interactive.
#
#SaveSCNL             * EHZ MB *
#SaveSCNL              * * * *
#SaveSCNL             KIWB SHZ AK 01


#
# OPTIONAL - specify StartTime and Duration for the interactive
#   trace request. If these are not specified, waveman2disk will
#   prompt the user for them.
#   StartTime Format: yyyymmddhhmmss
#  ** effective only if InputMethod is interactive.
#

StartTime            20000111120000
Duration             7200


#
# list of ip addresses and ports of the  WaveServers we're to use
#
WaveServer           aaa.bbb.cc.ddd      16021

#
# If a WaveServer doesn't talk to us in this 
# many seconds, we'll abort that request
#
# NOTE: If you are requesting large blocks of time, e.g 
#  in the tank mode, you will want to set this value to 
#  something quite high - longer than the time interval 
#  requested.
#
TimeoutSeconds        500


#
# Max number of traces we'll ever see in one event
#
MaxTraces             100

#
# TraceBufferLen: 
#    largest trace snippet we'll ever have to deal with (kb)
#
# SUDS: if you intend to produce PC-SUDS files for viewing 
# with sudsplot viewer, you will want to keep this number 
# low (around 1000) because of sudsplot's memory limitations
#
TraceBufferLen        72000

#
# number of sample periods after which we declare a gap
#
GapThresh              20

#
# SCNL list of stations to write for each trigger message,  these get
#      written in addition to scn's in the trigger message, or
#      those listed in SaveSCN section
# This is OPTIONAL and only meaningful in triglist mode
#
#TrigStation           GCMT EHZ MB 01
#TrigStation           BZMT EHZ MB --


#
# Minimum duration (seconds) for 
#
# Minimum number of seconds to save for extra channels that we
# are saving (i.e., those channels not in the TYPE_TRIGLIST message, 
# or channels specified in SaveSCN section)
#
MinDuration             30

#
# Select format of output data and the directory where it is written
# Only one of the following pairs should be uncommented.
#
# SUDS
#
#DataFormat                  suds
#OutDir                      "/home/earthworm/SUDS"
#
# SAC
#
DataFormat                   sac
OutDir                       "c:\earthworm\SAC"
#
# AH
#
#DataFormat                  ah
#OutDir                     "/home/earthworm/AH"
#
# SEISAN
#
#DataFormat                  seisan
#OutDir                     "/home/earthworm/seisan"
#
# GSE
#
#DataFormat                  gse_int
#OutDir                     "/home/earthworm/gse_int"
#
# Tankplayer
#
#DataFormat                 tank
#OutDir                     "./tanks/"

# UW2 format - UNIX ONLY
#DataFormat                uw
#OutDir                     "./uw/"
#
#
# Mini-SEED
# Mini-SEED output format is currently only available on Solaris!
#
#DataFormat                 mseed
#OutDir                     "/earthworm/data/mseed"

# PSN4
# NOTE PSN4 format is only available and compiled for Windows!
#
#DataFormat                 psn4
#OutDir                     "c:\earthworm\psn4"
#
# NOTE WFDISC format is only available and compiled for Linux!
# WFDISC
# 
#DataFormat					wfdisc
#OutDir						/home/tim/wfdisc_output


#
# Specify on what platform the output files will be used:
# intel or sparc - with this information, files will be written out
# in the correct byte order.
#
OutputFormat                sparc

