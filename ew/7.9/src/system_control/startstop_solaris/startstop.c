
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: startstop.c 2680 2007-02-22 21:03:44Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.12  2007/02/22 21:02:25  stefan
 *     lock changes
 *
 *     Revision 1.11  2006/04/04 18:16:05  stefan
 *     startstop with reconfigure and libraries
 *
 *     Revision 1.10  2005/09/07 20:10:28  friberg
 *     added in restart command to startstop for solaris
 *
 *     Revision 1.9  2003/09/23 00:16:41  kohler
 *     Fixed bug in call to logit_init()
 *
 *
 *     Revision 1.9  2003/09/22  Will Kohler
 *     Program was printing error messages at startup:
 *       Invalid arguments passed in.
 *       Call to get_prog_name failed.
 *       WARNING: Call logit_init before logit.
 *     To fix this problem, I changed the first parameter of logit_init() from
 *     "argv[1]" to "configFile".
 *
 *     Revision 1.8  2002/03/20 17:08:49  patton
 *     Made logit changes.
 *     JMP 03/20/2002
 *
 *     Revision 1.7  2001/07/18 21:35:04  lombard
 *     Removed DOS end-of-line chars that were causing "invalid white space character" errors.
 *
 *     Revision 1.6  2001/07/16 19:35:40  patton
 *     Modified startstop.c to wait a user defined number of seconds after
 *     starting statmgr first if present.  Also modifed earthworm shutdown
 *     so that if startstop fails to start a module, it will print out
 *     useful information, and exit cleanly.
 *
 *     Revision 1.5  2001/05/07 21:46:06  dietz
 *     Changed RestartChild to give child processes a chance to shut down
 *     gracefully before terminating them with a SIG_TERM signal.
 *
 *     Revision 1.4  2000/07/24 21:13:54  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.3  2000/05/26 22:52:50  dietz
 *     Fixed to check for NULL strings in reading config file
 *
 *     Revision 1.2  2000/03/25 01:14:05  dietz
 *     changed to terminate child processes with SIGTERM instead of SIGKILL in RestartChild()
 *
 *     Revision 1.1  2000/02/14 19:38:25  lucky
 *     Initial revision
 *
 *
 */

       /***********************************************************
        *                    Program startstop                    *
        *                                                         *
        *     Program to start and stop the Earthworm system      *
        ***********************************************************/

/* Changed 3/24/00 by LDD: changed signal used to terminate child processes
 * from SIGKILL (which cannot be blocked) to SIGTERM (which can be blocked)
 *
 * Changes:
 * Lombard: 11/19/98: V4.0 changes:
 *   0) no Y2K dates
 *   1) changed argument of logit_init to the config file name.
 *   2) process ID in heartbeat message: not applicable
 *   3) flush input transport ring: not applicable
 *   4) add `restartMe' to .desc file: not applicable
 *   5) multi-threaded logit: not applicable
 *
 * Changed 6/11/98 by PNL: added call to setpgid so that startstop would
 * the process group leader. This will let StopEarthworm() kill its
 * children but not other processes. Requested by Kent Lindquist, UAF.
 *
 * Changed 5/4/98 by KGL: migrated addition of Agent field to parameter
 * file into this version of startstop, so all the modules don't have
 * to run as root. Also added paragraph to allow specification of
 * configuration file name on command line. Added signal handler to
 * exit cleanly on SIGTERM.
 * Added nice() call to restore default nice value.
 *
 * Changed 4/7/98 by PNL: fixed problem when running startstop from a script.
 * fgets() returns ESPIPE instead of EIO.
 *
 * Changed 3/17/98 by LDD: changed Earthworm status request/response to
 * use the transport ring instead of Solaris msgsnd() & msgrcv().
 * Also moved Heartbeat() function from a separate thread back into the
 * main thread.
 *
 * Changed 1/13/98 by LDD: increased MAXLINE, changed parm field of the
 * CHILD struct to "char parm[MAXLINE]" to allow longer process command
 * strings in the startstop config file.
 *
 * Changed 12/2/97 by PNL: status message changed to include the arguments
 * given to each command from the startstop_sol.d file, up to 80 characters
 * so the message will still fit on an 80-column screen.
 *
 * Changed 11/1/97 by PNL: now can be run in background as well as in
 * foreground. This is done by setting SIGTTIN to IGNORE.
 */

#include <startstop_unix_generic.h>
#define DEBUG

int main( int argc, char *argv[] )
{
   int returnval;
   returnval = RunEarthworm( argc, argv ); /* found in startstop_unix_generic.c */
   UnlockStartstop();
   return returnval;
}

