#  configuration file for naqsser

MyModuleId        MOD_NAQSSERSDPT
RingName          WAVE_RING
HeartBeatInt      30
LogFile           1
Debug             1    # 0=no debugging messages
                       # 1=log a few header-type lines per packet.
                       # 2=log header lines plus all data samples per packet.
SocketDebug       0    # 0=off, non-zero=log debug messages (VERBOSE!!!!)

NaqsServer        10.10.1.100 28000        # IP address and Port of NaqsServer
MaxSamplePerMsg   256
DataPath       r:\tidegagedata\            # Path to write disk files
TideGageFile   u:\nostide\TideStation.dat  # file with control information


# Set up list of channels you want to receive from NaqsServer using
# a 'RequestChannel' command for each channel, with these arguments:
#
#   sta       station code you want to receive data from (no wildcards).
#   chan      channel (component) of data you want to receive (no wildcards).
#   net       2-character network code to label this channel with (no wildcards).
#             NaqsServer supplies its own station and component codes to its clients,
#             but not a network code. The sta and chan codes you supply in this file 
#             must match the codes supplied by NaqsServer. The TYPE_TRACEBUF messages 
#             output to the Earthworm system will be labeled with the sta, chan, net 
#             codes listed in this file.
#
#   pinno     pin number to assign to this channel (0-32767)
#
#   delay     short-term-completion time (in seconds) -1s <= s <= 300s.
#             When NaqsServer misses packets from the field, it will wait for the 
#             given amount of time for the gap to be filled by re-transmitted
#             packets before sending data to naqsser. Specifying delay=0 will 
#             guarantee that packets are in chronological order, without
#             waiting for missed data.
# 
#   sendbuf   Buffer flag:
#               0 = do not send buffered packets for these channels.
#               1 = send buffered packets for these channels.  This effectively
#                   moves the start of the data stream several packets into
#                   the past.  
#             WARNING: if naqsser is auto-restarted with sendbuffer=1, 
#               duplicate data could enter the Earthworm system.
#               Therefore, we strongly recommend setting sendbuffer = 0. 
#
#   dtexp     Expected time between samples (in seconds)
#
#   offset    Amount to add to data to bring to reference level
#
#   format    0=NOS output, 1=WCATWC output
#
#   gain      station scaling factor
#   
#   tgabbrev  character string (up to 7 chars) used to identify tide gauge 
#             station.
#
#   NOTE!!! The station order below MUST match the Select Serial Channels
#           section in the log file, if not, data may be logged to the wrong
#           file!!!
#
#                 sta    chan  net  pinno  delay(s)   sendbuf   dtexp   offset format gain  tgabbrev tgabbfile  
RequestChannel    CRAG   VTZ   AT    2004    45          0       15.    0.0      1    1.000  cragC   c:\cragC.abb
RequestChannel    OHAK   VTZ   AT    2005    45          0       15.    0.0      1    1.000  ohakC   c:\ohakC.abb
RequestChannel    SDPT   VTZ   AT    2001    45          0       15.    6.667    0    0.932  sdptC   c:\sdptC.abb    
RequestChannel    AKUT   VTZ   AT    2007    45          0       15.    0.0      1    1.000  akutC   c:\akutC.abb
RequestChannel    CEPE   VTZ   AT    2006    45          0       15.    0.0      1    1.000  amkaC   c:\amkaC.abb
RequestChannel    SMY    VTZ   AT    2003    45          0       15.    0.0      1    1.000  shmyC   c:\shmyC.abb
RequestChannel    SIT    VTZ   AT    2002    45          0       15.    8.000    0    1.334  sitkC   c:\sitkC.abb 
