/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2ewerrs.h 751 2001-08-08 16:11:48Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.1  2000/05/04 23:48:17  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2ewerrs.h:  K2-to-Earthworm error codes -- 1/9/99 -- [ET] */

#ifndef K2EWERRS_H           /* process file only once */
#define K2EWERRS_H 1         /* indicate file has been processed */

/* return codes from 'k2cirbuf.c': */
#define K2R_CB_BUFEMPTY 4 /* circular buffer is empty */
#define K2R_CB_WAITENT  3  /* entry is waiting to be filled */
#define K2R_CB_SKIPENT  2  /* entry is to be skipped */

/* Function return values from k2-packet chain of functions */
#define K2R_POSITIVE  1      /* positive success */
#define K2R_NO_ERROR  0      /* indifferent success */
#define K2R_ERROR    -1      /* error occured */
#define K2R_TIMEOUT  -2      /* Timout occurred */
#define K2R_PAYLOAD_ERR  -3  /* error in payload (after packet header) */

/* Error values in k2ewmain.c during stream packet handling */
#define K2ERR_BAD_STMNUM  -4     /* received stream # out of range */
#define K2ERR_BAD_DATACNT -5     /* received data count mismatch */

/* error codes from 'k2cirbuf.c': */
#define K2ERR_CB_NOTFOUND -6 /* requested entry not found */

const char *k2ew_get_errmsg(int errcd);   /* return msg str for error code */


#endif

