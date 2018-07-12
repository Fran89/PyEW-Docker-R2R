#  configuration file for naqs2ew

MyModuleId        MOD_NAQS2EW
RingName          WAVE_RING
HeartBeatInt      30
LogFile           1
Debug             0    # 0=no debugging messages
                       # 1=log a few header-type lines per packet.
                       # 2=log header lines plus all data samples per packet.
                       # 3=add debug logging from packet-reading level code.
                       # 4=add debug logging from bundle-reading level code.

SocketDebug       0    # 0=off, non-zero=log debug messages (VERBOSE!!!!)

NaqsServer 192.168.1.1 28000  # IP address and Port of NaqsServer

OutputMsgType TYPE_TRACEBUF2  # choose msg format: TYPE_TRACEBUF or TYPE_TRACEBUF2
                              # If command is omitted, naqs2ew outputs TYPE_TRACEBUF2

RepackageNmx        2  # Flag to control how Nmx packets are repackaged.
                       # 1 = create one EW msg from each Nmx packet. 
                       #   The entire Nmx packet will be converted immediately.
                       #   If the Nmx packet contains more than MaxSamplePerMsg,
                       #   then multiple EW msgs will be created. 
                       # 2 = create 1-second EW msgs (default). 
                       #   If the Nmx packet carries more or less than 1-second   
                       #   of data, some samples will be buffered until the  
                       #   receipt of next Nmx packet for that channel.

MaxSamplePerMsg   200  # each EW message will contain no more than this many
                       # samples. Some messages may contain fewer samples.

TimeJumpTolerance 600  # (in seconds) This term is used to catch packets with
                       # dubious timestamps.  If a time gap is detected in an
                       # incoming data stream, the new packet's timestamp is
                       # compared to the system clock.  If it is later than 
                       # the current system time by more than TimeJumpTolerance
                       # seconds, naqs2ew assumes the packet timestamp is bogus
                       # (it's in the future) and it ignores the entire packet.
                       # NOTE: if you use this feature with small tolerances, 
                       # the PC's system clock must be kept pretty close to 
                       # network time!!!
                       # Set to -1 if you never want to compare packet times
                       # to the system clock.
                       # Valid values are -1 or any number >= 0.0
    
# Set up list of channels you want to receive from NaqsServer using
# a 'RequestChanSCNL' command for each channel, with these arguments:
#
#   sta       station code you want to receive data from (no wildcards).
#   chan      channel (component) of data you want to receive (no wildcards).
#   net       2-character network code to label this channel with (no wildcards).
#             NaqsServer supplies its own station and component codes to its clients,
#             but not a network code. The sta and chan codes you supply in this file 
#             must match the codes supplied by NaqsServer. The TYPE_TRACEBUF messages 
#             output to the Earthworm system will be labeled with the sta, chan, net 
#             codes listed in this file.
#   loc       2-character location code to label this channel with (no wildcards). 
#             Valid characters are 0-9 and A-Z (upper case). Use "--" to denote a 
#             blank location code. If using the original 'RequestChannel' command, 
#             omit the 'loc' argument and the channel's location code will default 
#             to "--", Earthworm's default blank location code.
#
#   pinno     pin number to assign to this channel (0-32767)
#
#   delay     short-term-completion time (in seconds) -1s <= s <= 300s.
#             When NaqsServer misses packets from the field, it will wait for the 
#             given amount of time for the gap to be filled by re-transmitted
#             packets before sending data to naqs2ew. Specifying delay=0 will 
#             guarantee that packets are in chronological order, without
#             waiting for missed data. Specifying delay=-1 will tell NaqsServer
#             to send all packets to naqs2ew in the order they were received 
#             with no delay (causing out-of-chronological order data and possibly
#             duplicate data to be output into the Earthworm system).
#
#   format    format in which you want to receive the data:
#              -1 = compressed packets (raw format from HRD).
#               0 = uncompressed packets, original sample rate.
#             0<r = requested output sample rate.  NaqsServer will accomodate.
# 
#   sendbuf   Buffer flag:
#               0 = do not send buffered packets for these channels.
#               1 = send buffered packets for these channels.  This effectively
#                   moves the start of the data stream several packets into
#                   the past.  
#             WARNING: if naqs2ew is auto-restarted with sendbuffer=1, 
#               duplicate data could enter the Earthworm system.
#               Therefore, we strongly recommend setting sendbuffer = 0. 
#
#                 sta  chan net loc  pinno  delay(s) format  sendbuf
RequestChanSCNL   CSU1  EHZ  NC  01   2001    30       0        0
RequestChanSCNL   CSU1  EHN  NC  01   2002    30       0        0
RequestChanSCNL   CSU1  EHE  NC  01   2003    30       0        0
RequestChanSCNL   CSU1  HNZ  NC  01   2004    30       0        0
RequestChanSCNL   CSU1  HNN  NC  01   2005    30       0        0
RequestChanSCNL   CSU1  HNE  NC  01   2006    30       0        0
#
RequestChanSCNL   CCH1  EHZ  NC  01   2007    30       0        0
RequestChanSCNL   CCH1  EHN  NC  01   2008    30       0        0
RequestChanSCNL   CCH1  EHE  NC  01   2009    30       0        0
RequestChanSCNL   CCH1  HNZ  NC  01   2010    30       0        0
RequestChanSCNL   CCH1  HNN  NC  01   2011    30       0        0
RequestChanSCNL   CCH1  HNE  NC  01   2012    30       0        0
