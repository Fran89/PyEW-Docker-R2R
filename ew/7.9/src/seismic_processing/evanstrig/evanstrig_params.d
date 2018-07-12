
#
#  evanstrig_params.d
#
#   The parameters in this file are magic numbers
#   from John Evans' Tdetect input file.
#
Drate                100.       # Required input sample rate
Decim                  5.       # Decimation factor (whole number)
#
Ctlta                 10.       # Trigger-channel LTA 1/e time (s)
Ctsta                  1.       # Trigger-channel STA 1/e time (s)
Ctstav               100.       # Trigger-channel STAV 1/e time (s)
Tth1                   2.35     # Normal-event threshold (dB)
Tth2                  11.       # Big-event threshold (dB)
Tminen                 1L       # Trigger-channel minimum energy (counts)
Mndur1                 0.75     # Normal-event minimum duration (s)
Mndur2                 1.40     # Big-event minimum duration (s)
#
Cmlta                 10.       # Masking-channel LTA 1/e time (s)
Cmsta                  1.       # Masking-channel STA 1/e time (s)
Cmstav               100.       # Masking-channel STAV 1/e time (s)
Mth                    9.       # Masking-channel threshold (dB)
Mminen                 1L       # Masking-channel minimum energy (counts)
#
Aset                   0.       # Microseism caution window (s)
Bset                   1.       # Pre-event quiet (s)
Cset                  10.       # Post-event + pre-event quiet (s)
Eset                   3.       # Microseism caution start delay (s)
Sset                 200.       # Settling period (non-triggering) (s)
#
DCcoef                10.       # 1/e time for DC (high-pass) filter (s)
Quiet              1.0e-10      # Used to avoid floating-pt underflow
#
#  Coefficients of the FIR filters used to decimate the data and
#  separate it into trigger and masking bands.  Both sets of
#  coefficients are symmetric (they are linear-phase FIR's), so
#  only half+1 are shown. These values have been multiplied by
#  2^15 so filtering can be done in integer for speed.
#
# The decimation filter follows.
# Dflt_len must be an odd number and must be specified before the
# filter coefficients. The number of filter coefficients must be:
# (Dflt_len + 1) / 2
#
Dflt_len              31        # Filter length
Dflt                 138L       # Filter coefficients
Dflt                2570L
Dflt                 544L
Dflt                 394L
Dflt                 -57L
Dflt                -582L
Dflt               -1031L
Dflt               -1234L
Dflt               -1057L
Dflt                -425L
Dflt                 636L
Dflt                1990L
Dflt                3433L
Dflt                4708L
Dflt                5586L
Dflt                5897L
#
# The filter to separate into trigger and masking bands follows.
# Lflt_len must be an odd number and must be specified before the
# filter coefficients. The number of filter coefficients must be:
# (Lflt_len + 1) / 2
#
Lflt_len              21       # Filter length
Lflt                1240L      # Filter coefficients
Lflt                 461L
Lflt                -113L
Lflt                -964L
Lflt               -1600L
Lflt               -1431L
Lflt                -100L
Lflt                2273L
Lflt                5039L
Lflt                7257L
Lflt                8105L
