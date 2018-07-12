#
#                    CONFIGURATION FILE FOR SaraAdSend
#                    -----------------------------
ModuleId         MOD_SARAADSEND_A	# Module id of this instance of SaraAdSend
#AdcBoardType    1			# 0 = 24 bit, 1 = 16 bit
AdcBoardType     0			# 0 = 24 bit, 1 = 16 bit
OutRing          WAVE_RING      	# Transport ring to write waveforms to
CommPort         5              	# Comm Port Number
PortSpeed        38400          	# Comm Port Baud Rate
HeartbeatInt     15             	# Heartbeat interval in seconds
#
#                        CHANNEL CONFIGURATION
Nchan		 3			# specify number of channels that will be recorded
#Nchan		 4			# specify number of channels that will be recorded
ChanRate         50            		# Sampling rate in samples/second
#
#                         TIMING INFORMATION
TimeRefType      0			# 0 = Use PC, 1 = Use external ref.
TimeSyncError   100			# PC ref. only. The error in milliseconds before resync 
GmtCorrection	0			# This correction value is sent to the A/D board at startup
#
AdcDataSize      4                      # ADC Data trace buffer size. Can be 2 or 4 bytes
#
#               SCN AND PIN VALUES FOR EACH DAQ CHANNEL
# Chan lines must follow the Channel Configuration lines in this file.
# Unused channels may be omitted from the list.  Pin numbers are optional.
# If a pin number is not specified for a channel, the pin number is set to
# the DAQ channel number.
#
#     Chan  Station/Comp/Network/Location
#   -------   --------------
Chan    0     CH00 SHZ NC L1
Chan    1     CH01 SHN NC L1
Chan    2     CH02 SHE NC L1
#Chan    3     CH03 LHZ NC L1
