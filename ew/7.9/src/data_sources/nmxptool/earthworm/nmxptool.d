#
#                     Configuration File for nmxptool
#
MyModuleId           MOD_NMXPTOOL
RingName             WAVE_RING           # Transport ring to write data to.

HeartBeatInterval    10                  # Heartbeat interval, in seconds.
LogFile              1                   # 1 -> Keep log, 0 -> no log file
                                         # 2 -> write to module log but not stderr/stdout

#ForceTraceBuf1      0                   # On systems that support TRACEBUF2
                                         # messages this flag will force the module
                                         # to create TRACEBUF messages instead.
                                         # Most people will never need this.

Verbosity            16                  # Set level of verbosity. Verbosity is a bitmap:
                                         # 1 Channel State, 2 Channel, 4 Raw Stream,
                                         # 8 CRC32, 16 Connection flow,
                                         # 32 Packet Management, 64 Extra, 128 Date,
                                         # 256 Gap, 512 DOD, 1023 All messages.
                                         # It is equivalent to the option -v.

NmxpHost             naqs1a.int.ingv.it  # NaqsServer/DataServer hostname or IP address.
                                         # It is equivalent to the option -H.

NmxpPortPDS          28000               # Port number of NaqsServer (Default 28000)
                                         # It is equivalent to the option -P.

NmxpPortDAP          28002               # Port number of DataServer(Default 28002)
                                         # It is equivalent to the option -D.
#UserDAP              mtheo              # DataServer user name. Commented if 'none'.
                                         # It is equivalent to the option -u.
#PassDAP              mypass             # DataServer password. Commented if 'none'.
                                         # It is equivalent to the option -p.

#ShortTermCompletion  60                  # ShortTermCompletion, NOT use 'MaxTolerableLatency'.
                                         #  0 decompressed packets are received in chronological
                                         #    order without waiting for missing packets.
                                         # [1..300] decompressed packets are received in
                                         #    chronological order but waiting for missing packets
                                         #    at most SECs seconds.

MaxTolerableLatency  60                  # Raw Stream, NOT use 'ShortTermCompletion'.
                                         # Max tolerable latency for each channel.
                                         # (Default 600 sec.) [60..600].
                                         # Enable NaqsServer to send out retransmission requests
                                         # for missed packets. Inside the section NetworkInterface
                                         # of the file Naqs.ini set RetxRequest to Enabled.
                                         # If RetxRequest is not enabled then MaxTolerableLatency is ineffective.
                                         # In general, DO NOT use with parameter TimeoutRecv.
                                         # It is equivalent to the option -M.

#TimeoutRecv          30                 # Time-out in seconds for flushing queued data of each channel.
                                         # It sets mschan to 0/0 ((Default 0. No time-out) [10..300].
                                         # Useful for retrieving Data On Demand with minimum delay.
                                         # 'tsec' in nmxptool.desc should be greater than 'TimeoutRecv'.
                                         # It is equivalent to the option -T.

DefaultNetworkCode   IV                  # Default network code where in 'ChannelFile' or 'Channel' is not declared.
                                         # It is equivalent to the option -N.

# DefaultLocationCode   01               # Default location code where in 'ChannelFile' or 'Channel' is not declared.
                                         # It is equivalent to the option -n. (Default is NULL location)

                                         # N.B. nmxptool channel definition IS NOT equal to SCNL
                                         # It is NSCL, that is NET.STA.CHAN.LOC
                                         # NET  is optional and used only for output.
                                         # STA  can be '*', stands for all stations.
                                         # CHAN can contain '?', stands for any character.
                                         # LOC  is optional and used only for output.
                                         # Related to the parameters 'ChannelFile' and 'Channel'.
                                         # Network and location code will be assigned from the
                                         # first pattern that include station and channel.
                                         # Example: N1.AAA.HH?.01,N2.*.HH?,MMM.BH?.03
                                         # Second pattern includes the first. Unless AAA, all
                                         # stations with HH channels will have network to N2.
                                         # Stations N2.*.HH? will have default location defined by 'DefaultLocationCode'.
                                         # Station MMM will have default network defined by 'DefaultNetworkCode'.

#MaxDataToRetrieve    3600               # Max amount of data of the past to retrieve from the
                                         # DataServer when program restarts (default 0) [0..86400].
                                         # 0 to disable connection to DataServer.
                                         # It is equivalent to the option -A. Related to 'ChannelFile'.
                                         # If 'MaxDataToRetrieve' is zero and 'ChannelFile' is used,
                                         # only data buffered by NaqsServer will be retrieved.
                                         # Rather than using 'MaxDataToRetrieve', it is preferable,
                                         # inside the section Datastream of the file Naqs.ini,
                                         # setting DataBufferLength to a high value.
                                         # 'MaxDataToRetrieve' allows to retrieve much more data of the past
                                         # when the program restarts but it considerably slows down the execution.
                                         # It is extremely harmful when you have many channels,
                                         # in this case you might consider to subdivide the
                                         # channels into different nmxptool instances.

#mschan        280/9                     # mSECs/nC
                                         # mSECs are milliseconds to wait before the next request,
                                         # nC is the number of channels to request at a time.
                                         # Delaying and requesting few channels at a time make
                                         # data buffering on NaqsServer side more efficient.
                                         # Determined empiric values are default 280/9.
                                         # Condition: TotalNumberOfChannels * (mSECs/nC) < 15 sec. 
                                         # Related to -F and -b. 0/0 for disabling.

ChannelFile   /home/ew/nmxptool.list.txt # List of channel patterns, as in 'Channel'. One for each line.
                                         # This file will not be modified by nmxptool.
                                         # Load/Save time of last sample of each channel in a file
                                         # with the same name, same directory, appending suffix ".nmxpstate"
                                         # It enables request of recent packets in order to allow data
                                         # continuity when short disconnections occur or between program restarts.
                                         # Related to 'MaxDataToRetrieve', 
                                         # It is equivalent to the option -F. Related to 'MaxDataToRetrieve'.

    # DO NOT USE parameters 'Channel' and 'ChannelFile' together.
    # 'ChannelFile' is preferable. At restart you can retrieve data of the past
    # from the NaqsServer and optionally from the DataServer, see 'MaxDataToRetrieve'.

# Example of nmxptool channel definition
# Channel              ES.BOB.HH?
# Channel              MN.TIR.HH?
# Channel              MDI.HH?
# Channel              DOI.HH?.01
# Channel              SALO.HH?
# Channel              IV.MONC.HH?.03
# Channel              *.BHZ               # Channel selection

# Please, for other details about parameters, refer to the command line "nmxptool -h"


