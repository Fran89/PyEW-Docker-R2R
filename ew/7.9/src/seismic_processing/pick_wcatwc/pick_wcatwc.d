
#
#                     Pick_wcatwc's Configuration File
#
MyModId        MOD_PICK_WCATWC # This instance of pick_wcatwc
StaFile        "pick_wcatwc.sta" # File containing stations to be picked
StaDataFile    "station.dat"   # File with information about stations
ResponseFile        "calibs"   # Broadband response file
InRing           WAVE_RING     # Transport ring to find waveform data
OutRing          PICK_RING     # Transport ring to write pick output
AlarmRing       ALARM_RING     # Transport ring to write alarm messages to
HeartbeatInt            30     # Heartbeat interval, in seconds
MaxGap                  10     # Maximum gap to interpolate (in samples)
#    Add file created in ATPlayer if used in conjunction with player (optional)
#    Comment this line if using pick_wcatwc in real-time mode
#ATPLineupFileBB "\DoNotCopyToEB\ATP-BB.sta"    # Station config file
RedoLineupFile           1     # For player - 1->reset lineup file (regional locator=0)
#
#    pick_wcatwc will bandpass filter data prior to picking if the
#    filter flag is set in StaFile.  The following values are used
#    as filter coefficients.  The default values (0.5, 5.) relate to the
#    response coefficients given within pick_wcatwc.  If other values are
#    used, the filter response for those values must be computed and used 
#    instead of the standard response.
#
LowCutFilter           0.5     # In hz, low frequency bandpass filter cutoff
HighCutFilter          5.0     # In hz, high frequency bandpass filter cutoff
#
LTASeconds             15.     # Moving average length of time in seconds
MinFreq            0.33333     # Minimum frequency of interest to picker in hz
# These two parameters were added in June, 2007 which effectively add a 
# fourth stage for the picker.  If the pick is over MinFLoc and less than
# SNLocal strength, then it will rejected and not reported.
MinFLoc                1.5     # Frequency (hz) to separate potential local events
SNLocal                2.0     # Minimum S:N to be used for local events                               
#
#    After P-pick has been made, the following value speicifes the number
#    of half-cycles to examine to determine mb.  The maximum amplitude
#    (and corresponding half-period) within these cycles is used to compute mb.
#
MbCycles                20     # Number 1/2 cycles of P to examine for mb
#
LGSeconds              150     # Number seconds after P to examine Lg for Ml
                               #  (first MbCycles of this time is excluded for
                               #   Ml period and amplitude)
#    Mwp computations are made as described by Tsuboi, Whitmore, and Sokolowski,
#    BSSA, v89, 1345-1351.  The technique has been completely automated here.
#    Results should be examined before issuance to insure correctness.
#
MwpSeconds             200     # Max # seconds to evaluate after P start for Mwp
MwpSigNoise            2.5     # Signal-to-noise ratio necessary for Mwp comps.
#
AlarmOn                  0     # 1->Alarm function enabled, 0->disabled
AlarmTimeout           180.    # Time (seconds) to reset alarm after triggering
AlarmTime               45     # If no data arrives within this many seconds,
                               # send alarm
NeuralNet                0     # 1->Use neural network; 0-> don't
#
TwoStnAlarmOn            1     # 1->multi-station regional alarm on, o->off							   
TwoStnAlarmFile  "alarm.dat"   # Multi-stn alarm config file			   
#
Debug                    1     # If 1, print debugging message

PickOnAcc 0  # if 1, pick on acceleration chans
