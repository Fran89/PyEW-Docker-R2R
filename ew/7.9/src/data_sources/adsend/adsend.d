#
#                    CONFIGURATION FILE FOR ADSEND
#                    -----------------------------
#
#  If there are no mux boards in the system, OnboardChans (16) channels are
#  digitized.  If there are mux boards in the system,
#  (4 * OnboardChans * NumMuxBoards) channels are digitized.
#  The station file must contain an entry for each channel digitized.
#
#
ModuleId         MOD_ADSEND     # Module id of this instance of adsend
OutRing          WAVE_RING      # Transport ring to write waveforms to
HeartbeatInt     15             # Heartbeat interval in seconds
EnableBell       1              # If non-zero, ring the computer bell if time
                                #   code is out of synch or the guide channels
                                #   are not locked on.
#
#
#                        CHANNEL CONFIGURATION
OnboardChans     16             # Number of channels on the DAQ card
NumMuxBoards     0              # Number of 64-channel mux's (0,1,2, or 4)
ChanRate         100.0          # Sampling rate in samples/second
ChanMsgSize      100            # TraceBuf message size, in samples
Gain             2              # Channel gains (-1: +/-10V)(1: +/-5V)(2: +/-2.5V)
SampRate         250000.0       # Sample rate/scan (max for PCI-MIO-16E-4 = 250000.)
#Nchan 16                       # Optional: if specified, that many channels will be
                                # digitized. If omitted, all mux's will be acquired.
#
#
#                           GUIDE CHANNELS
NumGuide         1              # Number of guide channels (usually one per mux)
GuideChan        0              # The channels carrying the guide signal
MinGuideSignal   110.0          # Guides are declared dead if the mean value of
                                #   guide 1st differences < MinGuideSignal
MaxGuideNoise    8.0            # Guides are declared dead if standard error of
                                #   guide 1st differences > MaxGuideNoise
TimeForLock      5              # The guide channel system is declared "locked on"
                                #   if all guide channels remain locked on for
                                #   TimeForLock seconds.
ErrNoLockTime    30             # At program startup or after a restart, if the
                                #   guide channel system does not lock on for
                                #   ErrNoLockTime seconds, adsend will report an
                                #   error to statmgr (see adsend.desc).
#
#
#                         TIMING INFORMATION
NumTimeCodeChan  1              # Number of time code channels (must be >0)
TimeCodeChan     1              # Get the time code from these channels
Year             2004           # Current year (reset manually at New Years)
YearFromPC       0              # 1->Ignore "Year" and take from PC clock
#
SendBadTime      1              # NOTE: if set to one, adsend will not attempt to lock
                                # on to the IRIGE time code.
                                # Time stamp will contain PC system clock time.
UpdateSysClock   0              # 1->PC time updated with IRIGE
IrigeIsLocalTime 0              # 1 if Irige represents local time; 0 if GMT time.

TimeoutNoSend    120            # If no data is sent for TimeoutNoSend seconds,
                                # possibly due to bad time code, the program will
                                # switch to the next TimeCodeChan and restart the
                                # DAQ system.  If TimeoutNoSend = 0, this feature
                                # is disabled.

TimeoutNoSynch   1800           # If there is no time-code synch for TimeoutNoSynch
                                # seconds, the program will switch to the next
                                # TimeCodeChan and restart the DAQ system.
                                # If TimeoutNoSynch = 0, this feature is disabled.
#
#
#                         EXTERNAL TRIGGERING
# This program uses "High-Hysteresis Analog Triggering Mode".
# Triggering occurs when the trigger voltage becomes greater than HighValue.
# Detriggering occurs when the trigger voltage becomes less than LowValue.
# HighValue minus LowValue must be >= 0.1 volt.
# For more information on triggering, see the National Instruments "PCI-MIO
# E Series User Manual", Jan 1997 edition, chapter 3.
#
ExtTrig          0              # 0 for internal triggering; 1 for external
LowValue         2.75           # In volts (-10V to 10V) (must be < HighValue)
HighValue        3.25           # In volts (-10V to 10V) (must be > LowValue)

TimeoutNoTrig    5              # If no tracebuf messages are obtained for
                                # TimeoutNoTrigger seconds, the program will
                                # assume the external trigger is disconnected.
                                # The program will switch to internal triggering.
#
#
#               SCNL AND PIN VALUES FOR EACH DAQ CHANNEL
# Chan lines must follow the Channel Configuration lines in this file.
# Unused channels may be omitted from the list.  Pin numbers are optional.
# If a pin number is not specified for a channel, the pin number is set to
# the DAQ channel number.
#
#     DAQ
#   Channel  Sta/Comp/Net/Loc   Pin
#   -------  ----------------   ---
Chan    0     CH00  VHZ NC --     0
Chan    1     CH01  VHZ NC --     1
Chan    2     CH02  VHZ NC --     2
Chan    3     CH03  VHZ NC --     3
Chan    4     CH04  VHZ NC --     4
Chan    5     CH05  VHZ NC --     5
Chan    6     CH06  VHZ NC --     6
Chan    7     CH07  VHZ NC --     7
Chan    8     CH08  VHZ NC --     8
Chan    9     CH09  VHZ NC --     9
Chan   10     CH10  VHZ NC --    10
Chan   11     CH11  VHZ NC --    11
Chan   12     CH12  VHZ NC --    12
Chan   13     CH13  VHZ NC --    13
Chan   14     CH14  VHZ NC --    14
Chan   15     CH15  VHZ NC --    15
