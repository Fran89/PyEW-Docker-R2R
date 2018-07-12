#  1999/07/28
#  The working copy of earthworm.d should live in your EW_PARAMS directory.
#
#  An example copy of earthworm.d resides in the vX.XX/environment
#  directory of this Earthworm distribution.

#                       earthworm.d

#              Earthworm administrative setup:
#              Installation-specific info on
#                    shared memory rings
#                    module ids
#                    message types

#   Please read all comments before making changes to this file.
#   The character string <-> numerical value mapping for certain
#   module id's and message types are sacred to earthworm.d
#   and must not be changed!!!

#--------------------------------------------------------------------------
#                      Shared Memory Ring Keys
#
# Define unique keys for shared memory regions (transport rings).
# All string/value mappings for shared memory keys may be locally altered.
#
# The maximum length of ring string is 32 characters.
#--------------------------------------------------------------------------
     
 Ring   WAVE_RING        1000    # public waveform data
 Ring   PICK_RING        1005    # public parametric data
 Ring   HYPO_RING        1015    # public hypocenters etc.
 Ring   WAVE_RING2      1020    # private buffer for binder_ew
 Ring   EQALARM_EW_RING  1025    # private buffer for eqalam_ew
 Ring   DRINK_RING       1030    # DST drink messages
 Ring   EEW_RING         1035    # A/D waveform ring
 Ring   WAVE_RING_Z       1036    # private buffer for cubic_msg
 Ring   WAVE_RING_N       1037
 
 Ring   EXPPT_RING_BB      1041    # DST drink messages
 Ring   EXPPT_RING_BH      1042    # DST drink messages
 Ring   EXPPT_RING_ST      1043    # DST drink messages  
 Ring   PICK_RING_b		   1044

#--------------------------------------------------------------------------
#                           Module IDs
#
#  Define all module name/module id pairs for this installation
#  Except for MOD_WILDCARD, all string/value mappings for module ids
#  may be locally altered. The character strings themselves may also
#  be changed to be more meaningful for your installation.
#
#  0-255 are the only valid module ids.
#
# The maximum length of the module string is 32 characters.
#--------------------------------------------------------------------------

 Module   MOD_WILDCARD        0   # Sacred wildcard value - DO NOT CHANGE!!!
 Module   MOD_ADSEND_A        1
 Module   MOD_ADSEND_B        2
 Module   MOD_ADSEND_C        3
 Module   MOD_PICKER_A        4
 Module   MOD_PICKER_B        5
 Module   MOD_PICK_EW         6
 Module   MOD_COAXTORING_A    7
 Module   MOD_COAXTORING_B    8
 Module   MOD_RINGTOCOAX      9
 Module   MOD_BINDER_EW      10
 Module   MOD_DISKMGR        11
 Module   MOD_STATMGR        14
 Module   MOD_REPORT         15
 Module   MOD_STARTSTOP      18
 Module   MOD_STATUS         19
 Module   MOD_NANOBOX        20
 Module   MOD_WAVESERVER     21
 Module   MOD_PAGERFEEDER    23
 Module   MOD_EQPROC         24
 Module   MOD_TANKPLAYER     25
 Module   MOD_EQALARM_EW     26
 Module   MOD_EQPRELIM       27
 Module   MOD_IMPORT_GENERIC 28
 Module   MOD_EXPORT_GENERIC 29
 Module   MOD_GETDST         31
 Module   MOD_LPTRIG_A       32
 Module   MOD_LPTRIG_B       33
 Module   MOD_TRG_ASSOC      34
 Module   MOD_AD_DEMUX_A     35
 Module   MOD_AD_DEMUX_B     36
 Module   MOD_VDL_EW         37
 Module   MOD_CUBIC_MSG      38
 Module   MOD_GAPLIST        39
 Module   MOD_GETTERW        40
 Module   MOD_WAVESERVERV    41
 Module   MOD_NANO2TRACE     42
 Module   MOD_GETDST2        43
 Module   MOD_EXPORT_SCN     44
 Module   MOD_ARC2TRIG       45
 Module   MOD_RCV_EW         48
 Module	  MOD_HELI1	         49
 Module	  MOD_TCPD           50

 Module   MOD_PICK_EEW     70
 Module   MOD_PICK_EW_BH     71
 Module   MOD_PICK_EW_ST     72  
 
 Module   MOD_RINGDUP_SCN_ST 80
 Module   MOD_RINGDUP_SCN_BB 81
 Module   MOD_RINGDUP_SCN_BH 82
 
 Module   MOD_RINGDUP_SCN_Z 	 83
 Module   MOD_RINGDUP_SCN_N 	 84 
 Module    MOD_SLINK2EW    88
 
 Module   MOD_DCSN_DB         90
 Module   MOD_DCSN_XML        91
 Module   MOD_DCSN_SOC        92
 Module   MOD_DCSN_TRG        93
 
 Module	  MOD_CONVERT	     116 
 Module	  MOD_SCN2SCNL	     117 
 Module	  MOD_SCNL2SCN	     118 

 Module	  MOD_RINGDUP_SCN    119
 Module	  MOD_TIDAL	         120
 
 Module	  MOD_IMPORT_GENERIC_231	121
 
  Module	 MOD_EXPORT_GEN_ACTV 122
 Module   MOD_DCSN_DB_XML         123  

#--------------------------------------------------------------------------
#                          Message Types
#
#     !!!  DO NOT USE message types 0 thru 99 in earthworm.d !!!
#
#  Define all message name/message-type pairs for this installation.
#
#  VALID numbers are:
#
# 100-255 Message types 100-255 are defined in each installation's  
#         earthworm.d file, under the control of each Earthworm 
#         installation. These values should be used to label messages
#         which remain internal to an Earthworm system or installation.
#         The character strings themselves should not be changed because 
#         the strings are often hard-coded into the modules.
#         However, the string/value mappings can be locally altered.
#         Any message types for locally-produced code may be defined here.
#              
#
#  OFF-LIMITS numbers:
#
#   0- 99 Message types 0-99 are defined in the file earthworm_global.d.
#         These numbers are reserved by Earthworm Central to label types 
#         of messages which may be exchanged between installations. These 
#         string/value mappings must be global to all Earthworm systems 
#         in order for exchanged messages to be properly interpreted.
#         
# The maximum length of the type string is 32 characters.
#
#--------------------------------------------------------------------------

# Installation-specific message-type mappings (100-255):
 Message  TYPE_SPECTRA       100
 Message  TYPE_QUAKE2K       101
 Message  TYPE_LINK          102
 Message  TYPE_EVENT2K       103
 Message  TYPE_PAGE          104
 Message  TYPE_KILL          105
 Message  TYPE_DSTDRINK      106
 Message  TYPE_RESTART       107
 Message  TYPE_REQSTATUS     108
 Message  TYPE_STATUS        109
 Message  TYPE_EQDELETE      110
 Message  TYPE_EVENT_SCNL    111
 Message  TYPE_RECONFIG	     112
 Message  TYPE_STOP	     113  # stop a child. same as kill, except statmgr
				  # should not restart it
 Message  TYPE_EEW	     		 150  
 Message  Type_EEW_record	     160

#   !!!  DO NOT USE message types 0 thru 99 in earthworm.d !!!
