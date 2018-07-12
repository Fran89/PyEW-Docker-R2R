/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: terminat.h 3536 2009-01-15 22:09:51Z tim $
 * 
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *
 */

#ifndef TERMINAT_H
#define TERMINAT_H

/* status/termination codes which map into 'samtac2ew.desc': */
#define SAMTACTERM_UNKNOWN 0          /* unknown termination code */
#define SAMTACTERM_SAMTAC_STARTUP 1       /* Error during SAMTAC configuration/startup */
#define SAMTACTERM_SAMTAC_COMMERR 2       /* Error or timeout reading from SAMTAC */
#define SAMTACTERM_SIG_TRAP 4         /* SIGNAL caused samtac2ew to terminate */
#define SAMTACTERM_EW_PUTMSG 5        /* EW tport_putmsg() failed */
#define SAMTACTERM_EW_TERM 6          /* EW TERMINATE received */
#define SAMTACTERM_NUMENTS 6          /* number of possible termination codes */

#define SAMTACSTAT_OFF_BATT 8         /* External power restored */
#define SAMTACSTAT_LOW_DISK 14        /* low free disk space */
#define SAMTACSTAT_RESTART 20         /* SAMTAC has restarted */
#define SAMTACSTAT_GPSLOCK 21         /* GPS not synched to satellites */
#define SAMTACSTAT_VOLTAGE_ALARM 22   /* Power Supply Voltage off */

/* set code and message for 'statmgr' on exit */
void samtac2ew_enter_exitmsg(int termcode,const char *fmtstr,...);
void samtac2ew_exit(int);        /* exit program; with 'statmgr' msg & cleanup */
void samtac2ew_throw_error();	/* don't exit program, but do throw error in wave_ring */


#endif
