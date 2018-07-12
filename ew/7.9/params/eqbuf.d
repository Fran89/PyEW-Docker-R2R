#
#                 Eqbuf's Configuration File
#
#
MyModuleId  MOD_EQPROC     # Module id used to label the logfile.  Eqbuf is a
                           # descendent of the eqproc program.  All descendents
                           # of eqproc need to use eqproc's module id.
#
RingSize   10              # Buffer will hold RingSize items of size MAX_BYTES_PER_EQ
LogFile     1              # 0=no log; 1=log errors
                           # 2=write to module log but not to stderr/stdout
PipeTo  "eqcoda eqcoda.d"  # The child program to be spawned by eqbuf

