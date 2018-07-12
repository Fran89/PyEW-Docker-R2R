# FILE: srusb2ew.d                Copyright (c), Symmetric Research, 2010
#
# This is the srusb2ew parameter file for acquiring data with a Symmetric 
# Research USBxCH 24 bit A/D board and putting that data onto an 
# earthworm ring.
#
# The standard earthworm import_generic/export (or export_scnl) functions
# can be used to transfer USBxCH tracebuf data from a local WAVE_RING to
# a WAVE_RING on another machine.  If you have data from two different
# USBxCH boards arriving at the same ring, you must ensure that each is
# using a different ModuleId.  Otherwise, the two sets of data will 
# appear to be scrambled together which leads to lots of message sequence
# number errors.
#
# If you wish to use MOD_SRUSB2EW for a ModuleId instead of
# MOD_ADSEND_C, you must first define it in your earthworm.d file.
#
#
# You will also need to edit other earthworm .d files in order to
# acquire and work with USBxCH data.  A simple configuration where the
# USBxCH files are acquired, saved, and viewed on one computer would
# require making edits similar to those shown below for the following files:
# 
# startstop.d - Use this file to tell earthworm to run srusb2ew to 
#               acquire data by adding lines like these for Windows:
#
#   Process          "srusb2ew srusb2ew.d"
#   PriorityClass     Normal
#   ThreadPriority    Normal
#   Display           NewConsole
#
#               Or like these for Linux:
#
#   Process          "srusb2ew srusb2ew.d"
#   Class/Priority   RR 5
#
# wave_serverV.d - Use this file to save the acquired USBxCH data into
#                  tanks.  You will need to add lines giving the channel
#                  SCNL information, size, and tank location.  The proper
#                  size is determined by the sampling rate.  Each USBxCH
#                  record will be about 1 second long so the size needed
#                  is (SamplingRate+2)*sizeof(long)+sizeof(old tracebuf header).
#                  For example, at   78.1 sps ->   80*4+67 =  387
#                  and          at  130.2 sps ->  132*4+67 =  595
#                  and          at  651.0 sps ->  653*4+67 = 2679
#                  and          at 1302.1 sps -> 1304*4+67 = 5283 -> 2675
#                  This last value is greater than the max tracebuf size 
#                  of 4096, so the data is split into two half second
#                  buffers of 1304/2*4+67 = 2675 each.
#                  You may need to increase MaxMsgSize to accomodate the
#                  larger values.  For the USB4CH at 78 sps one might 
#                  add lines like these.
#
#   Tank    CH00 xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\CH00.tnk
#   Tank    CH01 xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\CH01.tnk
#   Tank    CH02 xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\CH02.tnk
#   Tank    CH03 xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\CH03.tnk
#   Tank    DGTL xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\DIG.tnk
#   Tank    PPS  xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\PPS.tnk
#   Tank    PWR  xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\PWR.tnk
#   Tank    TEMP xxx SR -- 400   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run\data\TEMP.tnk
# 
# wave_viewer.d - Use this file to view the acquired USBxCH data.  You 
#                 will need to give the IP address and port for the 
#                 wave_server storing the data.  You may also want to add
#                 group lines identifying the channels to display.
#
#   group USB4CH_Analog
#          CH00 xxx SR --
#          CH01 xxx SR --
#          CH02 xxx SR --
#          CH03 xxx SR --
#
#   group USB4CH_Other
#          DGTL xxx SR --
#          PPS  xxx SR --
#          PWR  xxx SR --
#          TEMP xxx SR --
#
# A more complicated configuration would involve acquiring the USBxCH
# data on one computer and sending it to another for saving, processing,
# and viewing.  In that case, you would need to edit startstop.d, 
# srusb2ew.d and export_scnl.d, on the acquiring computer.  On the 
# other computer, you would need to edit startstop.d, import_generic.d, 
# wave_serverV.d and wave_viewer.d.
#
# For the export/import pair, you will need to set their IP addresses to
# point to each other, set up the heartbeat strings so that each looks for 
# the string the other is sending, and set the timeouts so that each sender
# sends a heartbeat more often than the listener requires.  You will also 
# want to include lines identifying the channels to send.  For example
#
# Send_scnl       CH00 xxx SR --                # send this specific channel
# Send_scnl       CH01 xxx SR --                # send this specific channel
# Send_scnl       DGTL xxx SR --                # send this specific channel
#
#
# Now, on to the srusb2ew.d values ...
#

Debug               2               # control amount of info sent to log file 0-8
                                    # warnings and errors always given
                                    # 0 adds startup info
                                    # 1 adds status summaries every GpsReportInterval
                                    # 2 adds config + more detail (default)
                                    # 3 adds trace data message info every second
                                    # 4 adds counter values, NMEA strings every second
                                    #
                                    # Remaining levels mostly for code debugging
                                    # 5 adds analog, buf counter, heartbeat, TS
                                    # 6 adds fn calls
                                    # 7 adds more fn calls
                                    # 8 adds strncpy calls

OutputPakFiles      1               # output SR dat files in addition to the
                                    # normal EW tracebuf packets, good for 
                                    # debugging or alternate processing with SR
                                    # utilities such as Pak2Asc
PacketsPerFile     650              # packets in each .PAK file, will be adjusted
                                    # if less than sampling rate/2


#
# Basic Earthworm parameters:
#

MyModuleId         MOD_ADSEND_C     # module id for this instance of srusb2ew
RingName           WAVE_RING        # shared memory ring for outputting acquired data
LogFile            1                # 0 to turn off disk log file; 1 to turn it on
HeartBeatInterval  60               # seconds between heartbeats, only checked
                                    # after each second of data is acquired
#
# Optional Earthworm parameters:
#

OutputMsgType   TYPE_TRACEBUF2      # Older style waveforms were identified by
                                    # Station/Channel/Network (SCN), the newer
                                    # style also includes Location.  For Earthworm 
                                    # v7.0 and greater use, TYPE_TRACEBUF2.  If  
                                    # you need the older style, use TYPE_TRACEBUF.

#
# Basic USBxCH parameters:
#

AtodDriverName     SrUsbXch0        # name of USBxCH kernel mode device driver installed
AtodModelName      USB4CH           # which USBxCH is being used: USB4CH

SamplingRate       130.0            # requested sampling rate.   the actual rate may be 
                                    # different from this due to hardware considerations.
                                    # currently the only allowed rates are:
                                    # 19, 32, 39, 65, 78, 130, or 650 Hz
                                    # Note: All USBxCH **MUST** use same sampling rate

#
# Special USBxCH parameters when running with 2 USB4CH in one PC:
# These must be commented out when using the normal 1 USBxCH.
#

#AtodDriverNameN    SrUsbXch1        # name of 2nd USBxCH kernel mode device driver installed
#AtodModelNameN     USB4CH           # which 2nd USBxCH is being used: USB4CH



#
# Basic USBxCH GPS parameters:
#

GpsModelName       GARMIN           # usually GARMIN or TRIMBLE, may be
                                    # TCXO for nsample*sample rate or
                                    # PCTIME for NTP time 

                                    # NOTE: Windows OS only supplies time
                                    # accurate to 10 ms so PCTIME option is
                                    # not reliable, use TCXO instead

#
# Optional logging parameters:
#
SummaryInterval   1200              # seconds in log file between summary info
                                    # use 0 for no reporting
GpsReportInterval 3600              # seconds in log file between GPS lock lines
                                    # use 0 for no reporting
GpsBadLimit        300              # number of bad locks allowed per interval
                                    # before error message is sent



#
# Channel info: SCNL AND PIN VALUES FOR EACH A/D CHANNEL
#
# EwChannelScnl lines relate the USBxCH channel numbers to the SCNL and pin
# numbers used to identify channels in an earthworm system.  The pin
# numbers are optional.  If a pin number is not specified for a channel, 
# the pin number is set to the A/D channel number.
#
# Three different versions of this section are given below showing how
# the setup would look when using the USB4CH or USB8CH.  Only the
# version corresponding to the USBxCH you are actually using should
# be commented in, the other two should be commented out.
#
# The older keyword EwChannel is the same, except it has no location value.
# If OutputMsgType = TYPE_TRACEBUF2 and EwChannel is used, the location field
# defaults to "--" which means a blank location.
#

#
# Sample EwChannelScnl section for the USB4CH
#

#              A/D      
#            Channel   Sta   Comp  Net  Loc  Pin
#            -------   ---   ----  ---  ---  ---
#EwChannelScnl   0      CH00  xxx   SR   U4    0
#EwChannelScnl   1      CH01  xxx   SR   U4    1
#EwChannelScnl   2      CH02  xxx   SR   U4    2
#EwChannelScnl   3      CH03  xxx   SR   U4    3
#EwChannelScnl   4      DGTL  xxx   SR   U4    4
#EwChannelScnl   5      PPS   xxx   SR   U4    5
#EwChannelScnl   6      PWR   xxx   SR   U4    6
#EwChannelScnl   7      TEMP  xxx   SR   U4    7



#
# Sample EwChannelScnl section for the USB8CH
#

#              A/D      
#            Channel   Sta   Comp  Net  Loc  Pin
#            -------   ---   ----  ---  ---  ---
#EwChannelScnl   0      CH00  xxx   SR   U8    0
#EwChannelScnl   1      CH01  xxx   SR   U8    1
#EwChannelScnl   2      CH02  xxx   SR   U8    2
#EwChannelScnl   3      CH03  xxx   SR   U8    3
#EwChannelScnl   4      CH04  xxx   SR   U8    4
#EwChannelScnl   5      CH05  xxx   SR   U8    5
#EwChannelScnl   6      CH06  xxx   SR   U8    6
#EwChannelScnl   7      CH07  xxx   SR   U8    7
#EwChannelScnl   8      DGTL  xxx   SR   U8    8
#EwChannelScnl   9      PPS   xxx   SR   U8    9
#EwChannelScnl  10      PWR   xxx   SR   U8   10
#EwChannelScnl  11      TEMP  xxx   SR   U8   11



#
# Actual EwChannelScnl section being used ...
# replace with appropriate values for your installation
#

#              A/D      
#            Channel   Sta   Comp  Net  Loc  Pin
#            -------   ---   ----  ---  ---  ---
EwChannelScnl   0      CH00  SHZ   SR   U4    0
EwChannelScnl   1      CH01  SHZ   SR   U4    1
EwChannelScnl   2      CH02  SHE   SR   U4    2
EwChannelScnl   3      CH03  SHN   SR   U4    3
EwChannelScnl   4      DGTL  DIG   SR   U4    4
EwChannelScnl   5      PPS   GPS   SR   U4    5
EwChannelScnl   6      PWR   PWR   SR   U4    6
EwChannelScnl   7      TEMP  DEG   SR   U4    7
