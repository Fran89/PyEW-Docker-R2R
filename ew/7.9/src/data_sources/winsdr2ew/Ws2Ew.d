#                    CONFIGURATION FILE FOR WS2EW
#                    -----------------------------
ModuleId         MOD_WS2EW_A    # Module id of this instance of Ws2Ew
OutRing          WAVE_RING      # Transport ring to write waveforms to
Host		 10.0.0.52	# Host name or IP address of the system running WinSDR
Port             16064          # Port number to use when connecting to WinSDR 
HeartbeatInt     15             # Heartbeat interval in seconds
#
#                        OPTION PARAMETERS
SocketTimeout	60		# Socket timeout in seconds (default = 60)
RestartWaitTime 60		# Seconds to wait between reconnects (default = 60)
NoDataWaitTime  45              # Seconds to wait before resetting connection
NoHostMessages  1               # If 1 log messages from the host will not be processed (default = 0) 
AdcDataSize     2               # ADC Data trace buffer size. Can be 2 or 4 bytes
#
#                        CHANNEL CONFIGURATION
Nchan		 8		# Number of channels running on the host WinSDR system
ChanRate         100            # Sampling rate of the WinSDR system
#
#
#               SCN VALUES FOR EACH DAQ CHANNEL
# Chan lines must follow the Channel Configuration lines in this file.
#
#     DAQ      Station/
#   Channel    Comp/Net      Send to EW
#   -------    --------     -----------
Chan    0     PSN1 BHZ PN       YES
Chan    1     PSN1 BHN PN       NO
Chan    2     PSN1 BHE PN       YES
Chan    3     PSN1 LHZ PN       YES
Chan    4     PSN1 LHN PN       NO
Chan    5     PSN1 LHE PN       NO
Chan    6     PSN1 SHZ PN       NO
Chan    7     PSN1 SHN PN       YES
