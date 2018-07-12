/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac2ew_misc.h 3536 2009-01-15 22:09:51Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.2  2009/01/13 15:41:27  tim
 *     Removed more k2 references
 *
 *     Revision 1.1  2008/10/21 20:02:53  tim
 *     *** empty log message ***
 *
 *     Revision 1.4  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.3  2003/05/15 00:42:45  lombard
 *     Changed to version 2.31: fixed handling of channel map.
 *
 *     Revision 1.2  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.1  2000/05/04 23:48:22  lombard
 *     Initial revision
 *
 *
 *
 */

#ifndef SAMTAC2EW_MISC_H             /* process file only once */
#define SAMTAC2EW_MISC_H 1

void samtac2mi_status_hb(unsigned char type, short code, char* message );

#endif

