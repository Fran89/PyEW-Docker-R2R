#include <string.h>
#include <errno.h>
#include <error_ew.h>
  
/****************** GetLastError_ew ***********************
 *     Returns the error code for the most recent error.  *
 **********************************************************/
int GetLastError_ew()
{
  return( errno);
}

/*****************************************************************
 *  ew_fmt_err_msg: builds text for the Unix system error in a   *
 *       somewhat thread-sfae manner by using a buffer provided  *
 *       by the caller.                                          *
 *      error: the error number returned by GetLastError or, for *
 *             socket errors, by WSAGetLastError.                *
 *     retstr is expected to hold at least maxlen bytes          *
 *****************************************************************/
void ew_fmt_err_msg( int error, char *retstr, int maxlen)
{
  if (retstr != NULL && maxlen > 1)
  {
      char *msg = strerror(error);
      strncpy(retstr, msg, maxlen - 1);
      retstr[maxlen - 1] = '\0';  /* Make sure it is terminated */
  }
  return;
}

