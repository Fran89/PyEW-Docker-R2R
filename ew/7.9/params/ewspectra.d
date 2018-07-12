#
#         ewspectra Configuration File 
#
MyModuleId    MOD_EWSPEC # ewspectra's module id

wsTimeout	  1000	# timeout, in seconds, for talking to waveservers

# Waveservers to listen to: IP Port
# Can have up to 20
WaveServer        128.97.24.133 16022

WSDebug       3		# Debugging level of communication w/ waveserver

###
### Specify which SCNLs to process & what operation to perform on them
###

# Produce spectra from a specific SCNL
# If writing to a ring, second SCNL is needed to be used in messages written
PlainSpectraSCNL 5405 HNN NP GE
#PlainSpectraSCNL 5405 HNE NP GN

# Produce spectra from the difference between first SCNL and second
# If writing to a ring, third SCNL is needed to be used in messages written
#DiffSpectraSCNLs		5405 HNE NP DS 5405 HNE NP XW

# Produce spectra from the deconvolution of the second SCNL from the first
# If writing to a ring, third SCNL is needed to be used in messages written
#DeconvolveSpectraSCNLs	5405 HNN NP GE 5405 HNN NP XE 5405 HNN NP S1
#DeconvolveSpectraSCNLs	5405 HNN NP XE 5405 HNN NP GE 5405 HNN NP S2

###
### Specify timespan of data to process
###

# Timespan of data to process: Start Duration
#	Start is either YYYYMMDDHHMMSS or -S (for S seconds before now)
#	Duration is in seconds
#TimeSpan	-3600 29
#TimeSpan	20110329070000 29

TimeSpan	20110418000000 29	# 1st sample
#TimeSpan	20110414010000 2	# 2nd sample (hour later)

#InRing     TEST_RING	# Transport ring to find COMPUTE_SPECTRA messages on

###
### Specify options for pre-processing the timeseries data 
### before computing the SPECTRA
###

# Taper source data: Type Fraction
#		Type is either BARTLETT, HANNING, PARZAN, or BMHARRIS
#		Fraction is how much of each end of data is tapered
Taper	HANNING 0.1

# Butterworth filters: CutoffFreq Poles
#	CutoffFreq is the cutoff frequency
#	Poles is the number of poles used (must be an even number)
#	LowCut = highpass filter (cut frequencies below CutoffFreq)
#	HighCut = lowpass filter (cut frequencies above CutoffFreq)
LowCut 		 0.1	4
HighCut		 2.0	4

###
### Optional smoothing of SPECTRA
###
Smooth	0.1				# Source data smoothing; argument is size of window in secs


###
### Specify data to include in output & where it should go
###

#ReportPeaks	6	0.4	0.8		# Report the top N peaks between the two frequencies
ReportSpectra					# Report the full computed spectra

OutRing    HYPO_RING	# Transport ring to write SPECTRA and/or SPECTRA_PEAK messages to
#OutFile		./ews_out.txt	# File to write spectra and/or peak info to
