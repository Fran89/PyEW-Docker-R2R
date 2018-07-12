# FILE: srpar2ew.d                  Copyright (c), Symmetric Research, 2004-2010
#
# This is the SrPar2Ew parameter file for acquiring data with a 
# Symmetric Research PARxCH 24 bit A/D board and putting that
# data onto an earthworm ring.
#
# The standard earthworm import_generic/export (or export_scnl) functions
# can be used to transfer PARxCH tracebuf data from a local WAVE_RING to
# a WAVE_RING on another machine.  If you have data from two different
# PARxCH boards arriving at the same ring, you must ensure that each is
# using a different ModuleId.  Otherwise, the two sets of data will 
# appear to be scrambled together which leads to lots of message sequence
# number errors.
#
# If you wish to use MOD_SRPAR2EW for a ModuleId instead of
# MOD_ADSEND_C, you must first define it in your earthworm.d file.
#
#
# You will also need to edit other earthworm .d files in order to
# acquire and work with PARxCH data.  A simple configuration where the
# PARxCH files are acquired, saved, and viewed on one computer would
# require making edits similar to those shown below for the following files:
# 
# startstop.d - Use this file to tell earthworm to run srpar2ew to 
#               acquire data by adding lines like these for Windows:
#
#   Process          "srpar2ew srpar2ew.d"
#   PriorityClass     Normal
#   ThreadPriority    Normal
#   Display           NewConsole
#
#               Or like these for Linux:
#
#   Process          "srpar2ew srpar2ew.d"
#   Class/Priority   RR 5
#
# wave_serverV.d - Use this file to save the acquired PARxCH data into
#                  tanks.  You will need to add lines giving the channel
#                  SCNL information, size, and tank location.  The proper
#                  size is determined by the sampling rate.  Each PARxCH
#                  record will be about 1 second long so the size needed
#                  is (SamplingRate+1)*sizeof(long)+sizeof(tracebuf header).
#                  For example, at 100 sps -> 101*4+64 =  468 
#                  and          at 500 sps -> 501*4+64 = 2068
#                  You may need to increase MaxMsgSize to accomodate the
#                  larger values.  For the PAR1CH at 100 sps one might 
#                  add lines like these.
#
#   Tank    CH00 xxx SR -- 480   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run_symres\data\CH00.tnk
#   Tank    MARK xxx SR -- 480   INST_UNKNOWN  MOD_WILDCARD       1         10000         c:\earthworm\run_symres\data\MARK.tnk
# 
# wave_viewer.d - Use this file to view the acquired PARxCH data.  You 
#                 will need to give the IP address and port for the 
#                 wave_server storing the data.  You may also want to add
#                 group lines identifying the channels to display.
#
#   group PAR1CH_Sites
#          CH00 xxx SR --
#          MARK xxx SR --
#
# A more complicated configuration would involve acquiring the PARxCH
# data on one computer and sending it to another for saving, processing,
# and viewing.  In that case, you would need to edit startstop.d, 
# srpar2ew.d and export_scnl.d, on the acquiring computer.  On the 
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
# Send_scnl       MARK xxx SR --                # send this specific channel
#
#
# Now, on to the srpar2ew.d values ...
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
                                    # 6 adds fn calls, BestCounts, ts_select
                                    # 7 adds time method compare, more fn calls
                                    # 8 adds strncpy calls

#
# Basic Earthworm parameters:
#

MyModuleId         MOD_ADSEND_C     # module id for this instance of SrPar2Ew
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
# Basic PARxCH parameters:
#

AtodDriverName     SrParXch0        # name of PARxCH kernel mode device driver installed
AtodModelName      PAR8CH           # which PARxCH is being used: PAR1CH, PAR4CH, PAR8CH
PortMode           BPP              # protocol used for PC parallel port communications

SamplingRate       100.0            # requested sampling rate - the actual rate may be
                                    # slightly different from this due to hardware
                                    # considerations
                                    # Note: All PARxCH **MUST** use same sampling rate

#
# Special PARxCH parameters when running with 2 PAR8CH in one PC:
# These must be commented out when using the normal 1 PARxCH.
#

AtodDriverNameN    SrParXch1        # name of 2nd PARxCH kernel mode device driver installed
AtodModelNameN     PAR8CH           # which 2nd PARxCH is being used: must be PAR8CH
PortModeN          BPP              # protocol used for 2nd PC parallel port communications


#
# Basic PARGPS parameters:
#

GpsEnable          ON               # ON if using PARGPS, OFF otherwise
GpsDriverName      SrParGps0        # name of PARGPS kernel mode device driver installed
GpsModelName       TRIMBLE          # usually TRIMBLE or GARMIN, use PCTIME for NTP time
GpsSerialPort      1                # COM port being used for serial NMEA messages
                                    # Note: GPS interrupts enabled for 1st PARxCH only


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
# EwChannelScnl lines relate the PARxCH channel numbers to the SCNL and pin
# numbers used to identify channels in an earthworm system.  The pin
# numbers are optional.  If a pin number is not specified for a channel, 
# the pin number is set to the A/D channel number.
#
# Three different versions of this section are given below showing how
# the setup would look when using the PAR1CH, PAR4CH, or PAR8CH.  Only
# the version corresponding to the PARxCH you are actually using should
# be commented in, the other two should be commented out.
#
# The older keyword EwChannel is the same, except it has no location value.
# If OutputMsgType = TYPE_TRACEBUF2 and EwChannel is used, the location field
# defaults to "--" which means a blank location.
#

#
# Sample EwChannelScnl section for the PAR1CH
#

#              A/D      
#            Channel   Sta   Comp  Net  Loc  Pin
#            -------   ---   ----  ---  ---  ---
#EwChannelScnl   0      CH00  xxx   SR   --    0
#EwChannelScnl   1      MARK  xxx   SR   --    1   # Include this when using PARGPS



#
# Sample EwChannelScnl section for the PAR4CH
#

#              A/D      
#            Channel   Sta   Comp  Net  Loc  Pin
#            -------   ---   ----  ---  ---  ---
#EwChannelScnl   0      CH00  xxx   SR   --    0
#EwChannelScnl   1      CH01  xxx   SR   --    1
#EwChannelScnl   2      CH02  xxx   SR   --    2
#EwChannelScnl   3      CH03  xxx   SR   --    3
#EwChannelScnl   4      MARK  xxx   SR   --    4   # Include this when using PARGPS



#
# Sample EwChannelScnl section for the PAR8CH
#

#              A/D      
#            Channel   Sta   Comp  Net  Loc  Pin
#            -------   ---   ----  ---  ---  ---
#EwChannelScnl   0      CH00  xxx   SR   --    0
#EwChannelScnl   1      CH01  xxx   SR   --    1
#EwChannelScnl   2      CH02  xxx   SR   --    2
#EwChannelScnl   3      CH03  xxx   SR   --    3
#EwChannelScnl   4      CH04  xxx   SR   --    4
#EwChannelScnl   5      CH05  xxx   SR   --    5
#EwChannelScnl   6      CH06  xxx   SR   --    6
#EwChannelScnl   7      CH07  xxx   SR   --    7
#EwChannelScnl   8      DGTL  xxx   SR   --    8
#EwChannelScnl   9      MARK  xxx   SR   --    9   # Include this when using PARGPS
#EwChannelScnl  10      CNTR  xxx   SR   --   10   # Include this when using PARGPS



#
# Actual EwChannelScnl section being used ...
# replace with appropriate values for your installation
#

#              A/D      
#            Channel   Sta   Comp  Net  Loc  Pin
#            -------   ---   ----  ---  ---  ---
EwChannelScnl   0      CH00  SHZ   SR   P1    0   # Channels from 1st PAR8CH
EwChannelScnl   1      CH01  SHZ   SR   P1    1
EwChannelScnl   2      CH02  SHZ   SR   P1    2
EwChannelScnl   3      CH03  SHZ   SR   P1    3
EwChannelScnl   4      CH04  SHZ   SR   P1    4
EwChannelScnl   5      CH05  SHZ   SR   P1    5
EwChannelScnl   6      CH06  SHZ   SR   P1    6
EwChannelScnl   7      CH07  SHZ   SR   P1    7
EwChannelScnl   8      DGTL  DIG   SR   P1    8
EwChannelScnl   9      MARK  GPS   SR   P1    9
EwChannelScnl  10      CNTR  GPS   SR   P1   10
EwChannelScnl  11      CH08  SHZ   SR   P2    0   # Channels from 2nd PAR8CH
EwChannelScnl  12      CH09  SHZ   SR   P2    1
EwChannelScnl  13      CH10  SHZ   SR   P2    2
EwChannelScnl  14      CH11  SHZ   SR   P2    3
EwChannelScnl  15      CH12  SHZ   SR   P2    4
EwChannelScnl  16      CH13  SHZ   SR   P2    5
EwChannelScnl  17      CH14  SHZ   SR   P2    6
EwChannelScnl  18      CH15  SHZ   SR   P2    7
EwChannelScnl  19      DGTL  DIG   SR   P2    8
EwChannelScnl  20      MARK  GPS   SR   P2    9
EwChannelScnl  21      CNTR  GPS   SR   P2   10
