/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac2ew_errs.h 3536 2009-01-15 22:09:51Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.4  2009/01/13 17:12:53  tim
 *     Clean up source
 *
 *     Revision 1.3  2009/01/12 20:52:32  tim
 *     Removing K2 references
 *
 *     Revision 1.2  2008/10/29 21:11:56  tim
 *     change variable names to SAMTAC
 *
 *     Revision 1.1  2008/10/29 17:25:25  tim
 *     changed name
 *
 *     Revision 1.1  2008/10/29 17:22:41  tim
 *     changed from k2ewerrs.h
 *
 *     Revision 1.1  2008/10/21 14:18:07  tim
 *     added to samtac2ew
 *
 *     Revision 1.3  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.1  2000/05/04 23:48:17  lombard
 *     Initial revision
 *
 */

#ifndef SAMTAC2EWERRS_H           /* process file only once */
#define SAMTAC2EWERRS_H 1         /* indicate file has been processed */

/* Function return values from k2-packet chain of functions */
#define SAMTAC2R_POSITIVE  1      /* positive success */
#define SAMTAC2R_NO_ERROR  0      /* indifferent success */
#define SAMTAC2R_ERROR    -1      /* error occured */
#define SAMTAC2R_TIMEOUT  -2      /* Timout occurred */
#define SAMTAC2R_PAYLOAD_ERR  -3  /* error in payload (after packet header) */

/* Error values in k2ewmain.c during stream packet handling */
#define SAMTAC2ERR_BAD_STMNUM  -4     /* received stream # out of range */
#define SAMTAC2ERR_BAD_DATACNT -5     /* received data count mismatch */

/* error codes from 'k2cirbuf.c': */
#define SAMTAC2ERR_CB_NOTFOUND -6 /* requested entry not found */

#endif

