
#
#                    hyp71_mgr's Configuration File
#
MyModuleId MOD_EQPROC   # Label outgoing messages with this module id.
                        # This program is part of a mega-module which is
                        # started by eqproc or eqprelim.  All child
                        # processes of eqproc/eqprelim should use the
                        # same module id.

RingName   HYPO_RING    # Write output to this transport ring

LogFile    2            # 0=no log; 1=errors; 2=errors & hypocenters

SourceCode T            # Label summary cards with this character to 
                        # identify them as coming from Earthworm

#HYPO71PC path - REQUIRED
#If the path is correctly set up, only the name of the binary is necessary
#Otherwise, provides the full path to the executable
HYPO71PC_bin "Hypo71PC"

#Station list - REQUIRED
# binder station list format (can be the same)
site_file "hypo_sta.list"

#Velocity model - REQUIRED (at least one layer)
#Same syntax and usage as in binder
#@binder_model.d
lay   0.0     3.5
lay   3.0     6.0
lay   15.0    7.00
lay   30.0    8.00

#P/S velocity ratio - OPTIONNAl (default = 1.78)
psratio 1.76		# Default 1.78

#Hypo71PC Test parameters (MAX 15) - OPTIONNAL
#See Hypo71 Manual for explanations
Test 02  50
Test 03  0.2
Test 04  0.010
Test 05  5
Test 07  -0.87
Test 08  2.00
Test 09  0.0035
Test 10  5
Test 11  999.0

#Hypo71PC "distance taper" parameter (control card) - OPTIONNAL
Xnear	200		# Default 200km - distance at which weight begins to decrease
Xfar	450		# Default 450km - distance at which weight is null

#Hypo71PC trial focal depth (control card) - OPTIONNAL
#List of trial depths (MAX 15) to search before the final run
#Setting this will result in several runs, each time changing the trial depth
#And a final run with trial depth and a-priori location been the depth and location
#Corresponding to the lowest RMS and lowest sqrt(ERH*ERH+ERZ*ERZ) (ie most stable solution)
#If only one value is set, only one run is performed
# Default, only one run, 50km trial focal depth
TrialDepth 	50

#Directory to archive HYPO71PC.INP input files and hypo71PC sreen output
#May be usefull for debugging - OPTIONNAL
#Directory must be created manually
#ArchiveDirectory "hypo71_output"	# Default none, no archiving

#Localmag file to compute local magnitude - OPTIONNAL
#If not set, the magnitude is the duration magnitude computed by Hypo71PC based on coda duration
#If set, localmag magnitude is computed using "standalone" mode of localmag, and the configured localmag.d.
#If localmag doesn't find any local magnitude, duration magnitude is set on the output
#Example localmag .d file is included
#LocalmagFile "localmag.d"

#Turning Debug messages on - OPTIONNAL
#Debug 1

#Test with this hypo2000arc file (turns TestMode on) - OPTIONNAL
#TestMode runs the module reading an hypo2000_arc message in the file Hypo2000_Test_File and exits
#There are no interactions with any RingBuffers, either on input or output
#Everything else behaves normally
#Hypo2000_Test_File "hypo2000_test_1.arc"

