#
#                    CONFIGURATION FILE FOR ADSENDXS
#                    -------------------------------
# Earthworm digitizer using National Instruments X-series DAQ modules
# and Trimble TSIP GPS units.  Uses a 1-PPS pulse from the GPS to trigger
# collection of N samples, using NI Retrigerable Analog Input mode.
# This program replaces the adsend program, which works with an older-
# an older-model NI PCI-6040E board configured for +-2.5V analog signals.
#
ModuleId         MOD_ADSENDXS   # Earthworm module id of this instance of adsendxs
OutRing          IW_RING        # Transport ring to write waveforms to
HeartbeatInt     10             # Seconds between heartbeat messages to statmgr
Debug            0              # If 0, no debug messages.
                                # If 1, log partial GPS status.
                                # If 2, log complete GPS status.
#
#
#                         TIMING INFORMATION
ComPort          1              # The PC com port where time is obtained from
                                #    the Trimble GPS.
BaudRate         38400          # Must be a standard baud rate supported by
                                #    the PC and the GPS, eg 9600, 19200, 38400
ReadFileTimeout  200            # ReadFile times out in this many milliseconds if no data
                                #    are available from the GPS via the com port.
                                #    Recommended value is 200.
TriggerChan      /Dev1/PFI1     # The NI physical channel where the external
                                #    1-PPS trigger is found.  Program tested and works
                                #    correctly with TriggerChan set to /Dev1/PFI1 and
                                #    pin PFI0 jumpered to pin PFI1.
GetGpsDelay      200            # Delay this many milliseconds before the callback
                                #    routine requests GPS time.  Recommended value is 200.
UpdatePcClock    1              # If 1, the PC clock is synched to GPS.
                                # If 0, the PC clock is not synched to GPS.
PcClockUpdInt    86400          # Seconds between PC clock updates from GPS.

# SampRate: For every 1-pps trigger pulse provided by the GPS, SampRate
# samples are collected and written to one tracebuf message.
SampRate         100
#
#
#                         SEISNET-WATCH OUTPUT
SnwReportInterval  3600        # In seconds
StaName            B11TR       # Station name (5 chars max)
NetCode            NC          # Network code (2 chars max)
#
#
#                        CHANNEL CONFIGURATION
# Logical channels listed below are mapped to physical pins on
# an NI PCIe X-series DAQ module.  The "NI X-Series User Manual"
# contains channel mapping details for each X-Series module.
# We use NRSE analog input mode, with AISENSE jumpered to ADGND.
# For an NI PCIe-6343 module pin 29 = AIGND, pin 62 = AISENSE.
#
# Connector 0:
#       Pin    Sta/Comp/Net/Loc   DaqChan
#       ---    ----------------   -------
  Chan   68     1PPS  --- ZZ --   Dev1/ai0
  Chan   33     IRG1A ATT ZZ --   Dev1/ai1
  Chan   65     GUIDa --- ZZ --   Dev1/ai2
  Chan   30     L4TM  EHZ ZZ --   Dev1/ai3
  Chan   28     CMC   EHZ ZZ --   Dev1/ai4
  Chan   60     CSP   EHZ ZZ --   Dev1/ai5
  Chan   25     JBZ   EHZ ZZ --   Dev1/ai6
  Chan   57     JRG   EHZ ZZ --   Dev1/ai7
  Chan   34     JTG   EHZ ZZ --   Dev1/ai8
# Chan   66     CH09  EHZ ZZ --   Dev1/ai9
# Chan   31     CH10  EHZ ZZ --   Dev1/ai10
# Chan   63     CH11  EHZ ZZ --   Dev1/ai11
# Chan   61     CH12  EHZ ZZ --   Dev1/ai12
# Chan   26     CH13  EHZ ZZ --   Dev1/ai13
# Chan   58     CH14  EHZ ZZ --   Dev1/ai14
# Chan   23     CH15  EHZ ZZ --   Dev1/ai15

# Connector 1:
#       Pin    Sta/Comp/Net/Loc   DaqChan
#       ---    ----------------   -------
# Chan   68     CH16  EHZ ZZ --   Dev1/ai16
# Chan   33     CH17  EHZ ZZ --   Dev1/ai17
# Chan   65     CH18  EHZ ZZ --   Dev1/ai18
# Chan   30     CH19  EHZ ZZ --   Dev1/ai19
# Chan   28     CH20  EHZ ZZ --   Dev1/ai20
# Chan   60     CH21  EHZ ZZ --   Dev1/ai21
# Chan   25     CH22  EHZ ZZ --   Dev1/ai22
# Chan   57     CH23  EHZ ZZ --   Dev1/ai23
# Chan   34     CH24  EHZ ZZ --   Dev1/ai24
# Chan   66     CH25  EHZ ZZ --   Dev1/ai25
# Chan   31     CH26  EHZ ZZ --   Dev1/ai26
# Chan   63     CH27  EHZ ZZ --   Dev1/ai27
# Chan   61     CH28  EHZ ZZ --   Dev1/ai28
# Chan   26     CH29  EHZ ZZ --   Dev1/ai29
# Chan   58     CH30  EHZ ZZ --   Dev1/ai30
# Chan   23     CH31  EHZ ZZ --   Dev1/ai31
