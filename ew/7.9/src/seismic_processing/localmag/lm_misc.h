/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_misc.h 2094 2006-03-10 13:03:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_misc.h: header for lm_misc.c
 */

#ifndef LM_MISC_H
#define LM_MISC_H

#define STA_LEN 6
#define COMP_LEN 8
#define NET_LEN 8
#define LOC_LEN 3

double utmcal(double, double, double, double);
int fmtFilename(char *, char *, char *, char *, char *, int , char *);
int isMatch(char *, char *);
int strappend( char *, int, char * );

#endif
