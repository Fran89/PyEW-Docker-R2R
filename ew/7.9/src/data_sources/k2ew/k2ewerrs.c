/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2ewerrs.c 90 2000-05-04 23:48:43Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/05/04 23:48:15  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2ewerrs.c:  K2-to-Earthworm error messages -- 1/9/99 -- [ET] */

#include <stdio.h>
#include "k2comif.h"         /* K2 COM port interface routines */
#include "k2ewerrs.h"        /* K2-to-Earthworm error codes */


/**************************************************************************
 * k2ew_get_errmsg:  returns pointer to a static string containing the    *
 *      error message corresponding to the given error code               *
 **************************************************************************/

const char *k2ew_get_errmsg(int errcd)
{
  static char mbuff[128];

  switch(errcd)
  {
    case K2R_NO_ERROR:
      return "No error";

    case K2R_TIMEOUT:
      return "Timeout";

      /* error codes from 'k2cirbuf.c': */
    case K2R_CB_BUFEMPTY:
      return "Circular data buffer is empty";
    case K2R_CB_WAITENT:
      return "Circular data buffer entry is waiting to be filled";
    case K2R_CB_SKIPENT:
      return "Circular data buffer entry is to be skipped";
    case K2ERR_CB_NOTFOUND:
      return "Requested circular buffer entry not found";

    case K2ERR_BAD_STMNUM:
      return "Received logical stream number out of range";
    case K2ERR_BAD_DATACNT:
      return "Received data count (SDS sample rate) mismatch";
  }
  /* no match on error code; build error message with code */
  sprintf(mbuff,"Error code (%d)", errcd);
  return mbuff;         /* return built error message */
}

