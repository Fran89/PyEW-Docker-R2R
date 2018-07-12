# SCNL Parameters file for the Localmag program
#
# This files list optional parameters that apply to one SCN when processed by 
# localmag. The format is:
# SCNpar sta comp net magCorr ltoff lton hton htoff clipLimit
#  sta, comp, and net are the station, component and network names for a
#     given trace. Full names must be used here; no wildcards are used
#  magCorr is the magnitude correction to be added to this SCN's magnitude
#     to get the local magnitude. 
#     default: 0
#  ltoff, lton are the low-end frequencies (in Hz) for a cosine taper to be 
#     applied to the instrument response function in the frequency domain. 
#     Below frequency ltoff, the taper is 0; above lton, the taper is 1.
#     If lton and ltoff are equal, no taper is applied at the low end.
#     default: no low-end taper is applied
#  hton, htoff are the high-end frequencies (in Hz) for a cosine taper to be 
#     applied to the instrument response function in the frequency domain. 
#     Below frequency hton, the taper is 1; above htoff, the taper is 0.
#     If hton and htoff are equal, no taper is applied at the high end.
#     default: hton is 90% of trace Nyquist freq; htoff is 100% of Nyquist.
#     The trace Nyquist frequency is always one half of the trace sample rate.
#  clipLimit is the value beyond which the raw trace is considered to
#     have been clipped by the signal path.
#     default: 7.55e6:  90% of 2^23; clip for 24-bit digitizer.
# If an SCNL is not listed in the parameter file, then ALL the above defaults
# will apply to it. Each SCNLpar line must list all the values; no "partial
# defaults" apply.

#SCNLpar  sta comp net corr  ltoff lton hton  htoff clip
