#
#                    CONFIGURATION FILE FOR makeTrace
#                    --------------------------------
#
#	Produce dummy trace data messages. Cloned from adsend
#
#
ModuleId         MOD_ADSEND_A   # Module id of this instance of adsend
OutRing          WAVE_RING      # Transport ring to write waveforms to

#
ChanRate         100.0          # Sampling rate in samples/second
ChanMsgSize      100            # TraceBuf message size, in samples
Nchan			500				# Number of channels to produce
Debug			1				# 1=> yes, n => no debug output

#

