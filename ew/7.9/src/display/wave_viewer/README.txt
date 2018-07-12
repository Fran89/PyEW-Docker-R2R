###################
About wave_viewer and this file

Wave_viewer is a program developed primarily to display data
from EARTHWORM wave_servers in a graphical format.

During the Fall of 2001, this infamous WAVE VIEWER program
was overhauled.

A large portion of the guts of the program were rewritten.
The goal was to produce a less finicky program that could
be used to display all types of Earthworm based waveform
data, and could also be more easily maintained by current
programmers.

As part of this overhaul, new documentation was produced,
describing the design of the new wave_viewer, and the
programattic data flow within the program.

Documentation generated consists of:

1) wave_viewer_programmer_comments.txt
This file, describing the implementation of the guts of
wave_viewer.

2) wave_viewer_fd.vsd
A diagram displaying the various components that make up the
core portion of the wave_viewer experience.  The core wave_viewer
classes are depicted along with dataflow between the classes and
external processes.

3) wave_viewer_fd.gif
A GIF formatted image of the wave_viewer_fd.vsd file.

4) wave_viewer.d
A new, well documented sample configuration file for wave_viewer.

5) wave_viewer_user_doc.txt
A brief user doc for wave_viewer

6) wave_viewer_bugs.txt
A list of known bugs for wave_viewer


#######
CHANGES
#######

##################
 v2.01  01/10/2002



Display.h   // added support for ScreenDelay (SetScreenDelay(), dDelayFudgeFactor)

Trace.cpp  // fixed a bug in the trace buffer(cache) wrap around logic
           // that searched for gaps.  The bug prevented the first
           // data value in the buffer from being initialized to 
	   // an INIT value.  This caused an artificial gap break,
	   // and some strange behavior when retrieving trace to
           // fill gaps.

#known bugs, wave_viewer segfaults when shutting down

Display.cpp // Made the DISPLAY_DRAWING_DELAY_FUDGE_FACTOR a
            // configable variable.  The Fudge factor controls
	    // the difference between the right edge of the
	    // screen and real-time(end of tank), when you
	    // tell wave_viewer to go to the right of the tank.
	    // The samller the fudge factor (can even be negative)
	    // the closer that the right edge of the tank is to
            // the right side of the screen.

Wave.cpp    // Made the RequestQueueMaxSize (used for setting the
            // maximum number of data-requests to the wave_server
 	    // that will be queued by the Wave object) a variable.
	    // Added a method to set the variable (SetRequestQueueLen)

Wave.h      // Added prototype for SetRequestQueueLen()

WaveDoc.cpp // Added code to allow the CWave RequestQueue (see Wave.cpp)
            // to be configable.

resource.h  // Allocated another resource constant for who knows what.
            // unimportant.


surfDoc.cpp // Removed a premature return within HandleWSMenuReply()
            // that was affecting code executed during a menu refresh.
            // Now, all menu requests cause the data-request queue 
            // and Trace Blocks to be properly reset whenever the user
	    // requests a new menu (including when they jump to 
            // one end of the tank or the other.  (Go Oldest / Go Newest)
	    // See comments in the code marked with DavidK 20020115

surfView.cpp 
            // Added ScreenDelay config file variable.  (See 
            //  Display.cpp (Fudge Factor).  
            // Fixed a bug in the auto-advance scrolling, where the
            //  scrolling was being done slightly faster than actual time,
            //  because of a rounding error converting time to pixels.  We
            //  now adjust the scroll time based upon the actual time scrolled
            //  instead of the requested time scrolled.
            // Added support for Accelerator Bar (THUMBTRACK) (dragging little
            //  scrollbar box at the bottom of the screen.)  It had been disabled
            //  in v2.00.
	    // Modified HScroll() so that it returns a
            //  double (seconds actually scrolled) instead of void.

surfView.h  // Modified the prototype for HScroll.


 END v2.01 
##################

##################
 v2.10  06/03/2004

band.cpp
  Added the Location Code to the list of items to write out to 
  the header for each band. 

Display.cpp
  Changed the Header Width (header is area to the left of each trace
                            where the SCNL and other info is
                            printed)
  from 60 to 75, to properly accomodate all 4-character station names.

Group.cpp
  Added Loc support for group entries in wave_viewer's config file.
  Group entries without a stated Loc code will be given the default
  "--".

Group.h
  Added a constant for blank location code "STRING__NO_LOC" as "--".
  The same code is also defined in WaveDoc.h.

Phase.h
  Added Loc Code (cLoc) to the attributes of Phase.


Select.cpp
  Added Loc Code to the Select Tree control for viewing quake triggers

Site.h
Site.cpp
  Added Loc Code as unique site attribute

surf.rc
  Updated version to v2.10  06/03/2004

surfDoc.cpp
  Added Loc support to code that parses EW Trig message.
  New code will not work with SCN EW Trig messages, only
  SCNL EW Trig messages.

surfView.cpp
  Added Loc to the status bar message for each channel.

Wave.cpp
  Wholesale changed SCN to SCNL.
  Added iWaveServerType to CWaveDoc, to allow wave_viewer
  to differentiate between an SCN wave_server and an SCNL
  wave_server.  Added code to send server-type specific
  requests, and perform server-type specific parsing of replies.
  (Nothing complicated, just send/parse Loc for SCNL servers,
   and don't for SCN servers).

Wave.h
  Wholesale changed SCN to SCNL.

WaveDoc.h
  Wholesale changed SCN to SCNL.
  -
  Added a constant for blank location code "STRING__NO_LOC" as "--".
  The same code is also defined in Group.h.
  -
  Added WAVE_SERVER_TYPE_xxx  constants to define
  what kind of wave_server wave_viewer is talking to.
  Current useful types include SCN and SCNL.
  -
  Added bWaveServerTypeDefined attribute, to indicate whether
  the wave_server Type has been defined, and iWaveServerType
  to indicate the Type of the wave_server.
  

WaveDoc.cpp
  Wholesale changed SCN to SCNL.
  Added support for Loc code.
  Added code to differentiate between SCN and SCNL
  wave_servers.
  (Currently servers are differentiated by the original
   Menu Reply.  If a Loc code exists in the Menu listing,
   then the server is branded SCNL, otherwise the server
   is branded as SCN.)
  Changed the parsing logic to include Loc code
  parsing for SCNL servers.
  Modified the parsers for Trace Data and Menu Reply.



wave_viewer.d
  Added OPTIONAL Loc code for "Group" config commands.


 END v2.10 
##################

##################
 v2.11  06/10/2004
 Fixed unnoticable bug in WaveDoc.cpp.  Not worth the upgrade.
 END v2.11 
##################



#######
THE END
#######

