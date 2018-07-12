#  configuration file for naqssoh

MyModuleId       MOD_NAQSSOH
RingName         WAVE_RING
HeartBeatInt     30
LogFile          1
NaqsServer       192.168.1.1 28000   # IP address and Port of NaqsServer

Debug            0     # 0=no debugging messages
                       # 1=log a few header-type lines per packet.
                       # 2=log header lines plus all data samples per packet.
                       # 3=add debug logging from packet-reading level code.
                       # 4=add debug logging from bundle-reading level code.

SocketDebug      0  

LogGPSLocation   0     # If non-zero, write all GPS locations to logfile;
                       #   also log running average of GPS locations.
                       # If zero, do not write locations to logfile.

LogSOHInterval   10    # Minutes (int) between writing SOH values to logfile.
                       # Only those SOH channels being monitored will be logged.
                       # If zero, SOH values will not be logged at regular 
                       # interval.

SNW_Interval     10    # Minutes (int) between writing TYPE_SNW msgs to RingName.
                       # Only GPS/PLL status and those SOH channels being monitored 
                       # will be written to TYPE_SNW msgs for SeisNetWatch. 
                       # If zero, no TYPE_SNW msgs will be generated. 

SNW_ReplaceStaWithName 0  # Flag to control how TYPE_SNW msg is written:
                          # If zero,     it begins with "net-sta" (default)
                          # If non-zero, it begins with "net-name"
                          # This command is optional!

ReportOutOfRange 15.0  # minutes (float). Must be >= 0.0.
                       # An SOH value must be out-of-range for at least
                       # this many minutes before an error message is issued.
                       # This guards against repeated bad/good msgs if the
                       # SOH value is toggling around border value.
                       # Set to 0 if you want an error generated on the
                       # first value that's out of range.
    
ReportDead       60.0  # minutes (float). Must be > 0.0.
                       # If no new SOH packet is received for a given station
                       # in this many minutes, an error message is issued
                       # declaring that station dead.  If new SOH data is
                       # later received, an "unerror" message will be 
                       # issued, declaring that station alive again.

ReportClockBad   24.0  # hours (float). Must be > 0.0.
                       # An HRD's clock is considered good when:
                       #   PLLstatus = fine lock or coarse lock, and 
                       #   GPSstatus = 3D or 2D navigation.
                       # If those conditions are not met for over ReportClockBad
                       # hours, an error message is issued warning that
                       # GPS is unlocked and the clock is free-running.
                       # If the above PLL and GPS status conditions are met
                       # again later, an "unerror" message will be issued,
                       # declaring that GPS is locked and the clock is synced.


# Set up list of stations you want to receive from NaqsServer using
# a 'RequestSOH' command for each station, with these arguments:
#
#   net      network code to associate with the station (no wildcards)
#
#   sta      station code you want to receive SOH from (no wildcards).
#
#   name     up to 30 character name for the station. This name is written
#            to error messages for informational purposes only.  To include
#            blank spaces in name, enclose it in double quotes ("a name").
#            To make it blank, use "".
#
#   delay    short-term-completion time (in seconds) -1s <= s <= 300s.
#            When NaqsServer misses packets from the field, it will wait for the 
#            given amount of time for the gap to be filled by re-transmitted
#            packets before sending data to naqs2ew. Specifying delay=0 will 
#            guarantee that packets are in chronological order, without
#            waiting for missed data.
#
#   buf      Send Buffer flag:
#               0 = do not send buffered packets for these channels.
#               1 = send buffered packets for these channels.  This effectively
#                   moves the start of the data stream several packets into
#                   the past.  
#            WARNING: if naqssoh is auto-restarted with sendbuffer=1, 
#               duplicate data could enter the Earthworm system.
#               Therefore, we strongly recommend setting sendbuffer = 0. 
#
#   nsoh     Number of SOH fields to monitor for this station.  
#            For each SOH field you want to monitor, you must specify 4 additional 
#            fields on this line:  type, label, minimum and maximum.
# 
#   type[i]  Short character string specifying the type of SOH field 
#            to monitor. Valid strings are:            
#              HV    HRD   Voltage  
#              HT    HRD   Temperature
#              HSS1  HRD   Slow External SOH1 (n/a on Lynx HRD)
#              HSS2  HRD   Slow External SOH2 (n/a on Lynx HRD)
#              HSS3  HRD   Slow External SOH3 (n/a on Lynx HRD)
#              HFS1  HRD   Fast External SOH1
#              HFS2  HRD   Fast External SOH2
#              HFS3  HRD   Fast External SOH3
#              LFQE  Libra 10MHz Frequency Error
#              LSST  Libra SSPB Temperature
#              LWWT  Libra WW Temperature (controller)
#              LTXT  Libra TX Temperature (modem)
#              LV    Libra Voltage
#              LES1  Libra External SOH1
#              LES2  Libra External SOH2
#              LES3  Libra External SOH3
#
#   label[i] Longer character string to identify this SOH field with.
#            (up to 10 characters, no blanks allowed) 
#
#   min[i]   Minimum value (floating point) for this SOH field.
#   max[i]   Maximum value (floating point) for this SOH field.
#            Naqssoh will issue an error message if the current value falls
#            outside the configured range.  It will also send an "unerror" 
#            when the value returns to the configured range. 
#            
#
#                                        
#           net sta     name      delay buf nsoh  type label   min max   type label    min max   type label    min max  
#----------------------------------------------------------------------------------------------------------------------
# example HRD:
RequestSOH  NC  KRM   "Red Mtn"     10   0   2      HV voltage 10.5 15.    HT     temp -10  40 
# example Libra:
RequestSOH  NC  CAR05  CarinaA      10   0   3      HV HRDvolt 10.5 15.  LTXT   TXtemp -10  40   LES1  ACstatus 0  2.5
RequestSOH  NC  KSX   "Camp Six"    10   0   3      LV LNXvolt 10.5 15.  LSST SSPBtemp -10 100   LES1 TEGstatus 0  2.5
#
# THE END
