/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: error_ew.h 3410 2008-10-21 20:03:24Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2008/10/21 20:03:24  tim
 *     *** empty log message ***
 *
 *     Revision 1.2  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.1  2000/05/04 23:42:17  lombard
 *     Initial revision
 *
 *
 *
 */
/* Include file for Earthworm error-reporting routines */

#ifndef ERROR_EW
#define ERROR_EW

/* Function prototypes: */

/****************** GetLastError_ew ***********************
 *     Returns the error code for the most recent error.  *
 **********************************************************/
int GetLastError_ew();

/*****************************************************************
 *  ew_fmt_err_msg: builds text for the error                    *
 *      error: the error number returned by GetLastError or, for *
 *             socket errors, by WSAGetLastError.                *
 *     retstr is expected to hold at least maxlen bytes          *
 *****************************************************************/
void ew_fmt_err_msg( int error, char *retstr, int maxlen);


#endif
