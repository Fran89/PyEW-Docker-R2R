# $Id: grf2ew.d 3142 2007-11-02 21:19:16Z paulf $ 
# Copyright (c) 2000-2007 - DAQ Systems - All rights reserved.
# Configuration file for grf2ew version 1.3.8 or later
# Defaults for each entry are shown in ()

# The logging group is now deprecated as logging in now performed 
# using the Earthworm logging facility.

# GRF server settings...
Server {
    Endpoint          localhost         # GRF server endpoint to connect to
    ReadTimeout       10                # Socket read timeout in seconds (10)
}

# Earthworm settings...
Earthworm {
    Ring              WAVE_RING         # Destination ring for TRACEBUF messages. (WAVE_RING)
    ModuleName        MOD_GRF2EW        # Our module name. (MOD_GRF2EW)
    InstallationID    INST_WILDCARD     # Installation identifier (INST_WILDCARD).  May be entered
                                        #   as a lookup string or decimal value 0-255.  If the 
                                        #   environment variable EW_INSTALLATION is defined, its 
                                        #   contents are used.  This configuration entry 
                                        #   overrides the environment variable setting.
    Heartbeat         10                # Heartbeat interval in seconds. (10)
    CorrectRate       No                # Apply sampling rate corrections?  Yes or (No).
    MinTimeQuality    0                 # Drop data packets with time quality less than this value. 
                                        #   0=Unknown, 1=Bad, (2)=Poor, 3= Good, 4=Very good.
    MessageFormat     TRACE_BUF2        # Output message format: TRACE_BUF or TRACE_BUF2.
}
