/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *   
 *    $Id: error_ew_nt.c 3536 2009-01-15 22:09:51Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *
 */

#include <windows.h>
#include <windows.h>
#include <stdio.h>
#include <error_ew.h>

static char * get_last_WSAerror(int);

/****************** GetLastError_ew ***********************
 *     Returns the error code for the most recent error.  *
 **********************************************************/
int GetLastError_ew()
{
  return( (int) GetLastError());
}


/*****************************************************************
 *  ew_fmt_err_msg: builds text for the Windows or WinSock error *
 *      error: the error number returned by GetLastError or, for *
 *             socket errors, by WSAGetLastError.                *
 *     retstr is expected to hold at least maxlen bytes          *
 *****************************************************************/
void ew_fmt_err_msg( int error, char *retstr, int maxlen)
{
  if (retstr != NULL && maxlen > 1)
  {
    if (error > 0 && error < WSABASEERR)
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0l, 
                    retstr, (unsigned int) (maxlen - 1), NULL);
    else if (error > WSABASEERR)
    {
      char *msg = get_last_WSAerror(error);
      strncpy(retstr, msg, maxlen - 1);
      retstr[maxlen - 1] = '\0';  /* Make sure it is terminated */
    }
    else
      retstr[0] = '\0';
  }
  return;
}


/****************************************************************
 *   get_last_WSAerror: Returns the text corresponding to the   *
 *           WinSock error value return by WSAGetLastError().   *
 *           Since this text is not available from the WinSock  *
 *           library, we fake it, using text from Unix          *
 ****************************************************************/
static char * get_last_WSAerror(int errcd)
{
  static char buff[40];
  
  switch (errcd)
  {
  case WSAEINTR:
    return "Interrupted system call";
    break;
  case WSAEBADF:
    return "Bad file number";
    break;
  case WSAEACCES:
    return "Permission denied";
    break;
  case WSAEFAULT:
    return "Bad address";
    break;
  case WSAEINVAL:
    return "Invalid argument";
    break;
  case WSAEMFILE:
    return "Too many open files";
    break;
  case WSAEWOULDBLOCK:
    return "Operation would block";
    break;
  case WSAEINPROGRESS:
    return "Operation now in progress";
    break;
  case WSAEALREADY:
    return "Operation already in progress";
    break;
  case WSAENOTSOCK:
    return "Socket operation on non-socket";
    break;
  case WSAEDESTADDRREQ:
    return "Destination address required";
    break;
  case WSAEMSGSIZE:
    return "Message too long";
    break;
   case WSAEPROTOTYPE:
    return "Protocol wrong type for socket";
    break;
  case WSAENOPROTOOPT:
    return "Option not supported by protocol";
    break;
  case WSAEPROTONOSUPPORT:
    return "Protocol not supported";
    break;
  case WSAESOCKTNOSUPPORT:
    return "Socket type not supported";
    break;
  case WSAEOPNOTSUPP:
    return "Operation not supported on transport endpoint";
    break;
  case WSAEPFNOSUPPORT:
    return "Protocol family not supported";
    break;
  case WSAEAFNOSUPPORT:
    return "Address family not supported by protocol family";
    break;
  case WSAEADDRINUSE:
    return "Address already in use";
    break;
  case WSAEADDRNOTAVAIL:
    return "Cannot assign requested address";
    break;
  case WSAENETDOWN:
    return "Network is down";
    break;
  case WSAENETUNREACH:
    return "Network is unreachable";
    break;
  case WSAENETRESET:
    return "Network dropped connection because of reset";
    break;
  case WSAECONNABORTED:
    return "Software caused connection abort";
    break;
  case WSAECONNRESET:
    return "Connection reset by peer";
    break;
  case WSAENOBUFS:
    return "No buffer space available";
    break;
  case WSAEISCONN:
    return "Transport endpoint is already connected";
    break;
  case WSAENOTCONN:
    return "Transport endpoint is not connected";
    break;
  case WSAESHUTDOWN:
    return "Cannot send after socket shutdown";
    break;
  case WSAETOOMANYREFS:
    return "Too many references: cannot splice";
    break;
  case WSAETIMEDOUT:
    return "Connection timed out";
    break;
  case WSAECONNREFUSED:
    return "Connection refused";
    break;
  case WSAELOOP:
    return "Number of symbolic links encountered during path name traversal exceeds MAXSYMLINKS";
    break;
  case WSAENAMETOOLONG:
    return "File name too long";
    break;
  case WSAEHOSTDOWN:
    return "Host is down";
    break;
  case WSAEHOSTUNREACH:
    return "No route to host";
    break;
  case WSAENOTEMPTY:
    return "Directory not empty";
    break;
  case WSAEPROCLIM:
    return "Process limit reached";
    break;
  case WSAEUSERS:
    return "Too many users";
    break;
  case WSAEDQUOT:
    return "Disc quota exceeded";
    break;
  case WSAESTALE:
    return "Stale NFS file handle";
    break;
  case WSAEREMOTE:
    return "Object is remote";
    break;
  case WSASYSNOTREADY:
    return "Network system not ready";
    break;
  case WSAVERNOTSUPPORTED:
    return "Requested version of WinSock not supported";
    break;
  case WSANOTINITIALISED:
    return "WinSock system not initialized";
    break;
  case WSAEDISCON:
    return "virtual circuit was gracefully closed by the remote side";
    break;
  case WSAENOMORE:
    return "Np more data";
    break;
  case WSAECANCELLED:
    return "Call cancelled by another thread";
    break;
  case WSAEINVALIDPROCTABLE:
    return "Procedure call table is invalid";
    break;
  case WSAEINVALIDPROVIDER:
    return "Requested service provider is invalid";
    break;
  case WSAEPROVIDERFAILEDINIT:
    return "Requested service provider could not be loaded or initialized";
    break;
  case WSASYSCALLFAILURE:
    return "System call that should never fail has failed";
    break;
  case WSASERVICE_NOT_FOUND:
    return "No such service is known";
    break;
  case WSATYPE_NOT_FOUND:
    return "Protocol wrong type for socket";
    break;
  case WSA_E_NO_MORE:
    return "RM: Not owner";
    break;
  case WSA_E_CANCELLED:
    return "Call cancelled by another thread";
    break;
  case WSAEREFUSED:
    return "Database query failed because it was actively refused";
    break;
  case WSAHOST_NOT_FOUND:
    return "Host not found";
    break;
  case WSATRY_AGAIN:
    return "Host not found, or SERVERFAIL";
    break;
  case WSANO_RECOVERY:
    return "Non-recoverable errors";
    break;
  case WSANO_DATA:
    return "Valid name, no data record of requested type";
    break;
  default:
    sprintf(buff, "Error %d", errcd);
    return buff;
  }
}
