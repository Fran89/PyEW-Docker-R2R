#ifndef _MISC_H
#define _MISC_H

#define VER_NO "1.2.1 - 2008-10-15"
/* keep track of version notes here */
/*

	Version 0.0.1 - 2002 May 22 - first cut at MSS100
	Version 0.0.2 - 2002 May 29 - added serial I/O
	Version 0.0.3 - 2002 May 31 - fixed trace_buf end time
	Version 0.0.4 - 2002 June 11 - added a buffering queue
	Version 0.0.4a - 2002 June 11 - added BRP statistics 
	Version 0.0.5 - 2002 June 13 - added some packet statistics 
					and changed logit() init
	Version 0.0.6 - 2002 June 17 - NACK limit upped to 8 for sockets
	Version 0.0.7 - 2002 July 24 - changed logit_init() to use .d file instead of 
					the program name
	Version 0.0.8 - 2002 July 31 - some statistics kept on blocks read in/missed
					output in logit() call at death of app.
	Version 0.0.9 - 2002 Aug 23 - changed the initial connect() to non-blocking
	Version 0.0.10 - 2002 Aug 23 - moved the heartbeat BEFORE the gcf connection
	Version 0.0.11 - 2003 Feb 25 - added UDP client capability to talk to
					a gcfserv program

	Version 1.0.0 - 2006 May 16 - added InfoSCNL to produce TraceBuf2 type Location code savy packets
					relabelling this as V1.0 since v0.0.11 has been in production without complaint
					for 3 years now.


	Version 1.1.0 - 2006 Sep 4 - added in InjectSOH config item to send SOH packets into EW ring.

	Version 1.1.1 - 2006 Nov 9 - fixed the TRACEBUF2 version specification - it was missing!
  
	Version 1.2.0 - 2007 Jan 12 - fixed for MKIII digitizer
	Version 1.2.1 - 2008 Oct 15 - fixed a NCOMMAND issue that caused problems with config file in Linux
*/

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif /* _MISC_H */
