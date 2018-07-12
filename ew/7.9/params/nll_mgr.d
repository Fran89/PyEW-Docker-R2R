
#
#                    nll_mgr's Configuration File
#
MyModuleId MOD_EQPRELIM_NLL   # Label outgoing messages with this module id.
# This program is part of a mega-module which is
# started by eqproc or eqprelim.  All child
# processes of eqproc/eqprelim should use the
# same module id.

RingName   HYPO_RING    # Write output to this transport ring

LogFile    2            # 0=no log; 1=errors; 2=errors & hypocenters

SourceCode W            # Label summary cards with this character to 
# identify them as coming from Earthworm


################################################
# 2006/06/16 AJL - Added following

NllCtrlFile /home/anthony/opt/earthworm/nll_test/run/earthworm.in
# NLL control file with full path
# This files must exist.
# (LOCFILES statement will be overwritten by nll_mgr)

# Norther California
NllStaFile  /home/anthony/opt/earthworm/nll_test/run/NC.stations.EHZ.nll.txt	
# INGV
#NllStaFile  /home/anthony/opt/earthworm/nll_test/run/sta_list_ingv.in           
# NLL file containing station coordinates in NLL GTSRCE format
# This file must exist.
# Use an existing, empty file if station GTSRCE statements 
# are inside NllCtrlFile

NllTimePath /temp/nlloc_tmp/taup/ak135/ak135     # NLL travel-time grid files path/root
# These files must exist.

NllTimeSwap 1                                    # NLL LOCFILES iSwapBytesOnInput value
# (0 = NO, 1 = YES) flag to indicate if hi and low 
# bytes of input time grid files should be swapped. Allows reading of
# travel time grids from different computer architecture platforms 
# during TRANS GLOBAL mode location.

NllOutPath  /temp/nlloc_tmp/earthworm/nll_mgr0   # NLL output file path without NLL root
# This directory must exist.
# IMPORTANT! - this path should be different from
# the path used by any other
# instances of modules containing nll_mgr.
# NLL output to this path will only be overwritten
# by output from subsequent events with same output name 
# (i.e. same first phase time).

